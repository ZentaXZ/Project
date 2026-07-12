#include "app_blocker.h"
#include "schedule.h"
#include "process_monitor.h"
#include "reward_timer.h"
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

static bool app_blocker_terminate_pid(DWORD pid) {
    HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!process) {
        return false;
    }

    BOOL ok = TerminateProcess(process, 0);
    CloseHandle(process);
    return ok != 0;
}

static bool app_blocker_terminate_all_matching(const char* process_name) {
    if (!process_name) {
        return false;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(entry);
    bool any = false;

    if (Process32First(snapshot, &entry)) {
        do {
            if (_stricmp(entry.szExeFile, process_name) == 0) {
                if (app_blocker_terminate_pid(entry.th32ProcessID)) {
                    any = true;
                }
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return any;
}

bool app_blocker_terminate(const char* process_name) {
    return app_blocker_terminate_all_matching(process_name);
}

bool app_blocker_is_blocked_now(const char* process_name) {
    if (!process_name) {
        return false;
    }

    if (reward_timer_is_active()) {
        return false;
    }

    return !schedule_is_allowed_now(process_name);
}

void app_blocker_enforce(void) {
    if (reward_timer_is_active()) {
        return;
    }

    int block_count = schedule_get_blocklist_count();
    for (int i = 0; i < block_count; i++) {
        AppSchedule rule;
        if (!schedule_get_blocklist_rule(i, &rule)) {
            continue;
        }

        if (!app_blocker_is_blocked_now(rule.process_name)) {
            continue;
        }

        if (process_monitor_is_running(rule.process_name)) {
            app_blocker_terminate(rule.process_name);
        }
    }
}
