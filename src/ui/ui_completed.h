#ifndef UI_COMPLETED_H
#define UI_COMPLETED_H

#include <gtk/gtk.h>

/**
 * Build the completed tasks view with delete support.
 */
GtkWidget* ui_completed_build(void);

/**
 * Reload completed tasks from disk and refresh the list.
 */
void ui_completed_refresh(GtkWidget* view);

#endif // UI_COMPLETED_H
