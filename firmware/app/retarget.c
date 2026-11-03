/* No application printf on the binary protocol UART. */
#include <stdio.h>
#if defined(__CC_ARM)
#pragma import(__use_no_semihosting)
#endif
int fputc(int ch,FILE *stream) { (void)stream; return ch; }
void _ttywrch(int ch) { (void)ch; }
void _sys_exit(int code) { (void)code; for(;;) { } }
