#include "glcd.h"

/*
	11/12. status busy testing => not good.

	for 64x128.
	with two 64x64 controller. (By CS1, CS2)

	- status read를 사용하면 10% faster.
	- 그러나, 선이 하나 더 연결됨.(R/W)


	"signal" is used to seledt CS1, CS2. (CS1=0x40, CS2=0x80).
*/

#include <avr/io.h>
#include <util/delay.h>
#include "putil.h"


//---- A0
#if 1
#define	LCD_RS_COMMAND()	_CLR(PORTB, 4)	
#define	LCD_RS_DATA()		_SET(PORTB, 4)
#else
// A0 -> PC5.
#define	LCD_RS_COMMAND()	_CLR(PORTC, 5)
#define	LCD_RS_DATA()		_SET(PORTC, 5)
#endif
// CS1B - chip select.
#define	LCD_ENABLE()		_CLR(PORTB, 0)
#define	LCD_DISABLE()		_SET(PORTB, 0)

unsigned char GLCD_getStatus(unsigned char signal);

void GLCD_init()
{
//	unsigned char X_Addr, Y_Addr;
	PORTB |= 0b00000101;	/* Configure SCK/MOSI/CS as output */
	DDRB  |= 0b00000111;

	DDRB |= 0x10;			// PB4,3 A0, RST
	DDRC |= 0x04;			// PC2:RST

	SPCR = 0x52;			/* Enable SPI function in mode 0 */
	SPSR = 0x01;			/* SPI 2x mode */

	_delay_ms(100);

	// LCD normal (not reset)
	PORTC |= 0x04;			// PC2:RST


	//-- init lcd.
  	GLCD_command(0xAE);			// display ON

	GLCD_command(0xA3);			// bias set. 1/9
	GLCD_command(0xA0);			// ADC select : normal
	GLCD_command(0xC8);			// Common output scan direction : normal
		// UP side-down


	// V0 voltage...;; 전체적인 진하기  
	// 0x24 : 5.0 V - 까맣게 나옴.
	// 0x21 : 3.5 V - 적당히..
	GLCD_command(0x21);			// 중간..

	// electronic volumne (contrast ?)
	GLCD_command(0x81);
	//GLCD_command(0x20);			// median..
	GLCD_command(0x18);			// test 

	// power control setting.
	GLCD_command(0x2F);			// internal voltage... used.
  	GLCD_command(0xAF);			// display ON


  	GLCD_command(0x40);			// display position Set 0

	// set page addr.
  	GLCD_command(0xB0);			// page 0.

	// set column addr 	.
  	GLCD_command(0x10);			// column 0...
	GLCD_command(0x00);


//	GLCD_command(0xA5);			// show all points
}

// LCD_D		// data or instruction

/**
	User level coordination.

	x : 0..127	horizontal.
	y : 0..3	vertical

	note) address is swapped compared to WG12864A-YYH-V LCD driver.
*/
void GLCD_move(uint8 x, uint8 y)
{
	if (y < 4)
	{
		// set page addr.
	  	GLCD_command(0xB0 + y);			// page 0.
	}


	// set column addr 	.
  	GLCD_command(0x10 + (x>>4));			// column 0...
	GLCD_command(0x00 + (x & 0x0F));
}

/**
	Put vertical 8 bits point data, at current position.
	current position is set by gdev_move().
*/
void GLCD_put(uint8 data)
{
	// by current position, should set CS1 or CS2.
	GLCD_data(data);

#if 0
	// auto-increment mode.
	// if LCD_CS == LCD_CS1 and x exceeds 63, lcd_cs should change to CS2.
	lcd_x++;
	if ((lcd_cs == LCD_CS1) && (lcd_x > 63)) GLCD_move(lcd_x, lcd_y);
#endif
}


/**
	signal	LCD_CS1, LCD_CS2
*/
void GLCD_command(unsigned char command)       /* GLCD에 1개의 제어명령을 WRITE */
{ 
	LCD_RS_COMMAND();
	LCD_ENABLE();

	SPDR = command;
	loop_until_bit_is_set(SPSR, SPIF);

	LCD_DISABLE();
}

/*
	GLCD의 현재 위치에 1바이트 데이터 표시 
*/
void GLCD_data(unsigned char data)
{ 
	LCD_RS_DATA();
	LCD_ENABLE();

	SPDR = data;
	loop_until_bit_is_set(SPSR, SPIF);

	LCD_DISABLE();
}


/**
	Get status.
*/
unsigned char GLCD_getStatus(unsigned char signal)
{ 
	_delay_us(10);
	return 0;
}

/**
	TODO: make port... portable.
*/

void GLCD_clear(void)				/* clear GLCD screen */
{

	unsigned char i;

	GLCD_move(0, 0);
	for (i=0; i<128; i++) GLCD_data(0x00);

	GLCD_move(0, 1);
	for (i=0; i<128; i++) GLCD_data(0x00);

	GLCD_move(0, 2);
	for (i=0; i<128; i++) GLCD_data(0x00);

	GLCD_move(0, 3);
	for (i=0; i<128; i++) GLCD_data(0x00);
}
