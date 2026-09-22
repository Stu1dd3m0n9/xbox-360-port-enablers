# Lua notes

Not vendored. Fetch at build time:

- lua-5.4.7: https://www.lua.org/ftp/lua-5.4.7.tar.gz
- Exclude from the 360 lib: `lua.c`, `luac.c` (host tools), `loadlib.c`
  (no `dlopen` on 360; `compat360` stubs `luaopen_package` so `require`
  fails cleanly while the core language works).

Compile the rest with the picolibc game flags; link `helpers.a` + compat360
for `__fixdfdi`/`luaopen_package`. Verified: `dostring("return 2+2")` -> 4.
