#include "schedule.h"
#include "../utils/json_utils.h"
#include "../utils/time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SCHEDULE_RULES 64

static AppSchedule g_blocklist[MAX_SCHEDULE_RULES];
static int g_blocklist_count = 0;
static AppSchedule g_whitelist[MAX_SCHEDULE_RULES];
static int g_whitelist_count = 0;

static int schedule_parse_time_string(const char* time_str);

static int schedule_parse_time_string(const char* time_str) {
    if (!time_str) {
        return 0;
    }

    int hour = 0;
    int minute = 0;
    if (sscanf(time_str, "%d:%d", &hour, &minute) != 2) {
        return 0;
    }

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        return 0;
    }

    return hour * 60 + minute;
}

static bool schedule_load_file(const char* path, AppSchedule* rules, int* count) {
    *count = 0;

    cJSON* root = json_utils_load_file(path);
    if (!root) {
        return false;
    }

    cJSON* apps = cJSON_GetObjectItem(root, "apps");
    if (!cJSON_IsArray(apps)) {
        cJSON_Delete(root);
        return false;
    }

    cJSON* item = NULL;
    cJSON_ArrayForEach(item, apps) {
        if (*count >= MAX_SCHEDULE_RULES) {
            break;
        }

        cJSON* process_name = cJSON_GetObjectItem(item, "process_name");
        cJSON* allowed_from = cJSON_GetObjectItem(item, "allowed_from");
        cJSON* allowed_to = cJSON_GetObjectItem(item, "allowed_to");

        if (!process_name || !process_name->valuestring) {
            continue;
        }

        AppSchedule* rule = &rules[*count];
        memset(rule, 0, sizeof(AppSchedule));
        strncpy(rule->process_name, process_name->valuestring, sizeof(rule->process_name) - 1);
        rule->allowed_from_minutes = schedule_parse_time_string(
            allowed_from && allowed_from->valuestring ? allowed_from->valuestring : "00:00");
        rule->allowed_to_minutes = schedule_parse_time_string(
            allowed_to && allowed_to->valuestring ? allowed_to->valuestring : "23:59");
        (*count)++;
    }

    cJSON_Delete(root);
    return true;
}

static bool schedule_save_file(const char* path, AppSchedule* rules, int count) {
    cJSON* root = cJSON_CreateObject();
    cJSON* apps = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "apps", apps);

    for (int i = 0; i < count; i++) {
        char from_str[8];
        char to_str[8];
        schedule_minutes_to_time_string(rules[i].allowed_from_minutes, from_str, sizeof(from_str));
        schedule_minutes_to_time_string(rules[i].allowed_to_minutes, to_str, sizeof(to_str));

        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "process_name", rules[i].process_name);
        cJSON_AddStringToObject(item, "allowed_from", from_str);
        cJSON_AddStringToObject(item, "allowed_to", to_str);
        cJSON_AddItemToArray(apps, item);
    }

    bool ok = json_utils_save_file(path, root);
    cJSON_Delete(root);
    return ok;
}

static const AppSchedule* schedule_find_blocklist_rule(const char* process_name) {
    if (!process_name) {
        return NULL;
    }

    for (int i = 0; i < g_blocklist_count; i++) {
        if (_stricmp(g_blocklist[i].process_name, process_name) == 0) {
            return &g_blocklist[i];
        }
    }
    return NULL;
}

static bool schedule_minutes_in_window(int now, int from, int to) {
    if (from <= to) {
        return now >= from && now <= to;
    }
    return now >= from || now <= to;
}

void schedule_minutes_to_time_string(int minutes, char* out, size_t out_size) {
    if (!out || out_size == 0) {
        return;
    }

    if (minutes < 0) {
        minutes = 0;
    }
    if (minutes > 1439) {
        minutes = 1439;
    }

    snprintf(out, out_size, "%02d:%02d", minutes / 60, minutes % 60);
}

bool schedule_load(void) {
    g_blocklist_count = 0;
    g_whitelist_count = 0;
    schedule_load_file("data/app_blocklist.json", g_blocklist, &g_blocklist_count);
    schedule_load_file("data/app_whitelist.json", g_whitelist, &g_whitelist_count);
    return true;
}

bool schedule_is_allowed_now(const char* process_name) {
    const AppSchedule* rule = schedule_find_blocklist_rule(process_name);
    if (!rule) {
        return true;
    }

    int now = time_utils_minutes_since_midnight();
    return schedule_minutes_in_window(now, rule->allowed_from_minutes, rule->allowed_to_minutes);
}

bool schedule_add_rule(const char* process_name, int from_minutes, int to_minutes) {
    if (!process_name || process_name[0] == '\0' || g_blocklist_count >= MAX_SCHEDULE_RULES) {
        return false;
    }

    AppSchedule* rule = &g_blocklist[g_blocklist_count];
    memset(rule, 0, sizeof(AppSchedule));
    strncpy(rule->process_name, process_name, sizeof(rule->process_name) - 1);
    rule->allowed_from_minutes = from_minutes;
    rule->allowed_to_minutes = to_minutes;
    g_blocklist_count++;
    return true;
}

bool schedule_remove_blocklist_rule(int index) {
    if (index < 0 || index >= g_blocklist_count) {
        return false;
    }

    for (int i = index; i < g_blocklist_count - 1; i++) {
        g_blocklist[i] = g_blocklist[i + 1];
    }
    g_blocklist_count--;
    return true;
}

bool schedule_remove_whitelist_rule(int index) {
    if (index < 0 || index >= g_whitelist_count) {
        return false;
    }

    for (int i = index; i < g_whitelist_count - 1; i++) {
        g_whitelist[i] = g_whitelist[i + 1];
    }
    g_whitelist_count--;
    return true;
}

bool schedule_save(void) {
    bool block_ok = schedule_save_file("data/app_blocklist.json", g_blocklist, g_blocklist_count);
    bool white_ok = schedule_save_file("data/app_whitelist.json", g_whitelist, g_whitelist_count);
    return block_ok && white_ok;
}

int schedule_get_blocklist_count(void) {
    return g_blocklist_count;
}

int schedule_get_whitelist_count(void) {
    return g_whitelist_count;
}

bool schedule_get_blocklist_rule(int index, AppSchedule* out) {
    if (!out || index < 0 || index >= g_blocklist_count) {
        return false;
    }
    *out = g_blocklist[index];
    return true;
}

bool schedule_get_whitelist_rule(int index, AppSchedule* out) {
    if (!out || index < 0 || index >= g_whitelist_count) {
        return false;
    }
    *out = g_whitelist[index];
    return true;
}
