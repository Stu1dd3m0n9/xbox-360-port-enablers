/* test_grad.c: vertical gradient R->G->B + white last content row.
 * Wherever black appears on screen tells us what is missing. */
#include "sdl360.h"
#include "report360.h"
#include <string.h>
#include <stdio.h>
void DbgPrint(const char *fmt, ...);
int main(void) {
    sdl360_init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL360_Window *w = sdl360_create_window("GRAD", 0, 0);
    if (!w) { report360("GRAD FAIL nodevice"); for (;;) {} }
    for (int f = 0; f < 60; f++) {
        int pitch = 0;
        uint32_t *px = (uint32_t *)sdl360_lock_framebuffer(w, &pitch);
        uint32_t ww = 0, hh = 0;
        sdl360_size(w, &ww, &hh);
        if (px && ww && hh) {
            int stride = pitch / 4;
            for (uint32_t y = 0; y < hh; y++) {
                uint32_t c;
                if (y == hh - 1) c = 0xFFFFFFFFu;
                else if (y < hh / 3) c = 0xFFFF0000u;
                else if (y < 2 * hh / 3) c = 0xFF00FF00u;
                else c = 0xFF0000FFu;
                for (uint32_t x = 0; x < ww; x++)
                    px[y * stride + x] = c;
            }
        }
        sdl360_unlock_present(w);
    }
    {
        static char m[64];
        uint32_t vx = 0, vy = 0, vw = 0, vh = 0;
        sdl360_viewport(w, &vx, &vy, &vw, &vh);
        snprintf(m, sizeof m, "GRAD done vp=%u,%u %ux%u", (unsigned)vx, (unsigned)vy, (unsigned)vw, (unsigned)vh);
        report360(m);
    }
    for (;;) { sdl360_delay(1000); }
    return 0;
}
