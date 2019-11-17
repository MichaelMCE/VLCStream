
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


#ifndef _UI_H_
#define _UI_H_

#define PLAYER_NAME				"VLCStream"
#define PLAYER_DATE				"05NOV2019"	/* __DATE__ */
#define PLAYER_VERSION			PLAYER_NAME "-" PLAYER_DATE

#define VCHROMA					"RV32"			/* Select VLC output chroma */
#define VPITCH(w)				((w)<<2)		/* pitch based upon above chroma*/
#define DVIDBUFBPP				LFRM_BPP_32		/* use RGB where ARGB is not required (faster) */
#define ARTWORKBPP				DVIDBUFBPP
#define SKINFILEBPP				LFRM_BPP_32A
#define SKINDROOT				L"vsskin"
#define FONTD(x)				SKINDROOT "/data/"x
#define FUNSTUFFD(x)			SKINDROOT "/data/text/"x
#define CFGFILE					SKINDROOT "/config.cfg"
#define KP_CONFIGFILE			SKINDROOT "/keypad.cfg"
#define EXTS_CONFIGFILE			SKINDROOT "/fileext.cfg"
#define MAPS(x)					FONTD(L"mapnik/")x
#define SNAPSHOTFILE			L"vlcs_snapshot.png"

#define VLCSPLAYLIST			L"vlcsplaylist.m3u8"
#define VLCSPLAYLISTFILE		"vlcsplaylist"
#define VLCSPLAYLISTEXT			".m3u8"
#define VLCSPLAYLISTEXTW		L".m3u8"
#define PLAYLIST_PRIMARY		"root playlist"
#define GLOBALFILEMASK			L"*.*"		// used as part of the explorer file discovery
#define MYCOMPUTERNAME			L"My Computer"		/* default alias to use in the event actual alias could not be retrieved */
#define MYDOCUMENTS				L"My Documents"		/* as above */
#define MYDESKTOP				L"Desktop"
#define ALARM_INITIALTIME		"08:00"

#define CLK_DIGITS				"softglow"
#define HRM_DIGITS				"cold"
#define VOL_DIGITS				"warm"

#define WINTOOLBAR_NAME			"vlcS"			// case sensitive
#define WINTOOLBAR_ICONS		(5)				// play/stop/prev/next/flush
#define WINTOOLBAR_ICONWIDTH	(24)			// small = (24), large = (32+6)



#define MFONT						LFTW_B24 	/* WENQUANYI12P used where full unicode is required (track meta detail) */
#define LFONT						LFTW_B34


#define CLK_POLAR_LABEL_FONT		LFTW_B24
#define CLK_POLAR_INNER_FONT		LFTW_B34
#define CLK_PREDATOR_TIME_FONT		LFTW_RACER96
#define CLK_BOXDIGITAL_TIME_FONT	LFTW_198FIVE162
#define CLK_SBDK_DATE_FONT			LFTW_RACER96
#define PANE_FONT					LFTW_B28
#define PANE_LOCBAR_FONT			LFTW_B28
#define PANE_TITLE_FONT				LFTW_B26
#define PANE_TIMESTAMP_FONT			LFTW_B24
#define PANE_FILTER_FONT			LFTW_B34
#define PANE_SORT_FONT				PANE_FILTER_FONT
#define	PANE_FODLERSTATS_FONT		LFTW_B26
#define SEARCH_RESULT_FONT			LFTW_B28
#define SEARCH_CONTEXT_FONT			LFTW_B28
#define SEARCH_HELP_FONT			LFTW_B26
#define SEARCH_EDITBOX_FONT			LFTW_B24
#define SEARCH_HEADER_FONT			LFTW_B24
#define CFG_VARNAME_FONT			LFTW_76LONDON34
#define EDITBOX_FONT				LFTW_B24
#define DMSG_FONT					LFTW_B24
#define HOME_CLK_FONT				LFTW_76LONDON38
#define PLAYLIST_PANEL_FONT			LFTW_B24
#define PLAYLIST_TV_FONT			LFTW_B28
#define PLAYLIST_TV_NP_FONT			LFONT
#define PANEL_FONT					LFTW_B28
#define KEYPAD_INPUT_FONT			LFONT
#define BROWSER_FONT				LFONT
#define BROWSER_PATH_FONT			LFTW_B24
#define BROWSER_PANEL_SEP_FONT		LFTW_B24
#define CTRLOVR_LISTBOX_FONT		LFTW_B28
#define CTRLOVR_TIMESTAMP_FONT		LFTW_76LONDON38
#define CTRLOVR_SLIDER_FONT			LFTW_B24
#define CTRLOVR_META_FONT			LFTW_B34
#define CTRLOVR_NEWTITLE_FONT		LFTW_UNICODE72
#define ALBUM_FONT					LFONT
#define ALARM_SETTIME_FONT			LFTW_76LONDON150
#define ALARM_DAY_FONT				LFTW_UNICODE36
#define ALARM_TIME_FONT				LFTW_UNICODE72
#define ALARM_DATE_FONT				LFTW_UNICODE36
#define ALARM_STATUS_FONT			LFTW_B34
#define ALBUM_SLIDER_FONT			CTRLOVR_SLIDER_FONT
#define TEXTOVERLAY_FONT			LFTW_UNICODE72
#define EQ_SLIDER_FONT				LFTW_B24
#define EQ_PRESET_FONT				LFTW_76LONDON34
#define CHAPTERS_FONT				LFONT
#define TRACKPLAYMARQ_FONT			LFTW_B24
#define META_FONT					LFTW_B24
#define ESTREAM_FONT				LFTW_B24
#define PLAYLISTPLM_PATH_FONT		MFONT
#define PLAYLISTPLM_LABELS_FONT		LFTW_B24
#define SUBTITLE_FONT				LFTW_B24
#define VTRANSFORM_FONT				LFTW_B24
#define EPG_FONT					LFTW_B24
#define EPG_PROGRAMLISTBOX_FONT		LFTW_B28
#define UPDATERATE_FONT				LFTW_B24
#define GBK_PANEL_FONT				LFTW_B28
#define ANT_HEADING_FONT			LFTW_76LONDON38
#define GARMIN_HEADING_FONT			LFTW_76LONDON38
#define ANT_MARQUEE_FONT			LFTW_B24
#define TETRIS_STATS_FONT			LFTW_76LONDON38
#define TETRIS_KEYS_FONT			LFTW_76LONDON34
#define FILEPANE_FILTER_FONT		LFTW_B34
#define FILEPANE_SORT_FONT			LFTW_B34
#define STARTUP_LOADING_FONT		LFTW_76LONDON38

#define HIGHLIGHTPERIOD			200					/*   */
#define MCTRLOVERLAYPERIOD		6000
#define UPDATERATE_IDLE			(0.1)				/* maintain this fps when idle (0.1 = 1 update every 10 seconds) */
#define UPDATERATE_ALIVE		(50.0)				/* update rate at which the update ticker fires, from which renders are calculated off */
#define UPDATERATE_BASE_VISUALS	(25.0)
#define UPDATERATE_BASE_DEFAULT	(1.0)
#define UPDATERATE_MAXUI		(35.0)				/* maximum allowed UI rate with mouse (hooking) enabled */
#define UPDATERATE_LENGTH		(1000)
#define IDLETIME				(100 * 1000)		/* ms, time to go idle after this length of inactivity */
#define CACHEREADAHEAD			(12)					/* preload the meta data from this number of tracks in advance */
#define ARTWORKFLUSH_PERIOD		(120 * 60 * 1000)
#define ARTWORKTHREADS			(3)					/* default number of threads dedicated to the retrieval, loading and conversion of artwork */
#define MAX_PATH_UTF8			((8*MAX_PATH)+1) 	/* should be large enough to cover every combination and eventuality */
#define MOFFSETX				(4)					/* cursor offset point x within bitmap */
#define MOFFSETY				(3)					/* as above but for y */
#define MAX_CHAPTERS			(200)
#define OPTSEPARATOR			'|'
#define OPTSEPARATORSTR			"|"
#define	THREADSTACKSIZE			(8*1024*1024)
#define TV_DRAGDELTA			20
#define EDITBOX_CARET1			9478
#define EDITBOX_CARET2			9482
#define PANE_ACCELERATION_X		(3.0)				// default movement multiplier for the pane widget
#define PANE_ACCELERATION_Y		(3.0)

#define PLAYBACKMODE_VIDEO		(0)
#define PLAYBACKMODE_AUDIO		(1)
#define PLAYBACKMODE_IMAGE		(2)
#define MAXTINQUEUESIZE			(256)				// length of input queue
#define TOUCH_VINPUT			(0x1000)

// used with get/setVolume
#define VOLUME_APP				(0)		// vlc'splayback volume
#define VOLUME_MASTER			(1)		// windows master volume (wasapi)

#define COL_ALPHA(a)			((a)<<24)
#define COL_RED					(0xFF0000)
#define COL_GREEN				(0x00FF00)
#define COL_BLUE				(0x0000FF)
#define COL_WHITE				(0xFFFFFF)
#define COL_BLACK				(0x000000)
#define COL_GRAY				(0x777777)
#define COL_PURPLE				(COL_RED|COL_BLUE)
#define COL_YELLOW				(COL_RED|COL_GREEN)
#define COL_AQUA				(COL_GREEN|COL_BLUE)
#define COL_CYAN				(0x00B7EB)
#define COL_ORANGE				(0xFF7F11)
#define COL_BLUE_SEA_TINT		(0x508DC5)		/* blue, but not too dark nor too bright. eg; Glass Lite:Volume */
#define COL_GREEN_TINT			(0x00FF1E)		/* softer green. used for highlighting */
#define COL_PURPLE_GLOW			(0xFF10CF)		/* nice when used as a text glow effect */
#define COL_HOVER				(0xB328C672)
#define COL_TASKBARFR			(0x141414)
#define COL_TASKBARBK			(0xD4CAC8)
#define COL_SOFTBLUE			(0X7296D3)

/*
#ifdef RGB
#undef RGB
#endif
#define RGB(r,g,b) (((int)(r)<<16)|((int)(g)<<8)|(int)(b))
*/

#define SWH_PLY_IPLAYING		(0)
#define SWH_PLY_ISELECTED		(1)
#define SWH_PLY_RECTPLAYING		(2)
#define SWH_PLY_RECTSELECTED	(3)
#define SWH_PLY_PLACEHOLDER		(4)
#define SWH_PLY_PLAYPOSC1		(5)
#define SWH_PLY_PLAYPOSC2		(6)
#define SWH_PLY_PLAYPOSC3		(7)
#define SWH_PLY_PLAYPOSC4		(8)
#define SWH_PLY_PLAYPOSC5		(9)
#define SWH_PLY_TRKTEXT			(10)
#define SWH_PLY_TRKBACK			(11)
#define SWH_PLY_TRKOUTLINE		(12)
#define SWH_PLY_PLYLNAME		(13)
#define SWH_PLY_PLYLNAMEBK		(14)

#define SWH_OVR_CHAPMARK		(0)
#define SWH_OVR_CHAPMARKFILL	(1)
#define SWH_OVR_MARQTEXT		(2)
#define SWH_OVR_MARQTEXTBK		(3)
#define SWH_OVR_EBOXTEXT		(4)
#define SWH_OVR_EBOXTEXTBK		(5)
#define SWH_OVR_EBOXIPLAYING	(6)
#define SWH_OVR_EBOXISELECTED	(7)
#define SWH_OVR_FPSTEXT			(8)
#define SWH_OVR_FPSOUTLINE		(9)
#define SWH_OVR_TIMESTAMPTEXT	(10)
#define SWH_OVR_TIMESTAMPTEXTBK	(11)
#define SWH_OVR_PANELEDGE		(12)
#define SWH_OVR_TRKBUBBLE		(13)
#define SWH_OVR_TRKBUBBLEBK		(14)
#define SWH_OVR_TRKBUBBLEBOR	(15)

#define SWH_SUB_HIGHLIGHTED		(0)
#define SWH_SUB_NONHIGHLIGHTED	(1)

#define SWH_CHP_HIGHLIGHTED		(0)
#define SWH_CHP_NONHIGHLIGHTED	(1)
#define SWH_CHP_BACKGROUND		(2)
#define SWH_CHP_PAGEBORDER		(3)
#define SWH_CHP_TITLE			(4)

#define SWH_IOVR_IMGBORDER		(0)

#define SWH_ES_BLURBORDER		(0)
#define SWH_ES_TEXT				(1)
#define SWH_ES_TEXTBK			(2)

#define SWH_FB_LINK				(0)
#define SWH_FB_DIRS				(1)
#define SWH_FB_FILES			(2)
#define SWH_FB_MODULE			(3)
#define SWH_FB_IHIGHLIGHT		(4)
#define SWH_FB_PATH				(5)
#define SWH_FB_SELECTEDFILE		(6)
#define SWH_FB_FILESIZE			(7)
#define SWH_FB_EMPTYLOCTEXT		(8)

#define SWH_META_PAGEBORDER		(0)
#define SWH_META_METANAME		(1)
#define SWH_META_METAVALUE		(2)

#define SWH_EPG_TEXT			(0)
#define SWH_EPG_HIGHLIGHTED		(1)
#define SWH_EPG_NONHIGHLIGHTED	(2)
#define SWH_EPG_BLURBORDER		(3)


#define SHADOW_BLACK			1
#define SHADOW_BLUE				2
#define SHADOW_GREEN			3




//#define SKINDEFAULTA 		 "GlassLite320"
//#define SKINDEFAULTW		L"GlassLite320"
//#define SKINDEFAULTA 		 "GlassLite480"
//#define SKINDEFAULTW		L"GlassLite480"
#define SKINDEFAULTA 		 "FlatLite800"
#define SKINDEFAULTW		L"FlatLite800"


#if (LIBVLC_VERSION_MAJOR < 2)
#if 1
#define GOOMREZMODES \
	{"800","480"},\
	{"400","240"},\
	{"224","134"}
#else
#define GOOMREZMODES \
	{"480","272"},\
	{"320","180"},\
	{"160","90"}
/*	{"320","240"},\
	{"224","168"},\*/
#endif
#endif


enum _PAGE
{
	PAGE_BASEID = 100,
	PAGE_NONE = PAGE_BASEID,	/* video */

	PAGE_VIDEO = PAGE_NONE,
	PAGE_OVERLAY,		// 101		/* playback control icon overlay */
	PAGE_HOME,			// 102
	PAGE_PLY_SHELF,			// horizontal 'shelf'
	PAGE_PLY_TV,			// tree view playlist
	PAGE_PLY_PANEL,
	PAGE_PLY_PANE,
	PAGE_PLY_FLAT,			// flat playlist (plm) view
	PAGE_EXP_PANEL,
	PAGE_FILE_PANE,
	PAGE_CLOCK,
	PAGE_CFG,
	PAGE_SUB,
	PAGE_IMGOVR,
	PAGE_CHAPTERS,
	PAGE_TEXTOVERLAY,
	PAGE_EXIT,
	PAGE_META,
	PAGE_MEDIASTATS,
	PAGE_EPG,
	PAGE_ES,
	PAGE_TRANSFORM,
//#if (LIBVLC_VERSION_MAJOR >= 2 && LIBVLC_VERSION_MINOR >= 1)
	PAGE_EQ,
	PAGE_HOTKEYS,
//#endif

	PAGE_SEARCH,

	PAGE_ALARM,
	PAGE_TETRIS,
	PAGE_TASKMAN,
	PAGE_TCX,

#if (ENABLE_ANTPLUS)
	PAGE_ANTPLUS,
#endif

	PAGE_VKEYBOARD,
	PAGE_IMGPANE,
	
	PAGE_TOTAL = (PAGE_IMGPANE - PAGE_BASEID)+1,
	PAGE_DEFAULT = PAGE_HOME
};


// video overlay control
enum _VOVR
{
	VBUTTON_PRETRACK,
	VBUTTON_PLAY,
	VBUTTON_PAUSE,
	VBUTTON_STOP,
	VBUTTON_NEXTTRACK,
	VBUTTON_VOLUME,
	VBUTTON_VOLUMEBASE,
	VBUTTON_EPG_PROGRAMMES,
	VBUTTON_CHAPPREV,
	VBUTTON_CHAPNEXT,
	VBUTTON_TIMESTMP,
	VBUTTON_HOME,
	VBUTTON_MHOOK,
	VBUTTON_TOUCHPAD,	// touch pad area in centre of page. used
	VBUTTON_VOLUMEMODE,
	VBUTTON_TOTAL
};

enum _ant
{
	ANTPLUS_FIND,
	ANTPLUS_START,
	ANTPLUS_STOP,
	ANTPLUS_CLEAR,
	ANTPLUS_SHEETFOCUS,
	ANTPLUS_CLOSE,
	ANTPLUS_TOTAL
};

enum _chap
{
	CHPBUTTON_LEFT,
	CHPBUTTON_RIGHT,
	CHPBUTTON_BACK,
	CHPBUTTON_TOTAL
};

enum _epg
{
	EPGBUTTON_GENPLY,
	EPGBUTTON_GUIDE,
	EPGBUTTON_CHANNELS,
	EPGBUTTON_META,
	EPGBUTTON_LEFT,
	EPGBUTTON_RIGHT,
	EPGBUTTON_BACK,
	EPGBUTTON_TOTAL
};

enum _streaminfo
{
	SBUTTON_PREV,
	SBUTTON_NEXT,
	SBUTTON_ATRACK,
	SBUTTON_VTRACK,
	SBUTTON_SUBS,
	SBUTTON_TOTAL
};

enum _exit
{
	EXITBUTTON_YES,
	EXITBUTTON_NO,
	EXITBUTTON_GOIDLE,
	EXITBUTTON_TOTAL
};

enum _spl
{
	SPLBUTTON_BACK,
	SPLBUTTON_TOTAL
};

enum _eq
{
	EQ_RESET,		// reset current preset to default
	EQ_PRESET,		// cycle presets
	EQ_BACK,
	EQ_TOTAL
};

enum _alb
{
	ALBBUTTON_BACK,
	ALBBUTTON_PLAYLISTBACK,
	ALBBUTTON_MENUOPEN,
	ALBBUTTON_MENUOVERLAY,		// invisible overlay for closing panel
	ALBBUTTON_GOTOPLAYING,		// jump to playlist of currently playing track
	ALBBUTTON_PREV,
	ALBBUTTON_PLAY,
	ALBBUTTON_PAUSE,
	ALBBUTTON_STOP,
	ALBBUTTON_NEXT,
	ALBBUTTON_META,
	ALBBUTTON_TOTAL
};

enum _vkey
{
	VKEY_CLOSE,
	VKEY_CB_PASTE,		// paste clipboard contents to buffer
	VKEY_CB_COPY,		// copy buffer to clipboard
	VKEY_UNDO,
	VKEY_CARET_LEFT,
	VKEY_CARET_RIGHT,
	VKEY_CARET_START,
	VKEY_CARET_END,
	VKEY_TOTAL
};

enum _filepane
{
	FILEPANE_SHOWEXT,
	FILEPANE_REFRESH,
	FILEPANE_BACK,
	FILEPANE_IMPORT,				// toggle pane mode 
	FILEPANE_IMPORT_CONTENTS,		// additem via drag drop - only
	FILEPANE_IMPORT_TRACK,			// add single item only
	FILEPANE_TOTAL
};

enum _imgpane
{
	IMGPANE_NAMETOGGLE,
	IMGPANE_BACK,
	IMGPANE_TOTAL
};

enum _plytv
{
	PLYTV_RELOAD,
	PLYTV_EXPAND_OPENALL,
	PLYTV_EXPAND_CLOSEALL,
	PLYTV_SCROLLUP,
	PLYTV_SCROLLDOWN,
	PLYTV_DELETE,
	PLYTV_BACK,
	PLYTV_LOCKED,
	PLYTV_UNLOCKED,
	PLYTV_RENAME,		// enter rename / set title of track mode

	PLYTV_TOTAL
};

#define ABUTTON_MHOOK		PAGE_TOTAL
#define ABUTTON_DRAGRECT	(ABUTTON_MHOOK+1)
#define ABUTTON_MEDIASTATS	(ABUTTON_MHOOK+2)
#define ABUTTON_CONSOLE		(ABUTTON_MHOOK+3)

enum _imageoverlay
{
	IBUTTON_ROTATE,
	IBUTTON_PREV,
	IBUTTON_NEXT,
	IBUTTON_TOTAL
};

enum _meta
{
	METABUTTON_CLOSE,
	METABUTTON_UP,
	METABUTTON_DOWN,
	METABUTTON_LEFT,
	METABUTTON_RIGHT,
	METABUTTON_TOTAL
};

enum _transform
{
	TF_RESET,
	TF_CLOSE,
	TF_TOTAL
};


enum _vis
{
	VIS_DISABLED,
	VIS_VUMETER,
	VIS_SMETER,
	VIS_PINEAPPLE,
	VIS_SPECTRUM,
	VIS_SCOPE,
	VIS_GOOM_Q3,
	VIS_GOOM_Q2,
	VIS_GOOM_Q1,
	VIS_TOTAL
};

enum _timers
{
	TIMER_NEWTRACKVARS1,		//
	TIMER_NEWTRACKVARS2,		//
	TIMER_NEWTRACKVARS3,		// internal playback control
	TIMER_GETTRACKVARDELAYED,
	TIMER_GOTONEXTTRACK,
	TIMER_PLAYTRACK,			
	TIMER_PREVTRACK,			
	TIMER_NEXTTRACK,
	TIMER_FASTFORWARD,
	TIMER_REWIND,
	TIMER_PLAYPAUSE,			// toggle play/pause state
	TIMER_PAUSE,				// pause if playing, do nothing otherwise
	TIMER_STOPPLAY,				// stop before playing
	TIMER_PLAY,
	TIMER_STOP,
	TIMER_VOL_APP_UP,
	TIMER_VOL_APP_DN,
	TIMER_VOL_MASTER_UP,
	TIMER_VOL_MASTER_DN,
	TIMER_SHUTDOWN,
	//TIMER_SWITCHVIS,

	TIMER_ARTCLEANUP,
	TIMER_VLCEVENTS_CLEANUP,

	TIMER_SETIDLEA,
	TIMER_SETIDLEB,
	TIMER_SETIDLEC,
	TIMER_SAVECONFIG,
	TIMER_PATHREGWRITE,

	TIMER_SEARCH_ENDED,
	TIMER_SEARCH_UPDATEHEADER,
	TIMER_SEARCH_METACB,
	TIMER_EXPPAN_REBUILD,
	TIMER_EXPPAN_REBUILDSETPAGE,
	TIMER_CTRL_UPDATETIMESTAMP,
	TIMER_CTRL_DISPLAYVOLRESET,
	TIMER_CTRL_OVERLAYRESET,		// ui control
	TIMER_CTRL_PLAYLISTLBREFRESH,
	TIMER_PLYALB_REFRESH,
	TIMER_PLYPLM_REFRESH,
	TIMER_PLYTV_REFRESH,
	TIMER_PLYPAN_REBUILD,
	TIMER_PLYPAN_REBUILDCLNMETA,
	TIMER_PLYPANE_REFRESH,
	TIMER_EPG_GENDVBPLAYLIST,
	//TIMER_EPG_DISPLAYOSD,
	TIMER_EPG_UPDATE,
	TIMER_ES_UPDATE,
	TIMER_META_UPDATE,
	TIMER_SUB_UPDATE,
	TIMER_CHAPTER_UPDATE,
	TIMER_STATEHELPER,
	TIMER_IMAGECACHEFLUSH,
	TIMER_REG_TRACK_UPDATE,
	TIMER_SBUI_CONNECTED,
	TIMER_SBUI_DISCONNECTED,
	
	TIMER_ALARM,
	TIMER_FLUSH,
	TIMER_TASKBARTITLE_UPDATE,
	
	TIMER_testingonly,
	TIMER_TOTAL
};

enum _IMGC
{
	IMGC_EXIT,
	IMGC_EXITBOX,
	IMGC_POINTER,
	IMGC_NOART_SHELF_SELECTED,
	IMGC_NOART_SHELF_PLAYING,
	IMGC_BGIMAGE,
	IMGC_SHADOW_BLK,
	IMGC_SHADOW_BLU,
	IMGC_SHADOW_GRN,
	IMGC_SWATCH,
	IMGC_CHAPTERMARK,
	IMGC_CHAPTERMARKFILLED,
	IMGC_CLK_HOURSM,
	IMGC_CLK_MINUTESM,
	IMGC_CLK_SECONDSM,
	IMGC_ANT_DIGIT_0,
	IMGC_ANT_DIGIT_1,
	IMGC_ANT_DIGIT_2,
	IMGC_ANT_DIGIT_3,
	IMGC_ANT_DIGIT_4,
	IMGC_ANT_DIGIT_5,
	IMGC_ANT_DIGIT_6,
	IMGC_ANT_DIGIT_7,
	IMGC_ANT_DIGIT_8,
	IMGC_ANT_DIGIT_9,
	IMGC_ANT_COLON,
	IMGC_HRM_SIGNAL,
	IMGC_TOTAL,
};

typedef struct TVLCPLAYER TVLCPLAYER;		// shouldn't be here


typedef struct {
	int x;
	int y;
}TPOINT;

typedef struct{
	int x1;
	int y1;
	int x2;
	int y2;
}T4POINT;

#endif

