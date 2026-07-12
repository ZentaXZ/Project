#ifndef TASK_STORAGE_H
#define TASK_STORAGE_H

#include "task.h"
#include <stdbool.h>

/**
 * Load all regular tasks from data/tasks.json
 */
bool task_storage_load_tasks(Task** out_tasks, int* out_count);

/**
 * Save all regular tasks to data/tasks.json
 */
bool task_storage_save_tasks(Task* tasks, int count);

/**
 * Load all daily tasks from data/daily_tasks.json
 */
bool task_storage_load_daily(DailyTask** out_tasks, int* out_count);

/**
 * Save all daily tasks to data/daily_tasks.json
 */
bool task_storage_save_daily(DailyTask* tasks, int count);

/**
 * Append a completed task to data/completed_tasks.json
 */
bool task_storage_append_completed(const char* task_id, const char* task_title, const char* type);

/**
 * Remove a completed task entry by id from data/completed_tasks.json
 */
bool task_storage_remove_completed(const char* task_id);

#endif // TASK_STORAGE_H
