
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



#ifndef _VIDEOTRANSFORM_H_
#define _VIDEOTRANSFORM_H_



enum _transformSlders {
	SLIDER_ROTATE = 0,
	SLIDER_SCALE,
	SLIDER_BLUR,
	SLIDER_PIXELIZE,
	SLIDER_BRIGHTNESS,
	SLIDER_CONTRAST,
	SLIDER_SATURATION,
	SLIDER_GAMMA,
	SLIDER_TF_TOTAL
};

typedef struct{
	TSLIDER *slider;
	TFRAME *frame;
	int x;
	int y;
}TSTR;

typedef struct{
	TPAGE2COM *com;
	
	TSTR str[SLIDER_TF_TOTAL];
	TCCBUTTONS *btns;
}TTRANSFORM;



int page_tfCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);



#endif


