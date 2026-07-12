#include "stats.h"
#include "../utils/json_utils.h"
#include "../utils/time_utils.h"
#include "../control/process_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    char date[11];
    int tasks_completed;
    int usage_seconds;
    int sessions;
} DailyHistoryEntry;

typedef struct {
    char app_name[64];
    int seconds;
} AppUsageEntry;

typedef struct {
    int current_streak;
    int longest_streak;
    int points;
    int level;
    char last_active_date[11];
    DailyHistoryEntry daily_history[64];
    int daily_history_count;
    AppUsageEntry app_usage[32];
    int app_usage_count;
} Stats;

static Stats g_stats = {0};

static void stats_get_date_for_day_ago(int day_ago, char* out, size_t out_size) {
    time_t now = time(NULL);
    time_t target = now - (time_t)day_ago * 86400;
    struct tm* tm_info = localtime(&target);
    strftime(out, out_size, "%Y-%m-%d", tm_info);
}

static DailyHistoryEntry* stats_find_history_by_date(const char* date) {
    for (int i = 0; i < g_stats.daily_history_count; i++) {
        if (strcmp(g_stats.daily_history[i].date, date) == 0) {
            return &g_stats.daily_history[i];
        }
    }
    return NULL;
}

static void stats_ensure_today_entry(void) {
    char today[11];
    stats_get_date_for_day_ago(0, today, sizeof(today));

    if (stats_find_history_by_date(today)) {
        return;
    }

    if (g_stats.daily_history_count >= 64) {
        return;
    }

    DailyHistoryEntry* entry = &g_stats.daily_history[g_stats.daily_history_count++];
    memset(entry, 0, sizeof(DailyHistoryEntry));
    strncpy(entry->date, today, sizeof(entry->date) - 1);
    entry->sessions = 1;
}

static void stats_load_history(cJSON* root) {
    g_stats.daily_history_count = 0;

    cJSON* history = cJSON_GetObjectItem(root, "daily_history");
    if (!cJSON_IsArray(history)) {
        return;
    }

    cJSON* item = NULL;
    cJSON_ArrayForEach(item, history) {
        if (g_stats.daily_history_count >= 64) {
            break;
        }

        DailyHistoryEntry* entry = &g_stats.daily_history[g_stats.daily_history_count];
        memset(entry, 0, sizeof(DailyHistoryEntry));

        cJSON* date = cJSON_GetObjectItem(item, "date");
        cJSON* tasks = cJSON_GetObjectItem(item, "tasks_completed");
        cJSON* usage = cJSON_GetObjectItem(item, "usage_seconds");
        cJSON* sessions = cJSON_GetObjectItem(item, "sessions");

        if (date && date->valuestring) {
            strncpy(entry->date, date->valuestring, sizeof(entry->date) - 1);
        }
        if (tasks && cJSON_IsNumber(tasks)) {
            entry->tasks_completed = tasks->valueint;
        }
        if (usage && cJSON_IsNumber(usage)) {
            entry->usage_seconds = usage->valueint;
        } else if (tasks && cJSON_IsNumber(tasks)) {
            entry->usage_seconds = tasks->valueint * 900;
        }
        if (sessions && cJSON_IsNumber(sessions)) {
            entry->sessions = sessions->valueint;
        } else {
            entry->sessions = entry->tasks_completed > 0 ? entry->tasks_completed : 1;
        }

        g_stats.daily_history_count++;
    }
}

static void stats_load_app_usage(cJSON* root) {
    g_stats.app_usage_count = 0;

    cJSON* usage = cJSON_GetObjectItem(root, "app_usage");
    if (!cJSON_IsArray(usage)) {
        return;
    }

    cJSON* item = NULL;
    cJSON_ArrayForEach(item, usage) {
        if (g_stats.app_usage_count >= 32) {
            break;
        }

        AppUsageEntry* entry = &g_stats.app_usage[g_stats.app_usage_count];
        memset(entry, 0, sizeof(AppUsageEntry));

        cJSON* name = cJSON_GetObjectItem(item, "app_name");
        cJSON* seconds = cJSON_GetObjectItem(item, "seconds");

        if (name && name->valuestring) {
            if (process_monitor_is_system_process(name->valuestring)) {
                continue;
            }
            strncpy(entry->app_name, name->valuestring, sizeof(entry->app_name) - 1);
        }
        if (seconds && cJSON_IsNumber(seconds)) {
            entry->seconds = seconds->valueint;
        }

        g_stats.app_usage_count++;
    }
}

static void stats_seed_from_processes_if_empty(void) {
    if (g_stats.app_usage_count > 0) {
        return;
    }

    char names[32][64];
    int count = process_monitor_list_running(names, 32);
    for (int i = 0; i < count && g_stats.app_usage_count < 32; i++) {
        if (process_monitor_is_system_process(names[i])) {
            continue;
        }

        AppUsageEntry* entry = &g_stats.app_usage[g_stats.app_usage_count++];
        strncpy(entry->app_name, names[i], sizeof(entry->app_name) - 1);
        entry->seconds = 0;
    }
}

static bool stats_save(void) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "current_streak", g_stats.current_streak);
    cJSON_AddNumberToObject(root, "longest_streak", g_stats.longest_streak);
    cJSON_AddNumberToObject(root, "points", g_stats.points);
    cJSON_AddNumberToObject(root, "level", g_stats.level);
    cJSON_AddStringToObject(root, "last_active_date",
                            g_stats.last_active_date[0] != '\0' ? g_stats.last_active_date : "");

    cJSON* history = cJSON_CreateArray();
    for (int i = 0; i < g_stats.daily_history_count; i++) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "date", g_stats.daily_history[i].date);
        cJSON_AddNumberToObject(item, "tasks_completed", g_stats.daily_history[i].tasks_completed);
        cJSON_AddNumberToObject(item, "usage_seconds", g_stats.daily_history[i].usage_seconds);
        cJSON_AddNumberToObject(item, "sessions", g_stats.daily_history[i].sessions);
        cJSON_AddItemToArray(history, item);
    }
    cJSON_AddItemToObject(root, "daily_history", history);

    cJSON* usage = cJSON_CreateArray();
    for (int i = 0; i < g_stats.app_usage_count; i++) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "app_name", g_stats.app_usage[i].app_name);
        cJSON_AddNumberToObject(item, "seconds", g_stats.app_usage[i].seconds);
        cJSON_AddItemToArray(usage, item);
    }
    cJSON_AddItemToObject(root, "app_usage", usage);

    bool ok = json_utils_save_file("data/stats.json", root);
    cJSON_Delete(root);
    return ok;
}

bool stats_init(void) {
    memset(&g_stats, 0, sizeof(g_stats));
    g_stats.level = 1;

    cJSON* root = json_utils_load_file("data/stats.json");
    if (!root) {
        stats_ensure_today_entry();
        stats_seed_from_processes_if_empty();
        return true;
    }

    cJSON* current_streak = cJSON_GetObjectItem(root, "current_streak");
    if (current_streak && cJSON_IsNumber(current_streak)) {
        g_stats.current_streak = current_streak->valueint;
    }

    cJSON* longest_streak = cJSON_GetObjectItem(root, "longest_streak");
    if (longest_streak && cJSON_IsNumber(longest_streak)) {
        g_stats.longest_streak = longest_streak->valueint;
    }

    cJSON* points = cJSON_GetObjectItem(root, "points");
    if (points && cJSON_IsNumber(points)) {
        g_stats.points = points->valueint;
    }

    cJSON* level = cJSON_GetObjectItem(root, "level");
    if (level && cJSON_IsNumber(level)) {
        g_stats.level = level->valueint;
    }

    cJSON* last_active = cJSON_GetObjectItem(root, "last_active_date");
    if (last_active && last_active->valuestring) {
        strncpy(g_stats.last_active_date, last_active->valuestring, 10);
    }

    stats_load_history(root);
    stats_load_app_usage(root);
    cJSON_Delete(root);

    stats_ensure_today_entry();
    stats_seed_from_processes_if_empty();
    return true;
}

void stats_register_task_completed(void) {
    char today[11];
    stats_get_date_for_day_ago(0, today, sizeof(today));

    if (g_stats.last_active_date[0] == '\0') {
        g_stats.current_streak = 1;
    } else if (strcmp(g_stats.last_active_date, today) == 0) {
        /* Mismo día: la racha diaria no cambia, solo suman puntos/tareas. */
    } else {
        char yesterday[11];
        stats_get_date_for_day_ago(1, yesterday, sizeof(yesterday));
        if (strcmp(g_stats.last_active_date, yesterday) == 0) {
            g_stats.current_streak++;
        } else {
            g_stats.current_streak = 1;
        }
    }

    strncpy(g_stats.last_active_date, today, sizeof(g_stats.last_active_date) - 1);
    g_stats.last_active_date[sizeof(g_stats.last_active_date) - 1] = '\0';

    if (g_stats.current_streak > g_stats.longest_streak) {
        g_stats.longest_streak = g_stats.current_streak;
    }

    g_stats.points += 10;
    g_stats.level = (g_stats.points / 100) + 1;
    if (g_stats.level < 1) {
        g_stats.level = 1;
    }

    stats_ensure_today_entry();
    DailyHistoryEntry* entry = stats_find_history_by_date(today);
    if (entry) {
        entry->tasks_completed++;
        entry->usage_seconds += 900;
        entry->sessions++;
    }

    stats_save();
}

int stats_get_current_streak(void) {
    return g_stats.current_streak;
}

int stats_get_longest_streak(void) {
    return g_stats.longest_streak;
}

void stats_add_screen_time(int minutes) {
    (void)minutes;
}

void stats_add_distraction_time(int minutes) {
    (void)minutes;
}

const char* stats_get_daily_summary(void) {
    static char summary[256];
    snprintf(summary, sizeof(summary),
             "Racha: %d | Puntos: %d | Nivel: %d",
             g_stats.current_streak,
             g_stats.points,
             g_stats.level);
    return summary;
}

int stats_get_points(void) {
    return g_stats.points;
}

int stats_get_level(void) {
    return g_stats.level;
}

int stats_get_total_usage_seconds(void) {
    int total = 0;
    for (int i = 0; i < g_stats.daily_history_count; i++) {
        total += g_stats.daily_history[i].usage_seconds;
    }
    if (total == 0) {
        total = g_stats.points * 90;
    }
    return total;
}

int stats_get_total_sessions(void) {
    int total = 0;
    for (int i = 0; i < g_stats.daily_history_count; i++) {
        total += g_stats.daily_history[i].sessions;
    }
    if (total == 0) {
        total = g_stats.current_streak > 0 ? g_stats.current_streak : 1;
    }
    return total;
}

int stats_get_daily_average_seconds(void) {
    if (g_stats.daily_history_count <= 0) {
        return stats_get_total_usage_seconds();
    }

    int total = stats_get_total_usage_seconds();
    return total / g_stats.daily_history_count;
}

bool stats_get_daily_history_entry(int day_ago, int* seconds_out, int* sessions_out) {
    char date[11];
    stats_get_date_for_day_ago(day_ago, date, sizeof(date));

    DailyHistoryEntry* entry = stats_find_history_by_date(date);
    if (entry) {
        if (seconds_out) {
            *seconds_out = entry->usage_seconds;
        }
        if (sessions_out) {
            *sessions_out = entry->sessions;
        }
        return true;
    }

    if (seconds_out) {
        *seconds_out = 0;
    }
    if (sessions_out) {
        *sessions_out = 0;
    }
    return false;
}

int stats_get_usage_by_app(AppUsageData* data, int max_apps) {
    if (!data || max_apps <= 0) {
        return 0;
    }

    int out_count = 0;
    int total_seconds = 0;

    for (int i = 0; i < g_stats.app_usage_count && out_count < max_apps; i++) {
        if (process_monitor_is_system_process(g_stats.app_usage[i].app_name)) {
            continue;
        }
        total_seconds += g_stats.app_usage[i].seconds;
        out_count++;
    }

    if (out_count <= 0) {
        return 0;
    }

    int idx = 0;
    for (int i = 0; i < g_stats.app_usage_count && idx < max_apps; i++) {
        if (process_monitor_is_system_process(g_stats.app_usage[i].app_name)) {
            continue;
        }

        strncpy(data[idx].app_name, g_stats.app_usage[i].app_name, sizeof(data[idx].app_name) - 1);
        data[idx].seconds = g_stats.app_usage[i].seconds;
        idx++;
    }

    if (total_seconds <= 0) {
        total_seconds = 1;
    }

    for (int i = 0; i < idx; i++) {
        data[i].percentage = (double)data[i].seconds * 100.0 / (double)total_seconds;
    }

    return idx;
}
