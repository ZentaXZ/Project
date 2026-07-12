#include <gtk/gtk.h>
#include <stdio.h>
#include "utils/json_utils.h"
#include "utils/time_utils.h"
#include "tasks/task_manager.h"
#include "stats/stats.h"

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    // TODO: Initialize modules in order
    // 1. json_utils (implicit, used by others)
    // 2. time_utils
    // 3. task_manager
    // 4. stats
    // 5. UI modules
    // 6. control modules

    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Prompt Maestro - Gestor de Tareas");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget* label = gtk_label_new("Bienvenido a Prompt Maestro");
    gtk_container_add(GTK_CONTAINER(window), label);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
