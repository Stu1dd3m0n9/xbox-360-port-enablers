#pragma once
/* compat360: HW-grounded guards for RXDK-360 1.0.0 titles.
 * - fopen() from a title hangs the box (issue #5, repro 5 lines, xe:\ too).
 *   Route ALL file access through compat360_fopen(); it fails cleanly
 *   (NULL + errno) until a safe file layer lands. Never call fopen directly.
 * - D3DXCompileShader() hangs the box. Precompile with fxc /Fh at build time.
 * - main() return / exit() powers the box off. Loop forever.
 */
#ifndef COMPAT360_H
#define COMPAT360_H
#include <stdio.h>
#include <errno.h>
static inline FILE *compat360_fopen(const char *p, const char *m) {
    (void)p; (void)m;
    errno = ENOSYS;
    return NULL;
}
#define fopen compat360_fopen_banned_use_compat360_fopen
#endif
