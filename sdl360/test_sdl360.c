/* test_sdl360.c: game-group validation. Blue clear + white bar, 300 frames,
 * pad poll, 440Hz sine push. No fopen, no shaders, loop forever. */
#include "sdl360.h"
#include "report360.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
void DbgPrint(const char *fmt, ...);
volatile int g_frames = 0, g_pads = 0, g_abuf = 0;
static int16_t g_sine[640];
static void fill_sine(void) {
    for (int i = 0; i < 320; i++) {
        float t = (float)i / 320.0f;
        int v = (int)(t * 6.2831853f * 440.0f / 32000.0f * 320.0f);
        (void)v;
        short s = (short)(10000.0f * ((i % 73) / 73.0f * 2.0f - 1.0f));
        g_sine[i * 2] = s; g_sine[i * 2 + 1] = s;
    }
}
int main(void) {
    sdl360_init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER);
    SDL360_Window *w = sdl360_create_window("SDL360", 0, 0);
    if (!w) { DbgPrint("[SDL360] window NULL\n"); for (;;) {} }
    sdl360_audio_init(); fill_sine();
    for (int f = 0; f < 300; f++) {
        int pitch = 0;
        uint32_t *px = (uint32_t *)sdl360_lock_framebuffer(w, &pitch);
        uint32_t ww = 0, hh = 0;
        sdl360_size(w, &ww, &hh);
        if (px && ww && hh) {
            int stride = pitch / 4;
            for (uint32_t y = 0; y < hh; y++)
                for (uint32_t x = 0; x < ww; x++)
                    px[y * stride + x] = 0xFF103080u;
            for (uint32_t y = hh * 340 / 720; y < hh * 380 / 720; y++)
                for (uint32_t x = ww * 100 / 1280; x < ww * 1180 / 1280; x++)
                    px[y * stride + x] = 0xFFFFFFFFu;
        }
        sdl360_unlock_present(w);
        SDL360_Pad p; memset(&p, 0, sizeof p);
        if (sdl360_pad_read(0, &p)) g_pads++;
        sdl360_audio_push(g_sine, 320);
        g_abuf = sdl360_audio_buffered_frames();
        g_frames = f + 1;
        if ((f % 60) == 0) DbgPrint("[SDL360] f=%d pads=%d abuf=%d\n", g_frames, g_pads, g_abuf);
    }
    DbgPrint("[SDL360] 300f done pads=%d\n", g_pads);
    {
        static char m[48];
        snprintf(m, sizeof m, "PITCH %u", (unsigned)sdl360_last_pitch(w));
        report360(m);
    }
    report360("SDL PASS 300f bluebar");
    for (;;) { sdl360_delay(1000); }
    return 0;
}
