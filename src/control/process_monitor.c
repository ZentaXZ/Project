#include "process_monitor.h"
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

#define MAX_CACHED_PROCESSES 512

static char g_running_names[MAX_CACHED_PROCESSES][64];
static int g_running_count = 0;

static int process_monitor_collect(char names[][64], int max_names) {
    if (max_names <= 0) {
        return 0;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(entry);
    int count = 0;

    if (Process32First(snapshot, &entry)) {
        do {
            if (count >= max_names) {
                break;
            }
            strncpy(names[count], entry.szExeFile, 63);
            names[count][63] = '\0';
            count++;
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return count;
}

bool process_monitor_init(void) {
    process_monitor_poll();
    return true;
}

void process_monitor_poll(void) {
    g_running_count = process_monitor_collect(g_running_names, MAX_CACHED_PROCESSES);
}

bool process_monitor_is_running(const char* process_name) {
    if (!process_name) {
        return false;
    }

    for (int i = 0; i < g_running_count; i++) {
        if (_stricmp(g_running_names[i], process_name) == 0) {
            return true;
        }
    }
    return false;
}

int process_monitor_list_running(char names[][64], int max_names) {
    if (!names || max_names <= 0) {
        return 0;
    }

    int count = g_running_count < max_names ? g_running_count : max_names;
    for (int i = 0; i < count; i++) {
        strncpy(names[i], g_running_names[i], 63);
        names[i][63] = '\0';
    }
    return count;
}
