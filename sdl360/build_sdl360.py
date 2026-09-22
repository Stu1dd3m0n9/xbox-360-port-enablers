#!/usr/bin/env python3
"""Build sdl360+net360 (generic, no SM64 types). XDK impl + picolibc tests.
Usage: py -3 build_sdl360.py compile|link|all
"""
import concurrent.futures, os, subprocess, sys
ROOT = r"E:\360ports\enablers"
OUTDIR = r"E:\360ports\out"
OBJROOT = r"E:\360ports\build\obj360"
RXDK = r"C:\Program Files\RXDK-360"
CLANG = os.path.join(RXDK, "modern", "bin", "clang.exe")
LLD = os.path.join(RXDK, "modern", "bin", "ld.lld.exe")
XEXTOOL = os.path.join(RXDK, "modern", "bin", "XexTool.exe")
XDK_INC = os.path.join(RXDK, "legacy", "include", "xbox")
XDK_LIB = os.path.join(RXDK, "legacy", "lib")
MODERN = os.path.join(RXDK, "modern")
COFF = r"E:\sm64build\coff"
TRI = r"E:\sm64build\tri"
PYTHON = sys.executable
GAME_FLAGS = (["--target=powerpc-unknown-xbox360", "-O2", "-std=gnu99",
    "-D__Picolibc__", "-D_GNU_SOURCE", "-Wno-error=incompatible-pointer-types"] +
    ["-I" + os.path.join(MODERN, "include", "config"),
     "-I" + os.path.join(MODERN, "include", "picolibc"),
     "-include", "picolibc.h",
     "-I" + os.path.join(ROOT, "sdl360"), "-I" + os.path.join(ROOT, "net360"),
     "-I" + os.path.join(ROOT, "compat360")])
XDK_FLAGS = (["--target=powerpc-unknown-xbox360", "-O2", "-fms-extensions",
    "-fms-compatibility", "-fdeclspec", "-fno-autolink",
    "-D_WIN32=1", "-D_M_PPCBE=1", "-D_M_PPC=1", "-D_XBOX=1", "-D_XBOX_VER=200",
    "-D__export=", "-D_SIZE_T_DEFINED", "-D_XM_NO_INTRINSICS_",
    "-isystem", XDK_INC, "-Wno-pragma-pack", "-Wno-nonportable-include-path",
    "-Wno-macro-redefined", "-Wno-ignored-pragma-intrinsic"] +
    ["-I" + os.path.join(ROOT, "sdl360"), "-I" + os.path.join(ROOT, "net360")])
GAME_SRCS = [os.path.join(ROOT, "sdl360", "sdl360_input.c"),
    os.path.join(ROOT, "sdl360", "test_sdl360.c"),
    os.path.join(ROOT, "sdl360", "test_vclear.c"),
    os.path.join(ROOT, "sdl360", "test_snd.c"),
    os.path.join(ROOT, "sdl360", "test_pad.c"),
    os.path.join(ROOT, "net360", "test_net360.c"),
    os.path.join(ROOT, "sdl360", "test_red.c"),
    os.path.join(ROOT, "sdl360", "test_grad.c"),
    os.path.join(ROOT, "sdl360", "test_geo.c"),
    os.path.join(ROOT, "compat360", "compat360.c")]
XDK_SRCS = [os.path.join(ROOT, "sdl360", "sdl360_sys.cpp"),
    os.path.join(ROOT, "sdl360", "sdl360_video.cpp"),
    os.path.join(ROOT, "sdl360", "sdl360_audio.cpp"),
    os.path.join(ROOT, "sdl360", "sdl360_thread.cpp"),
    os.path.join(ROOT, "net360", "net360.cpp"),
    os.path.join(ROOT, "net360", "report360.cpp")]
def run(cmd, cwd=ROOT):
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if r.returncode != 0:
        print("$", " ".join(cmd)); print(r.stdout[-2000:]); print(r.stderr[-2000:])
        sys.exit(f"FAILED ({r.returncode}): {cmd[0]}")
    return r
def obj_for(src):
    rel = os.path.relpath(src, ROOT).replace(os.sep, "_").replace(".", "_")
    os.makedirs(OBJROOT, exist_ok=True); return os.path.join(OBJROOT, rel + ".o")
def one(args):
    src, obj, flags = args
    if os.path.exists(obj) and os.path.getmtime(obj) >= os.path.getmtime(src):
        return (src, True, "")
    fl = list(flags)
    if src.endswith(".cpp"): fl = [f for f in fl if not f.startswith("-std=")] + ["-std=c++11"]
    r = subprocess.run([CLANG] + fl + ["-c", src, "-o", obj], capture_output=True, text=True)
    if r.returncode != 0: return (src, False, (r.stdout + r.stderr)[-1500:])
    return (src, True, "")
def compile():
    jobs = [(s, obj_for(s), GAME_FLAGS) for s in GAME_SRCS] + [(s, obj_for(s), XDK_FLAGS) for s in XDK_SRCS]
    print(f"TUs: {len(GAME_SRCS)} game + {len(XDK_SRCS)} xdk = {len(jobs)}")
    fails = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as ex:
        for src, ok, err in ex.map(one, jobs):
            if not ok: fails.append((src, err))
    print(f"compiled {len(jobs)-len(fails)}/{len(jobs)}")
    for s, e in fails[:10]: print("FAIL:", s, e[-800:])
    if fails: sys.exit(f"{len(fails)} failures")
    return {s: obj_for(s) for s, _, _ in jobs}
def link(m):
    import os as _o
    _o.makedirs(OUTDIR, exist_ok=True)
    for lib in ["d3d9", "d3dx9", "xgraphics", "xaudio2", "xmcore", "xapilib", "xnet"]:
        out = _o.path.join(COFF, lib + ".a")
        if not _o.path.exists(out):
            run([PYTHON, _o.path.join(TRI, "coff2elf.py"), "archive",
                 _o.path.join(XDK_LIB, lib + ".lib"), "-o", out])
    helpers = _o.path.join(COFF, "helpers.a")
    groups = {
        "test_sdl360": [m[GAME_SRCS[0]], m[GAME_SRCS[9]]] + [m[s] for s in XDK_SRCS[1:4]] + [m[XDK_SRCS[0]], m[XDK_SRCS[5]], m[GAME_SRCS[1]]],
        "test_net360": [m[GAME_SRCS[9]], m[XDK_SRCS[0]], m[XDK_SRCS[4]], m[XDK_SRCS[5]], m[GAME_SRCS[5]]],
        "test_vclear": [m[GAME_SRCS[9]], m[XDK_SRCS[0]], m[XDK_SRCS[1]], m[XDK_SRCS[5]], m[GAME_SRCS[2]]],
        "test_snd": [m[GAME_SRCS[9]], m[XDK_SRCS[0]], m[XDK_SRCS[2]], m[XDK_SRCS[5]], m[GAME_SRCS[3]]],
        "test_pad": [m[GAME_SRCS[0]], m[GAME_SRCS[9]], m[XDK_SRCS[0]], m[XDK_SRCS[5]], m[GAME_SRCS[4]]],
        "test_red": [m[GAME_SRCS[9]], m[XDK_SRCS[0]], m[XDK_SRCS[1]], m[XDK_SRCS[5]], m[GAME_SRCS[6]]],
        "test_grad": [m[GAME_SRCS[9]], m[XDK_SRCS[0]], m[XDK_SRCS[1]], m[XDK_SRCS[5]], m[GAME_SRCS[7]]],
        "test_geo": [m[GAME_SRCS[9]], m[XDK_SRCS[0]], m[XDK_SRCS[1]], m[XDK_SRCS[5]], m[GAME_SRCS[8]]],
    }
    for name, objs in groups.items():
        xex = _o.path.join(OUTDIR, name + ".xex")
        cmd = ([PYTHON, _o.path.join(TRI, "mktitle.py")] + objs +
               ["--lib", "d3d9,d3dx9,xgraphics,xaudio2,xmcore,xapilib,xnet," + helpers,
                "--coff-dir", COFF, "--xdk", XDK_LIB, "--clang", CLANG, "--lld", LLD,
                "--keep-elf", "--ldflag=--no-dependent-libraries",
                "--ldflag=-Wl,--allow-multiple-definition", "-o", xex])
        r = subprocess.run(cmd, capture_output=True, text=True)
        tail = (r.stdout + r.stderr)[-3000:]
        if r.returncode != 0 or "unresolved" in tail: print(tail); sys.exit(f"link failed: {name}")
        print(f"packed: {xex}")
        run([XEXTOOL, "-m", "d", "-o", _o.path.join(OUTDIR, name + "_signed.xex"), xex])
        print(f"signed: {name}_signed.xex")
if __name__ == "__main__":
    ph = sys.argv[1] if len(sys.argv) > 1 else "all"
    if ph == "compile": compile()
    elif ph == "link": link({s: obj_for(s) for s in GAME_SRCS + XDK_SRCS})
    elif ph == "all": link(compile())
    else: sys.exit("usage: build_sdl360.py [compile|link|all]")
