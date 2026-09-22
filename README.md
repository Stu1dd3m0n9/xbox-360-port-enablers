# xbox360-enablers

Reusable building blocks for Xbox 360 homebrew (RGH/JTAG + devkits) built
with RXDK-360: D3D9 framebuffer blit, XAudio2 audio, XInput pads,
threads/timer, TCP reporting, plus Lua and stb notes. Everything here is
verified on real hardware.

Requires: RXDK-360 1.0.0, your own Xbox 360 SDK install, Visual Studio 2022,
Python 3 (`py -3` launcher). No XDK content is redistributed.

## Modules

| Module | What | HW proof |
|---|---|---|
| `sdl360/` | D3D9 blit (precompiled quad VS/PS), XAudio2 push, XInput, threads, timer | quadrants pixel-perfect, 300f video, 60x audio push, pad seen |
| `net360/` | XNet/WSA TCP wrapper + one-line result reporter (`:4243`) | round-trip reports from retail HW |
| `compat360/` | fopen guard, `__fixdfdi`/`__fixunsdfdi` helpers, `luaopen` stub | lua `2+2=4` links + runs |
| `lua/` | build notes + pinned tarball (fetched, not vendored) | see above |
| `tools/` | build scripts, `listen360.py` PC listener | — |
| `docs/` | hardware findings | — |

## Build

```text
py -3 tools/build_enablers.py all        # lua + stb validation titles
py -3 sdl360/build_sdl360.py all         # sdl360 + net360 validation titles
```

Output XEXs are dev-signed (`XexTool -m d`) and run on RGH/JTAG and devkits.
Deploy with `xbcp`, run with `xbreboot`, park the box back on xshell when done.

## Hardware rules (see docs/HW-NOTES.md)


1. No `D3DXCompileShader` on HW — precompile with `fxc /Fh`.
2. printf-style calls: only the first 5 varargs survive (picolibc vs MS-PPC ABI).
3. Textures are TILED: upload with `XGTileTextureLevel` into the locked pointer.
   `D3DUSAGE_DYNAMIC`, `StretchRect`, `GetRenderTargetData` don't exist here.
4. Backbuffer `LockRect` faults — never lock the backbuffer.

## License

MIT — see LICENSE. Third-party bits (Lua, stb) stay upstream; fetch, don't vendor.
