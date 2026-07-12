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

#endif // CONFIG_H
