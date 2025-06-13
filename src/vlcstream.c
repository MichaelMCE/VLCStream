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
	VLCStream for VLC 2.2.x
	VLCStream for VLC 3.0.19+

Install:
Copy vlcstream.exe and the vsskin directory to the root VLC directory where vlc.exe is located

Start with: 'vlcstream.exe "path/to/mediafile.ext"' or use the built-in explorer to initiate media playback
*/


#include "common.h"
#if !RELEASEBUILD
#include <conio.h>
#endif




TVLCPLAYER *g_vp = NULL;

volatile int SHUTDOWN = 0;
volatile double UPDATERATE_BASE = UPDATERATE_BASE_DEFAULT;
static volatile int MediaPlayerStopped_complete = 0;


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

void setTargetRate (TVLCPLAYER *vp, const double fps)
{
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
			isMediaDVB(name) ||
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
}

// TIMER_SETIDLEC
void timer_setIdleC (TVLCPLAYER *vp)
{
	//printf("timer_setIdleC %i\n", vp->gui.awake);
	
	if (vp->gui.awake) return;

	strcFlush(vp->strc);
	imageManagerFlush(vp->im);
	
	if (renderLock(vp)){
		artworkFlush(vp, vp->am);
		renderUnlock(vp);
	}

	invalidateShadows(vp->gui.shadow);
	ccLabelFlushAll(vp->cc);
	libmylcd_FlushFonts(vp->ml->hw);

	tagFlushOrfhansPlm(vp->tagc, vp->plm);
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
	if (renderLock(vp)){
		artworkFlush(vp, vp->am);
		renderUnlock(vp);
	}
	timerSet(vp, TIMER_ARTCLEANUP, ARTWORKFLUSH_PERIOD + (5*1000));
}

void setApplState (TVLCPLAYER *vp, int state)
{
	vp->applState = state;
	if (!state) SHUTDOWN = 1;
}

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
	/*
	0:audio or video with visualizations disabled
	1:audio with visuals enabled
	*/
	vp->currentFType = mode;
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

			while(MediaPlayerStopped_complete != 1){
				if (renderLock(vp)){
					timerCheckAndFire(vp, getTime(vp));
					renderUnlock(vp);
				}
				lSleep(5);
			}
			lSleep(25);
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
			wchar_t cmdline[4*len+1];
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
	if ((track < 0) || playlistGetItemType(plc, track) != PLAYLIST_OBJTYPE_TRACK)
		return 0;


	int ret = 0;
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
  	setVolume(vp, getVolume(vp, VOLUME_MASTER) + 5, VOLUME_MASTER);
}

// TIMER_VOL_MASTER_DN
void volumeWinDown (TVLCPLAYER *vp)
{
	setVolume(vp, getVolume(vp, VOLUME_MASTER) - 5, VOLUME_MASTER);
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
			(getPlaybackMode(vp) == PLAYBACKMODE_VIDEO));
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


int processKeyPress (TVLCPLAYER *vp, int ch)
{
	if (ch == 'p'){
		playerWriteDefaultPlaylist(vp, VLCSPLAYLIST);

	}else if (ch == 'f'){
		printf("~~~~~~~~~~~~~~\n");
		my_MemStatsDump(vp->ml->hw);
		printf("~~~~~~~~~~~~~~\n");
	}

	return 1;
}

static inline void processConsoleInput (TVLCPLAYER *vp)
{
	int ch = getKeyPress();
	if (!ch){
		//timerSet(vp, TIMER_SHUTDOWN, 0);
  		exitInitShutdown(vp);
	}else if (ch > 1){
		processKeyPress(vp, ch);
	}
}
#endif

double getFPS (TVLCPLAYER *vp)
{
	double t = 0.0;
	for (int i = 0; i < 16; i++)
		t += vp->dTime[i];
	t /= 16.0;
	return 1000.0/t;
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
	vp->gui.updateSignaled = 1;
}

void (CALLBACK updateTickerCB)(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)dwUser;

	if (getApplState(vp)){
		double t1 = getTime(vp);
		double lastRenderPeriod = (t1 - vp->lastRenderTime)/* - vp->rTime*/;
		double currentFps = (1.0/lastRenderPeriod)*1000.0;

		//printf("set event a %i %f\n", vp->gui.updateSignaled, currentFps);
		if (vp->gui.updateSignaled || currentFps - (currentFps*0.195) < vp->gui.targetRate){
			SetEvent(vp->gui.hUpdateEvent);
			//vp->gui.cursorMoved = 0;
			vp->gui.updateSignaled = 0;
			vp->lastRenderTime = t1;
		}
	}
}

void updateTickerStop (TVLCPLAYER *vp)
{
	if (vp->updateTimer){
		timeKillEvent(vp->updateTimer);
		vp->updateTimer = 0;
	}
}

void updateTickerStart (TVLCPLAYER *vp, double fps)
{
	if (fps > 100.0) fps = 100.0;
	if (fps != vp->gui.lastUpdateRate){
		vp->gui.lastUpdateRate = fps;
		updateTickerStop(vp);

		if (!vp->updateTimer){
			vp->updateTimer = (int)timeSetEvent((1.0/fps)*1000, 5, updateTickerCB, (DWORD_PTR)vp, TIME_PERIODIC|TIME_KILL_SYNCHRONOUS);
		}
	}
}

/*###########################################################################*/
/*###########################################################################*/
/*###########################################################################*/

void getNewTrackVariables (TVLCPLAYER *vp)
{
	if (!getApplState(vp) || !getPlayState(vp))
		return;
		
	static uint64_t tLast = 0;
	TVLCCONFIG *vlc = getConfig(vp);
	if (!vlc->mp) return;

	uint64_t t0 = getTickCount();
	if (t0 - tLast < 150){
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
				}
			}

			vlc_mediaParseAsync(vlc);
			cfgAttachmentsSetCount(vp, vlc_attachmentsGetCount(vlc));
		}
		loadUnlock(vp);
	}
	timerSet(vp, TIMER_CHAPTER_UPDATE, 1000);
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
	TVLCPLAYER *vp = g_vp;//(TVLCPLAYER*)udata;
	if (!getApplState(vp)) return;
	TVLCCONFIG *vlc = getConfig(vp);
	if (!vlc) return;


#if 0	/* print useful debug info but don't spam */
	if (event->type != libvlc_MediaPlayerPositionChanged && event->type != libvlc_MediaPlayerTimeChanged && event->type != libvlc_MediaDurationChanged && event->type != libvlc_MediaPlayerBuffering)
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
				char buffer[128];
				timeToString(vlc->length, buffer, sizeof(buffer)-1);
				if (*buffer)
					tagAdd(vp->tagc, path, MTAG_LENGTH, buffer, 1);
			}
			my_free(path);
		}

		if (getApplState(vp))
			timerSet(vp, TIMER_NEWTRACKVARS1, 1);

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

#if (ENABLE_SBUI)		
		sbuiDKStateChange();
#endif
		timerSet(vp, TIMER_CTRL_UPDATETIMESTAMP, 1);
				
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
			timerSet(vp, TIMER_NEWTRACKVARS1, 1); // 150

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
			timerSet(vp, TIMER_NEWTRACKVARS2, 175);
		break;

#if (LIBVLC_VERSION_MAJOR >= 2 /*&& LIBVLC_VERSION_MINOR >= 1*/)
	  case libvlc_MediaPlayerBuffering:
	  	//printf("buffering %%%.1f\n", event->u.media_player_buffering.new_cache);
	  	vp->vlc->bufferPos = event->u.media_player_buffering.new_cache/100.0;
	  	timerSet(vp, TIMER_NEWTRACKVARS3, 180);
	  	renderSignalUpdate(vp);
	  	break;
#endif

	  case libvlc_MediaSubItemAdded:{
	 	char *path = libvlc_media_get_mrl(event->u.media_subitem_added.new_child);
	 	if (path){
#if 1
	 		PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
			if (plc){
	 			playlistSetPath(plc, plc->pr->playingItem, path);
				timerSet(vp, TIMER_STOP, 1);
		 		timerSet(vp, TIMER_PLAY, 50);
	 		}
#endif
	 		free(path);
	 	}
	 	libvlc_media_release(event->u.media_subitem_added.new_child);
	 	break;
	  }
	  case libvlc_MediaParsedChanged:{
	  	if (getApplState(vp))
	  		timerSet(vp, TIMER_NEWTRACKVARS2, 1);

		char *title = getPlayingTitle(vp);
		if (!title)
			title = getPlayingPath(vp);

		if (title){
			TVIDEOOVERLAY *pctrl = pageGetPtr(vp, PAGE_OVERLAY);
			overlayAddTitle(pctrl, title);
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

	if (lockWait(g_vp->ctx.hVideoCBLock, INFINITE)){
		vlc_eventsCallback(event, udata);
		lockRelease(g_vp->ctx.hVideoCBLock);
	}
}

int loadMediaPlayer (TVLCPLAYER *vp, TVLCCONFIG *vlc, char *inlineOpts, char *opts)
{
	vlc_configure(vp, vlc, vlc->width, vlc->height, vlc->bpp, 0/*vp->gui.visual*/);

	if (inlineOpts && *inlineOpts)
		vlc_configureMediaOptions(vlc, inlineOpts);

	if (opts && *opts)
		vlc_configureMediaOptions(vlc, opts);

	vlc->mp = vlc_newFromMedia(vlc);
	if (vlc->mp){
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
	//vlc_addOption(vlc, "quiet");
	//vlc_addOption(vlc, "quiet-synchro");
	vlc_addOption(vlc, "http-album-art");
	vlc_addOption(vlc, "album-art=2");
	vlc_addOption(vlc, "album-art-filename=cover.jpg");

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

static inline int _browserLoadMediaFile (TVLCPLAYER *vp, char *path, char *opts)
{

	TVLCCONFIG *vlc = getConfig(vp);

	if (getPlayState(vp))
		player_stop(vp, vlc);

	if (vlc->isMediaLoaded)
		unloadMedia(vp, vlc);

	vlc->isMediaLoaded = loadMedia(vp, vlc, path, opts);
	if (vlc->isMediaLoaded){
		trackPlay(vp);
		vlc_mediaParseAsync(vlc);

		vlc->isMediaLoaded = vlc_willPlay(vlc);
		if (vlc->isMediaLoaded){
			timerSet(vp, TIMER_NEWTRACKVARS1, 1);
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

	return vlc->isMediaLoaded;
}

static inline int browserLoadMediaFile (TVLCPLAYER *vp, char *utf8path, char *opts)
{
	if (loadLock(vp)){
		int ret = _browserLoadMediaFile(vp, utf8path, opts);
		loadUnlock(vp);
		return ret;
	}
	return 0;
}

static inline void exitAppl (TVLCPLAYER *vp)
{
	SHUTDOWN = 1;
#if (ENABLE_SBUI)
	sbuiSetApplState(1);
#endif
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
			vlc->volume = volume;
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

		if (!wcscmp(argv[1], L"--noplaylist")){
			vp->playlist.noPlaylist = 1;
			return -1;
		}
		
		if (isPlaylistW(argv[1])){	 // import a utf8 encoded playlist
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
		if (pathNoOpt[ilen-1] == L'\\' || pathNoOpt[ilen-1] == L'/'){
			len = __mingw_snwprintf(buffer, MAX_PATH, L"%ls", pathNoOpt);
		}else{
			len = __mingw_snwprintf(buffer, MAX_PATH, L"%ls\\", pathNoOpt);
		}
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

void playerRun (TVLCPLAYER *vp)
{
	TFRAME *const front = getFrontBuffer(vp);

	do {
		int ret = waitForUpdateSignal(vp);
		if (!getApplState(vp)) break;

		if (!ret){
			if (renderLock(vp)){
				timerCheckAndFire(vp, getTime(vp));
				renderUnlock(vp);
			}
		}else if (ret){
			if (isVideoFrameAvailable(vp)){
				if (waitForVLCUpdateSignal(vp)){
					if (getApplState(vp)){
						//lockVLCVideoBuffer(vp);
						video_copySourceFrame(vp);
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
					video_composeFrame(vp, front);
					vp->rTime = getTime(vp) - t1;

					if (vp->gui.frameCt++&0x10) vp->gui.frameCt = 0;
					if (vp->gui.drawStats)
						video_drawFPSOverlay(vp, front, getFPS(vp), front->width-52, front->height-18);
						
					renderUnlock(vp);
				}
			}

#if !RELEASEBUILD
			processConsoleInput(vp);
#endif

			if (!vp->ctx.winRender.enable && vp->renderState)
				libmylcd_Render(front);

			if (vp->ctx.winRender.enable)
				video_copyToDesktop(vp, front, 0, 0);

			if (!vp->gui.idleDisabled && vp->gui.awake && (!getPlayState(vp) || getPlayState(vp) == 2 ||
				(getPlaybackMode(vp) == 1 && pageGet(vp) == PAGE_OVERLAY)) && pageGet(vp) != PAGE_HOTKEYS && pageGet(vp) != PAGE_VKEYBOARD
#if (ENABLE_ANTPLUS)
				  && (pageGet(vp) != PAGE_ANTPLUS) && (pageGet(vp) != PAGE_TCX)
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
	CloseHandle(vp->instanceEvent);

	TMYLCD *ml = vp->ml;
	my_free(vp);
	if (ml) libmylcd_Close(ml);
}

void playerShowDeviceExitScreen (TMYLCD *ml)
{
#if 0
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
		lPrintf(frame, x, y+=lineh, DMSG_FONT, LPRT_CPY, " libvlc v%s", libvlc_get_version());
		lPrintf(frame, x, y+=lineh, DMSG_FONT, LPRT_CPY, " compiler: %s", libvlc_get_compiler());
		//lPrintf(frame, x, y+=lineh, DMSG_FONT, LPRT_CPY, " compiler: gcc version %i.%i (GCC)", __GNUC__, __GNUC_MINOR__);
		lPrintf(frame, x, y+=lineh, DMSG_FONT, LPRT_CPY, " &#169; %s", mySELF);

		lSleep(20);
		lRefresh(frame);
		lSleep(20);
		lDeleteFrame(frame);
	}
#endif
}

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

static inline TFRAME *selectRandomVideoBackground (TVLCPLAYER *vp)
{
	// load and select a random background image
	vp->gui.skin.bgPathTotal = 0;
	
	str_list *strList = NULL;
	settingsGetW(vp, "skin.bgimage.", &strList);
	if (strList){
		if (!(vp->gui.skin.bgPathTotal=strList->total)){
			cfg_configStrListFreeStrings(strList);
			cfg_configStrListFree(strList);
			return NULL;
		}

		//if (vp->gui.runCount)
			vp->gui.skin.currentIdx = rand()%vp->gui.skin.bgPathTotal;
		//else
		//	vp->gui.skin.currentIdx = 0;

		wchar_t *path = (wchar_t*)cfg_configStrListItem(strList, vp->gui.skin.currentIdx);
		if (path){
			const int oldId = vp->gui.image[IMGC_BGIMAGE];
			if (oldId){
				imageManagerImageRelease(vp->im, oldId);
				imageManagerImageSetPath(vp->im, oldId, path);
			}else{
				vp->gui.image[IMGC_BGIMAGE] = imageManagerImageAdd(vp->im, path);
			}
		}

		cfg_configStrListFreeStrings(strList);
		cfg_configStrListFree(strList);
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
	lSetForegroundColour(hw, 0xFFE0E0E0);
	const int blurOp = LTR_BLUR4;
	lSetRenderEffect(hw, blurOp);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_COLOUR, COL_PURPLE_GLOW);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_RADIUS, 4);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_SETTOP, 1);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_ALPHA, 850);

	fastFrameCopy(vp->gui.skin.bg, frame, 0, 0);
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
	if (name){
		settingsGet(vp, "display.width", &width);
		settingsGet(vp, "display.height", &height);
	}else{
		name = my_strdup(DEVICE_DEFAULT_NAME);
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

int playerConfigLoad (TVLCPLAYER *vp)
{
	int ret = 0;
	int argc = 0;
	
	wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv){
		for (int i = 0; i < argc; i++){
			if (!wcscmp(argv[i], L"--config")){
				if (i+1 < argc){
					printf("Reading config: %ls\n", argv[i+1]);
					ret = configLoad(vp, argv[i+1], 0);
					break;
				}
			}
		}
		LocalFree(argv);
	}

	if (ret < 1)
		ret = configLoad(vp, CFGFILE, 1);
	return ret;
}

int playerDisplayStart (TVLCPLAYER *vp)
{
	return configStartDisplay(vp, 1);
}

int playerSetup (TVLCPLAYER *vp, const int startPage)
{
	timeBeginPeriod(1);
	QueryPerformanceCounter((LARGE_INTEGER*)&vp->tStart);
	QueryPerformanceFrequency((LARGE_INTEGER *)&vp->freq);
	vp->resolution = 1.0 / (double)vp->freq;
	
	srand(getTickCount()&0xFFFFF);

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
	
	TFRAME *bg = selectRandomVideoBackground(vp);
	if (!bg) return 0;
	
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
#if ENABLE_GARMINTCX
	page2Register(vp->pages, "Garmin .tcx parser", PAGE_TCX, 0, page_tcxCallback, sizeof(TTCX));
#endif
	page2Register(vp->pages, "Alarm", PAGE_ALARM, 0, page_alarmCb, sizeof(TALARM));
	page2Register(vp->pages, "Media Player", PAGE_PLY_QUEUE, 0, page_queueCb, sizeof(TPLYQUEUE));
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
#if (ENABLE_SBUI)
	timerInit(vp, TIMER_SBUI_CONNECTED, timer_sbuiConnected, NULL);
	timerInit(vp, TIMER_SBUI_DISCONNECTED, timer_sbuiDisconnected, NULL);
#endif
	timerInit(vp, TIMER_ALARM, timer_alarm, NULL);
	timerInit(vp, TIMER_FLUSH, timer_flushcaches, NULL);
	timerInit(vp, TIMER_SEARCH_ENDED, timer_searchEnded, NULL);
	timerInit(vp, TIMER_SEARCH_UPDATEHEADER, timer_searchUpdateHeader, NULL);
	timerInit(vp, TIMER_SEARCH_METACB, timer_metaCb, NULL);
	timerInit(vp, TIMER_testingonly, timertest, NULL);

#if (ENABLE_SBUI)
	if (isSBUIEnabled(vp))
		sbuiStartImageThread(vp, HIGH_PRIORITY_CLASS);
#endif

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
		page2Set(vp->pages, startPage, 1);
	}else{
		overlaySetOverlay(vp);
		if (getPlayState(vp) == 1){
			lSleep(10);
			timerSet(vp, TIMER_CTRL_OVERLAYRESET, 1000);
		}
	}

	page2Set(vp->pages, startPage, 1);

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

int playerInit ()
{
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

  	processSetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
	setHeapOptions();
	processSetPriority(ABOVE_NORMAL_PRIORITY_CLASS);
	resetCurrentDirectory();

	return 1;
}

int playerInstanceCheck (TVLCPLAYER *vp, const int argc, const char *argv[])
{

#if (ENABLE_SINGLEINSTANCE)
	if (vp->instanceCheck){
		singleInstancePassCmdLine(argc, argv);
		return -6;
	}
#endif

	return 1;
}

int playerStartup (TVLCPLAYER *vp, const int argc, const char *argv[])
{

	int lastPlaylist = -1;
	settingsGet(vp, "lasttrack.playlist", &lastPlaylist);
	if (lastPlaylist >= 0){
		vp->playlist.queued = playlistManagerGetUIDByIndex(vp->plm, lastPlaylist);
		vp->playlist.display = vp->playlist.queued;
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
			setDisplayPlaylist(vp, plc);
  			setQueuedPlaylist(vp, plc);
			if (plc->parent)
				plc->parent->pr->selectedItem = playlistManagerGetPlaylistIndex(vp->plm, plc);

#ifndef _DEBUG_
			playlistChangeEvent(vp, plc, 0);
#endif
		}

		TPLYPANEL *plypan = pageGetPtr(vp, PAGE_PLY_PANEL);
		plc = getDisplayPlaylist(vp);
		if (!plc) plc = getPrimaryPlaylist(vp);
		plyPanelBuild(vp, plypan->panel, plc);

		//should not be here
		playlistMenuSetTipTrack(vp, &vp->gui.tray, NULL, getQueuedPlaylistUID(vp), plc->pr->playingItem);
	}


#if RELEASEBUILD
	timerSet(vp, TIMER_PATHREGWRITE, 2000);
#endif
	timerSet(vp, TIMER_ARTCLEANUP, ARTWORKFLUSH_PERIOD+5000);
	timerSet(vp, TIMER_STATEHELPER, 2000);
	timerSet(vp, TIMER_IMAGECACHEFLUSH, 1*60*1000);
	timerSet(vp, TIMER_REG_TRACK_UPDATE, 2000);
	timerSet(vp, TIMER_TASKBARTITLE_UPDATE, 500);
	

#if (!ENABLE_SINGLEINSTANCE)
	vp->hWndOld = (int64_t)regGetQword(L"process_hwnd");
	vp->processIdOld = regGetDword(L"process_id");
#endif
	regSetDword(L"process_id", processGetId());
	regSetQword(L"process_hwnd", (QWORD)(intptr_t)vp->gui.hMsgWin);

	return 1;
}

int playerShutdown (TVLCPLAYER *vp)
{
	SHUTDOWN = 1;
		
#if (!ENABLE_SINGLEINSTANCE)
	//check if the instance previous to this is still running, if so revert to its details, otherwise clear
	HANDLE hProcess = processOpen(vp->processIdOld);
	if (!hProcess && (processGetId() != regGetDword(L"process_id"))){
		vp->processIdOld = regGetDword(L"process_id");
		hProcess = processOpen(vp->processIdOld);
		if (hProcess)
			vp->hWndOld = (intptr_t)getWindowHandle();
	}

	if (hProcess){
		processClose(hProcess);
		regSetDword(L"process_id", vp->processIdOld);
		regSetQword(L"process_hwnd", vp->hWndOld);
	}else{
		regSetDword(L"process_id", 0);
		regSetQword(L"process_hwnd", 0);
	}
#else
	regSetDword(L"process_id", 0);
	regSetQword(L"process_hwnd", 0);
#endif

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

	return 1;
}

int main (const int argc, const char *argv[])
{
	TVLCPLAYER *vp = playerNew();
	if (vp){
		g_vp = vp;
		
		if (playerInit() < 1)
			return EXIT_SUCCESS;

		if (playerInstanceCheck(vp, argc, argv) < 1)
			return EXIT_SUCCESS;

		playerConfigLoad(vp);		// will revert to default upon failure
		playerDisplayStart(vp);		// is allowed to fail, can retry later

		if (playerSetup(vp, PAGE_HOME) < 1)
			return EXIT_SUCCESS;

		if (playerStartup(vp, argc, argv)){
			playerRun(vp);
			playerShutdown(vp);
			playerClose(vp);
		}
		
		playerShowDeviceExitScreen(vp->ml);
		playerDelete(vp);
		
		printf("\n:: Exited\n");
	}
	
	return EXIT_SUCCESS;
}

