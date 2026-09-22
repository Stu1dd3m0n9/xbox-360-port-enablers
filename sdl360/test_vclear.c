/* test_vclear.c: video-clear-only. CreateDevice + Clear + Present x120. */
#include "sdl360.h"
#include "report360.h"
void DbgPrint(const char *fmt, ...);
volatile int g_vf = 0;
int main(void) {
    sdl360_init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL360_Window *w = sdl360_create_window("VCLEAR", 1280, 720);
    if (!w) { report360("VCLEAR FAIL nodevice"); for (;;) {} }
    for (int f = 0; f < 120; f++) {
        sdl360_clear(w, (uint8_t)(f & 255), 0x30, 0x80);
        sdl360_present(w);
        g_vf = f + 1;
    }
    report360("VCLEAR PASS 120f");
    for (;;) { sdl360_delay(1000); }
    return 0;
}
