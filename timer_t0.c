/**
	Timer utility
	- implementation using timer0, for atmega128,...
	
	v0.1
		- timer interrupt initialization
		- timer application utility.

	@project	TLib
	@author	tchan@TSoft
	@date	2014/01/01
*/
#include "avr/io.h"
#include "avr/interrupt.h"

#include "timer.h"

#define	MAX_SYS_TIMER	16

// 1ms interval, 1000 tick per seconds
#ifdef	F_CPU
#define	CPUCLK	F_CPU
#else
#define	CPUCLK		16000000L
#endif

#define	TIMER_TICK	1000
#define TIMER_TCNT (CPUCLK/256/TIMER_TICK)
//-- @endnote

typedef	struct _sys_timer
{
	uint8	flag;		// bit7:allocated, bit0:running flag
	uint32	value;
} sys_timer;

sys_timer timer_list[MAX_SYS_TIMER];

// internal timer count..
uint32 timerCount;


int timer_alloc()
{
	int i;
	for (i=0; i<MAX_SYS_TIMER; i++)
	{
		if ((timer_list[i].flag & 0x80) == 0)
		{
			timer_list[i].flag = 0x80;
			return i;
		}
	}
	return -1;
}

int timer_free(int id)
{
	if ( (id < 0) || (id >= MAX_SYS_TIMER) )
	{
		return -1;
	}

	timer_list[id].flag = 0;
	return 0;
}

int timer_set(int timer_id, uint32 time_value)
{
	if ( (timer_id < 0) || (timer_id >= MAX_SYS_TIMER) )
	{
		return -1;
	}
	
	if (timer_list[timer_id].flag & 0x80)	// allocated
	{
		timer_list[timer_id].value = time_value;
		timer_list[timer_id].flag  = 0x81;
		return 0;
	}
	else
	{
		return -1;
	}
}

/*
	@return	
		0	: is fired (timeout)
		1	: running (not fired yet)
		-1	: not-running
*/

int timer_get(int timer_id)
{
	if ( (timer_id < 0) || (timer_id >= MAX_SYS_TIMER) )
	{
		return -1;
	}

	if (timer_list[timer_id].flag == 0x81)
	{
		if (timer_list[timer_id].value == 0)
			return 0;
		else
			return 1;
	}
	else
	{
		return -1;
	}
}

int timer_clear(int timer_id)
{
	if ( (timer_id < 0) || (timer_id >= MAX_SYS_TIMER) )
	{
		return -1;
	}

	if (timer_list[timer_id].flag & 0x80)	// allocated
	{
		timer_list[timer_id].flag = 0x80;
		return -1;
	}
	else
	{
		return 0;
	}
}


/**
	initialize timer.
	- timer utility init
	- timer register setting
*/
void timer_init()
{
	for (int i=0; i<MAX_SYS_TIMER; i++)
	{
		timer_list[i].flag  = 0;
		timer_list[i].value = 0;
	}
	
#if defined(TIMSK) && defined(OCR0)
	// timer0 - ATmega128.
	// Timer/Counter 0 initialization
//	ASSR=0x00;

	// TCCR0 : FOC0, WGM00, COM01, COM00, WGM01, CS02, CS01, CS00
		// CS02,CS01,CS00 : 1=clk, 2=clk/8, 3=clk/32, 4=clk/64, 5=clk/128, 6=clk/256, 7=clk/1024.
		// WGM01,WGM00 : 0x10=CTC(Clear Timer on Compare match) mode; top = OCR0, 

	OCR0=TIMER_TCNT;		// about 1 ms : 31 tick at clk/256 mode.
	TCNT0=0x00;
	TCCR0=0x06 | 0x08;		// clk/256, CTC mode.

	TIMSK=0x02;
		// 0x02: OCIE0 - timer0 compare match interrupt enable.

#elif defined(TIMSK0)

	//---------------------------
	//1280,2560 - 8bit Å¸ÀÌ¸Ó A,B
	TCNT0  = 0;
				// WGM[2..0] bits = TCCR0A[......10],TCCR0B[.....2..]
				// COM0A[1..0] = TCCR0A[10......]
	TCCR0A = 0x02;		// Normal mode.  WGMn[2-0] = b[010] - CTC mode.
						// COM0A[1,0] = b[00], OC0A normal port operation.
	TCCR0B = 0x04;		// CSn2/1/0 = b100, clk_io/256, CTC mode.
	OCR0A = TIMER_TCNT;	//value=31 for 8Mhz;		// 1ms
	//
	TIMSK0 |=0x02;	// enable OCIE0A, don't touch other flags.
#else
#error "Not supported platform"
#endif
}

#if	defined(TIMSK) && defined(OCR0)
	// 128
	ISR(TIMER0_COMP_vect)
#elif defined(TIMSK0)
	// 1280
	ISR(TIMER0_COMPA_vect)
#else
#error "Not supported platform"
#endif
{
	int i;
	
	// Need not clear TCNT1, it is automatically cleared on CTC mode.(Clear Timer on Compare match)
	// TCNT1 = 0;
	// 1ms timer.
	timerCount++;

	// timer decreasing.
	for (i=0; i<MAX_SYS_TIMER; i++)
	{
		if (timer_list[i].value > 0)
		{
			timer_list[i].value--;
		}
	}
}

