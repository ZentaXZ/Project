#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char process_name[64];
    int allowed_from_minutes;
    int allowed_to_minutes;
} AppSchedule;

bool schedule_load(void);
bool schedule_is_allowed_now(const char* process_name);
bool schedule_add_rule(const char* process_name, int from_minutes, int to_minutes);
bool schedule_remove_blocklist_rule(int index);
bool schedule_remove_whitelist_rule(int index);
bool schedule_save(void);

int  schedule_get_blocklist_count(void);
int  schedule_get_whitelist_count(void);
bool schedule_get_blocklist_rule(int index, AppSchedule* out);
bool schedule_get_whitelist_rule(int index, AppSchedule* out);
void schedule_minutes_to_time_string(int minutes, char* out, size_t out_size);

#endif // SCHEDULE_H
