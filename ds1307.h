#ifndef	__DS1307_I2C_H__
#define	__DS1307_I2C_H__

#include "types.h"


typedef struct 
{
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
}DS1307_time;
	
typedef struct 
{
	unsigned char year;
	unsigned char month;
	unsigned char day; 
	unsigned char dow; 			// day of week
}DS1307_date;	

/*
void i2c_start(void);

void i2c_stop(void);

void write_i2c_byte(unsigned char byte);

unsigned char read_i2c_byte(unsigned char ch);

int ds1307_read(uint8 address);
*/

//void ds1307_initial_config(void);
/**
	
	dow		day of week
*/
void ds1307_initial_config(uint8 sec, uint8 min, uint8 hour, uint8 dow, uint8 date, uint8 month, uint8 year );

void ds1307_init(void);


void ds1307_set_datetime(uint8 sec, uint8 min, uint8 hour, uint8 date, uint8 month, uint8 year );

int ds1307_read_date(DS1307_date *pDate);

int ds1307_read_time(DS1307_time *pTime);

/**
	return decimal hour and min.
*/
int ds1307_read_time2(uint8 *pHour, uint8 *pMin);

#endif	//__DS1307_I2C_H__
