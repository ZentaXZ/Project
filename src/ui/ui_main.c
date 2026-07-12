#include "ui_main.h"
#include "ui_tasks.h"
#include "ui_completed.h"
#include "ui_stats.h"
#include "ui_settings.h"
#include <string.h>

typedef struct {
    GtkWidget* completed_view;
    GtkWidget* stats_view;
} MainWindowContext;

static void ui_main_on_stack_visible_child_changed(GObject* object, GParamSpec* pspec, gpointer user_data) {
    (void)pspec;
    MainWindowContext* ctx = (MainWindowContext*)user_data;
    GtkStack* stack = GTK_STACK(object);
    const char* name = gtk_stack_get_visible_child_name(stack);

    if (!name) {
        return;
    }

    if (strcmp(name, "completed") == 0) {
        ui_completed_refresh(ctx->completed_view);
    } else if (strcmp(name, "stats") == 0) {
        ui_stats_refresh(ctx->stats_view);
    }
}

GtkWidget* ui_main_build(GtkApplication* app) {
    MainWindowContext* ctx = g_new0(MainWindowContext, 1);

    GtkWidget* window = gtk_application_window_new(app);
    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget* stack = gtk_stack_new();
    GtkWidget* switcher = gtk_stack_switcher_new();
    GtkWidget* tasks_view = ui_tasks_build_normal();
    GtkWidget* daily_view = ui_tasks_build_daily();
    GtkWidget* completed_view = ui_completed_build();
    GtkWidget* stats_view = ui_stats_build();
    GtkWidget* settings_view = ui_settings_build();

    ctx->completed_view = completed_view;
    ctx->stats_view = stats_view;

    gtk_window_set_title(GTK_WINDOW(window), "Gestor de Tareas");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack));
    gtk_stack_add_titled(GTK_STACK(stack), tasks_view, "tasks", "Tareas");
    gtk_stack_add_titled(GTK_STACK(stack), daily_view, "daily", "Diarias");
    gtk_stack_add_titled(GTK_STACK(stack), completed_view, "completed", "Completadas");
    gtk_stack_add_titled(GTK_STACK(stack), stats_view, "stats", "Estadísticas");
    gtk_stack_add_titled(GTK_STACK(stack), settings_view, "settings", "Configuración");

    gtk_widget_set_vexpand(stack, TRUE);
    gtk_box_append(GTK_BOX(root), switcher);
    gtk_box_append(GTK_BOX(root), stack);
    gtk_window_set_child(GTK_WINDOW(window), root);

    g_object_set_data(G_OBJECT(window), "main-window-ctx", ctx);
    g_object_set_data_full(G_OBJECT(window), "main-window-ctx-free", ctx, g_free);

    g_signal_connect(stack, "notify::visible-child", G_CALLBACK(ui_main_on_stack_visible_child_changed), ctx);

    return window;
}
