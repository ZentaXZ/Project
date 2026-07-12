#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <time.h>
#include <stdbool.h>

/**
 * Get current time as ISO8601 string.
 * @return Dynamically allocated string (caller must free). Format: YYYY-MM-DDTHH:MM:SS
 */
char* time_utils_now_iso8601(void);

/**
 * Check if two time_t values are on the same day.
 * @param a First timestamp
 * @param b Second timestamp
 * @return true if same day, false otherwise
 */
bool time_utils_is_same_day(time_t a, time_t b);

/**
 * Check if a date is yesterday relative to a reference time.
 * @param date Timestamp to check
 * @param reference Reference timestamp
 * @return true if date is yesterday
 */
bool time_utils_is_yesterday(time_t date, time_t reference);

/**
 * Get minutes since midnight.
 * @return Minutes (0-1439)
 */
int time_utils_minutes_since_midnight(void);

#endif // TIME_UTILS_H
