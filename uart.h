#ifndef  __UART_H__
#define  __UART_H__
/**
	uart interface.

	@author	tchan@Tsoft.

	2010/10/22. multiple BAUD rate support,
				F_CPU definition MUST be supplied.
	2009/07/22. putter/getter interface added.
				uartX_init() method added.
*/

#if	defined(__GNUC__) 
//-- GNU Compiler
#include "avr/io.h"
#include "avr/interrupt.h"

#else
#pragma error "ONLY GCC supported"
#endif

#include "types.h"

#ifndef	F_CPU
	// NOTE: this value(system clock) should be provided.
#pragma	error "F_CPU definition needed"
#endif

#define	BAUD_9600		9600L
#define	BAUD_14400		14400L
#define	BAUD_19200		19200L
#define	BAUD_28800		28800L
#define	BAUD_38400		38400L
#define	BAUD_57600		57600L
#define	BAUD_76800		76800L
#define	BAUD_115200		115200L
#define	BAUD_230400		230400L
#define	BAUD_250000		250000L


/**
	command writer (via uart) setting

		int uart_putchar0(uint8 *c);
*/
typedef	int (*uart_getter)(uint8 *c);
typedef	int (*uart_putter)(uint8  c);


int uart_init(uint32 baud);
int uart_getchar(uint8 *c);
int uart_putchar(uint8 c);

#endif
//__UART_H__
