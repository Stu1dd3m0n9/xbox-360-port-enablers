#!/usr/bin/env python3
"""Build 360ports enablers stage 1 (lua + stb) with RXDK modern clang.
Mirrors sm64-360/tools/build_x360.py phases: compile (picolibc) -> mktitle link -> XexTool -m d sign.
Zero file I/O in tests (fopen hangs HW). Loop forever (return powers off).
Usage: py -3 build_enablers.py compile | link | all
"""
import concurrent.futures, os, re, subprocess, sys
ROOT = r"E:\360ports\enablers"
OUTDIR = r"E:\360ports\out"
OBJROOT = r"E:\360ports\build\obj"
LUA_SRC = os.path.join(ROOT, "lua", "lua-5.4.7", "src")
RXDK = r"C:\Program Files\RXDK-360"
CLANG = os.path.join(RXDK, "modern", "bin", "clang.exe")
LLD = os.path.join(RXDK, "modern", "bin", "ld.lld.exe")
XEXTOOL = os.path.join(RXDK, "modern", "bin", "XexTool.exe")
XDK_LIB = os.path.join(RXDK, "legacy", "lib")
MODERN = os.path.join(RXDK, "modern")
COFF = r"E:\sm64build\coff"
TRI = r"E:\sm64build\tri"
PYTHON = sys.executable
LUA_SKIP = {"lua.c", "luac.c", "loadlib.c"}  # tools / dlopen; io/os kept but unused by test
XDK_INC = os.path.join(RXDK, "legacy", "include", "xbox")
GAME_FLAGS = (["--target=powerpc-unknown-xbox360", "-O2", "-std=gnu99",
    "-D__Picolibc__", "-D_GNU_SOURCE", "-D_LUA_USE_POSIX=0",
    "-Wno-error=incompatible-pointer-types"] +
    ["-I" + os.path.join(MODERN, "include", "config"),
     "-I" + os.path.join(MODERN, "include", "picolibc"),
     "-include", "picolibc.h",
     "-I" + LUA_SRC, "-I" + os.path.join(ROOT, "stb"),
     "-I" + os.path.join(ROOT, "net360"),
     "-I" + os.path.join(ROOT, "compat360")])
XDK_FLAGS = (["--target=powerpc-unknown-xbox360", "-O2", "-fms-extensions",
    "-fms-compatibility", "-fdeclspec", "-fno-autolink",
    "-D_WIN32=1", "-D_M_PPCBE=1", "-D_M_PPC=1", "-D_XBOX=1", "-D_XBOX_VER=200",
    "-D__export=", "-D_SIZE_T_DEFINED", "-D_XM_NO_INTRINSICS_",
    "-isystem", XDK_INC, "-Wno-pragma-pack", "-Wno-nonportable-include-path",
    "-Wno-macro-redefined", "-Wno-ignored-pragma-intrinsic"] +
    ["-I" + os.path.join(ROOT, "net360")])
REPORT_SRC = os.path.join(ROOT, "net360", "report360.cpp")
def run(cmd, cwd=ROOT):
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if r.returncode != 0:
        print("$", " ".join(cmd)); print(r.stdout[-2000:]); print(r.stderr[-2000:])
        sys.exit(f"FAILED ({r.returncode}): {cmd[0]}")
    return r
def obj_for(src):
    rel = os.path.relpath(src, ROOT).replace(os.sep, "_").replace(".", "_")
    d = OBJROOT; os.makedirs(d, exist_ok=True)
    return os.path.join(d, rel + ".o")
def collect():
    lua = [os.path.join(LUA_SRC, f) for f in sorted(os.listdir(LUA_SRC))
           if f.endswith(".c") and f not in LUA_SKIP]
    compat = [os.path.join(ROOT, "compat360", "compat360.c")]
    t_lua = os.path.join(ROOT, "lua", "test_lua360.c")
    t_stb = os.path.join(ROOT, "stb", "test_stb360.c")
    return lua, compat, t_lua, t_stb
def compile_one(args):
    src, obj, flags = args
    if os.path.exists(obj) and os.path.getmtime(obj) >= os.path.getmtime(src):
        return (src, True, "")
    fl = list(flags)
    if src.endswith(".cpp"): fl = [f for f in fl if not f.startswith("-std=")] + ["-std=c++11"]
    cmd = [CLANG] + fl + ["-c", src, "-o", obj]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0: return (src, False, (r.stdout + r.stderr)[-1500:])
    return (src, True, "")
def phase_compile():
    lua, compat, t_lua, t_stb = collect()
    all_src = lua + compat + [t_lua, t_stb]
    print(f"TUs: {len(lua)} lua + {len(compat)} compat + 2 tests + 1 report = {len(all_src) + 1}")
    jobs = [(s, obj_for(s), GAME_FLAGS) for s in all_src] + [(REPORT_SRC, obj_for(REPORT_SRC), XDK_FLAGS)]
    fails = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as ex:
        for src, ok, err in ex.map(compile_one, jobs):
            if not ok: fails.append((src, err))
    print(f"compiled {len(jobs)-len(fails)}/{len(jobs)}")
    for src, err in fails[:10]: print("FAIL:", src, err[-800:])
    if fails: sys.exit(f"{len(fails)} compile failures")
    return {s: obj_for(s) for s in all_src + [REPORT_SRC]}
def phase_link(m):
    os.makedirs(OUTDIR, exist_ok=True)
    lua, compat, t_lua, t_stb = collect()
    helpers = os.path.join(COFF, "helpers.a")
    assert os.path.exists(helpers), f"missing {helpers}"
    xnet = os.path.join(COFF, "xnet.a")
    assert os.path.exists(xnet), f"missing {xnet}"
    rep = m[REPORT_SRC]
    groups = {
        "test_lua360": [m[s] for s in lua] + [m[s] for s in compat] + [m[t_lua], rep],
        "test_stb360": [m[s] for s in compat] + [m[t_stb], rep],
    }
    for name, objs in groups.items():
        xex = os.path.join(OUTDIR, name + ".xex")
        cmd = ([PYTHON, os.path.join(TRI, "mktitle.py")] + objs +
               ["--lib", "xnet," + helpers, "--coff-dir", COFF, "--xdk", XDK_LIB,
                "--clang", CLANG, "--lld", LLD, "--keep-elf",
                "--ldflag=--no-dependent-libraries",
                "--ldflag=-Wl,--allow-multiple-definition", "-o", xex])
        r = subprocess.run(cmd, capture_output=True, text=True)
        tail = (r.stdout + r.stderr)[-3000:]
        if r.returncode != 0 or "unresolved" in tail:
            print(tail); sys.exit(f"link failed: {name}")
        print(f"packed: {xex}")
        signed = os.path.join(OUTDIR, name + "_signed.xex")
        run([XEXTOOL, "-m", "d", "-o", signed, xex])
        print(f"signed: {signed}")
if __name__ == "__main__":
    phase = sys.argv[1] if len(sys.argv) > 1 else "all"
    if phase == "compile": phase_compile()
    elif phase == "link":
        lua, compat, t_lua, t_stb = collect()
        m = {s: obj_for(s) for s in lua + compat + [t_lua, t_stb, REPORT_SRC]}
        phase_link(m)
    elif phase == "all": phase_link(phase_compile())
    else: sys.exit("usage: build_enablers.py [compile|link|all]")
