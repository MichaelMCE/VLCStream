
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

#ifndef _VLCSTREAM_H_
#define _VLCSTREAM_H_

#include "vlcheaders.h"


#if 0
#ifndef _snprintf
#define _snprintf snprintf
#endif

#ifndef _snwprintf
#define _snwprintf snwprintf
#endif

#ifndef _swprintf
#define _swprintf swprintf
#endif

#ifndef _vswprintf
#define _vswprintf vswprintf
#endif
#endif


typedef struct{
	TTOUCHCOORD pos;
	int flags;
	void *ptr;
}TTOUCHINPOS;

#if (LIBVLC_VERSION_MAJOR < 2)
typedef struct{
	char *x;
	char *y;
}TGOOMRESSTR;
#endif

typedef struct{
	int ratio;
	char *description;
}TRATIO;

typedef struct{
    uint32_t *pixels;
    uint32_t *pixelBuffer;
    int currentBuffer;
    size_t bufferSize;
    HANDLE hEvent;
    TMLOCK *hVideoLock;
    TMLOCK *hVideoCBLock;
    TFRAME *vmem;
    TFRAME *working;
    TFRAME *workingTransform;
    
	struct{
		int preset;
		
		// custom values when preset is BTN_CFG_AR_CUSTOM
		double ratio;
		int x;
		int y;
		int width;
		int height;
		int clean;
	}aspect;
	
    struct {
		char bitBuff[sizeof(BITMAPINFO)+16];
		BITMAPINFO *bitHdr;
		HWND wnd;
		//int copyMode;		// 1=1:1 src copy, 2=best fit
		/*int cxScreen;
		int cyScreen;
		int wStretched;
		int hStretched;*/
		
		int enable;
		//HDC window_dc;
		//HBITMAP off_bitmap;
		//HDC off_dc;
		//void *p_pic_buffer;
		//BITMAPINFOHEADER bmiHeader;
	}winRender;
}TCTX;

typedef struct{
	TCCBUTTON *button;
	void *ptr;
}TBUTTONHLCB;

typedef struct{
	libvlc_track_description_t *desc;
	int total;		// total subtitles in desc
	int selected;	// manually selected subtitle, -1 for none/disabled/auto
}TVLCSPU;


typedef struct{
	libvlc_instance_t *hLib;
	libvlc_media_t *m;
	libvlc_media_player_t *mp;
	libvlc_event_manager_t *emp; // media play[er/ing] events
	libvlc_event_manager_t *em;	// media events
	
	libvlc_time_t length;
	TVLCSPU	spu;
	int volume;
	int swapColourBits;	// swap r and b components
	volatile double position;
	volatile int vlcPlayState;
	int playState;
	int playEnded;
	int playRandom;
	int isMediaLoaded;
	int openCount;
	int width;
	int height;
	int videoWidth;		// width of source video
	int videoHeight;	// height of source video
	int bpp;
	int visMode;		// vis_disabled, vis_goom, etc..
	double bufferPos;
	
	int hasAttachments;

	double rotateAngle;
	double scaleFactor;
	int blurRadius;
	int pixelize;
	double brightness;
	double contrast;
	double saturation;
	double gamma;
	int rotateOp;
	int scaleOp;
	
	int audioDesync;
	int subtitleDelay;
	
	double vTime0;		// current video frame time
	double dTime[32];	// time delta
	int fIndex;

	libvlc_instance_t *hLibTmp;
	TMLOCK *hLockLengths;
}TVLCCONFIG;

typedef struct{
	int bgPathTotal;
	TFRAME *bg;
	TSWATCH	swatch;
	int currentIdx;
	wchar_t folder[MAX_PATH+1];
}TSKIN;

typedef struct{
	int uid;
	int track;
}TTIMERPLAYTRACK;

typedef struct{
	int isBuilt;
	TFRAME *topLeft;
	TFRAME *topRight;
	TFRAME *btmLeft;
	TFRAME *btmRight;
	TFRAME *vertBarTop;
	TFRAME *vertBarBtm;
	TFRAME *horiBarLeft;
	TFRAME *horiBarRight;
}TSHADOWUNDER;

typedef struct{
	int save;				// take snapshot and save as SNAPSHOTFILE
	wchar_t filename[MAX_PATH+1];
	int annouce;
}TSNAPSHOT;

typedef struct{
	TSKIN skin;
	TGUIINPUT cursor;
	int modifierKeyStates;
	//TFRAME *cursorUnderlay;
	
	int ccIds[CCID_TOTAL];	// CC handle lookup
	int image[IMGC_TOTAL];	// imageCache handle Id's
	
	double inputCallLength;	// time taken to handle last completed touch input event
	uint64_t awakeTime;		// time since track last ended
	int idleTime;			// (IDLETIME)
	int mOvrTime;			// (MCTRLOVERLAYPERIOD)
	double idleFPS;			// (UPDATERATE_IDLE)
	int awake;
	double lastUpdateRate; // update rate before current rate was set

	int idleDisabled;
	TVLCSTASKBAR taskbar;
	TVLCSTRAY tray;

	struct{
		struct{
			wchar_t *path;
			int originX;
			int originY;
			int tile;
		}wallpaper;

	}desktop;

	int runCount;			// shouldn't be here
	int lastTrack;			// as above
	
	int drawStats;
	//int visual;			// which visual to use for audio tracks
	TSNAPSHOT snapshot;
	int drawMetaTrackbar;
	int drawControlIcons;
	int drawVisuals;

	//int ratio;
	int clockType;
	volatile int padctrlMode;	// who has touch pad focus, this application or OS. used by SBUI
#if ENABLE_BRIGHTNESS
	int brightness;			// hardware backlight level
#endif
	double targetRate;		// try to maintain this fps
	int page_gl;
	unsigned int frameCt;	// frame count loop
	unsigned int renderId;	// updated once per render pass
	
	int displayStats;
	int artSearchDepth;
	int artMaxWidth;
	int artMaxHeight;
	int shelfNoArtId;
	 
	TSHADOWUNDER shadow[4];		// black, blue and green underlays, idx 0 is empty
	TBUTTONHLCB btncb;
	
	unsigned int winMsgThreadID;
	uintptr_t hWinMsgThread;
	
	unsigned int winDispatchThreadID;
	uintptr_t hWinDispatchThread;
	TMLOCK *hDispatchLock;
	HANDLE hDispatchEvent;
	HANDLE hUpdateEvent;
	int updateSignaled;
	
	int pageAccessed[PAGE_TOTAL];		// manually set
	
	HWND hMsgWin;			// main input event message thread
	TMLOCK *hLoadLock;		// media load lock
	TMLOCK *hRenderLock;	// prevent render and input thread atemptting simultaneous writes
	TMARQUEE *marquee;		// a general purpose marquee (text only)
	TPICQUEUE *picQueue;	// similar to marqueue but image only
	TTIMERPLAYTRACK playtrack;
	
	struct{
		int isVlcRunning;
		int alwaysAccessible;	// should the global hotkeys page (PAGE_HOTKEYS) be available when VLC is not running

		int globalEnabled;		// global hotkeys enabled (vlc)
		int localEnabled;		// local hotkeys enabled (cursor/kbd hook)
				
		ATOM kid[32];		// keyboard media keys
		char cursor;		// global hotkey to enable mouse hook, default is ctrl+shift+'A'
		char console;		// internal console prompt, default is ctrl+shift+'L'
	}hotkeys;
	
	struct {
		unsigned long token;
	}gdip;
}TGUI;


typedef struct{
	TTIMER queue[TIMER_TOTAL];
}TTIMERS;


#if (!SINGLEINSTANCE_USE_CDS)
typedef struct{
	int state;
	unsigned int hash;
	int len;
	char *path;		// utf8 path
	int charPos;
	int action;
}TPATHMSG;
#endif


typedef struct{
	PLAYLISTCACHE *root;
	int queued;
	int display;
	int noPlaylist;		// set if "-noplaylist" was passed at cmdline. don't load or save a default playlist
}TPLAYLIST;



struct TVLCPLAYER {
	volatile int applState;
	volatile int renderState;		// active or not

	uint64_t freq;
	uint64_t tStart;
	double resolution;
	
	int currentFType;		// 0:video, 1:audio, 2:image (PLAYBACKMODE_xxx)
	int updateTimer;		// main refresh ticker id

	double fTime;			// current ui frame time
	double dTime[16];		// frame ui delta time (pre render)
	double rTime;			// time taken to render last frame
	double lastRenderTime;	// time update was last sent to display (post render)
	HANDLE instanceEvent;
	HANDLE instanceModule;
	int instanceCheck;
	unsigned int pid;
			
	TCC *cc;
	TCTX ctx;
	TGUI gui;
	TVLCCONFIG vlcconfig;	// libvlc config storage
	TVLCCONFIG *vlc;		// point to current libvlc player config
	TPAGES2 *pages;
	TTIMERS timers;
	TPLAYLIST playlist;
	TPLAYLISTMANAGER *plm;	// playlist manager
	TMETATAGCACHE *tagc;	// tag cache
	TFRAMESTRINGCACHE *strc;	// string (TFRAME) cache. TODO: rewrite this
	TSETTINGS settings;
	TEDITBOX input;
	TMYLCD *ml;
	TIMAGEMANAGER *im;		// general UI image manager - use this for UI only (.png only, anything known at compile time and which isn't artwork)
	TARTMANAGER *am;		// artwork image manager - use this for artwork (.jpg and .png only) and non UI only
	//job_controller *jc;		// tasked with searching for and downloading artwork
#if (ENABLE_BASS || !RELEASEBUILD)
	TBASSLIB bass;
#endif
	
#if (!SINGLEINSTANCE_USE_CDS)
	TPATHMSG pathmsg;
#endif
#if ENABLE_CMDFUNSTUFF
	struct{
		TCMDREPLY *sheets;
	}bot;
#endif
};







#define getFrontBuffer(x)	(((TVLCPLAYER*)(x))->ml->front)
#define getApplState(x)		(((TVLCPLAYER*)(x))->applState)
#define getConfig(x)		(((TVLCPLAYER*)(x))->vlc)
//#define getPlayState(x)		((getConfig(x)->playState) && (getConfig(x)->playState != 8))	/* broken */
#define isVirtual(x)		(((TVLCPLAYER*)(x))->ml->enableVirtualDisplay)


int getPlayState (TVLCPLAYER *vp);
double getFPS (TVLCPLAYER *vp);
void unloadMedia (TVLCPLAYER *vp, TVLCCONFIG *vlc);
void setApplState (TVLCPLAYER *vp, int state);

double getTime(TVLCPLAYER *vp);
uint64_t getTime64 (TVLCPLAYER *vp);


int cursorSetState (TGUIINPUT *cursor, const int state);
int cursorGetState (TGUIINPUT *cursor);

#if ENABLE_BRIGHTNESS
int setDisplayBrightness (TVLCPLAYER *vp, int level);
#endif

int playerWriteDefaultPlaylist (TVLCPLAYER *vp, const wchar_t *playlist);

int startPlaylistTrack (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int track);
int startPlaylistTrackUID (TVLCPLAYER *vp, const int uid, const int track);

void setPlaybackMode (TVLCPLAYER *vp, int mode);
int getPlaybackMode (TVLCPLAYER *vp);
void updateTickerStart (TVLCPLAYER *vp, double fps);
void waitForRenderUpdate (TVLCPLAYER *vp, const unsigned int timeout);

void setAwake (TVLCPLAYER *vp);
void wakeup (TVLCPLAYER *vp);
int getIdle (TVLCPLAYER *vp);

int setVolume (TVLCPLAYER *vp, int volume, const int whichVolume);
int getVolume (TVLCPLAYER *vp, const int whichVolume);
void toggleMute (TVLCPLAYER *vp);
int getMute (TVLCPLAYER *vp);

void cleanVideoBuffers (TVLCPLAYER *vp);

int renderLock (TVLCPLAYER *vp);
void renderUnlock (TVLCPLAYER *vp);
void resetHighlight (TVLCPLAYER *vp);

int setPlaybackRandom (TVLCPLAYER *vp, const int state, const int report);
int getPlaybackRandom (TVLCPLAYER *vp);


int isMediaDVB (const char *name);
int isMediaDShow (const char *name);
int isMediaFile (const char *name);
int isMediaScreen (const char *name);
int isMediaDVD (const char *name);
int isMediaHTTP (const char *name);
int isMediaMMS (const char *name);
int isMediaRTSP (const char *name);
int isMediaUDP (const char *name);
int isMediaRemote (const char *name);
int isMediaPlaylist (const char *name);
int isMediaLocal (const char *name);

void resetCurrentDirectory ();

void trackStop (TVLCPLAYER *vp);
void trackPlay (TVLCPLAYER *vp);
void trackPrev (TVLCPLAYER *vp);
void trackNext (TVLCPLAYER *vp);
void trackPlayPause (TVLCPLAYER *vp);

int startVlc (TVLCPLAYER *vp);
int startVlcTrackPlayback (TVLCPLAYER *vp);

void loadUnlock (TVLCPLAYER *vp);
int loadLock (TVLCPLAYER *vp);

int loadMediaPlayer (TVLCPLAYER *vp, TVLCCONFIG *vlc, char *inlineOpts, char *opts);
int loadMedia (TVLCPLAYER *vp, TVLCCONFIG *vlc, char *mediaPath, char *opts);

void renderStatsEnable (TVLCPLAYER *vp);
void renderStatsDisable (TVLCPLAYER *vp);
void renderStatsToggle (TVLCPLAYER *vp);

//#define setTargetRate(a,b) _setTargetRate(a,b,__func__, __LINE__)
//void _setTargetRate (TVLCPLAYER *vlc, const double fps, const char *func, const int line);
void setTargetRate (TVLCPLAYER *vlc, const double fps);	// when something is happening
void setBaseUpdateRate (const double rate);	// when nothing is happening but not yet idle
double getBaseUpdateRate ();

// force a render pass immediately
void renderSignalUpdateNow (TVLCPLAYER *vp);

// initiate render pass on next timer slot
void renderSignalUpdate (TVLCPLAYER *vp);
//void _renderSignalUpdate (TVLCPLAYER *vp, const char *func, const int line);
//#define renderSignalUpdate(a) _renderSignalUpdate(a, __func__, __LINE__)



int configStartDisplay (TVLCPLAYER *vp, const int enabledVD);
void configLoadSwatch (TVLCPLAYER *vp);

int hasPageBeenAccessed (TVLCPLAYER *vp, const int pageId);
void setPageAccessed (TVLCPLAYER *vp, const int pageId);
void resetPageAccessed (TVLCPLAYER *vp, const int pageId);


#endif



