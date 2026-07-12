#include "task_manager.h"
#include "task_storage.h"
#include "../utils/id_utils.h"
#include "../utils/time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Task* g_tasks = NULL;
static int g_task_count = 0;
static DailyTask* g_daily_tasks = NULL;
static int g_daily_count = 0;

bool task_manager_init(void) {
    task_storage_load_tasks(&g_tasks, &g_task_count);
    task_storage_load_daily(&g_daily_tasks, &g_daily_count);
    return true;
}

Task* task_manager_create(const char* title, const char* description, Priority priority) {
    if (!title) return NULL;

    Task* new_tasks = realloc(g_tasks, sizeof(Task) * (g_task_count + 1));
    if (!new_tasks) return NULL;

    g_tasks = new_tasks;
    Task* new_task = &g_tasks[g_task_count];
    memset(new_task, 0, sizeof(Task));

    generate_uuid(new_task->id);
    strncpy(new_task->title, title, 127);
    strncpy(new_task->description, description ? description : "", 511);
    new_task->priority = priority;
    new_task->created_at = time(NULL);
    new_task->completed = false;

    g_task_count++;
    task_storage_save_tasks(g_tasks, g_task_count);

    return new_task;
}

bool task_manager_complete(const char* task_id) {
    if (!task_id) return false;

    for (int i = 0; i < g_task_count; i++) {
        if (strcmp(g_tasks[i].id, task_id) == 0) {
            task_storage_append_completed(task_id, g_tasks[i].title, "normal");
            g_tasks[i].completed = true;
            task_storage_save_tasks(g_tasks, g_task_count);
            return true;
        }
    }
    return false;
}

bool task_manager_delete(const char* task_id) {
    if (!task_id) return false;

    for (int i = 0; i < g_task_count; i++) {
        if (strcmp(g_tasks[i].id, task_id) == 0) {
            // Remove from array
            for (int j = i; j < g_task_count - 1; j++) {
                g_tasks[j] = g_tasks[j + 1];
            }
            g_task_count--;
            task_storage_save_tasks(g_tasks, g_task_count);
            return true;
        }
    }
    return false;
}

Task** task_manager_get_all(int* out_count) {
    if (out_count) *out_count = g_task_count;
    return (Task**)g_tasks;
}

bool task_manager_add_subtask(const char* task_id, const char* subtask_title) {
    if (!task_id || !subtask_title) return false;

    for (int i = 0; i < g_task_count; i++) {
        if (strcmp(g_tasks[i].id, task_id) == 0) {
            if (g_tasks[i].subtask_count >= 10) return false;

            Subtask* new_subtask = &g_tasks[i].subtasks[g_tasks[i].subtask_count];
            generate_uuid(new_subtask->id);
            strncpy(new_subtask->title, subtask_title, 127);
            new_subtask->done = false;
            g_tasks[i].subtask_count++;
            task_storage_save_tasks(g_tasks, g_task_count);
            return true;
        }
    }
    return false;
}

bool task_manager_toggle_subtask(const char* task_id, const char* subtask_id) {
    if (!task_id || !subtask_id) return false;

    for (int i = 0; i < g_task_count; i++) {
        if (strcmp(g_tasks[i].id, task_id) == 0) {
            for (int j = 0; j < g_tasks[i].subtask_count; j++) {
                if (strcmp(g_tasks[i].subtasks[j].id, subtask_id) == 0) {
                    g_tasks[i].subtasks[j].done = !g_tasks[i].subtasks[j].done;
                    task_storage_save_tasks(g_tasks, g_task_count);
                    return true;
                }
            }
        }
    }
    return false;
}

bool daily_task_manager_create(const char* title, RecurrenceType recurrence, int recurrence_value) {
    if (!title) return false;

    DailyTask* new_tasks = realloc(g_daily_tasks, sizeof(DailyTask) * (g_daily_count + 1));
    if (!new_tasks) return false;

    g_daily_tasks = new_tasks;
    DailyTask* new_task = &g_daily_tasks[g_daily_count];
    memset(new_task, 0, sizeof(DailyTask));

    generate_uuid(new_task->id);
    strncpy(new_task->title, title, 127);
    new_task->recurrence = recurrence;
    new_task->recurrence_value = recurrence_value;
    new_task->streak_count = 0;

    g_daily_count++;
    task_storage_save_daily(g_daily_tasks, g_daily_count);

    return true;
}

bool daily_task_manager_complete(const char* daily_task_id) {
    if (!daily_task_id) return false;

    for (int i = 0; i < g_daily_count; i++) {
        if (strcmp(g_daily_tasks[i].id, daily_task_id) == 0) {
            g_daily_tasks[i].last_completed_date = time(NULL);
            g_daily_tasks[i].streak_count++;
            task_storage_append_completed(daily_task_id, g_daily_tasks[i].title, "daily");
            task_storage_save_daily(g_daily_tasks, g_daily_count);
            return true;
        }
    }
    return false;
}

DailyTask** daily_task_manager_get_all(int* out_count) {
    if (out_count) *out_count = g_daily_count;
    return (DailyTask**)g_daily_tasks;
}

bool daily_task_manager_reset_if_new_day(void) {
    // TODO: Check if new day and reset daily tasks
    return true;
}
