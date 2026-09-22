/* test_geo.c: quadrants TL=red TR=green BL=blue BR=white + black 8px
 * border. One screenshot gives coverage/flip/format truth. */
#include "sdl360.h"
#include "report360.h"
#include <string.h>
#include <stdio.h>
void DbgPrint(const char *fmt, ...);
int main(void) {
    sdl360_init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL360_Window *w = sdl360_create_window("GEO", 0, 0);
    if (!w) { report360("GEO FAIL nodevice"); for (;;) {} }
    for (int f = 0; f < 60; f++) {
        int pitch = 0;
        uint32_t *px = (uint32_t *)sdl360_lock_framebuffer(w, &pitch);
        uint32_t ww = 0, hh = 0;
        sdl360_size(w, &ww, &hh);
        if (px && ww && hh) {
            int stride = pitch / 4;
            for (uint32_t y = 0; y < hh; y++) {
                for (uint32_t x = 0; x < ww; x++) {
                    uint32_t c;
                    if (x < 8 || y < 8 || x >= ww - 8 || y >= hh - 8) c = 0xFF000000u;
                    else if (x < ww / 2 && y < hh / 2) c = 0xFFFF0000u;
                    else if (x >= ww / 2 && y < hh / 2) c = 0xFF00FF00u;
                    else if (x < ww / 2) c = 0xFF0000FFu;
                    else c = 0xFFFFFFFFu;
                    px[y * stride + x] = c;
                }
            }
        }
        sdl360_unlock_present(w);
    }
    {
        static char m[64];
        uint32_t vx = 0, vy = 0, vw = 0, vh = 0, tw = 0, th = 0;
        sdl360_viewport(w, &vx, &vy, &vw, &vh);
        sdl360_tex_size(w, &tw, &th);
        snprintf(m, sizeof m, "GEO vp=%u,%u %ux%u",
            (unsigned)vx, (unsigned)vy, (unsigned)vw, (unsigned)vh);
        report360(m);
        report360("GEO vXG tile");
        snprintf(m, sizeof m, "GEO tex=%u %u",
            (unsigned)tw, (unsigned)th);
        report360(m);
    }
    for (;;) { sdl360_delay(1000); }
    return 0;
}
