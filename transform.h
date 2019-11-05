
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



#ifndef _ROTATE_H_
#define _ROTATE_H_

#define SCALE_BILINEAR		1
#define SCALE_BICUBIC		2
#define SCALE_NEIGHBOUR		3
#define SCALE_CLEANDES		(0x0100)

#define ROTATE_BILINEAR		1
#define ROTATE_BICUBIC		2
#define ROTATE_NEIGHBOUR	3


int transRotate (TFRAME *src, TFRAME *des, const int angle, const int type);
int transScale (TFRAME *src, TFRAME *des, const int width, const int height, const int desX, const int desY, const int type);

void transBrightness (TFRAME *src, const int brightness);
void transContrast (TFRAME *src, const int contrast);
void transPixelize (TFRAME *src, const int size);
void transSharpen (TFRAME *src, const float value);
void transGrayscale (TFRAME *src);
void transBlur (TFRAME *src, const int radius);

#endif

