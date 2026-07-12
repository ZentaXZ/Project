#ifndef WATCHDOG_LAUNCHER_H
#define WATCHDOG_LAUNCHER_H

#include <stdbool.h>

bool watchdog_launcher_start(void);
void watchdog_launcher_ensure_alive(void);
void watchdog_launcher_sync_targets(void);
void watchdog_launcher_request_shutdown(void);
void watchdog_launcher_stop(void);
void watchdog_launcher_clear_shutdown_flag(void);

#endif
