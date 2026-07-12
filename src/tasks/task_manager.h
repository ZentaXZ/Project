#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "task.h"
#include <stdbool.h>

/**
 * Initialize task manager and load tasks from storage
 */
bool task_manager_init(void);

/**
 * Create a new regular task
 */
Task* task_manager_create(const char* title, const char* description, Priority priority);

/**
 * Mark task as completed and move to completed_tasks.json
 */
bool task_manager_complete(const char* task_id);

/**
 * Delete a task
 */
bool task_manager_delete(const char* task_id);

/**
 * Get all active tasks
 */
Task** task_manager_get_all(int* out_count);

/**
 * Add subtask to a task
 */
bool task_manager_add_subtask(const char* task_id, const char* subtask_title);

/**
 * Toggle subtask completion status
 */
bool task_manager_toggle_subtask(const char* task_id, const char* subtask_id);

/**
 * Create a new daily task
 */
bool daily_task_manager_create(const char* title, RecurrenceType recurrence, int recurrence_value);

/**
 * Mark daily task as completed (updates streak, does NOT move to history)
 */
bool daily_task_manager_complete(const char* daily_task_id);

/**
 * Get all daily tasks
 */
DailyTask** daily_task_manager_get_all(int* out_count);

/**
 * Reset daily tasks if new day (call on startup)
 */
bool daily_task_manager_reset_if_new_day(void);

#endif // TASK_MANAGER_H
