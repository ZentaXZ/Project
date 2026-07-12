#include "notify.h"
#include "../utils/config.h"
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

void notify_show(const char* title, const char* message) {
    if (!title || !message) {
        return;
    }

    MessageBoxA(NULL, message, title, MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
}

void notify_play_sound(const char* sound_file) {
    if (!config_get_sound_enabled()) {
        return;
    }

    if (sound_file && sound_file[0] != '\0') {
        DWORD attrs = GetFileAttributesA(sound_file);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            if (PlaySoundA(sound_file, NULL, SND_FILENAME | SND_ASYNC)) {
                return;
            }
        }
    }

    MessageBeep(MB_OK);
}

void notify_task_completed(bool is_daily) {
    if (is_daily) {
        notify_show("Tarea completada", "Completaste una tarea diaria. ¡Buen trabajo!");
        notify_play_sound("assets/sounds/task_daily_done.wav");
    } else {
        notify_show("Tarea completada", "Completaste una tarea. ¡Seguí así!");
        notify_play_sound("assets/sounds/task_normal_done.wav");
    }
}
