
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



#ifndef _CFG_H_
#define _CFG_H_


enum _cfgBtnLists
{
	CFG_BTNS_MISC,
	//CFG_BTNS_VISUALS,
	
#if ENABLE_BRIGHTNESS
	CFG_BTNS_BRIGHTNESS,		// needs icons created
#error	needs icons created
#endif

	CFG_BTNS_TOTAL
};

enum _btnMisc
{
	BTN_CFG_MISC_WRITEPLAYLIST,
	BTN_CFG_MISC_WRITEATTACH,
	BTN_CFG_MISC_ABOUT,
	BTN_CFG_MISC_CLOSE,
	BTN_CFG_MISC_SKIN,
	
	BTN_CFG_MISC_TOTAL
};

enum _btnAr
{
	BTN_CFG_AR_AUTO,
	BTN_CFG_AR_177,
	BTN_CFG_AR_155,
	BTN_CFG_AR_143,
	BTN_CFG_AR_133,
	BTN_CFG_AR_125,
	BTN_CFG_AR_122,
	BTN_CFG_AR_15,
	BTN_CFG_AR_16,
	BTN_CFG_AR_167,
	BTN_CFG_AR_185,
	BTN_CFG_AR_220,
	BTN_CFG_AR_233,
	BTN_CFG_AR_240,
	BTN_CFG_AR_CUSTOM,
	
	BTN_CFG_AR_TOTAL
};
/*
enum _btnVis
{
	BTN_CFG_VIS_OFF,
	BTN_CFG_VIS_DISABLED,
	BTN_CFG_VIS_VUMETER,
	BTN_CFG_VIS_SMETER,
	BTN_CFG_VIS_PINEAPPLE,
	BTN_CFG_VIS_SPECTRUM,
	BTN_CFG_VIS_SCOPE,
	BTN_CFG_VIS_GOOM_Q3,
	BTN_CFG_VIS_GOOM_Q2,
	BTN_CFG_VIS_GOOM_Q1,
	
	BTN_CFG_VIS_TOTAL
};
*/
enum _btnBl
{
	BTN_CFG_BRN_0,
	BTN_CFG_BRN_10,
	BTN_CFG_BRN_20,
	BTN_CFG_BRN_30,
	BTN_CFG_BRN_40,
	BTN_CFG_BRN_50,
	BTN_CFG_BRN_60,
	BTN_CFG_BRN_70,
	BTN_CFG_BRN_80,
	BTN_CFG_BRN_90,
	BTN_CFG_BRN_100,
	
	BTN_CFG_BRN_TOTAL
};

enum _btnClk
{
	BTN_CFG_CLK_ANALOGUE,
	BTN_CFG_CLK_DIGITAL,
	BTN_CFG_CLK_BOXDIGITAL,
	BTN_CFG_CLK_BUTTERFLY,
	BTN_CFG_CLK_POLAR,
	BTN_CFG_CLK_PREDATOR,
	
	BTN_CFG_CLK_TOTAL
};

enum _btnStats
{
	BTN_CFG_STATS_ON,
	BTN_CFG_STATS_OFF,
	
	BTN_CFG_STATS_TOTAL
};

enum _btnSwapRB
{
	BTN_CFG_SWAPRB_ON,
	BTN_CFG_SWAPRB_OFF,
	
	BTN_CFG_SWAPRB_TOTAL
};

enum _btnPadCtrl
{
	BTN_CFG_PADCTRL_OFF,	// OS has control
	BTN_CFG_PADCTRL_ON,		// this app has control
	
	BTN_CFG_PADCTRL_TOTAL
};

typedef struct{
	TLABEL *label;
	int     labelId;
	int     strId;
	int 	value;		// value, mode, state, etc..
}TCFGBTN;

typedef struct{
	TPAGE2COM *com;
	
	TCCBUTTONS *btns[BTN_CFG_MISC_TOTAL];
	
	// Aspect ratio mode
	TLABEL *aspectLabel;
	int     aspectLabelId;
	int     aspectStrId;
	int 	aspectRatio;

	// Clock type
	TLABEL *clockLabel;
	int     clockLabelId;
	int     clockStrId;
	int 	clockType;
	
	// Show stats mode
	TLABEL *statsLabel;
	int     statsLabelId;
	int     statsStrId;
	int		statsMode;
	

	// Swap red and blue colour components
	TLABEL *swaprbLabel;
	int     swaprbLabelId;
	int     swaprbStrId;
	int		swaprbMode;
	
	// who gets pad focus, this or OS
	TLABEL *padctrlLabel;
	int     padctrlLabelId;
	int     padctrlStrId;
	//int		padctrlMode;
	
	TCFGBTN	lbls[BTN_CFG_MISC_TOTAL];
}TCFG;


int page_cfgCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);



void reloadSkin (TVLCPLAYER *vp, const int randBack);
void setRBSwap (TVLCPLAYER *vp, int state);
//void setVis (TVLCPLAYER *vp, int visButton);
void setPadControl (TVLCPLAYER *vp, const int mode);
void setAR (TVLCPLAYER *vp, const int arButton);
void setClockType (TVLCPLAYER *vp, const int type);
void setBrightness (TVLCPLAYER *vp, int arButton);
void setShowStats (TVLCPLAYER *vp, int state);
int getShowStatsState (TVLCPLAYER *vp);

int clockStringToType (const char *str);
void cfgAttachmentsSetCount (TVLCPLAYER *vp, const int count);

int getPadControl (TVLCPLAYER *vp);


#endif

