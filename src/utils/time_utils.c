#include "time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* time_utils_now_iso8601(void) {
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);

    char* buffer = malloc(20); // YYYY-MM-DDTHH:MM:SS + null
    if (buffer) {
        strftime(buffer, 20, "%Y-%m-%dT%H:%M:%S", timeinfo);
    }
    return buffer;
}

bool time_utils_is_same_day(time_t a, time_t b) {
    struct tm* tm_a = localtime(&a);
    struct tm* tm_b = localtime(&b);

    return tm_a->tm_year == tm_b->tm_year &&
           tm_a->tm_yday == tm_b->tm_yday;
}

bool time_utils_is_yesterday(time_t date, time_t reference) {
    time_t one_day = 86400; // seconds in a day
    return time_utils_is_same_day(date, reference - one_day);
}

int time_utils_minutes_since_midnight(void) {
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    return timeinfo->tm_hour * 60 + timeinfo->tm_min;
}
