
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



#include "common.h"






typedef struct{
	int yRenderOffset;	// shouldn't be required once TLABEL text render is fixed
	char *name;
}TLABELSTRINGS;


static const TLABELSTRINGS cfgMisc[] = {
	{0, "Write m3u8"},
	{0, "Write attachments"},
	{0, "About"},
	{0, "Close"},
	{0, ""}
};
	  	
static const TLABELSTRINGS cfgAspect[] = {
	{9, "Aspect Ratio: Auto"},
	{9, "Aspect Ratio: 1.77:1 (16:9)"},
	{9, "Aspect Ratio: 1.55:1 (14:9)"},
	{9, "Aspect Ratio: 1.43:1 (43:30)"},
	{9, "Aspect Ratio: 1.33:1 (4:3)"},
	{9, "Aspect Ratio: 1.25:1 (5:4)"},
	{9, "Aspect Ratio: 1.22:1 (22:18)"},
	{9, "Aspect Ratio: 1.5:1 (3:2)"},
	{9, "Aspect Ratio: 1.6:1 (16:10)"},
	{9, "Aspect Ratio: 1.67:1 (5:3)"},
	{9, "Aspect Ratio: 1.85:1 (13:7)"},
	{9, "Aspect Ratio: 2.20:1 (11:5)"},
	{9, "Aspect Ratio: 2.33:1 (47:20)"},
	{9, "Aspect Ratio: 2.40:1 (12:5)"},
	{9, "Aspect Ratio: Custom"},
	{0, ""}
};


static const TLABELSTRINGS cfgClock[] = {
	{9, "Clock: Analogue"},
	{9, "Clock: Digital"},
	{9, "Clock: Box Digital"},
	{9, "Clock: Buffterfly"},
	{9, "Clock: Polar"},
	{9, "Clock: Predator"},
	{0, ""}
};

static const TLABELSTRINGS cfgStats[] = {
	{9, "Show Stats: Off"},
	{9, "Show Stats: On"},
	{0, ""}
};

static const TLABELSTRINGS cfgSwapRB[] = {
	{9, "Swap R/B: Off"},
	{9, "Swap R/B: On"},
	{0, ""}
};

static const TLABELSTRINGS cfgSbuiGesture[] = {
	{9, "Trackpad: OS"},
	{9, "Trackpad: App."},
	{0, ""}
};


static inline void cfgSetPadCtrlLabel (TCFG *cfg, const int mode)
{
	labelItemPositionSet(cfg->padctrlLabel, cfg->padctrlStrId, 0, cfgSbuiGesture[mode].yRenderOffset);
	labelStringSet(cfg->padctrlLabel, cfg->padctrlStrId, cfgSbuiGesture[mode].name);
}

static inline void cfgSetPadCtrlMode (TCFG *cfg, const int mode)
{
	cfg->com->vp->gui.padctrlMode = mode;
	cfgSetPadCtrlLabel(cfg, mode);
}

static inline void cfgSetSwapRBLabel (TCFG *cfg, const int mode)
{
	labelItemPositionSet(cfg->swaprbLabel, cfg->swaprbStrId, 0, cfgSwapRB[mode].yRenderOffset);
	labelStringSet(cfg->swaprbLabel, cfg->swaprbStrId, cfgSwapRB[mode].name);
}

void cfgSetSwapRBMode (TCFG *cfg, const int mode)
{
	cfg->swaprbMode = mode;
	cfgSetSwapRBLabel(cfg, cfg->swaprbMode);
}

static inline void cfgSetStatsLabel (TCFG *cfg, const int mode)
{
	labelItemPositionSet(cfg->statsLabel, cfg->statsStrId, 0, cfgStats[mode].yRenderOffset);
	labelStringSet(cfg->statsLabel, cfg->statsStrId, cfgStats[mode].name);
}

void cfgSetStatsMode (TCFG *cfg, const int mode)
{
	cfg->statsMode = mode;
	cfgSetStatsLabel(cfg, cfg->statsMode);
}

static inline void cfgSetClockLabel (TCFG *cfg, const int clk)
{
	labelItemPositionSet(cfg->clockLabel, cfg->clockStrId, 0, cfgClock[clk].yRenderOffset);
	labelStringSet(cfg->clockLabel, cfg->clockStrId, cfgClock[clk].name);
}

void cfgSetClockType (TCFG *cfg, const int clk)
{
	cfg->clockType = clk;
	cfgSetClockLabel(cfg, cfg->clockType);
}


static inline void cfgSetAspectLabel (TCFG *cfg, const int ar)
{
	labelItemPositionSet(cfg->aspectLabel, cfg->aspectStrId, 0, cfgAspect[ar].yRenderOffset);
	labelStringSet(cfg->aspectLabel, cfg->aspectStrId, cfgAspect[ar].name);
}

void cfgSetAspectRatio (TCFG *cfg, const int ar)
{
	cfg->aspectRatio = ar;
	cfgSetAspectLabel(cfg, cfg->aspectRatio);
}

static inline int64_t cfg_label_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER/* || msg == CC_MSG_INPUT*/) return 1;

	TLABEL *label = (TLABEL*)object;
	
	//printf("exp_label_cb: %i, %i %i\n", msg, (int)data1, (int)data2);
	
	if (msg == LABEL_MSG_TEXT_SELECTED_PRESS || msg == LABEL_MSG_BASE_SELECTED_PRESS){
		//TTOUCHCOORD *pos = (TTOUCHCOORD*)dataPtr;
		//printf("exp_label_cb: %i, %i %.3f\n", msg, pos->pen, pos->dt);
		
		if (1/*!pos->pen && pos->dt > 100*/){		// press down
			TCFG *cfg = ccGetUserData(label);
			
			if (label->id == cfg->aspectLabelId){
				if (++cfg->aspectRatio >= BTN_CFG_AR_TOTAL)
					cfg->aspectRatio = BTN_CFG_AR_AUTO;
				//printf("cfg_label_cb: aspect %i\n", cfg->aspectRatio);
				
				cfgSetAspectLabel(cfg, cfg->aspectRatio);
				setAR(cfg->com->vp, cfg->aspectRatio);
				renderSignalUpdate(label->cc->vp);
				
			}else if (label->id == cfg->clockLabelId){
				if (++cfg->clockType >= BTN_CFG_CLK_TOTAL)
					cfg->clockType = BTN_CFG_CLK_ANALOGUE;
				//printf("cfg_label_cb: clock %i\n", cfg->clockType);
				
				cfgSetClockLabel(cfg, cfg->clockType);
				setClockType(label->cc->vp, cfg->clockType);
				renderSignalUpdate(label->cc->vp);

			}else if (label->id == cfg->statsLabelId){
				if (++cfg->statsMode >= BTN_CFG_STATS_TOTAL)
					cfg->statsMode = BTN_CFG_STATS_ON;
				//printf("cfg_label_cb: stats %i\n", cfg->statsMode);
				
				cfgSetStatsLabel(cfg, cfg->statsMode);
				setShowStats(label->cc->vp, cfg->statsMode);
				renderSignalUpdate(label->cc->vp);

			}else if (label->id == cfg->swaprbLabelId){
				if (++cfg->swaprbMode >= BTN_CFG_SWAPRB_TOTAL)
					cfg->swaprbMode = BTN_CFG_SWAPRB_ON;
				//printf("cfg_label_cb: swaprb %i\n", cfg->swaprbMode);
				
				cfgSetSwapRBLabel(cfg, cfg->swaprbMode);
				setRBSwap(label->cc->vp, cfg->swaprbMode);
				renderSignalUpdate(label->cc->vp);

			}else if (label->id == cfg->padctrlLabelId){
				if (++cfg->com->vp->gui.padctrlMode >= BTN_CFG_PADCTRL_TOTAL)
					cfg->com->vp->gui.padctrlMode = BTN_CFG_PADCTRL_OFF;
			//	printf("cfg_label_cb: padctrl %i\n", vp->gui.padctrlMode);
				
				cfgSetPadCtrlLabel(cfg, cfg->com->vp->gui.padctrlMode);
				setPadControl(label->cc->vp, cfg->com->vp->gui.padctrlMode);
				renderSignalUpdate(label->cc->vp);
			}
			
		}
	}
	return 1;
}

static inline int64_t cfg_misc_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER/* || msg == CC_MSG_INPUT*/) return 1;

	TLABEL *label = (TLABEL*)object;
	
	if (msg == LABEL_MSG_TEXT_SELECTED_PRESS || msg == LABEL_MSG_BASE_SELECTED_PRESS){
		TTOUCHCOORD *pos = (TTOUCHCOORD*)dataPtr;
		//printf("exp_label_cb: %i %i\n", pos->pen, pos->dt);
		
		if (!pos->pen && pos->dt > 100){		// press down
			TCFG *cfg = ccGetUserData(label);
			TVLCPLAYER *vp = (TVLCPLAYER*)label->cc->vp;
			
			if (label->id == cfg->lbls[BTN_CFG_MISC_ABOUT].labelId){
				dbprintf(vp, " ");
	  			printAbout(vp);
	  			
			}else if (label->id == cfg->lbls[BTN_CFG_MISC_CLOSE].labelId){
				page2SetPrevious(page2PageStructGet(vp->pages, PAGE_CFG));
				
			}else if (label->id == cfg->lbls[BTN_CFG_MISC_WRITEPLAYLIST].labelId){
				savePlaylistAsync(vp);
				
			}else if (label->id == cfg->lbls[BTN_CFG_MISC_WRITEATTACH].labelId){
				if (vp->vlc->hasAttachments){
					int totalWritten = vlc_attachmentsSave(vp, vp->vlc);
					dbwprintf(vp, L"%i attachments saved", totalWritten);
				}
			}
			renderSignalUpdate(vp);
		}
	}
	return 1;
}

static inline int64_t btn_misc_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;
		
	TCCBUTTON *btn = (TCCBUTTON*)object;
	int idx = ccGetUserDataInt(btn);
	//printf("cfg btn_misc_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p, %i\n", btn->id, btn->type, msg, (int)data1, (int)data2, dataPtr, idx);

	if (msg == BUTTON_MSG_SELECTED_PRESS){
		//TCCBUTTONS *btns = (TCCBUTTONS*)ccGetUserData(btn);
		TVLCPLAYER *vp = (TVLCPLAYER*)btn->cc->vp;

		switch (idx){
		  case BTN_CFG_MISC_SKIN:
			reloadSkin(vp, 1);
			break;
		};
	}
	return 1;
}


#if ENABLE_BRIGHTNESS
static inline int64_t btn_bl_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;
		
	TCCBUTTON *btn = (TCCBUTTON*)object;
	int idx = ccGetUserDataInt(btn);
	//printf("cfg btn_bl_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p, %i\n", btn->id, btn->type, msg, (int)data1, (int)data2, dataPtr, idx);


	if (msg == BUTTON_MSG_SELECTED_PRESS){
		TCCBUTTONS *btns = (TCCBUTTONS*)ccGetUserData(btn);
		buttonsSetState(btns, 0);
	
		if (buttonsButtonGet(btns, ++idx)){
			buttonsStateSet(btns, idx, 1);
		}else{
			idx = 0;
			buttonsStateSet(btns, idx, 1);
		}
			
		TVLCPLAYER *vp = (TVLCPLAYER*)btn->cc->vp;
		switch (idx){
		  case BTN_CFG_BRN_0:
		  case BTN_CFG_BRN_10:
		  case BTN_CFG_BRN_20:
		  case BTN_CFG_BRN_30:
		  case BTN_CFG_BRN_40:
		  case BTN_CFG_BRN_50:
		  case BTN_CFG_BRN_60:
		  case BTN_CFG_BRN_70:
		  case BTN_CFG_BRN_80:
		  case BTN_CFG_BRN_90:
		  case BTN_CFG_BRN_100:
			setBrightness(vp, idx);
		};
	}
	return 1;
}
#endif

int clockStringToType (const char *str)
{
	if (!stricmp("digital", str)) return BTN_CFG_CLK_DIGITAL;
	else if (!stricmp("analogue", str)) return BTN_CFG_CLK_ANALOGUE;
	else if (!stricmp("butterfly", str)) return BTN_CFG_CLK_BUTTERFLY;
	else if (!stricmp("boxdigital", str)) return BTN_CFG_CLK_BOXDIGITAL;
	else if (!stricmp("polar", str)) return BTN_CFG_CLK_POLAR;
	else if (!stricmp("predator", str)) return BTN_CFG_CLK_PREDATOR;
	else return BTN_CFG_CLK_DIGITAL;
}

void setAR (TVLCPLAYER *vp, int arButton)
{
	vp->ctx.aspect.preset = arButton;
	cfgSetAspectRatio(pageGetPtr(vp, PAGE_CFG),arButton);
}

void setClockType (TVLCPLAYER *vp, int type)
{
	if (type >= BTN_CFG_CLK_TOTAL)
		type = BTN_CFG_CLK_ANALOGUE;
		
	vp->gui.clockType = type;
	cfgSetClockType(pageGetPtr(vp, PAGE_CFG),type);
}

static int skinReloadBackground (TVLCPLAYER *vp, TGUI *gui, const int randBack)
{

	//printf("skinReloadBackground\n");
	
	int ret = 0;

	str_list *strList = NULL;
	settingsGetW(vp, "skin.bgimage.", &strList);
	if (strList){
		gui->skin.bgPathTotal = strList->total;
		
		if (randBack){
			if (++gui->skin.currentIdx >= gui->skin.bgPathTotal)
				gui->skin.currentIdx = 0;
		}

		wchar_t *path = (wchar_t*)cfg_configStrListItem(strList, gui->skin.currentIdx);
		if (path){
			ret = imageManagerImageSetPath(vp->im, gui->image[IMGC_BGIMAGE], path);
			if (ret) dbwprintf(vp, L"%i: '%s'\n", gui->skin.currentIdx, path);
		}

		cfg_configStrListFree(strList);
		my_free(strList);
	}
	return ret;	
}


void reloadSkin (TVLCPLAYER *vp, const int randBack)
{
	//printf("reloadSkin\n");
	
	if (renderLock(vp)){
		if (vp->applState){
			imageManagerFlush(vp->im);
			artManagerFlush(vp->am);
			ccLabelFlushAll(vp->cc);
			//libmylcd_FlushFonts(vp->ml->hw);
			skinReloadBackground(vp, &vp->gui, randBack);
			invalidateShadows(vp->gui.shadow);
			configLoadSwatch(vp);
			if (hasPageBeenAccessed(vp, PAGE_PLY_SHELF))
				invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), -1);
			if (hasPageBeenAccessed(vp, PAGE_PLY_FLAT))
				invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_FLAT), -1);

			imageManagerImageRelease(vp->im, vp->gui.image[IMGC_BGIMAGE]);

			int width = 0, height = 0;
			if (artManagerImageGetMetrics(vp->im, vp->gui.image[IMGC_BGIMAGE], &width, &height)){
				const int w = getFrontBuffer(vp)->width;
				const int h = getFrontBuffer(vp)->height;
				if (width != w || height != h)
					artManagerImageResize(vp->im, vp->gui.image[IMGC_BGIMAGE], w, h);
			}
			vp->gui.skin.bg = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_BGIMAGE]);
		}
		renderUnlock(vp);
	}
}

/*void setVis (TVLCPLAYER *vp, int visButton)
{
	vp->gui.visual = visButton - BTN_CFG_VIS_DISABLED;
}*/

#if ENABLE_BRIGHTNESS
void setBrightness (TVLCPLAYER *vp, int arButton)
{
	if (!vp->ml->enableBrightness) return;

	vp->gui.brightness = arButton;
	setDisplayBrightness(vp, (255.0/100.0)*(float)(vp->gui.brightness*10));
	setDisplayBrightness(vp, (255.0/100.0)*(float)(vp->gui.brightness*10));
}
#endif

int getShowStatsState (TVLCPLAYER *vp)
{
	return vp->gui.drawStats;
}

void setShowStats (TVLCPLAYER *vp, const int mode)
{
	vp->gui.drawStats = mode;
	cfgSetStatsMode(pageGetPtr(vp, PAGE_CFG), mode);
	
	if (mode == BTN_CFG_STATS_ON)
		renderStatsDisable(vp);
	else if (mode == BTN_CFG_STATS_OFF)
		renderStatsEnable(vp);
}

void cfgAttachmentsSetCount (TVLCPLAYER *vp, const int count)
{
	//printf("cfgAttachmentsSetCount %i\n", count);
	
	TCFG *cfg = pageGetPtr(vp, PAGE_CFG);
	vp->vlc->hasAttachments = count;
	
	if (count > 0)
		labelItemEnable(cfg->lbls[BTN_CFG_MISC_WRITEATTACH].label, cfg->lbls[BTN_CFG_MISC_WRITEATTACH].strId);
	else
		labelItemDisable(cfg->lbls[BTN_CFG_MISC_WRITEATTACH].label, cfg->lbls[BTN_CFG_MISC_WRITEATTACH].strId);
}

int getPadControl (TVLCPLAYER *vp)
{
	//TCFG *cfg = pageGetPtr(vp, PAGE_CFG);
	return vp->gui.padctrlMode;
}

void setPadControl (TVLCPLAYER *vp, const int mode)
{
	TCFG *cfg = pageGetPtr(vp, PAGE_CFG);
	if (cfg) cfgSetPadCtrlMode(cfg, mode);
	sbuiCfgSetPadControl(vp, mode);
}

void setRBSwap (TVLCPLAYER *vp, const int mode)
{
	cfgSetSwapRBMode(pageGetPtr(vp, PAGE_CFG), mode);
	vp->vlc->swapColourBits = mode&0x01;
}

int cfgInit (TVLCPLAYER *vp, TFRAME *frame, TCFG *cfg)
{

	const int isSbuiEnabled = isSBUIEnabled(vp);

	int y = 0;

	int tbuttons = 5 + vp->ml->enableBrightness;
	float dy = frame->height/tbuttons;	// 6 buttons per side

	const int fw = frame->width;
	const int fh = frame->height;

	tbuttons = 5 + isSbuiEnabled;
	dy = frame->height/(float)tbuttons;	// 6 buttons per side


	cfg->btns[CFG_BTNS_MISC] = buttonsCreate(vp->cc, PAGE_CFG, BTN_CFG_MISC_TOTAL, btn_misc_cb);
	//cfg->btns[CFG_BTNS_VISUALS] = buttonsCreate(vp->cc, PAGE_CFG, BTN_CFG_VIS_TOTAL, btn_vis_cb);
#if ENABLE_BRIGHTNESS
	cfg->btns[CFG_BTNS_BRIGHTNESS] = buttonsCreate(vp->cc, PAGE_CFG, BTN_CFG_BRN_TOTAL, btn_bl_cb);
#endif

	
	int xOffset = 10;
	int yOffset = fh * (0.13 - (isSbuiEnabled * 0.03));

	cfg->aspectLabel = ccCreate(vp->cc, PAGE_CFG, CC_LABEL, cfg_label_cb, &cfg->aspectLabelId, 480, 59);
	ccSetUserData(cfg->aspectLabel, cfg);
	cfg->aspectStrId = labelTextCreate(cfg->aspectLabel, " ", 0, BROWSER_FONT, 0, 9);
	labelStringRenderFlagsSet(cfg->aspectLabel, cfg->aspectStrId, PF_CLIPDRAW);
	labelRenderFlagsSet(cfg->aspectLabel, LABEL_RENDER_TEXT|LABEL_RENDER_HOVER);
	labelRenderBlurRadiusSet(cfg->aspectLabel, cfg->aspectStrId, 3);
	labelRenderFilterSet(cfg->aspectLabel, cfg->aspectStrId, 2);
	labelRenderFontSet(cfg->aspectLabel, cfg->aspectStrId, CFG_VARNAME_FONT);
	labelRenderColourSet(cfg->aspectLabel, cfg->aspectStrId, 255<<24|COL_WHITE, 255<<24|COL_BLUE_SEA_TINT, 255<<24|COL_BLUE_SEA_TINT);
	cfg->aspectLabel->canDrag = 0;
	ccSetMetrics(cfg->aspectLabel, xOffset, yOffset, -1, -1);
	ccEnable(cfg->aspectLabel);

	yOffset += dy;
	cfg->clockLabel = ccCreate(vp->cc, PAGE_CFG, CC_LABEL, cfg_label_cb, &cfg->clockLabelId, 280, 59);
	ccSetUserData(cfg->clockLabel, cfg);
	cfg->clockStrId = labelTextCreate(cfg->clockLabel, " ", 0, BROWSER_FONT, 0, 9);
	labelStringRenderFlagsSet(cfg->clockLabel, cfg->clockStrId, PF_CLIPDRAW);
	labelRenderFlagsSet(cfg->clockLabel, LABEL_RENDER_TEXT|LABEL_RENDER_HOVER);
	labelRenderBlurRadiusSet(cfg->clockLabel, cfg->clockStrId, 3);
	labelRenderFilterSet(cfg->clockLabel, cfg->clockStrId, 2);
	labelRenderFontSet(cfg->clockLabel, cfg->clockStrId, CFG_VARNAME_FONT);
	labelRenderColourSet(cfg->clockLabel, cfg->clockStrId, 255<<24|COL_WHITE, 255<<24|COL_BLUE_SEA_TINT, 255<<24|COL_BLUE_SEA_TINT);
	cfg->clockLabel->canDrag = 0;
	ccSetMetrics(cfg->clockLabel, xOffset, yOffset, -1, -1);
	ccEnable(cfg->clockLabel);
	
	yOffset += dy;
	cfg->statsLabel = ccCreate(vp->cc, PAGE_CFG, CC_LABEL, cfg_label_cb, &cfg->statsLabelId, 280, 59);
	ccSetUserData(cfg->statsLabel, cfg);
	cfg->statsStrId = labelTextCreate(cfg->statsLabel, " ", 0, BROWSER_FONT, 0, 9);
	labelStringRenderFlagsSet(cfg->statsLabel, cfg->statsStrId, PF_CLIPDRAW);
	labelRenderFlagsSet(cfg->statsLabel, LABEL_RENDER_TEXT|LABEL_RENDER_HOVER);
	labelRenderBlurRadiusSet(cfg->statsLabel, cfg->statsStrId, 3);
	labelRenderFilterSet(cfg->statsLabel, cfg->statsStrId, 2);
	labelRenderFontSet(cfg->statsLabel, cfg->statsStrId, CFG_VARNAME_FONT);
	labelRenderColourSet(cfg->statsLabel, cfg->statsStrId, 255<<24|COL_WHITE, 255<<24|COL_BLUE_SEA_TINT, 255<<24|COL_BLUE_SEA_TINT);
	cfg->statsLabel->canDrag = 0;
	ccSetMetrics(cfg->statsLabel, xOffset, yOffset, -1, -1);
	ccEnable(cfg->statsLabel);

	yOffset += dy;
	cfg->swaprbLabel = ccCreate(vp->cc, PAGE_CFG, CC_LABEL, cfg_label_cb, &cfg->swaprbLabelId, 280, 59);
	ccSetUserData(cfg->swaprbLabel, cfg);
	cfg->swaprbStrId = labelTextCreate(cfg->swaprbLabel, " ", 0, BROWSER_FONT, 0, 9);
	labelStringRenderFlagsSet(cfg->swaprbLabel, cfg->swaprbStrId, PF_CLIPDRAW);
	labelRenderFlagsSet(cfg->swaprbLabel, LABEL_RENDER_TEXT|LABEL_RENDER_HOVER);
	labelRenderBlurRadiusSet(cfg->swaprbLabel, cfg->swaprbStrId, 3);
	labelRenderFilterSet(cfg->swaprbLabel, cfg->swaprbStrId, 2);
	labelRenderFontSet(cfg->swaprbLabel, cfg->swaprbStrId, CFG_VARNAME_FONT);
	labelRenderColourSet(cfg->swaprbLabel, cfg->swaprbStrId, 255<<24|COL_WHITE, 255<<24|COL_BLUE_SEA_TINT, 255<<24|COL_BLUE_SEA_TINT);
	cfg->swaprbLabel->canDrag = 0;
	ccSetMetrics(cfg->swaprbLabel, xOffset, yOffset, -1, -1);
	ccEnable(cfg->swaprbLabel);
	
	yOffset += dy;
	cfg->padctrlLabel = ccCreate(vp->cc, PAGE_CFG, CC_LABEL, cfg_label_cb, &cfg->padctrlLabelId, 280, 59);
	ccSetUserData(cfg->padctrlLabel, cfg);
	cfg->padctrlStrId = labelTextCreate(cfg->padctrlLabel, " ", 0, BROWSER_FONT, 0, 9);
	labelStringRenderFlagsSet(cfg->padctrlLabel, cfg->padctrlStrId, PF_CLIPDRAW);
	labelRenderFlagsSet(cfg->padctrlLabel, LABEL_RENDER_TEXT|LABEL_RENDER_HOVER);
	labelRenderBlurRadiusSet(cfg->padctrlLabel, cfg->padctrlStrId, 3);
	labelRenderFilterSet(cfg->padctrlLabel, cfg->padctrlStrId, 2);
	labelRenderFontSet(cfg->padctrlLabel, cfg->padctrlStrId, CFG_VARNAME_FONT);
	labelRenderColourSet(cfg->padctrlLabel, cfg->padctrlStrId, 255<<24|COL_WHITE, 255<<24|COL_BLUE_SEA_TINT, 255<<24|COL_BLUE_SEA_TINT);
	cfg->padctrlLabel->canDrag = 0;
	ccSetMetrics(cfg->padctrlLabel, xOffset, yOffset, -1, -1);
	
	if (!isSbuiEnabled)
		ccDisable(cfg->padctrlLabel);
	else
		ccEnable(cfg->padctrlLabel);
	
	tbuttons = 5;
	dy = frame->height/(float)tbuttons;	// 6 buttons per side
	yOffset = fh * 0.05;

	TCCBUTTONS *btns = cfg->btns[CFG_BTNS_MISC];
	TCCBUTTON *btn = buttonsCreateButton(btns, L"title.png", NULL, BTN_CFG_MISC_SKIN, 1, 1, 0, 0);
	ccSetPosition(btn, fw-ccGetWidth(btn)-xOffset, y+yOffset);
	
	yOffset += (dy-16);
	for (int i = 0; cfgMisc[i].name[0]; i++){
		TCFGBTN *btn = &cfg->lbls[i];
		TLABEL *label = ccCreate(vp->cc, PAGE_CFG, CC_LABEL, cfg_misc_cb, &btn->labelId, 256, 59);
		btn->label = label;
		
		ccSetUserData(label, cfg);
		btn->strId = labelTextCreate(label, cfgMisc[i].name, 0, BROWSER_FONT, 0, 9);
		labelStringRenderFlagsSet(label, btn->strId, PF_CLIPDRAW);
		labelRenderFlagsSet(label, LABEL_RENDER_TEXT|LABEL_RENDER_HOVER);
		labelRenderBlurRadiusSet(label, btn->strId, 3);
		labelRenderFilterSet(label, btn->strId, 2);
		labelRenderFontSet(label, btn->strId, CFG_VARNAME_FONT);
		labelRenderColourSet(label, btn->strId, 255<<24|COL_WHITE, 255<<24|COL_BLUE_SEA_TINT, 255<<24|COL_BLUE_SEA_TINT);
		label->canDrag = 0;
		
		int width = 0, height = 0;
		labelStringGetMetrics(label, btn->strId, NULL, NULL, &width, &height);
		ccSetMetrics(label, (fw-10) - width, yOffset, width, -1);
		yOffset += dy;
		
		ccEnable(label);
	}
	
	
	/*yOffset += dy;
	btns = cfg->btns[CFG_BTNS_VISUALS];
	buttonsCreateButton(btns, L"config/vdisabled.png", NULL, BTN_CFG_VIS_OFF, 0, 1, xOffset, yOffset);
	buttonsCreateButton(btns, L"config/vdisabled.png", NULL, BTN_CFG_VIS_DISABLED, 1, 1, xOffset, yOffset);
	buttonsCreateButton(btns, L"config/vvumeter.png", NULL, BTN_CFG_VIS_VUMETER, 0, 0, xOffset, yOffset);
	buttonsCreateButton(btns, L"config/vspectrometer.png", NULL, BTN_CFG_VIS_SMETER, 0, 1, xOffset, yOffset);
	buttonsCreateButton(btns, L"config/vpineapple.png", NULL, BTN_CFG_VIS_PINEAPPLE, 0, 1, xOffset, yOffset);
	buttonsCreateButton(btns, L"config/vspectrum.png", NULL, BTN_CFG_VIS_SPECTRUM, 0, 1, xOffset, yOffset);
	buttonsCreateButton(btns, L"config/vscope.png", NULL, BTN_CFG_VIS_SCOPE, 0, 1, xOffset, yOffset);
	buttonsCreateButton(btns, L"config/vgoomq3.png", NULL, BTN_CFG_VIS_GOOM_Q3, 0, 1, xOffset, yOffset);
	buttonsCreateButton(btns, L"config/vgoomq2.png", NULL, BTN_CFG_VIS_GOOM_Q2, 0, 1, xOffset, yOffset);
	buttonsCreateButton(btns, L"config/vgoomq1.png", NULL, BTN_CFG_VIS_GOOM_Q1, 0, 1, xOffset, yOffset);*/
	
#if ENABLE_BRIGHTNESS
	if (vp->ml->enableBrightness){
		yOffset += dy;
		btns = cfg->btns[CFG_BTNS_BRIGHTNESS];
		wchar_t name[64];
		
		for (int i = 0; i < BTN_CFG_BRN_TOTAL; i++){
			__mingw_snwprintf(name, 64, L"config/brn%i.png", i*10);
			buttonsCreateButton(btns, name, NULL, i, 1, 1, xOffset, yOffset);
		}
	}
#endif


	// todo; sync these with actual read config settings
#if ENABLE_BRIGHTNESS
	setBrightness(vp, BTN_CFG_BRN_60);
#endif
	
	
	char *values = NULL;
	settingsGet(vp, "clock.type", &values);
	if (values){
		setClockType(vp, clockStringToType(values));
		my_free(values);
	}
	values = NULL;
	settingsGet(vp, "video.aspect.preset", &values);
	if (values){
		setAR(vp, aspectStrToIdx(values));
		my_free(values);
	}
	
	//setVis(vp, vp->gui.visual + BTN_CFG_VIS_DISABLED);
	setShowStats(vp, vp->gui.drawStats);
	setRBSwap(vp, vp->vlc->swapColourBits);
	if (isSbuiEnabled)
		setPadControl(vp, BTN_CFG_PADCTRL_ON);
	cfgAttachmentsSetCount(vp, vp->vlc->hasAttachments);

	return 1;
}

static inline int page_cfgRender (TCFG *cfg, TVLCPLAYER *vp, TFRAME *frame)
{
	ccRender(cfg->aspectLabel, frame);
	ccRender(cfg->clockLabel, frame);
	ccRender(cfg->statsLabel, frame);
	ccRender(cfg->swaprbLabel, frame);
	ccRender(cfg->padctrlLabel, frame);
	
	for (int i = 0; i < BTN_CFG_MISC_SKIN; i++){
		TCFGBTN *btn = &cfg->lbls[i];
		ccRender(btn->label, frame);
	}
	
	for (int i = 0; i < CFG_BTNS_TOTAL; i++)
		buttonsRenderAll(cfg->btns[i], frame, BUTTONS_RENDER_HOVER);

	return 1;
}

static inline int page_cfgInput (TCFG *cfg, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
  			
	if (msg == PAGE_IN_WHEEL_FORWARD){
		const int x = flags >> 16;
		const int y = flags & 0xFFFF;

		if (ccPositionIsOverlapped(cfg->aspectLabel, x, y)){
			if (--cfg->aspectRatio < 0)
				cfg->aspectRatio = BTN_CFG_AR_TOTAL-1;
				
			cfgSetAspectLabel(cfg, cfg->aspectRatio);
			setAR(vp, cfg->aspectRatio);
			renderSignalUpdate(vp);
		}
	}else if (msg == PAGE_IN_WHEEL_BACK){
		const int x = flags >> 16;
		const int y = flags & 0xFFFF;
		
		if (ccPositionIsOverlapped(cfg->aspectLabel, x, y)){
			if (++cfg->aspectRatio >= BTN_CFG_AR_TOTAL)
				cfg->aspectRatio = BTN_CFG_AR_AUTO;

			cfgSetAspectLabel(cfg, cfg->aspectRatio);
			setAR(vp, cfg->aspectRatio);
			renderSignalUpdate(vp);
		}
	}

	return 1;
}

static inline int page_cfgStartup (TCFG *cfg, TVLCPLAYER *vp, const int width, const int height)
{
	return 1;
}

static inline int page_cfgInitalize (TCFG *cfg, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_CFG);
	return cfgInit(vp, getFrontBuffer(vp), cfg);
}

static inline int page_cfgShutdown (TCFG *cfg, TVLCPLAYER *vp)
{
	ccDelete(cfg->aspectLabel);
	ccDelete(cfg->clockLabel);
	ccDelete(cfg->statsLabel);
	ccDelete(cfg->swaprbLabel);
	ccDelete(cfg->padctrlLabel);
	
	for (int i = 0; i < CFG_BTNS_TOTAL; i++)
		buttonsDeleteAll(cfg->btns[i]);

	return 1;
}

static inline int page_cfgRenderBegin (TCFG *cfg, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	return 1;
}

static inline int page_cfgRenderEnd (TCFG *cfg, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, void *opaquePtr)
{
	lSetRenderEffect(vp->ml->hw, LTR_DEFAULT);
	return 1;
}


int page_cfgCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TCFG *cfg = (TCFG*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_cfgCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_cfgRender(cfg, cfg->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_cfgInput(cfg, cfg->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_START){
		return page_cfgRenderBegin(cfg, cfg->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		return page_cfgRenderEnd(cfg, cfg->com->vp, dataInt1, dataInt2, opaquePtr);

	}else if (msg == PAGE_CTL_STARTUP){
		return page_cfgStartup(cfg, cfg->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_cfgInitalize(cfg, cfg->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_cfgShutdown(cfg, cfg->com->vp);
		
	}
	
	return 1;
}

