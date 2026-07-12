#include <gtk/gtk.h>
#include "ui/ui_main.h"
#include "tasks/task_manager.h"
#include "stats/stats.h"
#include "utils/config.h"
#include "control/schedule.h"
#include "control/soft_punishment.h"
#include "control/process_monitor.h"
#include "control/app_blocker.h"
#include "control/reward_timer.h"

static gboolean on_process_poll(gpointer user_data) {
    (void)user_data;
    process_monitor_poll();
    app_blocker_enforce();
    return G_SOURCE_CONTINUE;
}

static gboolean on_reward_tick(gpointer user_data) {
    (void)user_data;
    reward_timer_tick();
    return G_SOURCE_CONTINUE;
}

static void activate(GtkApplication* app, gpointer user_data) {
    (void)user_data;

    config_load();
    task_manager_init();
    daily_task_manager_reset_if_new_day();
    stats_init();
    schedule_load();
    soft_punishment_check_on_startup();
    process_monitor_init();
    reward_timer_on_expire_callback(app_blocker_enforce);

    GtkWidget* window = ui_main_build(app);
    gtk_widget_set_visible(window, TRUE);

    g_timeout_add(5000, on_process_poll, NULL);
    g_timeout_add(60000, on_reward_tick, NULL);
}

int main(int argc, char** argv) {
    GtkApplication* app = gtk_application_new("com.tuusuario.gestortareas", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
