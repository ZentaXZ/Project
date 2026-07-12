#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

bool config_load(void);
bool config_save(void);

int  config_get_reward_minutes(void);
void config_set_reward_minutes(int minutes);

bool config_get_sound_enabled(void);
void config_set_sound_enabled(bool enabled);

double config_get_punishment_multiplier(void);
void config_apply_punishment_today(void);

bool config_get_watchdog_enabled(void);
void config_set_watchdog_enabled(bool enabled);

bool config_get_close_guard_enabled(void);
void config_set_close_guard_enabled(bool enabled);

const char* config_get_stayfree_exe_path(void);
void config_set_stayfree_exe_path(const char* path);

#endif // CONFIG_H
