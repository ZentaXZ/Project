#include "reward_timer.h"
#include <stdio.h>

static int g_minutes_left = 0;
static void (*g_expire_callback)(void) = NULL;

void reward_timer_grant(int minutes) {
    if (minutes <= 0) {
        return;
    }

    g_minutes_left += minutes;
    printf("[reward_timer] Otorgados %d minutos. Total restante: %d min\n", minutes, g_minutes_left);
}

void reward_timer_tick(void) {
    if (g_minutes_left <= 0) {
        return;
    }

    g_minutes_left--;
    printf("[reward_timer] Tick. Minutos restantes: %d\n", g_minutes_left);

    if (g_minutes_left <= 0 && g_expire_callback) {
        printf("[reward_timer] Tiempo de recompensa agotado, ejecutando callback.\n");
        g_expire_callback();
    }
}

bool reward_timer_is_active(void) {
    return g_minutes_left > 0;
}

int reward_timer_minutes_left(void) {
    return g_minutes_left;
}

void reward_timer_on_expire_callback(void (*callback)(void)) {
    g_expire_callback = callback;
}
