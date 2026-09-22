# Hardware notes (xdkbuild Zephyr behind RGLoader, RXDK-360)

All reproduced on real hardware. Numbers are load-bearing, hunches are marked.

## 1. Title fopen() hangs the whole box

Any `fopen()` — read or write, relative or `xe:\` absolute — never returns.
No exception, XBDM dies, hard reboot required. Zero-file-I/O titles run fine.
Workaround: embed every asset (byte arrays / `.inc.c` / assembler `.incbin`),
fail cleanly (`NULL + errno`) anywhere a path can't be resolved.

## 2. D3DXCompileShader hangs the whole box

Any profile, trivial shader, even on a 4 MB-stack thread. Guide button dies.
Workaround: precompile everything with XDK `fxc /Fh` (raw DWORD arrays —
`/Fo` containers are rejected as `E_OUTOFMEMORY`). 360 fxc rejects `discard`
and custom `INPUTn` semantics; use alpha-test state + `TEXCOORDn` varyings.

## 3. Returning from main() powers the box off

`exit()` / main-return calls `HalReturnToFirmware(0)`. Every title ends in an
infinite loop. Validation titles report over TCP, then loop on a delay.

## 4. D3D9 gaps vs PC

- No backbuffer `LockRect`: access-violates inside `d3d9!FindSurfaceWithinTexture`
  (`0xC0000005`). DEFAULT-pool surfaces are not lockable. Ever.
- No `StretchRect`, `GetRenderTargetData`, `CreateOffscreenPlainSurface`,
  no `D3DUSAGE_DYNAMIC`. Confirmed absent from 21256 headers.
- `D3DXLoadSurfaceFromMemory` returns `0x8876086c` (`INVALIDCALL`) for
  texture-level destinations on this path. Do not rely on it.
- `D3DUSAGE_CPU_CACHED_MEMORY` (0x4, 360-only) is the sanctioned CPU-write flag.

## 5. Printf varargs: 5 survive

`VARG 1 2 3 4 5 0 0 0 -1 0`: picolibc `vsnprintf` loses every vararg past the
5th register-passed one (PPC MS-PPC ABI vs picolibc `va_list` layout).
Fixed-arg calls are unaffected. Keep debug prints to <=5 varargs or split them.
Suspected contributor to garbled logs elsewhere — audit prints first.

## 6. Textures are TILED; upload with XG

`LockRect` exposes raw tiled bytes (pitch is decorative). Linear memcpy,
`LIN_` formats, and D3DX all produce the classic tile mosaic. Proven path:

- plain tiled `X8R8G8B8`, `D3DPOOL_DEFAULT`
- `LockRect` -> `XGTileTextureLevel(w,h,0,XGGetGpuFormat(fmt),XGTILE_NONPACKED,
  locked,NULL,linear,pitch,NULL)` -> `UnlockRect`
- draw fullscreen strip with precompiled VS/PS (`CULL_NONE`, explicit
  viewport at native mode from `XGetVideoMode`, POINT+CLAMP, half-texel UVs,
  32-aligned texture height with content-mapped `v1`)

Verified pixel-perfect (R/G/B/W quadrants) at 1920x1088 texture for 1080p.

## 7. Compiler-helper runtime is incomplete

`modern\lib` lacks PPC helpers the backend emits: savres/restore family,
`__u64tod`, `_RtlCheckStack12` (carve from XDK `libcMT.lib`), plus float/int
conversions (`__fixdfdi` etc. — XDK CRT has none; integer-only IEEE754
implementations live in `compat360/`). stb v2.30 additionally needs `.TOC.`
(thread-local) support that does not link — stay on v2.19 (HW-proven).

## 8. XDK/network environment

- Debug NIC (XBDM/tools) vs title NIC (game traffic) have different MACs/IPs.
  Reports leave via the title NIC; listeners bind `0.0.0.0`.
- `XNetStartup(BYPASS_SECURITY)` before `WSAStartup` (proven order).
- `xbcapture` returns static for custom titles — verify visuals on TV/capture card.
- After test runs, park the box back on xshell; titles that sit in delay loops
  can wedge later launches on flaky setups.
