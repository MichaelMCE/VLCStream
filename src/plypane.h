
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


#ifndef _PLYPANE_H_
#define _PLYPANE_H_


#define PLYPANE_SCROLL_DELTA	40

typedef struct {
	TPAGE2COM *com;
	
	TPANE *pane;
	TSTACK *uidStack;	// history of which playlist we've viewed (stack contains playlist Id's (uid/pid)
	TLABEL *locBar;
	TLABEL *title;
	
	
	struct{
		int audio;
		int folder;
		int back;
		int play;
		int art;		// noart replacement
		int closePane;
	}icons;
	
	int playingItemId;
	int timestampItemId;
}TPLYPANE;



int page_plyPaneCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

int plypaneSetPaneUID (TPLYPANE *plypane, const int uid);
int plypaneGetPaneUID (TPLYPANE *plypane);


int plyPaneRefresh (TVLCPLAYER *vp);
void plypaneUpdateTimestamp (TPLYPANE *plypane);

void timer_plyPaneRefresh (TVLCPLAYER *vp);
int plypaneSetPlaylist (TVLCPLAYER *vp, const int uid);



#endif
