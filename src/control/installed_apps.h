#ifndef INSTALLED_APPS_H
#define INSTALLED_APPS_H

#include <stdbool.h>

#define INSTALLED_APP_NAME_MAX 256
#define INSTALLED_APP_EXE_MAX 64

typedef struct {
    char display_name[INSTALLED_APP_NAME_MAX];
    char exe_name[INSTALLED_APP_EXE_MAX];
} InstalledApp;

int installed_apps_list(InstalledApp* apps, int max_apps);

#endif
