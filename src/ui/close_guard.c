#include "close_guard.h"
#include "../utils/config.h"
#include "../control/watchdog_launcher.h"
#include <stdio.h>

#define CLOSE_CONFIRMATIONS_REQUIRED 10

typedef struct {
    GtkWindow* window;
    int confirm_count;
} CloseGuardContext;

static void close_guard_shutdown_watchdog_and_quit(CloseGuardContext* ctx) {
    watchdog_launcher_request_shutdown();
    watchdog_launcher_stop();

    GtkApplication* app = gtk_window_get_application(ctx->window);
    gtk_window_destroy(ctx->window);

    if (app) {
        g_application_quit(G_APPLICATION(app));
    }
}

static void show_close_confirmation(CloseGuardContext* ctx);

static void on_confirm_response(GtkDialog* dialog, int response, gpointer user_data) {
    CloseGuardContext* ctx = (CloseGuardContext*)user_data;
    gtk_window_destroy(GTK_WINDOW(dialog));

    if (response != GTK_RESPONSE_YES) {
        ctx->confirm_count = 0;
        return;
    }

    ctx->confirm_count++;

    if (ctx->confirm_count >= CLOSE_CONFIRMATIONS_REQUIRED) {
        close_guard_shutdown_watchdog_and_quit(ctx);
        return;
    }

    show_close_confirmation(ctx);
}

static void show_close_confirmation(CloseGuardContext* ctx) {
    char message[256];
    snprintf(message, sizeof(message),
        "¿Seguro que querés cerrar? (%d/%d)\nEsto puede afectar tu racha y tus tareas pendientes.",
        ctx->confirm_count, CLOSE_CONFIRMATIONS_REQUIRED);

    GtkWidget* dialog = gtk_message_dialog_new(
        ctx->window,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_YES_NO,
        "%s", message
    );

    g_signal_connect(dialog, "response", G_CALLBACK(on_confirm_response), ctx);
    gtk_widget_set_visible(dialog, TRUE);
}

static gboolean on_close_request_with_guard(GtkWindow* window, gpointer user_data) {
    CloseGuardContext* ctx = (CloseGuardContext*)user_data;
    (void)window;
    show_close_confirmation(ctx);
    return TRUE;
}

static gboolean on_close_request_allow(GtkWindow* window, gpointer user_data) {
    (void)window;
    (void)user_data;
    watchdog_launcher_request_shutdown();
    watchdog_launcher_stop();
    return FALSE;
}

void close_guard_attach(GtkWindow* window) {
    if (!config_get_close_guard_enabled()) {
        g_signal_connect(window, "close-request", G_CALLBACK(on_close_request_allow), NULL);
        return;
    }

    CloseGuardContext* ctx = g_new0(CloseGuardContext, 1);
    ctx->window = window;
    ctx->confirm_count = 0;

    g_signal_connect(window, "close-request", G_CALLBACK(on_close_request_with_guard), ctx);
    g_object_set_data_full(G_OBJECT(window), "close-guard-ctx", ctx, g_free);
}
