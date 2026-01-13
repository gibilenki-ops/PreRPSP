#ifndef	__TFONT_H__
#define	__TFONT_H__

#include "types.h"

/**
	TFont,

	a simple dot-font. Specially useful for small graphic system.
	
	has 
	- size (width, height) in pixel
	- font data size (in bytes)

	note) graphic device does not know data structure of the char/font.
*/
typedef	struct _TFont {
	int (*getWidth)();
	int (*getHeight)();
	/**
		Font Data size in bytes
	*/
	int (*getDataSize)();

	void (*getFontData)(unsigned int code, unsigned char *fontData);

	int (*getPixel)(unsigned char *fontData, int i, int j);
} TFont;

#endif
//__TFONT_H__
