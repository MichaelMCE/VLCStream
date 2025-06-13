
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



#ifndef _PAGE_VIDEO_H_
#define _PAGE_VIDEO_H_


typedef struct{
	TPAGE2COM *com;

}TPAGEVIDEO;


int page_videoCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

void video_copySourceFrame (TVLCPLAYER *vp);	// copy latest video frame supplied by VLC
void video_composeFrame (TVLCPLAYER *vp, TFRAME *frame);
void video_saveSnapshot (TVLCPLAYER *vp, TFRAME *frame, wchar_t *filename, const int annouce);
void video_drawFPSOverlay (TVLCPLAYER *vp, TFRAME *frame, const float fps, const int x, int y);
void video_copyToDesktop (TVLCPLAYER *vp, TFRAME *frame, const int cx, const int cy);




#endif


