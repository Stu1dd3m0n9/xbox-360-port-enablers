/* sdl360_video.cpp: D3D9 device owner + SW framebuffer upload. XDK group.
 * No runtime shader compile (HW hang): StretchRect from system-mem texture
 * to backbuffer + Clear. 1280x720 X8R8G8B8. Present in unlock_present.
 */
#include <xtl.h>
#include <d3dx9.h>
#include <xgraphics.h>
#include "sdl360.h"
#include <string.h>
#include <stdlib.h>
#include "shaders/blit_vs.h" /* const DWORD g_blit_vs[] */
#include "shaders/blit_ps.h" /* const DWORD g_blit_ps[] */
extern "C" int DbgPrint(const char *, ...);
struct SDL360_Window {
    D3DDevice *dev;
    D3DSurface *locked;
    uint32_t w, h;
    /* blit path: shadow CPU buffer -> MANAGED texture -> precompiled quad */
    D3DTexture *tex;
    D3DVertexDeclaration *decl;
    D3DVertexShader *vsh;
    D3DPixelShader *psh;
    uint32_t *shadow;
    int blit_ok;
    uint32_t last_pitch;
    uint32_t tex_w, tex_h;
    uint32_t tw, th; /* texture dims (32-aligned) vs content dims */
};
struct BLITVERT { float x, y, z, u, v; };
static Direct3D *g_d3d;
static SDL360_Window g_win;
static int g_video_ok;
static void gfx_x360_wm_init(const char *game_name, bool start_in_fullscreen);
SDL360_Window *sdl360_create_window(const char *title, int w, int h) {
    (void)title;
    /* Query real video mode like tri.cpp: hardcoded 720p may not own the
     * display on setups where the dash runs another mode. */
    XVIDEO_MODE vm;
    ZeroMemory(&vm, sizeof(vm));
    XGetVideoMode(&vm);
    uint32_t vw = vm.dwDisplayWidth, vh = vm.dwDisplayHeight;
    DbgPrint("[SDL360] XGetVideoMode %dx%d wide=%d\n", vw, vh, (int)vm.fIsWideScreen);
    /* Match native mode: a smaller backbuffer does not take over the full
     * 1080p scanout (image lands as a centered band). */
    g_win.w = (w > 0) ? (uint32_t)w : vw;
    g_win.h = (h > 0) ? (uint32_t)h : vh;
    if (g_win.w == 0 || g_win.h == 0) { g_win.w = 1280; g_win.h = 720; }
    if (g_video_ok) return &g_win;
    g_d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_d3d) { DbgPrint("[SDL360] Direct3DCreate9 failed\n"); return 0; }
    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));
    pp.BackBufferWidth = g_win.w; pp.BackBufferHeight = g_win.h;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8; pp.BackBufferCount = 1;
    pp.EnableAutoDepthStencil = FALSE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    HRESULT hr = g_d3d->CreateDevice(0, D3DDEVTYPE_HAL, NULL,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &g_win.dev);
    if (FAILED(hr) || !g_win.dev) { DbgPrint("[SDL360] CreateDevice hr=0x%08x\n", hr); return 0; }
    {
        D3DVIEWPORT9 vp;
        ZeroMemory(&vp, sizeof(vp));
        vp.X = 0; vp.Y = 0;
        vp.Width = pp.BackBufferWidth; vp.Height = pp.BackBufferHeight;
        vp.MinZ = 0.0f; vp.MaxZ = 1.0f;
        g_win.dev->SetViewport(&vp);
        DbgPrint("[SDL360] viewport %dx%d\n", vp.Width, vp.Height);
    }
    g_win.locked = 0;
    g_video_ok = 1;
    DbgPrint("[SDL360] video %dx%d ready\n", g_win.w, g_win.h);
    return &g_win;
}
void sdl360_destroy_window(SDL360_Window *win) { (void)win; }
void *sdl360_lock_framebuffer(SDL360_Window *win, int *pitch) {
    if (!win || !win->dev) return 0;
    if (!win->blit_ok) {
        HRESULT hr;
        hr = win->dev->CreateVertexShader((const DWORD *)g_blit_vs, &win->vsh);
        if (FAILED(hr) || !win->vsh) { DbgPrint("[SDL360] VS hr=0x%08x\n", hr); return 0; }
        hr = win->dev->CreatePixelShader((const DWORD *)g_blit_ps, &win->psh);
        if (FAILED(hr) || !win->psh) { DbgPrint("[SDL360] PS hr=0x%08x\n", hr); return 0; }
        D3DVERTEXELEMENT9 el[] = {
            { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
            { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
            D3DDECL_END()
        };
        hr = win->dev->CreateVertexDeclaration(el, &win->decl);
        if (FAILED(hr) || !win->decl) { DbgPrint("[SDL360] decl hr=0x%08x\n", hr); return 0; }
        /* Tile-size experiment: texture 736 tall (32-aligned), content 720.
         * Upload covers the full texture; UVs map the 720 content rows. */
        win->tw = win->w; win->th = (win->h + 31) & ~31u;
        hr = win->dev->CreateTexture(win->tw, win->th, 1, 0, D3DFMT_X8R8G8B8,
                                     D3DPOOL_DEFAULT, &win->tex, NULL);
        if (FAILED(hr) || !win->tex) { DbgPrint("[SDL360] tex hr=0x%08x\n", hr); return 0; }
        win->shadow = (uint32_t *)malloc(win->tw * win->th * 4);
        memset(win->shadow, 0, win->tw * win->th * 4);
        if (!win->shadow) { DbgPrint("[SDL360] shadow malloc failed\n"); return 0; }
        D3DSURFACE_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        if (SUCCEEDED(win->tex->GetLevelDesc(0, &desc))) {
            win->tex_w = desc.Width; win->tex_h = desc.Height;
            DbgPrint("[SDL360] texdesc %ux%u fmt=0x%x\n", desc.Width, desc.Height, desc.Format);
        }
        win->blit_ok = 1;
        DbgPrint("[SDL360] blit ready %dx%d\n", win->w, win->h);
    }
    if (pitch) *pitch = (int)(win->w * 4);
    return win->shadow;
}
void sdl360_unlock_present(SDL360_Window *win); /* fwd */
void sdl360_blit(SDL360_Window *win) {
    /* Manual XG tiling: LockRect exposes the (tiled) texture memory and no
     * driver path (memcpy/D3DX/LIN_) lands rows correctly, so tile the
     * linear shadow ourselves with the sanctioned XG function. */
    D3DLOCKED_RECT lr;
    if (FAILED(win->tex->LockRect(0, &lr, NULL, 0))) { DbgPrint("[SDL360] tex lock failed\n"); return; }
    win->last_pitch = (uint32_t)lr.Pitch;
    XGTileTextureLevel(win->tw, win->th, 0, XGGetGpuFormat(D3DFMT_X8R8G8B8),
                       XGTILE_NONPACKED, lr.pBits, NULL, win->shadow, win->tw * 4, NULL);
    win->tex->UnlockRect(0);
    static BLITVERT q[4];
    static int qinit = 0;
    if (!qinit) {
        /* Half-texel inset in TEXTURE space; v1 stops at content height. */
        float hu = 0.5f / (float)win->tw, hv = 0.5f / (float)win->th;
        float v1 = ((float)win->h - 0.5f) / (float)win->th;
        q[0].x = -1; q[0].y =  1; q[0].z = 0; q[0].u = hu;        q[0].v = hv;
        q[1].x =  1; q[1].y =  1; q[1].z = 0; q[1].u = 1.0f - hu;  q[1].v = hv;
        q[2].x = -1; q[2].y = -1; q[2].z = 0; q[2].u = hu;        q[2].v = v1;
        q[3].x =  1; q[3].y = -1; q[3].z = 0; q[3].u = 1.0f - hu;  q[3].v = v1;
        qinit = 1;
    }
    win->dev->SetVertexDeclaration(win->decl);
    win->dev->SetVertexShader(win->vsh);
    win->dev->SetPixelShader(win->psh);
    win->dev->SetTexture(0, win->tex);
    win->dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    win->dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    win->dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    win->dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    win->dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    win->dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(BLITVERT));
}
void sdl360_unlock_present(SDL360_Window *win) {
    if (!win || !win->dev) return;
    if (win->blit_ok && win->shadow) sdl360_blit(win);
    sdl360_present(win);
}
void sdl360_present(SDL360_Window *win) {
    if (!win || !win->dev) return;
    if (win->locked) { win->locked->UnlockRect(); win->locked->Release(); win->locked = 0; }
    HRESULT hr = win->dev->Present(NULL, NULL, NULL, NULL);
    if (FAILED(hr)) DbgPrint("[SDL360] Present hr=0x%08x\n", hr);
}
void sdl360_clear(SDL360_Window *win, uint8_t r, uint8_t g, uint8_t b) {
    if (!win || !win->dev) return;
    win->dev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(r, g, b), 1.0f, 0);
}
uint32_t sdl360_last_pitch(SDL360_Window *win) {
    return win ? win->last_pitch : 0;
}
void sdl360_tex_size(SDL360_Window *win, uint32_t *w, uint32_t *h) {
    if (w) *w = win ? win->tex_w : 0;
    if (h) *h = win ? win->tex_h : 0;
}
void sdl360_size(SDL360_Window *win, uint32_t *w, uint32_t *h) {
    if (w) *w = win ? win->w : 0;
    if (h) *h = win ? win->h : 0;
}
void sdl360_viewport(SDL360_Window *win, uint32_t *x, uint32_t *y, uint32_t *w, uint32_t *h) {
    D3DVIEWPORT9 vp;
    ZeroMemory(&vp, sizeof(vp));
    if (win && win->dev && SUCCEEDED(win->dev->GetViewport(&vp))) {
        if (x) *x = vp.X; if (y) *y = vp.Y;
        if (w) *w = vp.Width; if (h) *h = vp.Height;
    }
}
