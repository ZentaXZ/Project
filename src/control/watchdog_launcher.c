#include "watchdog_launcher.h"
#include "../utils/config.h"
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>

#define WATCHDOG_EXE "watchdog.exe"
#define SHUTDOWN_FLAG_PATH "data/allow_shutdown"
#define WATCHDOG_TARGETS_PATH "data/watchdog_targets.txt"

static bool g_shutdown_requested = false;

static bool shutdown_flag_exists(void) {
    return GetFileAttributesA(SHUTDOWN_FLAG_PATH) != INVALID_FILE_ATTRIBUTES;
}

static bool create_shutdown_flag(void) {
    HANDLE file = CreateFileA(
        SHUTDOWN_FLAG_PATH,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    CloseHandle(file);
    return true;
}

static bool is_watchdog_running(void) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    bool found = false;

    if (Process32First(snapshot, &entry)) {
        do {
            if (_stricmp(entry.szExeFile, WATCHDOG_EXE) == 0) {
                found = true;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return found;
}

static bool launch_watchdog_process(void) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    char command_line[MAX_PATH];
    snprintf(command_line, sizeof(command_line), "%s", WATCHDOG_EXE);

    if (CreateProcessA(NULL, command_line, NULL, NULL, FALSE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    return false;
}

static void terminate_watchdog_processes(void) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return;
    }

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &entry)) {
        do {
            if (_stricmp(entry.szExeFile, WATCHDOG_EXE) != 0) {
                continue;
            }

            HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
            if (process) {
                TerminateProcess(process, 0);
                CloseHandle(process);
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
}

void watchdog_launcher_clear_shutdown_flag(void) {
    g_shutdown_requested = false;
    DeleteFileA(SHUTDOWN_FLAG_PATH);
}

void watchdog_launcher_request_shutdown(void) {
    g_shutdown_requested = true;
    create_shutdown_flag();
}

void watchdog_launcher_stop(void) {
    terminate_watchdog_processes();
}

void watchdog_launcher_sync_targets(void) {
    FILE* file = fopen(WATCHDOG_TARGETS_PATH, "w");
    if (!file) {
        return;
    }

    fprintf(file, "gestor-tareas.exe|gestor-tareas.exe\n");
    fprintf(file, "StayFree.exe|%s\n", config_get_stayfree_exe_path());
    fclose(file);
}

bool watchdog_launcher_start(void) {
    if (!config_get_watchdog_enabled()) {
        return false;
    }
    if (g_shutdown_requested || shutdown_flag_exists()) {
        return false;
    }

    watchdog_launcher_sync_targets();

    if (is_watchdog_running()) {
        return true;
    }
    return launch_watchdog_process();
}

void watchdog_launcher_ensure_alive(void) {
    if (!config_get_watchdog_enabled()) {
        return;
    }
    if (g_shutdown_requested || shutdown_flag_exists()) {
        return;
    }

    watchdog_launcher_sync_targets();

    if (!is_watchdog_running()) {
        launch_watchdog_process();
    }
}
