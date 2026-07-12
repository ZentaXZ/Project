#ifndef TASK_H
#define TASK_H

#include <time.h>
#include <stdbool.h>

typedef enum {
    PRIORITY_LOW,
    PRIORITY_MEDIUM,
    PRIORITY_HIGH,
    PRIORITY_URGENT
} Priority;

typedef enum {
    RECUR_DAILY,
    RECUR_WEEKLY,
    RECUR_EVERY_X_DAYS
} RecurrenceType;

typedef struct {
    char id[37];        // UUID as string
    char title[128];
    bool done;
} Subtask;

typedef struct {
    char id[37];
    char title[128];
    char description[512];
    Priority priority;
    char tags[5][32];       // up to 5 tags
    int tag_count;
    time_t due_date;        // 0 if none
    Subtask subtasks[10];   // up to 10 subtasks
    int subtask_count;
    time_t created_at;
    bool completed;
} Task;

typedef struct {
    char id[37];
    char title[128];
    char description[512];
    RecurrenceType recurrence;
    int recurrence_value;
    time_t last_completed_date; // 0 if never
    int streak_count;
} DailyTask;

#endif // TASK_H
