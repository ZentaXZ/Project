#include "ui_completed.h"
#include "../utils/json_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char title[128];
    char type[16];
    char completed_at[32];
} CompletedEntry;

typedef struct {
    GtkWidget* listbox;
} CompletedViewContext;

static const char* ui_completed_type_label(const char* type) {
    if (strcmp(type, "daily") == 0) {
        return "Diaria";
    }
    return "Normal";
}

static int ui_completed_compare_entries(const void* a, const void* b) {
    const CompletedEntry* entry_a = (const CompletedEntry*)a;
    const CompletedEntry* entry_b = (const CompletedEntry*)b;
    return strcmp(entry_b->completed_at, entry_a->completed_at);
}

static void ui_completed_refresh_internal(CompletedViewContext* ctx) {
    GtkListBox* listbox = GTK_LIST_BOX(ctx->listbox);
    GtkWidget* child;

    while ((child = gtk_widget_get_first_child(ctx->listbox)) != NULL) {
        gtk_list_box_remove(listbox, child);
    }

    cJSON* root = json_utils_load_file("data/completed_tasks.json");
    if (!root) {
        return;
    }

    cJSON* completed_array = cJSON_GetObjectItem(root, "completed");
    if (!cJSON_IsArray(completed_array)) {
        cJSON_Delete(root);
        return;
    }

    int count = cJSON_GetArraySize(completed_array);
    if (count <= 0) {
        cJSON_Delete(root);
        return;
    }

    CompletedEntry* entries = calloc((size_t)count, sizeof(CompletedEntry));
    if (!entries) {
        cJSON_Delete(root);
        return;
    }

    int idx = 0;
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, completed_array) {
        cJSON* title = cJSON_GetObjectItem(item, "title");
        cJSON* type = cJSON_GetObjectItem(item, "type");
        cJSON* completed_at = cJSON_GetObjectItem(item, "completed_at");

        if (title && title->valuestring) {
            strncpy(entries[idx].title, title->valuestring, sizeof(entries[idx].title) - 1);
        }
        if (type && type->valuestring) {
            strncpy(entries[idx].type, type->valuestring, sizeof(entries[idx].type) - 1);
        }
        if (completed_at && completed_at->valuestring) {
            strncpy(entries[idx].completed_at, completed_at->valuestring, sizeof(entries[idx].completed_at) - 1);
        }
        idx++;
    }

    qsort(entries, (size_t)count, sizeof(CompletedEntry), ui_completed_compare_entries);

    for (int i = 0; i < count; i++) {
        GtkWidget* row = gtk_list_box_row_new();
        GtkWidget* row_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        GtkWidget* title_label = gtk_label_new(entries[i].title);
        char details[192];

        snprintf(details, sizeof(details), "%s  ·  %s",
                 ui_completed_type_label(entries[i].type),
                 entries[i].completed_at[0] ? entries[i].completed_at : "Sin fecha");

        GtkWidget* details_label = gtk_label_new(details);

        gtk_widget_set_halign(title_label, GTK_ALIGN_START);
        gtk_widget_set_halign(details_label, GTK_ALIGN_START);
        gtk_label_set_xalign(GTK_LABEL(title_label), 0.0f);
        gtk_label_set_xalign(GTK_LABEL(details_label), 0.0f);
        gtk_widget_add_css_class(details_label, "dim-label");

        gtk_box_append(GTK_BOX(row_box), title_label);
        gtk_box_append(GTK_BOX(row_box), details_label);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
        gtk_list_box_append(listbox, row);
    }

    free(entries);
    cJSON_Delete(root);
}

void ui_completed_refresh(GtkWidget* view) {
    CompletedViewContext* ctx = (CompletedViewContext*)g_object_get_data(G_OBJECT(view), "completed-view-ctx");
    if (ctx) {
        ui_completed_refresh_internal(ctx);
    }
}

GtkWidget* ui_completed_build(void) {
    CompletedViewContext* ctx = g_new0(CompletedViewContext, 1);

    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget* scrolled = gtk_scrolled_window_new();
    GtkWidget* listbox = gtk_list_box_new();

    ctx->listbox = listbox;

    gtk_widget_set_margin_start(root, 12);
    gtk_widget_set_margin_end(root, 12);
    gtk_widget_set_margin_top(root, 12);
    gtk_widget_set_margin_bottom(root, 12);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), listbox);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled, TRUE);

    gtk_box_append(GTK_BOX(root), scrolled);

    g_object_set_data(G_OBJECT(root), "completed-view-ctx", ctx);
    g_object_set_data_full(G_OBJECT(root), "completed-view-ctx-free", ctx, g_free);

    ui_completed_refresh_internal(ctx);

    return root;
}
