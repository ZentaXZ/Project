#include "ui_tasks.h"
#include "../tasks/task_manager.h"
#include "../tasks/task.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    GtkWidget* entry;
    GtkWidget* listbox;
    int is_daily;
} TasksViewContext;

static void ui_tasks_refresh(TasksViewContext* ctx);
static void ui_tasks_on_add_clicked(GtkButton* button, gpointer user_data);
static void ui_tasks_on_normal_toggled(GtkCheckButton* check, gpointer user_data);
static void ui_tasks_on_daily_toggled(GtkCheckButton* check, gpointer user_data);

static void ui_tasks_refresh(TasksViewContext* ctx) {
    GtkListBox* listbox = GTK_LIST_BOX(ctx->listbox);
    GtkWidget* child;

    while ((child = gtk_widget_get_first_child(ctx->listbox)) != NULL) {
        gtk_list_box_remove(listbox, child);
    }

    if (ctx->is_daily) {
        int count = 0;
        DailyTask* tasks = (DailyTask*)daily_task_manager_get_all(&count);

        for (int i = 0; i < count; i++) {
            GtkWidget* row = gtk_list_box_row_new();
            GtkWidget* row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            GtkWidget* check = gtk_check_button_new();
            GtkWidget* label = gtk_label_new(tasks[i].title);
            char* task_id = g_strdup(tasks[i].id);

            gtk_widget_set_halign(label, GTK_ALIGN_START);
            gtk_widget_set_hexpand(label, TRUE);
            gtk_box_append(GTK_BOX(row_box), check);
            gtk_box_append(GTK_BOX(row_box), label);
            gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
            g_object_set_data_full(G_OBJECT(check), "task-id", task_id, g_free);
            g_object_set_data(G_OBJECT(check), "tasks-view", ctx);
            g_signal_connect(check, "toggled", G_CALLBACK(ui_tasks_on_daily_toggled), NULL);
            gtk_list_box_append(listbox, row);
        }
    } else {
        int count = 0;
        Task* tasks = (Task*)task_manager_get_all(&count);

        for (int i = 0; i < count; i++) {
            if (tasks[i].completed) {
                continue;
            }

            GtkWidget* row = gtk_list_box_row_new();
            GtkWidget* row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            GtkWidget* check = gtk_check_button_new();
            GtkWidget* label = gtk_label_new(tasks[i].title);
            char* task_id = g_strdup(tasks[i].id);

            gtk_widget_set_halign(label, GTK_ALIGN_START);
            gtk_widget_set_hexpand(label, TRUE);
            gtk_box_append(GTK_BOX(row_box), check);
            gtk_box_append(GTK_BOX(row_box), label);
            gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
            g_object_set_data_full(G_OBJECT(check), "task-id", task_id, g_free);
            g_object_set_data(G_OBJECT(check), "tasks-view", ctx);
            g_signal_connect(check, "toggled", G_CALLBACK(ui_tasks_on_normal_toggled), NULL);
            gtk_list_box_append(listbox, row);
        }
    }
}

static void ui_tasks_on_add_clicked(GtkButton* button, gpointer user_data) {
    TasksViewContext* ctx = (TasksViewContext*)user_data;
    const char* text = gtk_editable_get_text(GTK_EDITABLE(ctx->entry));

    if (!text || text[0] == '\0') {
        return;
    }

    if (ctx->is_daily) {
        daily_task_manager_create(text, RECUR_DAILY, 0);
    } else {
        task_manager_create(text, "", PRIORITY_MEDIUM);
    }

    gtk_editable_set_text(GTK_EDITABLE(ctx->entry), "");
    ui_tasks_refresh(ctx);
    (void)button;
}

static void ui_tasks_on_normal_toggled(GtkCheckButton* check, gpointer user_data) {
    (void)user_data;

    if (!gtk_check_button_get_active(check)) {
        return;
    }

    const char* task_id = (const char*)g_object_get_data(G_OBJECT(check), "task-id");
    TasksViewContext* ctx = (TasksViewContext*)g_object_get_data(G_OBJECT(check), "tasks-view");

    if (!task_id || !ctx) {
        return;
    }

    task_manager_complete(task_id);
    ui_tasks_refresh(ctx);
}

static void ui_tasks_on_daily_toggled(GtkCheckButton* check, gpointer user_data) {
    (void)user_data;

    if (!gtk_check_button_get_active(check)) {
        return;
    }

    const char* task_id = (const char*)g_object_get_data(G_OBJECT(check), "task-id");
    TasksViewContext* ctx = (TasksViewContext*)g_object_get_data(G_OBJECT(check), "tasks-view");

    if (!task_id || !ctx) {
        return;
    }

    daily_task_manager_complete(task_id);
    ui_tasks_refresh(ctx);
}

static GtkWidget* ui_tasks_build(int is_daily) {
    TasksViewContext* ctx = g_new0(TasksViewContext, 1);
    ctx->is_daily = is_daily;

    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget* input_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* entry = gtk_entry_new();
    GtkWidget* add_button = gtk_button_new_with_label("Agregar");
    GtkWidget* scrolled = gtk_scrolled_window_new();
    GtkWidget* listbox = gtk_list_box_new();

    ctx->entry = entry;
    ctx->listbox = listbox;

    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), is_daily ? "Nueva tarea diaria..." : "Nueva tarea...");
    gtk_widget_set_margin_start(root, 12);
    gtk_widget_set_margin_end(root, 12);
    gtk_widget_set_margin_top(root, 12);
    gtk_widget_set_margin_bottom(root, 12);
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_box_append(GTK_BOX(input_row), entry);
    gtk_box_append(GTK_BOX(input_row), add_button);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), listbox);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled, TRUE);

    gtk_box_append(GTK_BOX(root), input_row);
    gtk_box_append(GTK_BOX(root), scrolled);

    g_object_set_data_full(G_OBJECT(root), "tasks-view-ctx", ctx, g_free);
    g_signal_connect(add_button, "clicked", G_CALLBACK(ui_tasks_on_add_clicked), ctx);
    g_signal_connect(entry, "activate", G_CALLBACK(ui_tasks_on_add_clicked), ctx);

    ui_tasks_refresh(ctx);

    return root;
}

GtkWidget* ui_tasks_build_normal(void) {
    return ui_tasks_build(0);
}

GtkWidget* ui_tasks_build_daily(void) {
    return ui_tasks_build(1);
}
