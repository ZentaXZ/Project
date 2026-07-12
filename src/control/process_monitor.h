#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <stdbool.h>

bool process_monitor_init(void);
bool process_monitor_is_running(const char* process_name);
int  process_monitor_list_running(char names[][64], int max_names);
void process_monitor_poll(void);

#endif // PROCESS_MONITOR_H
