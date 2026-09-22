/* test_stb360.c: stb_image self-test with embedded 1x1 PNG (no fopen).
 * Transparent 1x1 PNG bytes; expect 1x1x4, RGBA all 0.
 * Loops forever; result visible via XBDM globals + DbgPrint.
 */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "report360.h"
#include <stdio.h>
#include <stdlib.h>
void DbgPrint(const char *fmt, ...);
volatile int g_stb_w = -1, g_stb_h = -1, g_stb_c = -1, g_stb_ok = 0;
static const unsigned char kPngStored[] = {
    0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44,0x52,
    0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x06,0x00,0x00,0x00,0x1F,0x15,0xC4,
    0x89,0x00,0x00,0x00,0x10,0x49,0x44,0x41,0x54,0x78,0x01,0x01,0x05,0x00,0xFA,0xFF,
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x00,0x01,0x64,0x78,0x95,0x38,0x00,0x00,0x00,
    0x00,0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82
};
static const unsigned char kPngGray[] = {
    0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44,0x52,
    0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x00,0x00,0x00,0x00,0x3A,0x7E,0x9B,
    0x55,0x00,0x00,0x00,0x0A,0x49,0x44,0x41,0x54,0x78,0x9C,0x63,0xA8,0x07,0x00,0x00,
    0x81,0x00,0x80,0xD3,0x94,0x53,0x4A,0x00,0x00,0x00,0x00,0x49,0x45,0x4E,0x44,0xAE,
    0x42,0x60,0x82
};
static const unsigned char kPng1x1[] = {
    0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44,0x52,
    0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x06,0x00,0x00,0x00,0x1F,0x15,0xC4,
    0x89,0x00,0x00,0x00,0x0D,0x49,0x44,0x41,0x54,0x78,0x9C,0x63,0x60,0x60,0x60,0x60,
    0x00,0x00,0x00,0x05,0x00,0x01,0xA5,0xF6,0x45,0x40,0x00,0x00,0x00,0x00,0x49,0x45,
    0x4E,0x44,0xAE,0x42,0x60,0x82
};
int main(void) {
    int w = 0, h = 0, c = 0;
    int p0 = -999, p1 = -999, p2 = -999, p3 = -999, haspx = 0;
    unsigned char *px = stbi_load_from_memory(kPng1x1, sizeof(kPng1x1), &w, &h, &c, 4);
    g_stb_w = w; g_stb_h = h; g_stb_c = c;
    if (px) {
        haspx = 1; p0 = px[0]; p1 = px[1]; p2 = px[2]; p3 = px[3];
    }
    if (px && w == 1 && h == 1) {
        g_stb_ok = (px[0] == 0 && px[1] == 0 && px[2] == 0 && px[3] == 0) ? 1 : -1;
        stbi_image_free(px);
    } else {
        g_stb_ok = -1;
        if (px) stbi_image_free(px);
    }
    DbgPrint("[STB] %dx%d c=%d ok=%d\n", g_stb_w, g_stb_h, g_stb_c, g_stb_ok);
    int iw = -1, ih = -1, ic = -1;
    int infor = stbi_info_from_memory(kPng1x1, (int)sizeof(kPng1x1), &iw, &ih, &ic);
    void *m = malloc(32);
    int mok = (m != 0) ? 1 : 0;
    if (m) free(m);
    (void)iw; (void)ih; (void)ic;
    {
        /* Probe 1 (2 varargs, reg-only, reliable): is .rodata intact? */
        static char m1[64];
        uint32_t sig = ((uint32_t)kPng1x1[0] << 24) | ((uint32_t)kPng1x1[1] << 16) |
                       ((uint32_t)kPng1x1[2] << 8) | (uint32_t)kPng1x1[3];
        snprintf(m1, sizeof m1, "SIG %08X len=%d", sig, (int)sizeof(kPng1x1));
        report360(m1);
        /* Probe 2 (10 varargs): do stack-passed varargs survive? expect 1..10 */
        static char m2[128];
        snprintf(m2, sizeof m2, "VARG %d %d %d %d %d %d %d %d %d %d",
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        report360(m2);
        /* Probe 3: decode verdict (5 varargs max, reg-only, reliable) */
        static char m3[64];
        snprintf(m3, sizeof m3, "STB w=%d h=%d c=%d ok=%d px=%d",
            g_stb_w, g_stb_h, g_stb_c, g_stb_ok, haspx);
        report360(m3);
        /* Probe 4: malloc + info radically separated (<=2 varargs each) */
        static char m4[32], m5[32], m6[48];
        snprintf(m4, sizeof m4, "MALLOC %d", mok);
        report360(m4);
        snprintf(m5, sizeof m5, "INFO %d", infor);
        report360(m5);
        snprintf(m6, sizeof m6, "IWH %d %d %d", iw, ih, ic);
        report360(m6);
        /* Probe 5: stored-block RGBA (no huffman) vs dynamic-huffman gray */
        {
            int sw = 0, sh = 0, sc = 0, gw = 0, gh = 0, gc = 0;
            unsigned char *spx = stbi_load_from_memory(kPngStored, (int)sizeof(kPngStored), &sw, &sh, &sc, 4);
            int sok = (spx && sw == 1 && sh == 1) ? 1 : 0;
            if (spx) stbi_image_free(spx);
            unsigned char *gpx = stbi_load_from_memory(kPngGray, (int)sizeof(kPngGray), &gw, &gh, &gc, 1);
            int gok = (gpx && gw == 1 && gh == 1 && gpx[0] == 0x7F) ? 1 : 0;
            if (gpx) stbi_image_free(gpx);
            static char m7[48], m8[48];
            snprintf(m7, sizeof m7, "STORED %d %dx%dx%d", sok, sw, sh, sc);
            report360(m7);
            snprintf(m8, sizeof m8, "GRAY %d %dx%dx%d", gok, gw, gh, gc);
            report360(m8);
        }
    }
    for (;;) {}
    return 0;
}
