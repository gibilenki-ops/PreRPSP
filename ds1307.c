/**
	Device : DS1307
		I2C with 2 port.
		BCD timer.
	
	SCL : I2C_CLOCK
	SDA : I2C_DATA
*/

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "putil.h"
#include "ds1307.h"

// utility to convert BCD to int, int to BCD. (2 digits)
int bcdToint(uint8 bcd);
uint8 intTobcd(uint8 value);

#define nop()	asm("nop")


// TODO.
#define	SCL_SET()		_SET(PORTF, 5)
#define	SCL_CLR()		_CLR(PORTF, 5)

#define	SDA_SET()		_SET(PORTF, 4)
#define	SDA_CLR()		_CLR(PORTF, 4)

//#define	SCL_DIN()		(DDRD &= 	~0x01)			// not used.
#define	SCL_DOUT()		_SET(DDRF, 5)

#define	SDA_DIN()		_CLR(DDRF, 4)
#define	SDA_DOUT()		_SET(DDRF, 4)

#define	SDA_IS_HIGH()	((PINF & 0x10) == 0x10)


/**
	SCL interval(width) for 50K Hz.
	NOTE: SCL max 100 KHz.
*/


#define	DELAY()		_delay_us(10)


void ds1307_init(void)
{
	// Set SCL, SDA to output
	// Set SCL, SDA High
	SDA_SET();
	SCL_SET();

	SDA_DOUT();
	SCL_DOUT();
}


void i2c_start(void)
{
	SDA_SET();
	SCL_SET();
	SDA_DOUT();
	SCL_DOUT();
	
 	_delay_us(7);

	SDA_CLR();
 	_delay_us(7);
 	SCL_CLR();
}


void i2c_stop(void)
{
	SDA_DOUT();
	SCL_CLR();
 	_delay_us(7);

	SCL_SET();
 	_delay_us(4);
	SDA_SET();
 	SDA_DIN();
}


/**
	write bytes
	note: start seq is at i2c_start().
*/
void write_i2c_byte(unsigned char byte)
{
	uint8 i;

	SDA_SET();		// to enable pullup.
	SDA_DOUT();
	SCL_CLR();

	for (i = 0;  i < 8; i++)
	{
		// set data first
		if((byte & 0x80) == 0x80)	SDA_SET();
		else						SDA_CLR();
		//nops(1);
		DELAY();

		// clk.
		SCL_SET();
		byte = byte << 1;	// Shift data in buffer right one
	 	//nops(1);			// Small Delay
		DELAY();

	 	//if(i == 0)	nops(2);
	 	
	 	SCL_CLR();
	}

	SDA_SET();		// to enable pullup.
	SDA_DIN();

	DELAY();
	
	// wait ack.
 	SCL_SET();
	DELAY();
 	SCL_CLR();
	DELAY();
	//nops(45);			// Small Delay

	// wait ack.
	//if(i2c_ackn()==1)	ucNack=1;
}

/**
	Read ..
*/
unsigned char read_i2c_byte(unsigned char ch)
{
	uint8 i, buff = 0;

	SDA_SET();		// to enable internal pull-up : should be done..
	SDA_DIN();
	_delay_us(5);

	for(i = 0; i < 8; i++)
	{
		buff <<= 1;

		SCL_SET();
	 	//nops(3);		// Small Delay

		// Read data on SDA pin
		if (SDA_IS_HIGH()) {
			buff |= 0x01;
		}
		DELAY();
		
		SCL_CLR();
	 	//nops(3);		// Small Delay
		DELAY();
	}

	//nops(80);			// Long Delay

	// write ack.
	SDA_DOUT();

	if(ch == 0)	// Ack
	{
		SDA_CLR();	// ack..
		SCL_SET();
		DELAY();
		SCL_CLR();
		SDA_CLR();
	}
	else		// No Ack
	{
		SDA_SET();
		SCL_SET();
		DELAY();
		SCL_CLR();
		SDA_CLR();
	}
	
	return buff;
}

/**
	digit value.
*/
void ds1307_initial_config(uint8 sec, uint8 min, uint8 hour, uint8 day, uint8 date, uint8 month, uint8 year )
{
	// Set SCL, SDA to output
	// Set SCL, SDA High
	SDA_SET();
	SCL_SET();

	SDA_DOUT();
	SCL_DOUT();

	i2c_start();

	write_i2c_byte(0xD0);
	write_i2c_byte(0x00);		// Start
	write_i2c_byte(intTobcd(sec	));		// sec		0
	write_i2c_byte(intTobcd(min	));		// min		1
	write_i2c_byte(intTobcd(hour));		// hr
	write_i2c_byte(intTobcd(day	));		// day of week
	write_i2c_byte(intTobcd(date));		// dt
	write_i2c_byte(intTobcd(month));		// mon
	write_i2c_byte(intTobcd(year));		// yr
	write_i2c_byte(0x10);		// sqwe

	i2c_stop();
}

void ds1307_set_datetime(uint8 sec, uint8 min, uint8 hour, uint8 date, uint8 month, uint8 year )
{
	// Set SCL, SDA High
	SDA_SET();
	SCL_SET();

	SDA_DOUT();
	SCL_DOUT();

	i2c_start();

	write_i2c_byte(0xD0);
	write_i2c_byte(0x00);		// Start
	write_i2c_byte(intTobcd(sec	));		// sec		0
	write_i2c_byte(intTobcd(min	));		// min		1
	write_i2c_byte(intTobcd(hour));		// hr
	write_i2c_byte(0x10);		// sqwe

	i2c_stop();

	// Set SCL, SDA High
	SDA_SET();
	SCL_SET();

	SDA_DOUT();
	SCL_DOUT();

	i2c_start();

	write_i2c_byte(0xD0);
	write_i2c_byte(0x04);		// Start
	write_i2c_byte(intTobcd(date));		// dt
	write_i2c_byte(intTobcd(month));		// mon
	write_i2c_byte(intTobcd(year));		// yr
	write_i2c_byte(0x10);		// sqwe
	i2c_stop();
}

/**
	read clock
*/
static int ds1307_read(uint8 address)
{
	uint8 buffer;

	i2c_start();
	write_i2c_byte(0xD0);
	write_i2c_byte(address);
	i2c_stop();
	
	i2c_start();
	write_i2c_byte(0xD1);				// Data Read - slave transmiter mode
	DELAY();

	buffer = read_i2c_byte(0);			// with ack.	
	i2c_stop();
	
	return buffer;
}

/**
	return decimal hour and min.
*/
int ds1307_read_time2(uint8 *pHour, uint8 *pMin)
{
	//
	*pHour	= bcdToint(ds1307_read(2));		//
	*pMin	= bcdToint(ds1307_read(1));		//

	return 0;
}

/**
	return decimal hour and min.

	sec				0
	min				1
	hour			2
	day of week	`	3
	day				4
	month			5
	year			6
*/
int ds1307_read_date(DS1307_date *pDate)
{
	pDate->year = bcdToint(ds1307_read(6));
	pDate->month = bcdToint(ds1307_read(5));
	pDate->day = bcdToint(ds1307_read(4));

	return 0;
}

int ds1307_read_time(DS1307_time *pTime)
{
	pTime->hour   = bcdToint(ds1307_read(2));
	pTime->minute = bcdToint(ds1307_read(1));
	pTime->second = bcdToint(ds1307_read(0));

	return 0;
}

int bcdToint(uint8 bcd)
{
	return (bcd >> 4) * 10 + (bcd & 0x0F);
}

/**
	return BCD value.
*/
uint8 intTobcd(uint8 value)
{
	return ((value / 10) << 4) + (value % 10);
}
