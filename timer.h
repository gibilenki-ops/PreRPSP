/**
	Timer utility
	
	v0.1
		- timer interrupt initialization
		- timer application utility.

	@project	TLib
	@author	tchan@TSoft
	@date	2014/01/01
*/
#ifndef	__TIMER_H__
#define	__TIMER_H__

#include "types.h"

#define	timer_isfired(x)	((timer_get(x)==0))

void timer_init();


int timer_alloc();

int timer_free(int id);

/*
	time value in milliseconds.
	- about 4000000.000 seconds will be counted.
*/
int timer_set(int timer_id, uint32 time_value);
int timer_get(int timer_id);
int timer_clear(int timer_id);

#endif
