/**
	
*/
#ifndef	__GLCD_H__
#define	__GLCD_H__

#include "types.h"


void GLCD_init();

/**
	User level coordination.

	x : 0..127	horizontal.
	y : 0..7	vertical : 8 bits order.

	note) address is swapped compared to WG12864A-YYH-V LCD driver.
*/
void GLCD_move(uint8 x, uint8 y);

/**
	Put vertical 8 bits point data, at current position.
	current position is set by gdev_move().
*/
void GLCD_put(uint8 data);

/*
	GLCD에 1개의 제어명령을 WRITE 
*/
void GLCD_command(unsigned char command);

/**
	GLCD의 현재 위치에 1바이트 데이터 표시 
*/
void GLCD_data(unsigned char character);

/**
	clear GLCD screen 
*/
void GLCD_clear(void);


#endif	
//__GLCD_H__
