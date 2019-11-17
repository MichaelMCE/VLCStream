
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



#ifndef _IMGOVR_H_
#define _IMGOVR_H_

#define IMGOVR_ROTATE_NONE		0
#define IMGOVR_ROTATE_LEFT		1
#define IMGOVR_ROTATE_RIGHT		2
#define IMGOVR_ROTATE_180		3



typedef struct{
	TPAGE2COM *com;
	
	int enabled;
	int x;				// destination render location pt x
	int y;				// destination render location pt y
	int width;			// max width (n < display width)
	int height;			// max height (n < display height)
	PLAYLISTCACHE *plc;
	TSLIDER *slider;
	
	int dwidth;
	int dheight;		// width/height of currently displayed image
	int currentImg;		// index in to :plc of the displayed image
	int rotateImg;
	TFRAME *img;			// scaled (may or may not include a border)
	TFRAME *protectedImg;	// final image for rendering, clone of :img but protected via locks
	
	uintptr_t hImgOvrLoadThread;
	TMLOCK *hLoadLock;
	HANDLE hLoadEvent;
	unsigned int loadLockThreadID;
	TMLOCK *hNewImgLock;
	int threadState;

	int nextWidth;
	int nextHeight;
	char *nextPath8;
	int nextIsReady;
	
	TCCBUTTONS *btns;
}TIOVR;


int iOvrBuildImageList (TIOVR *iovr, TVLCPLAYER *vp, wchar_t *mrl);
void imageOverSetImage (TIOVR *iovr, const int dwidth, const int dheight, char *filename);


int page_imgOvrCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);



#endif


