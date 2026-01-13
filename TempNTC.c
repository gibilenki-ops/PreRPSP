/**
	NTC 온도 센서

	if t < -50, 단선.
	if t > 200, 합선.

*/
#include <avr/io.h>
#include <math.h>

#include "TempNTC.h"

/***********************************************************************
		Temp sensor
************************************************************************/

float C3 = -2.222E-08;
float C2 = 2.8458E-04;
float C1 = 2.69726E-03;


static float ntc_toTemperature(float adc)
{
	if (adc < 0.1) adc = 0.1;

//	float r = 5000 * (1023 / adc - 1);

	// sensor at base.
	float adcV = (adc / 1023.) * 5;
	if (adcV > 4.9) adcV = 4.9;
	float r = (5000 * adcV) / (5 - adcV);


	// get temperature :
	float tR = r / 1000.;

	float t = 1 / (C1 + C2 * log(tR) + C3 * pow(log(tR), 3));

	return t - 273.15;
}


static int ntc_getAdcValue()
{
	//int current_sensor_value;

	ADMUX= 0b00000011;	//ADC3 CHANNEL
	SREG =0x80;
    ADCSRA=0b11010111;                 // ADEN=1, ADSC = 1 변환 시작 ,ADPS = 1/1/1 분주비 128
    while((ADCSRA & 0b00010000) == 0); // ADIF=1이 될 때까지 대기
	//current_sensor_value = ADCL;	//10비트 5V/1024
	//current_sensor_value += ADCH<<8;

	return ADCW;
}


/**
	현재 측정온도를 읽음.
*/
int ntc_getTemperature()
{
	int adc = ntc_getAdcValue();

	return (int)ntc_toTemperature(adc);
}

