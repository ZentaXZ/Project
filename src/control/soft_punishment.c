#include "soft_punishment.h"
#include "../utils/config.h"
#include "../utils/json_utils.h"
#include "../utils/time_utils.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static void soft_punishment_get_yesterday_date(char* out, size_t out_size) {
    time_t now = time(NULL);
    time_t yesterday = now - 86400;
    struct tm* tm_yesterday = localtime(&yesterday);
    strftime(out, out_size, "%Y-%m-%d", tm_yesterday);
}

void soft_punishment_check_on_startup(void) {
    char yesterday[11];
    soft_punishment_get_yesterday_date(yesterday, sizeof(yesterday));

    cJSON* root = json_utils_load_file("data/stats.json");
    if (!root) {
        return;
    }

    cJSON* history = cJSON_GetObjectItem(root, "daily_history");
    if (!cJSON_IsArray(history)) {
        cJSON_Delete(root);
        return;
    }

    int tasks_completed_yesterday = -1;
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, history) {
        cJSON* date = cJSON_GetObjectItem(item, "date");
        cJSON* completed = cJSON_GetObjectItem(item, "tasks_completed");

        if (date && date->valuestring && strcmp(date->valuestring, yesterday) == 0 &&
            completed && cJSON_IsNumber(completed)) {
            tasks_completed_yesterday = completed->valueint;
            break;
        }
    }

    cJSON_Delete(root);

    if (tasks_completed_yesterday == 0) {
        printf("[soft_punishment] Ayer (%s) no se completaron tareas. Aplicando castigo.\n", yesterday);
        config_apply_punishment_today();
    }
}
