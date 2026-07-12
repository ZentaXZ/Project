#ifndef REWARD_TIMER_H
#define REWARD_TIMER_H

#include <stdbool.h>

void reward_timer_grant(int minutes);
void reward_timer_tick(void);
bool reward_timer_is_active(void);
int  reward_timer_minutes_left(void);
void reward_timer_on_expire_callback(void (*callback)(void));

#endif // REWARD_TIMER_H
