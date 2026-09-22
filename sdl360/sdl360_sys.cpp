/* sdl360_sys.cpp: init/quit + timer. XDK group. Generic, no video. */
#include <xtl.h>
#include "sdl360.h"
int sdl360_init(uint32_t flags) { (void)flags; return 0; }
void sdl360_quit(void) {}
uint32_t sdl360_get_ticks(void) {
    LARGE_INTEGER f, t;
    QueryPerformanceFrequency(&f); QueryPerformanceCounter(&t);
    if (!f.QuadPart) return 0;
    return (uint32_t)((t.QuadPart * 1000) / f.QuadPart);
}
void sdl360_delay(uint32_t ms) { Sleep(ms); }
