#include "installed_apps.h"
#include <stdio.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static int installed_apps_stricmp(const char* a, const char* b) {
    if (!a || !b) {
        return (a != b);
    }
    return _stricmp(a, b);
}

static void installed_apps_copy_basename(const char* path, char* out, size_t out_size) {
    if (!path || !out || out_size == 0) {
        return;
    }

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

static bool installed_apps_path_looks_like_exe(const char* path) {
    if (!path) {
        return false;
    }

    size_t len = strlen(path);
    if (len < 4) {
        return false;
    }

    const char* ext = path + len - 4;
    return _stricmp(ext, ".exe") == 0;
}

static void installed_apps_extract_exe_from_path_field(const char* field, char* exe_out, size_t exe_out_size) {
    if (!field || !exe_out || exe_out_size == 0 || field[0] == '\0') {
        return;
    }

    char buffer[512];
    strncpy(buffer, field, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char* comma = strchr(buffer, ',');
    if (comma) {
        *comma = '\0';
    }

    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == ' ' || buffer[len - 1] == '"')) {
        buffer[--len] = '\0';
    }

    size_t start = 0;
    while (buffer[start] == ' ' || buffer[start] == '"') {
        start++;
    }

    const char* path = buffer + start;
    if (!installed_apps_path_looks_like_exe(path)) {
        return;
    }

    installed_apps_copy_basename(path, exe_out, exe_out_size);
}

static void installed_apps_guess_exe(HKEY app_key, char* exe_out, size_t exe_out_size) {
    if (!exe_out || exe_out_size == 0) {
        return;
    }

    exe_out[0] = '\0';

    char display_icon[512];
    DWORD display_icon_size = sizeof(display_icon);
    DWORD value_type = 0;

    if (RegQueryValueExA(app_key, "DisplayIcon", NULL, &value_type,
                         (LPBYTE)display_icon, &display_icon_size) == ERROR_SUCCESS &&
        value_type == REG_SZ && display_icon[0] != '\0') {
        installed_apps_extract_exe_from_path_field(display_icon, exe_out, exe_out_size);
        if (exe_out[0] != '\0') {
            return;
        }
    }

    char uninstall_string[512];
    DWORD uninstall_string_size = sizeof(uninstall_string);
    if (RegQueryValueExA(app_key, "UninstallString", NULL, &value_type,
                         (LPBYTE)uninstall_string, &uninstall_string_size) == ERROR_SUCCESS &&
        value_type == REG_SZ && uninstall_string[0] != '\0') {
        installed_apps_extract_exe_from_path_field(uninstall_string, exe_out, exe_out_size);
    }
}

static bool installed_apps_is_duplicate(const InstalledApp* apps, int count, const char* display_name) {
    for (int i = 0; i < count; i++) {
        if (installed_apps_stricmp(apps[i].display_name, display_name) == 0) {
            return true;
        }
    }
    return false;
}

static void installed_apps_sort(InstalledApp* apps, int count) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (installed_apps_stricmp(apps[i].display_name, apps[j].display_name) > 0) {
                InstalledApp tmp = apps[i];
                apps[i] = apps[j];
                apps[j] = tmp;
            }
        }
    }
}

static int installed_apps_enum_uninstall_key(HKEY root, const char* subkey_path,
                                             InstalledApp* apps, int max_apps, int start_index) {
    HKEY uninstall_key;
    if (RegOpenKeyExA(root, subkey_path, 0, KEY_READ, &uninstall_key) != ERROR_SUCCESS) {
        return start_index;
    }

    char subkey_name[256];
    DWORD subkey_name_size;
    int index = start_index;
    DWORD i = 0;

    while (index < max_apps) {
        subkey_name_size = sizeof(subkey_name);
        LONG result = RegEnumKeyExA(uninstall_key, i, subkey_name, &subkey_name_size,
                                    NULL, NULL, NULL, NULL);
        if (result != ERROR_SUCCESS) {
            break;
        }

        HKEY app_key;
        if (RegOpenKeyExA(uninstall_key, subkey_name, 0, KEY_READ, &app_key) == ERROR_SUCCESS) {
            char display_name[INSTALLED_APP_NAME_MAX];
            DWORD display_name_size = sizeof(display_name);
            DWORD value_type = 0;

            LONG name_result = RegQueryValueExA(app_key, "DisplayName", NULL,
                                                &value_type, (LPBYTE)display_name,
                                                &display_name_size);

            if (name_result == ERROR_SUCCESS && value_type == REG_SZ && display_name[0] != '\0') {
                DWORD system_component = 0;
                DWORD sc_size = sizeof(DWORD);
                RegQueryValueExA(app_key, "SystemComponent", NULL, NULL,
                                 (LPBYTE)&system_component, &sc_size);

                if (system_component == 0 &&
                    !installed_apps_is_duplicate(apps, index, display_name)) {
                    strncpy(apps[index].display_name, display_name, INSTALLED_APP_NAME_MAX - 1);
                    apps[index].display_name[INSTALLED_APP_NAME_MAX - 1] = '\0';
                    installed_apps_guess_exe(app_key, apps[index].exe_name, INSTALLED_APP_EXE_MAX);
                    index++;
                }
            }

            RegCloseKey(app_key);
        }

        i++;
    }

    RegCloseKey(uninstall_key);
    return index;
}

int installed_apps_list(InstalledApp* apps, int max_apps) {
    if (!apps || max_apps <= 0) {
        return 0;
    }

    int count = 0;

    count = installed_apps_enum_uninstall_key(
        HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        apps, max_apps, count);

    count = installed_apps_enum_uninstall_key(
        HKEY_LOCAL_MACHINE,
        "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        apps, max_apps, count);

    count = installed_apps_enum_uninstall_key(
        HKEY_CURRENT_USER,
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        apps, max_apps, count);

    installed_apps_sort(apps, count);
    return count;
}
