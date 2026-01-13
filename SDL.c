/**
	SDLog remote(serial) interface

	@project	201406-FWL
	@author	tchan@TSoft
	@date	2015/11/25
*/
#include <stdio.h>
#include <string.h>
#include "uart1.h"

#include "SDL.h"

static char sdl_state;

/**
	Create log file.
*/
int sdl_init()
{
	uart1_init(BAUD_38400);

	sdl_state = 0;
	return 0;
}

/**
	state
	'R'	:
	'D'
*/
char sdl_getState()
{
	if ((sdl_state == 'R') || (sdl_state == 'D'))	return sdl_state;
	else	return 'E';
}

/**
	Create file..

	note) filename should not include ','
		only 8.3 format for name

	
*/
int sdl_create(char *fileName)
{
	char buff[32+8];

	sprintf(buff, "@@FO=0,%s,a\r", fileName);

	// write command.
	sdl_write(buff);	
	return 0;
}

/**
	Close file, if open.

	note) don't care whether open or not.
*/
int sdl_close()
{
	sdl_write("@@FC=0\r");
	return 0;
}


/**
	write a line of log

	note) no end of line char needed.
*/
int sdl_writeLine(char *str)
{
	char *p = str;

	while (*p)
	{
		if ((*p == '\r') || (*p == '\n')) break;

		while (uart1_putchar(*p) == 0);
		p++;
	}

	// put "end of line" char.
	while (uart1_putchar('\r') == 0);

	return 0;
}

int sdl_write(char *str)
{
	char *p = str;

	while (*p)
	{
		while (uart1_putchar(*p) == 0);
		p++;
	}

	return 0;
}

/**
	Parse SDLogger response/events.


	SE:<st><CR><LF>
		<st> = 'R', 'D'.
	OK
	ERROR
*/
void sdl_process()
{
	static char rxBuff[8];
	static int	 rxLen = 0;

	uint8 c;
	if (uart1_getchar(&c) == 1)
	{
		if (c == '\r')
		{
			rxBuff[rxLen] = 0;		// make string.

			if (strncmp(rxBuff, "SE:", 3) == 0)
			{
				sdl_state = rxBuff[3];
			}
			rxLen = 0;
		}
		else if (c == '\n')	// ignore this.
		{
			// do nothing.
		}
		else
		{
			if (rxLen < 7)		// 7 char only for string.
			{
				rxBuff[rxLen++] = c;
			}
		}
	}
}
