/* sdl360_thread.cpp: CreateThread wrapper. XDK group. Generic. 64KB stack min. */
#include <xtl.h>
#include "sdl360.h"
struct SDL360_Thread { HANDLE h; int rc; };
struct Pack { SDL360_ThreadFn fn; void *arg; SDL360_Thread *t; };
static DWORD WINAPI Run(LPVOID p) {
    Pack *k = (Pack *)p;
    int r = k->fn(k->arg); k->t->rc = r;
    delete k; return (DWORD)r;
}
extern "C" SDL360_Thread *sdl360_create_thread(SDL360_ThreadFn fn, void *arg) {
    if (!fn) return 0;
    SDL360_Thread *t = new SDL360_Thread; t->h = 0; t->rc = 0;
    Pack *k = new Pack; k->fn = fn; k->arg = arg; k->t = t;
    t->h = CreateThread(NULL, 65536, Run, k, 0, NULL);
    if (!t->h) { delete k; delete t; return 0; }
    return t;
}
extern "C" void sdl360_wait_thread(SDL360_Thread *t) {
    if (!t) return;
    WaitForSingleObject(t->h, INFINITE);
    CloseHandle(t->h); delete t;
}
