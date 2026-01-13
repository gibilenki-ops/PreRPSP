#ifndef	__STRCB_OPTIONS_H__
#define	__STRCB_OPTIONS_H__

/**
	STRCB Project build options

	- all files must include this option file.
*/

// debug at SDCard port
//#define	CONSOLE_DEBUG

// since 2016/06/07 (REV. 3.0 board)
// -> only applies to DOOR sensor.  (at STR_**.c)
#define	HW_REV03


#ifdef	CONSOLE_DEBUG

#define	DPRINTF(args...)		printf(args)

#else
#define	DPRINTF(args...)	
#endif


#endif
