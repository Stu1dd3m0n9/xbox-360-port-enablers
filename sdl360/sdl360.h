/* sdl360.h: minimal SDL2-compatible subset for Xbox 360 (RXDK modern).
 * Generic - no SM64 types. Header stays includable from picolibc game code
 * (no xtl.h here; impl .cpps cast opaque handles to D3D/XAudio types).
 * Video: 1280x720 X8R8G8B8, no shaders (StretchRect upload, no D3DXCompile).
 * Audio: 32kHz stereo S16 push. Input: 4x XInput pads. Timer/threads included.
 */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define SDL_INIT_VIDEO   0x20u
#define SDL_INIT_AUDIO   0x10u
#define SDL_INIT_JOYSTICK 0x200u
#define SDL_INIT_TIMER   0x01u
typedef struct SDL360_Window SDL360_Window;
typedef struct SDL360_Thread SDL360_Thread;
typedef int (*SDL360_ThreadFn)(void *arg);
typedef struct {
    uint16_t buttons; /* bitmask below */
    int16_t lx, ly, rx, ry;
    uint8_t lt, rt;
} SDL360_Pad;
#define SDL360_BTN_A 0x0001u
#define SDL360_BTN_B 0x0002u
#define SDL360_BTN_X 0x0004u
#define SDL360_BTN_Y 0x0008u
#define SDL360_BTN_START 0x0010u
#define SDL360_BTN_BACK 0x0020u
#define SDL360_BTN_LB 0x0040u
#define SDL360_BTN_RB 0x0080u
#define SDL360_BTN_DU 0x0100u
#define SDL360_BTN_DD 0x0200u
#define SDL360_BTN_DL 0x0400u
#define SDL360_BTN_DR 0x0800u
int sdl360_init(uint32_t flags);
void sdl360_quit(void);
SDL360_Window *sdl360_create_window(const char *title, int w, int h);
void sdl360_destroy_window(SDL360_Window *win);
void *sdl360_lock_framebuffer(SDL360_Window *win, int *pitch);
void sdl360_unlock_present(SDL360_Window *win);
void sdl360_present(SDL360_Window *win);
void sdl360_clear(SDL360_Window *win, uint8_t r, uint8_t g, uint8_t b);
uint32_t sdl360_last_pitch(SDL360_Window *win);
void sdl360_tex_size(SDL360_Window *win, uint32_t *w, uint32_t *h);
void sdl360_size(SDL360_Window *win, uint32_t *w, uint32_t *h);
void sdl360_viewport(SDL360_Window *win, uint32_t *x, uint32_t *y, uint32_t *w, uint32_t *h);
int sdl360_audio_init(void);
void sdl360_audio_push(const int16_t *pcm_stereo, int frames);
int sdl360_audio_buffered_frames(void);
int sdl360_pad_count(void);
int sdl360_pad_read(int index, SDL360_Pad *out);
uint32_t sdl360_get_ticks(void);
void sdl360_delay(uint32_t ms);
SDL360_Thread *sdl360_create_thread(SDL360_ThreadFn fn, void *arg);
void sdl360_wait_thread(SDL360_Thread *t);
#ifdef __cplusplus
}
#endif
