#ifndef APP_BLOCKER_H
#define APP_BLOCKER_H

#include <stdbool.h>

bool app_blocker_terminate(const char* process_name);
bool app_blocker_is_blocked_now(const char* process_name);
void app_blocker_enforce(void);

#endif // APP_BLOCKER_H
