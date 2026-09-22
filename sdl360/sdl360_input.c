/* sdl360_input.c: XInput pads via local ABI decls (no xtl in game group).
 * Generic mapping to SDL360_Pad bits. Links via xapilib (XamInputGetState). */
#include <stdint.h>
#include <string.h>
#include "sdl360.h"
typedef struct { uint16_t b; uint8_t lt, rt; int16_t lx, ly, rx, ry; } GP;
typedef struct { uint32_t pkt; GP g; } ST;
#define N 4
#define DP_U 0x0001u
#define DP_D 0x0002u
#define DP_L 0x0004u
#define DP_R 0x0008u
#define STRT 0x0010u
#define BACK 0x0020u
#define LB 0x0100u
#define RB 0x0200u
#define A 0x1000u
#define B 0x2000u
#define X 0x4000u
#define Y 0x8000u
#define THR 30
#define OK 0
unsigned int XInputGetState(unsigned int i, ST *s);
int sdl360_pad_count(void) {
    int n = 0;
    for (int i = 0; i < N; i++) { ST s; memset(&s, 0, sizeof s);
        if (XInputGetState((unsigned)i, &s) == OK) n++; }
    return n;
}
int sdl360_pad_read(int index, SDL360_Pad *o) {
    if (!o || index < 0 || index >= N) return 0;
    ST s; memset(&s, 0, sizeof s);
    if (XInputGetState((unsigned)index, &s) != OK) return 0;
    memset(o, 0, sizeof *o);
    uint16_t b = s.g.b;
    if (b & A) o->buttons |= SDL360_BTN_A;
    if (b & B) o->buttons |= SDL360_BTN_B;
    if (b & X) o->buttons |= SDL360_BTN_X;
    if (b & Y) o->buttons |= SDL360_BTN_Y;
    if (b & STRT) o->buttons |= SDL360_BTN_START;
    if (b & BACK) o->buttons |= SDL360_BTN_BACK;
    if (b & LB) o->buttons |= SDL360_BTN_LB;
    if (b & RB) o->buttons |= SDL360_BTN_RB;
    if (b & DP_U) o->buttons |= SDL360_BTN_DU;
    if (b & DP_D) o->buttons |= SDL360_BTN_DD;
    if (b & DP_L) o->buttons |= SDL360_BTN_DL;
    if (b & DP_R) o->buttons |= SDL360_BTN_DR;
    o->lx = s.g.lx; o->ly = s.g.ly; o->rx = s.g.rx; o->ry = s.g.ry;
    o->lt = s.g.lt; o->rt = s.g.rt;
    (void)THR;
    return 1;
}
