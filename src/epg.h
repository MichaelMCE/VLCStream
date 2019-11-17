
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



#ifndef _EPG_H_
#define _EPG_H_


#define EPG_PANE_SCROLL_DELTA		40		// scroll by n pixels either direction
#define EPG_GUIDE_EVENT_INVALID		0xFFFFFFFFFFF
#define EPG_GUIDE_ROOTID			101


typedef struct {
	int pid;				// stream/channel id with which this event belongs
    time64_t start;			// interpreted as a value returned by time()	(time64_t is defined in alarm.h)
    int32_t duration;		// duration of the event in seconds
    char *name;
    char *descriptionLong;
    char *descriptionShort;
    
    struct{
    	int paneExpanded;
    }ui;
}TGUIDE_EVENT;


typedef struct {
	TPAGE2COM *com;
	TCCBUTTONS *btns;

	int displayMode;

	struct{
		TVLCEPG **epg;
		int total;

		struct {
			int channelIdx;		// currently playing channel/stream (::epg)
			int programmeIdx;	// currently playing programme in above channel
			int pid;			// playing stream (this is not the event/show id)
			time64_t start;		// start time of playing event
		}playing;
	}vepg;

	struct{
		TLB *listbox;
	}programme;
	
    struct{
		TPANE *paneChannels;		// list of channels, left side of screen
		TPANE *paneContents;		// short guide description, right side of screen
		
		int yOffset;
		int xOffset;
		
    	struct{
    		int pgmEvent;
    		int updatePending;
    		int pendingX;
    		int pendingY;
    	}icons;

		TTREE *database;	// the actual guide is stored here, referenced by stream id then start time
		int programmeIdx;	// whichever stream/guide is on display
		int isRoot;
		int updatePending;	// true if vepg.epg has been updated after .database was last built
    }guide;
}TEPG;


int page_epgCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);



int epgFillProgrammeListbox (TEPG *epg, TLB *lb);
void epgProgrammeListboxRefresh (TEPG *epg);

void epgDvbGenPlaylist (TVLCPLAYER *vp);

int epgProgrammeLbGetTotal (TEPG *epg);
void epgDisplayOSD (TVLCPLAYER *vp);
void epgGetUpdate (TVLCPLAYER *vp);

int epgGetTotalProgrammes (TVLCPLAYER *vp);

#endif


