/**
	SDLog remote(serial) interface

	with synchronous method..

	@project	201406-FWL
	@author	tchan@TSoft
	@date	2015/11/25
*/
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include "uart1.h"

#include "SDL.h"

static char sdl_state;		// MMC card detection state : 'R', 'D', 'E'
//static char file_state;		// 0:not open, 1:open
static uint8 rFlag = 0;		// response check flag.

// wait response
static int sdl_wait(uint16 timeoutMs);

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
	
	// wait maximum 7 seconds.
	rFlag = 0;
	return sdl_wait(7*1000);	
}

/**
	Close file, if open.

	note) don't care whether open or not.
*/
int sdl_close()
{
	sdl_write("@@FC=0\r");
	rFlag = 0;
	return sdl_wait(3*1000);	
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
	int wCnt = 0;		// write count

	while (*p)
	{
		while (uart1_putchar(*p) == 0);
		p++;

		wCnt++;
		
		// wait per 100 char.
		if (wCnt > 100)
		{
			// wait ok.
			rFlag = 0;
			sdl_wait(300);
			wCnt = 0;
		} 
	}

	return 0;
}

/**
	max timeout 60 secons
*/
static int sdl_wait(uint16 timeoutMs)
{
	do
	{
		sdl_process();

		if (rFlag == 1)
		{
			return 0;
		}

		_delay_us(500);		// consider process time
		timeoutMs--;
	}
	while (timeoutMs > 0);

	// timeout
	return -1;
}


/**
	handle response line.
	--
*/
static void sdl_handle_response(const char *resBuff)
{
	if (strncmp(resBuff, "SE:", 3) == 0)
	{
		sdl_state = resBuff[3];
	}
	else if (strncmp(resBuff, "OK", 2) == 0)
	{
		rFlag = 1;
	}
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
#define	MAX_LEN	16
	static char rxBuff[MAX_LEN+1];
	static int	 rxLen = 0;

	uint8 c;
	if (uart1_getchar(&c) == 1)
	{
		if (c == '\r')
		{
			rxBuff[rxLen] = 0;		// make string.

			sdl_handle_response(rxBuff);

			/*
			if (strncmp(rxBuff, "SE:", 3) == 0)
			{
				sdl_state = rxBuff[3];
			}
			else if (strncmp(rxBuff, "OK", 2) == 0)
			{
				rFlag = 1;
			}
			*/

// special command for... : format = "SET SN=AF001"
void handle_config_command(const char *resBuff);

			// new for serial no configuration.
			handle_config_command(rxBuff);

			rxLen = 0;
		}
		else if (c == '\n')	// ignore this.
		{
			// do nothing.
		}
		else
		{
			if (rxLen < MAX_LEN)		// 7 char only for string.
			{
				rxBuff[rxLen++] = c;
			}
		}
	}
}

//-----------------------------------------------------------------------------


