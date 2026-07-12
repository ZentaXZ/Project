#include "stats.h"
#include "../utils/json_utils.h"
#include "../utils/time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int current_streak;
    int longest_streak;
    int points;
    int level;
    char last_active_date[11]; // YYYY-MM-DD
} Stats;

static Stats g_stats = {0};

bool stats_init(void) {
    cJSON* root = json_utils_load_file("data/stats.json");
    if (!root) {
        g_stats.current_streak = 0;
        g_stats.longest_streak = 0;
        g_stats.points = 0;
        g_stats.level = 1;
        return true;
    }

    cJSON* current_streak = cJSON_GetObjectItem(root, "current_streak");
    if (current_streak && cJSON_IsNumber(current_streak)) g_stats.current_streak = current_streak->valueint;

    cJSON* longest_streak = cJSON_GetObjectItem(root, "longest_streak");
    if (longest_streak && cJSON_IsNumber(longest_streak)) g_stats.longest_streak = longest_streak->valueint;

    cJSON* points = cJSON_GetObjectItem(root, "points");
    if (points && cJSON_IsNumber(points)) g_stats.points = points->valueint;

    cJSON* level = cJSON_GetObjectItem(root, "level");
    if (level && cJSON_IsNumber(level)) g_stats.level = level->valueint;

    cJSON* last_active = cJSON_GetObjectItem(root, "last_active_date");
    if (last_active && last_active->valuestring) strncpy(g_stats.last_active_date, last_active->valuestring, 10);

    cJSON_Delete(root);
    return true;
}

void stats_register_task_completed(void) {
    g_stats.current_streak++;
    if (g_stats.current_streak > g_stats.longest_streak) {
        g_stats.longest_streak = g_stats.current_streak;
    }
    g_stats.points += 10; // TODO: Configurable points per task

    // TODO: Save stats to file
}

int stats_get_current_streak(void) {
    return g_stats.current_streak;
}

int stats_get_longest_streak(void) {
    return g_stats.longest_streak;
}

void stats_add_screen_time(int minutes) {
    // TODO: Implement
}

void stats_add_distraction_time(int minutes) {
    // TODO: Implement
}

const char* stats_get_daily_summary(void) {
    static char summary[256];
    snprintf(summary, sizeof(summary),
        "Racha: %d | Puntos: %d | Nivel: %d",
        g_stats.current_streak,
        g_stats.points,
        g_stats.level
    );
    return summary;
}

int stats_get_points(void) {
    return g_stats.points;
}

int stats_get_level(void) {
    return g_stats.level;
}
