/**
	Key scan functions

	v2.

	key scanning with "pressed", "released" event.
*/
#ifndef	__KEY_H__
#define	__KEY_H__


#define		KEY_PRESSED		0x0000
#define		KEY_RELEASED	0x1000

//
// define key as char'.
//
#define		KEY_MODE	'0'
#define		KEY_START	'1'
#define		KEY_STOP	'2'
#define		KEY_UP		'3'
#define		KEY_DOWN	'4'


extern void key_init(void);

extern int key_getKey();

#define	IS_KEY_PRESSED(scanCode)	(((scanCode & 0xFF00)==0x0000)&&(scanCode!=0))


#define	KEY_CODE(scanCode)		((char)(scanCode & 0xFF))



#endif
//__KEY_H__
