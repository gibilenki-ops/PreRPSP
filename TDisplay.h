#ifndef	__TD_H__
#define	__TD_H__

#include "types.h"
#include "TFont.h"


//#define	BUFFERED_PUT	// for speed up.
//#define	USE_GRAPHIC_FT

/**
	- pixel,
	- line
	- box
	- buffer.
	- text
*/

void td_init();


void td_clear();

/**
	Set or clear pixel.
*/
void td_pixel(int x, int y, int color);



#ifdef	USE_GRAPHIC_FT

void td_line(int x0, int y0, int x2, int y2);

/**
	Draw line to given point.
*/
void td_lineTo(int x, int y);

void td_rectangle(int x0, int y0, int x2, int y2);

/**
	
*/
void td_fillRectangle(int x0, int y0, int x2, int y2);

void td_circle(int cx, int cy, int r);

void td_fillCircle(int cx, int cy, int r);
/**
	center and width, height ?
*/
void td_ellipse(int xx, int yy, int a0, int b0);

void td_fillEllipse(int xx, int yy, int a0, int b0);

#endif

/**
	Draw text without font.
*/
void td_text(int x0, int y0, char *string);



void td_setEFont(TFont *font);


void td_setHFont(TFont *font);

//-----------------------------------------------------------------------------
// Buffered display mode control.
//
/**
	@param	mode
		0	: do not update, until td_updateDisplay() ft is called.
		1	: update each pixel writing.
*/
void td_setUpdateMode(int mode);

/**
	Update changes in graphics to LCD display.
*/
void td_updateDisplay();

#endif	
//__TD_H__
