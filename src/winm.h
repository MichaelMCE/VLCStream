
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



#ifndef _WINM_H_
#define _WINM_H_


#define APP_ICON					101

enum _bitmapicons {
	BM_FOLDER,
	BM_PLAYING,
	BM_FOLDERPLY,
	BM_POSITION,
	BM_CHAPTERS,
	BM_PLAY,
	BM_PAUSE,
	BM_STOP,
	BM_PREV,
	BM_NEXT,
	BM_VLC,
	BM_MUTE,
	BM_MUTED,
	BM_VOLUME,
	BM_PROGRAMME,
	BM_TOTAL
};




#define TB_TRANSPARENT				(1)
#define TB_OPAQUE					(2)

#define TRAY_PLY_BASEID				(100)
#define TRAY_PLY_TITLE				(TRAY_PLY_BASEID-1)
#define TRAY_PLY_BACK				(TRAY_PLY_TITLE+1)



#define TRAY_MENU_BASE				(10)

enum _context_menu{
	TRAY_MENU_POSITION = TRAY_MENU_BASE,
	TRAY_MENU_VOLUME,
	TRAY_MENU_MUTE,
	TRAY_MENU_PLAYINVLC,
	TRAY_MENU_CHAPTERS,
	TRAY_MENU_PROGRAMME,
	TRAY_MENU_SUBTITLES,
	TRAY_MENU_PLAY,
	TRAY_MENU_STOP,
	TRAY_MENU_PREV,
	TRAY_MENU_NEXT,
	TRAY_MENU_SCREENSHOT,
	TRAY_MENU_SAVEPLAYLIST,
	TRAY_MENU_FLUSH,
	TRAY_MENU_RTD,		// render to desktop
	TRAY_MENU_EXIT
};

#define TRAY_MENU_CONTEXT_BASE		(TRAY_MENU_BASE)
#define TRAY_MENU_CONTEXT_MAX		(50)
#define TRAY_MENU_CONTEXT_UPPER		(TRAY_MENU_CONTEXT_BASE+TRAY_MENU_CONTEXT_MAX-1)

#define TRAY_MENU_CHAPTERS_BASE		(TRAY_MENU_BASE+300)
#define TRAY_MENU_CHAPTERS_MAX		(200)
#define TRAY_MENU_CHAPTERS_UPPER	(TRAY_MENU_CHAPTERS_BASE+TRAY_MENU_CHAPTERS_MAX-1)

#define TRAY_MENU_SUBTITLES_BASE	(TRAY_MENU_BASE+600)
#define TRAY_MENU_SUBTITLES_MAX		(50)
#define TRAY_MENU_SUBTITLES_UPPER	(TRAY_MENU_SUBTITLES_BASE+TRAY_MENU_SUBTITLES_MAX-1)

#define TRAY_MENU_PROGRAMME_BASE	(TRAY_MENU_BASE+700)
#define TRAY_MENU_PROGRAMME_MAX		(200)
#define TRAY_MENU_PROGRAMME_UPPER	(TRAY_MENU_PROGRAMME_BASE+TRAY_MENU_PROGRAMME_MAX-1)

#define TRACK_POSITION_LENGTH		(200)
#define TRACK_POSITION_RANGE		(TRACK_POSITION_LENGTH+20)
#define TRACK_POSITION_STEPSIZE		(TRACK_POSITION_RANGE/10)

#define TRACK_VOLUME_HEIGHT			(150)
#define TRACK_VOLUME_RANGE			(100)	// should match the volume range of libvlc
#define TRACK_VOLUME_STEPSIZE		(TRACK_VOLUME_RANGE/10)





#define WINTOOLBAR_NANELENGTH		(63)
#define WINSYSTRAY_IMAGETOTAL		BM_TOTAL

typedef struct{
	int enabled;
	char toolbarName[WINTOOLBAR_NANELENGTH+1];
	HWND hwnd;
	T2POINT pos;		// print text here
	
	vlcs_tb_colour colour;	// defined in settings.h
	vlcs_tb_font font;
	vlcs_tb_strings string;
}TVLCSTASKBAR;


typedef struct{
	int enabled;			// tray, menu, etc.. anything else
	int tipsEnabled;
	int infoEnabled;
	HANDLE hwnd;			// handle to main window
	HBITMAP hbm[WINSYSTRAY_IMAGETOTAL];	// BM_ resource icons
	
	struct{					// playlist browser
		volatile HMENU hmenu;
		POINT menuPos;
		int lastSelection;
	}playlist;
	
	struct{					// right click context menu
		volatile HMENU hmenu;
		POINT menuPos;
		int menuChaptersAdded;	// was chapters entry added
		int menuProgrammeAdded;	// was programme entry added
		int menuSubtitlesAdded;	// was subtitles entry added
	}context;

	struct{					// right click context menu
		volatile HMENU hmenu;
		POINT menuPos;
		int lastSelection;
		int isVisable;
	}chapters;
	
	struct{
		volatile HMENU hmenu;
		POINT menuPos;
		int *pidLookup;
		int pidTotal;
		int isVisable;
	}programme;	
		
	struct{
		volatile HMENU hmenu;
		int lastSelection;
	}subtitles;
		
	struct{					// playlist track position trackbar
		volatile HWND hwnd;
		POINT menuPos;
		int isVisable;
	}position;
	
	struct{					// volume control
		volatile HANDLE hwnd;
		POINT menuPos;
		int isVisable;
	}volume;
}TVLCSTRAY;

int startMouseCapture (TVLCPLAYER *vp);
void endMouseCapture (TVLCPLAYER *vp);

int captureMouseToggleState (TVLCPLAYER *vp);
void wmTakeSnapshot (TVLCPLAYER *vp);

void taskbarRemoveString (TVLCSTASKBAR *tb);
void timer_drawTaskbarTrackTitle (TVLCPLAYER *vp);

int playlistImportPath (TVLCPLAYER *vp, PLAYLISTCACHE *plc, char *pathIn);


void cds_send (HWND hWin, const int msg, void *ptr, const int data);
void cds_receive (TVLCPLAYER *vp, WPARAM wParam, COPYDATASTRUCT *cds);

int captureKeyboard (TVLCPLAYER *vp, const int state);
int getModifierKeyState (TVLCPLAYER *vp, const int kp_key);


void playlistMenuSetTipTrack (TVLCPLAYER *vp, TVLCSTRAY *tray, char *title, const int uid, const int track);
int contextMenuIsTrackbarVisable (TVLCPLAYER *vp);
int contextMenuIsVolumeVisable (TVLCPLAYER *vp);
int contextMenuIsChaptersVisable (TVLCPLAYER *vp);
void taskbarPostMessage (TVLCPLAYER *vp, const int msg, const int var1, const intptr_t var2);






#if (MOUSEHOOKCAP)

int captureMouse (TVLCPLAYER *vp, const int state);
void consoleToggle (TVLCPLAYER *vp);
void touchSimulate (const TTOUCHCOORD *pos, const int flags, TVLCPLAYER *vp);

#else

#define captureMouse(a,b)
#define consoleToggle(a)
#define touchSimulate(a,b,c)

#endif




#endif


