#include "focus_mode.h"
#include "schedule.h"
#include <string.h>

static bool g_focus_active = false;

void focus_mode_enable(void) {
    g_focus_active = true;
}

void focus_mode_disable(void) {
    g_focus_active = false;
}

bool focus_mode_is_active(void) {
    return g_focus_active;
}

bool focus_mode_is_allowed(const char* process_name) {
    if (!g_focus_active || !process_name) {
        return true;
    }

    int count = schedule_get_whitelist_count();
    for (int i = 0; i < count; i++) {
        AppSchedule rule;
        if (schedule_get_whitelist_rule(i, &rule) &&
            _stricmp(rule.process_name, process_name) == 0) {
            return true;
        }
    }

    return false;
}
