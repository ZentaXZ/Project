#include "process_monitor.h"
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

#define MAX_CACHED_PROCESSES 512

static char g_running_names[MAX_CACHED_PROCESSES][64];
static int g_running_count = 0;

static const char* SYSTEM_PROCESS_BLOCKLIST[] = {
    "System",
    "Registry",
    "smss.exe",
    "csrss.exe",
    "wininit.exe",
    "services.exe",
    "lsass.exe",
    "svchost.exe",
    "dwm.exe",
    "winlogon.exe",
    "fontdrvhost.exe",
    "RuntimeBroker.exe",
    "SearchIndexer.exe",
    "SearchHost.exe",
    "ShellExperienceHost.exe",
    "StartMenuExperienceHost.exe",
    "TextInputHost.exe",
    "ctfmon.exe",
    "explorer.exe",
    "conhost.exe",
    "dllhost.exe",
    "backgroundTaskHost.exe",
    "ApplicationFrameHost.exe",
    "SystemSettings.exe",
    "Idle",
    NULL
};

typedef struct {
    DWORD pid;
    bool has_visible_window;
} WindowCheckContext;

static BOOL CALLBACK enum_windows_callback(HWND hwnd, LPARAM lparam) {
    WindowCheckContext* ctx = (WindowCheckContext*)lparam;

    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }

    DWORD window_pid = 0;
    GetWindowThreadProcessId(hwnd, &window_pid);
    if (window_pid != ctx->pid) {
        return TRUE;
    }

    if (GetWindowTextLengthA(hwnd) > 0) {
        ctx->has_visible_window = true;
        return FALSE;
    }

    return TRUE;
}

bool process_monitor_is_system_process(const char* process_name) {
    if (!process_name) {
        return true;
    }

    for (int i = 0; SYSTEM_PROCESS_BLOCKLIST[i] != NULL; i++) {
        if (_stricmp(process_name, SYSTEM_PROCESS_BLOCKLIST[i]) == 0) {
            return true;
        }
    }
    return false;
}

static bool process_has_visible_window(DWORD pid) {
    WindowCheckContext ctx = { pid, false };
    EnumWindows(enum_windows_callback, (LPARAM)&ctx);
    return ctx.has_visible_window;
}

static int process_monitor_collect_unfiltered(char names[][64], int max_names) {
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

static int process_monitor_collect_filtered(char names[][64], int max_names) {
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

            if (process_monitor_is_system_process(entry.szExeFile)) {
                continue;
            }

            if (!process_has_visible_window(entry.th32ProcessID)) {
                continue;
            }

            bool already_added = false;
            for (int i = 0; i < count; i++) {
                if (_stricmp(names[i], entry.szExeFile) == 0) {
                    already_added = true;
                    break;
                }
            }
            if (already_added) {
                continue;
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
    g_running_count = process_monitor_collect_unfiltered(g_running_names, MAX_CACHED_PROCESSES);
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
    return process_monitor_collect_filtered(names, max_names);
}
