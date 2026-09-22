/* test_net360.c: game-group. WSAStartup + title IP print. No external server
 * needed; connect attempt to PC listener is optional and never blocks boot. */
#include "net360.h"
#include "report360.h"
void DbgPrint(const char *fmt, ...);
volatile int g_net = 0;
int main(void) {
    char ip[128];
    g_net = net360_init() ? 1 : -1;
    net360_title_ip(ip, sizeof ip);
    DbgPrint("[NET360] init=%d\n", g_net);
    report360(g_net == 1 ? "NET PASS wsa+xnaddr" : "NET FAIL");
    for (;;) {}
    return 0;
}
