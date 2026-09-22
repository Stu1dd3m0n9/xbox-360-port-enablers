/* compat360.c: placeholder TU so the lib has a backend object.
 * No file I/O, no shaders, no exit. Loops are owned by callers.
 * Also stubs lua dynamic-loader entry (loadlib.c excluded: no dlopen on 360),
 * and provides PPC compiler helpers missing from modern/lib (see RXDK issue #1):
 * __fixdfdi / __fixunsdfdi via integer-only IEEE754 decode (no FP->int cast,
 * so clang cannot recurse back into the same helper). Fast path covers
 * |d| < 2^31 exactly; larger magnitudes saturate (TODO full 64-bit shift
 * once a game's lua needs it -- logged via return extremes, no hang).
 */
#include "compat360.h"
#include <string.h>
#include <stdint.h>
volatile int g_compat360_version = 100;
int compat360_init(void) { return g_compat360_version; }
/* lua 5.4 linit.c references luaopen_package even when loadlib.c is dropped.
 * Return 0 open functions: require() fails cleanly, core language unaffected.
 * Test uses dostring only, no require. */
int luaopen_package(void *L) { (void)L; return 0; }
/* Decode double bits BE-safe without 64-bit shifts (all 32-bit hw ops). */
static void dbl_bits(double d, uint32_t *hi, uint32_t *lo) {
    unsigned char b[8];
    memcpy(b, &d, 8);
    *hi = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
          ((uint32_t)b[2] << 8) | (uint32_t)b[3];
    *lo = ((uint32_t)b[4] << 24) | ((uint32_t)b[5] << 16) |
          ((uint32_t)b[6] << 8) | (uint32_t)b[7];
}
long long __fixdfdi(double d) {
    uint32_t hi, lo;
    dbl_bits(d, &hi, &lo);
    int sign = (int)(hi >> 31);
    int exp = (int)((hi >> 20) & 0x7FF);
    uint32_t mhi = hi & 0xFFFFFu;
    uint32_t mlo = lo;
    if (exp < 1023) return 0;               /* |d| < 1 */
    if (exp >= 1054) {                      /* >= 2^31: saturate */
        return sign ? (-9223372036854775807LL - 1) : 9223372036854775807LL;
    }
    /* implicit leading 1 -> 21-bit hid */
    mhi |= 0x100000u;
    /* e = exp-1023-52: shift of 53-bit mantissa; result fits 31 bits here */
    int sh = (exp - 1023) - 52;
    uint32_t v;
    if (sh >= 0) {
        /* sh <= 30-? here (exp<1054 => sh< -21? no: exp 1054 => sh=-21? recompute:
           exp=1054 -> unbiased 31 -> sh=31-52=-21. So sh negative in this range.
           Left-shift path kept for completeness via 32-bit hid. */
        v = (mhi << sh) | (sh ? (mlo >> (32 - sh)) : mlo);
    } else {
        int r = -sh; /* 21..52 */
        if (r >= 32) {
            uint32_t hid21 = mhi; /* 21 bits */
            v = (r >= 53) ? 0 : (hid21 >> (r - 32));
        } else {
            /* shift 53-bit (mhi:mlo) right by r using 32-bit ops */
            uint32_t low = (mlo >> r) | (mhi << (32 - r));
            uint32_t high = mhi >> r;
            (void)high; /* high must be 0 for exp<1054 & value<2^31; else saturate */
            v = low;
            if (high) return sign ? (-9223372036854775807LL - 1) : 9223372036854775807LL;
        }
    }
    long long s = sign ? -(long long)(int)v : (long long)(int)v;
    return s;
}
unsigned long long __fixunsdfdi(double d) {
    if (d <= 0.0) return 0ULL;
    if (d >= 4294967296.0) return 4294967295ULL; /* saturate: full 64-bit TODO */
    {
        uint32_t hi, lo;
        dbl_bits(d, &hi, &lo);
        int exp = (int)((hi >> 20) & 0x7FF);
        uint32_t mhi = (hi & 0xFFFFFu) | 0x100000u;
        uint32_t mlo = lo;
        int sh = (exp - 1023) - 52;
        uint32_t v;
        if (sh >= 0) v = (mhi << sh) | (sh ? (mlo >> (32 - sh)) : mlo);
        else {
            int r = -sh;
            if (r >= 32) v = (r >= 53) ? 0 : (mhi >> (r - 32));
            else v = (mlo >> r) | (mhi << (32 - r));
        }
        return (unsigned long long)v;
    }
}
