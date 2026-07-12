#ifndef UI_TASKS_H
#define UI_TASKS_H

#include <gtk/gtk.h>

/**
 * Build the normal tasks view (entry + add button + list).
 */
GtkWidget* ui_tasks_build_normal(void);

/**
 * Build the daily tasks view (entry + add button + list).
 */
GtkWidget* ui_tasks_build_daily(void);

#endif // UI_TASKS_H
