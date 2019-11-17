
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
#include <pdh.h>
#include <assert.h>


#define HOME_BTN_TOTAL	(PAGE_TOTAL)
#define HOME_BTN_ID(a)	((a)-PAGE_BASEID)



typedef struct{
	int enabled;
	int page;
	wchar_t *path;			// front face
	wchar_t *pathAlt;		// alt face
	int ptype;
	int animate;
}THOMEBTN;
	
static THOMEBTN homeBtns[] = {
	{1, PAGE_OVERLAY,		L"home/overlay.png", NULL ,1, 0},
	{1, PAGE_EXP_PANEL,		L"home/filepane.png", NULL, 1, 0},
	{1, PAGE_CLOCK,			L"home/clock.png", L"home/empty.png", 1, 0},
	{1, PAGE_CFG,			L"home/cfg.png", NULL, 1, 0},
	{1, PAGE_EXIT,			L"home/quit.png", NULL, 2, 1},
	
	{1, PAGE_PLY_SHELF,		L"home/playlisttree.png", NULL, 1, 0},
	{1, PAGE_PLY_PANE,		L"home/playlistpane.png", NULL, 1, 0},
	{1, PAGE_PLY_PANEL,		L"home/playlistpanel.png", NULL, 1, 0},
	{1, PAGE_PLY_TV,		L"home/playlisttv.png", NULL, 1, 0},
	{1, PAGE_META,			L"home/meta.png", NULL, 3, 1},
	{1, PAGE_PLY_FLAT,		L"home/playlistflat.png", NULL, 1, 0},

	{0, PAGE_CHAPTERS,		L"home/chapters.png", NULL, 1, 0},
	{0, PAGE_SUB,			L"home/subtitles.png", NULL, 1, 0},
	{0, PAGE_EPG,			L"home/dvb.png", NULL, 1, 0},
	{0, PAGE_ES,			L"home/es.png", NULL, 1, 0},
	{0, PAGE_TRANSFORM,		L"home/transform.png", NULL, 1, 0},
	{0, PAGE_EQ,			L"home/eq.png", NULL, 1, 0},
	{1, PAGE_SEARCH,		L"home/search.png", NULL, 1, 0},
	
	{1, PAGE_ALARM,			L"home/alarm.png", NULL, 1, 0},
#if ENABLE_ANTPLUS
	{1, PAGE_ANTPLUS,		L"home/antplus.png", L"home/antplusoff.png", 1, 0},
#endif

	{1, PAGE_TETRIS,		L"home/tetris.png", NULL, 1, 0},
	{1, PAGE_TASKMAN,		L"home/taskman.png", NULL, 1, 0},
	{1, PAGE_TCX,			L"home/tcx.png", NULL, 1, 0},
	
	{0, PAGE_HOTKEYS,		L"home/hotkeysvlc.png", NULL, 1, 0},
		
	{0, PAGE_VKEYBOARD,		L"home/keypad.png", NULL, 1, 0},
	{0, PAGE_FILE_PANE,		L"home/filepane.png", NULL, 1, 0},
	{0, PAGE_IMGPANE,		L"home/stub.png", NULL, 1, 0},
	{0, PAGE_MEDIASTATS,	L"home/stub.png", NULL, 2, 0},
	{0, PAGE_TEXTOVERLAY,	L"home/stub.png", NULL, 2, 0},
	{0, PAGE_HOME,			L"home/stub.png", NULL, 0, 0},
	{0, PAGE_IMGOVR,		L"home/stub.png", NULL, 2, 0},
	{0, PAGE_NONE,			L"home/stub.png", NULL, 0, 0},
	{0, 0, L"", NULL, 0, 0}
};



static inline int alButtonPress (TAPPLAUNCHER *al, TCCBUTTON *btn, int id, const TTOUCHCOORD *pos)
{
	TVLCPLAYER *vp = btn->cc->vp;
	al->btns->t0 = getTickCount();
		
	//printf("alButtonPress %i %i\n", btn->id, id);

	if (!id) return 0;
	
	if (id == ABUTTON_MHOOK){
		ccDisable(btn);

	  	if (mHookGetState()){
			captureMouse(vp, 0);
			mHookUninstall();
		}
	}else if (id == ABUTTON_DRAGRECT){
		TGUIINPUT *cursor = &vp->gui.cursor;
		cursor->dragRectIsEnabled ^= 1;
		dbprintf(vp, "drag rect %s", cursor->dragRectIsEnabled ? "enabled":"disabled");
		
	}else if (id == ABUTTON_MEDIASTATS){
		vp->gui.displayStats = page2RenderGetState(vp->pages, PAGE_MEDIASTATS);
		vp->gui.displayStats ^= 1;
		
		dbprintf(vp, "media stats %s", vp->gui.displayStats ? "enabled":"disabled");
		
		if (vp->gui.displayStats){
			page2Set(vp->pages, PAGE_RENDER_CONCURRENT|PAGE_MEDIASTATS, 0);
		}else{
			page2Set(vp->pages, PAGE_MEDIASTATS, 0);		// remove concurrent
			page2RenderDisable(vp->pages, PAGE_MEDIASTATS);
		}

	}else if (id == ABUTTON_CONSOLE){
	  	consoleToggle(vp);
	  	
	}else{
		id += PAGE_BASEID;

	  	for (int i = 0; i < HOME_BTN_TOTAL-1 && homeBtns[i].page; i++){
	  		if (homeBtns[i].page == id){
	  			if (homeBtns[i].ptype == 1){
	  				if (id == PAGE_CHAPTERS){
	  					TCHAPTER *chapt = pageGetPtr(vp, PAGE_CHAPTERS);
	  					ccEnable(chapt->pane);
	  					
	  				}else if (id == PAGE_SUB){
	  					TSUB *sub = pageGetPtr(vp, PAGE_SUB);
	  					ccEnable(sub->lb);
	  					
	  				}else if (id == PAGE_VKEYBOARD){
	  					TKEYPAD *kp = keyboardGetKeypad(al);
	  					keypadListenerRemoveAll(kp);
	  					//keypadListenerAdd(kp, btn->id, KP_INPUT_OPENED|KP_INPUT_CLOSED|KP_INPUT_SINGLE|KP_INPUT_COMPLETE8|KP_INPUT_COMPLETEW, btn->id);
	  					keypadEditboxSetUserData(&kp->editbox, 0);	// if nobody is listening then there shouldn't be a cc Id
	  					ccEnable(kp);
	  					
					}else if (id == PAGE_PLY_PANEL){
						timerSet(vp, TIMER_PLYPAN_REBUILD, 1000);
	  				}

	  				pageSet(vp, id);
	  				//pageSetSec(vp, -1);
	  				
	  				break;

	  			}else if (homeBtns[i].ptype == 2){
	  				TCCBUTTONS *btns = NULL;
	  				if (id == PAGE_EXIT)
	  					btns = ((TEXIT*)pageGetPtr(vp, PAGE_EXIT))->btns;
	  				else if (id == PAGE_META)
	  					btns = ((TMETA*)pageGetPtr(vp, PAGE_META))->btns;
	  				
	  				if (btns) btns->t0 = getTickCount();
	  				page2Set(vp->pages, PAGE_RENDER_CONCURRENT|id, 1);
	  				break;
					
				}else if (homeBtns[i].ptype == 3){
					if (homeBtns[i].page == PAGE_META){
	  					PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	  					if (!plc) plc = getDisplayPlaylist(vp);
		
						TMETA *meta = pageGetPtr(vp, PAGE_META);
						TMETADESC *desc = &meta->desc;
						
						if (plc->pr->playingItem >= 0)
							desc->trackPosition = plc->pr->playingItem;
						else if (plc->pr->selectedItem >= 0)
							desc->trackPosition = plc->pr->selectedItem;
						else
							desc->trackPosition = 0;
						desc->uid = plc->uid;
						//printf("using playlist %X, track %i\n", plc->uid, desc->trackPosition);
						
						page2Set(vp->pages, PAGE_RENDER_CONCURRENT|homeBtns[i].page, 1);
						return 1;
					}
					//pageSetSec(vp, homeBtns[i].page);
					page2Set(vp->pages, homeBtns[i].page, 1);
	  			}
	  		}
	  	}
	}
	return 1;
}

static inline void homeDrawClockA (TAPPLAUNCHER *home, TFRAME *frame, const TMETRICS *metrics)
{
	double nanos;
	const struct tm *tdate = getTimeReal(&nanos);
	const double hr = tdate->tm_hour;
	const double min = tdate->tm_min;
	const double sec = tdate->tm_sec + nanos;
	const double m = fmod(6.0 * min, 360.0);	// 30 = 360.0 / 12.0
	const double h = 30.0 * (hr+((1.0/60.0)*min));
	const double s = 6.0 * sec;	// 6 = (360.0 / 60.0)

	// faster and better looking
	rotate(home->clock.sec, frame, metrics, s, home->clock.sec->width);
	rotate(home->clock.hr, frame, metrics, h, home->clock.hr->width-1);
	rotate(home->clock.min, frame, metrics, m, home->clock.min->width);


#if DRAWMISCDETAIL
	lDrawLine(frame, metrics->x, metrics->y, metrics->x+metrics->width-1, metrics->y+metrics->height+1, 0xFFFF0000);
	lDrawLine(frame, metrics->x+metrics->width-1, metrics->y, metrics->x, metrics->y+metrics->height+1, 0xFFFF0000);
#endif
}

// multi pass string rendering
static inline void homeRenderDigitStringD (TAPPLAUNCHER *home, TFRAME *frame, const TMETRICS *metrics, const int font, const char *str)
{

	TLPRINTR rt;
	memset(&rt, 0, sizeof(TLPRINTR));

	int x = metrics->x - 4;
	int y = metrics->y + (metrics->height/4);

	int blurOp = LTR_BLUR5;
	lSetForegroundColour(frame->hw, COL_WHITE);
	lSetRenderEffect(frame->hw, blurOp);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, COL_WHITE);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_RADIUS, 5);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_SETTOP, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_ALPHA, 1000);
	
	rt.sx = x; rt.sy = y;
	lPrintEx(frame, &rt, font, PF_IGNOREFORMATTING, LPRT_CPY, str);
	rt.sx = x; rt.sy = y;
		
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, COL_WHITE);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_RADIUS, 1);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_ALPHA, 900);
	lPrintEx(frame, &rt, font, PF_IGNOREFORMATTING, LPRT_CPY, str);


	/*lSetFilterAttribute(frame->hw, LTR_OUTLINE3, 0, 0x4F000000|COL_GREEN);
	lSetRenderEffect(frame->hw, LTR_OUTLINE3);
	lPrintEx(frame, &rt, font, PF_IGNOREFORMATTING, LPRT_CPY, str);*/
	
	blurOp = LTR_BLUR4;
	lSetRenderEffect(frame->hw, blurOp);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, COL_BLACK);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_RADIUS, 4);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_SETTOP, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_ALPHA, 850);

	rt.sx = x; rt.sy = y;
	lPrintEx(frame, &rt, font, PF_IGNOREFORMATTING, LPRT_CPY, str);
	lSetRenderEffect(frame->hw, LTR_DEFAULT);

}

static inline void homeDrawClockD (TAPPLAUNCHER *home, TFRAME *frame, const TMETRICS *metrics)
{
#if 0
	const struct tm *tdate = getTimeReal(NULL);
	const int hr = tdate->tm_hour;
	const int min = tdate->tm_min;

	char buffer[32];
	__mingw_snprintf(buffer, 31, "%.2i:%.2i", hr, min);
#else
	char buffer[32];
	char *format = "hh\':\'mm";
	GetTimeFormatA(0, TIME_FORCE24HOURFORMAT, 0, format, buffer, sizeof(buffer));
#endif	
	homeRenderDigitStringD(home, frame, metrics, HOME_CLK_FONT, buffer);
}

static inline void homeDrawClock (TAPPLAUNCHER *home, TFRAME *frame, const TMETRICS *metrics)
{

	if (home->clock.type == CLOCK_ANALOGUE || home->clock.type == CLOCK_BUTTERFLY)
		homeDrawClockA(home, frame, metrics);
	else
		homeDrawClockD(home, frame, metrics);

#if DRAWMISCDETAIL
	lDrawRectangle(frame, metrics->x, metrics->y, metrics->x+metrics->width-1, metrics->y+metrics->height+1, 0xFF00FF00);
#endif
}

static inline int64_t ccbtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (/*msg == CC_MSG_RENDER*/ /*|| msg == CC_MSG_INPUT ||*/ msg == CC_MSG_SETPOSITION) return 1;
	//if (msg == CC_MSG_ENABLED || msg == CC_MSG_DISABLED) return 1;
		
	TCCBUTTON *btn = (TCCBUTTON*)object;
	//printf("ccbtn_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", btn->id, btn->type, msg, (int)data1, (int)data2, dataPtr);
	//const int id = (int)data2;

	/*if (msg == CC_MSG_RENDER){
		TAPPLAUNCHER *home = pageGetPtr(btn->cc->vp, ccGetOwner(btn));
		if (btn->id == home->ccId){
			TFRAME *frame = (TFRAME*)dataPtr;
			TMETRICS metrics;
			ccGetMetrics(btn, &metrics);
			homeDrawClockA(home, frame, &metrics);
		}

	}else*/ if (msg == BUTTON_MSG_SELECTED_PRESS){
		TAPPLAUNCHER *al = pageGetPtr(btn->cc->vp, ccGetOwner(btn));
		int idx = buttonsButtonToIdx(al->btns, btn);
		return alButtonPress(al, btn, idx, dataPtr);
	}
	return 1;
}

static inline void homeSetPageIconFace (TAPPLAUNCHER *home, const int pageId, const int face)
{
	for (int i = 0; i < HOME_BTN_TOTAL-1 && homeBtns[i].page; i++){
		if (homeBtns[i].page == pageId){
			TCCBUTTON *btn = buttonsButtonGet(home->btns, HOME_BTN_ID(homeBtns[i].page));
			buttonFaceActiveSet(btn, face);
			return;
		}
	}
}

void applLaunchSetPageIconState (TVLCPLAYER *vp, const int pageId, int state)
{
	state &= 1;

	for (int i = 0; i < HOME_BTN_TOTAL-1 && homeBtns[i].page; i++){
		if (homeBtns[i].page == pageId){
			homeBtns[i].enabled = state;
			break;
		}
	}

	TAPPLAUNCHER *al = pageGetPtr(vp, PAGE_HOME);
	buttonsStateSet(al->btns, HOME_BTN_ID(pageId), state);
}

static inline int applLaunchGetEnabledTotal (TAPPLAUNCHER *al)
{
	int ct = 0;
	for (int i = 0; i < HOME_BTN_TOTAL-1 && homeBtns[i].page; i++)
		ct += buttonsStateGet(al->btns, HOME_BTN_ID(homeBtns[i].page));
	return ct;
}

static inline int page_homeRender (TAPPLAUNCHER *al, TVLCPLAYER *vp, TFRAME *frame)
{

	TCCBUTTONS *btns = al->btns;

	if (getPlayState(vp) && getPlaybackMode(vp) == PLAYBACKMODE_VIDEO)
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_TRANSFORM), 1);
	else
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_TRANSFORM), 0);

	if (vp->vlc->mp && vp->vlc->spu.total > 0)
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_SUB), 1);
	else
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_SUB), 0);
		
	if (hasPageBeenAccessed(vp, PAGE_EPG)){
		if (!epgProgrammeLbGetTotal(pageGetPtr(vp, PAGE_EPG)))
			buttonsStateSet(btns, HOME_BTN_ID(PAGE_EPG), 0);
		else
			buttonsStateSet(btns, HOME_BTN_ID(PAGE_EPG), 1);
	}

	if (hasPageBeenAccessed(vp, PAGE_CHAPTERS)){
		if (vp->vlc->mp && chaptersGetTotal(pageGetPtr(vp,PAGE_CHAPTERS)) > 0)
			buttonsStateSet(btns, HOME_BTN_ID(PAGE_CHAPTERS), 1);
		else
			buttonsStateSet(btns, HOME_BTN_ID(PAGE_CHAPTERS), 0);
	}

	if (hasPageBeenAccessed(vp, PAGE_ES)){
		TSTREAMINFO *sinfo = pageGetPtr(vp, PAGE_ES);
		if (!vp->vlc->mp || !sinfo->tCategories)
			buttonsStateSet(btns, HOME_BTN_ID(PAGE_ES), 0);
		else
			buttonsStateSet(btns, HOME_BTN_ID(PAGE_ES), 1);
	}

	PLAYLISTCACHE *plc = getPrimaryPlaylist(vp);
	if (!playlistGetTotal(plc)){
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_OVERLAY), 0);
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_SEARCH), 0);
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_PLY_SHELF), 0);
		//buttonDisable(vp, PAGE_HOME, HOME_BTN_ID(PAGE_PLY_TV);
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_PLY_TV), 0);
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_PLY_PANEL), 0);
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_PLY_PANE), 0);
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_PLY_FLAT), 0);
		
		if (!getPlayState(vp)){
			buttonsStateSet(btns, HOME_BTN_ID(PAGE_CHAPTERS), 0);
			buttonsStateSet(btns, HOME_BTN_ID(PAGE_META), 0);
			buttonsStateSet(btns, HOME_BTN_ID(PAGE_SUB), 0);
			buttonsStateSet(btns, HOME_BTN_ID(PAGE_EPG), 0);
			buttonsStateSet(btns, HOME_BTN_ID(PAGE_ES), 0);
			buttonsStateSet(btns, HOME_BTN_ID(PAGE_TRANSFORM), 0);
//#if (LIBVLC_VERSION_MAJOR >= 2 && LIBVLC_VERSION_MINOR >= 1)
			buttonsStateSet(btns, HOME_BTN_ID(PAGE_EQ), 0);
//#endif
		}
	}else{
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_OVERLAY), 1);
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_SEARCH), 1);
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_PLY_SHELF), 1);
		//buttonEnable(vp, PAGE_HOME, PAGE_PLY_TV);
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_PLY_TV), 1);
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_PLY_PANEL), 1);
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_PLY_PANE), 1);
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_PLY_FLAT), 1);
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_META), 1);
//#if (LIBVLC_VERSION_MAJOR >= 2 && LIBVLC_VERSION_MINOR >= 1)
		buttonsStateSet(btns, HOME_BTN_ID(PAGE_EQ), 1);
//#endif
	}


	int btnPerRow = al->layout.btnsPerRow;
	const int total = applLaunchGetEnabledTotal(al);	
	if (al->layout.autoExpand){
		int height = ceilf(total / (float)al->layout.btnsPerRow) * (float)(al->layout.rowSpace + al->layout.btnHeight);
		//printf("homeRender height %i\n", height);
		if (height >= frame->height-20)
			btnPerRow++;
	}

	int xOffset, yOffset;
	if (total > btnPerRow){
		xOffset = (frame->width  - (btnPerRow * (al->layout.btnWidth + al->layout.colSpace)-al->layout.colSpace))/2;
		yOffset = (frame->height - ((((total-1)/btnPerRow)+0.5) * (al->layout.btnHeight + al->layout.rowSpace)+al->layout.rowSpace))/2;
	}else{
		xOffset = (frame->width  - (total * (al->layout.btnWidth + al->layout.colSpace)))/2;
		yOffset = (frame->height  - (1 * (al->layout.btnHeight + al->layout.rowSpace)))/2;
	}

	for (int i = 0, btnCt = 0; i < HOME_BTN_TOTAL-1 && homeBtns[i].page; i++){
		TCCBUTTON *btn = buttonsButtonGet(al->btns, HOME_BTN_ID(homeBtns[i].page));
		
		if (btn && btn->enabled){
			int c = btnCt % btnPerRow;
			int x = c * (al->layout.btnWidth + al->layout.colSpace);
			int r = btnCt++ / btnPerRow;
			int y = r * (al->layout.btnHeight + al->layout.rowSpace);
			ccSetPosition(btn, x+xOffset, y+yOffset);
		}
	}

	buttonsRenderAll(btns, frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);

	for (int i = 0; i < HOME_BTN_TOTAL-1 && homeBtns[i].page; i++){
		if (homeBtns[i].page == PAGE_CLOCK){
			TMETRICS metrics;
			TCCBUTTON *btn = buttonsButtonGet(al->btns, HOME_BTN_ID(homeBtns[i].page));
			ccGetMetrics(btn, &metrics);
			homeDrawClock(al, frame, &metrics);
			break;
		}
	}

	if (hasPageBeenAccessed(vp, PAGE_OVERLAY)){
		TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
		marqueeDraw(vp, frame, plyctrl->marquee, 2, 2);
	}

	return 1;
}

static inline int page_homeInput (TAPPLAUNCHER *al, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
		setVolumeDisplay(vp, setVolume(vp, getVolume(vp, VOLUME_APP)+3, VOLUME_APP));
		break;
			
	  case PAGE_IN_WHEEL_BACK:
	  	setVolumeDisplay(vp, setVolume(vp, getVolume(vp, VOLUME_APP)-3, VOLUME_APP));
		break;

	  /*case PAGE_IN_TOUCH_DOWN:
	  case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  	break;*/
	}

	return 1;
}

void time64ToTime (const uint64_t t64, date64_t *date)
{
	date->seconds = (t64 / 1000) % 60;
	date->minutes = ((t64 / 1000) / 60) % 60;
	date->hours = (((t64 / 1000) / 60) / 60) % 24;
	date->days = ((((t64 / 1000) / 60) / 60) / 24) % 7;
    date->weeks = (((((t64 / 1000) / 60) / 60) / 24) / 7) % 52;
}

static inline int page_homeStartup (TAPPLAUNCHER *al, TVLCPLAYER *vp, const int fw, const int fh)
{

	//wprintf(L"page_homeStartup: %i %i %i %i '%s'\n", PAGE_HOME, HOME_BTN_TOTAL, (sizeof(homeBtns) / sizeof(THOMEBTN))-1, ABUTTON_MHOOK, homeBtns[27].path);

	
#if (!RELEASEBUILD)
	//assert((sizeof(homeBtns) / sizeof(THOMEBTN)) == (HOME_BTN_TOTAL+1));
	if ((sizeof(homeBtns) / sizeof(THOMEBTN)) != (HOME_BTN_TOTAL+1)){
		printf("home sanity check: (sizeof(homeBtns) / sizeof(THOMEBTN)) != PAGE_TOTAL\n");
		exit(0);
	}
#endif


	TCCBUTTONS *btns = al->btns = buttonsCreate(vp->cc, PAGE_HOME, HOME_BTN_TOTAL+4, ccbtn_cb);

	TCCBUTTON *btn = buttonsCreateButton(btns, L"home/mhook.png", NULL, ABUTTON_MHOOK, 1, 0, 0, 0);
	btn->flags.disableRender = 1;

	btn = buttonsCreateButton(btns, L"home/dragrect.png", NULL, ABUTTON_DRAGRECT, 1, 0, 0, 0);
	btn->flags.disableRender = 1;
	ccSetPosition(btn, fw-1 - ccGetWidth(btn), fh - ccGetHeight(btn));

	btn = buttonsCreateButton(btns, L"home/dragrect.png", NULL, ABUTTON_MEDIASTATS, 1, 0, 0, 0);
	btn->flags.disableRender = 1;
	ccSetPosition(btn, 0, fh - ccGetHeight(btn));

	btn = buttonsCreateButton(btns, L"home/dragrect.png", NULL, ABUTTON_CONSOLE, 1, 0, 0, 0);
	btn->flags.disableRender = 1;
	ccSetPosition(btn, fw-1 - ccGetWidth(btn), 0);

		
	for (int i = 0; i < HOME_BTN_TOTAL && homeBtns[i].page; i++){
		//wprintf(L"creating %i, '%s'\n", i, homeBtns[i].path);
		btn = buttonsCreateButton(btns, homeBtns[i].path, homeBtns[i].pathAlt, HOME_BTN_ID(homeBtns[i].page), homeBtns[i].enabled, 0, 0, 0);
		if (homeBtns[i].animate) buttonAnimateSet(btn, homeBtns[i].animate);
		/*else if (homeBtns[i].page == PAGE_CLOCK)
			al->ccId = btn->id;*/
	}

	int val = 0;
	settingsGet(vp, "home.allowKeypad", &val);
	applLaunchSetPageIconState(vp, PAGE_VKEYBOARD, val);


#if (ENABLE_ANTPLUS)
	applLaunchSetPageIconState(vp, PAGE_ANTPLUS, antConfigIsAntEnabled(al));
#endif

#if (ENABLE_GARMINTCX)
	applLaunchSetPageIconState(vp, PAGE_TCX, garminConfigIsTcxEnabled(al));
#endif

	btn = buttonsButtonGet(btns, homeBtns[0].page-PAGE_BASEID);
	al->layout.btnWidth = ccGetWidth(btn);
	al->layout.btnHeight = ccGetHeight(btn);	
	settingsGet(vp, "home.layout.rowSpace", &al->layout.rowSpace);
	settingsGet(vp, "home.layout.columnSpace", &al->layout.colSpace);
	settingsGet(vp, "home.layout.btnsPerRow", &al->layout.btnsPerRow);
	settingsGet(vp, "home.layout.autoExpand", &al->layout.autoExpand);

	return 1;
}

/*
static HQUERY query;
static HCOUNTER counter;

int GetData (int64_t *value64)
{
	if (!counter) return 0;
	
	PDH_RAW_COUNTER rawcounter;
	PDH_FMT_COUNTERVALUE fmtValue;
	
	PdhCollectQueryData(query);
	PdhGetFormattedCounterValue(counter, PDH_FMT_LARGE | PDH_FMT_NOSCALE, 0, &fmtValue);
	PdhGetRawCounterValue(counter, 0, &rawcounter);

	int nonzero = 0;
	
	if (fmtValue.longValue != 0) {
		*value64 = (int64_t)fmtValue.largeValue;
		nonzero++;
	}
	return nonzero;
}

void AddCounter (const char *path)
{
	printf("AddCounter #%s#\n", path);
	PdhAddCounterA(query, path, 0, &counter);
}
	
void AddObject (const char *objStr, const char *counterStr, const char *interfaceStr)
{
	
	char objs[4096];
	unsigned long objsize = sizeof(objs);

	if (ERROR_SUCCESS != PdhEnumObjects(NULL, NULL, objs, &objsize, PERF_DETAIL_WIZARD, FALSE))
		return;

	char counterlist[4096];
	char instancelist[4096];
	char temppath[4096];
		
	unsigned long csize = sizeof(counterlist);
	unsigned long isize = sizeof(instancelist);
		
	// loop over strings
	for (char *obj = objs; *obj; obj += strlen(obj) + 1){
		if (!strstr(obj, objStr)) continue;


		
		if (PdhEnumObjectItems(NULL, NULL, obj,  counterlist, &csize, instancelist, &isize, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS)
			continue;

		for (char *counter = counterlist; *counter; counter += strlen(counter) + 1){
			if (counterStr && !strstr(counter, counterStr))	
				continue;
			
			//printf(" Counter '%s'\n", counter);

			if (!*instancelist){
				sprintf(temppath, "\\%s\\%s", obj, counter);
				AddCounter(temppath);
				continue;
			}

			for (char *instance = instancelist; *instance; instance += strlen(instance) + 1){
				if (interfaceStr && !strstr(instance, interfaceStr))
					continue;
				
				sprintf(temppath, "\\%s(%s)\\%s", obj, instance, counter);
				AddCounter(temppath);
			}
		}
	}
}

void ChooseUI ()
{
	
	PDH_BROWSE_DLG_CONFIG brwDlgCfg;
	TCHAR szCounterPath[4096];
	
	// Initialize all the fields of a PDH_BROWSE_DLG_CONFIG structure, in
	// preparation for calling PdhBrowseCounters
	
	memset(&brwDlgCfg, 0, sizeof(PDH_BROWSE_DLG_CONFIG));
	
	brwDlgCfg.bSingleCounterPerDialog = 1;
	brwDlgCfg.bLocalCountersOnly = 1;
	
	brwDlgCfg.szReturnPathBuffer = szCounterPath;
	brwDlgCfg.cchReturnPathLength = sizeof(szCounterPath);
	brwDlgCfg.dwDefaultDetailLevel = PERF_DETAIL_WIZARD;
	brwDlgCfg.szDialogBoxCaption = (char*)"PDH Chooser";
	
	szCounterPath[0] = szCounterPath[1] = 0;

	// Bring up the counter browsing dialog
	if (ERROR_SUCCESS != PdhBrowseCounters(&brwDlgCfg))
		return;
	
	for (char *p = szCounterPath; *p; p = p + strlen(p) + 1){
		printf("Selected '%s'\n", p);
	}
}
*/

static inline int page_homeInitalize (TAPPLAUNCHER *home, TVLCPLAYER *vp, const int width, const int height)
{
	
	setPageAccessed(vp, PAGE_HOME);
	
	vp->gui.image[IMGC_CLK_HOURSM] = imageManagerImageAdd(vp->im, L"clock/hr_sm.png");
	vp->gui.image[IMGC_CLK_MINUTESM] = imageManagerImageAdd(vp->im, L"clock/min_sm.png");
	vp->gui.image[IMGC_CLK_SECONDSM] = imageManagerImageAdd(vp->im, L"clock/sec_sm.png");

#if ENABLE_ANTPLUS	
	if (antGetDeviceCount(home))
		homeSetPageIconFace(home, PAGE_ANTPLUS, BUTTON_PRI);
	else
		homeSetPageIconFace(home, PAGE_ANTPLUS, BUTTON_SEC);
#endif
	
	
/*
	PdhOpenQuery(NULL, 0, &query);
	
	//ChooseUI();
	AddObject("Network Interface", "Bytes Received", "_2");
	AddObject("Processor Information", "% Processor Time", "0,_Total");


	int64_t value = 0;
	
	for (int i = 0; i < 100; i++){
		if (GetData(&value))
			printf("value %I64d\n", value);
		
		lSleep(1000);
	}

	PdhCloseQuery(query);
*/
	return 1;
}

static inline int page_homeShutdown (TAPPLAUNCHER *al, TVLCPLAYER *vp)
{
	buttonsDeleteAll(al->btns);
	return 1;
}

void page_homeRenderEnd (TAPPLAUNCHER *al, TVLCPLAYER *vp, int64_t destId, int64_t data2, void *opaquePtr)
{
	ccHoverRenderSigDisable(vp->cc);
	
	if (playlistManagerGetTotal(vp->plm) <= 1){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, 0);
		if (!plc || playlistGetCount(plc, PLAYLIST_OBJTYPE_TRACK) < 1){
			//printf("home end %i, destId:%i\n", pageRenderGetTop(vp->pages), (int)destId);
			if (destId == PAGE_OVERLAY || destId == PAGE_NONE || destId == PAGE_META)
				page2Set(vp->pages, PAGE_HOME, 0);
		}
	}else{
		timerSet(vp, TIMER_CTRL_UPDATETIMESTAMP, 0);
		overlayActivateOverlayResetTimer(vp);
	}
	
	imageManagerImageRelease(vp->im, vp->gui.image[IMGC_CLK_HOURSM]);
	imageManagerImageRelease(vp->im, vp->gui.image[IMGC_CLK_MINUTESM]);
	imageManagerImageRelease(vp->im, vp->gui.image[IMGC_CLK_SECONDSM]);

	lRenderEffectReset(vp->ml->hw, HOME_CLK_FONT, LTR_BLUR4);
	lRenderEffectReset(vp->ml->hw, HOME_CLK_FONT, LTR_BLUR5);
	lSetRenderEffect(vp->ml->hw, LTR_DEFAULT);
}

static inline int page_homeRenderBegin (TAPPLAUNCHER *home, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	artManagerFlush(vp->am);
	
	lRenderEffectReset(frame->hw, HOME_CLK_FONT, LTR_BLUR4);
	lRenderEffectReset(frame->hw, HOME_CLK_FONT, LTR_BLUR5);
	
	home->clock.type = vp->gui.clockType;
	
	if (home->clock.type == CLOCK_ANALOGUE || home->clock.type == CLOCK_BUTTERFLY){
		home->clock.hr = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_CLK_HOURSM]);
		home->clock.min = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_CLK_MINUTESM]);
		home->clock.sec = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_CLK_SECONDSM]);
		homeSetPageIconFace(home, PAGE_CLOCK, BUTTON_PRI);
	}else{
		lRenderEffectReset(frame->hw, HOME_CLK_FONT, LTR_BLUR5);
		lRenderEffectReset(frame->hw, HOME_CLK_FONT, LTR_BLUR4);
		homeSetPageIconFace(home, PAGE_CLOCK, BUTTON_SEC);
	}
	
	ccHoverRenderSigEnable(vp->cc, 16.0);
	return 1;
}

static inline int page_homeRenderInit (TAPPLAUNCHER *home, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	return 1;
}

int page_homeCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TAPPLAUNCHER *home = (TAPPLAUNCHER*)pageStruct;
	
	//if (msg != PAGE_CTL_RENDER)
		//printf("# page_homeCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_homeRender(home, home->com->vp, dataPtr);


	}else if (msg == PAGE_CTL_RENDER_START){
		return page_homeRenderBegin(home, home->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	}else if (msg == PAGE_CTL_RENDER_END){
		page_homeRenderEnd(home, home->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_homeInput(home, home->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_homeStartup(home, home->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_homeInitalize(home, home->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_homeShutdown(home, home->com->vp);

	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_homeRenderInit(home, home->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);;
	
#if ENABLE_ANTPLUS
	}else if (msg == PAGE_MSG_DEVICE_ARRIVE){
		int vid = dataInt1;
		int pid = dataInt2;
		int cfgvid, cfgpid;
		
		antConfigGetDeviceIds(home, &cfgvid, &cfgpid);
		if (vid == cfgvid && pid == cfgpid)
			homeSetPageIconFace(home, PAGE_ANTPLUS, BUTTON_PRI);

	}else if (msg == PAGE_MSG_DEVICE_DEPART){
		int vid = dataInt1;
		int pid = dataInt2;
		int cfgvid, cfgpid;
		
		antConfigGetDeviceIds(home, &cfgvid, &cfgpid);
		if (vid == cfgvid && pid == cfgpid){
			if (!antGetDeviceCount(home))
				homeSetPageIconFace(home, PAGE_ANTPLUS, BUTTON_SEC);
		}
#endif
	}else if (msg == PAGE_MSG_INPUT_MCAP){
		buttonsStateSet(home->btns, ABUTTON_MHOOK, dataInt1);
	}

	
	return 1;
}