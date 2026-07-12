#include "task_storage.h"
#include "../utils/json_utils.h"
#include "../utils/time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool task_storage_load_tasks(Task** out_tasks, int* out_count) {
    if (!out_tasks || !out_count) return false;

    cJSON* root = json_utils_load_file("data/tasks.json");
    if (!root) {
        *out_count = 0;
        return false;
    }

    cJSON* tasks_array = cJSON_GetObjectItem(root, "tasks");
    if (!cJSON_IsArray(tasks_array)) {
        cJSON_Delete(root);
        *out_count = 0;
        return false;
    }

    int count = cJSON_GetArraySize(tasks_array);
    Task* tasks = malloc(sizeof(Task) * count);
    if (!tasks) {
        cJSON_Delete(root);
        return false;
    }

    int idx = 0;
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, tasks_array) {
        Task* task = &tasks[idx];
        memset(task, 0, sizeof(Task));

        cJSON* id = cJSON_GetObjectItem(item, "id");
        if (id && id->valuestring) strncpy(task->id, id->valuestring, 36);

        cJSON* title = cJSON_GetObjectItem(item, "title");
        if (title && title->valuestring) strncpy(task->title, title->valuestring, 127);

        cJSON* description = cJSON_GetObjectItem(item, "description");
        if (description && description->valuestring) strncpy(task->description, description->valuestring, 511);

        cJSON* priority = cJSON_GetObjectItem(item, "priority");
        if (priority && priority->valuestring) {
            if (strcmp(priority->valuestring, "high") == 0) task->priority = PRIORITY_HIGH;
            else if (strcmp(priority->valuestring, "urgent") == 0) task->priority = PRIORITY_URGENT;
            else if (strcmp(priority->valuestring, "medium") == 0) task->priority = PRIORITY_MEDIUM;
            else task->priority = PRIORITY_LOW;
        }

        cJSON* completed = cJSON_GetObjectItem(item, "completed");
        task->completed = cJSON_IsTrue(completed);

        // TODO: Load tags, subtasks, due_date, created_at

        idx++;
    }

    *out_tasks = tasks;
    *out_count = count;
    cJSON_Delete(root);
    return true;
}

bool task_storage_save_tasks(Task* tasks, int count) {
    if (!tasks) return false;

    cJSON* root = cJSON_CreateObject();
    cJSON* tasks_array = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "tasks", tasks_array);

    for (int i = 0; i < count; i++) {
        cJSON* task_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(task_obj, "id", tasks[i].id);
        cJSON_AddStringToObject(task_obj, "title", tasks[i].title);
        cJSON_AddStringToObject(task_obj, "description", tasks[i].description);
        cJSON_AddBoolToObject(task_obj, "completed", tasks[i].completed);
        // TODO: Add other fields
        cJSON_AddItemToArray(tasks_array, task_obj);
    }

    bool result = json_utils_save_file("data/tasks.json", root);
    cJSON_Delete(root);
    return result;
}

bool task_storage_load_daily(DailyTask** out_tasks, int* out_count) {
    if (!out_tasks || !out_count) return false;

    cJSON* root = json_utils_load_file("data/daily_tasks.json");
    if (!root) {
        *out_count = 0;
        return false;
    }

    cJSON* daily_array = cJSON_GetObjectItem(root, "daily_tasks");
    if (!cJSON_IsArray(daily_array)) {
        cJSON_Delete(root);
        *out_count = 0;
        return false;
    }

    int count = cJSON_GetArraySize(daily_array);
    DailyTask* tasks = malloc(sizeof(DailyTask) * count);
    if (!tasks) {
        cJSON_Delete(root);
        return false;
    }

    int idx = 0;
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, daily_array) {
        DailyTask* task = &tasks[idx];
        memset(task, 0, sizeof(DailyTask));

        cJSON* id = cJSON_GetObjectItem(item, "id");
        if (id && id->valuestring) strncpy(task->id, id->valuestring, 36);

        cJSON* title = cJSON_GetObjectItem(item, "title");
        if (title && title->valuestring) strncpy(task->title, title->valuestring, 127);

        cJSON* recurrence = cJSON_GetObjectItem(item, "recurrence");
        if (recurrence && recurrence->valuestring) {
            if (strcmp(recurrence->valuestring, "weekly") == 0) task->recurrence = RECUR_WEEKLY;
            else if (strcmp(recurrence->valuestring, "every_x_days") == 0) task->recurrence = RECUR_EVERY_X_DAYS;
            else task->recurrence = RECUR_DAILY;
        }

        cJSON* recurrence_value = cJSON_GetObjectItem(item, "recurrence_value");
        if (recurrence_value && cJSON_IsNumber(recurrence_value)) task->recurrence_value = recurrence_value->valueint;

        cJSON* streak = cJSON_GetObjectItem(item, "streak_count");
        if (streak && cJSON_IsNumber(streak)) task->streak_count = streak->valueint;

        idx++;
    }

    *out_tasks = tasks;
    *out_count = count;
    cJSON_Delete(root);
    return true;
}

bool task_storage_save_daily(DailyTask* tasks, int count) {
    if (!tasks) return false;

    cJSON* root = cJSON_CreateObject();
    cJSON* daily_array = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "daily_tasks", daily_array);

    for (int i = 0; i < count; i++) {
        cJSON* task_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(task_obj, "id", tasks[i].id);
        cJSON_AddStringToObject(task_obj, "title", tasks[i].title);
        cJSON_AddStringToObject(task_obj, "description", tasks[i].description);
        // TODO: Add other fields
        cJSON_AddItemToArray(daily_array, task_obj);
    }

    bool result = json_utils_save_file("data/daily_tasks.json", root);
    cJSON_Delete(root);
    return result;
}

bool task_storage_append_completed(const char* task_id, const char* task_title, const char* type) {
    if (!task_id || !task_title || !type) return false;

    cJSON* root = json_utils_load_file("data/completed_tasks.json");
    if (!root) {
        root = cJSON_CreateObject();
        cJSON_AddArrayToObject(root, "completed");
    }

    cJSON* completed_array = cJSON_GetObjectItem(root, "completed");
    if (!cJSON_IsArray(completed_array)) {
        cJSON_Delete(root);
        return false;
    }

    cJSON* entry = cJSON_CreateObject();
    cJSON_AddStringToObject(entry, "id", task_id);
    cJSON_AddStringToObject(entry, "title", task_title);
    cJSON_AddStringToObject(entry, "type", type);

    char* now_iso = time_utils_now_iso8601();
    cJSON_AddStringToObject(entry, "completed_at", now_iso);
    free(now_iso);

    cJSON_AddItemToArray(completed_array, entry);

    bool result = json_utils_save_file("data/completed_tasks.json", root);
    cJSON_Delete(root);
    return result;
}

bool task_storage_remove_completed(const char* task_id) {
    if (!task_id || task_id[0] == '\0') {
        return false;
    }

    cJSON* root = json_utils_load_file("data/completed_tasks.json");
    if (!root) {
        return false;
    }

    cJSON* completed_array = cJSON_GetObjectItem(root, "completed");
    if (!cJSON_IsArray(completed_array)) {
        cJSON_Delete(root);
        return false;
    }

    bool removed = false;
    int count = cJSON_GetArraySize(completed_array);
    for (int i = count - 1; i >= 0; i--) {
        cJSON* item = cJSON_GetArrayItem(completed_array, i);
        cJSON* id = cJSON_GetObjectItem(item, "id");
        if (id && id->valuestring && strcmp(id->valuestring, task_id) == 0) {
            cJSON_DeleteItemFromArray(completed_array, i);
            removed = true;
            break;
        }
    }

    if (!removed) {
        cJSON_Delete(root);
        return false;
    }

    bool result = json_utils_save_file("data/completed_tasks.json", root);
    cJSON_Delete(root);
    return result;
}
