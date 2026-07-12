#ifndef STATS_H
#define STATS_H

#include <stdbool.h>

/**
 * Initialize stats system
 */
bool stats_init(void);

/**
 * Register task completion
 */
void stats_register_task_completed(void);

/**
 * Get current streak
 */
int stats_get_current_streak(void);

/**
 * Get longest streak
 */
int stats_get_longest_streak(void);

/**
 * Add screen time (minutes)
 */
void stats_add_screen_time(int minutes);

/**
 * Add distraction time (minutes)
 */
void stats_add_distraction_time(int minutes);

/**
 * Get daily summary as formatted string
 */
const char* stats_get_daily_summary(void);

/**
 * Get points
 */
int stats_get_points(void);

/**
 * Get level
 */
int stats_get_level(void);

#endif // STATS_H
