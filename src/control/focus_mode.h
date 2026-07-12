#ifndef FOCUS_MODE_H
#define FOCUS_MODE_H

#include <stdbool.h>

void focus_mode_enable(void);
void focus_mode_disable(void);
bool focus_mode_is_active(void);
bool focus_mode_is_allowed(const char* process_name);

#endif // FOCUS_MODE_H
