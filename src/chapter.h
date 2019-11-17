
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



#ifndef _CHAPTER_H_
#define _CHAPTER_H_


#define CHAPTER_SCROLL_DELTA		16		// scroll by 16 pixels either direction


typedef struct {
	TPAGE2COM *com;
	
	TLPOINTEX rect;
	int ttitles;	// total titles
	int tchapters;	// chapters in selected title
	int ctitle;		// currently viewing title
	int schapter;	// selected chapter
	int cchapter;	// currently playing chapter
	int title;		// currently playing title
	int displayMode;
		    
    TCCBUTTONS *btns;
    TPANE *pane;
    
	struct{
		TMLOCK *lock;
		TTITLE *list;
		int total;
    }titles;

    struct{
    	int chapters;
    	int play;
    }icons;
}TCHAPTER;


int page_chapCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

void chaptersUnlock (TCHAPTER *chap);
int chaptersLock (TCHAPTER *chap);

// lock must be held
uint64_t chaptersGetChapterTimeOffset (TCHAPTER *chap, const int titleIdx, const int chapIdx);


void chapterGetDetails (TVLCPLAYER *vp, TVLCCONFIG *vlc, TCHAPTER *chapt);
int chaptersGetTotal (TCHAPTER *chap);
void chaptersGetUpdate (TVLCPLAYER *vp);

char *getChapterNameByTime (TVLCPLAYER *vp, int64_t timeOffset);
char *getChapterNameByIndex (TVLCPLAYER *vp, const int chapIdx);
int getTotalChapters (TVLCPLAYER *vp);
void setChapter (TVLCPLAYER *vp, const int chapterIdx);
int getPlayingChapter (TVLCPLAYER *vp);

#endif


