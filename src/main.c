#include <gtk/gtk.h>
#include "ui/ui_main.h"
#include "tasks/task_manager.h"
#include "stats/stats.h"

static void activate(GtkApplication* app, gpointer user_data) {
    (void)user_data;

    task_manager_init();
    stats_init();

    GtkWidget* window = ui_main_build(app);
    gtk_widget_set_visible(window, TRUE);
}

int main(int argc, char** argv) {
    GtkApplication* app = gtk_application_new("com.tuusuario.gestortareas", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
