#ifndef USAGE_LIMITS_H
#define USAGE_LIMITS_H

#include <stdbool.h>
#include <time.h>

typedef struct {
    char process_name[64];
    int max_minutes_per_day;
} DailyLimitRule;

typedef struct {
    char process_name[64];
    int seconds_remaining;
} SessionTimerStatus;

bool usage_limits_load(void);
bool usage_limits_save(void);

bool usage_limits_add_daily_rule(const char* process_name, int max_minutes_per_day);
bool usage_limits_remove_daily_rule(int index);
int  usage_limits_get_daily_rule_count(void);
bool usage_limits_get_daily_rule(int index, DailyLimitRule* out);

bool usage_limits_start_session(const char* process_name, int minutes);
bool usage_limits_cancel_session(const char* process_name);
bool usage_limits_has_active_session(const char* process_name);
int  usage_limits_get_session_seconds_left(const char* process_name);

int  usage_limits_get_daily_seconds_used(const char* process_name);
int  usage_limits_get_daily_seconds_remaining(const char* process_name);
bool usage_limits_is_daily_exceeded(const char* process_name);

bool usage_limits_is_blocked(const char* process_name);
void usage_limits_tick(int elapsed_seconds);
int  usage_limits_get_session_count(void);
bool usage_limits_get_session_info(int index, const char** process_name_out, time_t* ends_at_out);

#endif
