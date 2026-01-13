/**
	STRCB
	
	Key scan functions

	@author	tchan
	@date	2015/08/26
*/

#include <avr/io.h>
#include <util/delay.h>
#include "putil.h"
#include "key2.h"

/**

*/
void key_init()
{
	// PC0 : STOP
	// PC1 : START
	// PC3,4,5 : MODE, UP, DOWN
}

static char scanKeyA()
{
	unsigned char temp;	
	unsigned char keyCode = 0;

	asm("nop");

	//temp = (PINC & 0x07) | (PINB & 0x18);
	temp = PINC;
	
	if ((temp & 0x08) == 0)		keyCode = KEY_MODE;
	else if((temp & 0x10) == 0)	keyCode = KEY_UP;
	else if((temp & 0x20) == 0)	keyCode = KEY_DOWN;
	else if((temp & 0x02) == 0)	keyCode = KEY_STOP;
	else if((temp & 0x01) == 0)	keyCode = KEY_START;

	return keyCode;
}

/**
	Check key and get current key.

	- key is checked when 10ms continuous situation.

	@note) Assume to call at 1ms interval.
*/
int key_getKey()
{
	static char oldKeyA = 0;
	static char keyA = 0;
	static char repKey = 0;
	static char repCount = 0;

	int tempKey = 0;

	keyA = scanKeyA();

	if (repKey == keyA)
	{
		// same key.
		if (repCount < 10)
		{
			repCount++;
		}
		else
		{
			// same key state while 10 ms.
			if (keyA == 0)
			{
				if (oldKeyA != 0)
				{
					tempKey = (oldKeyA | KEY_RELEASED);
					oldKeyA = keyA;

					return tempKey;	
				}
			}
			else if (keyA != 0)
			{
				if (oldKeyA == 0)
				{
					oldKeyA = keyA;

					return (keyA  | KEY_PRESSED);
				}
			}
		}
	}
	else
	{
		repKey = keyA;
		repCount = 0;
	}

	return 0;
}



