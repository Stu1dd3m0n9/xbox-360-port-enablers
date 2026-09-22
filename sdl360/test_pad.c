/* test_pad.c: input-only. Poll pad 0 for ~8s, report buttons seen. */
#include "sdl360.h"
#include "report360.h"
#include <string.h>
#include <stdio.h>
void DbgPrint(const char *fmt, ...);
volatile int g_seen = 0, g_n = 0;
int main(void) {
    sdl360_init(SDL_INIT_JOYSTICK | SDL_INIT_TIMER);
    g_n = sdl360_pad_count();
    for (int f = 0; f < 240; f++) {
        SDL360_Pad p; memset(&p, 0, sizeof p);
        if (sdl360_pad_read(0, &p)) g_seen |= p.buttons;
        sdl360_delay(33);
    }
    static char m[48];
    snprintf(m, sizeof m, "PAD done n=%d seen=0x%04x", g_n, (unsigned)g_seen);
    report360(m);
    for (;;) { sdl360_delay(1000); }
    return 0;
}
