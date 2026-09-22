/* test_snd.c: audio-only. XAudio2 init + 60 sine pushes. No video. */
#include "sdl360.h"
#include "report360.h"
void DbgPrint(const char *fmt, ...);
volatile int g_push = 0, g_buf = 0;
static int16_t g_s[640];
int main(void) {
    sdl360_init(SDL_INIT_AUDIO | SDL_INIT_TIMER);
    if (!sdl360_audio_init()) { report360("SND FAIL noinit"); for (;;) {} }
    for (int i = 0; i < 320; i++) {
        short s = (short)(8000.0f * ((i % 53) / 53.0f * 2.0f - 1.0f));
        g_s[i * 2] = s; g_s[i * 2 + 1] = s;
    }
    for (int f = 0; f < 60; f++) {
        sdl360_audio_push(g_s, 320);
        g_push = f + 1; g_buf = sdl360_audio_buffered_frames();
        sdl360_delay(30);
    }
    report360("SND PASS 60push");
    for (;;) { sdl360_delay(1000); }
    return 0;
}
