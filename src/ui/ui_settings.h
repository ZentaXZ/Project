#ifndef UI_SETTINGS_H
#define UI_SETTINGS_H

#include <gtk/gtk.h>

GtkWidget* ui_settings_build(void);
void ui_settings_refresh_rules(GtkWidget* view);
void ui_settings_refresh_apps(GtkWidget* view);
void ui_settings_start_live_refresh(GtkWidget* view);
void ui_settings_stop_live_refresh(GtkWidget* view);

#endif // UI_SETTINGS_H
