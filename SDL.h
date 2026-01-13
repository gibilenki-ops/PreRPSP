/**
	SDLog remote(serial) interface

	@project	201406-FWL
	@author	tchan@TSoft
	@date	2015/11/25
	
	ver 1.0	first release
*/
#ifndef	__SDL_H__
#define	__SDL_H__

/**
	Create log file.
*/
int sdl_init();

/**
	state
	'R'	: Ready
	'D'	: Disconnected
*/
char sdl_getState();

/**
	Create file (or open file if exising)

	note) filename should not include ','
		only 8.3 format for name
		max 32 char.
	ex)
		"L20152134-123424.txt"
*/
int sdl_create(char *fileName);

/**
	Close file, if open.

	note) don't care whether open or not.
*/
int sdl_close();


/**
	write a line of log

	note) no end of line char needed.
*/
int sdl_writeLine(char *str);

int sdl_write(char *str);


/**
	Parse SDLogger response/events.

	SE:<st><CR><LF>
		<st> = 'R', 'D'.
	
	note) call this at 10 ms interval
*/
void sdl_process();

#endif
