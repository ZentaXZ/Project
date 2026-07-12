#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>

#define CHECK_INTERVAL_MS 2000
#define SHUTDOWN_FLAG_PATH "data/allow_shutdown"
#define WATCHDOG_TARGETS_PATH "data/watchdog_targets.txt"
#define MAX_TARGETS 8
#define PATH_MAX_LEN 512

typedef struct {
    char process_name[64];
    char launch_path[PATH_MAX_LEN];
    char working_dir[PATH_MAX_LEN];
} WatchdogTarget;

static WatchdogTarget g_targets[MAX_TARGETS];
static int g_target_count = 0;

static int shutdown_requested(void) {
    return GetFileAttributesA(SHUTDOWN_FLAG_PATH) != INVALID_FILE_ATTRIBUTES;
}

static void watchdog_copy_basename(const char* path, char* out, size_t out_size) {
    const char* last_slash = strrchr(path, '\\');
    const char* last_fwd = strrchr(path, '/');
    const char* base = path;

    if (last_slash && last_slash + 1 > base) {
        base = last_slash + 1;
    }
    if (last_fwd && last_fwd + 1 > base) {
        base = last_fwd + 1;
    }

    strncpy(out, base, out_size - 1);
    out[out_size - 1] = '\0';
}

static void watchdog_set_working_dir(const char* launch_path, char* working_dir, size_t working_dir_size) {
    if (!launch_path || !working_dir || working_dir_size == 0) {
        return;
    }

    strncpy(working_dir, launch_path, working_dir_size - 1);
    working_dir[working_dir_size - 1] = '\0';

    char* last_slash = strrchr(working_dir, '\\');
    char* last_fwd = strrchr(working_dir, '/');
    char* cut = last_slash;
    if (last_fwd && (!cut || last_fwd > cut)) {
        cut = last_fwd;
    }
    if (cut) {
        *cut = '\0';
    } else {
        working_dir[0] = '\0';
    }
}

static void watchdog_add_default_targets(void) {
    g_target_count = 0;

    strncpy(g_targets[g_target_count].process_name, "gestor-tareas.exe", 63);
    strncpy(g_targets[g_target_count].launch_path, "gestor-tareas.exe", PATH_MAX_LEN - 1);
    g_targets[g_target_count].working_dir[0] = '\0';
    g_target_count++;

    strncpy(g_targets[g_target_count].process_name, "StayFree.exe", 63);
    strncpy(g_targets[g_target_count].launch_path,
            "C:\\Program Files\\WindowsApps\\37081StayFreeApps.StayFree3_3.4.2.0_x64__fqhk48m1tsma0\\app\\StayFree.exe",
            PATH_MAX_LEN - 1);
    watchdog_set_working_dir(g_targets[g_target_count].launch_path,
                             g_targets[g_target_count].working_dir,
                             sizeof(g_targets[g_target_count].working_dir));
    g_target_count++;
}

static void watchdog_load_targets(void) {
    FILE* file = fopen(WATCHDOG_TARGETS_PATH, "r");
    if (!file) {
        watchdog_add_default_targets();
        return;
    }

    g_target_count = 0;
    char line[1024];

    while (fgets(line, sizeof(line), file) && g_target_count < MAX_TARGETS) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0 || line[0] == '#') {
            continue;
        }

        char* separator = strchr(line, '|');
        if (!separator) {
            continue;
        }

        *separator = '\0';
        const char* process_name = line;
        const char* launch_path = separator + 1;
        if (launch_path[0] == '\0') {
            continue;
        }

        WatchdogTarget* target = &g_targets[g_target_count];
        memset(target, 0, sizeof(WatchdogTarget));

        if (strchr(process_name, '\\') || strchr(process_name, '/')) {
            watchdog_copy_basename(process_name, target->process_name, sizeof(target->process_name));
        } else {
            strncpy(target->process_name, process_name, sizeof(target->process_name) - 1);
        }

        strncpy(target->launch_path, launch_path, sizeof(target->launch_path) - 1);
        watchdog_set_working_dir(target->launch_path, target->working_dir, sizeof(target->working_dir));
        g_target_count++;
    }

    fclose(file);

    if (g_target_count == 0) {
        watchdog_add_default_targets();
    }
}

static int is_process_running(const char* process_name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    int found = 0;

    if (Process32First(snapshot, &entry)) {
        do {
            if (_stricmp(entry.szExeFile, process_name) == 0) {
                found = 1;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return found;
}

static void relaunch_process(const WatchdogTarget* target) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char command_line[PATH_MAX_LEN + 4];

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (strchr(target->launch_path, ' ') != NULL) {
        snprintf(command_line, sizeof(command_line), "\"%s\"", target->launch_path);
    } else {
        snprintf(command_line, sizeof(command_line), "%s", target->launch_path);
    }

    const char* working_dir = target->working_dir[0] != '\0' ? target->working_dir : NULL;
    BOOL started = FALSE;

    if (CreateProcessA(NULL, command_line, NULL, NULL, FALSE,
                       0, NULL, working_dir, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        started = TRUE;
    }

    if (!started) {
        HINSTANCE result = ShellExecuteA(NULL, "open", target->launch_path,
                                         NULL, working_dir, SW_SHOWNORMAL);
        if ((INT_PTR)result > 32) {
            started = TRUE;
        }
    }
}

static void watchdog_ensure_target_alive(const WatchdogTarget* target) {
    if (shutdown_requested()) {
        return;
    }
    if (!is_process_running(target->process_name)) {
        relaunch_process(target);
    }
}

int main(void) {
    watchdog_load_targets();

    while (1) {
        if (shutdown_requested()) {
            break;
        }

        for (int i = 0; i < g_target_count; i++) {
            watchdog_ensure_target_alive(&g_targets[i]);
        }

        Sleep(CHECK_INTERVAL_MS);
    }
    return 0;
}
