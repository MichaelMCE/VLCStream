
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



#ifndef _ANTPLUS_H_
#define _ANTPLUS_H_


enum _imglist {
	IMG_NUM0,
	IMG_NUM1,
	IMG_NUM2,
	IMG_NUM3,
	IMG_NUM4,
	IMG_NUM5,
	IMG_NUM6,
	IMG_NUM7,
	IMG_NUM8,
	IMG_NUM9,
	IMG_SIGNAL,
	IMG_TOTAL
};

typedef struct{
	TFRAME *list[IMG_TOTAL];
}TIMAGES;

#include "antplus/garminhr.h"
#include "antplus/anthrm.h"


#define ANTPANEL_SET_FIND		1
#define ANTPANEL_SET_START		2
#define ANTPANEL_SET_STOP		3

#define SHEET_CURRENT		0				// current hr and 800 seconds of history (moving right (latest) to left (oldest)). fixed scale
#define SHEET_HISTORY		1				// complete HR history, autoscaled
#define SHEET_MODE			2				// complete mode histroy, autoscaled
#define SHEET_TOTAL			3


typedef struct{
	TPAGE2COM *com;
	
	THR *hr;
	TPANE *pane;		// container for control icons
	TIMAGES images;
	TGRAPH *graph;

	int graphSheetIds[SHEET_TOTAL];
	int cursorIds[SHEET_TOTAL];
	int lsId;
	int enableOverlay;
	int sheetFocus;		// which graph responds to hover
	int paneSet;
	int connectStatus;
	
	struct {
		int current;
		int average;
		int mode;
		THRBUFFER rate;
	}stats;
	
	struct {
		int vid;
		int pid;
		int index;
		int id;		// match this device [index] with this [external] sensor device id, 0 = match anything
		unsigned char key[8];
		int activateOnInsertion;
	}device;
	
}TANTPLUS;

int page_antCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

int antGetDeviceCount (void *pageStruct);
int antConfigGetDeviceIds (void *pageStruct, int *vid, int *pid);
int antConfigIsAntEnabled (void *pageStruct);
int antIsAntEnabled (TVLCPLAYER *vp);



#endif
