#include "config.h"
#include "json_utils.h"
#include <stdio.h>
#include <string.h>

#define CONFIG_STAYFREE_PATH_MAX 512
#define CONFIG_STAYFREE_DEFAULT_PATH \
    "C:\\Program Files\\WindowsApps\\37081StayFreeApps.StayFree3_3.4.2.0_x64__fqhk48m1tsma0\\app\\StayFree.exe"

typedef struct {
    int reward_minutes_per_task;
    int base_reward_minutes_per_task;
    bool sound_enabled;
    double punishment_multiplier;
    char theme[16];
    bool punishment_applied_today;
    bool watchdog_enabled;
    bool close_guard_enabled;
    char stayfree_exe_path[CONFIG_STAYFREE_PATH_MAX];
} Config;

static Config g_config = {
    .reward_minutes_per_task = 15,
    .base_reward_minutes_per_task = 15,
    .sound_enabled = true,
    .punishment_multiplier = 0.5,
    .theme = "light",
    .punishment_applied_today = false,
    .watchdog_enabled = true,
    .close_guard_enabled = true,
    .stayfree_exe_path = CONFIG_STAYFREE_DEFAULT_PATH
};

bool config_load(void) {
    cJSON* root = json_utils_load_file("data/config.json");
    if (!root) {
        return false;
    }

    cJSON* reward = cJSON_GetObjectItem(root, "reward_minutes_per_task");
    if (reward && cJSON_IsNumber(reward)) {
        g_config.reward_minutes_per_task = reward->valueint;
        g_config.base_reward_minutes_per_task = reward->valueint;
    }

    cJSON* sound = cJSON_GetObjectItem(root, "sound_enabled");
    if (sound && cJSON_IsBool(sound)) {
        g_config.sound_enabled = cJSON_IsTrue(sound);
    }

    cJSON* multiplier = cJSON_GetObjectItem(root, "punishment_multiplier");
    if (multiplier && cJSON_IsNumber(multiplier)) {
        g_config.punishment_multiplier = multiplier->valuedouble;
    }

    cJSON* theme = cJSON_GetObjectItem(root, "theme");
    if (theme && theme->valuestring) {
        strncpy(g_config.theme, theme->valuestring, sizeof(g_config.theme) - 1);
    }

    cJSON* watchdog = cJSON_GetObjectItem(root, "watchdog_enabled");
    if (watchdog && cJSON_IsBool(watchdog)) {
        g_config.watchdog_enabled = cJSON_IsTrue(watchdog);
    }

    cJSON* close_guard = cJSON_GetObjectItem(root, "close_guard_enabled");
    if (close_guard && cJSON_IsBool(close_guard)) {
        g_config.close_guard_enabled = cJSON_IsTrue(close_guard);
    }

    cJSON* stayfree_path = cJSON_GetObjectItem(root, "stayfree_exe_path");
    if (stayfree_path && stayfree_path->valuestring && stayfree_path->valuestring[0] != '\0') {
        strncpy(g_config.stayfree_exe_path, stayfree_path->valuestring,
                sizeof(g_config.stayfree_exe_path) - 1);
    }

    cJSON_Delete(root);
    g_config.punishment_applied_today = false;
    return true;
}

bool config_save(void) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "reward_minutes_per_task", g_config.base_reward_minutes_per_task);
    cJSON_AddStringToObject(root, "theme", g_config.theme);
    cJSON_AddBoolToObject(root, "sound_enabled", g_config.sound_enabled);
    cJSON_AddNumberToObject(root, "punishment_multiplier", g_config.punishment_multiplier);
    cJSON_AddBoolToObject(root, "watchdog_enabled", g_config.watchdog_enabled);
    cJSON_AddBoolToObject(root, "close_guard_enabled", g_config.close_guard_enabled);
    cJSON_AddStringToObject(root, "stayfree_exe_path", g_config.stayfree_exe_path);

    bool ok = json_utils_save_file("data/config.json", root);
    cJSON_Delete(root);
    return ok;
}

int config_get_reward_minutes(void) {
    return g_config.reward_minutes_per_task;
}

void config_set_reward_minutes(int minutes) {
    if (minutes < 0) {
        minutes = 0;
    }
    g_config.reward_minutes_per_task = minutes;
    g_config.base_reward_minutes_per_task = minutes;
}

bool config_get_sound_enabled(void) {
    return g_config.sound_enabled;
}

void config_set_sound_enabled(bool enabled) {
    g_config.sound_enabled = enabled;
}

double config_get_punishment_multiplier(void) {
    return g_config.punishment_multiplier;
}

void config_apply_punishment_today(void) {
    if (g_config.punishment_applied_today) {
        return;
    }

    g_config.reward_minutes_per_task =
        (int)(g_config.base_reward_minutes_per_task * g_config.punishment_multiplier);
    if (g_config.reward_minutes_per_task < 1) {
        g_config.reward_minutes_per_task = 1;
    }

    g_config.punishment_applied_today = true;
    printf("[config] Castigo aplicado hoy: reward_minutes=%d (base=%d, mult=%.2f)\n",
           g_config.reward_minutes_per_task,
           g_config.base_reward_minutes_per_task,
           g_config.punishment_multiplier);
}

bool config_get_watchdog_enabled(void) {
    return g_config.watchdog_enabled;
}

void config_set_watchdog_enabled(bool enabled) {
    g_config.watchdog_enabled = enabled;
}

bool config_get_close_guard_enabled(void) {
    return g_config.close_guard_enabled;
}

void config_set_close_guard_enabled(bool enabled) {
    g_config.close_guard_enabled = enabled;
}

const char* config_get_stayfree_exe_path(void) {
    return g_config.stayfree_exe_path;
}

void config_set_stayfree_exe_path(const char* path) {
    if (!path) {
        return;
    }
    strncpy(g_config.stayfree_exe_path, path, sizeof(g_config.stayfree_exe_path) - 1);
    g_config.stayfree_exe_path[sizeof(g_config.stayfree_exe_path) - 1] = '\0';
}
