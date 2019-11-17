
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



#ifndef _CTRLOVERLAY_H_
#define _CTRLOVERLAY_H_

#define CTRL_TRACK_COLOUR		((0xFF<<24)|COL_WHITE)
#define CTRL_PLAYLIST_COLOUR	((0xFF<<24)|COL_YELLOW|100)
#define CTRL_HIGHLIGHT_COLOUR	((0xFF<<24)|COL_GREEN_TINT)


#define CTRL_PLAY				VBUTTON_PLAY
#define CTRL_PAUSE				VBUTTON_PAUSE
#define CTRL_STOP				VBUTTON_STOP
#define CTRL_PRETRACK			VBUTTON_PRETRACK
#define CTRL_NEXTTRACK			VBUTTON_NEXTTRACK


#define BTNPANEL_SET_PLAY		1
#define BTNPANEL_SET_PAUSE		2
#define BTNPANEL_SET_STOP		3



typedef struct{
	TLPOINTEX loc;
	int state;
}TPADSIPE;

typedef struct{
	TLABEL *label;
	int lblId;
	int lblIdText;
		
	double sigma;
	double rho;
	double expMultiplier;
	double expMember;
	double time0;
	double accel;
	
	char *text;
	int font;
	int displayType;
	int textWidth;
	int textHeight;
}TTEXTSCALE;

typedef struct{
	TLISTITEM head;
	double timeLast;
	int count;
}TTEXTSCALELLIST;


#define VOLUMEIMAGETOTAL		10		// 0 to 9

typedef struct{
	TPAGE2COM *com;

	TSLIDER *trackSlider;			// track position time slider
	TFRAME *chapterMark;
	TFRAME *chapterMarkFilled;
	TMARQUEE *marquee;				// for display track titles only
	TLABELSTR *title;
	TLABELSTR *album;

	int lbUID;						// current listbox playlist uid (what its filled by)
	int listboxMaxHeight;

	TCCBUTTONS *btns;
	TBTNPANEL ctrlpan;
	int lbUnderlayImgId;
	
	TTEXTSCALELLIST scale;
		
	struct {
		int imageIds[VOLUMEIMAGETOTAL];
		TFRAME *images[VOLUMEIMAGETOTAL];

		int verticalOffset;
		int charOverlap;
		int doRender;
		int val;
		int changeDelta;
	}volume;
	
	
	struct {
		TLISTBOX *lb;
	}listbox;
	
}TVIDEOOVERLAY;


int page_plyctrlCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

int plyctrlButtonPress (TVIDEOOVERLAY *plyctrl, TCCBUTTON *btn, const int btnId, const TTOUCHCOORD *pos);


void timer_playlistListboxRefresh (TVLCPLAYER *vp);

void overlayActivateOverlayResetTimer (TVLCPLAYER *vp);

void overlaySetOverlay (TVLCPLAYER *vp);
void overlayResetOverlay (TVLCPLAYER *vp);
void overlayDisplayVolReset (TVLCPLAYER *vp);
void overlayTimeStampSetTime (TVLCPLAYER *vp);
void setVolumeDisplay (TVLCPLAYER *vp, const int volume);

void ctrlPanCalcPositions (TBTNPANEL *btnpan, const int set);
void ctrlNewTrackEvent (TVIDEOOVERLAY *plyctrl, unsigned int uid, const int trackIdx);
void ctrlPlayback (TVLCPLAYER *vp, const int func);

// also used by AntPlus page
int ctrlPanGetEnabledTotal (TBTNPANEL *btnpan, const int total);


void overlayAddTitle (TVIDEOOVERLAY *plyctrl, const char *text);



#endif


