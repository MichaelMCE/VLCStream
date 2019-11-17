
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


#ifndef _VIDEOFILTER_H_
#define _VIDEOFILTER_H_




enum _filters
{
	VIDEO_FILTER_WAVE,
	VIDEO_FILTER_WALL,
	VIDEO_FILTER_TRANSFORM,
	VIDEO_FILTER_SWSCALE,
	VIDEO_FILTER_SHARPEN,
	VIDEO_FILTER_SEPIA,
	VIDEO_FILTER_ROTATE,
	VIDEO_FILTER_RIPPLE,
	VIDEO_FILTER_PUZZLE,
	VIDEO_FILTER_PSYCHEDELIC,
	VIDEO_FILTER_POSTPROC,
	VIDEO_FILTER_POSTERIZE,
	VIDEO_FILTER_PANORAMIX,
	VIDEO_FILTER_MOTIONDETECT,
	VIDEO_FILTER_MOTIONBLUR,
	VIDEO_FILTER_MIRROR,
	VIDEO_FILTER_MAGNIFY,
	VIDEO_FILTER_INVERT,
	VIDEO_FILTER_HQDN3D,
	VIDEO_FILTER_GRAIN,
	VIDEO_FILTER_GRADIENT,
	VIDEO_FILTER_GRADFUN,
	VIDEO_FILTER_GAUSSIANBLUR,
	VIDEO_FILTER_EXTRACT,
	VIDEO_FILTER_ERASE,
	VIDEO_FILTER_DEINTERLACE,
	VIDEO_FILTER_CROPPADD,
	VIDEO_FILTER_COLORTHRES,
	VIDEO_FILTER_CLONE,
	VIDEO_FILTER_CANVAS,
	VIDEO_FILTER_BLUESCREEN,
	VIDEO_FILTER_BLENDBENCH,
	VIDEO_FILTER_BALL,
	VIDEO_FILTER_ATMO,
	VIDEO_FILTER_ANTIFLICKER,
	VIDEO_FILTER_ALPHAMASK,
	VIDEO_FILTER_ADJUST,
	VIDEO_FILTER_TOTAL
};



void videoFilter_add (TVLCCONFIG *vlc, const unsigned int filter);
void videoFilter_remove (TVLCCONFIG *vlc, const unsigned int filter);



#endif

