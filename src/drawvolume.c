
// libmylcd - http://mylcd.sourceforge.net/
// An LCD framebuffer library
// Michael McElligott
// okio@users.sourceforge.net

//  Copyright (c) 2005-2009  Michael McElligott
// 
//  This program is free software; you can redistribute it and/or

//  modify it under the terms of the GNU LIBRARY GENERAL PUBLIC LICENSE
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU LIBRARY GENERAL PUBLIC LICENSE for more details.
//
//	You should have received a copy of the GNU Library General Public
//	License along with this library; if not, write to the Free
//	Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "common.h"



static inline int getPixel32_NBr (const TFRAME *frm, const int x, const int row)
{
	return *(uint32_t*)(frm->pixels+(row+(x<<2)));
}

static inline void setPixel32_NBr (const TFRAME *frm, const int x, const int row, const int value)
{
	*(uint32_t*)(frm->pixels+(row+(x<<2))) = value;
} 

static inline unsigned int ablend (const unsigned int des, const unsigned int src)
{
	const unsigned int alpha = (src&0xFF000000)>>24;
	const unsigned int odds2 = (src>>8) & 0xFF00FF;
	const unsigned int odds1 = (des>>8) & 0xFF00FF;
	const unsigned int evens1 = des & 0xFF00FF;
	const unsigned int evens2 = src & 0xFF00FF;
	const unsigned int evenRes = ((((evens2-evens1)*alpha)>>8) + evens1)& 0xFF00FF;
	const unsigned int oddRes = ((odds2-odds1)*alpha + (odds1<<8)) &0xFF00FF00;
	return (evenRes + oddRes);
}

void drawVolume (const TFRAME *src, TFRAME *des, const int Xoffset, const int Yoffset, const int srcx1, const int srcy1, const int srcWidth, const int srcHeight)
{
	for (int y = srcy1; y < srcHeight; y++){
		int rowdes = (y+Yoffset)*des->pitch;
		int rowsrc = y*src->pitch;
					
		for (int x = srcx1; x < srcWidth; x++){
			unsigned int spix = getPixel32_NBr(src, x, rowsrc);
			if (spix&0xFF000000){
				unsigned int dpix = getPixel32_NBr(des, x+Xoffset, rowdes);
				unsigned int blend = ablend(dpix, spix);
				setPixel32_NBr(des, x+Xoffset, rowdes, blend);
			}
		}
	}
}
