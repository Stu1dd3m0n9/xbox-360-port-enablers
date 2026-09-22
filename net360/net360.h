/* net360.h: thin XNet/WSA wrapper. Generic. No curl/mbedTLS (too heavy for
 * 512MB PPC stage 1). Raw TCP client proven on HW; TLS deferred.
 */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int net360_init(void);
int net360_tcp_connect(const char *ip, uint16_t port);
int net360_send_all(int s, const void *buf, int len);
int net360_recv_some(int s, void *buf, int cap);
void net360_close(int s);
void net360_title_ip(char *out, int cap);
#ifdef __cplusplus
}
#endif
