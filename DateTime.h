/**
	Date/Time utility.
*/
#ifndef	__DATE_TIME_H__
#define	__DATE_TIME_H__

typedef struct 
{
	unsigned char hour;			// 0..23
	unsigned char minute;		// 0..59
	unsigned char second;		// 0..59
} TTime;

typedef struct 
{
	unsigned char year;			// 2000 + alpha.
	unsigned char month;		// 1..12
	unsigned char date; 		// 1..31
} TDate;


#endif
