#include "usage_limits.h"
#include "process_monitor.h"
#include "../utils/json_utils.h"
#include "../utils/time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MAX_DAILY_RULES 32
#define MAX_USAGE_TRACKED 32
#define MAX_SESSIONS 16
#define POLL_SECONDS_DEFAULT 5

typedef struct {
    char process_name[64];
    int seconds_used;
} DailyUsageEntry;

typedef struct {
    char process_name[64];
    time_t ends_at;
} SessionEntry;

static DailyLimitRule g_daily_rules[MAX_DAILY_RULES];
static int g_daily_rule_count = 0;

static char g_usage_date[11] = "";
static DailyUsageEntry g_usage[MAX_USAGE_TRACKED];
static int g_usage_count = 0;

static SessionEntry g_sessions[MAX_SESSIONS];
static int g_session_count = 0;

static void usage_limits_get_today(char* out, size_t out_size) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(out, out_size, "%Y-%m-%d", tm_info);
}

static int usage_limits_find_daily_rule_index(const char* process_name) {
    for (int i = 0; i < g_daily_rule_count; i++) {
        if (_stricmp(g_daily_rules[i].process_name, process_name) == 0) {
            return i;
        }
    }
    return -1;
}

static DailyUsageEntry* usage_limits_find_usage_entry(const char* process_name) {
    for (int i = 0; i < g_usage_count; i++) {
        if (_stricmp(g_usage[i].process_name, process_name) == 0) {
            return &g_usage[i];
        }
    }
    return NULL;
}

static DailyUsageEntry* usage_limits_ensure_usage_entry(const char* process_name) {
    DailyUsageEntry* existing = usage_limits_find_usage_entry(process_name);
    if (existing) {
        return existing;
    }

    if (g_usage_count >= MAX_USAGE_TRACKED) {
        return NULL;
    }

    DailyUsageEntry* entry = &g_usage[g_usage_count++];
    memset(entry, 0, sizeof(DailyUsageEntry));
    strncpy(entry->process_name, process_name, sizeof(entry->process_name) - 1);
    return entry;
}

static SessionEntry* usage_limits_find_session(const char* process_name) {
    for (int i = 0; i < g_session_count; i++) {
        if (_stricmp(g_sessions[i].process_name, process_name) == 0) {
            return &g_sessions[i];
        }
    }
    return NULL;
}

static void usage_limits_reset_usage_if_new_day(void) {
    char today[11];
    usage_limits_get_today(today, sizeof(today));

    if (g_usage_date[0] != '\0' && strcmp(g_usage_date, today) == 0) {
        return;
    }

    strncpy(g_usage_date, today, sizeof(g_usage_date) - 1);
    g_usage_date[sizeof(g_usage_date) - 1] = '\0';
    g_usage_count = 0;
}

static void usage_limits_prune_expired_sessions(void) {
    time_t now = time(NULL);
    int write = 0;

    for (int i = 0; i < g_session_count; i++) {
        if (g_sessions[i].ends_at > now) {
            if (write != i) {
                g_sessions[write] = g_sessions[i];
            }
            write++;
        }
    }

    g_session_count = write;
}

static void usage_limits_remove_session_at(int index) {
    for (int i = index; i < g_session_count - 1; i++) {
        g_sessions[i] = g_sessions[i + 1];
    }
    g_session_count--;
}

static bool usage_limits_load_daily_rules(void) {
    g_daily_rule_count = 0;

    cJSON* root = json_utils_load_file("data/daily_limits.json");
    if (!root) {
        return false;
    }

    cJSON* limits = cJSON_GetObjectItem(root, "limits");
    if (!cJSON_IsArray(limits)) {
        cJSON_Delete(root);
        return false;
    }

    cJSON* item = NULL;
    cJSON_ArrayForEach(item, limits) {
        if (g_daily_rule_count >= MAX_DAILY_RULES) {
            break;
        }

        cJSON* process_name = cJSON_GetObjectItem(item, "process_name");
        cJSON* max_minutes = cJSON_GetObjectItem(item, "max_minutes_per_day");

        if (!process_name || !process_name->valuestring) {
            continue;
        }

        DailyLimitRule* rule = &g_daily_rules[g_daily_rule_count++];
        memset(rule, 0, sizeof(DailyLimitRule));
        strncpy(rule->process_name, process_name->valuestring, sizeof(rule->process_name) - 1);
        rule->max_minutes_per_day = (max_minutes && cJSON_IsNumber(max_minutes)) ? max_minutes->valueint : 60;
        if (rule->max_minutes_per_day < 1) {
            rule->max_minutes_per_day = 1;
        }
    }

    cJSON_Delete(root);
    return true;
}

static bool usage_limits_load_usage(void) {
    g_usage_count = 0;
    g_usage_date[0] = '\0';

    cJSON* root = json_utils_load_file("data/daily_usage.json");
    if (!root) {
        usage_limits_reset_usage_if_new_day();
        return false;
    }

    cJSON* date = cJSON_GetObjectItem(root, "date");
    if (date && date->valuestring) {
        strncpy(g_usage_date, date->valuestring, sizeof(g_usage_date) - 1);
    }

    char today[11];
    usage_limits_get_today(today, sizeof(today));
    if (g_usage_date[0] == '\0' || strcmp(g_usage_date, today) != 0) {
        cJSON_Delete(root);
        usage_limits_reset_usage_if_new_day();
        return true;
    }

    cJSON* apps = cJSON_GetObjectItem(root, "apps");
    if (cJSON_IsArray(apps)) {
        cJSON* item = NULL;
        cJSON_ArrayForEach(item, apps) {
            if (g_usage_count >= MAX_USAGE_TRACKED) {
                break;
            }

            cJSON* process_name = cJSON_GetObjectItem(item, "process_name");
            cJSON* seconds_used = cJSON_GetObjectItem(item, "seconds_used");

            if (!process_name || !process_name->valuestring) {
                continue;
            }

            DailyUsageEntry* entry = &g_usage[g_usage_count++];
            memset(entry, 0, sizeof(DailyUsageEntry));
            strncpy(entry->process_name, process_name->valuestring, sizeof(entry->process_name) - 1);
            entry->seconds_used = (seconds_used && cJSON_IsNumber(seconds_used)) ? seconds_used->valueint : 0;
        }
    }

    cJSON_Delete(root);
    return true;
}

static bool usage_limits_load_sessions(void) {
    g_session_count = 0;

    cJSON* root = json_utils_load_file("data/session_timers.json");
    if (!root) {
        return false;
    }

    cJSON* sessions = cJSON_GetObjectItem(root, "sessions");
    if (!cJSON_IsArray(sessions)) {
        cJSON_Delete(root);
        return false;
    }

    time_t now = time(NULL);
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, sessions) {
        if (g_session_count >= MAX_SESSIONS) {
            break;
        }

        cJSON* process_name = cJSON_GetObjectItem(item, "process_name");
        cJSON* ends_at = cJSON_GetObjectItem(item, "ends_at");

        if (!process_name || !process_name->valuestring || !ends_at || !cJSON_IsNumber(ends_at)) {
            continue;
        }

        if ((time_t)ends_at->valueint <= now) {
            continue;
        }

        SessionEntry* session = &g_sessions[g_session_count++];
        memset(session, 0, sizeof(SessionEntry));
        strncpy(session->process_name, process_name->valuestring, sizeof(session->process_name) - 1);
        session->ends_at = (time_t)ends_at->valueint;
    }

    cJSON_Delete(root);
    return true;
}

static bool usage_limits_save_daily_rules(void) {
    cJSON* root = cJSON_CreateObject();
    cJSON* limits = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "limits", limits);

    for (int i = 0; i < g_daily_rule_count; i++) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "process_name", g_daily_rules[i].process_name);
        cJSON_AddNumberToObject(item, "max_minutes_per_day", g_daily_rules[i].max_minutes_per_day);
        cJSON_AddItemToArray(limits, item);
    }

    bool ok = json_utils_save_file("data/daily_limits.json", root);
    cJSON_Delete(root);
    return ok;
}

static bool usage_limits_save_usage(void) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "date", g_usage_date);
    cJSON* apps = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "apps", apps);

    for (int i = 0; i < g_usage_count; i++) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "process_name", g_usage[i].process_name);
        cJSON_AddNumberToObject(item, "seconds_used", g_usage[i].seconds_used);
        cJSON_AddItemToArray(apps, item);
    }

    bool ok = json_utils_save_file("data/daily_usage.json", root);
    cJSON_Delete(root);
    return ok;
}

static bool usage_limits_save_sessions(void) {
    cJSON* root = cJSON_CreateObject();
    cJSON* sessions = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "sessions", sessions);

    for (int i = 0; i < g_session_count; i++) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "process_name", g_sessions[i].process_name);
        cJSON_AddNumberToObject(item, "ends_at", (double)g_sessions[i].ends_at);
        cJSON_AddItemToArray(sessions, item);
    }

    bool ok = json_utils_save_file("data/session_timers.json", root);
    cJSON_Delete(root);
    return ok;
}

bool usage_limits_load(void) {
    usage_limits_load_daily_rules();
    usage_limits_load_usage();
    usage_limits_load_sessions();
    usage_limits_reset_usage_if_new_day();
    usage_limits_prune_expired_sessions();
    return true;
}

bool usage_limits_save(void) {
    bool rules_ok = usage_limits_save_daily_rules();
    bool usage_ok = usage_limits_save_usage();
    bool sessions_ok = usage_limits_save_sessions();
    return rules_ok && usage_ok && sessions_ok;
}

bool usage_limits_add_daily_rule(const char* process_name, int max_minutes_per_day) {
    if (!process_name || process_name[0] == '\0' || g_daily_rule_count >= MAX_DAILY_RULES) {
        return false;
    }

    if (max_minutes_per_day < 1) {
        max_minutes_per_day = 1;
    }

    int existing = usage_limits_find_daily_rule_index(process_name);
    if (existing >= 0) {
        g_daily_rules[existing].max_minutes_per_day = max_minutes_per_day;
        return true;
    }

    DailyLimitRule* rule = &g_daily_rules[g_daily_rule_count++];
    memset(rule, 0, sizeof(DailyLimitRule));
    strncpy(rule->process_name, process_name, sizeof(rule->process_name) - 1);
    rule->max_minutes_per_day = max_minutes_per_day;
    return true;
}

bool usage_limits_remove_daily_rule(int index) {
    if (index < 0 || index >= g_daily_rule_count) {
        return false;
    }

    for (int i = index; i < g_daily_rule_count - 1; i++) {
        g_daily_rules[i] = g_daily_rules[i + 1];
    }
    g_daily_rule_count--;
    return true;
}

int usage_limits_get_daily_rule_count(void) {
    return g_daily_rule_count;
}

bool usage_limits_get_daily_rule(int index, DailyLimitRule* out) {
    if (!out || index < 0 || index >= g_daily_rule_count) {
        return false;
    }
    *out = g_daily_rules[index];
    return true;
}

bool usage_limits_start_session(const char* process_name, int minutes) {
    if (!process_name || process_name[0] == '\0' || minutes < 1) {
        return false;
    }

    time_t ends_at = time(NULL) + (time_t)minutes * 60;
    SessionEntry* session = usage_limits_find_session(process_name);

    if (session) {
        session->ends_at = ends_at;
        usage_limits_save_sessions();
        return true;
    }

    if (g_session_count >= MAX_SESSIONS) {
        return false;
    }

    session = &g_sessions[g_session_count++];
    memset(session, 0, sizeof(SessionEntry));
    strncpy(session->process_name, process_name, sizeof(session->process_name) - 1);
    session->ends_at = ends_at;
    usage_limits_save_sessions();
    return true;
}

bool usage_limits_cancel_session(const char* process_name) {
    if (!process_name) {
        return false;
    }

    for (int i = 0; i < g_session_count; i++) {
        if (_stricmp(g_sessions[i].process_name, process_name) == 0) {
            usage_limits_remove_session_at(i);
            usage_limits_save_sessions();
            return true;
        }
    }
    return false;
}

bool usage_limits_has_active_session(const char* process_name) {
    if (!process_name) {
        return false;
    }

    SessionEntry* session = usage_limits_find_session(process_name);
    if (!session) {
        return false;
    }

    return session->ends_at > time(NULL);
}

int usage_limits_get_session_seconds_left(const char* process_name) {
    SessionEntry* session = usage_limits_find_session(process_name);
    if (!session) {
        return 0;
    }

    time_t now = time(NULL);
    if (session->ends_at <= now) {
        return 0;
    }

    return (int)(session->ends_at - now);
}

int usage_limits_get_daily_seconds_used(const char* process_name) {
    if (!process_name) {
        return 0;
    }

    usage_limits_reset_usage_if_new_day();
    DailyUsageEntry* entry = usage_limits_find_usage_entry(process_name);
    return entry ? entry->seconds_used : 0;
}

int usage_limits_get_daily_seconds_remaining(const char* process_name) {
    int rule_index = usage_limits_find_daily_rule_index(process_name);
    if (rule_index < 0) {
        return -1;
    }

    int max_seconds = g_daily_rules[rule_index].max_minutes_per_day * 60;
    int used = usage_limits_get_daily_seconds_used(process_name);
    int remaining = max_seconds - used;
    return remaining > 0 ? remaining : 0;
}

bool usage_limits_is_daily_exceeded(const char* process_name) {
    int rule_index = usage_limits_find_daily_rule_index(process_name);
    if (rule_index < 0) {
        return false;
    }

    int max_seconds = g_daily_rules[rule_index].max_minutes_per_day * 60;
    int used = usage_limits_get_daily_seconds_used(process_name);
    return used >= max_seconds;
}

bool usage_limits_is_blocked(const char* process_name) {
    if (!process_name) {
        return false;
    }

    if (usage_limits_has_active_session(process_name)) {
        return false;
    }

    if (usage_limits_is_daily_exceeded(process_name)) {
        return true;
    }

    SessionEntry* session = usage_limits_find_session(process_name);
    if (session && session->ends_at <= time(NULL)) {
        return true;
    }

    return false;
}

void usage_limits_tick(int elapsed_seconds) {
    if (elapsed_seconds <= 0) {
        elapsed_seconds = POLL_SECONDS_DEFAULT;
    }

    usage_limits_reset_usage_if_new_day();

    for (int i = 0; i < g_daily_rule_count; i++) {
        const char* process_name = g_daily_rules[i].process_name;
        if (!process_monitor_is_running(process_name)) {
            continue;
        }

        DailyUsageEntry* entry = usage_limits_ensure_usage_entry(process_name);
        if (!entry) {
            continue;
        }

        entry->seconds_used += elapsed_seconds;
    }

    usage_limits_save_usage();
}

int usage_limits_get_session_count(void) {
    return g_session_count;
}

bool usage_limits_get_session_info(int index, const char** process_name_out, time_t* ends_at_out) {
    if (index < 0 || index >= g_session_count) {
        return false;
    }

    if (process_name_out) {
        *process_name_out = g_sessions[index].process_name;
    }
    if (ends_at_out) {
        *ends_at_out = g_sessions[index].ends_at;
    }
    return true;
}
