/* report360.h: one-line TCP result report to PC listener. Generic.
 * Picolibc-safe (no xtl). Listener: E:\360ports\out\listen360.py :4243.
 */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void report360(const char *line);
#ifdef __cplusplus
}
#endif
