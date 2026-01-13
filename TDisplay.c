#include "TDisplay.h"

/*
	Graphic device for black/white.
	An abstract layer. 
	- buffered graphics device.

	High level API for 128x32 graphic lcd device driver.

	- drawing library.
	- x is dot, y is 8dot coord.
	

	@author	tchan@TSoft
	@date 2015/07/16

	07/18 : graphic function TODO use buffered put.

	03/14 ; added BUFFERED_ 방식 : pixel update를 모아서 한꺼번에 처리하도록 수정.(속도향상)
	03/21 ; line, rect, circle, ellipse added.
*/
#include "glcd.h"
#include "TFont.h"


static TFont *eFont = 0;
static TFont *hFont = 0;


#define	LCD_COLS		128			// in dots
#define	LCD_ROWS		32			// in dots

#define	NUM_PAGE		4			// 32/8

// graphic buffer. (128 x 32 bits)
uint8 td_buffer[NUM_PAGE][LCD_COLS];

uint8 td_updateFlag[LCD_COLS];		// 128 column in, bit0 is y=0, bit7 is y=7.

uint8 td_updateMode = 1;			// 0 : don't update now, 1 : update now


/**
	Display TFont char.
*/
static void td_displayFont(int x, int y, TFont *pFont, unsigned char *pFontData);


void td_init()
{
	GLCD_init();
	//
	GLCD_clear();

	int i;
	for (i=0; i<LCD_COLS; i++)
	{
		td_updateFlag[i] = 0;
	}
}

/**
	To aggregate pixel update in one shot.

	mode
	0	: do not update directly.
	1	: do update directly.
*/
void td_setUpdateMode(int mode)
{
	td_updateMode = mode;
}

/**
	Find updated data and refresh to display.
*/
void td_updateDisplay()
{
	int py, x;
	int continueUpdate = 0;			// is it continuous bytes updated ?
	for (py=0; py<NUM_PAGE; py++)
	{
		int b = 1 << py;

		for (x=0; x<LCD_COLS; x++)
		{
			if (td_updateFlag[x] & b)		// if py page needs update
			{
				// updated.
				if (!continueUpdate)
				{
					GLCD_move(x, py);
					continueUpdate = 1;
				}

				GLCD_put(td_buffer[py][x]);
			}
			else
			{
				continueUpdate = 0;
			}
		}
		continueUpdate = 0;
	}

	for (x=0; x<LCD_COLS; x++)
	{
		td_updateFlag[x] = 0;
	}
}

/**
	Set or clear pixel.
	using buffer.
*/
void td_pixel(int x, int y, int color)
{
	int py = y >> 3;

	if ((x < 0) || (x >= LCD_COLS)) return;
	if ((y < 0) || (y >= LCD_ROWS)) return;

	if (color)
	{
		td_buffer[py][x] |= 1 << (y & 0x07);
	}
	else
	{
		td_buffer[py][x] &= ~(1 << (y & 0x07));
	}

	if (td_updateMode)
	{
		GLCD_move(x, py);
		GLCD_put(td_buffer[py][x]);
	}
	else
	{
		td_updateFlag[x] |= 1<<py;
	}
}


/**
	note) 07/18 not used yet.

	Set or clear pixel.
	write vertical 8 bits data to graphics memory.

	if y = 1, 7 bits are in py = 0, 1 bits are in py = 1.
*/
void td_put8(int x, int y, uint8 data)
{
	int py = y >> 3;
	int bits = y & 0x07;

	//-- lower bits in first.

	// clear	0xFF << bits;
	// fill		data << bits;
	td_buffer[py][x] &= ~(0xFF << bits);
	td_buffer[py][x] |= data << bits;

	GLCD_move(x, py);
	GLCD_put(td_buffer[py][x]);

	// fill higher bits in...
	if (bits && (py < 7))
	{
		td_buffer[py+1][x] &= ~(0xFF >> bits);
		td_buffer[py+1][x] |= data >> bits;

		GLCD_move(x, py+1);
		GLCD_put(td_buffer[py+1][x]);
	}	
}

#ifdef	USE_GRAPHIC_FT
/*
	Bresenham’s algorithm for |m|<1.0 

	void lineBres( int x0, int y0, int xEnd, int yEnd )
*/
void td_line(int x0, int y0, int x2, int y2)
{
	// internal variables : x, y, xEnd, yEnd.
	int x, y, xEnd, yEnd;

	int dx = ((x2 - x0)>=0)?(x2 - x0):(x0 - x2);	// abs.
	int dy = ((y2 - y0)>=0)?(y2 - y0):(y0 - y2);

	int p = 2 * dy - dx;
	int twoDy = 2 * dy;
	int twoDyMinusDx = 2 * (dy - dx);

	int twoDx = 2 * dx;
	int twoDyMinusDy = 2 * (dx - dy);

	
	uint8 old_updateMode = td_updateMode;
	td_setUpdateMode(0);	// do not update now.

	/* Determine which endpoint to use as start position */
	if (dx >= dy)
	{
		if( x0 > x2 ) 
		{
			x = x2;
			xEnd = x0;

			y = y0;		// y2.
			yEnd = y2;
		}
		else 
		{
			x = x0;
			y = y0;
			xEnd = x2;
			yEnd = y2;
		}

		td_pixel( x, y, 1);

		while( x < xEnd ) 
		{
			x++;
			if( p < 0 )
			{
				p += twoDy;
			}
			else 
			{
				y++;
				p += twoDyMinusDx;
			}
			td_pixel( x, y, 1 );
		}
	} 
	else
	{
		if( y0 > y2 ) 
		{
			y = y2;
			yEnd = y;

			x = x0;
			xEnd = x2;
		}
		else 
		{
			x = x0;
			y = y0;
			xEnd = x2;
			yEnd = y2;
		}

		td_pixel( x, y, 1);

		while( y < yEnd ) 
		{
			y++;
			if( p < 0 )
			{
				p += twoDx;
			}
			else 
			{
				x++;
				p += twoDyMinusDy;
			}
			td_pixel( x, y, 1 );
		}		
	}

	if (old_updateMode)	td_updateDisplay();
	td_setUpdateMode(old_updateMode);
}

void td_lineTo(int x0, int y0)
{
	//
}

void td_rectangle(int x0, int y0, int x2, int y2)
{
	int i; 

	uint8 old_updateMode = td_updateMode;
	td_setUpdateMode(0);	// do not update now.

	for (i=x0; i<=x2; i++)
	{
		td_pixel(i, y0, 1);
		td_pixel(i, y2, 1);
	}

	for (i=y0; i<=y2; i++)
	{
		td_pixel(x0, i, 1);
		td_pixel(x2, i, 1);
	}

	if (old_updateMode)	td_updateDisplay();
	td_setUpdateMode(old_updateMode);
}

void td_fillRectangle(int x0, int y0, int x2, int y2)
{
	int x, y; 

	uint8 old_updateMode = td_updateMode;
	td_setUpdateMode(0);	// do not update now.

	for (x=x0; x<=x2; x++)
	{
		for (y=y0; y<=y2; y++)
		{
			td_pixel(x, y, 1);
		}
	}

	if (old_updateMode)	td_updateDisplay();
	td_setUpdateMode(old_updateMode);
}

void td_circle(int cx, int cy, int r)
{
	int xx = 0;
	int yy = r;

	uint8 old_updateMode = td_updateMode;
	td_setUpdateMode(0);	// do not update now.

	int d = 1-r;

	td_pixel(cx+xx, cy+yy, 1);     
	td_pixel(cx+xx, cy-yy, 1);     
	td_pixel(cx-yy, cy+xx, 1);     
	td_pixel(cx+yy, cy+xx, 1);          
	while (xx < yy) 
	{      
		xx = xx + 1;       
		if(d < 0)
		{
			d += 2*xx + 1;
		}
		else 
		{        
			yy = yy - 1;         
			d += 2*(xx - yy) + 1;  
		}
		           
		td_pixel(cx+xx, cy+yy, 1);    
		td_pixel(cx+xx, cy-yy, 1);  
		td_pixel(cx-xx, cy+yy, 1);   
		td_pixel(cx-xx, cy-yy, 1);    
		td_pixel(cx+yy, cy+xx, 1);
		td_pixel(cx+yy, cy-xx, 1);  
		td_pixel(cx-yy, cy+xx, 1);    
		td_pixel(cx-yy, cy-xx, 1);    
	}

	if (old_updateMode)	td_updateDisplay();
	td_setUpdateMode(old_updateMode);
} 

void td_fillCircle(int cx, int cy, int r)
{
	int xx = 0;
	int yy = r;

	int d = 1-r;
	int i;

	uint8 old_updateMode = td_updateMode;
	td_setUpdateMode(0);	// do not update now.

	for (i=-yy; i<=yy; i++) td_pixel(cx+xx, cy+i, 1);
	for (i=-yy; i<=yy; i++) td_pixel(cx+i, cy+xx, 1);

	while (xx < yy) 
	{      
		xx = xx + 1;       
		if(d < 0)
		{
			d += 2*xx + 1;
		}
		else 
		{        
			yy = yy - 1;         
			d += 2*(xx - yy) + 1;  
		}
          
		for (i=-yy; i<=yy; i++) td_pixel(cx+xx, cy+i, 1);
		for (i=-yy; i<=yy; i++) td_pixel(cx-xx, cy+i, 1);
		for (i=-xx; i<=xx; i++) td_pixel(cx+yy, cy+i, 1);
		for (i=-xx; i<=xx; i++) td_pixel(cx-yy, cy+i, 1);
	}

	if (old_updateMode)	td_updateDisplay();
	td_setUpdateMode(old_updateMode);
} 

/**
	center and width, height ?
*/
void td_ellipse(int xx, int yy, int a0, int b0)
{
    int x=0;
    int y=b0;
 
 	// note) these should be long.
    long a=a0;
    long b=b0;
    long a2 = a*a;
    long a22 = 2*a2;
    long b2 = b*b;
    long b22 = 2*b2;
 
    long d;
    long dx,dy;
 
    d=b2 - a2 * b + a2 / 4L;
    dx = 0L;
    dy = a22*b;
 
	uint8 old_updateMode = td_updateMode;
	td_setUpdateMode(0);	// do not update now.

    while(dx < dy)
    {
        //symmetry4(xx, yy, x, y);
 	    td_pixel(xx-x, yy+y, 1);
	    td_pixel(xx+x, yy-y, 1);
	    td_pixel(xx-x, yy-y, 1);
	    td_pixel(xx+x, yy+y, 1);

        if(d>0){
 
            --y;
            dy -= a22;
            d -= dy;
        }
 
        ++x;
        dx += b22;
        d += b2+dx;
    }
 
    d += (3*(a2 - b2) / 2L - (dx + dy)) / 2L;
 
    while( y>=0) 
	{
        //symmetry4(xx,yy,x,y);
	    td_pixel(xx-x, yy+y, 1);
	    td_pixel(xx+x, yy-y, 1);
	    td_pixel(xx-x, yy-y, 1);
	    td_pixel(xx+x, yy+y, 1);

        if(d<0){
            ++x;
            dx += b22;
            d += dx;
        }
 
        --y;
        dy -= a22;
        d += a2 -dy;
 
    }

	if (old_updateMode)	td_updateDisplay();
	td_setUpdateMode(old_updateMode);
}
 
 

/**
	center and width, height ?
*/
void td_fillEllipse(int xx, int yy, int a0, int b0)
{
    int x=0;
    int y=b0;

 	// note) these should be long.
    long a=a0;
    long b=b0;
    long a2 = a*a;
    long a22 = 2*a2;
    long b2 = b*b;
    long b22 = 2*b2;
 
    long d;
    long dx,dy;

	int i;
 
    d=b2 - a2 * b + a2 / 4L;
    dx = 0L;
    dy = a22*b;
 
 	uint8 old_updateMode = td_updateMode;
	td_setUpdateMode(0);	// do not update now.

    while(dx < dy)
    {
		for (i=-y; i<=y; i++)
		{
			td_pixel(xx-x, yy+i, 1);
			td_pixel(xx+x, yy+i, 1);
		}
		
        if(d>0){
 
            --y;
            dy -= a22;
            d -= dy;
        }
 
        ++x;
        dx += b22;
        d += b2+dx;
    }
 
    d += (3*(a2 - b2) / 2L - (dx + dy)) / 2L;
 
    while( y>=0) 
	{
		for (i=-y; i<=y; i++)
		{
			td_pixel(xx-x, yy+i, 1);
			td_pixel(xx+x, yy+i, 1);
		}

 
        if(d<0){
            ++x;
            dx += b22;
            d += dx;
        }
 
        --y;
        dy -= a22;
        d += a2 -dy;
 
    }

	if (old_updateMode)	td_updateDisplay();
	td_setUpdateMode(old_updateMode);
}
#endif	// USE_GRAHIC_FT

 
void td_setEFont(TFont *font)
{
	eFont = font;
}

void td_setHFont(TFont *font)
{
	hFont = font;
}

/**
	Draw text with preset font.
*/
void td_text(int x0, int y0, char *string)
{
	char *p = string;
	uint8 buff[32];

	uint8 old_updateMode = td_updateMode;
	td_setUpdateMode(0);	// do not update now.

	while (*p)
	{
		// display english.
		if (eFont)
		{
			//Font_getEnglish(*p, buff);
			eFont->getFontData(*p, buff);
			td_displayFont(x0, y0, eFont, buff);
			//gd_displayEnglish16(x0, y0, buff);
		}
		x0 += eFont->getWidth();

		p++;
	}

	if (old_updateMode)	td_updateDisplay();
	td_setUpdateMode(old_updateMode);
}

static void td_displayFont(int x, int y, TFont *pFont, unsigned char *pFontData)
{
	int i, j;

#if 1
	// general font
	//fontData.width, height; 또는 font에서 width/height.
	for (i=0; i<pFont->getWidth(); i++)
	{
		for (j=0; j<pFont->getHeight(); j++)
		{
			td_pixel(x+i, y+j, pFont->getPixel(pFontData, i, j));
		}
	}

#else
	// specially 8 height font only.
	if (y & 0x07)
	{
		//fontData.width, height; 또는 font에서 width/height.
		for (i=0; i<pFont->getWidth(); i++)
		{
			for (j=0; j<pFont->getHeight(); j++)
			{
				td_pixel(x+i, y+j, pFont->getPixel(pFontData, i, j));
			}
		}
	}
	else
	{
		j = y >> 3;
		for (i=0; i<pFont->getWidth(); i++)
		{
			// only when pFont height == 8.
			td_buffer[j][x+i] = pFontData[i];

			td_updateFlag[x+i] |= 1<<j;
		}
	}
#endif
}
