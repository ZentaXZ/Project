#ifndef NOTIFY_H
#define NOTIFY_H

#include <stdbool.h>

void notify_show(const char* title, const char* message);
void notify_play_sound(const char* sound_file);
void notify_task_completed(bool is_daily);

#endif // NOTIFY_H
