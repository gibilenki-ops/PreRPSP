#ifndef  __UART1_H__
#define  __UART1_H__
/**
	AT128 uart interface.

	@version 1.1
		- WIN32 simulation codes removed. (see uart-sim.c/h)

	2010/10/22. multiple BAUD rate support,
				F_CPU definition MUST be supplied.
	2009/07/22. putter/getter interface added.
				uartX_init() method added.
*/
#include "uart.h"

int uart1_init(uint32 baud);
int	uart1_getchar(uint8 *c);
int uart1_putchar(uint8 c);

#endif
//__UART1_H__
