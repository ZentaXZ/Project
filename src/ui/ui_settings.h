#ifndef UI_SETTINGS_H
#define UI_SETTINGS_H

#include <gtk/gtk.h>

GtkWidget* ui_settings_build(void);
void ui_settings_refresh_rules(GtkWidget* view);

#endif // UI_SETTINGS_H
