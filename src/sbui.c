
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


#include <math.h>
#include "common.h"




extern volatile int SHUTDOWN;

static int isResyncing = 0;
static int currentGestureState = 3;
static HANDLE hDkStateEvent = NULL;
static HANDLE hSbuiDkThread = NULL;

static int key10State = BTN_CFG_PADCTRL_ON;
static int lastPosi = 0;
static int currentDk[16];
	


static int ids[11];
static const wchar_t *images[11] = {L"sbui/trackpre.png", L"sbui/play.png", L"sbui/stop.png",    L"sbui/tracknext.png", L"sbui/playlist.png", 
									L"sbui/home.png",     L"sbui/ctrl.png", L"sbui/browser.png", L"sbui/vlc.png",       L"sbui/quit.png", 
									L"sbui/trackpad.png"};


static int idsPlay[12];
static const wchar_t *imagesPlay[12] = {L"sbui/pause_0.png", L"sbui/pause_1.png", L"sbui/pause_2.png", L"sbui/pause_3.png", L"sbui/pause_4.png", 
										L"sbui/pause_5.png", L"sbui/pause_6.png", L"sbui/pause_7.png", L"sbui/pause_8.png", L"sbui/pause_9.png",
										L"sbui/pause_10.png",L"sbui/playpause.png"};


typedef void (*sbuibkcb)(TVLCPLAYER *vp, const int state);
static inline int sbuiDKCB (const int dk, const int state, void *ptr);


lDISPLAY sbuiGetLibmylcdDID (THWD *hw)
{
	lDISPLAY did = lDriverNameToID(hw, "switchbladefio", LDRV_DISPLAY);
	//if (!did) did = lDriverNameToID(hw, "sbui153", LDRV_DISPLAY);
	return did;
}

// is/was an sbui device activated
int isSBUIEnabled (TVLCPLAYER *vp)
{
	return (sbuiGetLibmylcdDID(vp->ml->hw) > 0);
}

int sbuiSetDKImage (TVLCPLAYER *vp, const int key, TFRAME *img)
{
	lDISPLAY did = sbuiGetLibmylcdDID(vp->ml->hw);
	if (!did || SHUTDOWN) return 0;
	
	//printf("sbuiSetDKImage %i\n", key);
	
	TSBGESTURESETDK sbdk;
	sbdk.size = sizeof(TSBGESTURESETDK);
	sbdk.op = SBUI_SETDK_IMAGE;
	sbdk.u.image.key = key;
	sbdk.u.image.image = img;
	
#if DRAWMISCDETAIL
	lDrawRectangle(img, 0, 0, img->width-1, img->height-1, 255<<24|COL_GREEN);
	lDrawLine(img, 0, 0, img->width-1, img->height-1, 255<<24|COL_GREEN);
	lDrawLine(img, 0, img->height-1, img->width-1, 0, 255<<24|COL_GREEN);
#endif

	return lSetDisplayOption(vp->ml->hw, did, lOPT_SBUI_SETDK, (intptr_t*)&sbdk);
}

#if 1
int sbuiSetDKImageEx (TVLCPLAYER *vp, TFRAME *img, const int desX, const int desY, const int x1, const int y1, const int x2, const int y2)
{
	lDISPLAY did = sbuiGetLibmylcdDID(vp->ml->hw);
	if (!did || SHUTDOWN) return 0;
	
	TSBGESTURESETDK sbdk;
	sbdk.size = sizeof(TSBGESTURESETDK);
	sbdk.op = SBUI_SETDK_DIRECT;
	sbdk.u.direct.src.x1 = x1;
	sbdk.u.direct.src.y1 = y1;
	sbdk.u.direct.src.x2 = x2;
	sbdk.u.direct.src.y2 = y2;
	sbdk.u.direct.des.x = desX;		// dk lcd
	sbdk.u.direct.des.y = desY;		// dk lcd
	sbdk.u.direct.image = img;

#if (DRAWMISCDETAIL)
	lDrawRectangle(img, 0, 0, img->width-1, img->height-1, 255<<24|COL_GREEN);
	lDrawLine(img, 0, 0, img->width-1, img->height-1, 255<<24|COL_GREEN);
	lDrawLine(img, 0, img->height-1, img->width-1, 0, 255<<24|COL_GREEN);
#endif
	
	return lSetDisplayOption(vp->ml->hw, did, lOPT_SBUI_SETDK, (intptr_t*)&sbdk);
}
#endif


#if 0
int sbuiSetDKImageFile (TVLCPLAYER *vp, const int key, wchar_t *file)
{

	lDISPLAY did = sbuiGetLibmylcdDID(vp->ml->hw);
	if (!did || SHUTDOWN) return 0;

	TSBGESTURESETDK sbdk;
	wchar_t drive[_MAX_DRIVE+1];
	wchar_t dir[_MAX_DIR+1];
	wchar_t path[MAX_PATH+1];
	wchar_t buffer[MAX_PATH+1];
	

	GetModuleFileNameW(NULL, path, MAX_PATH);
	_wsplitpath(path, drive, dir, NULL, NULL);
	__mingw_swprintf(path, L"%ls%ls%ls", drive, dir, buildSkinD(vp, buffer, file));

	sbdk.size = sizeof(TSBGESTURESETDK);
	sbdk.op = SBUI_SETDK_FILE;
	sbdk.u.file.path = path;
	sbdk.u.file.key = key;	
	return lSetDisplayOption(vp->ml->hw, did, lOPT_SBUI_SETDK, (intptr_t*)&sbdk);
}
#endif

int sbuiSetDKImageArtId (TVLCPLAYER *vp, const int key, const int id)
{
	//printf("sbuiSetDKImageArtId %i %i\n", key, id);
	
	if (SHUTDOWN) return 0;
	
	int ret = 0;
	TFRAME *img = artManagerImageAcquire(vp->im, id);
	if (img){
		ret = sbuiSetDKImage(vp, key, img);
		artManagerImageRelease(vp->im, id);
	}
	return ret;
}

int sbuiSetDKImageChar (TVLCPLAYER *vp, const int dk, const int font, const unsigned int colour, const char Char)
{
	const char txt[2] = {Char, 0};
	const unsigned int hash = (dk<<24) | (font << 16) | (Char << 8) | ((colour^Char)&0xFF);
	
	TFRAME *img = strcFindString(vp->strc, hash);
	if (img){
		return sbuiSetDKImage(vp, dk, img);
	}else{
		TMETRICS metrics = {8, 8, SBUI_DK_WIDTH, SBUI_DK_HEIGHT};
		TFRAME *img = newStringEx3(vp->ml->hw, &metrics, LFRM_BPP_32A, 0, font, txt, 0, 0, metrics.width, metrics.height);
		if (img){
			strcAddString(vp->strc, img, hash);
			return sbuiSetDKImage(vp, dk, img);
		}
	}
	return 0;
}

void sbuiDKStateChange ()
{
	if (hDkStateEvent)
		SetEvent(hDkStateEvent);
}

static inline unsigned int __stdcall sbuiDKImageThread (void *ptr)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;
	//printf("sbuiDKImageThread started\n");

	//wchar_t path[MAX_PATH+1];
	sbuiDKSetImages(vp);
	currentDk[SBUI_DK_2] = -1;//getPlayState(vp);		// play/pause key

	lSleep(250);	
	//printf("hDkStateEvent %p\n", hDkStateEvent);

	while (!SHUTDOWN && hDkStateEvent){
		int ret = WaitForSingleObject(hDkStateEvent, vp->gui.awake ? 1000 : INFINITE);
		
		//printf("WaitForSingleObject %i %i\n", ret, vp->renderState);
		if (ret != WAIT_OBJECT_0 && ret != WAIT_TIMEOUT)
			break;
		if (SHUTDOWN) break;
		if (!vp->renderState) continue;
		
		volatile double pos = vp->vlc->position;
		clipFloat(pos);
		int posi = (int)((10.0*pos)+0.5);
		if (posi < 0) posi = 0;
		else if (posi > 10) posi = 10;


		//const volatile int playState = vlc_getState(vp->vlc);
		const volatile int playState = vp->vlc->vlcPlayState;
		//printf("sbui dk state %i\n", playState);
		
		if (playState != currentDk[SBUI_DK_2] || posi != lastPosi){
			SBUI_DK_2[currentDk] = playState;
			switch (playState){
			  case libvlc_Opening:
			  case libvlc_Buffering:
			  case libvlc_Playing:
				lastPosi = posi;
		  		sbuiSetDKImageArtId(vp, SBUI_DK_2, idsPlay[posi]);
				break;

			  case libvlc_Paused:{
			  	sbuiSetDKImageArtId(vp, SBUI_DK_2, idsPlay[11]);
			  	break;
			  }
			  case libvlc_NothingSpecial:
				lastPosi = posi;
				ALLOW_FALLTHROUGH;
			  case libvlc_Stopped:
			  case libvlc_Ended:
			  case libvlc_Error:{
				sbuiSetDKImageArtId(vp, SBUI_DK_2, ids[1]);
			  }
				break;
			}
			//SBUI_DK_2[currentDk] = playState;
		}
		
		if (key10State != vp->gui.padctrlMode){
			key10State = vp->gui.padctrlMode;
		
			int id;
			if (key10State == BTN_CFG_PADCTRL_ON)
				id = 9;
			else
				id = 10;

			sbuiSetDKImageArtId(vp, SBUI_DK_10, ids[id]);
		}
	}
	
	//printf("sbuiDKImageThread exited\n");
	_endthreadex(1);
	return 1;
}

unsigned int sbuiStartImageThread (TVLCPLAYER *vp, const int threadFlags)
{
	unsigned int tid = 0;
	hSbuiDkThread = (HANDLE)_beginthreadex(NULL, THREADSTACKSIZE, sbuiDKImageThread, vp, threadFlags, &tid);
	return tid;
}

void sbuiSetApplState (const int state)
{
	SHUTDOWN = state;
}

static inline void sbuiDK_1_cb (TVLCPLAYER *vp, const int state)
{
	// printf("dk 1, %i\n", state);
	if (state == SBUI_DK_DOWN)
		timerSet(vp, TIMER_PREVTRACK, 0);
}

static inline void sbuiDK_2_cb (TVLCPLAYER *vp, const int state)
{
	if (state == SBUI_DK_DOWN){
		if (getPlayState(vp) == 1 || getPlayState(vp) == 2)		// is paused
			trackPlayPause(vp);
		else
  			timerSet(vp, TIMER_PLAY, 0);
  	}
  	wakeup(vp);
}

static inline void sbuiDK_3_cb (TVLCPLAYER *vp, const int state)
{
	// printf("dk 3, %i\n", state);
	
	if (state == SBUI_DK_DOWN)
		timerSet(vp, TIMER_STOP, 0);
}

static inline void sbuiDK_4_cb (TVLCPLAYER *vp, const int state)
{
	//printf("dk 4, %i %i\n", state, preState);
	//static int preState = 0;
	
	if (state == SBUI_DK_DOWN){
	/*	timerSet(vp, TIMER_FASTFORWARD, 500);
		preState = 1;
	}else{
		timerReset(vp, TIMER_FASTFORWARD);
		if (!preState)*/
			timerSet(vp, TIMER_NEXTTRACK, 0);
		//preState = 0;
	}
}

static inline void sbuiDK_5_cb (TVLCPLAYER *vp, const int state)
{
	 //printf("dk 5, %i\n", state);

	const int playlist = PAGE_PLY_PANE;
	
	if (state == SBUI_DK_DOWN){
		if (pageGet(vp) == playlist){
			page2SetPrevious(page2PageStructGet(vp->pages, playlist));
		}else{
			if (playlistManagerGetTotal(vp->plm) <= 1){
				PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, 0);
				if (!plc || playlistGetCount(plc, PLAYLIST_OBJTYPE_TRACK) < 1)
					return;
			}
			page2Set(vp->pages, playlist, 1);
		}
	}
}

static inline void sbuiDK_6_cb (TVLCPLAYER *vp, const int state)
{
	// printf("dk 6, %i\n", state);

	if (state == SBUI_DK_DOWN){
		if (pageGet(vp) == PAGE_HOME)
			page2SetPrevious(page2PageStructGet(vp->pages, PAGE_HOME));
		else
			page2Set(vp->pages, PAGE_HOME, 1);
	}
}

static inline void sbuiDK_7_cb (TVLCPLAYER *vp, const int state)
{
	// printf("dk 7, %i\n", state);
	if (state == SBUI_DK_DOWN){
		if (pageGet(vp) == PAGE_OVERLAY)
			page2SetPrevious(page2PageStructGet(vp->pages, PAGE_OVERLAY));
		else
			page2Set(vp->pages, PAGE_OVERLAY, 1);
	}
}

static inline void sbuiDK_8_cb (TVLCPLAYER *vp, const int state)
{
	//printf("dk 8, %i\n", state);
	
	if (state == SBUI_DK_DOWN){
		if (pageGet(vp) == PAGE_EXP_PANEL)
			page2SetPrevious(page2PageStructGet(vp->pages, pageRenderGetTop(vp->pages)));
		else
			timerSet(vp, TIMER_EXPPAN_REBUILDSETPAGE, 0);
	}
}

static inline void sbuiDK_9_cb (TVLCPLAYER *vp, const int state)
{
	// printf("dk 9, %i %i\n", state, getPlayState(vp));

	if (state == SBUI_DK_DOWN){
		if (getPlayState(vp) == 1 || getPlayState(vp) == 2)
			startVlcTrackPlayback(vp);
		else
			startVlc(vp);
	}
}

static inline void sbuiDK_10_cb (TVLCPLAYER *vp, const int state)
{
	//printf("dk 10, %i\n", state);

	static uint64_t t0 = 0;
		
	if (state == SBUI_DK_DOWN){
		if (getPadControl(vp) == BTN_CFG_PADCTRL_OFF){
			//printf("pad is off\n");
		
			setPadControl(vp, BTN_CFG_PADCTRL_ON);
			setAwake(vp);
			currentGestureState = 3;
			sbuiDKStateChange();

			renderSignalUpdate(vp);
			t0 = getTickCount();
			return;
		}
	}

	if (state == SBUI_DK_DOWN){
		t0 = getTickCount();

		if (page2RenderGetState(vp->pages, PAGE_EXIT))
			page2RenderDisable(vp->pages, PAGE_EXIT);
		else
			page2Set(vp->pages, PAGE_EXIT, 1);

		renderSignalUpdate(vp);
		
	}else if (state == SBUI_DK_UP){
		if (getTickCount() - t0 >= 4000){
			printf("#### force shutdown initiated ####\n\n");
			
			//printf("%u %i, %u\n", (int)getTickCount(), (int)t0, (int)getTickCount()-t0);
			
			sbuiSetApplState(1);	// we're shutting down
			setApplState(vp, 0);	// let everyone else know..
			
			playlistManagerUnlock(vp->plm);
			playlistManagerUnlock(vp->plm);
			loadUnlock(vp);
			loadUnlock(vp);
			lockRelease(vp->ctx.hVideoLock);
			lockRelease(vp->ctx.hVideoLock);
			lockRelease(vp->ctx.hVideoCBLock);
			lockRelease(vp->ctx.hVideoCBLock);
			renderUnlock(vp);
			renderUnlock(vp);
			vlcEventsUnlock(vp->vlc);
			vlcEventsUnlock(vp->vlc);
			
			if (hasPageBeenAccessed(vp, PAGE_CHAPTERS)){
				TCHAPTER *chap = pageGetPtr(vp, PAGE_CHAPTERS);
				chaptersUnlock(chap);
				chaptersUnlock(chap);
			}
			return;
		}
		renderSignalUpdate(vp);
	}
}

static inline int sbuiGestureCB (const TSBGESTURE *sbg, void *ptr)
{
	if (!sbg || !ptr) return 0;	// shouldn't ever happen
	
	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;
	if (!vp->applState) return 0;
	if (!vp->renderState) return 1;

	static int btnId = 1024;	// todo: put these somewhere
	static int streamId = -1;
	static int dumpId = -1;
	static uint64_t lastUptime = 0;
	
	
	int total = sbg->params;
	if (total > 1) total = 1;

	if (sbg->type != SBUICB_GESTURE_PRESS){
		//printf("invalid gesture\n");
		return 0;
	}
		
	TTOUCHCOORD pos;
	pos.x = sbg->x;
	pos.y = sbg->y;
	pos.z1 = sbg->z;
	pos.z2 = sbg->z;
	pos.time = getTime(vp);// sbg->time;
	pos.dt = sbg->dt;
	pos.count = sbg->ct;
	pos.id = sbg->id;
	pos.pen = 1 & ~total;
	pos.pressure = 100;
	int pressState = 0;
	
	//printf("press type:%i, %i,%i '%i' %f %i:%i %f %f\n", sbg->type, sbg->x, sbg->y, 1&~total, sbg->dt, sbg->ct, sbg->id, pos.dt, sbg->time);
	
	if (streamId != sbg->id){	// attempt to get rid of invalid and/or ghost taps from the sbui device
		if (sbg->dt < 20){
			dumpId = sbg->id;
		}else if (!pos.pen && (pos.time - lastUptime) <= 40){
			dumpId = sbg->id;
		}
	}

	if (sbg->id == dumpId){
		//printf("dumping %i\n", dumpId);
		return 1;
	}

	
	//dumpId = -1;
	streamId = sbg->id;
	if (pos.pen)
		lastUptime = pos.time;
	
	if (currentGestureState == 3){						// was up
		pressState = 1;									// but is now down
		pos.pen = 0;
		pos.id = ++btnId;
		pos.dt = 150;

	}else if (currentGestureState == 1 && !pos.pen){	// was down
		pressState = 2;									// is still down, so must be a drag
		pos.id = btnId;
		
	}else if (currentGestureState == 1 && pos.pen){		// was down
		pressState = 3;									// but is now up
		pos.id = btnId;
		
	}else if (currentGestureState == 2 && !pos.pen){	// was down, draging
		pressState = 2;									// is still draging
		pos.id = btnId;
		
	}else if (currentGestureState == 2 && pos.pen){		// was down, draging
		pressState = 3;									// but is now up
		pos.id = btnId;
	}

	currentGestureState = pressState;	
	
	switch (pressState){
	  case 1:
		//printf("## down %i\n", pos.id);
		touchSimulate(&pos, TOUCH_VINPUT|0, ptr);
		break;
	  case 2:
		//printf("## drag %i\n", pos.id);
		touchSimulate(&pos, TOUCH_VINPUT|1, ptr);
		break;
	  case 3:
		//printf("## up %i\n", pos.id);
		touchSimulate(&pos, TOUCH_VINPUT|3, ptr);
		break;
	}
	
	return 1;
}

static inline int sbuiGestureCfg (TVLCPLAYER *vp, const int op, const int gesture, const int state)
{
	int ret = 0;

	lDISPLAY did = sbuiGetLibmylcdDID(vp->ml->hw);
	if (did){
		TSBGESTURECBCFG sbcfg;
		sbcfg.op = op;
		sbcfg.gesture = gesture;
		sbcfg.state = state;
		ret = lSetDisplayOption(vp->ml->hw, did, lOPT_SBUI_GESTURECFG, (intptr_t*)&sbcfg);
	}
	return ret;
}

int sbuiGestureCBEnable (TVLCPLAYER *vp)
{
	lDISPLAY did = sbuiGetLibmylcdDID(vp->ml->hw);
	if (did){
		if (lSetDisplayOption(vp->ml->hw, did, lOPT_SBUI_UDATAPTR, (intptr_t*)vp)){
			if (lSetDisplayOption(vp->ml->hw, did, lOPT_SBUI_GESTURECB, (intptr_t*)sbuiGestureCB)){
				sbuiGestureCfg(vp, SBUICB_OP_GestureEnable, SBUICB_GESTURE_PRESS, SBUICB_STATE_ENABLED);
				//sbuiGestureCfg(vp, SBUICB_OP_GestureSetNotification, SBUICB_GESTURE_PRESS, SBUICB_STATE_ENABLED);
				//sbuiGestureCfg(vp, SBUICB_OP_GestureSetOSNotification, SBUICB_GESTURE_PRESS, SBUICB_STATE_DISABLED);
				return 1;
			}
		}
	}else{
		// printf("SwitchBladeUI not found\n");
	}
	return 0;
}

void sbuiCfgSetPadControl (TVLCPLAYER *vp, const int mode)
{
	if (sbuiGetLibmylcdDID(vp->ml->hw)){
		if (mode == BTN_CFG_PADCTRL_ON){			// enable gesture control, disable OS notification
			sbuiGestureCfg(vp, SBUICB_OP_GestureSetOSNotification, SBUICB_GESTURE_TAP|SBUICB_GESTURE_PRESS, SBUICB_STATE_DISABLED);
			sbuiGestureCfg(vp, SBUICB_OP_GestureEnable, SBUICB_GESTURE_PRESS, SBUICB_STATE_ENABLED);
			sbuiDKStateChange();
			
		}else if (mode == BTN_CFG_PADCTRL_OFF){		// disable gesture control, enable OS notification
			sbuiGestureCfg(vp, SBUICB_OP_GestureSetOSNotification, SBUICB_GESTURE_TAP|SBUICB_GESTURE_PRESS, SBUICB_STATE_ENABLED);
			sbuiGestureCfg(vp, SBUICB_OP_GestureEnable, SBUICB_GESTURE_PRESS, SBUICB_STATE_DISABLED);	
			sbuiDKStateChange();
#if 1
			// SBUI SDK won't send a touch up event after this, so simulate one, otherwise the event queue state gets confused
			TTOUCHCOORD pos;
			pos.x = 0;
			pos.y = 0;
			pos.z1 = 0;
			pos.z2 = 0;
			pos.time = 0;
			pos.dt = 5;
			pos.count = 1;
			pos.id = 0;
			pos.pen = 1;
			pos.pressure = 100;
			touchSimulate(&pos, TOUCH_VINPUT|3, vp);
#endif
		}
	}
}

void sbuiGestureCBDisable (TVLCPLAYER *vp)
{
	lDISPLAY did = sbuiGetLibmylcdDID(vp->ml->hw);
	if (did){
		lSetDisplayOption(vp->ml->hw, did, lOPT_SBUI_DKCB, 0);
		sbuiGestureCfg(vp, SBUICB_OP_GestureEnable, SBUICB_GESTURE_PRESS, SBUICB_STATE_DISABLED);
		lSetDisplayOption(vp->ml->hw, did, lOPT_SBUI_GESTURECB, 0);
	}
}

static inline void sbuiEndDKManager ()
{
	int pid = processGetPid("RzDKManager.exe");
	if (pid){
		processKillWindow(pid);		// seems to be enough, at times
		lSleep(1);
		processKillOpenThread(pid);
		lSleep(1);
		processKillRemoteThread(pid);
		lSleep(1);
	}
}

int sbuiDKCBEnable (TVLCPLAYER *vp)
{
	//printf("sbuiDKCBEnable\n");
	
	lDISPLAY did = sbuiGetLibmylcdDID(vp->ml->hw);
	if (did){
		if (lSetDisplayOption(vp->ml->hw, did, lOPT_SBUI_UDATAPTR, (intptr_t*)vp)){
			if (lSetDisplayOption(vp->ml->hw, did, lOPT_SBUI_DKCB, (intptr_t*)sbuiDKCB)){
				if (!hDkStateEvent)
					hDkStateEvent = CreateEvent(NULL, 0, 0, NULL);
				//printf("sbuiDKCBEnable hDkStateEvent %p\n", hDkStateEvent);
				sbuiEndDKManager();
				return 1;
			}
		}
	}
	return 0;
}

void sbuiDKCBDisable (TVLCPLAYER *vp)
{
	lDISPLAY did = sbuiGetLibmylcdDID(vp->ml->hw);
	if (did){
		lSetDisplayOption(vp->ml->hw, did, lOPT_SBUI_DKCB, 0);
		if (hDkStateEvent){
			ResumeThread(hSbuiDkThread);
			SetEvent(hDkStateEvent);
			WaitForSingleObject(hSbuiDkThread, INFINITE);
			CloseHandle(hSbuiDkThread);
			hSbuiDkThread = NULL;
			CloseHandle(hDkStateEvent);
			hDkStateEvent = NULL;
		}
	}
}

int sbuiDKSetImages (TVLCPLAYER *vp)
{
	if (SHUTDOWN) return 0;

	int ret = 0;
	resetCurrentDirectory();

	key10State = BTN_CFG_PADCTRL_ON;
	lastPosi = 0;
	memset(currentDk, 255, sizeof(currentDk));
	currentDk[SBUI_DK_2] = getPlayState(vp);		// play/pause key

	//lSleep(20);

	if (!idsPlay[0] || !ids[0]){
		for (int i = 0; i < 12; i++)
			idsPlay[i] = artManagerImageAdd(vp->im, imagesPlay[i]);
		
		for (int i = 0; i < 11; i++)
			ids[i] = artManagerImageAdd(vp->im, images[i]);
	}		

	//double t0 = getTime(vp);	
	for (int i = 0; i < 10; i++)
		ret += sbuiSetDKImageArtId(vp, i+1, ids[i]);

	//double t1 = getTime(vp);
	//printf("ret %i, %.2f %.2f\n", ret, t1-t0, (t1-t0)/(double)ret);
	
	return ret;
}

static const sbuibkcb dkCBLookup[12] = {
	NULL,
	sbuiDK_1_cb,
	sbuiDK_2_cb,
	sbuiDK_3_cb,
	sbuiDK_4_cb,
	sbuiDK_5_cb,
	sbuiDK_6_cb,
	sbuiDK_7_cb,
	sbuiDK_8_cb,
	sbuiDK_9_cb,
	sbuiDK_10_cb,
	NULL
};

// called when app has woken up from an event not directly generated by the switchblade hardware
void sbuiWoken (TVLCPLAYER *vp)
{
	if (isSBUIEnabled(vp)){
		sbuiDKSetImages(vp);
		//sbuiDKStateChange();
	}
}

static inline int sbuiDKCB (const int dk, const int state, void *ptr)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;
	if (SHUTDOWN || !vp || !getApplState(vp)) return 0;
	
	//printf("@ sbuiDKCB: dk %i, state %i, %i\n", dk, state, getApplState(vp));

	if (dk == SBUI_DK_CLOSE){
		if (!isResyncing)
			exitInitShutdown(vp);
		return 1;
	}else if (dk == SBUI_DK_EXIT){
		exitInitShutdown(vp);
		return 1;
		
	}else if (dk == SBUI_DK_ACTIVATE){
		//printf("SBUI_DK_ACTIVATE\n");
		if (state == SBUI_DK_DOWN){
		
			currentGestureState = 3;	// up (not pressed)
#if 0
			sbuiGestureCBEnable(vp);	// workaround 'feature' in sdk 2.0.0.2
			sbuiDKCBEnable(vp);
			//sbuiDKSetImages(vp);
#endif
			sbuiDKSetImages(vp);

			if (SHUTDOWN) return 1;
			vp->renderState = 1;

			if (!vp->gui.awake){
				updateTickerStart(vp, UPDATERATE_ALIVE);
				if (pageGet(vp) == PAGE_CLOCK/* && pageGetSec(vp) == -1*/){
					void *ptr = page2PageStructGet(vp->pages, PAGE_CLOCK);
					page2SetPrevious(ptr);
				}
			}
			vp->gui.frameCt = 0;
#if 1
			timerSet(vp, TIMER_STOP, 10);		// because key 3 (STOP) is broken on my device
#endif
			wakeup(vp);
			sbuiDKStateChange();
			//timerSet(vp, TIMER_SBUI_CONNECTED, 100);
			renderSignalUpdate(vp);
		}
	}else if (dk == SBUI_DK_DEACTIVATE){
		//printf("SBUI_DK_DEACTIVATE\n");
		
		vp->renderState = 0;
		if (mHookGetState()){
			captureMouse(vp, 0);
			mHookUninstall();
		}
		if (kHookGetState()){
			kHookOff();
			kHookUninstall();
		}
		currentGestureState = 3;	// up (not pressed)
		
	}else if ((dk >= SBUI_DK_1 && dk <= SBUI_DK_10) && (state == SBUI_DK_UP || state == SBUI_DK_DOWN)){
		if (!vp->gui.awake){
			updateTickerStart(vp, UPDATERATE_ALIVE);
			if (pageGet(vp) == PAGE_CLOCK/* && pageGetSec(vp) == -1*/){
				void *ptr = page2PageStructGet(vp->pages, PAGE_CLOCK);
				page2SetPrevious(ptr);
			}
		}

		setAwake(vp);
		if (vp->renderState){
			if (renderLock(vp)){
				dkCBLookup[dk](vp, state);
				vp->gui.frameCt = 0;
				renderSignalUpdate(vp);
				renderUnlock(vp);
			}
		}
	}
	
	return 1;
}

int sbuiSimulateDk (const int dk, void *ptr)
{
	int ret = sbuiDKCB(dk, SBUI_DK_DOWN, ptr);
	lSleep(10);
	sbuiDKCB(dk, SBUI_DK_UP, ptr);
	return ret;
}

int sbuiResync (TVLCPLAYER *vp, lDISPLAY did)
{
	isResyncing = 1;
	
	intptr_t intp;
	if (!lSetDisplayOption(vp->ml->hw, did, lOPT_SBUI_RECONNECT, &intp)){
		//printf("sbui reconnect failed\n");
	}else if (!sbuiGestureCBEnable(vp)){
		//printf("sbui gesture cb initialization failed\n");
	}else if (!sbuiDKCBEnable(vp)){
		//printf("sbui dk cb initialization failed\n");
	}else{
		int ret = sbuiDKSetImages(vp);
		lSleep(100);
		isResyncing = 0;
		
		if (ret != 20){	// 20 = 10 keys with 2 images per key
			//printf("sbui dk image setup failed (%i)\n", ret);
		}else{
			return 1;
		}
	}
	isResyncing = 0;
	return 0;
}

static inline int sbuiInitDisplay (TVLCPLAYER *vp)
{
	int did = 0;
	
	if (renderLock(vp)){
		did = configStartDisplay(vp, 0);
		renderUnlock(vp);
	}
	
	return did;
}
// TIMER_SBUI_CONNECTED
void timer_sbuiConnected (TVLCPLAYER *vp)
{
	//printf("timer_sbuiConnected\n");
	static int tempDID = -1;
	
	if (!isSBUIEnabled(vp)){		// then activate it
		int did = sbuiInitDisplay(vp);
		if (!did){
			if (vp->ml->killRzDKManagerOnConnectFailRetry)			// needed otherwise it'll never activate
				sbuiEndDKManager();
			lSleep(50);
			sbuiInitDisplay(vp);
		}
		
		if (isSBUIEnabled(vp)){
			//if (vp->ml->enableVirtualDisplay){
				vp->ml->enableVirtualDisplay = 0;
				if (vp->ml->display[DISPLAYMAX-1]->did){
					lCloseDevice(vp->ml->hw, vp->ml->display[DISPLAYMAX-1]->did);
					vp->ml->display[DISPLAYMAX-1]->did = 0;
				}
			//}
		}
	}
	
	if (isSBUIEnabled(vp)){
		if (tempDID != -1 && vp->ml->enableVirtualDisplay){
			lCloseDevice(vp->ml->hw, tempDID);
			tempDID = -1;
			vp->ml->enableVirtualDisplay = 0;
		}
		vp->gui.drawControlIcons = 0;
		
		lDISPLAY did = sbuiGetLibmylcdDID(vp->ml->hw);
		if (did){
			sbuiResync(vp, did);
			ResumeThread(hSbuiDkThread);
		}
	}

	sbuiEndDKManager();
}

// TIMER_SBUI_DISCONNECTED
void timer_sbuiDisconnected (TVLCPLAYER *vp)
{
	//printf("timer_sbuiDisonnected\n");
	
	if (!isSBUIEnabled(vp)){
		SuspendThread(hSbuiDkThread);
		return;
	}

#if 0
	if (tempDID == -1 && !vp->ml->enableVirtualDisplay){
		TDISPLAY *disp = libmylcd_DisplayCfg("DDRAW", vp->ml->width, vp->ml->height, vp->ml->bpp);
		if (disp){
			tempDID = libmylcd_DisplayStart(vp->ml->hw, disp);
			if (tempDID >= 0) vp->ml->enableVirtualDisplay = 1;
			libmylcd_DisplayFree(disp);
		}
	}
#endif
}
