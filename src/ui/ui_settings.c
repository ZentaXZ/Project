#include "ui_settings.h"
#include "../control/schedule.h"
#include "../control/process_monitor.h"
#include "../control/installed_apps.h"
#include "../control/usage_limits.h"
#include "../utils/config.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <glib.h>

#define MAX_INSTALLED_APPS 512

typedef struct {
    GtkWidget* reward_spin;
    GtkWidget* sound_switch;
    GtkWidget* process_entry;
    GtkWidget* from_entry;
    GtkWidget* to_entry;
    GtkWidget* rules_listbox;
    GtkWidget* daily_rules_listbox;
    GtkWidget* sessions_listbox;
    GtkWidget* daily_limit_spin;
    GtkWidget* session_minutes_spin;
    GtkWidget* apps_stack;
    GtkWidget* running_apps_box;
    GtkWidget* installed_apps_listbox;
    GtkWidget* installed_search_entry;
    GtkWidget* configured_apps_listbox;
    GtkWidget* selected_label;
    guint live_timer_id;
    InstalledApp installed_apps[MAX_INSTALLED_APPS];
    int installed_apps_count;
    bool installed_apps_loaded;
} SettingsViewContext;

static void ui_settings_refresh_daily_rules_internal(SettingsViewContext* ctx);
static void ui_settings_refresh_sessions_internal(SettingsViewContext* ctx);
static void ui_settings_on_delete_rule_clicked(GtkButton* button, gpointer user_data);

static int ui_settings_parse_time_entry(GtkWidget* entry) {
    const char* text = gtk_editable_get_text(GTK_EDITABLE(entry));
    if (!text) {
        return 0;
    }

    int hour = 0;
    int minute = 0;
    if (sscanf(text, "%d:%d", &hour, &minute) != 2) {
        return 0;
    }

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        return 0;
    }

    return hour * 60 + minute;
}

static void ui_settings_format_duration(int total_seconds, char* out, size_t out_size) {
    if (total_seconds < 0) {
        total_seconds = 0;
    }
    snprintf(out, out_size, "%dm %02ds", total_seconds / 60, total_seconds % 60);
}

static gboolean ui_settings_on_live_tick(gpointer user_data) {
    SettingsViewContext* ctx = (SettingsViewContext*)user_data;
    ui_settings_refresh_daily_rules_internal(ctx);
    ui_settings_refresh_sessions_internal(ctx);
    return G_SOURCE_CONTINUE;
}

void ui_settings_start_live_refresh(GtkWidget* view) {
    SettingsViewContext* ctx = (SettingsViewContext*)g_object_get_data(G_OBJECT(view), "settings-view-ctx");
    if (!ctx || ctx->live_timer_id != 0) {
        return;
    }
    ctx->live_timer_id = g_timeout_add(1000, ui_settings_on_live_tick, ctx);
}

void ui_settings_stop_live_refresh(GtkWidget* view) {
    SettingsViewContext* ctx = (SettingsViewContext*)g_object_get_data(G_OBJECT(view), "settings-view-ctx");
    if (!ctx || ctx->live_timer_id == 0) {
        return;
    }
    g_source_remove(ctx->live_timer_id);
    ctx->live_timer_id = 0;
}

static void ui_settings_select_process(SettingsViewContext* ctx,
                                       const char* process_name,
                                       const char* label_text) {
    if (!process_name || process_name[0] == '\0') {
        return;
    }

    gtk_editable_set_text(GTK_EDITABLE(ctx->process_entry), process_name);

    if (label_text && label_text[0] != '\0') {
        char buffer[320];
        snprintf(buffer, sizeof(buffer), "Seleccionado: %s", label_text);
        gtk_label_set_text(GTK_LABEL(ctx->selected_label), buffer);
    } else {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Seleccionado: %s", process_name);
        gtk_label_set_text(GTK_LABEL(ctx->selected_label), buffer);
    }
}

static void ui_settings_update_selected_label(SettingsViewContext* ctx) {
    const char* process_name = gtk_editable_get_text(GTK_EDITABLE(ctx->process_entry));
    char buffer[128];

    if (process_name && process_name[0] != '\0') {
        snprintf(buffer, sizeof(buffer), "Seleccionado: %s", process_name);
    } else {
        snprintf(buffer, sizeof(buffer), "(ninguno)");
    }

    gtk_label_set_text(GTK_LABEL(ctx->selected_label), buffer);
}

static void ui_settings_clear_container_children(GtkWidget* container, bool is_listbox) {
    if (is_listbox) {
        GtkWidget* child;
        while ((child = gtk_widget_get_first_child(container)) != NULL) {
            gtk_list_box_remove(GTK_LIST_BOX(container), child);
        }
        return;
    }

    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(container)) != NULL) {
        gtk_box_remove(GTK_BOX(container), child);
    }
}

static void ui_settings_on_running_app_clicked(GtkButton* button, gpointer user_data) {
    SettingsViewContext* ctx = (SettingsViewContext*)user_data;
    const char* name = (const char*)g_object_get_data(G_OBJECT(button), "process-name");

    if (name && name[0] != '\0') {
        ui_settings_select_process(ctx, name, name);
    }
    (void)button;
}

static void ui_settings_refresh_running_internal(SettingsViewContext* ctx) {
    ui_settings_clear_container_children(ctx->running_apps_box, false);

    char names[128][64];
    int count = process_monitor_list_running(names, 128);

    if (count == 0) {
        GtkWidget* empty = gtk_label_new("No hay programas visibles en ejecución");
        gtk_widget_set_margin_start(empty, 4);
        gtk_widget_set_margin_end(empty, 4);
        gtk_box_append(GTK_BOX(ctx->running_apps_box), empty);
        return;
    }

    for (int i = 0; i < count; i++) {
        GtkWidget* btn = gtk_button_new_with_label(names[i]);
        g_object_set_data_full(G_OBJECT(btn), "process-name", g_strdup(names[i]), g_free);
        g_signal_connect(btn, "clicked", G_CALLBACK(ui_settings_on_running_app_clicked), ctx);
        gtk_widget_set_margin_start(btn, 2);
        gtk_widget_set_margin_end(btn, 2);
        gtk_box_append(GTK_BOX(ctx->running_apps_box), btn);
    }
}

static bool ui_settings_installed_matches_filter(const InstalledApp* app, const char* filter) {
    if (!filter || filter[0] == '\0') {
        return true;
    }

    if (strstr(app->display_name, filter) != NULL) {
        return true;
    }

    if (app->exe_name[0] != '\0' && g_ascii_strcasecmp(app->exe_name, filter) == 0) {
        return true;
    }

    gchar* display_lower = g_ascii_strdown(app->display_name, -1);
    gchar* filter_lower = g_ascii_strdown(filter, -1);
    bool match = (strstr(display_lower, filter_lower) != NULL);
    g_free(display_lower);
    g_free(filter_lower);
    return match;
}

static void ui_settings_on_installed_row_activated(GtkListBox* listbox, GtkListBoxRow* row, gpointer user_data) {
    SettingsViewContext* ctx = (SettingsViewContext*)user_data;
    (void)listbox;

    const char* process_name = (const char*)g_object_get_data(G_OBJECT(row), "process-name");
    const char* label_text = (const char*)g_object_get_data(G_OBJECT(row), "label-text");
    ui_settings_select_process(ctx, process_name, label_text);
}

static void ui_settings_populate_installed_list(SettingsViewContext* ctx) {
    ui_settings_clear_container_children(ctx->installed_apps_listbox, true);

    const char* filter = gtk_editable_get_text(GTK_EDITABLE(ctx->installed_search_entry));
    int visible = 0;

    for (int i = 0; i < ctx->installed_apps_count; i++) {
        const InstalledApp* app = &ctx->installed_apps[i];
        if (!ui_settings_installed_matches_filter(app, filter)) {
            continue;
        }

        const char* process_name = app->exe_name[0] != '\0' ? app->exe_name : app->display_name;
        char label_text[320];

        if (app->exe_name[0] != '\0') {
            snprintf(label_text, sizeof(label_text), "%s  (%s)", app->display_name, app->exe_name);
        } else {
            snprintf(label_text, sizeof(label_text), "%s  (revisar .exe manualmente)", app->display_name);
        }

        GtkWidget* row = gtk_list_box_row_new();
        GtkWidget* label = gtk_label_new(label_text);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);

        g_object_set_data_full(G_OBJECT(row), "process-name", g_strdup(process_name), g_free);
        g_object_set_data_full(G_OBJECT(row), "label-text", g_strdup(label_text), g_free);

        gtk_list_box_append(GTK_LIST_BOX(ctx->installed_apps_listbox), row);
        visible++;
    }

    if (visible == 0) {
        GtkWidget* row = gtk_list_box_row_new();
        GtkWidget* label = gtk_label_new("No se encontraron programas instalados");
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(GTK_LIST_BOX(ctx->installed_apps_listbox), row);
    }
}

static void ui_settings_refresh_installed_internal(SettingsViewContext* ctx) {
    if (!ctx->installed_apps_loaded) {
        ctx->installed_apps_count = installed_apps_list(ctx->installed_apps, MAX_INSTALLED_APPS);
        ctx->installed_apps_loaded = true;
    }

    ui_settings_populate_installed_list(ctx);
}

static void ui_settings_on_configured_row_activated(GtkListBox* listbox, GtkListBoxRow* row, gpointer user_data) {
    SettingsViewContext* ctx = (SettingsViewContext*)user_data;
    (void)listbox;

    const char* process_name = (const char*)g_object_get_data(G_OBJECT(row), "process-name");
    ui_settings_select_process(ctx, process_name, process_name);
}

static void ui_settings_refresh_configured_internal(SettingsViewContext* ctx) {
    ui_settings_clear_container_children(ctx->configured_apps_listbox, true);

    char seen[128][64];
    int seen_count = 0;

    int block_count = schedule_get_blocklist_count();
    for (int i = 0; i < block_count; i++) {
        AppSchedule rule;
        if (!schedule_get_blocklist_rule(i, &rule)) {
            continue;
        }

        bool already = false;
        for (int j = 0; j < seen_count; j++) {
            if (g_ascii_strcasecmp(seen[j], rule.process_name) == 0) {
                already = true;
                break;
            }
        }
        if (already) {
            continue;
        }

        strncpy(seen[seen_count], rule.process_name, 63);
        seen[seen_count][63] = '\0';
        seen_count++;
    }

    int white_count = schedule_get_whitelist_count();
    for (int i = 0; i < white_count; i++) {
        AppSchedule rule;
        if (!schedule_get_whitelist_rule(i, &rule)) {
            continue;
        }

        bool already = false;
        for (int j = 0; j < seen_count; j++) {
            if (g_ascii_strcasecmp(seen[j], rule.process_name) == 0) {
                already = true;
                break;
            }
        }
        if (already) {
            continue;
        }

        strncpy(seen[seen_count], rule.process_name, 63);
        seen[seen_count][63] = '\0';
        seen_count++;
    }

    int daily_count = usage_limits_get_daily_rule_count();
    for (int i = 0; i < daily_count; i++) {
        DailyLimitRule rule;
        if (!usage_limits_get_daily_rule(i, &rule)) {
            continue;
        }

        bool already = false;
        for (int j = 0; j < seen_count; j++) {
            if (g_ascii_strcasecmp(seen[j], rule.process_name) == 0) {
                already = true;
                break;
            }
        }
        if (already) {
            continue;
        }

        strncpy(seen[seen_count], rule.process_name, 63);
        seen[seen_count][63] = '\0';
        seen_count++;
    }

    if (seen_count == 0) {
        GtkWidget* row = gtk_list_box_row_new();
        GtkWidget* label = gtk_label_new("Todavía no hay programas con reglas definidas");
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(GTK_LIST_BOX(ctx->configured_apps_listbox), row);
        return;
    }

    for (int i = 0; i < seen_count; i++) {
        GtkWidget* row = gtk_list_box_row_new();
        GtkWidget* label = gtk_label_new(seen[i]);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        g_object_set_data_full(G_OBJECT(row), "process-name", g_strdup(seen[i]), g_free);
        gtk_list_box_append(GTK_LIST_BOX(ctx->configured_apps_listbox), row);
    }
}

static void ui_settings_refresh_current_apps_tab(SettingsViewContext* ctx) {
    const char* tab = gtk_stack_get_visible_child_name(GTK_STACK(ctx->apps_stack));

    if (tab && strcmp(tab, "running") == 0) {
        ui_settings_refresh_running_internal(ctx);
    } else if (tab && strcmp(tab, "installed") == 0) {
        ui_settings_refresh_installed_internal(ctx);
    } else if (tab && strcmp(tab, "configured") == 0) {
        ui_settings_refresh_configured_internal(ctx);
    }
}

void ui_settings_refresh_apps(GtkWidget* view) {
    SettingsViewContext* ctx = (SettingsViewContext*)g_object_get_data(G_OBJECT(view), "settings-view-ctx");
    if (ctx) {
        ui_settings_refresh_current_apps_tab(ctx);
    }
}

static void ui_settings_on_apps_stack_changed(GObject* object, GParamSpec* pspec, gpointer user_data) {
    (void)pspec;
    SettingsViewContext* ctx = (SettingsViewContext*)user_data;
    (void)object;
    ui_settings_refresh_current_apps_tab(ctx);
}

static void ui_settings_on_installed_search_changed(GtkEditable* editable, gpointer user_data) {
    SettingsViewContext* ctx = (SettingsViewContext*)user_data;
    (void)editable;
    ui_settings_populate_installed_list(ctx);
}

static void ui_settings_refresh_rules_internal(SettingsViewContext* ctx);
static void ui_settings_refresh_daily_rules_internal(SettingsViewContext* ctx);
static void ui_settings_refresh_sessions_internal(SettingsViewContext* ctx);

static GtkWidget* ui_settings_make_rule_row(const char* line_text,
                                            const char* rule_type,
                                            int rule_index,
                                            SettingsViewContext* ctx) {
    GtkWidget* row = gtk_list_box_row_new();
    GtkWidget* row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* label = gtk_label_new(line_text);
    GtkWidget* delete_button = gtk_button_new_with_label("Eliminar");

    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(label, TRUE);

    g_object_set_data(G_OBJECT(delete_button), "rule-type", (gpointer)rule_type);
    g_object_set_data(G_OBJECT(delete_button), "rule-index", GINT_TO_POINTER(rule_index));
    g_signal_connect(delete_button, "clicked", G_CALLBACK(ui_settings_on_delete_rule_clicked), ctx);

    gtk_box_append(GTK_BOX(row_box), label);
    gtk_box_append(GTK_BOX(row_box), delete_button);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
    return row;
}

static void ui_settings_on_delete_rule_clicked(GtkButton* button, gpointer user_data) {
    SettingsViewContext* ctx = (SettingsViewContext*)user_data;
    const char* rule_type = (const char*)g_object_get_data(G_OBJECT(button), "rule-type");
    int rule_index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "rule-index"));

    if (!rule_type) {
        return;
    }

    if (strcmp(rule_type, "blocklist") == 0) {
        schedule_remove_blocklist_rule(rule_index);
        schedule_save();
    } else if (strcmp(rule_type, "whitelist") == 0) {
        schedule_remove_whitelist_rule(rule_index);
        schedule_save();
    } else if (strcmp(rule_type, "daily") == 0) {
        usage_limits_remove_daily_rule(rule_index);
        usage_limits_save();
    } else if (strcmp(rule_type, "session") == 0) {
        const char* process_name = (const char*)g_object_get_data(G_OBJECT(button), "process-name");
        if (process_name) {
            usage_limits_cancel_session(process_name);
            usage_limits_save();
        }
    }

    ui_settings_refresh_rules_internal(ctx);
    ui_settings_refresh_daily_rules_internal(ctx);
    ui_settings_refresh_sessions_internal(ctx);
    (void)button;
}

static void ui_settings_refresh_rules_internal(SettingsViewContext* ctx) {
    GtkListBox* listbox = GTK_LIST_BOX(ctx->rules_listbox);
    GtkWidget* child;

    while ((child = gtk_widget_get_first_child(ctx->rules_listbox)) != NULL) {
        gtk_list_box_remove(listbox, child);
    }

    int block_count = schedule_get_blocklist_count();
    for (int i = 0; i < block_count; i++) {
        AppSchedule rule;
        if (!schedule_get_blocklist_rule(i, &rule)) {
            continue;
        }

        char from_str[8];
        char to_str[8];
        char line[160];
        schedule_minutes_to_time_string(rule.allowed_from_minutes, from_str, sizeof(from_str));
        schedule_minutes_to_time_string(rule.allowed_to_minutes, to_str, sizeof(to_str));
        snprintf(line, sizeof(line), "[Horario] %s  %s - %s",
                 rule.process_name, from_str, to_str);

        gtk_list_box_append(listbox,
            ui_settings_make_rule_row(line, "blocklist", i, ctx));
    }

    int white_count = schedule_get_whitelist_count();
    for (int i = 0; i < white_count; i++) {
        AppSchedule rule;
        if (!schedule_get_whitelist_rule(i, &rule)) {
            continue;
        }

        char from_str[8];
        char to_str[8];
        char line[160];
        schedule_minutes_to_time_string(rule.allowed_from_minutes, from_str, sizeof(from_str));
        schedule_minutes_to_time_string(rule.allowed_to_minutes, to_str, sizeof(to_str));
        snprintf(line, sizeof(line), "[Whitelist] %s  %s - %s",
                 rule.process_name, from_str, to_str);

        gtk_list_box_append(listbox,
            ui_settings_make_rule_row(line, "whitelist", i, ctx));
    }

    if (block_count == 0 && white_count == 0) {
        GtkWidget* row = gtk_list_box_row_new();
        GtkWidget* label = gtk_label_new("No hay reglas de horario");
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(listbox, row);
    }

    if (gtk_stack_get_visible_child_name(GTK_STACK(ctx->apps_stack)) &&
        strcmp(gtk_stack_get_visible_child_name(GTK_STACK(ctx->apps_stack)), "configured") == 0) {
        ui_settings_refresh_configured_internal(ctx);
    }
}

static void ui_settings_refresh_daily_rules_internal(SettingsViewContext* ctx) {
    GtkListBox* listbox = GTK_LIST_BOX(ctx->daily_rules_listbox);
    GtkWidget* child;

    while ((child = gtk_widget_get_first_child(ctx->daily_rules_listbox)) != NULL) {
        gtk_list_box_remove(listbox, child);
    }

    int count = usage_limits_get_daily_rule_count();
    for (int i = 0; i < count; i++) {
        DailyLimitRule rule;
        if (!usage_limits_get_daily_rule(i, &rule)) {
            continue;
        }

        int used_seconds = usage_limits_get_daily_seconds_used(rule.process_name);
        int remaining_seconds = usage_limits_get_daily_seconds_remaining(rule.process_name);
        bool running = process_monitor_is_running(rule.process_name);
        char used_text[24];
        char remain_text[24];
        char line[280];

        ui_settings_format_duration(used_seconds, used_text, sizeof(used_text));
        ui_settings_format_duration(remaining_seconds, remain_text, sizeof(remain_text));
        snprintf(line, sizeof(line),
                 "%s  ·  %s  ·  límite %d min  ·  usado %s  ·  restan %s",
                 rule.process_name,
                 running ? "EN USO" : "cerrado",
                 rule.max_minutes_per_day,
                 used_text,
                 remain_text);

        gtk_list_box_append(listbox,
            ui_settings_make_rule_row(line, "daily", i, ctx));
    }

    if (count == 0) {
        GtkWidget* row = gtk_list_box_row_new();
        GtkWidget* label = gtk_label_new("No hay límites diarios configurados");
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(listbox, row);
    }
}

static void ui_settings_refresh_sessions_internal(SettingsViewContext* ctx) {
    GtkListBox* listbox = GTK_LIST_BOX(ctx->sessions_listbox);
    GtkWidget* child;

    while ((child = gtk_widget_get_first_child(ctx->sessions_listbox)) != NULL) {
        gtk_list_box_remove(listbox, child);
    }

    int count = usage_limits_get_session_count();
    for (int i = 0; i < count; i++) {
        const char* process_name = NULL;
        time_t ends_at = 0;
        if (!usage_limits_get_session_info(i, &process_name, &ends_at)) {
            continue;
        }

        int seconds_left = usage_limits_get_session_seconds_left(process_name);
        bool running = process_monitor_is_running(process_name);
        char remain_text[24];
        char line[280];

        ui_settings_format_duration(seconds_left, remain_text, sizeof(remain_text));
        if (seconds_left > 0) {
            snprintf(line, sizeof(line), "%s  ·  %s  ·  restan %s",
                     process_name,
                     running ? "EN USO" : "cerrado",
                     remain_text);
        } else {
            snprintf(line, sizeof(line), "%s  ·  sesión vencida",
                     process_name);
        }

        GtkWidget* row = gtk_list_box_row_new();
        GtkWidget* row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget* label = gtk_label_new(line);
        GtkWidget* delete_button = gtk_button_new_with_label("Cancelar");

        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_widget_set_hexpand(label, TRUE);

        g_object_set_data(G_OBJECT(delete_button), "rule-type", "session");
        g_object_set_data_full(G_OBJECT(delete_button), "process-name",
                              g_strdup(process_name), g_free);
        g_signal_connect(delete_button, "clicked", G_CALLBACK(ui_settings_on_delete_rule_clicked), ctx);

        gtk_box_append(GTK_BOX(row_box), label);
        gtk_box_append(GTK_BOX(row_box), delete_button);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
        gtk_list_box_append(listbox, row);
    }

    if (count == 0) {
        GtkWidget* row = gtk_list_box_row_new();
        GtkWidget* label = gtk_label_new("No hay cronómetros de sesión activos");
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(listbox, row);
    }
}

void ui_settings_refresh_rules(GtkWidget* view) {
    SettingsViewContext* ctx = (SettingsViewContext*)g_object_get_data(G_OBJECT(view), "settings-view-ctx");
    if (ctx) {
        ui_settings_refresh_rules_internal(ctx);
        ui_settings_refresh_daily_rules_internal(ctx);
        ui_settings_refresh_sessions_internal(ctx);
    }
}

static void ui_settings_on_refresh_apps_clicked(GtkButton* button, gpointer user_data) {
    SettingsViewContext* ctx = (SettingsViewContext*)user_data;
    const char* tab = gtk_stack_get_visible_child_name(GTK_STACK(ctx->apps_stack));

    if (tab && strcmp(tab, "installed") == 0) {
        ctx->installed_apps_loaded = false;
    }

    ui_settings_refresh_current_apps_tab(ctx);
    (void)button;
}

static void ui_settings_on_add_rule_clicked(GtkButton* button, gpointer user_data) {
    SettingsViewContext* ctx = (SettingsViewContext*)user_data;
    const char* process_name = gtk_editable_get_text(GTK_EDITABLE(ctx->process_entry));
    int from_minutes = ui_settings_parse_time_entry(ctx->from_entry);
    int to_minutes = ui_settings_parse_time_entry(ctx->to_entry);

    if (!process_name || process_name[0] == '\0') {
        return;
    }

    schedule_add_rule(process_name, from_minutes, to_minutes);
    schedule_save();
    gtk_editable_set_text(GTK_EDITABLE(ctx->process_entry), "");
    ui_settings_update_selected_label(ctx);
    ui_settings_refresh_rules_internal(ctx);
    (void)button;
}

static void ui_settings_on_add_daily_limit_clicked(GtkButton* button, gpointer user_data) {
    SettingsViewContext* ctx = (SettingsViewContext*)user_data;
    const char* process_name = gtk_editable_get_text(GTK_EDITABLE(ctx->process_entry));
    int max_minutes = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(ctx->daily_limit_spin));

    if (!process_name || process_name[0] == '\0' || max_minutes < 1) {
        return;
    }

    usage_limits_add_daily_rule(process_name, max_minutes);
    usage_limits_save();
    ui_settings_refresh_daily_rules_internal(ctx);
    (void)button;
}

static void ui_settings_on_start_session_clicked(GtkButton* button, gpointer user_data) {
    SettingsViewContext* ctx = (SettingsViewContext*)user_data;
    const char* process_name = gtk_editable_get_text(GTK_EDITABLE(ctx->process_entry));
    int minutes = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(ctx->session_minutes_spin));

    if (!process_name || process_name[0] == '\0' || minutes < 1) {
        return;
    }

    usage_limits_start_session(process_name, minutes);
    ui_settings_refresh_sessions_internal(ctx);
    (void)button;
}

static void ui_settings_on_save_clicked(GtkButton* button, gpointer user_data) {
    SettingsViewContext* ctx = (SettingsViewContext*)user_data;

    int reward = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(ctx->reward_spin));
    config_set_reward_minutes(reward);
    config_set_sound_enabled(gtk_switch_get_active(GTK_SWITCH(ctx->sound_switch)));

    config_save();
    schedule_save();
    usage_limits_save();
    ui_settings_refresh_rules_internal(ctx);
    ui_settings_refresh_daily_rules_internal(ctx);
    ui_settings_refresh_sessions_internal(ctx);
    (void)button;
}

GtkWidget* ui_settings_build(void) {
    SettingsViewContext* ctx = g_new0(SettingsViewContext, 1);

    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget* general_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget* reward_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* sound_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* reward_label = gtk_label_new("Recompensa (min):");
    GtkAdjustment* reward_adj = gtk_adjustment_new(config_get_reward_minutes(), 0, 120, 1, 5, 0);
    ctx->reward_spin = gtk_spin_button_new(reward_adj, 1, 0);

    GtkWidget* sound_label = gtk_label_new("Sonido:");
    ctx->sound_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(ctx->sound_switch), config_get_sound_enabled());

    GtkWidget* apps_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* apps_title = gtk_label_new("Programa:");
    GtkWidget* refresh_apps_button = gtk_button_new_with_label("Actualizar");
    ctx->selected_label = gtk_label_new("(ninguno)");
    gtk_label_set_xalign(GTK_LABEL(ctx->selected_label), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(ctx->selected_label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_halign(ctx->selected_label, GTK_ALIGN_START);

    ctx->apps_stack = gtk_stack_new();
    GtkWidget* apps_switcher = gtk_stack_switcher_new();
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(apps_switcher), GTK_STACK(ctx->apps_stack));

    GtkWidget* running_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget* running_scrolled = gtk_scrolled_window_new();
    ctx->running_apps_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(running_scrolled), ctx->running_apps_box);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(running_scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_widget_set_size_request(running_scrolled, -1, 40);
    gtk_box_append(GTK_BOX(running_page), running_scrolled);

    GtkWidget* installed_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    ctx->installed_search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->installed_search_entry), "Buscar...");
    GtkWidget* installed_scrolled = gtk_scrolled_window_new();
    ctx->installed_apps_listbox = gtk_list_box_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(installed_scrolled), ctx->installed_apps_listbox);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(installed_scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(installed_scrolled, -1, 100);
    gtk_box_append(GTK_BOX(installed_page), ctx->installed_search_entry);
    gtk_box_append(GTK_BOX(installed_page), installed_scrolled);

    GtkWidget* configured_page = gtk_scrolled_window_new();
    ctx->configured_apps_listbox = gtk_list_box_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(configured_page), ctx->configured_apps_listbox);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(configured_page),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(configured_page, -1, 100);

    gtk_stack_add_titled(GTK_STACK(ctx->apps_stack), running_page, "running", "Ejecución");
    gtk_stack_add_titled(GTK_STACK(ctx->apps_stack), installed_page, "installed", "Instaladas");
    gtk_stack_add_titled(GTK_STACK(ctx->apps_stack), configured_page, "configured", "Configurados");
    gtk_widget_set_vexpand(ctx->apps_stack, TRUE);
    gtk_widget_set_size_request(ctx->apps_stack, -1, 110);

    GtkWidget* limits_stack = gtk_stack_new();
    GtkWidget* limits_switcher = gtk_stack_switcher_new();
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(limits_switcher), GTK_STACK(limits_stack));

    GtkWidget* schedule_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget* add_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    ctx->process_entry = gtk_entry_new();
    ctx->from_entry = gtk_entry_new();
    ctx->to_entry = gtk_entry_new();
    GtkWidget* add_button = gtk_button_new_with_label("Agregar");
    GtkWidget* scrolled = gtk_scrolled_window_new();
    ctx->rules_listbox = gtk_list_box_new();

    GtkWidget* daily_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget* daily_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* daily_label = gtk_label_new("Min/día:");
    GtkAdjustment* daily_adj = gtk_adjustment_new(120, 1, 1440, 5, 15, 0);
    ctx->daily_limit_spin = gtk_spin_button_new(daily_adj, 1, 0);
    GtkWidget* add_daily_button = gtk_button_new_with_label("Agregar");
    GtkWidget* daily_scrolled = gtk_scrolled_window_new();
    ctx->daily_rules_listbox = gtk_list_box_new();

    GtkWidget* session_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget* session_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* session_label = gtk_label_new("Minutos:");
    GtkAdjustment* session_adj = gtk_adjustment_new(30, 1, 480, 5, 10, 0);
    ctx->session_minutes_spin = gtk_spin_button_new(session_adj, 1, 0);
    GtkWidget* start_session_button = gtk_button_new_with_label("Iniciar");
    GtkWidget* sessions_scrolled = gtk_scrolled_window_new();
    ctx->sessions_listbox = gtk_list_box_new();

    GtkWidget* save_button = gtk_button_new_with_label("Guardar");

    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->process_entry), "proceso.exe");
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->from_entry), "18:00");
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->to_entry), "20:00");
    gtk_editable_set_text(GTK_EDITABLE(ctx->from_entry), "18:00");
    gtk_editable_set_text(GTK_EDITABLE(ctx->to_entry), "20:00");
    gtk_widget_set_size_request(ctx->from_entry, 56, -1);
    gtk_widget_set_size_request(ctx->to_entry, 56, -1);
    gtk_widget_set_hexpand(ctx->process_entry, TRUE);

    gtk_widget_set_margin_start(root, 8);
    gtk_widget_set_margin_end(root, 8);
    gtk_widget_set_margin_top(root, 8);
    gtk_widget_set_margin_bottom(root, 8);

    gtk_box_append(GTK_BOX(reward_row), reward_label);
    gtk_box_append(GTK_BOX(reward_row), ctx->reward_spin);
    gtk_box_append(GTK_BOX(sound_row), sound_label);
    gtk_box_append(GTK_BOX(sound_row), ctx->sound_switch);
    gtk_box_append(GTK_BOX(general_row), reward_row);
    gtk_box_append(GTK_BOX(general_row), sound_row);

    gtk_box_append(GTK_BOX(apps_header), apps_title);
    gtk_box_append(GTK_BOX(apps_header), refresh_apps_button);

    gtk_box_append(GTK_BOX(add_row), ctx->process_entry);
    gtk_box_append(GTK_BOX(add_row), ctx->from_entry);
    gtk_box_append(GTK_BOX(add_row), ctx->to_entry);
    gtk_box_append(GTK_BOX(add_row), add_button);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), ctx->rules_listbox);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled, -1, 72);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(daily_scrolled), ctx->daily_rules_listbox);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(daily_scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(daily_scrolled, -1, 72);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sessions_scrolled), ctx->sessions_listbox);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sessions_scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(sessions_scrolled, -1, 72);

    gtk_box_append(GTK_BOX(daily_row), daily_label);
    gtk_box_append(GTK_BOX(daily_row), ctx->daily_limit_spin);
    gtk_box_append(GTK_BOX(daily_row), add_daily_button);
    gtk_box_append(GTK_BOX(session_row), session_label);
    gtk_box_append(GTK_BOX(session_row), ctx->session_minutes_spin);
    gtk_box_append(GTK_BOX(session_row), start_session_button);

    gtk_box_append(GTK_BOX(schedule_page), add_row);
    gtk_box_append(GTK_BOX(schedule_page), scrolled);
    gtk_box_append(GTK_BOX(daily_page), daily_row);
    gtk_box_append(GTK_BOX(daily_page), daily_scrolled);
    gtk_box_append(GTK_BOX(session_page), session_row);
    gtk_box_append(GTK_BOX(session_page), sessions_scrolled);

    gtk_stack_add_titled(GTK_STACK(limits_stack), schedule_page, "schedule", "Horario");
    gtk_stack_add_titled(GTK_STACK(limits_stack), daily_page, "daily", "Diario");
    gtk_stack_add_titled(GTK_STACK(limits_stack), session_page, "session", "Sesión");
    gtk_widget_set_vexpand(limits_stack, TRUE);
    gtk_widget_set_size_request(limits_stack, -1, 110);

    gtk_box_append(GTK_BOX(root), general_row);
    gtk_box_append(GTK_BOX(root), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_box_append(GTK_BOX(root), apps_header);
    gtk_box_append(GTK_BOX(root), apps_switcher);
    gtk_box_append(GTK_BOX(root), ctx->apps_stack);
    gtk_box_append(GTK_BOX(root), ctx->selected_label);
    gtk_box_append(GTK_BOX(root), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_box_append(GTK_BOX(root), limits_switcher);
    gtk_box_append(GTK_BOX(root), limits_stack);
    gtk_box_append(GTK_BOX(root), save_button);

    GtkWidget* outer_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(outer_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(outer_scroll), root);
    gtk_widget_set_vexpand(outer_scroll, TRUE);

    g_object_set_data(G_OBJECT(outer_scroll), "settings-view-ctx", ctx);
    g_object_set_data_full(G_OBJECT(outer_scroll), "settings-view-ctx-free", ctx, g_free);

    g_signal_connect(refresh_apps_button, "clicked", G_CALLBACK(ui_settings_on_refresh_apps_clicked), ctx);
    g_signal_connect(ctx->apps_stack, "notify::visible-child", G_CALLBACK(ui_settings_on_apps_stack_changed), ctx);
    g_signal_connect(ctx->installed_search_entry, "changed", G_CALLBACK(ui_settings_on_installed_search_changed), ctx);
    g_signal_connect(ctx->installed_apps_listbox, "row-activated", G_CALLBACK(ui_settings_on_installed_row_activated), ctx);
    g_signal_connect(ctx->configured_apps_listbox, "row-activated", G_CALLBACK(ui_settings_on_configured_row_activated), ctx);
    g_signal_connect(add_button, "clicked", G_CALLBACK(ui_settings_on_add_rule_clicked), ctx);
    g_signal_connect(add_daily_button, "clicked", G_CALLBACK(ui_settings_on_add_daily_limit_clicked), ctx);
    g_signal_connect(start_session_button, "clicked", G_CALLBACK(ui_settings_on_start_session_clicked), ctx);
    g_signal_connect(save_button, "clicked", G_CALLBACK(ui_settings_on_save_clicked), ctx);

    ui_settings_refresh_running_internal(ctx);
    ui_settings_refresh_rules_internal(ctx);
    ui_settings_refresh_daily_rules_internal(ctx);
    ui_settings_refresh_sessions_internal(ctx);
    return outer_scroll;
}
