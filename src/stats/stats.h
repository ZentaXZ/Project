#ifndef STATS_H
#define STATS_H

#include <stdbool.h>

typedef struct {
    char app_name[64];
    int seconds;
    double percentage;
} AppUsageData;

bool stats_init(void);
void stats_register_task_completed(void);
int stats_get_current_streak(void);
int stats_get_longest_streak(void);
void stats_add_screen_time(int minutes);
void stats_add_distraction_time(int minutes);
const char* stats_get_daily_summary(void);
int stats_get_points(void);
int stats_get_level(void);

int stats_get_total_usage_seconds(void);
int stats_get_total_sessions(void);
int stats_get_daily_average_seconds(void);
bool stats_get_daily_history_entry(int day_ago, int* seconds_out, int* sessions_out);
int stats_get_usage_by_app(AppUsageData* data, int max_apps);

#endif // STATS_H
