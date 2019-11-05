// http://mylcd.sourceforge.net/
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

 

/*
	VLCStream for VLC 2.0.x

Install:
Copy vlcstream.exe and the vsskin directory to the root VLC directory where vlc.exe is located

Start with: 'vlcstream.exe "path/to/mediafile.ext"' or use the built-in explorer to initiate media playback
*/



#include "common.h"
#include <rgbmasks.h>
#if !RELEASEBUILD
#include <conio.h>
#endif



typedef struct{
	TPAGE2COM *com;
}TPAGEVIDEO;



//TVLCPLAYER *g_vp = NULL;

volatile int SHUTDOWN = 0;
volatile double UPDATERATE_BASE = UPDATERATE_BASE_DEFAULT;

static volatile int MediaPlayerStopped_complete = 0;


static inline inline void _imageBestFit (const int bg_w, const int bg_h, int fg_w, int fg_h, int *w, int *h);
void vlc_configure (TVLCPLAYER *vp, TVLCCONFIG *vlc, const int width, const int height, const int bpp, const int visMode);
int vlc_configureMediaOptions (TVLCCONFIG *vlc, char *utf8Path);
char *vlc_configureGetOptions (char *utf8Path);
static inline void exitAppl (TVLCPLAYER *vp);
static inline int browserLoadMediaFile (TVLCPLAYER *vp, char *utf8path, char *opts);



int cursorGetState (TGUIINPUT *cursor)
{
	return cursor->draw;
}

int cursorSetState (TGUIINPUT *cursor, const int state)
{
	int olds = cursor->draw;
	cursor->draw = state;
	return olds;
}

/*
void resetPageAccessed (TVLCPLAYER *vp, const int pageId)
{
	vp->gui.pageAccessed[pageId-PAGE_BASEID] = 0;
}
*/

void setPageAccessed (TVLCPLAYER *vp, const int pageId)
{
	vp->gui.pageAccessed[pageId-PAGE_BASEID] = 1;
}

int hasPageBeenAccessed (TVLCPLAYER *vp, const int pageId)
{
	return vp->gui.pageAccessed[pageId-PAGE_BASEID];
}

double getBaseUpdateRate ()
{
	return UPDATERATE_BASE;
}

void setBaseUpdateRate (const double rate)
{
	UPDATERATE_BASE = rate;
}


//void _setTargetRate (TVLCPLAYER *vp, const double fps, const char *func, const int line)
void setTargetRate (TVLCPLAYER *vp, const double fps)
{
	//printf("setTargetRate %.2f, %i:'%s'\n", fps, line, func);
	//printf("setTargetRate %f\n", fps);
	
	vp->gui.targetRate = fps;
}

#if 1
// this is broken
// play logic needs a complete rewrite.. everywhere
int getPlayState (TVLCPLAYER *vp)
{
	//return (getConfig(vp)->playState) && (getConfig(vp)->playState != 8);
	return getConfig(vp)->playState;
}
#endif

int isMediaFile (const char *name)
{
	return (stristr(name, "file://") != NULL);
}

static inline int isMediaMRL (const char *name)
{
	return (strstr(name, "://") != NULL);
}

int isMediaDVB (const char *name)
{
	int ret =((stristr(name, "dvb-t://"))	||
			  (stristr(name, "dvb-s://"))	||
			  (stristr(name, "dvb-c://"))	||
			  (stristr(name, "dvb-t2://"))	||
			  (stristr(name, "dvb-s2://"))	||
			  (stristr(name, "atsc://"))	||
			  (stristr(name, "cqam://"))	||
			  isTsVideo8(name));
	return ret;
}

int isMediaRTSP (const char *name)
{
	return (stristr(name, "rtsp://") != NULL);
}

int isMediaMMS (const char *name)
{
	return (stristr(name, "mms://") != NULL);
}

int isMediaUDP (const char *name)
{
	return (stristr(name, "udp://") != NULL);
}

int isMediaHTTP (const char *name)
{
	return (stristr(name, "http://") != NULL);
}

int isMediaDShow (const char *name)
{
	return (stristr(name, "dshow://") != NULL);
}

int isMediaDVD (const char *name)
{
	return isDVDLocation(name) || (stristr(name, "dvd://") != NULL);
}

int isMediaScreen (const char *name)
{
	return (stristr(name, "screen://") != NULL);
}

int isMediaRemote (const char *name)
{
	return (isMediaHTTP(name) ||
			isMediaUDP(name)  ||
			isMediaRTSP(name) ||
			isMediaMMS(name));
}

int isMediaLocal (const char *name)
{
	const char *extTags[] = {EXTAUDIOA, EXTVIDEOA, ""};

	return (hasPathExtA(name, extTags) ||
			isMediaFile(name) ||
			isMediaDVD(name) ||
#if 0
			isMediaDVB(name) ||		/* unsure if this should be here */
#endif
			isMediaScreen(name) ||
			isMediaDShow(name));
}

int isMediaPlaylist (const char *name)
{
	const char *extTags[] = {EXTPLAYLISTSA, ""};
	return (hasPathExtA(name, extTags));
}

// TIMER_SHUTDOWN
void shutdownAppl (TVLCPLAYER *vp)
{
	exitAppl(vp);
}

static inline void setIdle (TVLCPLAYER *vp)
{
	//printf("setIdle\n");
	
	vp->gui.awake = 0;
	page2Set(vp->pages, PAGE_CLOCK, 1);
}

// return 1/true if in idle mode, otherwise 0
int getIdle (TVLCPLAYER *vp)
{
	return (vp->gui.awake == 0);
}

// TIMER_PLAY
void timer_play (TVLCPLAYER *vp)
{
	if (playlistManagerGetTotal(vp->plm) <= 1){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, 0);
		if (!plc || playlistGetCount(plc, PLAYLIST_OBJTYPE_TRACK) < 1)
			return;
	}

	ctrlPlayback(vp, VBUTTON_PLAY);
}

// TIMER_STOPPLAY
void timer_stopplay (TVLCPLAYER *vp)
{
	trackStop(vp);
	timer_play(vp);
}

void updateModuleRegPathEntry (TVLCPLAYER *vp)
{
	wchar_t szPath[MAX_PATH+1];
	GetModuleFileNameW(NULL, szPath, MAX_PATH);
	//wprintf(L"#%s#\n", szPath);
	setInstallPath(szPath, wcslen(szPath)*sizeof(wchar_t));
}

// TIMER_SETIDLEA
void timer_setIdleA (TVLCPLAYER *vp)
{
	//printf("timer_setIdleA\n");
	
	vp->gui.awake = 0;
	vp->gui.idleDisabled = 0;
	page2Set(vp->pages, PAGE_CLOCK, 1);

	if (mHookGetState()){
		captureMouse(vp, 0);
		mHookUninstall();
	}
	if (kHookGetState()){
		kHookUninstall();
	}

#if 0		/* could set this as an option.. 'stop paused playback on idle' */
	if (getPlayState(vp)){
		trackStop(vp);
		unloadMedia(vp, vp->vlc);
	}
#endif

	timerSet(vp, TIMER_SETIDLEB, 300);
	renderSignalUpdate(vp);
}

// TIMER_SETIDLEB
void timer_setIdleB (TVLCPLAYER *vp)
{
	//printf("timer_setIdleB\n");
	
	vp->gui.idleDisabled = 0;
	vp->gui.awake = 0;
	vp->gui.frameCt = 0;

	setIdle(vp);
	updateTickerStart(vp, vp->settings.general.idleFps);
	timerSet(vp, TIMER_SETIDLEC, 120*60*1000);
	renderSignalUpdate(vp);

	//tagFlushOrfhensPlm(vp->tagc, vp->plm);
}

// TIMER_SETIDLEC
void timer_setIdleC (TVLCPLAYER *vp)
{
	//printf("timer_setIdleC %i\n", vp->gui.awake);
	
	if (vp->gui.awake) return;
	
#if 1

	strcFlush(vp->strc);
	imageManagerFlush(vp->im);
	
	if (renderLock(vp)){
		artworkFlush(vp, vp->am);
		renderUnlock(vp);
	}

	invalidateShadows(vp->gui.shadow);
	ccLabelFlushAll(vp->cc);
	libmylcd_FlushFonts(vp->ml->hw);

#else
	printf("stringCache %i\n", vp->strc->totalAdded);
	strcFlush(vp->strc);
	int flushed = ccLabelFlushAll(vp->cc);
	printf("ccLabelFlushAll flushed %i images\n", flushed);

	flushed = imageManagerFlush(vp->im);
	printf("im Flush flushed %i images\n", flushed);

	flushed = libmylcd_FlushFonts(vp->ml->hw);
	printf("libmylcd_FlushFonts flushed %i chars\n", flushed);
		
	if (renderLock(vp)){
		printf("6 in\n");
  		flushed = artworkFlush(vp, vp->am);
  		printf("6 out\n");
		renderUnlock(vp);
	}
  	printf("artworkFlush flushed %i images\n", flushed);

#endif

	tagFlushOrfhensPlm(vp->tagc, vp->plm);
	vlcEventListInvalidate(vp->vlc);

  	//timerSet(vp, TIMER_PATHREGWRITE, 500);
  	updateModuleRegPathEntry(vp);
  	//timerSet(vp, TIMER_SAVECONFIG, 120*60*1000);
  	
}

// TIMER_SAVECONFIG
void timer_saveConfig (TVLCPLAYER *vp)
{
	pageDispatchMessage(vp->pages, PAGE_MSG_CFG_WRITE, 0, 0, NULL);
	configSave(vp, CFGFILE);
}

// TIMER_PLAYTRACK
void timer_playtrack (TVLCPLAYER *vp)
{
	TTIMERPLAYTRACK *tpt = &vp->gui.playtrack;
	startPlaylistTrackUID(vp, tpt->uid, tpt->track);
}

void stateHelper (TVLCPLAYER *vp)
{
	const int page = pageGet(vp);
	const int isIdle = getIdle(vp);

	if (!isIdle){
		if (page == PAGE_HOME){
			if (vp->gui.hotkeys.alwaysAccessible){
				applLaunchSetPageIconState(vp, PAGE_HOTKEYS, 1);
			}else{
				vp->gui.hotkeys.isVlcRunning = ghkIsVlcRunning();
				applLaunchSetPageIconState(vp, PAGE_HOTKEYS, vp->gui.hotkeys.isVlcRunning);
			}
		}else if (page == PAGE_HOTKEYS){
			TGLOBALHOTKEYS *ghk = pageGetPtr(vp, PAGE_HOTKEYS);
			
			if (vp->gui.hotkeys.alwaysAccessible)
				vp->gui.hotkeys.isVlcRunning = ghkIsVlcRunning();
			//if (ghk->isVlcRunning)
				ghkGetRunningVLCWindowTitle(FindWindowExW(0, 0, L"QWidget", NULL), ghk->vlcTitle, MAX_PATH_UTF8);
		}
	}

	timerSet(vp, TIMER_STATEHELPER, 2000);
}


// TIMER_FLUSH
void timer_flushcaches (TVLCPLAYER *vp)
{
	//printf("timer_flushcaches\n");
	
	int page = pageGet(vp);
	if (page == PAGE_SEARCH){
		//artManagerFlush(vp->am);
		artManagerFlush(vp->im);
		pageDispatchMessage(vp->pages, PAGE_MSG_IMAGE_FLUSH, page, 0, NULL);
		return;
	}
	
	dbprintf(vp, "stringCache flushed: %i %iKb", (int)vp->strc->totalAdded, (int)strcGetStoredSize(vp->strc)/1024);
	strcFlush(vp->strc);

	int ct = imageManagerFlush(vp->im);
	dbprintf(vp, "imageManagerFlush: %i", ct);
	
	ct = artManagerFlush(vp->am);
	dbprintf(vp, "artManagerFlush: %i", ct);

	invalidateShadows(vp->gui.shadow);
	dbprintf(vp, "shadows cache flushed");
	
	ct = ccLabelFlushAll(vp->cc);
	dbprintf(vp, "labels flushed: %i", ct);
	
	dbprintf(vp, "chars flushed: %i", libmylcd_FlushFonts(vp->ml->hw));
	pageDispatchMessage(vp->pages, PAGE_MSG_IMAGE_FLUSH, page, 0, NULL);
	
	renderSignalUpdate(vp);
}

// TIMER_testingonly, debug use only
void timertest (TVLCPLAYER *vp)
{
}

void artcleanup (TVLCPLAYER *vp)
{
	//printf("artcleanup in: %i\n", (int)GetCurrentThreadId());
		
	if (renderLock(vp)){
		artworkFlush(vp, vp->am);
		renderUnlock(vp);
	}
	timerSet(vp, TIMER_ARTCLEANUP, ARTWORKFLUSH_PERIOD + (5*1000));
	
	//printf("artcleanup out: %i\n", (int)GetCurrentThreadId());
}

void setApplState (TVLCPLAYER *vp, int state)
{
	vp->applState = state;
	if (!state) SHUTDOWN = 1;

	//printf("SHUTDOWN == %i\n", SHUTDOWN);
}

/*
static int getApplState (TVLCPLAYER *vp)
{
	return vp->applState;
}
*/
/*
// must not be called from render thread
void waitForRenderUpdate (TVLCPLAYER *vp, const unsigned int timeout)
{
	// scyncronise render thread with this
	renderLock(vp);
	int fps0 = vp->gui.frameCt;
	renderUnlock(vp);

	// force a render pass
	renderSignalUpdate(vp);

	// wait for render thread to complete a new pass
	int ttime = timeout;
	while (ttime > 0 && fps0 == vp->gui.frameCt && getApplState(vp)){
		double t0 = getTime(vp);
		lSleep(5);
		ttime -= (int)(getTime(vp)-t0);
	}

	renderSignalUpdate(vp);

	fps0 = vp->gui.frameCt;
	ttime = timeout;
	while (ttime > 0 && fps0 == vp->gui.frameCt && getApplState(vp)){
		double t0 = getTime(vp);
		lSleep(5);
		ttime -= (int)(getTime(vp)-t0);
	}
}*/

int loadLock (TVLCPLAYER *vp)
{
	int ret = lockWait(vp->gui.hLoadLock, INFINITE);
	return ret;
}

void loadUnlock (TVLCPLAYER *vp)
{
	lockRelease(vp->gui.hLoadLock);
}

int renderLock (TVLCPLAYER *vp)
{
	int ret = lockWait(vp->gui.hRenderLock, INFINITE);
	return ret;
}

void renderUnlock (TVLCPLAYER *vp)
{
	lockRelease(vp->gui.hRenderLock);
}

double getTime (TVLCPLAYER *vp)
{
	uint64_t t1 = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&t1);
	return ((double)((uint64_t)(t1 - vp->tStart) * vp->resolution) * 1000.0);
}

uint64_t getTime64 (TVLCPLAYER *vp)
{
	uint64_t t1 = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&t1);
	return ((uint64_t)((uint64_t)(t1 - vp->tStart) / (double)vp->freq/** vp->resolution*/) * 1000);
}

// leave idle mode
void setAwake (TVLCPLAYER *vp)
{
	if (!vp->gui.awake){
		updateTickerStart(vp, UPDATERATE_ALIVE);
		vp->gui.frameCt = 0;
	}

	vp->gui.awake = 1;
	vp->gui.awakeTime = timeGetTime();
}

void wakeup (TVLCPLAYER *vp)
{
	if (!vp->gui.awake){
		updateTickerStart(vp, UPDATERATE_ALIVE);
		page2Set(vp->pages, PAGE_NONE, 0);
		renderSignalUpdate(vp);
	}
	setAwake(vp);
}

static inline int isMediaLoaded (TVLCPLAYER *vp)
{
	return vp->vlc->isMediaLoaded;
}

int setPlaybackRandom (TVLCPLAYER *vp, const int state, const int report)
{
	TVLCCONFIG *vlc = getConfig(vp);
	
	if (state == -1)
		vlc->playRandom ^= 1;
	else
		vlc->playRandom = state;

	if (report){
		if (!vlc->playRandom)
			dbprintf(vp, "Random track playback is disabled");
		else
			dbprintf(vp, "Random track playback is enabled");
	}
		
	return vlc->playRandom;
}

int getPlaybackRandom (TVLCPLAYER *vp)
{
	TVLCCONFIG *vlc = getConfig(vp);
	return vlc->playRandom;
}

void setPlaybackMode (TVLCPLAYER *vp, const int mode)
{
	//printf("setPlaybackMode %i, %i\n", mode, vp->currentFType);
	
	/*
	0:audio or video with visualizations disabled
	1:audio with visuals enabled
	*/
	vp->currentFType = mode;
	
#if 0
	vp->currentFType = PLAYBACKMODE_AUDIO;
	vp->gui.visual = 1;
#endif
}

int getPlaybackMode (TVLCPLAYER *vp)
{
	return vp->currentFType;
}

static inline TVLCCONFIG *selectVLCConfig (TVLCPLAYER *vp)
{
	return (vp->vlc = &vp->vlcconfig);
}

int startVlc (TVLCPLAYER *vp)
{
	char *vars;
	settingsGet(vp, "vlc.startArguments", &vars);
	
	char cmdline[MAX_PATH_UTF8+1];
	//--qt-system-tray --vout=any TV_all.xspf
	__mingw_snprintf(cmdline, MAX_PATH_UTF8, "vlc.exe %s", vars);
	my_free(vars);

	if (processCreate(cmdline)){
		if (mHookGetState()){
			captureMouse(vp, 0);
			mHookUninstall();
		}
		return 1;
	}
	return 0;
}

int startVlcTrackPlayback (TVLCPLAYER *vp)
{
	const double position = vp->vlc->position;
	MediaPlayerStopped_complete = -1;
	int success = 0;
	int stopOnVlcPlayback = 1;


	settingsGet(vp, "vlc.stopOnVlcPlayback", &stopOnVlcPlayback);
	if (stopOnVlcPlayback){
		if (getPlayState(vp) != 2){
			trackStop(vp);
			//printf("vp->vlc->playEnded %i\n", vp->vlc->playEnded);
			while(MediaPlayerStopped_complete != 1){
				if (renderLock(vp)){
					timerCheckAndFire(vp, getTime(vp));
					renderUnlock(vp);
				}
				lSleep(5);
			}
			lSleep(25);
			//printf("vp->vlc->playEnded %i %i %f\n", vp->vlc->playEnded, vp->vlc->playState, vp->vlc->position );
		}

	}
	MediaPlayerStopped_complete = 0;
	
	wchar_t *vars;
	settingsGetW(vp, "vlc.playbackArguments", &vars);

	wchar_t *pathw = getPlayingPathW(vp);
	if (pathw){
		const wchar_t *cl = L"vlc.exe \"%ls\" :start-time=%f %ls";
		int len = __mingw_snwprintf(NULL, 0, cl, pathw, position*(double)vp->vlc->length, vars);
		if (len > 1){
			//int len = 2047;
			wchar_t cmdline[len+1];
			 __mingw_snwprintf(cmdline, len, cl, pathw, position*(double)vp->vlc->length, vars);

			success = processCreateW(cmdline);
			if (success && mHookGetState()){
				captureMouse(vp, 0);
				mHookUninstall();
			}

		}
		my_free(pathw);
	}

	my_free(vars);
	return success;
}

int startPlaylistTrack (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int track)
{
	//printf("startPlaylistTrack %i: alb:'%s'\n", track, plc->title);

	if (playlistGetItemType(plc, track) != PLAYLIST_OBJTYPE_TRACK){
		//printf("%i is not a track\n", track);
		return 0;
	}

	int ret = 0;
	if (track < 0) return 0;

	if (playlistLock(plc)){
		char path[MAX_PATH_UTF8+1];

		playlistGetPath(plc, track, path, MAX_PATH_UTF8);
		playlistUnlock(plc);
		
		if (*path){
			wchar_t *out = converttow(path);
			if (out){
				if (!isVideoFile(out)){	// filter type must be set before track is started
					filepaneSetFilterMask(pageGetPtr(vp, PAGE_FILE_PANE), FILEMASKS_AUDIO);
					setPlaybackMode(vp, 1);
				}else{
					filepaneSetFilterMask(pageGetPtr(vp, PAGE_FILE_PANE), FILEMASKS_VIDEO);
					setPlaybackMode(vp, 0);
				}

				char opts[MAX_PATH_UTF8+1];
				int pos = playlistGetPositionByHash(plc, getHash(path));
				if (pos >= 0)
					playlistGetOptions(plc, pos, opts, MAX_PATH_UTF8);

				if (browserLoadMediaFile(vp, path, opts)){
					setQueuedPlaylist(vp, plc);
					trackLoadEvent(vp, plc, track);
					ret = 1;
				}

				my_free(out);
			}
		}
		
		//playlistUnlock(plc);
	}
	return ret;
}

int startPlaylistTrackUID (TVLCPLAYER *vp, const int uid, const int track)
{
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	if (plc)
		return startPlaylistTrack(vp, plc, track);
	else
		return 0;
}

static inline void player_pause (TVLCPLAYER *vp, TVLCCONFIG *vlc)
{
	vlc_pause(vlc);
}

static inline void player_stop (TVLCPLAYER *vp, TVLCCONFIG *vlc)
{
	vlc_stop(vlc);
 	unloadMedia(vp, vlc);
 	vlc->playState = 0;
 	vlc->playEnded = 0;
}

static inline void player_play (TVLCPLAYER *vp, TVLCCONFIG *vlc)
{
	//printf("player_play \n");

	if (vlc->playEnded || vlc->playState != 2)
		vlc_stop(vlc);

	vlc->playState = 1;
	vlc->playEnded = 0;
	vlc_play(vlc);
}

static inline int player_prevTrack (TVLCPLAYER *vp, TVLCCONFIG *vlc)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	PLAYLISTRENDER *pr = plc->pr;

	int iprev = playlistGetPrevItem(plc, PLAYLIST_OBJTYPE_TRACK, pr->playingItem);
	if (iprev == -1) return -1;

	pr->playingItem = iprev;
	startPlaylistTrack(vp, plc, pr->playingItem);
	return pr->playingItem;
}

static inline int player_nextTrack (TVLCPLAYER *vp, TVLCCONFIG *vlc)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	PLAYLISTRENDER *pr = plc->pr;

	int inext = playlistGetNextItem(plc, PLAYLIST_OBJTYPE_TRACK, pr->playingItem);
	if (inext == -1) return -1;

	pr->playingItem = inext;
	startPlaylistTrack(vp, plc, pr->playingItem);
	return pr->playingItem;
}

static inline int player_randomTrack (TVLCPLAYER *vp, TVLCCONFIG *vlc)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	PLAYLISTRENDER *pr = plc->pr;

	const int total = playlistGetTotal(plc);
	int trklist[total];
	for (int i = total-1; i >= 0; i--)
		trklist[i] = vlc_lrand48()%total;

	int trk = trklist[vlc_lrand48()%total];
	//printf("random %i, %i %i\n", pr->playingItem, trk, total);

	if (trk >= playlistGetTotal(plc)) trk = 0;
	pr->playingItem = trk;
	startPlaylistTrack(vp, plc, pr->playingItem);
	return pr->playingItem;
}

// TIMER_PREVTRACK
void trackPrev (TVLCPLAYER *vp)
{

	PLAYLISTCACHE *plcQ = getQueuedPlaylist(vp);

	if (plcQ){
		int prev = player_prevTrack(vp, vp->vlc);
		if (prev >= 0){
			trackLoadEvent(vp, plcQ, prev);
		}
	}

}

// TIMER_NEXTTRACK
void trackNext (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plcQ = getQueuedPlaylist(vp);
	if (plcQ){
		int next = player_nextTrack(vp, vp->vlc);
		if (next >= 0)
			trackLoadEvent(vp, plcQ, next);
	}
}

// TIMER_REWIND
void trackRewind (TVLCPLAYER *vp)
{
	//printf("trackRewind\n");
	
	if (getPlayState(vp) && getPlayState(vp) != 8){
		const double tskip = 3.00;
		if (tskip > 0.00000){
			double pos = 0.0;
			double tlen = vp->vlc->length;
			double dt = (1.0/tlen) * tskip;
			pos = vp->vlc->position - dt;

			clipFloat(pos);
			vp->vlc->position = pos;
			vlc_setPosition(vp->vlc, vp->vlc->position);
		}
	}
	//timerSet(vp, TIMER_FASTFORWARD, 250);
}

// TIMER_FASTFORWARD
void trackFastforward (TVLCPLAYER *vp)
{
	//printf("trackFastforward\n");
	
	if (getPlayState(vp) && getPlayState(vp) != 8){
		const double tskip = 3.00;
		if (tskip > 0.00000){
			double pos = 0.0;
			double tlen = vp->vlc->length;
			double dt = (1.0/tlen) * tskip;
			pos = vp->vlc->position + dt;
			clipFloat(pos);
			vp->vlc->position = pos;
			vlc_setPosition(vp->vlc, vp->vlc->position);
		}
	}
	//timerSet(vp, TIMER_FASTFORWARD, 250);
}

// TIMER_PAUSE
void trackPause (TVLCPLAYER *vp)
{
	if (getPlayState(vp) == 1){
  		vp->vlc->playState = 2;
		player_pause(vp, vp->vlc);
	}
}

// TIMER_PLAYPAUSE
void trackPlayPause (TVLCPLAYER *vp)
{
	if (getPlayState(vp) == 1){
  		vp->vlc->playState = 2;
		player_pause(vp, vp->vlc);
	}else if (getPlayState(vp) == 2){
		player_play(vp, vp->vlc);
	}
}

// TIMER_PLAY
void trackPlay (TVLCPLAYER *vp)
{
	player_play(vp, vp->vlc);
}

// TIMER_STOP
void trackStop (TVLCPLAYER *vp)
{
	player_stop(vp, vp->vlc);
}


// TIMER_VOL_MASTER_UP
void volumeWinUp (TVLCPLAYER *vp)
{
	//printf("volumeWinUp %i\n", getVolume(vp, VOLUME_MASTER));
	
  	setVolume(vp, getVolume(vp, VOLUME_MASTER) + 5, VOLUME_MASTER);
	//overlaySetOverlay(vp);
}

// TIMER_VOL_MASTER_DN
void volumeWinDown (TVLCPLAYER *vp)
{
	//printf("volumeWinDown %i\n", getVolume(vp, VOLUME_MASTER));
	
	setVolume(vp, getVolume(vp, VOLUME_MASTER) - 5, VOLUME_MASTER);
	//overlaySetOverlay(vp);
}


// TIMER_VOL_APP_UP
// todo: change the hardcoded 5 to a config setting
void volumeUp (TVLCPLAYER *vp)
{
  	setVolume(vp, vp->vlc->volume + 5, VOLUME_APP);
	//overlaySetOverlay(vp);
}

// TIMER_VOL_APP_DN
void volumeDown (TVLCPLAYER *vp)
{
	setVolume(vp, vp->vlc->volume - 5, VOLUME_APP);
	//overlaySetOverlay(vp);
}

#if 0
void switchVis (TVLCPLAYER *vp)
{
  	if (getPlayState(vp)){
 		char *path = getPlayingPath(vp);
  		if (path){
  			const int volume = vp->vlc->volume;
  			setVolume(vp, 0, VOLUME_APP);
  			const float position = vlc_getPosition(vp->vlc);
  			trackStop(vp);
  			cleanVideoBuffers(vp);

  			char opts[MAX_PATH_UTF8+1];
			opts[0] = 0;

			PLAYLISTCACHE *plcQ = getQueuedPlaylist(vp);
			int pos = playlistGetPositionByHash(plcQ, getHash(path));
			if (pos >= 0)
				playlistGetOptions(plcQ, pos, opts, MAX_PATH_UTF8);
  			browserLoadMediaFile(vp, path, opts);
	  		vlc_setPosition(vp->vlc, position);
			setVolume(vp, volume, VOLUME_APP);
			my_free(path);
  		}
  	}
}
#endif

static inline int waitForUpdateSignal (TVLCPLAYER *vp)
{
	unsigned int period;
	if (getIdle(vp))
		period = 2000;
	else
		period = 75;
	return (WaitForSingleObject(vp->gui.hUpdateEvent, period) == WAIT_OBJECT_0);
}

static inline void renderSignalVideoFrameUpdate (TVLCPLAYER *vp)
{
	SetEvent(vp->ctx.hEvent);
}

static inline int waitForVLCUpdateSignal (TVLCPLAYER *vp)
{
	return (WaitForSingleObject(vp->ctx.hEvent, 1) == WAIT_OBJECT_0);
}

static inline void lockVLCVideoBuffer (TVLCPLAYER *vp)
{
	lockWait(vp->ctx.hVideoLock, INFINITE);
}

static inline void unlockVLCVideoBuffer (TVLCPLAYER *vp)
{
	lockRelease(vp->ctx.hVideoLock);
}

static inline int isVideoFrameAvailable (TVLCPLAYER *vp)
{
	return 	(isMediaLoaded(vp)		&&
			(getPlayState(vp) >  0)	&&
			(getPlayState(vp) != 2) &&
			 getApplState(vp)		&&
			 vp->renderState		&&
			(getPlaybackMode(vp) == PLAYBACKMODE_VIDEO/* || (getPlaybackMode(vp) == PLAYBACKMODE_AUDIO && vp->gui.visual)*/));
}

void *vmem_lock (void *data, void **pp_ret)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)data;
	
    lockVLCVideoBuffer(vp);
    pp_ret[0] = vp->ctx.pixels;
    return vp->ctx.pixels;
}

void vmem_unlock (void *data, void *id, void *const *p_pixels)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)data;
    unlockVLCVideoBuffer(vp);

	double t1 = getTime(vp);
	if (++vp->vlc->fIndex >= 30) vp->vlc->fIndex = 0;
	vp->vlc->dTime[vp->vlc->fIndex] = t1 - vp->vlc->vTime0;
	vp->vlc->vTime0 = t1;

	renderSignalVideoFrameUpdate(vp);
	if (vp->currentFType != PLAYBACKMODE_AUDIO)
		renderSignalUpdate(vp);
}

/*
void vmem_display (void *data, void *id)
{
	printf("vmem_display %p\n", id);
	TVLCPLAYER *vp = (TVLCPLAYER*)data;

	float t1 = getTime(vp);
	if (++vp->vlc->fIndex >= 30) vp->vlc->fIndex = 0;
	vp->vlc->dTime[vp->vlc->fIndex] = t1 - vp->vlc->vTime0;
	vp->vlc->vTime0 = t1;
	SetEvent(vp->ctx.hEvent);
	renderSignalUpdate(vp);
}*/

void cleanVideoBuffers (TVLCPLAYER *vp)
{
	lockVLCVideoBuffer(vp);
	memset(lGetPixelAddress(vp->ctx.vmem, 0, 0), 0, vp->ctx.vmem->frameSize);
	memset(lGetPixelAddress(vp->ctx.working, 0, 0), 0, vp->ctx.working->frameSize);
	unlockVLCVideoBuffer(vp);
}

int playerWriteDefaultPlaylist (TVLCPLAYER *vp, const wchar_t *playlist)
{
	int cret = 0;
	
	PLAYLISTCACHE *plc = getPrimaryPlaylist(vp);
	if (plc && playlistGetTotal(plc)){
		TM3U *m3u = m3uNew();
		if (m3u){
			if (m3uOpen(m3u, VLCSPLAYLIST, M3U_OPENWRITE)){
#if !RELEASEBUILD
				printf("\n****** writing playlist *********\n");

				double t0 = getTime(vp);
				cret = m3uWritePlaylist(m3u, getPrimaryPlaylist(vp), vp->tagc, vp->am, 0);
				double t1 = getTime(vp);

				__mingw_wprintf(L"%i records written to %ls in %.2fms\n", cret, playlist, t1-t0);
				printf("*** writing playlist complete ***\n");
#else
				m3uWritePlaylist(m3u, getPrimaryPlaylist(vp), vp->tagc, vp->am, 0);
#endif
				m3uClose(m3u);
			}
			m3uFree(m3u);
		}
	}
	return cret;
}

#if !RELEASEBUILD
static inline int getKeyPress ()
{
	int ch = 1;
	if (kbhit())
		ch = getch();

	if (ch == 27 || ch == 13)	// escape
		return 0;
	else
		return ch;
}

#if 0
static int paneAlpha = 0xa4;
static int paneRed = 0x44;
static int paneGreen = 0x3c;
static int paneBlue = 0x5f;
//A4 44 3C 5F


void setPaneColour (TVLCPLAYER *vp)
{
	TEPG *epg = pageGetPtr(vp, PAGE_EPG);
	
	paneAlpha &= 0xFF;
	paneRed &= 0xFF;
	paneGreen &= 0xFF;
	paneBlue &= 0xFF;
	
	if (paneAlpha < 0) paneAlpha = 0;
	if (paneRed < 0) paneRed = 0;
	if (paneGreen < 0) paneGreen = 0;
	if (paneBlue < 0) paneBlue = 0;
	
	unsigned int colour = (paneAlpha<<24)|(paneRed<<16)|(paneGreen<<8)|paneBlue;
	
	printf("colour %X, %i %i %i %i\n", colour, paneAlpha, paneRed, paneGreen, paneBlue);
	
	labelBaseColourSet(epg->guide.paneContents->base, colour);
}
#endif

#if 0
int processKeyPress (TVLCPLAYER *vp, int ch)
{

	//PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	//PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	//PLAYLISTCACHE *plc = getPrimaryPlaylist(vp);

	if (ch == 'a'){

	}else if (ch == 'p'){
		playerWriteDefaultPlaylist(vp, VLCSPLAYLIST);
	}
	/*if (ch == 'a'){
		int orig = getQueuedPlaylistUID(vp);
		int uid = getQueuedPlaylistLeft(vp);
		int ret = setQueuedPlaylistByUID(vp, uid);
		printf("left: %X -> %X %X\n", orig, uid, ret);
		
	}else if (ch == 's'){
		int orig = getQueuedPlaylistUID(vp);
		int uid = getQueuedPlaylistRight(vp);
		int ret = setQueuedPlaylistByUID(vp, uid);
		printf("right: %X -> %X %X\n", orig, uid, ret);
	
	}else if (ch == 'w'){
		int orig = getQueuedPlaylistUID(vp);
		int uid = getQueuedPlaylistParent(vp);
		int ret = setQueuedPlaylistByUID(vp, uid);
		printf("up: %X ^-> %X %X\n", orig, uid, ret);
	
	}else if (ch == 'z'){
		printf("stringCache %i %iKb\n", (int)vp->strc->totalAdded, (int)strcGetStoredSize(vp->strc)/1024);
		strcFlush(vp->strc);

		int ct = imageManagerFlush(vp->im);
		printf("imageManagerFlush %i\n", ct);
		ct = artManagerFlush(vp->am);
		printf("artManagerFlush %i\n", ct);

		invalidateShadows(vp->gui.shadow);
		ccLabelFlushAll(vp->cc);
		printf("chars flushed %i\n", libmylcd_FlushFonts(vp->ml->hw));	

	}else if (ch == 'p'){
		playerWriteDefaultPlaylist(vp, VLCSPLAYLIST);
		*/
#if 0
	int dt = 4;
	if (ch == 'q'){
		paneAlpha += dt;
		setPaneColour(vp);
		
	}else if (ch == 'w'){
		paneRed += dt;
		setPaneColour(vp);
		
	}else if (ch == 'e'){
		paneGreen += dt;
		setPaneColour(vp);
		
	}else if (ch == 'r'){
		paneBlue += dt;
		setPaneColour(vp);

	}else if (ch == 'a'){
		paneAlpha -= dt;
		setPaneColour(vp);
		
	}else if (ch == 's'){
		paneRed -= dt;
		setPaneColour(vp);
		
	}else if (ch == 'd'){
		paneGreen -= dt;
		setPaneColour(vp);
		
	}else if (ch == 'f'){
		paneBlue -= dt;
		setPaneColour(vp);
		
	}else 
#endif
#if 0		
	}else if (ch == 'l'){
		printf("stringCache %i %iKb\n", vp->strc->totalAdded, strcGetStoredSize(vp->strc)/1024);
		strcFlush(vp->strc);

		int ct = imageManagerFlush(vp->im);
		printf("imageManagerFlush %i\n", ct);
		ct = artManagerFlush(vp->am);
		printf("artManagerFlush %i\n", ct);

		invalidateShadows(vp->gui.shadow);
		ccLabelFlushAll(vp->cc);
		printf("chars flushed %i\n", libmylcd_FlushFonts(vp->ml->hw));

	}else if (ch == 'l'){
		printf("chars flushed %i\n", libmylcd_FlushFonts(vp->ml->hw));
	}else if (ch == 'l'){
		printf("artwork flushed %i\n", artworkFlush(vp, vp->am));
		
	}else if (ch == 'l'){
		printf("\nimage manager:");
		artManagerDumpStats(vp->im);
		printf("\nart manager:");
		artManagerDumpStats(vp->am);

		int ct = 0;
		TCCOBJ *obj = vp->cc->objs;
		while((obj=obj->next)){
			if (obj->obj)
				ct++;
		}
		printf("cc objects %i %i\n", vp->cc->ccIdIdx, ct);

#endif
#if 0		
	}else if (ch == 'z'){
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		double sigma = spl->shelf->sigma - 0.025;
		if (sigma < 0.10) sigma = 1.0;
		shelfSetSigma(spl->shelf, sigma);
		renderSignalUpdate(vp);
		printf("sigma %f\n", sigma);
		
	}else if (ch == 'x'){
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		double sigma = spl->shelf->sigma + 0.025;
		shelfSetSigma(spl->shelf, sigma);
		renderSignalUpdate(vp);
		printf("sigma %f\n", sigma);
		
	}else if (ch == 'f'){
		printf("~~~~~~~~~~~~~~\n");
		my_MemStatsDump(vp->ml->hw);
		printf("~~~~~~~~~~~~~~\n");
	}
#endif
	return 1;
}
#endif

static inline void processConsoleInput (TVLCPLAYER *vp)
{
	int ch = getKeyPress();
	if (!ch){
		//timerSet(vp, TIMER_SHUTDOWN, 0);
  		exitInitShutdown(vp);
	}else if (ch > 1){
		//processKeyPress(vp, ch);
	}
}
#endif

/*static inline*/ double getFPS (TVLCPLAYER *vp)
{
	double t = 0.0;
	for (int i = 0; i < 16; i++)
		t += vp->dTime[i];
	t /= 16.0;
	return 1000.0/t;
}

static inline void drawFPSOverlay (TVLCPLAYER *vp, TFRAME *frame, const float fps, const int x, int y)
{
	//const unsigned int *col = swatchGetPage(vp, PAGE_OVERLAY);
	lSetForegroundColour(frame->hw, 0xFFFFFFFF);
	lSetCharacterEncoding(frame->hw, CMT_UTF8);
	outlineTextEnable(frame->hw, 0xFF000000);


	//unsigned int colour = lGetPixel(frame, vp->gui.cursor.dx-MOFFSETX, vp->gui.cursor.dy-MOFFSETY);
	const int showVideoFps = getPlaybackMode(vp) == PLAYBACKMODE_VIDEO/* || (getPlaybackMode(vp) == PLAYBACKMODE_AUDIO && vp->gui.visual)*/;
	y -= 72;
	if (showVideoFps) y -= 22;

	//lPrintf(frame, x-45, y, UPDATERATE_FONT, LPRT_CPY, "%.8X", colour);

	const uint64_t mem = processGetMemUsage(vp->pid);
	lPrintf(frame, x-10, y += 22, UPDATERATE_FONT, LPRT_CPY, "%.1f", (double)mem/1024.0/1024.0);

	if (showVideoFps){
		double t = 0.0;
		for (int i = 0; i < 30; i++) t += vp->vlc->dTime[i];
		t /= 30.0;
		if (t > 1.0)
			lPrintf(frame, x, y += 22, UPDATERATE_FONT, LPRT_CPY, "%.1f", 1000.0/t);
	}

	lPrintf(frame, x-4, y += 22, UPDATERATE_FONT, LPRT_CPY, "%.2f", vp->rTime);
	lPrintf(frame, x, y += 22, UPDATERATE_FONT, LPRT_CPY, "%.1f", fps);
	outlineTextDisable(frame->hw);
}

static inline void drawCursor (TVLCPLAYER *vp, TFRAME *frame, TGUI *gui)
{
	TFRAME *cur = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_POINTER]);
	//if (cur){
		int x = (gui->cursor.dx + MOFFSETX) - cur->width;		// right pointing for left handed
		//int x = gui->cursor.dx - MOFFSETX;					// left pointing for right handed
		drawImage(cur, frame, x, gui->cursor.dy-MOFFSETY, cur->width-1, cur->height-1);
		imageManagerImageRelease(vp->im, vp->gui.image[IMGC_POINTER]);
	//}
}


void renderStatsEnable (TVLCPLAYER *vp)
{
	vp->gui.drawStats = 1;
}

void renderStatsDisable (TVLCPLAYER *vp)
{
	vp->gui.drawStats = 0;
}

void renderStatsToggle (TVLCPLAYER *vp)
{
	vp->gui.drawStats ^= 1;
}


/*###########################################################################*/
/*###########################################################################*/
/*###########################################################################*/
void renderSignalUpdateNow (TVLCPLAYER *vp)
{
	vp->gui.updateSignaled = 0;
	SetEvent(vp->gui.hUpdateEvent);
	vp->lastRenderTime = getTime(vp);
}

//void _renderSignalUpdate (TVLCPLAYER *vp, const char *func, const int line)
void renderSignalUpdate (TVLCPLAYER *vp)
{
	//printf("renderSignalUpdate %i\n", (int)GetTickCount());
	//printf("update: %s:%i\n", func, line);
	vp->gui.updateSignaled = 1;
}

void (CALLBACK updateTickerCB)(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)dwUser;
	//printf("set event %i\n", getApplState(vp));

	if (getApplState(vp)){
		double t1 = getTime(vp);
		double lastRenderPeriod = (t1 - vp->lastRenderTime)/* - vp->rTime*/;
		double currentFps = (1.0/lastRenderPeriod)*1000.0;

		//printf("set event a %i %f\n", vp->gui.updateSignaled, currentFps);
		if (vp->gui.updateSignaled || currentFps - (currentFps*0.195) < vp->gui.targetRate){
			//printf("set event b\n");
			SetEvent(vp->gui.hUpdateEvent);
			//vp->gui.cursorMoved = 0;
			vp->gui.updateSignaled = 0;
			vp->lastRenderTime = t1;
		}
	}
}

void updateTickerStop (TVLCPLAYER *vp)
{
	//printf("updateTickerStop %i\n", vp->updateTimer);
	if (vp->updateTimer){
		timeKillEvent(vp->updateTimer);
		vp->updateTimer = 0;
	}
}

void updateTickerStart (TVLCPLAYER *vp, double fps)
{
	//printf("updateTickerStart %lf\n", fps);
	
	
	if (fps > 100.0) fps = 100.0;
	if (fps != vp->gui.lastUpdateRate){
		vp->gui.lastUpdateRate = fps;
		updateTickerStop(vp);

		if (!vp->updateTimer){
			vp->updateTimer = (int)timeSetEvent((1.0/fps)*1000, 5, updateTickerCB, (DWORD_PTR)vp, TIME_PERIODIC|TIME_KILL_SYNCHRONOUS);
			//printf("updateTickerStart Set %f\n", fps);
		}
	}else{
		//printf("updateTickerStart Not Set %f\n", fps);
	}
}

/*###########################################################################*/
/*###########################################################################*/
/*###########################################################################*/


static inline void _imageBestFit (const int bg_w, const int bg_h, int fg_w, int fg_h, int *w, int *h)
{
	const int fg_sar_num = 1; const int fg_sar_den = 1;
	const int bg_sar_den = 1; const int bg_sar_num = 1;

	if (fg_w < 1 || fg_w > 8191) fg_w = bg_w;
	if (fg_h < 1 || fg_h > 8191) fg_h = bg_h;
	*w = bg_w;
	*h = (bg_w * fg_h * fg_sar_den * bg_sar_num) / (float)(fg_w * fg_sar_num * bg_sar_den);
	if (*h > bg_h){
		*w = (bg_h * fg_w * fg_sar_num * bg_sar_den) / (float)(fg_h * fg_sar_den * bg_sar_num);
		*h = bg_h;
	}
}

static inline void copyImage (TFRAME *src, TFRAME *des, const int sw, const int sh, int dx, int dy, const int dw, const int dh)
{
	#define getPixelAddr32(f,x,y)	(f->pixels+(((y)*(const int)f->pitch)+((x)<<2)))

	int *sp, *dp;
	int y2;
	const double scaley = (double)dh / (double)sh;
	const double scalex = (double)dw / (double)sw;
	if (dx < 0) dx = 0;
	if (dy < 0) dy = 0;

	//printf("%i %i %i %i %i %i, %.3f %.3f\n", sw, sh, dx, dy, dw, dh, scalex, scaley);

	int xlookup[dw];
	for (int i = 0; i < dw; i++)
		xlookup[i] = i/scalex;

	for (int y = dy; y < dy+dh; y++){
		y2 = (y-dy)/scaley;
		sp = (int*)getPixelAddr32(src, 0, y2);
		dp = (int*)getPixelAddr32(des, dx, y);

		for (int x = 0; x < dw; x++)
			dp[x] = sp[xlookup[x]];
	}
}

static inline void copyVideo (TVLCPLAYER *vp, TVLCCONFIG *vlc, TFRAME *src, TFRAME *des, int ratio)
{

#define CW(a) ((double)(dh)*((double)a))
#define CH(a) ((double)(dw)/((double)a))
#define CX(a) ((double)((dw)-CW((a)))/(double)2.0)
#define CY(a) ((double)((dh)-CH((a)))/(double)2.0)

#define copy(a)													\
	if (CY((a)) < 0.0)											\
		copyImage(src, des, dw, dh, CX((a)), 0, CW((a)), dh);	\
	else														\
		copyImage(src, des, dw, dh, 0, CY((a)), dw, CH((a)))


	const int dw = des->width;
	const int dh = des->height;


	switch (ratio){
	  case BTN_CFG_AR_AUTO:{
		int w, h;
		_imageBestFit(dw, dh, vlc->videoWidth, vlc->videoHeight, &w, &h);

		if (w < dw-2 || h < dh-2){
			int x = 0, y = 0;
			if (w < dw-2) x = (dw-w)/2.0;
			if (h < dh-2) y = (dh-h)/2.0;

	  		//printf("copyVideo %ix%i, %ix%i, %ix%i\n", dw, dh, vlc->videoWidth, vlc->videoHeight, w, h);

			memset(des->pixels, 0, des->frameSize);
			copyImage(src, des, dw, dh, x, y, w, h);
			break;
		}else{
			my_memcpy((uint32_t*)des->pixels, (uint32_t*)src->pixels, des->frameSize);
			break;
		}
	  }
	  case BTN_CFG_AR_CUSTOM:
	  	if (vp->ctx.aspect.clean)
	  		memset(des->pixels, 0, des->frameSize);

	  	if (vp->ctx.aspect.ratio > 0.1){
	  		copy(vp->ctx.aspect.ratio);
	  	}else{
	  		int width = vp->ctx.aspect.width;
	  		if (width > dw) width = dw;
	  		int x = vp->ctx.aspect.x;
	  		if (x+width > dw) x = 0;

	  		int height = vp->ctx.aspect.height;
	  		if (height > dh) height = dh;
	  		int y = vp->ctx.aspect.y;
	  		if (y+height > dh) y = 0;

			if (dw == width && dh == height && !x && !y){
				my_memcpy((uint32_t*)des->pixels, (uint32_t*)src->pixels, des->frameSize);
			}else{
	  			copyImage(src, des, dw, dh, x, y, width, height);
	  		}
	  	}
		break;
	  case BTN_CFG_AR_177:
		memset(des->pixels, 0, des->frameSize);
	  	copy(1.77777);
	  	break;
	  case BTN_CFG_AR_125:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.25);
		break;
	  case BTN_CFG_AR_133:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.333);
		break;
	  case BTN_CFG_AR_122:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.222);
		break;
	  case BTN_CFG_AR_15:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.5);
		break;
	  case BTN_CFG_AR_16:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.6);
		break;
	  case BTN_CFG_AR_167:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.6667);
		break;
	  case BTN_CFG_AR_143:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.43);
		break;
	  case BTN_CFG_AR_185:
	  	//printf("%i %i, %.1f %.1f :%f %f %f\n",dw, dh, CW(1.85), CH(1.85), CY(1.85), (dw/dh) * (CW(1.85)/CH(1.85)),(dw/dh) / (CW(1.85)/CH(1.85)) );
	  	memset(des->pixels, 0, des->frameSize);
	  	copy(1.85);
		break;
	  case BTN_CFG_AR_220:
	  	memset(des->pixels, 0, des->frameSize);
		copy(2.20);
		break;
	  case BTN_CFG_AR_233:
	  	memset(des->pixels, 0, des->frameSize);
		copy(2.33);
		break;
	  case BTN_CFG_AR_240:
	  	memset(des->pixels, 0, des->frameSize);
		copy(2.40);
		break;
	  case BTN_CFG_AR_155:
		copyAreaScaled(src, des, 0, (dh/100.0)*7.2, dw, (dh/100.0)*85.8, 0, 0, dw, dh);
		break;
	};
}


#if 0
void getAudioOutputDetails (TVLCCONFIG *vlc)
{
#if 1

	libvlc_audio_output_t *aud = libvlc_audio_output_list_get(vlc->hLib);
	if (aud){
		printf("audio device list:\n");

		libvlc_audio_output_t *a = aud;
		while(a){
			int acount = libvlc_audio_output_device_count(vlc->hLib, a->psz_name);
			if (acount){
				printf("%i: '%s', '%s'\n",acount, a->psz_name, a->psz_description);
				for (int i = 0; i < acount; i++){
					char *longname = libvlc_audio_output_device_longname(vlc->hLib, a->psz_name, i);
					char *id = libvlc_audio_output_device_id(vlc->hLib, a->psz_name, i);
					printf("   %i: '%s'\n        '%s'\n", i, id, longname);

					if (i == 2){
						libvlc_audio_output_set(vlc->mp, a->psz_name);
						libvlc_audio_output_device_set(vlc->mp, a->psz_name, id);
						printf("### device set to: -%s- -%s-\n", a->psz_name, id);
					}

					free(longname);
					free(id);
				}
			}
			a = a->p_next;
		}
		libvlc_audio_output_list_release(aud);
	}

#if 0
	static const char *audio_output_device_types[] = {
    	"Error",
    	"Mono",
    	"Stereo",
    	"Error",
	    "2F2R",
	    "3F2R",
	    "5_1",
	    "6_1",
	    "7_1",
	    "Error",
	    "SPDIF",
	    ""};

	int otype = libvlc_audio_output_get_device_type(vlc->mp);
	if (otype < 0 || otype > 10) otype = 0;
	printf("\naudio output device type: '%s'\n",audio_output_device_types[otype]);
#endif

#endif

#if 0
	int aTracks = libvlc_audio_get_track_count(vlc->mp);
	printf("number of audio tracks: %i\n", aTracks);

	libvlc_track_description_t *trks = libvlc_audio_get_track_description(vlc->mp);
	if (trks){
		libvlc_track_description_t *t = trks;

		while(t){
			printf("  '%i' '%s'\n", t->i_id, t->psz_name);
			t = t->p_next;
		}
		libvlcTrackDescriptionRelease(trks);
		printf("current audio track: %i\n", libvlc_audio_get_track(vlc->mp));
	}
	printf("\n");
#endif
}
#endif

void getNewTrackVariables (TVLCPLAYER *vp)
{
	if (!getApplState(vp) || !getPlayState(vp))
		return;
		
	static uint64_t tLast = 0;
	TVLCCONFIG *vlc = getConfig(vp);
	if (!vlc->mp) return;

	uint64_t t0 = getTickCount();
	if (t0 - tLast < 150){
		//printf("getNewTrackVariables: too soon %i\n",(int)(t0 - tLast));
		timerSet(vp, TIMER_GETTRACKVARDELAYED, 10*1000);
		return;
	}else{
		tLast = t0;
	}

	if (loadLock(vp)){
		if (getPlayState(vp)){
			if (getPlaybackMode(vp) != PLAYBACKMODE_AUDIO){
				int w = 0, h = 0;
				vlc_getVideoSize(vlc, &w, &h);
				if (w && h){
					vlc->videoWidth = w;
					vlc->videoHeight = h;
					//printf("getNewTrackVariables: size %ix%i\n", w, h);
				}
			}

			vlc_mediaParseAsync(vlc);
			cfgAttachmentsSetCount(vp, vlc_attachmentsGetCount(vlc));
			//getAudioOutputDetails(vlc);
		}
		loadUnlock(vp);
	}
	//timerSet(vp, TIMER_CHAPTER_UPDATE, 1000);
}

void startNextTrackPlayback (TVLCPLAYER *vp)
{

	if (getApplState(vp)){
		if (getPlayState(vp))
			trackStop(vp);

		//if (getPlaybackMode(vp) == PLAYBACKMODE_AUDIO){	// don't auto play next video track
			PLAYLISTCACHE *plcQ = getQueuedPlaylist(vp);
			if (!plcQ) return;

			if (!vp->vlc->playRandom && plcQ->pr->playingItem < playlistGetTotal(plcQ)-1){
				int trk = player_nextTrack(vp, vp->vlc);
				trackLoadEvent(vp, plcQ, trk);
			}else if (vp->vlc->playRandom){
				int trk = player_randomTrack(vp, vp->vlc);
				trackLoadEvent(vp, plcQ, trk);
			}
		//}
	}
}

void vlc_eventsCallback (const libvlc_event_t *event, void *udata)
{
	// a few sanity checks
	if (event == NULL || udata == NULL) return;
	TVLCPLAYER *vp = (TVLCPLAYER*)udata;
	if (!getApplState(vp)) return;
	TVLCCONFIG *vlc = getConfig(vp);
	if (!vlc) return;


#if 0	/* print useful debug info but don't spam */
	if (event->type != libvlc_MediaPlayerPositionChanged && event->type != libvlc_MediaPlayerTimeChanged)
    	printf("vlc_eventsCallback %i: '%s' (state:%i)\n", event->type, vlc_EventTypeToName(event->type), vlc->playState);
#endif


	if (event->p_obj != vlc->m && event->p_obj != vlc->mp)
		return;

	setAwake(vp);

    switch (event->type){
	  case libvlc_MediaMetaChanged:{
	  	const int tagid = event->u.media_meta_changed.meta_type;
	  	char *meta = vlc_getMeta(vlc, tagid);
	  	//printf("libvlc_MediaMetaChanged: meta %p\n", meta);
	  	if (!meta) break;

		/*if (tagid == libvlc_meta_ArtworkURL){
			if (doesArtworkExistUtf8(meta)){
				char *path = getPlayingPath(vp);
				if (path){
					forceGetArtwork(vp, path, meta);
					my_free(path);
				}
			}
		}else*/
		if (tagid == libvlc_meta_Publisher){
			char *path = getPlayingPath(vp);
			if (path){
				tagAdd(vp->tagc, path, tagid, meta, 1);
				my_free(path);
			}


		// used with stremaing media such as DVB-T
		// this grabs the stream publisher and current programme title then displays
		// together when both are available, but once only per programme.
		// as this information is live, it should not be stored beyond current programme run time of programme
		}else if (tagid == libvlc_meta_NowPlaying){
			//printf("MTAG_NowPlaying '%s'\n", meta);

			char *path = getPlayingPath(vp);
			if (path){
				char tag[MAX_PATH_UTF8+1];
				int different = 1;

				tagRetrieve(vp->tagc, path, MTAG_NowPlaying, tag, MAX_PATH_UTF8);
				if (*tag)
					different = strcmp(meta, tag) != 0;

				if (different){
					/*int displayed = epg_displayOSD(vlc->mp, 6000);
					if (displayed == -1)
						timerSet(vp, TIMER_EPG_DISPLAYOSD, 1000);*/
					int displayed = 0;
					if (displayed != 1){
						TVIDEOOVERLAY *pctrl = pageGetPtr(vp, PAGE_OVERLAY);
						tagRetrieve(vp->tagc, path, MTAG_Publisher, tag, MAX_PATH_UTF8);
						if (*tag){
							char title[MAX_PATH_UTF8+1];
							__mingw_snprintf(title, MAX_PATH_UTF8, "%s - %s", tag, meta);
							marqueeAdd(vp, pctrl->marquee, title, getTime(vp)+7000);
						}else{
							marqueeAdd(vp, pctrl->marquee, meta, getTime(vp)+7000);
						}
					}
					tagAdd(vp->tagc, path, tagid, meta, 1);
				}
				my_free(path);
				//-timerSet(vp, TIMER_NEWTRACKVARS3, 1000);
			}
		}
		//printf("libvlc_MediaMetaChanged: meta free %p\n", meta);
		my_free(meta);
		break;
	  }

	  case libvlc_MediaPlayerPositionChanged:{
		vlc->position = event->u.media_player_position_changed.new_position;
		clipFloat(vlc->position);
		timerSet(vp, TIMER_CTRL_UPDATETIMESTAMP, 1);

      	break;
	  }
      case libvlc_MediaDurationChanged:{
		vlc->length = event->u.media_duration_changed.new_duration / 1000;
		timerSet(vp, TIMER_CTRL_UPDATETIMESTAMP, 1);

		char *path = getPlayingPath(vp);
		if (path){
			//char length[32];
			//tagRetrieve(vp->tagc, path, MTAG_LENGTH, length, sizeof(length));
			if (/**length &&*/ vlc->length){
				char buffer[64];
				timeToString(vlc->length, buffer, sizeof(buffer)-1);
				if (*buffer)
					tagAdd(vp->tagc, path, MTAG_LENGTH, buffer, 1);
			}
			my_free(path);
		}

		if (getApplState(vp))
			timerSet(vp, TIMER_NEWTRACKVARS1, 100);

      	break;
	  }
      case libvlc_MediaStateChanged:
		vlc->vlcPlayState = event->u.media_state_changed.new_state;
      	//printf("libvlc_MediaStateChanged %i %i\n", state, libvlc_Stopped);
		//vlc->vlcPlayState = vlc_getState(vlc);
    
      	if (MediaPlayerStopped_complete == -1 || vlc->vlcPlayState == libvlc_Ended){
      		timerReset(vp, TIMER_NEWTRACKVARS1);
      		timerReset(vp, TIMER_NEWTRACKVARS2);
      		timerReset(vp, TIMER_NEWTRACKVARS3);
      	    vlc->playEnded = 1;
      		vlc->playState = 8;
			vlc->position = 0.0;
		}
		
		sbuiDKStateChange();
		timerSet(vp, TIMER_CTRL_UPDATETIMESTAMP, 50);
				
		if (vp->gui.drawVisuals){
			if (vlc->vlcPlayState == libvlc_Playing || vlc->vlcPlayState == libvlc_Paused){
				setBaseUpdateRate(UPDATERATE_BASE_VISUALS);
				setTargetRate(vp, UPDATERATE_BASE_VISUALS);
			}else if (vlc->vlcPlayState == libvlc_Stopped || vlc->vlcPlayState == libvlc_Ended){
				setBaseUpdateRate(UPDATERATE_BASE_DEFAULT);
				setTargetRate(vp, UPDATERATE_BASE_DEFAULT);
			}
		}
		timerSet(vp, TIMER_TASKBARTITLE_UPDATE, 100);

		if (vlc->vlcPlayState == libvlc_Stopped || vlc->vlcPlayState == libvlc_Playing)
			taskbarPostMessage(vp, WM_TRACKPLAYNOTIFY, -1, 0);
      	break;

	  //case libvlc_MediaPlayerOpening:{
	  	//char *str = libvlc_media_get_mrl(libvlc_media_player_get_media(event->p_obj));
	  	//printf(":: '%s'\n", str);
	  	//break;
	  //}
      case libvlc_MediaPlayerPlaying:{
     	 //printf("libvlc_MediaPlayerPlaying\n");

		TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
		ctrlPanCalcPositions(&plyctrl->ctrlpan, BTNPANEL_SET_PLAY);
		if (hasPageBeenAccessed(vp, PAGE_PLY_SHELF)){
			TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
			albPanCalcPositions(&spl->albpan, BTNPANEL_SET_PLAY);
		}
      	vlc->playState = 1;

      	// on vlc < v2.0 don't call a libvlc func from within its callback to avoid a deadlock
		//if (LIBVLC_VERSION_MAJOR > 1/* && LIBVLC_VERSION_MINOR >= 2*/)
		//	setVolume(vp, vp->vlc->volume, VOLUME_APP);
      	// use this to retrieve video size as calling via VLC's callback seems to generate a deadlock
      	if (getApplState(vp))
			timerSet(vp, TIMER_NEWTRACKVARS1, 150);

      	break;
	  }
      case libvlc_MediaPlayerPaused:{
      	TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
      	ctrlPanCalcPositions(&plyctrl->ctrlpan, BTNPANEL_SET_PAUSE);
		if (hasPageBeenAccessed(vp, PAGE_PLY_SHELF)){
			TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
			albPanCalcPositions(&spl->albpan, BTNPANEL_SET_PAUSE);
		}
      	vlc->playState = 2;
		break;
	  }
      case libvlc_MediaPlayerStopped:{
      	timerReset(vp, TIMER_GETTRACKVARDELAYED);
		TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
		ctrlPanCalcPositions(&plyctrl->ctrlpan, BTNPANEL_SET_STOP);
		if (hasPageBeenAccessed(vp, PAGE_PLY_SHELF)){
			TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
			albPanCalcPositions(&spl->albpan, BTNPANEL_SET_STOP);
		}
      	TCHAPTER *chapt = pageGetPtr(vp, PAGE_CHAPTERS);
      	chapt->ttitles = 0;

      	if (pageGet(vp) == PAGE_CHAPTERS)
      		page2RenderDisable(vp->pages, PAGE_CHAPTERS);
      	cfgAttachmentsSetCount(vp, 0);
      	
      	MediaPlayerStopped_complete = 1;
      	break;
	  }
	  case libvlc_MediaPlayerEncounteredError:{
	  	char *path = getPlayingPath(vp);
		if (path && *path){
	  		//dbprintf(vp, "error playing %i:'%s'", getPlayingItem(vp)+1, path);
	  		TVIDEOOVERLAY *pctrl = pageGetPtr(vp, PAGE_OVERLAY);
	  		char buffer[MAX_PATH_UTF8+1];
	  		__mingw_snprintf(buffer, MAX_PATH_UTF8, "can not play: '%s'", path);
			marqueeAdd(vp, pctrl->marquee, buffer, getTime(vp)+6000);
	  		my_free(path);
	  	}

		TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
		ctrlPanCalcPositions(&plyctrl->ctrlpan, BTNPANEL_SET_STOP);
		if (hasPageBeenAccessed(vp, PAGE_PLY_SHELF)){
			TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
			albPanCalcPositions(&spl->albpan, BTNPANEL_SET_STOP);
		}
		vlc->playState = 8;

		// faulted on this track. try playing next
	  }
	  	ALLOW_FALLTHROUGH;
      case libvlc_MediaPlayerEndReached:	// end reached so move on to next track, don't hang around
		cfgAttachmentsSetCount(vp, 0);
      	timerReset(vp, TIMER_NEWTRACKVARS1);
		timerReset(vp, TIMER_NEWTRACKVARS2);
		timerReset(vp, TIMER_NEWTRACKVARS3);
		timerReset(vp, TIMER_GETTRACKVARDELAYED);

       	if (vlc->playState == 8)
      		timerSet(vp, TIMER_GOTONEXTTRACK, 0);

      	vlc->playEnded = 1;
      	vlc->playState = 8;
		vlc->position = 0.0;
		timerSet(vp, TIMER_CTRL_UPDATETIMESTAMP, 1);
		break;

	  case libvlc_MediaPlayerTitleChanged:
		if (getApplState(vp))
			timerSet(vp, TIMER_NEWTRACKVARS2, 750);
		break;

#if (LIBVLC_VERSION_MAJOR >= 2 /*&& LIBVLC_VERSION_MINOR >= 1*/)
	  case libvlc_MediaPlayerBuffering:
	  	//printf("buffering %%%.1f\n", event->u.media_player_buffering.new_cache);
	  	vp->vlc->bufferPos = event->u.media_player_buffering.new_cache/100.0;
	  	timerSet(vp, TIMER_NEWTRACKVARS3, 500);
	  	renderSignalUpdate(vp);
	  	break;
#endif

	  case libvlc_MediaSubItemAdded:{
	 	//printf("@@@ sub item added %p %p\n", event->u.media_subitem_added.new_child, vp->vlc->m);

	 	char *path = libvlc_media_get_mrl(event->u.media_subitem_added.new_child);
	 	if (path){
	 		//printf("'%s'\n", path);

	 		//PLAYLISTCACHE *plc = playlistManagerCreatePlaylist(vp->plm, path, 1);
	 		//if (plc){
	 		//	playlistAdd(getQueuedPlaylist(vp), path);
	 		//	if (!playlistIsChild(plc, getQueuedPlaylist(vp)))
	 		//		playlistAddPlc(getQueuedPlaylist(vp), plc);
	 		//}
#if 0
			for (int i = 0; i < 17; i++){
				char *title = libvlc_media_get_meta(event->u.media_subitem_added.new_child, i);
				if (title){
					printf("%i:'%s'\n", i, title);
					free(title);
				}
			}
#endif
#if 1
	 		PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
			if (plc){
	 			playlistSetPath(plc, plc->pr->playingItem, path);
				timerSet(vp, TIMER_STOP, 10);
		 		timerSet(vp, TIMER_PLAY, 110);
		 		//m_tmp = event->u.media_subitem_added.new_child;
	 		}
#endif
	 		free(path);
	 	}
	 	libvlc_media_release(event->u.media_subitem_added.new_child);
	 	break;
	  }
	  case libvlc_MediaParsedChanged:{
	  	if (getApplState(vp))
	  		timerSet(vp, TIMER_NEWTRACKVARS2, 50);

		char *title = getPlayingTitle(vp);
		if (!title)
			title = getPlayingPath(vp);

		if (title){
			TVIDEOOVERLAY *pctrl = pageGetPtr(vp, PAGE_OVERLAY);
#if 0
			char *length = getPlayingLengthStr(vp);
			if (length){
				char buffer[MAX_PATH_UTF8+1];
	  			__mingw_snprintf(buffer, MAX_PATH_UTF8, "%s - %s", title, length);
	  			marqueeAdd(vp, pctrl->marquee, buffer, getTime(vp)+5000);
	  			my_free(length);
	  		}else{
	  			marqueeAdd(vp, pctrl->marquee, title, getTime(vp)+5000);
	  		}
#else
			overlayAddTitle(pctrl, title);
			//printf("overlayAddTitle '%s'\n", title);
#endif
			my_free(title);
		}
	    break;
	  }
	  //default:
		//return;
	}
}

void vlc_eventsCallbackLocked (const libvlc_event_t *event, void *udata)
{
	if (SHUTDOWN) return;
	TVLCPLAYER *vp = (TVLCPLAYER*)udata;


	if (lockWait(vp->ctx.hVideoCBLock, INFINITE)){
		vlc_eventsCallback(event, udata);
		lockRelease(vp->ctx.hVideoCBLock);
	}
}

#if 0
void aplayCb (void *data, const void *samples, unsigned count, int64_t pts)
{
	int *buffer = (int*)samples;
	printf("audio_play_cb: %i %i\n", count, buffer[1]);
}

void apauseCb (void *data, int64_t pts)
{
	printf("audio_pause_cb\n");
}

void aresumeCb (void *data, int64_t pts)
{
	printf("audio_resume_cb\n");
}

void adrainCb (void *data)
{
	printf("audio_drain_cb\n");
}

void aflushCb (void *data, int64_t pts)
{
	printf("audio_flush_cb\n");
}

void setupaudio (TVLCPLAYER *vp, TVLCCONFIG *vlc)
{


	//libvlc_audio_set_callbacks(vlc->mp, aplayCb, apauseCb, aresumeCb, aflushCb, adrainCb, vp);

	var_SetAddress((vlc_object_t*)vlc->mp, "amem-play", aplayCb);
    var_SetAddress((vlc_object_t*)vlc->mp, "amem-pause", apauseCb);
    var_SetAddress((vlc_object_t*)vlc->mp, "amem-resume", aresumeCb);
    var_SetAddress((vlc_object_t*)vlc->mp, "amem-flush", aflushCb);
    var_SetAddress((vlc_object_t*)vlc->mp, "amem-drain", adrainCb);
    var_SetAddress((vlc_object_t*)vlc->mp, "amem-data", vp);


	//var_SetString((vlc_object_t*)vlc->mp, "sout", "waveout,amem");
	//vlc_addOption(vlc, "sout=waveout,amem");
	
	var_SetString((vlc_object_t*)vlc->mp, "aout", "amem");
	vlc_addOption(vlc, "sout-keep");
	vlc_addOption(vlc, "sout-display-audio");

	libvlc_audio_set_format(vlc->mp, "FL32", 48000, 2);

	//aout_EnableFilter((vlc_object_t*)vlc->mp, "directx", 1);

	//var_SetString((vlc_object_t*)vlc->mp, "sout-keep", "");
	//var_SetString((vlc_object_t*)vlc->mp, "sout", "directx");

	//vlc_addOption(vlc, "sout-keep");
	//vlc_addOption(vlc, "sout=win32");

	char *aout = var_CreateGetString((vlc_object_t*)vlc->mp, "aout");
	if (aout)
		printf("@@ Setup Audio aout #%s#\n", aout);

	//var_SetString((vlc_object_t*)vlc->mp, "audio-filter", "amem");
	//vlc_addOption(vlc, "audio-filter=amem");
	
	
	//vlc_addOption(vlc, "sout=waveout,amem");
}
#endif


int loadMediaPlayer (TVLCPLAYER *vp, TVLCCONFIG *vlc, char *inlineOpts, char *opts)
{
	vlc_configure(vp, vlc, vlc->width, vlc->height, vlc->bpp, 0/*vp->gui.visual*/);

	if (inlineOpts && *inlineOpts)
		vlc_configureMediaOptions(vlc, inlineOpts);

	if (opts && *opts)
		vlc_configureMediaOptions(vlc, opts);

	vlc->mp = vlc_newFromMedia(vlc);
	if (vlc->mp){
		//setupaudio(vp, vlc);
		//getAudioOutputDetails(vlc);

		vlc_setVideoFormat(vlc, VCHROMA, vlc->width, vlc->height, VPITCH(vlc->width));
		vlc_setVideoCallbacks(vlc, vmem_lock, vmem_unlock, NULL, vp);
		vlc_attachEvents(vlc, vlc_eventsCallbackLocked, vp);

		return 1;
	}
	return 0;
}

void unloadMedia (TVLCPLAYER *vp, TVLCCONFIG *vlc)
{
	vlc_inputEventCbDel(vp->vlc, vp);
	vlc_detachEvents(vlc, vlc_eventsCallbackLocked, vp);
	vlc_mp_release(vlc);

	vlc->isMediaLoaded = 0;
	vlc->hasAttachments = 0;
}

int loadMedia (TVLCPLAYER *vp, TVLCCONFIG *vlc, char *mediaPath, char *opts)
{
	//printf("@@@ loadMedia open '%s' \n", mediaPath);
	
	int ret = 0;
	char *inlineOpts = vlc_configureGetOptions(mediaPath);

	if (!isMediaMRL(mediaPath))
		vlc->m = vlc_new_path(vlc, mediaPath);
	else
		vlc->m = vlc_new_mrl(vlc, mediaPath);

	if (vlc->m){
		ret = loadMediaPlayer(vp, vlc, inlineOpts, opts);
		if (!ret)
			vlc_mediaRelease(vlc);
	}
		
	if (inlineOpts)
		my_free(inlineOpts);

	if (!ret)
		dbprintf(vp, "can not open '%s'",mediaPath);
	return ret;
}

static inline void closeVLCInstance (TVLCPLAYER *vp, TVLCCONFIG *vlc)
{
	if (vlc->openCount){
		vlc->openCount = 0;
		unloadMedia(vp, vlc);
    	vlc_releaseLib(vlc);

    	if (vp->vlc->hLibTmp){
    		vlcEventFreeHandles(vp, 1);
    		libvlc_release(vp->vlc->hLibTmp);
    		vlcEventsLock(vlc);
    		lockClose(vlc->hLockLengths);
    		vlc->hLockLengths = NULL;
    	}
    }
}

static inline void closeVLC (TVLCPLAYER *vp)
{
	closeVLCInstance(vp, selectVLCConfig(vp));
}

char *vlc_configureGetOptions (char *path)
{
	const int len = strlen(path);
	char *popts = NULL;

	for (int i = 0; i < len; i++){
		if (path[i] == '<' && path[i+1] && path[i+1] != '>'){
			if (!popts)
				popts = my_strdup(&path[i]);

			for (int j = 0; path[i] && path[i] != '>'; j++, i++)
				path[i] = ' ';
			if (path[i] == '>') path[i] = ' ';
		}
	}
	removeTrailingSpaces(path);
	return popts;
}

int vlc_configureMediaOptions (TVLCCONFIG *vlc, char *options)
{
	const int len = strlen(options);
	if (!len) return 0;

	int ct = 0;
	char *path = options;
	char option[MAX_PATH_UTF8+1];
	int closed = 0;

	for (int i = 0; i < len; i++){
		if (path[i] == '<' && path[i+1] && path[i+1] != '>' && path[i+1] != OPTSEPARATOR){
			i++; // skip over <
			closed = 0;

			for (int j = 0; path[i] && !closed; j++, i++){
				option[j] = path[i];

				if (option[j] == OPTSEPARATOR){
					option[j] = 0;
					if (j)
						vlc_addOption(vlc, option);

					j = -1;
					ct++;
				}else if (option[j] == '>'){
					option[j] = 0;
					if (j)
						vlc_addOption(vlc, option);

					closed = 1;
					i--;
					ct++;
				}
			}
		}
	}

	return ct;
}

void vlc_configure (TVLCPLAYER *vp, TVLCCONFIG *vlc, const int width, const int height, const int bpp, const int visMode)
{

	//vlc_addOption(vlc, "aout=any");

	//vlc_addOption(vlc, "vout=opengl");
//	vlc_addOption(vlc, "quiet");
//	vlc_addOption(vlc, "quiet-synchro");
	vlc_addOption(vlc, "http-album-art");
	vlc_addOption(vlc, "album-art=1");
	//vlc_addOption(vlc, "album-art-filename=cover.jpg");

	vlc_addOption(vlc, "spu");
	vlc_addOption(vlc, "sub-fps=25");
	//vlc_addOption(vlc, "sub-type=auto");
	//vlc_addOption(vlc, "sub-language=english");
	//vlc_addOption(vlc, "sub-autodetect-file");
	//vlc_addOption(vlc, "sub-autodetect-fuzzy=1");
	//vlc_addOption(vlc, "sub-autodetect-path=.\\subs");
	//vlc_addOption(vlc, "no-mkv-preload-local-dir");

	//vlc_addOption(vlc, "no-audio");
	//vlc_addOption(vlc, "no-video");

	//snprintf(buffer, 128, "swscale-mode=%i", swmode);
	//printf("%s\n", buffer);
	//vlc_addOption(vlc, buffer);

	vlc_addOption(vlc, "screen-fps=10.0");
	//vlc_addOption(vlc, "no-overlay");
	vlc_addOption(vlc, "no-video-title-show");
	//vlc_addOption(vlc, "no-video-on-top");
	//vlc_addOption(vlc, "video-title-timeout=1");
	vlc_addOption(vlc, "ignore-config");
	vlc_addOption(vlc, "plugin-path=plugins");
	//vlc_addOption(vlc, "no-stats");
	vlc_addOption(vlc, "no-media-library");

	//vlc_addOption(vlc, "screen-top=10");
	//vlc_addOption(vlc, "screen-left=10");
	//vlc_addOption(vlc, "screen-width=480");
	//vlc_addOption(vlc, "screen-height=272");
	//vlc_addOption(vlc, "screen-follow-mouse");
	//vlc_addOption(vlc, "screen-fragment-size=16");

	//vlc_addOption(vlc, "aspect-ratio=1.77");
	//vlc_addOption(vlc, "custom-aspect-ratio=1.77");
	//vlc_addOption(vlc, "aspect-ratio=1.66");
	//vlc_addOption(vlc, "custom-aspect-ratio=1.66");

	/*vlc_addOption(vlc, "audio-visual=projectm");
	vlc_addOption(vlc, "effect-list=projectm");
	vlc_addOption(vlc, "audio-filter=projectm");
	vlc_addOption(vlc, "vout=opengl");
	vlc_addOption(vlc, "projectm-preset-path=C:/Program Files (x86)/VLC/projectm");
	vlc_addOption(vlc, "projectm-title-font=C:/WINDOWS/Fonts/arial.ttf");
	vlc_addOption(vlc, "projectm-menu-font=C:/WINDOWS/Fonts/arial.ttf");
	vlc_addOption(vlc, "projectm-width=480");
	vlc_addOption(vlc, "projectm-height=272");
	vlc_addOption(vlc, "effect-width=480");
	vlc_addOption(vlc, "effect-height=272");*/


#if (0 || LIBVLC_VERSION_MAJOR < 2)
	char buffer[640];

	if (getPlaybackMode(vp) == PLAYBACKMODE_VIDEO){
		vlc_addOption(vlc, "audio-filter=");
	}else{
/*
<projectm-title-font=C:\WINDOWS\Fonts\arial.ttf,<projectm-menu-font=C:\WINDOWS\Fonts\arial.ttf,projectm-preset-path=C:\Program Files (x86)\VLC\projectm,vout=opengl,projectm-width=480,projectm-height=272,audio-filter=projectm>
*/

		__mingw_snprintf(buffer, sizeof(buffer)-1, "effect-width=%i", vlc->width);
		vlc_addOption(vlc, buffer);
		__mingw_snprintf(buffer, sizeof(buffer)-1, "effect-height=%i", vlc->height);
		vlc_addOption(vlc, buffer);
		vlc_addOption(vlc, "audio-visual=visual");

		switch (visMode){
	  	  case VIS_VUMETER:
	  	  	printf("vu meter\n");
	  		vlc_addOption(vlc, "audio-filter=visual");
	  		vlc_addOption(vlc, "effect-list=vumeter");
			break;

		  case VIS_SMETER:
		  	printf("spec meter\n");
			vlc_addOption(vlc, "audio-filter=visual");
			vlc_addOption(vlc, "effect-list=spectrometer");
			vlc_addOption(vlc, "spect-radius=62");
			vlc_addOption(vlc, "spect-sections=3");
			vlc_addOption(vlc, "spect-separ=4");
			vlc_addOption(vlc, "spect-amp=9");
			vlc_addOption(vlc, "spect-peak-width=61");
			vlc_addOption(vlc, "spect-peak-height=3");
			break;

	  	  case VIS_PINEAPPLE:
	  	  	printf("pineapple\n");
			vlc_addOption(vlc, "audio-filter=visual");
			vlc_addOption(vlc, "effect-list=spectrometer");
			vlc_addOption(vlc, "spect-radius=100");
			vlc_addOption(vlc, "spect-sections=5");
			vlc_addOption(vlc, "spect-separ=0");
			vlc_addOption(vlc, "spect-amp=2");
			vlc_addOption(vlc, "spect-peak-width=12");
			vlc_addOption(vlc, "spect-peak-height=50");
			break;

	  	  case VIS_SPECTRUM:
	  	  	printf("spectrum\n");
		  	vlc_addOption(vlc, "audio-filter=visual");
	  		vlc_addOption(vlc, "effect-list=spectrum");
			break;
			
	  	  case VIS_SCOPE:
	  	  	printf("scope\n");
	  		vlc_addOption(vlc, "audio-filter=visual");
	  		vlc_addOption(vlc, "effect-list=scope");
			break;
	  	  case VIS_GOOM_Q3:
	  	  case VIS_GOOM_Q2:
	  	  case VIS_GOOM_Q1:{
			const TGOOMRESSTR goomres[] = {GOOMREZMODES};

	  	  	vlc_addOption(vlc, "audio-filter=goom");
			__mingw_snprintf(buffer, sizeof(buffer)-1, "goom-width=%s", goomres[visMode-VIS_GOOM_Q3].x);
			vlc_addOption(vlc, buffer);
			__mingw_snprintf(buffer, sizeof(buffer)-1, "goom-height=%s", goomres[visMode-VIS_GOOM_Q3].y);
			vlc_addOption(vlc, buffer);
			break;
		}
	  	default:
		  	break;
		}
	}
#else
	char buffer[32];
#endif

	__mingw_snprintf(buffer, sizeof(buffer), "audio-desync=%i", vlc->audioDesync);
	vlc_addOption(vlc, buffer);

	if (vlc->subtitleDelay){
		__mingw_snprintf(buffer, sizeof(buffer), "sub-delay=%i", vlc->subtitleDelay/100);
		vlc_addOption(vlc, buffer);
	}




#if 0
	vlc_addOption(vlc, "dshow-chroma="DSCHROMA);

#else

//	setTunerChannel(vp, vp->tuner.channelIdx);
//	sprintf(vp->tuner.cchannel, "dshow-tuner-channel=%d", vp->tuner.channel);
//	vlc_addOption(vlc, vp->tuner.cchannel);

//	setTunerCountry(vp, CHN_COUNTRY);
//	sprintf(vp->tuner.ccountry, "dshow-tuner-country=%d", vp->tuner.country);
//	vlc_addOption(vlc, vp->tuner.ccountry);

/*
	vlc_addOption(vlc, "dshow-caching="DSCACHING);
	vlc_addOption(vlc, "dshow-tuner-input="DSINPUT_TYPE);
	//vlc_addOption(vlc, "dshow-chroma="DSCHROMA);
	vlc_addOption(vlc, "dshow-audio-samplerate="DSAUDIO_SAMRATE);
	vlc_addOption(vlc, "dshow-audio-bitspersample="DSAUDIO_BPS);
	vlc_addOption(vlc, "dshow-audio-channels="DSAUDIO_CHNS);
*/

#ifdef DSVIDEO_SIZE
	vlc_addOption(vlc, "dshow-size="DSVIDEO_SIZE);
#endif
#ifdef DSDEVICE_VIDEO
	vlc_addOption(vlc, "dshow-vdev="DSDEVICE_VIDEO);
#endif
#ifdef DSDEVICE_AUDIO
	vlc_addOption(vlc, "dshow-adev="DSDEVICE_AUDIO);
#endif
#endif

}

static inline int createVideoBuffers (TVLCPLAYER *vp)
{
	// we read from this, don't write. read only upon vlc signals so
 	vp->ctx.vmem = lNewFrame(vp->ml->hw, vp->vlc->width, vp->vlc->height, vp->vlc->bpp);

 	// copy of the above with write permitted
 	vp->ctx.working = lNewFrame(vp->ml->hw, vp->vlc->width, vp->vlc->height, vp->vlc->bpp);
	vp->ctx.workingTransform = lNewFrame(vp->ml->hw, vp->vlc->width, vp->vlc->height, vp->vlc->bpp);
   	vp->ctx.bufferSize = vp->ctx.working->frameSize;
   	vp->ctx.pixels = lGetPixelAddress(vp->ctx.vmem, 0, 0);
 	vp->ctx.pixelBuffer = lGetPixelAddress(vp->ctx.working, 0, 0);

   	return (vp->ctx.pixels && vp->ctx.pixelBuffer);
}

static inline void freeVideoBuffers (TVLCPLAYER *vp)
{
	lDeleteFrame(vp->ctx.vmem);
	lDeleteFrame(vp->ctx.working);
	lDeleteFrame(vp->ctx.workingTransform);
	vp->ctx.pixels = NULL;
	vp->ctx.pixelBuffer = NULL;
	vp->ctx.bufferSize = 0;
}

static inline int createVLCInstance (TVLCCONFIG *vlc, const int width, const int height, const int visMode)
{
 	vlc->hLib = NULL;
 	vlc->m = NULL;
 	vlc->mp = NULL;
 	vlc->emp = NULL;
 	vlc->em = NULL;
	vlc->hLibTmp = NULL;

	char adesync[40];
	__mingw_snprintf(adesync, sizeof(adesync), "--audio-desync=%i", vlc->audioDesync);

	char *opts[] = {
		//"--audio-filter=projectm",
		//"--projectm-preset-path=C:/Program Files (x86)/VLC/projectm",
		//"--audio-filter=goom",
		//scope, spectrum, spectrometer and vuMeter.
		
		//"--audio-filter=visual",
		//"--effect-list=spectrometer",
		//"--spect-show-original",
		
		//"--effect-list=scope",
		//"--effect-list=spectrum",
		//"--no-visual-80-bands",
		//"--effect-list=vuMeter",
		"--effect-width=800",
		"--effect-height=480",
		"--goom-width=800",
		"--goom-height=480",
		//"--aout=directx",
		/*"--equalizer-preset=flat",
		"--equalizer-preamp=12.00",
		"--equalizer-bands=0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0",
		"--audio-filter=equalizer",*/
		"--no-video-title-show",
		//"--video-on-top",
		"--verbose=0",
		adesync,
		"--quiet"
	};

	vlc->hLib = vlc_init(vlc, sizeof(opts)/sizeof(*opts), (char**)&opts);
	if (vlc->hLib){
		vlc->openCount++;
		libvlc_set_user_agent(vlc->hLib, "VLC media player", PLAYER_VERSION);
	}

	const char *const optsTmp[] = {
		"--no-audio",
		"--no-video",
		"--ignore-config",
		"--album-art=2",
		"--verbose=0",
		"--quiet"
	};

	vlc->hLibTmp = libvlc_new(sizeof(optsTmp)/sizeof(*optsTmp), (const char**)&optsTmp);
	vlc->hLockLengths = lockCreate("MediaLengths");

	vlc->visMode = visMode;
	vlc->swapColourBits = 0;
	vlc->volume = 50;
	vlc->position = 0.0;
	vlc->playState = 0;
	vlc->width = width;
 	vlc->height = height;
 	vlc->bpp = DVIDBUFBPP;
 	vlc->spu.total = 0;
 	vlc->spu.desc = NULL;
 	vlc->spu.selected = -1;

	return (vlc->hLib != NULL);
}

/*
static char *manglePath (TVLCPLAYER *vp, const char *src)
{
	// if we don't do this subtitles won't be detected
	if (isLocalMedia(src) > 0 && getPlaybackMode(vp) == PLAYBACKMODE_VIDEO){
		size_t len = strlen(src);
		char *tmp = my_calloc(1, len + 16);		// enough for source path + file:///
		char *path = my_calloc(8, len + 16);	// enough space for a uri encoded utf8 path
		if (!path || !tmp) return 0;
		sprintf(tmp, "file:///%s", src);

#if 1	// we do this because vlc/src/text/strings.c:decode_URI() fucks up utf8 encoded paths
		encodeURI(tmp, path, strlen(tmp));
#else
		strcpy(path, tmp);
#endif
		my_free(tmp);
		return path;
	}else{
		// dshow:// and screen:// pass straight through
		return my_strdup(src);
	}
}
*/

static inline int _browserLoadMediaFile (TVLCPLAYER *vp, char *path, char *opts)
{

	TVLCCONFIG *vlc = getConfig(vp);

	if (getPlayState(vp))
		player_stop(vp, vlc);

	if (vlc->isMediaLoaded)
		unloadMedia(vp, vlc);

	//char *path = manglePath(vp, utf8path);
	//if (path == NULL) return 0;

	vlc->isMediaLoaded = loadMedia(vp, vlc, path, opts);
	if (vlc->isMediaLoaded){
		trackPlay(vp);
		vlc_mediaParseAsync(vlc);

		vlc->isMediaLoaded = vlc_willPlay(vlc);
		if (vlc->isMediaLoaded){
			timerSet(vp, TIMER_NEWTRACKVARS1, 200);
			//if (LIBVLC_VERSION_MAJOR == 1 && LIBVLC_VERSION_MINOR <= 1)
				setVolume(vp, vlc->volume, VOLUME_APP);
//#if (LIBVLC_VERSION_MAJOR >= 2 && LIBVLC_VERSION_MINOR >= 1)
			eqApply(pageGetPtr(vp, PAGE_EQ), vlc, 0);
//#endif
			vlc_inputEventCbSet(vlc, vp);
		}else{
			player_stop(vp, vlc);
		}
	}

	//my_free(path);
	return vlc->isMediaLoaded;
}

static inline int browserLoadMediaFile (TVLCPLAYER *vp, char *utf8path, char *opts)
{
	if (loadLock(vp)){
		//printf("browserLoadMediaFile in\n");
		int ret = _browserLoadMediaFile(vp, utf8path, opts);
		//printf("browserLoadMediaFile out\n");
		loadUnlock(vp);
		return ret;
	}
	return 0;
}

static inline void exitAppl (TVLCPLAYER *vp)
{
	//printf("exitAppl %i\n", vp->applState);
	SHUTDOWN = 1;
	sbuiSetApplState(1);

	if (hasPageBeenAccessed(vp, PAGE_SEARCH))
		searchForceStop(vp);

  	if (getPlayState(vp))
		trackStop(vp);
	vp->vlc->playState = 0;
	unloadMedia(vp, vp->vlc);
	setApplState(vp, 0);
}

int getVolume (TVLCPLAYER *vp, const int whichVolume)
{
	if (whichVolume == VOLUME_MASTER)
#if (0 && ENABLE_BASS)
		return bass_volumeGet(&vp->bass);
#else
		return master_volumeGet();
#endif
	else
		return vlc_getVolume(getConfig(vp));
}

int getMute (TVLCPLAYER *vp)
{
	TVLCCONFIG *vlc = getConfig(vp);
	if (vlc)
		return vlc_getMute(vlc);
	else
		return 0;
}

void toggleMute (TVLCPLAYER *vp)
{
	TVLCCONFIG *vlc = getConfig(vp);
	vlc_setMute(vlc, vlc_getMute(vlc)^1);
}

int setVolume (TVLCPLAYER *vp, int volume, const int whichVolume)
{
	
	TVLCCONFIG *vlc = getConfig(vp);
	if (volume > 100) volume = 100;
	else if (volume < 0) volume = 0;	
	
	if (whichVolume == VOLUME_MASTER){
#if (0 && ENABLE_BASS)
		if (!volume){
			bass_muteSet(&vp->bass, 1);
		}else{
			bass_volumeSet(&vp->bass, volume);
			bass_muteSet(&vp->bass, 0);
		}
		return bass_volumeGet(&vp->bass);
#else
		if (!volume){
			master_muteSet(1);
		}else{
			master_volumeSet(volume);
			master_muteSet(0);
		}
		return master_volumeGet();
#endif

	}else{
		if (getPlayState(vp) > 0){
#if (LIBVLC_VERSION_MAJOR == 2 && LIBVLC_VERSION_MINOR == 0)
			vlc_setVolume(vlc, volume);
			vlc->volume = vlc_getVolume(vlc);

			vlc_setMute(vlc, vlc->volume < 1);
			if (vlc_getMute(vlc))
				vlc->volume = -1;
#else
			vlc_setVolume(vlc, volume);
			vlc->volume = volume;//vlc_getVolume(vlc);

			/*vlc_setMute(vlc, (vlc->volume <= 0));
			if (vlc_getMute(vlc))
				vlc->volume = -1;*/
#endif
		}
	}
	return vlc->volume;

}

static inline PLAYLISTCACHE *getFirstTrackPlaylist (TPLAYLISTMANAGER *plm)
{
	const int total = playlistManagerGetTotal(plm);
	for (int i = 0; i < total; i++){
		 PLAYLISTCACHE *plc = playlistManagerGetPlaylist(plm, i);
		 if (playlistGetCount(plc, PLAYLIST_OBJTYPE_TRACK))
		 	return plc;
	}

	return NULL;
}

static inline void initCS (TVLCPLAYER *vp)
{
	vp->gui.hUpdateEvent = CreateEvent(NULL, 0, 0, NULL);
	vp->ctx.hEvent = CreateEvent(NULL, 0, 0, NULL);
	vp->ctx.hVideoLock = lockCreate("videoLock");
	vp->ctx.hVideoCBLock = lockCreate("vlcEventsLock");
}

static inline void deleteCS (TVLCPLAYER *vp)
{
	CloseHandle(vp->gui.hUpdateEvent);
	CloseHandle(vp->ctx.hEvent);
	lockClose(vp->ctx.hVideoLock);
	lockClose(vp->ctx.hVideoCBLock);
}

static inline int page_videoStartup (TPAGEVIDEO *video, TVLCPLAYER *vp, const int width, const int height)
{
	
	// set a few defaults
	TGUIINPUT *cursor = &vp->gui.cursor;
	cursor->isHooked = 0;
	cursor->slideHoverEnabled = 0;
	cursor->draw = 1;
	cursor->x = 0;
	cursor->y = 0;
	cursor->dx = 0;
	cursor->dy = 0;
	cursor->LBState = 0;
	cursor->MBState = 0;
	cursor->RBState = 0;
	cursor->dragRect0.x = 0;
	cursor->dragRect0.y = 0;
	cursor->dragRect1.x = 0;
	cursor->dragRect1.y = 0;
	cursor->dragRectIsEnabled = 0;

	vp->gui.padctrlMode = BTN_CFG_PADCTRL_ON;
	vp->gui.hotkeys.cursor = 'A';
	vp->gui.hotkeys.console = 'L';
	vp->gui.idleTime = IDLETIME;
	vp->gui.idleFPS = UPDATERATE_IDLE;
	vp->gui.targetRate = UPDATERATE_ALIVE;
	vp->gui.mOvrTime = MCTRLOVERLAYPERIOD;
	vp->gui.artSearchDepth = 4;
	vp->gui.artMaxWidth = width/1.500;
	vp->gui.artMaxHeight = height/1.083;
	vp->gui.runCount = 0;
	vp->gui.lastTrack = -1;
	vp->gui.page_gl = PAGE_NONE;
	vp->vlc->volume = 50;

	vp->lastRenderTime = 0.0;

	//vp->gui.hRenderLock = lockCreate("frameRender");
	//vp->gui.hLoadLock = lockCreate("mediaLoad");

	return 1;
}


static inline int page_videoInitalize (TPAGEVIDEO *video, TVLCPLAYER *vp, const int width, const int height)
{
	setPlaybackMode(vp, PLAYBACKMODE_AUDIO);

	vp->gui.image[IMGC_SHADOW_BLK] = imageManagerImageAdd(vp->im, L"common/artshadow_blk.png");
	vp->gui.image[IMGC_SHADOW_BLU] = imageManagerImageAdd(vp->im, L"common/artshadow_blu.png");
	vp->gui.image[IMGC_SHADOW_GRN] = imageManagerImageAdd(vp->im, L"common/artshadow_grn.png");
	
	vp->gui.image[IMGC_NOART_SHELF_SELECTED] = imageManagerImageAdd(vp->im, L"shelf/noartselected.png");
	vp->gui.image[IMGC_NOART_SHELF_PLAYING] = imageManagerImageAdd(vp->im, L"shelf/noartplaying.png");
	vp->gui.image[IMGC_POINTER] = imageManagerImageAdd(vp->im, L"common/cursor.png");

	//vp->gui.cursorUnderlay = lNewFrame(cur->hw, cur->width, cur->height, LFRM_BPP_32);
		
	createVideoBuffers(vp);
	initCS(vp);
	wcscpy(vp->gui.snapshot.filename, SNAPSHOTFILE);
	vp->gui.marquee = marqueeNew(height/21.0, MARQUEE_LEFT, DMSG_FONT);
	vp->gui.picQueue = picQueueNew(vp->im, 16, 16);
	return 1;
}

static inline int page_videoShutdown (TPAGEVIDEO *video, TVLCPLAYER *vp)
{

	//lDeleteFrame(vp->gui.cursorUnderlay);
	deleteCS(vp);
	freeVideoBuffers(vp);
	picQueueDelete(vp->gui.picQueue);
	marqueeDelete(vp->gui.marquee);

	lockClose(vp->gui.hRenderLock);
	vp->gui.hRenderLock = NULL;
	lockClose(vp->gui.hLoadLock);
	vp->gui.hLoadLock = NULL;

	return 1;
}

static inline int page_videoInput (TPAGEVIDEO *video, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	//printf("@ page_videoInput msg:%i\n",msg);
	
#if 1
	if (0 && vp->gui.cursor.isHooked && getPlayState(vp) && !flags){
		vlc_cursorSet(vp->vlc->mp, vp->gui.cursor.dx, vp->gui.cursor.dy);
		vlc_cursorClicked(vp->vlc->mp, vp->gui.cursor.dx, vp->gui.cursor.dy);
		
		//printf("page_videoInput: %i %i\n", vp->gui.cursor.dx, vp->gui.cursor.dy);
	}else{
	
		switch(msg){
		  case PAGE_IN_TOUCH_DOWN:
		  case PAGE_IN_TOUCH_SLIDE:
			overlaySetOverlay(vp);
			break;
			
		  case PAGE_IN_WHEEL_FORWARD:
	  		setVolumeDisplay(vp, setVolume(vp, getVolume(vp, VOLUME_APP)+3, VOLUME_APP));
			break;
			
		  case PAGE_IN_WHEEL_BACK:
	  		setVolumeDisplay(vp, setVolume(vp, getVolume(vp, VOLUME_APP)-3, VOLUME_APP));
			break;
		}
	}
#endif
	return 1;
}

static inline int page_videoRender (TPAGEVIDEO *video, TVLCPLAYER *vp,  TFRAME *frame)
{
#if 1
	//printf("page_videoRender\n");
	
	if (pageRenderGetTop(vp->pages) != PAGE_META){
		TVIDEOOVERLAY *playctrl = pageGetPtr(vp, PAGE_OVERLAY);
		marqueeDraw(vp, frame, playctrl->marquee, 2, 2);

		if (0 && vp->gui.cursor.isHooked && getPlayState(vp))
			vlc_cursorSet(vp->vlc->mp, vp->gui.cursor.dx, vp->gui.cursor.dy);
	}
#endif
	return 1;
}

void page_videoRenderStart (TPAGEVIDEO *video, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	if (playlistManagerGetTotal(vp->plm) <= 1){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, 0);
		if (!plc || playlistGetCount(plc, PLAYLIST_OBJTYPE_TRACK) < 1)
			page2Set(vp->pages, PAGE_HOME, 0);
	}
	vp->gui.frameCt = 0;
}

static inline void OnDriveNotification (TPAGEVIDEO *video, const int event, const char drive, const int isUsb, const char *drivePath)
{
	wchar_t imgTop[MAX_PATH+1];
	__mingw_snwprintf(imgTop, MAX_PATH, L"common/letters/%lc.png", drive);
	wchar_t *imgBtm = NULL;
	
	switch (event){
	case PAGE_MSG_DRIVE_ARRIVE:
		if (isUsb)
			imgBtm = L"common/drives/driveadded_usb.png";
		else
			imgBtm = L"common/drives/driveadded.png";
		break; 

	case PAGE_MSG_DRIVE_DEPART:
		if (isUsb)		// .. or was
			imgBtm = L"common/drives/driveremoved_usb.png";
		else
			imgBtm = L"common/drives/driveremoved.png";
		break;
		              
	case PAGE_MSG_MEDIA_ARRIVE:
		if (isUsb)
			imgBtm = L"common/drives/mediaadded_usb.png";
		else
			imgBtm = L"common/drives/mediaadded.png";
		break;

	case PAGE_MSG_MEDIA_DEPART:
		if (isUsb)
			imgBtm = L"common/drives/mediaremoved_usb.png";
		else
			imgBtm = L"common/drives/mediaremoved.png";
		break;
	}
	
    if (imgBtm){
    	TVLCPLAYER *vp = video->com->vp;
    	wakeup(vp);
		picQueueAdd(vp->gui.picQueue, imgBtm, imgTop, getTickCount()+10000);
		timerSet(vp, TIMER_EXPPAN_REBUILD, 50);
		renderSignalUpdate(vp);
	}
}


int page_videoCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TPAGEVIDEO *video = (TPAGEVIDEO*)pageStruct;
	
	//if (msg != PAGE_CTL_RENDER)
	//	 printf("# page_videoCallback: %p %i %I64d %I64d %p %p\n", video, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_videoRender(video, video->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		page_videoRenderStart(video, video->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);

	}else if (msg == PAGE_CTL_RENDER_END){
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_videoInput(video, video->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_videoStartup(video, video->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_videoInitalize(video, video->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_videoShutdown(video, video->com->vp);
	
	}else if (msg == PAGE_MSG_DRIVE_ARRIVE || msg == PAGE_MSG_MEDIA_ARRIVE || msg == PAGE_MSG_DRIVE_DEPART || msg == PAGE_MSG_MEDIA_DEPART){
		OnDriveNotification(video, msg, dataInt1%0xFF, dataInt2, dataPtr);
		return 0;
	
	}else if (msg == PAGE_MSG_DEVICE_ARRIVE || msg == PAGE_MSG_DEVICE_DEPART){
		int vid = dataInt1;
		int pid = dataInt2;

		//printf("page_videoCallback %X %X\n", vid, pid);

		if (vid == SBUI_ISV_VID && pid == SBUI_ISV_PID){
			if (msg == PAGE_MSG_DEVICE_ARRIVE)
				timerSet(video->com->vp, TIMER_SBUI_CONNECTED, 1000);
			else
				timerSet(video->com->vp, TIMER_SBUI_DISCONNECTED, 1000);
		}
	}
	
	return 1;
}
		
static inline void renderEditbox (TVLCPLAYER *vp, TFRAME *frame, int x, int y, const int width, const int height_unused)
{
	const unsigned int *col = swatchGetPage(vp, PAGE_OVERLAY);
	lSetForegroundColour(frame->hw, col[SWH_OVR_EBOXTEXT]);
	lSetBackgroundColour(frame->hw, col[SWH_OVR_EBOXTEXTBK]);

	lSetCharacterEncoding(frame->hw, CMT_UTF16);
	PLAYLISTCACHE *plcD = getDisplayPlaylist(vp);
	TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);

	if (vp->input.iOffset > vp->input.caretPos-1)
		vp->input.iOffset = vp->input.caretPos;
	addCaret(&vp->input, vp->input.workingBuffer, vp->input.caretBuffer, EDITBOXIN_INPUTBUFFERLEN-1);
	y = drawEditBox(&vp->input, frame, x, y, width, vp->input.caretBuffer, &vp->input.iOffset);

	lSetCharacterEncoding(frame->hw, CMT_UTF8);
	x += 24+drawStrInt(frame, x+8, y-=16, "#", spl->from+1, col[SWH_OVR_EBOXTEXTBK]);
	x += 24+drawStrInt(frame, x, y, "", playlistGetTotal(plcD), col[SWH_OVR_EBOXTEXTBK]);
	drawStrInt(frame, x, y, "", playlistManagerGetTotal(vp->plm), col[SWH_OVR_EBOXTEXTBK]);
	//drawStrInt(frame, frame->width-100, y, "", artworkGetTotal(vp->tagc), col[SWH_OVR_EBOXTEXTBK]);
}

static inline void applyVideoTransformations (TFRAME *frame, TFRAME *working, TVLCCONFIG *vlc)
{

	if (vlc->swapColourBits){
		uint32_t *restrict pixels = (uint32_t*)lGetPixelAddress(frame, 0, 0);
		const int pxs = frame->frameSize>>2;
		for (int i = 0; i < pxs; i++)
			pixels[i] = ((pixels[i]&RGB_32_RED)>>16) | (pixels[i]&RGB_32_GREEN) | ((pixels[i]&RGB_32_BLUE)<<16);
	}

	if (vlc->pixelize)
		transPixelize(frame, vlc->pixelize);

	if (vlc->scaleFactor < 0.999){
		const int x = (frame->width-(frame->width*vlc->scaleFactor))/2;
		const int y = (frame->height-(frame->height*vlc->scaleFactor))/2;

		transScale(frame, working, frame->width*vlc->scaleFactor, frame->height*vlc->scaleFactor, x, y, vlc->scaleOp|SCALE_CLEANDES);
		if (vlc->rotateAngle){
			memset(frame->pixels, 0, frame->frameSize);
			transRotate(working, frame, vlc->rotateAngle, vlc->rotateOp);

		}else{
			my_memcpy(frame->pixels, working->pixels, frame->frameSize);
		}
	}else if (vlc->scaleFactor > 1.001){
		if (vlc->rotateAngle){
			memset(working->pixels, 0, working->frameSize);

			transRotate(frame, working, vlc->rotateAngle, vlc->rotateOp);
			transScale(working, frame, frame->width*vlc->scaleFactor, frame->height*vlc->scaleFactor, 0, 0, vlc->scaleOp|SCALE_CLEANDES);
		}else{
			transScale(frame, working, frame->width*vlc->scaleFactor, frame->height*vlc->scaleFactor, 0, 0, vlc->scaleOp);
			my_memcpy(frame->pixels, working->pixels, frame->frameSize);
		}
	}else{
		if (vlc->rotateAngle){
			memset(working->pixels, 0, working->frameSize);

			transRotate(frame, working, vlc->rotateAngle, vlc->rotateOp);
			my_memcpy(frame->pixels, working->pixels, frame->frameSize);
		}
	}

	if (vlc->blurRadius)
		transBlur(frame, vlc->blurRadius);
}

static inline int drawUnderlayMeta (TVLCPLAYER *vp, TFRAME *frame)
{
	/*const int noneState = page2RenderGetState(vp->pages, PAGE_NONE);
	const int mediaState = page2RenderGetState(vp->pages, PAGE_MEDIASTATS);
	const int metaState = page2RenderGetState(vp->pages, PAGE_META);
	printf("drawUnderlayMeta %i %i %i, %i %i\n", noneState, metaState, mediaState, pageGet(vp), pageInputGetTop(vp->pages));
	*/
	
	if (pageInputGetTop(vp->pages) == PAGE_NONE){
		TVIDEOOVERLAY *playctrl = pageGetPtr(vp, PAGE_OVERLAY);
		if (!playctrl->marquee->ready){
			PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
			if (!plc) plc = getDisplayPlaylist(vp);

	  		if (plc){
	  			TMETA *meta = pageGetPtr(vp, PAGE_META);
				TMETADESC desc;
				metaCopyDesc(&desc, meta);

	  			desc.x = 6; desc.y = 2;
	  			desc.w = frame->width - (desc.x*4);
	  			desc.h = frame->height - (desc.y*2);
	  			desc.trackPosition = plc->pr->playingItem;
	  			desc.textHeight = 0;
	  			desc.uid = playlistManagerGetPlaylistUID(vp->plm, plc);
	  			
	  			//printf("metaRender\n");
				metaRender(vp, frame, meta, &desc, META_FONT, 0);
				return 1;
			}
		}
	}
	return 0;
}

static inline void saveSnapshot (TVLCPLAYER *vp, TFRAME *frame, wchar_t *filename, const int annouce)
{
	if (lSaveImage(frame, filename, IMG_PNG, 0, 0) && annouce)
		dbwprintf(vp, L"Snapshot written to %s", filename);
}

static inline void composeFrame (TVLCPLAYER *vp, TFRAME *frame)
{
	if (getPlayState(vp) && (getPlaybackMode(vp) == PLAYBACKMODE_VIDEO /*|| (getPlaybackMode(vp) == PLAYBACKMODE_AUDIO && vp->gui.visual)*/)){
		copyVideo(vp, vp->vlc, vp->ctx.working, frame, vp->ctx.aspect.preset);
		applyVideoTransformations(frame, vp->ctx.workingTransform, vp->vlc);

	}else{
		if (getIdle(vp)){
			memset(frame->pixels, 0, frame->frameSize);

		}else if (vp->gui.skin.bg){
			TFRAME *base = vp->gui.skin.bg;
			const size_t blen = MIN(base->frameSize, frame->frameSize);
			TVIDEOOVERLAY *playctrl = pageGetPtr(vp, PAGE_OVERLAY);

			if (!playctrl->marquee->ready && !vp->gui.marquee->ready && !vp->gui.picQueue->total
			  && !page2RenderGetState(vp->pages, PAGE_EXIT) && !page2RenderGetState(vp->pages, PAGE_MEDIASTATS)
			  && (pageRenderGetTop(vp->pages) == PAGE_VIDEO) && !mHookGetState() && !kHookGetState()){
			  	
			  	const int playState = getPlayState(vp);
				//int draw = (vp->gui.drawMetaTrackbar && ((playState == 1 && vp->gui.frameCt&0x01) || ((playState == 2 || !playState) && vp->gui.frameCt < 3)));
				//draw |= (!vp->gui.drawMetaTrackbar && (vp->gui.frameCt < 3));
				//draw |= (playState && vp->gui.drawVisuals);

				const int draw =
					(vp->gui.frameCt < 3) ||
					(vp->gui.drawMetaTrackbar && (playState == 1 && vp->gui.frameCt&0x01));

				if (draw){
					my_memcpy((uint32_t*)frame->pixels, (uint32_t*)base->pixels, blen);
					drawUnderlayMeta(vp, frame);
				}
			}else{
				my_memcpy((uint32_t*)frame->pixels, (uint32_t*)base->pixels, blen);
				//memset(frame->pixels, 0, frame->frameSize);
				drawUnderlayMeta(vp, frame);
			}
		}
	}

	page2Render(vp->pages, frame, PAGE_TEXTOVERLAY);
	

#if (0 && ENABLE_BASS)
	if (!getIdle(vp) && getPlayState(vp))
		bass_render(&vp->bass, frame, ((frame->width - vp->bass.vwidth)/2)-27, frame->height-10);
#endif

	marqueeDraw(vp, frame, vp->gui.marquee, 4, 2);
	if (picQueueGetTotal(vp->gui.picQueue))
		picQueueRender(vp->gui.picQueue, frame, getTickCount(), 2, 2);

	if (kHookGetState() && !page2RenderGetState(vp->pages, PAGE_TETRIS) && !vp->gui.snapshot.save)
		renderEditbox(vp, frame, 3, frame->height-3, frame->width-6, 0);

	TGUIINPUT *cursor = &vp->gui.cursor;
	if (cursor->dragRectIsEnabled && cursor->LBState == 2){
		int x1 = cursor->dragRect0.x;
		int y1 = cursor->dragRect0.y;
		int x2 = cursor->dragRect1.x;
		int y2 = cursor->dragRect1.y;
		lDrawRectangleFilled(frame, x1, y1, x2, y2, 60<<24|COL_PURPLE_GLOW);
		lDrawRectangle(frame, x1-1, y1-1, x2+1, y2+1, 180<<24|COL_PURPLE_GLOW);
	}


#if MOUSEHOOKCAP
	if (vp->gui.cursor.isHooked && cursorGetState(&vp->gui.cursor)){
#if DRAWCURSORCROSS
		//drawCursor(vp, frame, &vp->gui);
		lDrawLine(frame, vp->gui.cursor.dx, 0, vp->gui.cursor.dx, frame->height-1, 0xFF<<24 | COL_RED);
		lDrawLine(frame, 0, vp->gui.cursor.dy, frame->width-1, vp->gui.cursor.dy,  0xFF<<24 | COL_RED);
#else
		drawCursor(vp, frame, &vp->gui);
#endif
	}
#endif

	if (vp->gui.snapshot.save){
		if (vp->gui.snapshot.save == 2){
			vp->gui.snapshot.save = 0;
			saveSnapshot(vp, frame, vp->gui.snapshot.filename, vp->gui.snapshot.annouce);
			vp->gui.snapshot.annouce = 0;
		}else if (vp->gui.snapshot.save == 1){
			vp->gui.snapshot.save = 2;
			renderSignalUpdate(vp);
		}
	}
}

// don't allow Windows drag'n'drop to screw with the path and abililty to find the data files
void resetCurrentDirectory ()
{
	wchar_t drive[MAX_PATH+1];
	wchar_t dir[MAX_PATH+1];
	wchar_t szPath[MAX_PATH+1];
	GetModuleFileNameW(NULL, szPath, MAX_PATH);
	_wsplitpath(szPath, drive, dir, NULL, NULL);
	__mingw_swprintf(szPath, L"%ls%ls", drive, dir);
	
	SetCurrentDirectoryW(szPath);
}

static inline int initVLC (TVLCPLAYER *vp)
{
	return createVLCInstance(selectVLCConfig(vp), vp->ml->width, vp->ml->height, VIS_DISABLED);
}

#if ENABLE_BRIGHTNESS
int setDisplayBrightness (TVLCPLAYER *vp, int level)
{
	if (!vp->ml->enableVirtualDisplay && vp->ml->enableBrightness && vp->ml->display[0]){
		intptr_t value = (intptr_t)level;
		return lSetDisplayOption(vp->ml->hw, vp->ml->display[0]->did, lOPT_USBD480_BRIGHTNESS, &value);
	}
	return -1;
}
#endif

// attempt to play whatever has been passed through the command line
int processCommandline (TVLCPLAYER *vp)
{
	int ret = 0;
	int argc = 0;
	wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	PLAYLISTCACHE *plc = getPrimaryPlaylist(vp);
	TPLAYLISTMANAGER *plm = vp->plm;

	if (argc > 1){
		int len = wcslen(argv[1]);		// fixup a Windows bug
		if (len > 2){
			if (argv[1][len-1] == L'\"')
				argv[1][len-1] = L'\\';
		}

		if (!wcscmp(argv[1], L"-noplaylist")){
			vp->playlist.noPlaylist = 1;
			return -1;
			
		}else if (isPlaylistW(argv[1])){	 // import a utf8 encoded playlist
			TFILEPANE *filepane = pageGetPtr(vp, PAGE_FILE_PANE);
			ret = importPlaylistW(plm, plc, vp->tagc, vp->am, argv[1], filepane);
			resetCurrentDirectory();
			int p1total = playlistGetTotal(plc);

			if (ret && p1total){
				int startTrack = 0;					// load track 'n-1' from playlist. -1 == play last track

				if (argc > 2){
					wchar_t *end = L"\0\0";
					int trk = wcstol(argv[2], &end, 0);

					if (trk > 0 && trk <= ret){		// set start track
						startTrack = trk-1;

					}else if (trk == -1){			// set last track as start
						startTrack = ret-1;

					}else if (!trk){				// perform a search for argv[2]
						char *out = convertto8(argv[2]);
						if (out){
#if !RELEASEBUILD
							printf("searching for '%s'\n",out);
#endif
							startTrack = playlistSearch(plc, vp->tagc, out, 0);
							my_free(out);
						}
					}
				}

				const unsigned int hash = playlistGetHash(plc, startTrack);
				const int pos = setPlaylistPlayingItem(vp, plc, startTrack, hash);
				char path[MAX_PATH_UTF8+1];

				playlistGetPath(plc, pos, path, MAX_PATH_UTF8);
				if (*path){
					wchar_t *out = converttow(path);
					if (out){
						if (!isVideoFile(out)){	// filter type must be set before track is started
							filepaneSetFilterMask(filepane, FILEMASKS_AUDIO);
							setPlaybackMode(vp, 1);
						}else{
							filepaneSetFilterMask(filepane, FILEMASKS_VIDEO);
							setPlaybackMode(vp, 0);
						}
						my_free(out);
					}
					if (pos >= 0)
						ret = startPlaylistTrack(vp, plc, pos);
				}
			}
			LocalFree(argv);
			return ret;
		}
	}else{
		if (argv) LocalFree(argv);
		return 0;
	}

	// try to play media file if passed. if not a file then assume a directory so import it
	char *out = convertto8(argv[1]);
	char *options = vlc_configureGetOptions(out);

	setPlaybackMode(vp, isVideoFile(argv[1]) == 0);
	
	wchar_t drive[MAX_PATH+1];
	wchar_t dir[MAX_PATH+1];
	wchar_t fname[MAX_PATH+1];
	wchar_t ext[MAX_PATH+1];
	wchar_t buffer[MAX_PATH+1];

	int len, isDir;
	wchar_t *pathNoOpt = argv[1];
	wchar_t *pathfull = my_wcsdup(argv[1]);

	if (stripOptionsW(pathfull)){
		removeTrailingSpacesW(pathfull);
		pathNoOpt = removeLeadingSpacesW(pathfull);
		isDir = isDirectoryW(pathNoOpt);
	}else{
		isDir = isDirectoryW(pathfull);
	}

	if (isDir){
		plc = playlistManagerCreatePlaylist(plm, "imported folder", 0);
		playlistAddPlc(getPrimaryPlaylist(vp), plc);
  		//vp->displayPlaylist = playlistManagerGetPlaylistIndex(vp->plm, plc);
		//vp->queuedPlaylist = vp->displayPlaylist;
		setDisplayPlaylist(vp, plc);
	  	setQueuedPlaylist(vp, plc);
		plc->parent->pr->selectedItem = playlistManagerGetPlaylistIndex(vp->plm, plc);
		//plc->parent->pr->selectedItem = getDisplayPlaylistUID(vp);
	}

	int ilen = wcslen(pathNoOpt);
	if (isDir){
		if (pathNoOpt[ilen-1] == L'\\' || pathNoOpt[ilen-1] == L'/')
			len = __mingw_snwprintf(buffer, MAX_PATH, L"%ls", pathNoOpt);
		else
			len = __mingw_snwprintf(buffer, MAX_PATH, L"%ls\\", pathNoOpt);
			_wsplitpath(buffer, drive, dir, fname, ext);
	}else{
		_wsplitpath(pathNoOpt, drive, dir, fname, ext);
		len = __mingw_snwprintf(buffer, MAX_PATH, L"%ls%ls", drive, dir);
	}

	if (!len){
		wcscpy(buffer, argv[1]);
		len = wcslen(buffer);
	}

	if (len){
		int trkStart = -1, pos = -1;

		if (!isVideoFile(pathNoOpt))	// filter type must be set before playlist is built
			filepaneSetFilterMask(pageGetPtr(vp, PAGE_FILE_PANE), FILEMASKS_MEDIA);
		else
			filepaneSetFilterMask(pageGetPtr(vp, PAGE_FILE_PANE), FILEMASKS_VIDEO);

		if (pathNoOpt[ilen-1] == L'\"') pathNoOpt[ilen-1] = L'\\';

		//wprintf(L"media #%s#\n", out);

		if (isMediaScreen(out)){
			trkStart = playlistAdd(plc, out);
			playlistSetTitle(plc, trkStart, "Desktop", 0);

		}else if (isMediaDVD(out)){
			trkStart = playlistAdd(plc, out);
			playlistSetTitle(plc, trkStart, "DVD", 0);

		}else if (isMediaDShow(out)){
			trkStart = playlistAdd(plc, out);
			playlistSetTitle(plc, trkStart, "DirectShow", 0);

		}else if (isMediaDVB(out)){
			trkStart = playlistAdd(plc, out);
			playlistSetTitle(plc, trkStart, "Digital TV", 0);

		}else if (isMediaRemote(out)){
			trkStart = playlistAdd(plc, out);
			playlistSetTitle(plc, trkStart, out, 0);
			
		}else if (isDir){
			//trkStart = browserImportPlaylistByDirW(plm, fb, pathNoOpt, plc, 1, NULL);
			TFILEPANE *filepane = pageGetPtr(vp, PAGE_FILE_PANE);
			trkStart = filepaneBuildPlaylistDir(filepane, plc, buffer, FILEMASKS_MEDIA, 1);
			if (trkStart == -1){
				if (isVideoFile(pathNoOpt))
					filepaneSetFilterMask(filepane, FILEMASKS_AUDIO);
				else
					filepaneSetFilterMask(filepane, FILEMASKS_VIDEO);
				//playlistDelete(plc);
				//trkStart = browserImportPlaylistByDirW(plm, fb, pathNoOpt, plc, 1);
			}

			// give the playlist a title, in this case, the folder name
			if (wcslen(dir) > 1){
				if (pathNoOpt[ilen-1] == L'\\' || pathNoOpt[ilen-1] == L'/') pathNoOpt[ilen-1] = 0;
				wchar_t tmp[2][MAX_PATH+1];

				__mingw_snwprintf(tmp[1], _MAX_FNAME, L"%ls.ext", pathNoOpt);
				_wsplitpath(tmp[1], NULL, NULL, tmp[2], NULL);

				char *name = convertto8(tmp[2]);
				if (name){
					playlistSetName(plc, name);
					my_free(name);
				}
			}else{
				__mingw_snwprintf(buffer, _MAX_FNAME, L"%ls\\", drive);
				char *name = convertto8(buffer);
				if (name){
					playlistSetName(plc, name);
					my_free(name);
				}
			}
		}else{
			//trkStart = browserImportPlaylistByDirW(plm, fb, buffer, plc, 0, NULL);
			TFILEPANE *filepane = pageGetPtr(vp, PAGE_FILE_PANE);
			trkStart = filepaneBuildPlaylistDir(filepane, plc, buffer, FILEMASKS_MEDIA, 0);
			if (trkStart)
				trkStart = playlistGetPositionByHash(plc, getHash(out));

			// unable to auto import, or track does not have a path
			// try adding manually
			if (trkStart == -1) trkStart = playlistAdd(plc, out);
		}

		if (!isDir){
			tagAdd(vp->tagc, out, MTAG_PATH, out, 1);
			pos = setPlaylistPlayingItem(vp, plc, trkStart, getHash(out));

			if (options)
				playlistSetOptions(plc, pos, options, strlen(options));

		}else if (playlistGetTotal(plc)){
			if (argc > 2){
				wchar_t *end = L"\0";
				pos = wcstol(argv[2], &end, 0)-1;
				if (pos >= 0)
					setPlaylistPlayingItem(vp, plc, pos, playlistGetHash(plc, pos));
			}else{
				pos = 0;
				playlistChangeEvent(vp, plc, pos);
			}
		}

		if (pos >= 0)
			ret = startPlaylistTrack(vp, plc, pos);
#if 0
		if (ret){
			// set browser location to the current input path
			__mingw_snwprintf(dir, MAX_PATH, L"%ls%ls", fname, ext);
			if (browserNavigateToDirectoryAndFile(vp, fb, buffer/*the dir*/, dir/*the file*/)){
				if (!fb->path->isRoot) ccDisable(fb->panel);
			}
		}
		if (!ret)
			ret = browserNavigateToDirectory(vp, fb, buffer/*argv[1]*/);
		if (ret && !fb->path->isRoot) ccDisable(fb->panel);
#endif

#if 0
		if (isDir)
			playlistGetTrackLengths(vp, plc, 1, 0);
#endif
	}

	my_free(out);
	if (pathfull) my_free(pathfull);
	if (options) my_free(options);
	if (argv) LocalFree(argv);

	return ret;
}

static inline void copySourceVideoFrame (TVLCPLAYER *vp)
{
	my_memcpy((uint32_t*)vp->ctx.pixelBuffer, (uint32_t*)vp->ctx.pixels, vp->ctx.bufferSize);
}

void playerRun (TVLCPLAYER *vp)
{
	//TFRAME *tmp = lCloneFrame(getFrontBuffer(vp));

	TFRAME *const front = getFrontBuffer(vp);
	//int ct = 0;
	//double tt = 0.0;

	do{
		int ret = waitForUpdateSignal(vp);
		if (!getApplState(vp)) break;

		if (!ret){
			if (renderLock(vp)){
				timerCheckAndFire(vp, getTime(vp));
				renderUnlock(vp);
			}
		/*if (!ret){

			if (vp->gui.cursorMoved){
				int x = vp->gui.cursorX;
				int y = vp->gui.cursorY;
				TFRAME *cur = imageCacheGetImage(vp->imgc, vp->gui.image[IMGC_POINTER]);
				TFRAME *under = vp->gui.cursorUnderlay;

				copyAreaNoBlend(under, getFrontBuffer(vp), x, y, 0, 0, cur->width-1, cur->height-1);
				lRefreshArea(getFrontBuffer(vp), x, y, x+cur->width-1, y+cur->height-1);
				x = vp->gui.dx-MOFFSETX;
				y = vp->gui.dy-MOFFSETY;
				drawCursor(vp, getFrontBuffer(vp), &vp->gui);
				lRefreshArea(getFrontBuffer(vp), x, y, x+cur->width-1, y+cur->height-1);
				vp->gui.cursorMoved = 0;
			}*/
		}else if (ret){
			if (isVideoFrameAvailable(vp)){
				if (waitForVLCUpdateSignal(vp)){
					if (getApplState(vp)){
						//lockVLCVideoBuffer(vp);
						copySourceVideoFrame(vp);
						//unlockVLCVideoBuffer(vp);
					}
				}
			}

			double t1 = getTime(vp);
			if (renderLock(vp)){
				timerCheckAndFire(vp, t1);
				renderUnlock(vp);
			}

			if (vp->renderState){
				if (renderLock(vp)){
					vp->gui.renderId++;
					t1 = getTime(vp);
					vp->dTime[vp->gui.frameCt&0x0F] = t1 - vp->fTime;
					vp->fTime = t1;
					composeFrame(vp, front);
					vp->rTime = getTime(vp) - t1;
					//printf("@@ frameTime %.2f\n", vp->rTime);
				
					if (vp->gui.frameCt++&0x10) vp->gui.frameCt = 0;
					if (vp->gui.drawStats)
						drawFPSOverlay(vp, front, getFPS(vp), front->width-52, front->height-18);
						
					renderUnlock(vp);
				}
			}

#if !RELEASEBUILD
			processConsoleInput(vp);
#endif

			if (!vp->ctx.winRender.enable && vp->renderState)
				libmylcd_Render(front);

			//printf("%p %p %p %p\n", GetForegroundWindow(), GetDesktopWindow(), GetShellWindow(), getDesktopHWND());

			if (vp->ctx.winRender.enable){
				//t1 = getTime(vp);
				HDC hdc = GetDC(vp->ctx.winRender.wnd);
				
				//if (vp->ctx.winRender.copyMode == 1){
					int cx = 0;//(vp->ctx.winRender.cxScreen - front->width) / 2;
					//if (cx < 0) cx = 0;
					int cy = 0;//(vp->ctx.winRender.cyScreen - front->height) / 2;
					//if (cy < 0) cy = 0;

					SetDIBitsToDevice(hdc, cx, cy, front->width, front->height,
						0, 0, 0, front->height, front->pixels, vp->ctx.winRender.bitHdr, DIB_RGB_COLORS);

				/*}else if (vp->ctx.winRender.copyMode == 2){
					SetStretchBltMode(hdc, COLORONCOLOR);
					StretchDIBits(hdc, 0, 0, vp->ctx.winRender.wStretched, vp->ctx.winRender.hStretched, 
						0, 0, front->width, front->height, front->pixels, vp->ctx.winRender.bitHdr, DIB_RGB_COLORS, SRCCOPY);
				}else if (vp->ctx.winRender.copyMode == 3){
					SelectObject(hdc, vp->ctx.winRender.off_bitmap);
 					BitBlt(hdc, 0, 0,
               			vp->ctx.winRender.wStretched, vp->ctx.winRender.hStretched,
               			vp->ctx.winRender.off_dc, 0, 0, SRCCOPY);
				}*/
				ReleaseDC(vp->ctx.winRender.wnd, hdc);
				
				/*vp->rTime = getTime(vp) - t1;
				tt += vp->rTime;
				ct++;
				if (ct >= 30){
					printf("@@ frameTime %.4f %.4f %.4f\n", vp->rTime, tt, tt/(double)ct);
					ct = 0;
					tt = 0.0;
				}*/
			}

			if (!vp->gui.idleDisabled && vp->gui.awake && (!getPlayState(vp) || getPlayState(vp) == 2 ||
				(/*vp->gui.visual == VIS_DISABLED &&*/ getPlaybackMode(vp) == 1 && pageGet(vp) == PAGE_OVERLAY))
				&& pageGet(vp) != PAGE_HOTKEYS && pageGet(vp) != PAGE_VKEYBOARD
#if (ENABLE_ANTPLUS)
				&& pageGet(vp) != PAGE_ANTPLUS 
				&& pageGet(vp) != PAGE_TCX
#endif
				){
				if ((uint32_t)((uint32_t)timeGetTime() - vp->gui.awakeTime) > vp->gui.idleTime){
					if (!kHookGetState() && !mHookGetState()){	// don't idle if editbox or mouse is enabled
						//setIdle(vp);
	  					timerSet(vp, TIMER_SETIDLEB, 0);
						updateTickerStart(vp, vp->settings.general.idleFps);
						vp->gui.frameCt = 0;
						renderSignalUpdate(vp);
					}
				}
			}
			if (renderLock(vp)){
				timerCheckAndFire(vp, getTime(vp));
				renderUnlock(vp);
			}
		}
	}while(getApplState(vp));

	//lDeleteFrame(tmp);
}

static inline int isAnotherInstanceRunning ()
{
	HANDLE handle = CreateEvent(NULL, 0, 0, NAME_INSTANCEEVENT);
	int ret = (GetLastError() == ERROR_ALREADY_EXISTS);
	if (handle)
		CloseHandle(handle);
	return ret;
}

static inline HANDLE getWindowHandle ()
{
	return FindWindowA(NAME_WINMSG, NULL);
}

#if (ENABLE_SINGLEINSTANCE)

static inline void singleInstancePassCmdLine (const int argc, const char *argv[])
{
	HANDLE hWin = getWindowHandle();
	if (hWin && argc > 1){
		PostMessage(hWin, WM_WAKEUP, 0, 0);

		int argcw = 0;
		wchar_t **argvw = CommandLineToArgvW(GetCommandLineW(), &argcw);
		if (argvw){
			HANDLE hEvent = CreateEvent(NULL, 0, 0, NAME_INSTANCEEVENT);
			if (hEvent){
				SendMessage(hWin, WM_WAKEUP, 0, 0);

				if (WaitForSingleObject(hEvent, 8000) == WAIT_OBJECT_0){	// don't hang around if remote thread doesn't wake or respond, try anyways
					for (int i = 1; i < argcw; i++){
						char *path = convertto8(argvw[i]);
						if (path){
#if SINGLEINSTANCE_USE_CDS
							cds_send(hWin, WM_CDS_ADDTRACK, path, strlen(path));
#else
							const unsigned int hash = getHash(path);
							const int len = strlen(path);
							SendMessage(hWin, WM_ADDTRACKINIT, hash, len+1);
							for (int i = 0; i <= len; i++)
								SendMessage(hWin, WM_ADDTRACKCHAR, hash, (i<<16)|(unsigned char)path[i]);
#endif
							my_free(path);
						}
					}
				}
				CloseHandle(hEvent);
			}
			LocalFree(argvw);
		}
	}
}

#endif

TVLCPLAYER * playerNew ()
{
	TVLCPLAYER *vp = my_calloc(1, sizeof(TVLCPLAYER));
	if (vp){
		vp->instanceModule = GetModuleHandle(0);
		vp->instanceCheck = isAnotherInstanceRunning();
		vp->instanceEvent = CreateEventA(NULL, 0, 0, NAME_INSTANCEEVENT);
		vp->pid = processGetId();
		
		vp->gui.hRenderLock = lockCreate("frameRender");
		vp->gui.hLoadLock = lockCreate("mediaLoad");
	}
	return vp;
}

void playerDelete (TVLCPLAYER *vp)
{
	//printf("playerDelete %p %p\n", vp->instanceEvent, vp->ml);
	CloseHandle(vp->instanceEvent);

	TMYLCD *ml = vp->ml;
	my_free(vp);
	if (ml) libmylcd_Close(ml);
}

#if 0
void setDeviceExitScreen (TMYLCD *ml)
{
	TFRAME *frame = lNewFrame(ml->hw, ml->width, ml->height, ml->bpp);
	if (frame){
		const int x = 20;
		int y = 20;
		const int lineh = 24;
		lSetForegroundColour(ml->hw, 255<<24|COL_WHITE);

		//lPrintf(frame, x, y, DMSG_FONT, LPRT_CPY, " VlcStream, by %s", mySELF);
		lPrintf(frame, x, y, DMSG_FONT, LPRT_CPY, " %s-%s", PLAYER_VERSION, libmylcdVERSION);
		lPrintf(frame, x, y+=lineh, DMSG_FONT, LPRT_CPY, "%s", " web: mylcd.sourceforge.net");
		lPrintf(frame, x, y+=lineh, DMSG_FONT, LPRT_CPY, "%s", " email: okio@users.sourceforge.net");
		lPrintf(frame, x, y+=lineh, DMSG_FONT, LPRT_CPY, "%s", " libvlc v%s", libvlc_get_version());
		lPrintf(frame, x, y+=lineh, DMSG_FONT, LPRT_CPY, "%s", " compiler: %s", libvlc_get_compiler());
		//lPrintf(frame, x, y+=lineh, DMSG_FONT, LPRT_CPY, " compiler: gcc version %i.%i (GCC)", __GNUC__, __GNUC_MINOR__);
		lPrintf(frame, x, y+=lineh, DMSG_FONT, LPRT_CPY, " &#169; %s", mySELF);

		lSleep(30);
		lRefresh(frame);
		lSleep(30);
		lDeleteFrame(frame);
	}
}
#endif

int playerLoadDefaultPlaylist (TVLCPLAYER *vp, const wchar_t *playlist)
{

	int ret = 0;

	TM3U *m3u = m3uNew();
	if (m3u){
		if (m3uOpen(m3u, VLCSPLAYLIST, M3U_OPENREAD)){
			printf("\n****** reading playlist *********\n");

			double t0 = getTime(vp);
			ret = m3uReadPlaylist(m3u, vp->plm, getPrimaryPlaylist(vp), vp->tagc, vp->am, pageGetPtr(vp, PAGE_FILE_PANE));
			double t1 = getTime(vp);
			__mingw_wprintf(L"%i records read from '%ls' in %.2fms\n", ret, playlist, t1-t0);
			printf("*** reading playlist complete ***\n");
			m3uClose(m3u);
		}

		m3uFree(m3u);
		if (ret)		// tell the playlist renderer to update itself as we've changed something
			playlistChangeEvent(vp, getPrimaryPlaylist(vp), 0);
	}
	return ret;
}


static inline TFRAME *displayBackgroundLoadRandom (TVLCPLAYER *vp)
{
	//printf("displayGetRandomBackground\n");
	
	// load and select a random background image
	vp->gui.skin.bgPathTotal = 0;
	
	str_list *strList = NULL;
	settingsGetW(vp, "skin.bgimage.", &strList);
	if (strList){
		if (!(vp->gui.skin.bgPathTotal=strList->total)){
			cfg_configStrListFree(strList);
			my_free(strList);
			return NULL;
		}

		if (1 || vp->gui.runCount)
			vp->gui.skin.currentIdx = rand()%vp->gui.skin.bgPathTotal;
		else
			vp->gui.skin.currentIdx = 0;

		wchar_t *path = (wchar_t*)cfg_configStrListItem(strList, vp->gui.skin.currentIdx);
		if (path){
			const int oldId = vp->gui.image[IMGC_BGIMAGE];
			if (oldId){
				imageManagerImageRelease(vp->im, oldId);
				imageManagerImageSetPath(vp->im, oldId, path);
			}else{
				vp->gui.image[IMGC_BGIMAGE] = imageManagerImageAdd(vp->im, path);
			}
			//vp->gui.image[IMGC_BGIMAGE] = imageManagerImageAdd(vp->im, path);
			//if (oldId && oldId != vp->gui.image[IMGC_BGIMAGE])
			//	imageManagerImageDelete(vp->im, oldId);
		}

		cfg_configStrListFree(strList);
		my_free(strList);
	}

	int width = 0, height = 0;
	if (artManagerImageGetMetrics(vp->im, vp->gui.image[IMGC_BGIMAGE], &width, &height)){
		const int w = getFrontBuffer(vp)->width;
		const int h = getFrontBuffer(vp)->height;
		if (width != w || height != h)
			artManagerImageResize(vp->im, vp->gui.image[IMGC_BGIMAGE], w, h);
	}

	vp->gui.skin.bg = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_BGIMAGE]);
	return vp->gui.skin.bg;
}

static inline int displaySetStartupScreen (TVLCPLAYER *vp, THWD *hw, TFRAME *frame)
{
	TFRAME *bg = displayBackgroundLoadRandom(vp);
	if (!bg) return 0;
		
	lSetForegroundColour(hw, 0xFFE0E0E0);
	const int blurOp = LTR_BLUR4;
	lSetRenderEffect(hw, blurOp);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_COLOUR, COL_PURPLE_GLOW);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_RADIUS, 4);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_SETTOP, 1);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_ALPHA, 850);

	fastFrameCopy(bg, frame, 0, 0);
	lPrintf(frame, 10, 10, STARTUP_LOADING_FONT, 0, "%s", "Loading...");
	libmylcd_Render(frame);
	lSetRenderEffect(hw, LTR_DEFAULT);
	
	return 1;
}

int configStartDisplay (TVLCPLAYER *vp, const int enabledVD)
{
	int did = 0;
	int width = DEVICE_DEFAULT_WIDTH;	// fail safe values
	int height = DEVICE_DEFAULT_HEIGHT;
	
	char *name = settingsGetStr(vp, "display.device");
	if (!name) return 0;
	if (strcmp(name, DEVICE_DEFAULT_NAME)){
		settingsGet(vp, "display.width", &width);
		settingsGet(vp, "display.height", &height);
	}

	TMYLCD *ml = libmylcd_Init(width, height, SKINFILEBPP);
	if (!ml) return 0;

	did = libmylcd_StartDisplay(ml, name, width, height, 0);
	if (!did){
		settingsGet(vp, "display.width", &width);
		settingsGet(vp, "display.height", &height);

		libmylcd_Close(ml);
		ml = libmylcd_Init(width, height, SKINFILEBPP);
		did = libmylcd_StartDisplay(ml, name, width, height, 0);
	}

	if (!did && enabledVD){
		int limit = 1;
		settingsGet(vp, "device.virtual.restrictWindowSize", &limit);
		if (limit){
			width = MIN(GetSystemMetrics(SM_CXSCREEN), width);
			height = MIN(GetSystemMetrics(SM_CYSCREEN), height);
			libmylcd_Close(ml);
			ml = libmylcd_Init(width, height, SKINFILEBPP);
		}

		did = libmylcd_StartDisplay(ml, "DDRAW", width, height, DISPLAYMAX-1);
		ml->enableTouchInput = 0;
		ml->enableBrightness = 0;
		ml->enableVirtualDisplay = 1;
		ml->virtualDisplayId = did;
	}
	
	if (did){
		if (vp->ml)
			libmylcd_Close(vp->ml);
		vp->ml = ml;
	}else{
		libmylcd_Close(ml);
	}	
		
	my_free(name);
	return did;
}

void configLoadSwatch (TVLCPLAYER *vp)
{
	wchar_t *swatchFile = NULL;
	settingsGetW(vp, "skin.swatch", &swatchFile);
	if (swatchFile){
		swatchLoad(vp, &vp->gui.skin.swatch, swatchFile);
		my_free(swatchFile);
	}
}

int playerSetup (TVLCPLAYER *vp)
{
	QueryPerformanceCounter((LARGE_INTEGER*)&vp->tStart);
	QueryPerformanceFrequency((LARGE_INTEGER *)&vp->freq);
	vp->resolution = 1.0 / (double)vp->freq;
	
	//srand((int)((double)(getTickCount()&0x1FFFFF)/getTime(vp)));
	srand(getTickCount()&0xFFFFF);

	configLoad(vp, CFGFILE);
	configStartDisplay(vp, 1);

	vp->im = imageManagerNew(vp->ml->hw);
	vp->am = artManagerNew(vp->ml->hw);
	if (!vp->im || !vp->am) return 0;

	wchar_t buffer[MAX_PATH+1];
	wchar_t *skin = NULL;
	settingsGetW(vp, "skin.folder", &skin);
	if (skin){
		__mingw_snwprintf(buffer, MAX_PATH, L"%ls/%ls", SKINDROOT, skin);
		imageManagerSetPathPrefix(vp->im, buffer);
		my_free(skin);
	}
	
	if (!displaySetStartupScreen(vp, vp->ml->hw, getFrontBuffer(vp)))
		return 0;

	configLoadSwatch(vp);

	vp->cc = ccInit(vp, &vp->gui.cursor);
	if (vp->cc){
		ccSetImageManager(vp->cc, CC_IMAGEMANAGER_ART, vp->am);
		ccSetImageManager(vp->cc, CC_IMAGEMANAGER_IMAGE, vp->im);
	}
	
	vp->strc = strcNew(vp->ml->hw);
	vp->tagc = tagcNew(vp->ml->hw);
	vp->plm = playlistManagerNew();
	if (!vp->tagc || !vp->plm)
		return 0;

	//if (!vp->jc)
	//	vp->jc = jobControllerNew(vp, jobThreadWorkerFunc);
	
	vp->renderState = 1;
	vlcEventsInit();
	vp->playlist.root = playlistManagerCreatePlaylist(vp->plm, PLAYLIST_PRIMARY, 0);
	vp->playlist.display = -1;
	vp->playlist.queued = -1;

	InitCom();
	initVLC(vp);
#if ENABLE_CMDFUNSTUFF
	vp->bot.sheets = sheetsNew(4);
#endif
	editboxDoCmdRegistration(&vp->input, vp);

	lCacheCharacterRange(vp->ml->hw, 0, 126, LFONT);
	lCacheCharacterRange(vp->ml->hw, 0, 126, CTRLOVR_LISTBOX_FONT);

	vp->pages = pages2New(vp, PAGE_VIDEO);
	page2Register(vp->pages, "Video", PAGE_VIDEO, 0, page_videoCallback, sizeof(TPAGEVIDEO));
	page2Register(vp->pages, "Playback ctrl", PAGE_OVERLAY, 0, page_plyctrlCallback, sizeof(TVIDEOOVERLAY));
	page2Register(vp->pages, "Home", PAGE_HOME, 0, page_homeCallback, sizeof(TAPPLAUNCHER));
	page2Register(vp->pages, "My Computer", PAGE_EXP_PANEL, 0, page_expPanCallback, sizeof(TEXPPANEL));
	page2Register(vp->pages, "Playlist Treeview", PAGE_PLY_TV, 0, page_plyTvCallback, sizeof(TPLYTV));
	page2Register(vp->pages, "Playlist Panel", PAGE_PLY_PANEL, 0, page_plyPanCallback, sizeof(TPLYPANEL));
	page2Register(vp->pages, "Playlist Pane", PAGE_PLY_PANE, 0, page_plyPaneCallback, sizeof(TPLYPANE));
	page2Register(vp->pages, "Playlist Album", PAGE_PLY_SHELF, 0, page_plyAlbCallback, sizeof(TSPL));
	page2Register(vp->pages, "Playlist Flat (plm)", PAGE_PLY_FLAT, 0, page_plyPlmCallback, sizeof(TSPL));
	page2Register(vp->pages, "Config", PAGE_CFG, 0, page_cfgCallback, sizeof(TCFG));
	page2Register(vp->pages, "Meta", PAGE_META, 0, page_metaCallback, sizeof(TMETA));
	page2Register(vp->pages, "Media playback stats", PAGE_MEDIASTATS, 0, page_msCallback, sizeof(TMEDIASTATS));
	page2Register(vp->pages, "General text overlay", PAGE_TEXTOVERLAY, 0, page_textCallback, sizeof(TTEXTOVERLAY));
	page2Register(vp->pages, "Chapters", PAGE_CHAPTERS, 0, page_chapCallback, sizeof(TCHAPTER));
	page2Register(vp->pages, "Subtitles", PAGE_SUB, 0, page_subCallback, sizeof(TSUB));
	page2Register(vp->pages, "Programme guide", PAGE_EPG, 0, page_epgCallback, sizeof(TEPG));
	page2Register(vp->pages, "Codec and ES info", PAGE_ES, 0, page_esCallback, sizeof(TSTREAMINFO));
	page2Register(vp->pages, "Clock", PAGE_CLOCK, 0, page_clkCallback, sizeof(TCLK));
	page2Register(vp->pages, "Image", PAGE_IMGOVR, 0, page_imgOvrCallback, sizeof(TIOVR));
	page2Register(vp->pages, "Exit ctrl", PAGE_EXIT, 0, page_exitCallback, sizeof(TEXIT));
	page2Register(vp->pages, "Image pane", PAGE_IMGPANE, 0, imgPane_Callback, sizeof(TIMGPANE));
	page2Register(vp->pages, "File pane", PAGE_FILE_PANE, 0, page_filePaneCallback, sizeof(TFILEPANE));
	page2Register(vp->pages, "Video transform", PAGE_TRANSFORM, 0, page_tfCallback, sizeof(TTRANSFORM));
	page2Register(vp->pages, "Audio equalizer", PAGE_EQ, 0, page_eqCallback, sizeof(TEQ));
	page2Register(vp->pages, "Search", PAGE_SEARCH, 0, page_searchCb, sizeof(TSEARCH));
	page2Register(vp->pages, "Global hotkeys", PAGE_HOTKEYS, 0, page_ghkCallback, sizeof(TGLOBALHOTKEYS));
	page2Register(vp->pages, "Virtual Keyboard", PAGE_VKEYBOARD, 0, page_vkbCallback, sizeof(TKEYBOARD));
	page2Register(vp->pages, "Tetris", PAGE_TETRIS, 0, page_tetrisCallback, sizeof(TTETRIS));
	page2Register(vp->pages, "Task manager", PAGE_TASKMAN, 0, page_taskmanCallback, sizeof(TTASKMAN));
	page2Register(vp->pages, "Garmin .tcx parser", PAGE_TCX, 0, page_tcxCallback, sizeof(TTCX));
	page2Register(vp->pages, "Alarm", PAGE_ALARM, 0, page_alarmCb, sizeof(TALARM));
#if ENABLE_ANTPLUS
	page2Register(vp->pages, "Ant+ HRM", PAGE_ANTPLUS, 0, page_antCallback, sizeof(TANTPLUS));
#endif

	timerInit(vp, TIMER_NEWTRACKVARS1, getNewTrackVariables, NULL);
	timerInit(vp, TIMER_NEWTRACKVARS2, getNewTrackVariables, NULL);
	timerInit(vp, TIMER_NEWTRACKVARS3, getNewTrackVariables, NULL);
	timerInit(vp, TIMER_GETTRACKVARDELAYED, getNewTrackVariables, NULL);
	timerInit(vp, TIMER_GOTONEXTTRACK, startNextTrackPlayback, NULL);
	timerInit(vp, TIMER_PREVTRACK, trackPrev, NULL);
	timerInit(vp, TIMER_NEXTTRACK, trackNext, NULL);
	timerInit(vp, TIMER_REWIND, trackRewind, NULL);
	timerInit(vp, TIMER_FASTFORWARD, trackFastforward, NULL);
	timerInit(vp, TIMER_PLAYPAUSE, trackPlayPause, NULL);
	timerInit(vp, TIMER_PAUSE, trackPause, NULL);
	timerInit(vp, TIMER_STOPPLAY, timer_stopplay, NULL);	// stop befor eplaying
	timerInit(vp, TIMER_PLAY, timer_play, NULL);
	timerInit(vp, TIMER_STOP, trackStop, NULL);
	timerInit(vp, TIMER_VOL_APP_UP, volumeUp, NULL);
	timerInit(vp, TIMER_VOL_APP_DN, volumeDown, NULL);
	timerInit(vp, TIMER_VOL_MASTER_UP, volumeWinUp, NULL);
	timerInit(vp, TIMER_VOL_MASTER_DN, volumeWinDown, NULL);
	timerInit(vp, TIMER_SHUTDOWN, shutdownAppl, NULL);
	//timerInit(vp, TIMER_SWITCHVIS, switchVis, NULL);
	timerInit(vp, TIMER_ARTCLEANUP, artcleanup, NULL);
	timerInit(vp, TIMER_PLAYTRACK, timer_playtrack, NULL);
	timerInit(vp, TIMER_SETIDLEA, timer_setIdleA, NULL);
	timerInit(vp, TIMER_SETIDLEB, timer_setIdleB, NULL);
	timerInit(vp, TIMER_SETIDLEC, timer_setIdleC, NULL);
	timerInit(vp, TIMER_SAVECONFIG, timer_saveConfig, NULL);
	timerInit(vp, TIMER_VLCEVENTS_CLEANUP, vlcEventsCleanup, NULL);
	timerInit(vp, TIMER_PATHREGWRITE, updateModuleRegPathEntry, NULL);

	//timerInit(vp, TIMER_EXP_FILTERCHANGE, expFilterChange, NULL);
	timerInit(vp, TIMER_EXPPAN_REBUILD, expanTimerPanelRebuild, NULL);
	timerInit(vp, TIMER_EXPPAN_REBUILDSETPAGE, expanTimerPanelRebuildSetPage, NULL);
	timerInit(vp, TIMER_CTRL_PLAYLISTLBREFRESH, timer_playlistListboxRefresh, NULL);
	timerInit(vp, TIMER_CTRL_UPDATETIMESTAMP, overlayTimeStampSetTime, NULL);
	timerInit(vp, TIMER_CTRL_DISPLAYVOLRESET, overlayDisplayVolReset, NULL);
	timerInit(vp, TIMER_CTRL_OVERLAYRESET, overlayResetOverlay, NULL);

	timerInit(vp, TIMER_PLYALB_REFRESH, plyAlbRefresh, NULL);
	timerInit(vp, TIMER_PLYPLM_REFRESH, plyPlmRefresh, NULL);
	timerInit(vp, TIMER_PLYTV_REFRESH, plytvRefresh, NULL);
	timerInit(vp, TIMER_PLYPAN_REBUILD, plyPanRebuild, NULL);
	timerInit(vp, TIMER_PLYPANE_REFRESH, timer_plyPaneRefresh, NULL);
	timerInit(vp, TIMER_PLYPAN_REBUILDCLNMETA, plyPanRebuildCleanMeta, NULL);
	timerInit(vp, TIMER_EPG_GENDVBPLAYLIST, epgDvbGenPlaylist, NULL);
	//timerInit(vp, TIMER_EPG_DISPLAYOSD, epgDisplayOSD, NULL);	// is unstable
	timerInit(vp, TIMER_EPG_UPDATE, epgGetUpdate, NULL);
	timerInit(vp, TIMER_ES_UPDATE, esGetUpdate, NULL);
	timerInit(vp, TIMER_META_UPDATE, metaGetUpdate, NULL);
	timerInit(vp, TIMER_SUB_UPDATE, subtitleGetUpdate, NULL);
	timerInit(vp, TIMER_CHAPTER_UPDATE, chaptersGetUpdate, NULL);
	timerInit(vp, TIMER_STATEHELPER, stateHelper, NULL);
	timerInit(vp, TIMER_IMAGECACHEFLUSH, timer_cacheFlush, NULL);
	timerInit(vp, TIMER_REG_TRACK_UPDATE, timer_regTrackInfoUpdate, NULL);
	timerInit(vp, TIMER_TASKBARTITLE_UPDATE, timer_drawTaskbarTrackTitle, NULL);
	timerInit(vp, TIMER_SBUI_CONNECTED, timer_sbuiConnected, NULL);
	timerInit(vp, TIMER_SBUI_DISCONNECTED, timer_sbuiDisconnected, NULL);
	timerInit(vp, TIMER_ALARM, timer_alarm, NULL);
	timerInit(vp, TIMER_FLUSH, timer_flushcaches, NULL);
	timerInit(vp, TIMER_SEARCH_ENDED, timer_searchEnded, NULL);
	timerInit(vp, TIMER_SEARCH_UPDATEHEADER, timer_searchUpdateHeader, NULL);
	timerInit(vp, TIMER_SEARCH_METACB, timer_metaCb, NULL);
	
	timerInit(vp, TIMER_testingonly, timertest, NULL);


	if (isSBUIEnabled(vp))
		sbuiStartImageThread(vp, HIGH_PRIORITY_CLASS);
	else
		sbuiStartImageThread(vp, HIGH_PRIORITY_CLASS|CREATE_SUSPENDED);


	setApplState(vp, 1);
	configApply(vp);
	pageDispatchMessage(vp->pages, PAGE_MSG_CFG_READ, 0, 0, NULL);
	page2Enable(vp->pages, PAGE_META);
	

#if ((ENABLE_BASS || !RELEASEBUILD) && !WIN64)
	bass_start(&vp->bass, vp->gui.drawVisuals, getFrontBuffer(vp)->width/1.25f, getFrontBuffer(vp)->height/2, vp);
#endif

	touchDispatcherStart(vp, touchIn, vp);

	// try to play whatever has been passed via the command line
	// if unsuccessful then try to load the default playlist (created at last exit)
	int cret = processCommandline(vp);
	if (!cret)
		cret = playerLoadDefaultPlaylist(vp, VLCSPLAYLIST);

	wakeup(vp);
	
	if (cret < 1){
		page2Set(vp->pages, PAGE_HOME, 1);
	}else{
		overlaySetOverlay(vp);
		if (getPlayState(vp) == 1){
			lSleep(10);
			timerSet(vp, TIMER_CTRL_OVERLAYRESET, 1000);
		}
	}

	vp->gui.runCount++;
	return 1;
}

void playerClose (TVLCPLAYER *vp)
{
	
	//ccCleanupMemory(vp->cc);
	touchDispatcherStop(vp);
	
	vlcEventsClose();
	vlcEventListInvalidate(vp->vlc);
	//if (vp->jt) artQueueFlush(artThreadGetWork(vp->jt));


	pageDispatchMessage(vp->pages, PAGE_MSG_CFG_WRITE, 0, 0, NULL);
	configSave(vp, CFGFILE);
	pages2Delete(vp->pages);
#if ENABLE_CMDFUNSTUFF
	sheetsFree(vp->bot.sheets);
#endif

	deleteShadows(vp->gui.shadow);
	//jobControllerDelete(vp->jc);
	ccDestroy(vp->cc);
	my_free(vp->cc);

#if (ENABLE_BASS || !RELEASEBUILD)
	bass_close(&vp->bass);
#endif

	
	CleanupVistaVolume();
	UninitCom();
	closeVLC(vp);
	strcFree(vp->strc);
	tagcFree(vp->tagc);
	playlistManagerDelete(vp->plm);
	configFree(vp);

	imageManagerImageRelease(vp->im, vp->gui.image[IMGC_BGIMAGE]);
	imageManagerDelete(vp->im);
	artManagerDelete(vp->am);

}

#if (ENFORCE_VLCVERSION)
static inline void invalidVLCVersion (const char *need, const char *have)
{
	char buffer[1024];
	__mingw_snprintf(buffer, 1024,
	  "This version of VLC is incompatible with %s.\n"
	  "(Require '%s' but found '%s')\n"
	  "\n"
	  "VLC %s may be downloaded from: http://www.videolan.org\n"
	  "\n"
	  "\n"
	  "Click 'OK' to Exit",
	  PLAYER_NAME, need, have, need
	);

	MessageBoxA(NULL,
	  buffer,
	  PLAYER_NAME" - "PLAYER_DATE,
	  MB_SYSTEMMODAL|MB_ICONSTOP|MB_OK
	);
}
#endif

static inline int setHeapOptions ()
{
	return HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
}

/*
int IsCurrentUserLocalAdministrator()
{
	SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
	PSID AdministratorsGroup;
  
	int b = AllocateAndInitializeSid(&NtAuthority,2,SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,0, 0, 0, 0, 0, 0,&AdministratorsGroup);
	if (b){
    	if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
        	 b = 0;
		FreeSid(AdministratorsGroup);
	}

	return b;
}*/



static inline int my_main (TVLCPLAYER *vp, const int argc, const char *argv[])
{
	//printf("main tid:%i %i\n", (int)GetCurrentThreadId, (int)GetCurrentProcessorNumber());
	//printf("main isadmin:%i\n", (int)IsCurrentUserLocalAdministrator());

#if (!ALLOWDEBUGGER)
	if (IsDebuggerPresent()) return -1;
#endif

#if (ENFORCE_VLCVERSION)
	const char *version = (const char*)vlc_getVersion();

#if (LIBVLC_VERSION_MAJOR == 2 && LIBVLC_VERSION_MINOR == 0)
	if (strncmp(version, "2.0", 3)){
		invalidVLCVersion("2.0.x", version);
		return -2;
	}
#elif (LIBVLC_VERSION_MAJOR == 2 && LIBVLC_VERSION_MINOR == 1)
	if (strncmp(version, "2.1", 3)){
		invalidVLCVersion("2.1.x", version);
		return -3;
	}
#elif (LIBVLC_VERSION_MAJOR == 1 && LIBVLC_VERSION_MINOR == 1)
	if (strncmp(version, "1.1", 3)){
		invalidVLCVersion("1.1.x", version);
		return -4;
	}

#endif
#endif


#if (ENABLE_ENFORCEMINSPEC)
#if (LIBVLC_VERSION_MAJOR >= 2)
	int supports = cpuHasMMX();
	supports += cpuHasSSE();
	supports += cpuHasSSE2();

	if (supports != 3 || cpuGetProcessorCount() < 2){
		MessageBoxA(NULL,
		  "System does not meet minimum required CPU spec for "PLAYER_NAME".\n(Dual core with MMX, SSE and SSE2)\n\nClick 'OK' to Exit",
		  PLAYER_NAME" - "PLAYER_DATE,
		  MB_SYSTEMMODAL|MB_ICONSTOP|MB_OK
		);
		return -5;
	}
#endif
#endif

#if (ENABLE_SINGLEINSTANCE)
	if (vp->instanceCheck){
		singleInstancePassCmdLine(argc, argv);
		return -6;
	}
#endif


  	processSetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
	setHeapOptions();
	processSetPriority(ABOVE_NORMAL_PRIORITY_CLASS);
	resetCurrentDirectory();
	timeBeginPeriod(1);
	
	if (!playerSetup(vp)){
		//exit(EXIT_FAILURE);
		//printf("we're going down!!\n");
		return 0;
	}

	int lastPlaylist = -1;
	settingsGet(vp, "lasttrack.playlist", &lastPlaylist);
	if (lastPlaylist >= 0){
		//vp->queuedPlaylist = vp->displayPlaylist = displayPlaylist;
		vp->playlist.queued = playlistManagerGetUIDByIndex(vp->plm, lastPlaylist);
		vp->playlist.display = vp->playlist.queued;

		//PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, vp->playlist.display);
		//printf("idx:%i, uid:%i '%s'\n", lastPlaylist, vp->playlist.queued, plc->title);
	}

	if (playlistManagerGetTotal(vp->plm) <= 1){
		vp->playlist.display = playlistManagerGetPlaylistUID(vp->plm, getPrimaryPlaylist(vp));
		vp->playlist.queued = vp->playlist.display;

	}else{

		// ensure meta is retrieved for first/current playlist
		// and set previously playing track and playlist
		PLAYLISTCACHE *plc;
		if (lastPlaylist < 0 || lastPlaylist >= playlistManagerGetTotal(vp->plm)){
			plc = getFirstTrackPlaylist(vp->plm);

		}else{
			plc = getQueuedPlaylist(vp);
			if (plc){
				if (vp->gui.lastTrack < playlistGetTotal(plc))
					plc->pr->selectedItem = vp->gui.lastTrack;
				else
					plc->pr->selectedItem = -1;

				const int isTrack = playlistGetItemType(plc, plc->pr->selectedItem) == PLAYLIST_OBJTYPE_TRACK;
				if (!isTrack){
					plc = getFirstTrackPlaylist(vp->plm);
					plc->pr->selectedItem = 0;
				}

				plc->pr->playingItem = plc->pr->selectedItem;
				//printf("vp->gui.lastTrack %i %i %i\n", vp->gui.lastTrack, plc->pr->selectedItem, isTrack);
			}
		}

		if (plc){
			//TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
			if (1/*overlayPlaylistListboxFill(vp, plyctrl->lbPlaylist, plc)*/){
				setDisplayPlaylist(vp, plc);
	  			setQueuedPlaylist(vp, plc);
				if (plc->parent)
					plc->parent->pr->selectedItem = playlistManagerGetPlaylistIndex(vp->plm, plc);
			}

#ifndef _DEBUG_
			playlistChangeEvent(vp, plc, 0);
			//invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), 0);
			//bringAlbumToFocus(pageGetPtr(vp, PAGE_PLY_SHELF),0);
#endif
		}

		TPLYPANEL *plypan = pageGetPtr(vp, PAGE_PLY_PANEL);
		plc = getDisplayPlaylist(vp);
		if (!plc) plc = getPrimaryPlaylist(vp);
		plyPanelBuild(vp, plypan->panel, plc);

		//should not be here
		playlistMenuSetTipTrack(vp, &vp->gui.tray, NULL, getQueuedPlaylistUID(vp), plc->pr->playingItem);
	}


	//pageSet(vp, PAGE_HOME);
	//pageSet(vp, PAGE_TCX);
	//pageSet(vp, PAGE_TASKMAN);
	//pageSet(vp, PAGE_ALARM);
	//pageSet(vp, PAGE_OVERLAY);
	//pageSet(vp, PAGE_EXP_PANEL);
	//pageSet(vp, PAGE_PLY_TV);
	//pageSet(vp, PAGE_PLY_PANEL);
	//pageSet(vp, PAGE_PLY_SHELF);
	//pageSet(vp, PAGE_PLY_PANE);
	//pageSet(vp, PAGE_PLY_FLAT);
	//pageSet(vp, PAGE_CHAPTERS);
	//pageSet(vp, PAGE_SUB);
	//pageSet(vp, PAGE_CFG);
	//pageSet(vp, PAGE_TRANSFORM);
	//pageSet(vp, PAGE_CLOCK);
	//pageSet(vp, PAGE_SEARCH);
	//pageSet(vp, PAGE_EQ);
	//pageSet(vp, PAGE_ANTPLUS);
	//pageSet(vp, PAGE_HOTKEYS);
	//pageSet(vp, PAGE_VKEYBOARD);
	//pageSet(vp, PAGE_IMGOVR);
	//pageSet(vp, PAGE_IMGPANE);
	//pageSet(vp, PAGE_FILE_PANE);
	//pageSet(vp, PAGE_TEXTOVERLAY);
	//page2Set(vp->pages, PAGE_TEXTOVERLAY | PAGE_RENDER_CONCURRENT, 0);


#if RELEASEBUILD
	timerSet(vp, TIMER_PATHREGWRITE, 2000);
#endif
	timerSet(vp, TIMER_ARTCLEANUP, ARTWORKFLUSH_PERIOD+5000);
	timerSet(vp, TIMER_STATEHELPER, 2000);
	timerSet(vp, TIMER_IMAGECACHEFLUSH, 1*60*1000);
	timerSet(vp, TIMER_REG_TRACK_UPDATE, 2000);
	timerSet(vp, TIMER_TASKBARTITLE_UPDATE, 500);
	

#if (!ENABLE_SINGLEINSTANCE)
	int64_t hWndOld = (int64_t)regGetQword(L"process_hwnd");
	int processIdOld = regGetDword(L"process_id");
#endif
	regSetDword(L"process_id", processGetId());
	regSetQword(L"process_hwnd", (QWORD)(intptr_t)vp->gui.hMsgWin);

	
	playerRun(vp);

	SHUTDOWN = 1;

#if (!ENABLE_SINGLEINSTANCE)
	//check if the instance previous to this is still running, if so revert to its details, otherwise clear
	HANDLE hProcess = processOpen(processIdOld);
	if (!hProcess && (processGetId() != regGetDword(L"process_id"))){
		processIdOld = regGetDword(L"process_id");
		hProcess = processOpen(processIdOld);
		if (hProcess)
			hWndOld = (intptr_t)getWindowHandle();
	}

	if (hProcess){
		processClose(hProcess);
		regSetDword(L"process_id", processIdOld);
		regSetQword(L"process_hwnd", hWndOld);
	}else{
		regSetDword(L"process_id", 0);
		regSetQword(L"process_hwnd", 0);
	}
#else
	regSetDword(L"process_id", 0);
	regSetQword(L"process_hwnd", 0);
#endif

	//printf("vp->imgc %i\n", vp->imgc->total);
	//vp->jt->threadState = ARTWORK_THREAD_EXIT;

	updateTickerStop(vp);
	timeEndPeriod(1);
	if (kHookGetState())
		kHookUninstall();
	if (getPlayState(vp)){
		trackStop(vp);
		unloadMedia(vp, vp->vlc);
	}
	 
	if (!vp->playlist.noPlaylist)
		playerWriteDefaultPlaylist(vp, VLCSPLAYLIST);

	playerClose(vp);
	//setDeviceExitScreen(vp->ml);

	return 1;
}

#if 0
unsigned int __stdcall keeper (void *uptr)
{
	printf("keeper started\n");
	
	while (!SHUTDOWN){
		fflush(stdout);
		
		int a = WaitForSingleObject(hMutex, 10000);
		printf("keeper %X\n", a);
		Sleep(1000000);
		lockRelease(hMutex);
	}

	printf("keeper ended\n");
	return 1;
}
#endif

int main (const int argc, const char *argv[])
{
	TVLCPLAYER *vp = playerNew();
	if (vp){
		//g_vp = vp;
		//unsigned int tid;
		//_beginthreadex(NULL, 0, keeper, NULL, 0, &tid);
		
		my_main(vp, argc, argv);
		playerDelete(vp);
		
		printf("\n:: Exited\n");
	}
	
	return EXIT_SUCCESS;
}

