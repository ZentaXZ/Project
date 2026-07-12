#include "ui_settings.h"
#include "../control/schedule.h"
#include "../utils/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    GtkWidget* reward_spin;
    GtkWidget* sound_switch;
    GtkWidget* process_entry;
    GtkWidget* from_entry;
    GtkWidget* to_entry;
    GtkWidget* rules_listbox;
} SettingsViewContext;

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
        snprintf(line, sizeof(line), "[Blocklist] %s  %s - %s",
                 rule.process_name, from_str, to_str);

        GtkWidget* row = gtk_list_box_row_new();
        GtkWidget* label = gtk_label_new(line);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(listbox, row);
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

        GtkWidget* row = gtk_list_box_row_new();
        GtkWidget* label = gtk_label_new(line);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(listbox, row);
    }
}

void ui_settings_refresh_rules(GtkWidget* view) {
    SettingsViewContext* ctx = (SettingsViewContext*)g_object_get_data(G_OBJECT(view), "settings-view-ctx");
    if (ctx) {
        ui_settings_refresh_rules_internal(ctx);
    }
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
    gtk_editable_set_text(GTK_EDITABLE(ctx->process_entry), "");
    ui_settings_refresh_rules_internal(ctx);
    (void)button;
}

static void ui_settings_on_save_clicked(GtkButton* button, gpointer user_data) {
    SettingsViewContext* ctx = (SettingsViewContext*)user_data;

    int reward = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(ctx->reward_spin));
    config_set_reward_minutes(reward);
    config_set_sound_enabled(gtk_switch_get_active(GTK_SWITCH(ctx->sound_switch)));

    config_save();
    schedule_save();
    ui_settings_refresh_rules_internal(ctx);
    (void)button;
}

GtkWidget* ui_settings_build(void) {
    SettingsViewContext* ctx = g_new0(SettingsViewContext, 1);

    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget* reward_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* reward_label = gtk_label_new("Minutos de recompensa por tarea:");
    GtkAdjustment* reward_adj = gtk_adjustment_new(config_get_reward_minutes(), 0, 120, 1, 5, 0);
    ctx->reward_spin = gtk_spin_button_new(reward_adj, 1, 0);

    GtkWidget* sound_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* sound_label = gtk_label_new("Sonido habilitado:");
    ctx->sound_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(ctx->sound_switch), config_get_sound_enabled());

    GtkWidget* add_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    ctx->process_entry = gtk_entry_new();
    ctx->from_entry = gtk_entry_new();
    ctx->to_entry = gtk_entry_new();
    GtkWidget* add_button = gtk_button_new_with_label("Agregar regla");
    GtkWidget* save_button = gtk_button_new_with_label("Guardar");

    GtkWidget* scrolled = gtk_scrolled_window_new();
    ctx->rules_listbox = gtk_list_box_new();

    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->process_entry), "proceso.exe");
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->from_entry), "18:00");
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->to_entry), "20:00");
    gtk_editable_set_text(GTK_EDITABLE(ctx->from_entry), "18:00");
    gtk_editable_set_text(GTK_EDITABLE(ctx->to_entry), "20:00");
    gtk_widget_set_hexpand(ctx->process_entry, TRUE);

    gtk_widget_set_margin_start(root, 12);
    gtk_widget_set_margin_end(root, 12);
    gtk_widget_set_margin_top(root, 12);
    gtk_widget_set_margin_bottom(root, 12);

    gtk_box_append(GTK_BOX(reward_row), reward_label);
    gtk_box_append(GTK_BOX(reward_row), ctx->reward_spin);
    gtk_box_append(GTK_BOX(sound_row), sound_label);
    gtk_box_append(GTK_BOX(sound_row), ctx->sound_switch);

    gtk_box_append(GTK_BOX(add_row), ctx->process_entry);
    gtk_box_append(GTK_BOX(add_row), ctx->from_entry);
    gtk_box_append(GTK_BOX(add_row), ctx->to_entry);
    gtk_box_append(GTK_BOX(add_row), add_button);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), ctx->rules_listbox);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_set_size_request(scrolled, -1, 200);

    gtk_box_append(GTK_BOX(root), reward_row);
    gtk_box_append(GTK_BOX(root), sound_row);
    gtk_box_append(GTK_BOX(root), gtk_label_new("Reglas de horario (blocklist / whitelist):"));
    gtk_box_append(GTK_BOX(root), add_row);
    gtk_box_append(GTK_BOX(root), scrolled);
    gtk_box_append(GTK_BOX(root), save_button);

    g_object_set_data(G_OBJECT(root), "settings-view-ctx", ctx);
    g_object_set_data_full(G_OBJECT(root), "settings-view-ctx-free", ctx, g_free);

    g_signal_connect(add_button, "clicked", G_CALLBACK(ui_settings_on_add_rule_clicked), ctx);
    g_signal_connect(save_button, "clicked", G_CALLBACK(ui_settings_on_save_clicked), ctx);

    ui_settings_refresh_rules_internal(ctx);
    return root;
}
