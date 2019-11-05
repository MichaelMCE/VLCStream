
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


#ifndef _SETTINGS_H_
#define _SETTINGS_H_


#define CFG_PATHSEPARATOR		"<|>"
#define CFG_COMMENTCOL			(50)
#define CFG_COMMENTCHAR			'\''
#define CFG_SEPARATOR			":"
#define CFG_COMMENT				"'"


enum _cfg_types {
	CFG_INT = 1,	// 32bit
	CFG_INT64,		// 64bit
	CFG_HEX,		// stored as a 64bit unsigned int, written to file in caps (ABC) 
	CFG_STRING,
	CFG_FLOAT,
	CFG_DOUBLE,
	CFG_CHAR,
	CFG_STRLIST,
	CFG_BREAK
};

typedef struct{
	char *key;
	char *value;
}str_keyvalue;

typedef struct {
	char *strings[256];
	int total;
}str_list;

typedef union {
	int32_t	 val32;
	int64_t	 val64;
	uint64_t valu64;		// hex storage
	char* 	valStr;
	float	valFloat;
	double	valDouble;
	char	valChar;
	str_list strList;
}TCFG_VALUE;

typedef struct {
	char *key;
	char type;			// data type

	TCFG_VALUE u;
	
	void *ptr;
	//uint64_t *ptr;
	int hash;
	char *comment;
}TCFGENTRY;



#define V_STR(x)	CFG_STRING, .u.valStr=(x)
#define V_INT32(x)	CFG_INT,	.u.val32=(x)
#define V_INT64(x)	CFG_INT64,	.u.val64=(x)
#define V_HEX(x)	CFG_HEX,	.u.valu64=(x)
#define V_FLT(x)	CFG_FLOAT,	.u.valFloat=(x)
#define V_DBL(x)	CFG_DOUBLE,	.u.valDouble=(x)
#define V_CHR(x)	CFG_CHAR,	.u.valChar=(x)
#define V_BRK(x)	CFG_BREAK,	.u.val32=(x)


#define V_SLIST1(a)							CFG_STRLIST,\
				.u.strList.total=1,\
				.u.strList.strings[0]=a,\
				.u.strList.strings[1]=NULL
				
#define V_SLIST2(a,b)						CFG_STRLIST,\
				.u.strList.total=2,\
				.u.strList.strings[0]=a,\
				.u.strList.strings[1]=b,\
				.u.strList.strings[2]=NULL
				
#define V_SLIST3(a,b,c)						CFG_STRLIST,\
				.u.strList.total=3,\
				.u.strList.strings[0]=a,\
				.u.strList.strings[1]=b,\
				.u.strList.strings[2]=c,\
				.u.strList.strings[3]=NULL

#define V_SLIST4(a,b,c,d)					CFG_STRLIST,\
				.u.strList.total=4,\
				.u.strList.strings[0]=a,\
				.u.strList.strings[1]=b,\
				.u.strList.strings[2]=c,\
				.u.strList.strings[3]=d,\
				.u.strList.strings[4]=NULL

#define V_SLIST5(a,b,c,d,e)					CFG_STRLIST,\
				.u.strList.total=5,\
				.u.strList.strings[0]=a,\
				.u.strList.strings[1]=b,\
				.u.strList.strings[2]=c,\
				.u.strList.strings[3]=d,\
				.u.strList.strings[4]=e,\
				.u.strList.strings[5]=NULL

#define V_SLIST6(a,b,c,d,e,f)				CFG_STRLIST,\
				.u.strList.total=6,\
				.u.strList.strings[0]=a,\
				.u.strList.strings[1]=b,\
				.u.strList.strings[2]=c,\
				.u.strList.strings[3]=d,\
				.u.strList.strings[4]=e,\
				.u.strList.strings[5]=f,\
				.u.strList.strings[6]=NULL

#define V_SLIST7(a,b,c,d,e,f,g)				CFG_STRLIST,\
				.u.strList.total=7,\
				.u.strList.strings[0]=a,\
				.u.strList.strings[1]=b,\
				.u.strList.strings[2]=c,\
				.u.strList.strings[3]=d,\
				.u.strList.strings[4]=e,\
				.u.strList.strings[5]=f,\
				.u.strList.strings[6]=g,\
				.u.strList.strings[7]=NULL

#define V_SLIST8(a,b,c,d,e,f,g,h)			CFG_STRLIST,\
				.u.strList.total=8,\
				.u.strList.strings[0]=a,\
				.u.strList.strings[1]=b,\
				.u.strList.strings[2]=c,\
				.u.strList.strings[3]=d,\
				.u.strList.strings[4]=e,\
				.u.strList.strings[5]=f,\
				.u.strList.strings[6]=g,\
				.u.strList.strings[7]=h,\
				.u.strList.strings[8]=NULL

#define V_SLIST9(a,b,c,d,e,f,g,h,i)			CFG_STRLIST,\
				.u.strList.total=9,\
				.u.strList.strings[0]=a,\
				.u.strList.strings[1]=b,\
				.u.strList.strings[2]=c,\
				.u.strList.strings[3]=d,\
				.u.strList.strings[4]=e,\
				.u.strList.strings[5]=f,\
				.u.strList.strings[6]=g,\
				.u.strList.strings[7]=h,\
				.u.strList.strings[8]=i,\
				.u.strList.strings[9]=NULL
			
#define V_SLIST10(a,b,c,d,e,f,g,h,i,j)		CFG_STRLIST,\
				.u.strList.total=10,\
				.u.strList.strings[0]=a,\
				.u.strList.strings[1]=b,\
				.u.strList.strings[2]=c,\
				.u.strList.strings[3]=d,\
				.u.strList.strings[4]=e,\
				.u.strList.strings[5]=f,\
				.u.strList.strings[6]=g,\
				.u.strList.strings[7]=h,\
				.u.strList.strings[8]=i,\
				.u.strList.strings[9]=j,\
				.u.strList.strings[10]=NULL
				
#define V_SLIST11(a,b,c,d,e,f,g,h,i,j,k)	CFG_STRLIST,\
				.u.strList.total=11,\
				.u.strList.strings[0]=a,\
				.u.strList.strings[1]=b,\
				.u.strList.strings[2]=c,\
				.u.strList.strings[3]=d,\
				.u.strList.strings[4]=e,\
				.u.strList.strings[5]=f,\
				.u.strList.strings[6]=g,\
				.u.strList.strings[7]=h,\
				.u.strList.strings[8]=i,\
				.u.strList.strings[9]=j,\
				.u.strList.strings[10]=k,\
				.u.strList.strings[11]=NULL

#define V_SLIST12(a,b,c,d,e,f,g,h,i,j,k,l)	CFG_STRLIST,\
				.u.strList.total=12,\
				.u.strList.strings[0]=a,\
				.u.strList.strings[1]=b,\
				.u.strList.strings[2]=c,\
				.u.strList.strings[3]=d,\
				.u.strList.strings[4]=e,\
				.u.strList.strings[5]=f,\
				.u.strList.strings[6]=g,\
				.u.strList.strings[7]=h,\
				.u.strList.strings[8]=i,\
				.u.strList.strings[9]=j,\
				.u.strList.strings[10]=k,\
				.u.strList.strings[11]=l,\
				.u.strList.strings[12]=NULL



typedef struct{
	int visual;
	int showStats;
	int idleTimeout;
	double idleFps;
	int overlayPeriod;
	int randomTrack;
	int runCount;
}TCFG_GENERAL;

typedef struct{
	struct{
		int rowSpace;
		int columnSpace;
		int btnsPerRow;
		int autoExpand;
	}layout;
	
	int showKeypad;
	int showAntplus;
	int showTcx;
	int hotkeysAlwaysAccessible;
}TCFG_HOME;

typedef struct{
	int last;
	int changeDelta;
	char *digits;
}TCFG_VOLUME;

typedef struct{
	char *folder;		// must be within SKINDROOT
	char *swatch;
	str_list bgImageList;
}TCFG_SKIN;

typedef struct{
	char *filterBy;
	char *sortBy;
	int showFileSize;
	int showRemotePath;
	int showRemoteComputer;
	int showLocalComputerName;
	int showDriveFreeSpace;
	str_list shortcutList;
	str_list moduleList;
}TCFG_BROWSER;

typedef struct{
	char cursor;
	char console;
	int showNames;
	int localEnabled;
	int globalEnabled;
		
	str_list key;
}TCFG_HOTKEYS;

typedef struct{
	int drawTrackbar;
}TCFG_META;

typedef struct{
	int playlist;
	int track;
	uint64_t hash;
}TCFG_LASTTRACK;

typedef struct{
	int searchDepth;
	int maxWidth;
	int maxHeight;
	int threads;
	int panelImgSize;
}TCFG_ARTWORK;

typedef struct{
	char *device;
	int width;
	int height;
	int backlight;
}TCFG_DISPLAY;

typedef struct{
	double preamp;
	int preset;
	str_list freq;
}TCFG_EQ;

typedef struct{
	double rotate;
	double scale;
	int blur;
	int pixelize;
	
	double brightness;
	double contrast;
	double saturation;
	double gamma;
	
	int rotateOp;
	int scaleOp;
}TCFG_VFILTER;

typedef struct{
	int desync;
	int visuals;
}TCFG_AUDIO;

typedef struct{
	int delay;
}TCFG_SUBTITLE;

typedef struct{
	TCFG_VFILTER filter;
	TCFG_SUBTITLE subtitle;
	int swapRB;
	struct{
		char *preset;
		double ratio;
		int x;
		int y;
		int width;
		int height;
		int clean;
	}aspect;
}TCFG_VIDEO;

typedef struct{
	int killRzDKManagerOnConnectFailRetry;
	str_list dkCfg;
}TCFG_SBUI;

typedef struct{
	TCFG_SBUI sbui;
	struct {
		int enableCtrlButtons;
		int restrictSize;
	}virtual;
}TCFG_DEVICE;


typedef struct{
	int deviceVID;
	int devicePID;
	int deviceIndex;
	int sensorId;
	char *deviceKey;
	int activateOnInsertion;
	int enableOverlay;
	char *digits;
}TCFG_HRM;

typedef struct{
	int font;
	uint64_t foreColour;
	int foreAlpha;
	uint64_t backColour;
	int backRadius;
	int backAlpha;
	int clearUnused;
}TCLK_SBDATE_CFG;

typedef struct{
	char *type;
	int bfFaceCx;
	int bfFaceCy;
	char *digitalDigits;
	int digitalCharOverlap;

	struct {	
		TCLK_SBDATE_CFG	analogue;
		TCLK_SBDATE_CFG	digital;
		TCLK_SBDATE_CFG	boxdigital;
		TCLK_SBDATE_CFG	butterfly;
		TCLK_SBDATE_CFG	polar;
		TCLK_SBDATE_CFG	predator;
	}date;
}TCFG_CLOCK;

typedef struct{
	int status;
	char *time;
	char *period;
	str_list days;
	struct{
		int mode;
		struct{
			char *title;
			uint64_t uid;
			int trkNo;
			int volume;
		}track;
		struct{
			char *path8;		// complete path to process (utf8)
		}starttask;
		struct{
			int pid;			// if exists, stop this task, else
			char *module;		// kill all processes with this module path
		}endtask;
		struct{
			
		}beep;

		struct{
			uint64_t colour1;
			uint64_t colour2;
			int period;			// ms delay between loop
			int repeat;			// repeat n times
		}flash;
	}action;
}TCFG_ALARM;

typedef struct{
	char *extVlcArguments;		// when diverting playback to vlc
	char *startVlcArguments;	// launching vlc only
	int stopLocalPlayback;
}TCFG_VLC;

typedef struct{
	int track;
	int title;
	int artist;
	int album;
	int description;
	int path;
}vlcs_tb_strings;

typedef struct{
	uint64_t fore;
	uint64_t back;
	int bkMode;		// 1:transparent, 2:opaque
}vlcs_tb_colour;

typedef struct{
	char *name;
	int width;
	int height;
	int point;
	int weight;
	int quality;
	HANDLE hfontObj;
}vlcs_tb_font;

typedef struct{
	int enabled;
	char *tbName;
	T2POINT pos;
	vlcs_tb_colour colour;
	vlcs_tb_font font;
	vlcs_tb_strings string;
}TCFG_TASKBAR;

typedef struct{
	int enabled;
	int tipsEnabled;
	int infoEnabled;
	
	int persistantMenu;
	int showPlaylistTrackNo;
	int showPlaylistTrackLength;
	int showPlaylistUID;
	int showDvbPid;
	int maxTextLength;
}TCFG_SYSTRAY;

typedef struct{
	double sigma;
	double rho;
	double expMult;
	double rate;
	double spacing;
	
	double artOpacityMult;
	double artScaleMult;
	
	double sliderX;
	double sliderY;
	double sliderW;
	double titleY;
	double textVSpace;
}TCFG_SHELF;

typedef struct{
	int metaDepth;
	int ignoreCase;
}TCFG_SEARCH;

typedef struct{
	struct{
		int enable;
		int mode;
		str_list colourList;
	}cpugraph;
	
	struct {
		int rows;
		int offset;
	}layout;
}TCFG_TASKMAN;

typedef struct{
	char *file;
	double scaleMultiplier;
	double scaleInitial;
	uint64_t backgroundColour;
	str_list colourList;
}TCFG_TCX;


typedef struct {
	TCFG_GENERAL	general;
	TCFG_HOME		home;
	TCFG_SKIN		skin;
	TCFG_VOLUME		volume;
	TCFG_BROWSER	browser;
	TCFG_HOTKEYS	hotkeys;
	TCFG_LASTTRACK	lasttrack;
	TCFG_ARTWORK	artwork;
	TCFG_DISPLAY	display;
	TCFG_META		meta;
	TCFG_EQ			eq;
	TCFG_VIDEO		video;
	TCFG_AUDIO		audio;
	TCFG_DEVICE		device;
	TCFG_HRM		hrm;
	TCFG_CLOCK		clock;
	TCFG_ALARM		alarm;
	TCFG_VLC		vlc;
	TCFG_TASKBAR	taskbar;
	TCFG_SYSTRAY	systray;
	TCFG_SHELF		album;
	TCFG_SHELF		plm;
	TCFG_SEARCH		search;
	TCFG_TASKMAN	taskman;
	TCFG_TCX		tcx;
	
	TCFGENTRY		**config;
}TSETTINGS;





TCFGENTRY **cfg_configCreate (TSETTINGS *cfg);
void cfg_configFree (TCFGENTRY **config);

int settingsSave (TSETTINGS *cfg, const wchar_t *name);
int settingsLoad (TSETTINGS *cfg, const wchar_t *name);

int cfg_configRead (TCFGENTRY **config, const wchar_t *name);
int cfg_configWrite (TCFGENTRY **config, const wchar_t *name);
void cfg_configApplyDefaults (TCFGENTRY **config);
void cfg_configDump (TCFGENTRY **config);

int cfg_keySet (TCFGENTRY **config, const char *key, void *value);
int cfg_keyGet (TCFGENTRY **config, const char *key, void *value);
int cfg_keyGetW (TCFGENTRY **config, const char *key, void *value);

int settingsSet (TVLCPLAYER *vp, const char *key, void *value);
int settingsGet (TVLCPLAYER *vp, const char *key, void *value);
int settingsGetW (TVLCPLAYER *vp, const char *key, void *value);

char *settingsGetStr (TVLCPLAYER *vp, const char *key);
wchar_t *settingsGetStrW (TVLCPLAYER *vp, const char *key);

str_list *cfg_configStrListNew (const int total);
void cfg_configStrListFree (str_list *strList);
char *cfg_configStrListItem (str_list *strList, const int index);

int cfg_commentSet (TCFGENTRY **config, char *key, char *comment);
char *cfg_commentGet (TCFGENTRY **config, char *key);

TCFGENTRY **cfg_configDup (const TCFGENTRY *config_cfg);

TCFGENTRY *cfg_keyFind (TCFGENTRY **config, const char *key);


#endif

