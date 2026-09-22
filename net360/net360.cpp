/* net360.cpp: WSAStartup + blocking TCP client over XNet. XDK group. */
#include <xtl.h>
#include <winsockx.h>
#include "net360.h"
#include <string.h>
#include <stdio.h>
extern "C" int DbgPrint(const char *, ...);
static int g_ok;
int net360_init(void) {
    if (g_ok) return 1;
    XNetStartupParams p; ZeroMemory(&p, sizeof p);
    p.cfgSizeOfStruct = sizeof p; p.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
    XNetStartup(&p); /* proven order: XNetStartup before WSAStartup */
    WSADATA d; memset(&d, 0, sizeof d);
    int r = WSAStartup(MAKEWORD(2, 2), &d);
    DbgPrint("[NET360] WSAStartup r=%d\n", r);
    g_ok = (r == 0); return g_ok;
}
int net360_tcp_connect(const char *ip, uint16_t port) {
    if (!g_ok || !ip) return -1;
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return -1;
    SOCKADDR_IN a; memset(&a, 0, sizeof a);
    a.sin_family = AF_INET; a.sin_port = htons(port);
    a.sin_addr.s_addr = inet_addr(ip);
    if (connect(s, (SOCKADDR *)&a, sizeof a) != 0) { closesocket(s); return -1; }
    return (int)s;
}
int net360_send_all(int s, const void *buf, int len) {
    const char *p = (const char *)buf; int sent = 0;
    while (sent < len) {
        int n = send((SOCKET)s, p + sent, len - sent, 0);
        if (n <= 0) return -1; sent += n;
    }
    return sent;
}
int net360_recv_some(int s, void *buf, int cap) {
    if (!buf || cap <= 0) return -1;
    return recv((SOCKET)s, (char *)buf, cap, 0);
}
void net360_close(int s) { closesocket((SOCKET)s); }
void net360_title_ip(char *out, int cap) {
    if (!out || cap <= 0) return;
    XNADDR xn; memset(&xn, 0, sizeof xn);
    DWORD st = XNetGetTitleXnAddr(&xn);
    sprintf_s(out, (size_t)cap, "st=0x%08x ip=%u.%u.%u.%u",
        st, xn.ina.S_un.S_un_b.s_b1, xn.ina.S_un.S_un_b.s_b2,
        xn.ina.S_un.S_un_b.s_b3, xn.ina.S_un.S_un_b.s_b4);
    DbgPrint("[NET360] %s\n", out);
}
