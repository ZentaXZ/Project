#ifndef UI_MAIN_H
#define UI_MAIN_H

#include <gtk/gtk.h>

/**
 * Build the main application window with tab navigation.
 * @return The top-level GtkApplicationWindow (not yet visible).
 */
GtkWidget* ui_main_build(GtkApplication* app);

#endif // UI_MAIN_H
