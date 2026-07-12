#include <gtk/gtk.h>

static void activate(GtkApplication* app, gpointer user_data) {
    (void)user_data;

    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Gestor de Tareas");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget* label = gtk_label_new("Hola, GTK4!");
    gtk_window_set_child(GTK_WINDOW(window), label);

    gtk_widget_set_visible(window, TRUE);
}

int main(int argc, char** argv) {
    GtkApplication* app = gtk_application_new("com.tuusuario.gestortareas", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
