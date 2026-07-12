#include "ui_stats.h"
#include "../stats/stats.h"
#include <stdio.h>

typedef struct {
    GtkWidget* current_streak_label;
    GtkWidget* longest_streak_label;
    GtkWidget* points_label;
    GtkWidget* level_label;
    GtkWidget* summary_label;
} StatsViewContext;

static void ui_stats_refresh_internal(StatsViewContext* ctx) {
    char buffer[128];

    snprintf(buffer, sizeof(buffer), "Racha actual: %d", stats_get_current_streak());
    gtk_label_set_text(GTK_LABEL(ctx->current_streak_label), buffer);

    snprintf(buffer, sizeof(buffer), "Racha más larga: %d", stats_get_longest_streak());
    gtk_label_set_text(GTK_LABEL(ctx->longest_streak_label), buffer);

    snprintf(buffer, sizeof(buffer), "Puntos: %d", stats_get_points());
    gtk_label_set_text(GTK_LABEL(ctx->points_label), buffer);

    snprintf(buffer, sizeof(buffer), "Nivel: %d", stats_get_level());
    gtk_label_set_text(GTK_LABEL(ctx->level_label), buffer);

    gtk_label_set_text(GTK_LABEL(ctx->summary_label), stats_get_daily_summary());
}

void ui_stats_refresh(GtkWidget* view) {
    StatsViewContext* ctx = (StatsViewContext*)g_object_get_data(G_OBJECT(view), "stats-view-ctx");
    if (ctx) {
        ui_stats_refresh_internal(ctx);
    }
}

static GtkWidget* ui_stats_make_label_row(const char* initial_text) {
    GtkWidget* label = gtk_label_new(initial_text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    return label;
}

GtkWidget* ui_stats_build(void) {
    StatsViewContext* ctx = g_new0(StatsViewContext, 1);

    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    ctx->current_streak_label = ui_stats_make_label_row("Racha actual: 0");
    ctx->longest_streak_label = ui_stats_make_label_row("Racha más larga: 0");
    ctx->points_label = ui_stats_make_label_row("Puntos: 0");
    ctx->level_label = ui_stats_make_label_row("Nivel: 1");
    ctx->summary_label = ui_stats_make_label_row("Resumen del día");

    gtk_widget_set_margin_start(root, 12);
    gtk_widget_set_margin_end(root, 12);
    gtk_widget_set_margin_top(root, 12);
    gtk_widget_set_margin_bottom(root, 12);

    gtk_box_append(GTK_BOX(root), ctx->current_streak_label);
    gtk_box_append(GTK_BOX(root), ctx->longest_streak_label);
    gtk_box_append(GTK_BOX(root), ctx->points_label);
    gtk_box_append(GTK_BOX(root), ctx->level_label);
    gtk_box_append(GTK_BOX(root), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_box_append(GTK_BOX(root), ctx->summary_label);

    g_object_set_data(G_OBJECT(root), "stats-view-ctx", ctx);
    g_object_set_data_full(G_OBJECT(root), "stats-view-ctx-free", ctx, g_free);

    ui_stats_refresh_internal(ctx);
    return root;
}
