
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



#ifndef _TASKMAN_H_
#define _TASKMAN_H_



typedef struct {
	uint64_t time;
	process_list_extended *ple;
	int count;
}ple_snapshot;

typedef struct{
	TPAGE2COM *com;
	TPANE *pane;
	TARTMANAGER *am;

	int procTimerId;
	int idleDisabled;
				
	struct {
		ple_snapshot *previous;
		ple_snapshot *current;
	}list;

	struct {
		wchar_t *system32;
		wchar_t *system64;
		wchar_t *windows;
		wchar_t *icons;
	}folders;

	struct {
		int imageSize;
		int rows;
		int lineSpacing;
		int textOffset;			// text overlay offset on vertical from bottom
	}layout;
	
	struct{
		int sysImg;
		int noIcon;
	}imageIds;

	struct{
		TGRAPH *graph;
		int enabled;
		int mode;
		int sheetIds[32];
		int colours[32];
		uint64_t lastUpdateTime;
	}cpugraph;
	
	struct {
		TPANE *pane;
		int font;
		int tItems;
		int imgId;
	}context;			 // process options information 


	int canRender;
	int stub2;
	int stub3;
	int stub4;
	int stub5;
	int stub6;
}TTASKMAN;


int page_taskmanCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);


#endif

