#include "id_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static unsigned long g_id_counter = 0;
static int g_id_seeded = 0;

static void id_utils_seed_once(void) {
    if (!g_id_seeded) {
        srand((unsigned int)(time(NULL) ^ (unsigned long)&g_id_counter));
        g_id_seeded = 1;
    }
}

void generate_uuid(char* out_buffer) {
    id_utils_seed_once();

    time_t now = time(NULL);
    unsigned long counter = ++g_id_counter;

    snprintf(out_buffer, 37, "%08lx-%04lx-%04lx-%04lx-%08lx%04lx",
             (unsigned long)(now & 0xFFFFFFFFUL),
             (counter >> 16) & 0xFFFFUL,
             counter & 0xFFFFUL,
             (unsigned long)(rand() & 0xFFFF),
             (unsigned long)(rand() & 0xFFFFFFFFUL),
             (unsigned long)(rand() & 0xFFFF));
}
