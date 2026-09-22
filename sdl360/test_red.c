/* test_red.c: solid fullscreen red. If TV red is perfect, upload+fetch
 * agree and the bar artifact is pattern-specific. No new D3D calls. */
#include "sdl360.h"
#include "report360.h"
#include <string.h>
#include <stdio.h>
void DbgPrint(const char *fmt, ...);
int main(void) {
    sdl360_init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL360_Window *w = sdl360_create_window("RED", 0, 0);
    if (!w) { report360("RED FAIL nodevice"); for (;;) {} }
    for (int f = 0; f < 60; f++) {
        int pitch = 0;
        uint32_t *px = (uint32_t *)sdl360_lock_framebuffer(w, &pitch);
        if (!px) { report360("RED FAIL nolock"); for (;;) {} }
        uint32_t ww = 0, hh = 0;
        sdl360_size(w, &ww, &hh);
        int stride = pitch / 4;
        for (uint32_t y = 0; y < hh; y++)
            for (uint32_t x = 0; x < ww; x++)
                px[y * stride + x] = 0xFFFF0000u;
        sdl360_unlock_present(w);
    }
    report360("RED PASS 60f");
    {
        static char m[48];
        uint32_t tw = 0, th = 0;
        sdl360_tex_size(w, &tw, &th);
        snprintf(m, sizeof m, "TEXDESC %u %u", (unsigned)tw, (unsigned)th);
        report360(m);
    }
    for (;;) { sdl360_delay(1000); }
    return 0;
}
