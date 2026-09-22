/* report360.cpp: XNetStartup -> WSAStartup -> TCP 192.168.137.1:4243, send
 * line, close. XDK group. Fire-and-forget: never hangs boot (short timeout
 * via non-blocking connect + select; gives up silently).
 */
#include <xtl.h>
#include <winsockx.h>
#include <string.h>
#include "report360.h"
extern "C" int DbgPrint(const char *, ...);
static int g_net;
static void netup(void) {
    if (g_net) return;
    XNetStartupParams p; ZeroMemory(&p, sizeof p);
    p.cfgSizeOfStruct = sizeof p; p.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
    if (XNetStartup(&p) != 0) return;
    WSADATA w; ZeroMemory(&w, sizeof w);
    if (WSAStartup(MAKEWORD(2, 2), &w) != 0) return;
    g_net = 1;
}
extern "C" void report360(const char *line) {
    netup();
    if (!g_net || !line) return;
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return;
    u_long nb = 1; ioctlsocket(s, FIONBIO, &nb);
    SOCKADDR_IN a; memset(&a, 0, sizeof a);
    a.sin_family = AF_INET; a.sin_port = htons(4243);
    a.sin_addr.s_addr = inet_addr("192.168.137.1");
    connect(s, (const SOCKADDR *)&a, sizeof a); /* non-blocking: EINPROGRESS ok */
    fd_set w; FD_ZERO(&w); FD_SET(s, &w);
    TIMEVAL tv; tv.tv_sec = 3; tv.tv_usec = 0;
    if (select(0, NULL, &w, NULL, &tv) > 0) {
        nb = 0; ioctlsocket(s, FIONBIO, &nb);
        int n = (int)strlen(line);
        send(s, line, n, 0);
    }
    closesocket(s);
    DbgPrint("[RPT] %s\n", line);
}
