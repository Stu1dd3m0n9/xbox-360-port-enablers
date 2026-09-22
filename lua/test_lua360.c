/* test_lua360.c: zero-file-I/O lua validation for RXDK modern.
 * Builds lua as game-group objects (picolibc), links via mktitle.py,
 * runs dostring("return 2+2") and traps result in a volatile for XBDM.
 * Never returns (return powers the box off).
 */
#include <stddef.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "report360.h"
void DbgPrint(const char *fmt, ...);
volatile int g_lua_result = -1;
volatile int g_lua_done = 0;
int main(void) {
    lua_State *L = luaL_newstate();
    if (!L) { DbgPrint("[LUA] newstate NULL\n"); for (;;) {} }
    luaL_openlibs(L);
    if (luaL_dostring(L, "return 2+2") != LUA_OK) {
        DbgPrint("[LUA] dostring err\n"); for (;;) {}
    }
    if (!lua_isinteger(L, -1)) { DbgPrint("[LUA] not int\n"); for (;;) {} }
    g_lua_result = (int)lua_tointeger(L, -1);
    g_lua_done = (g_lua_result == 4) ? 1 : -1;
    DbgPrint("[LUA] result=%d done=%d\n", g_lua_result, g_lua_done);
    report360(g_lua_done == 1 ? "LUA PASS 2+2=4" : "LUA FAIL");
    lua_close(L);
    for (;;) {}
    return 0;
}
