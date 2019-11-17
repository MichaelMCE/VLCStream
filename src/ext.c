
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




#define 	FUNCCMDCBS			\
	{CMD_SHUTDOWN,				cmd_shutdown},			\
	{CMD_MEDIA_PLAY,			cmd_play},				\
	{CMD_MEDIA_PAUSE,			cmd_pause},				\
	{CMD_MEDIA_PLAYPAUSE,		cmd_playpause},			\
	{CMD_MEDIA_STOP,			cmd_stop},				\
	{CMD_MEDIA_NEXT_TRACK,		cmd_nextTrack},			\
	{CMD_MEDIA_PREV_TRACK,		cmd_preTrack},			\
	{CMD_MEDIA_RANDOM,			cmd_randomTrack},		\
	{CMD_MEDIA_VOL_UP,			cmd_mediaVolUp},		\
	{CMD_MEDIA_VOL_DOWN,		cmd_mediaVolDown},		\
	{CMD_WIN_VOL_UP,			cmd_winVolUp},			\
	{CMD_WIN_VOL_DOWN,			cmd_winVolDown},		\
	{CMD_IDLE,					cmd_idle},				\
	{CMD_WAKE,					cmd_wake},				\
	{CMD_CLOCK,					cmd_clock},				\
	{CMD_SNAPSHOT,				cmd_ss},				\
	{CMD_RESYNC,				cmd_resync},			\
	{CMD_PLAYLIST_GOTO,			cmd_playlistGoTo},		\
	{CMD_PLAYLIST_NAME,			cmd_playlistRename},	\
	{CMD_MEDIA_VOL,				cmd_mediaVol},			\
	{CMD_MEDIA_TIME,			cmd_time},				\
	{CMD_MEDIA_POSITION,		cmd_position},			\
	{CMD_SBDK_PRESS,			cmd_sbdkPress},			\
	{CMD_EQ_BAND_SET,			cmd_eqBandSet},			\
	{CMD_EQ_PROFILE_SET,		cmd_eqProfileSet},		\
	{CMD_MEDIA_DVDNAV,			cmd_dvdNav},			\
	{CMD_MEDIA_CHAPTER,			cmd_chapter},			\
	{CMD_MEDIA_TITLE,			cmd_title},				\
	{CMD_INPUT_COPY,			cmd_inputCopy},			\
	{CMD_INPUT_PASTE,			cmd_inputPaste},		\
	{CMD_BOT_MORBID,			cmd_botMorbid},			\
	{CMD_BOT_BOFH,				cmd_botBOFH},			\
	{CMD_BOT_DUBYA,				cmd_botDubya},			\
	{CMD_BOT_FACTS,				cmd_botFacts},			\
	{CMD_FLUSH,					cmd_flush},				\
	{CMD_PLAYLIST_MVQ_UP,		cmd_ply_mvq_up},		\
	{CMD_PLAYLIST_MVQ_LEFT,		cmd_ply_mvq_left},		\
	{CMD_PLAYLIST_MVQ_RIGHT,	cmd_ply_mvq_right},		\
	{CMD_TASKBAR_TITLE_UPDATE,	cmd_taskbarupdate}
    



typedef struct{
	int op;
	void (*funcCb) (TVLCPLAYER *vp, const char *var1, const char *var2);
}TEXTCMD;



static inline void cmd_shutdown (TVLCPLAYER *vp, const char *var, const char *var2)
{
	timerSet(vp, TIMER_SHUTDOWN, 0);
}

static inline void cmd_play (TVLCPLAYER *vp, const char *var, const char *var2)
{	
	wakeup(vp);

	if (var && *var){
		int slen = strlen(var);
		
		if (slen < 4){
			int trk = decToInt(var);
			
			PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
			if (!plc || playlistGetTotal(plc) < trk) return;
			if (trk < 1)
				trk = 0;
			else
				trk--;
			playlistTimerStartTrackPLC(vp, plc, trk);	
			
		}else if (slen >= 4){		// uid is minimum 4 chars
			int uid = 0, trk = -1;
			hexIntToInt2(var, &uid, &trk);
			//printf("CMD_MEDIA_PLAY %X %i\n", uid, trk);
			
			PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
			if (!plc || playlistGetTotal(plc) < trk) return;
			if (trk < 1)		// TODO: handle track -1 and 0 (-1:recursive search and play, 0:select first playable track in playlist)
				trk = 0;
			else
				trk--;
			
			// TODO: if item isn't a track then ignore cmd. eg; if 232d:4 == playlist then ignore, otherwise attempt playback
			playlistTimerStartTrackUID(vp, uid, trk);
		}
	}else{
		timerSet(vp, TIMER_PLAY, 0);
	}
}

static inline void cmd_pause (TVLCPLAYER *vp, const char *var, const char *var2)
{
	wakeup(vp);
	timerSet(vp, TIMER_PAUSE, 0);
}

static inline void cmd_playpause (TVLCPLAYER *vp, const char *var, const char *var2)
{
	wakeup(vp);
	//if (!getPlayState(vp) || getPlayState(vp) == 8)
	if (!getPlayState(vp))
		cmd_play(vp, var, var2);
	else
		timerSet(vp, TIMER_PLAYPAUSE, 0);
}

static inline void cmd_stop (TVLCPLAYER *vp, const char *var, const char *var2)
{
	wakeup(vp);
	timerSet(vp, TIMER_STOP, 0);
}

static inline void cmd_nextTrack (TVLCPLAYER *vp, const char *var, const char *var2)
{
	wakeup(vp);
	timerSet(vp, TIMER_NEXTTRACK, 0);
}

static inline void cmd_preTrack (TVLCPLAYER *vp, const char *var, const char *var2)
{
	wakeup(vp);
	timerSet(vp, TIMER_PREVTRACK, 0);
}

static inline void cmd_time (TVLCPLAYER *vp, const char *var, const char *var2)
{
	if (!var || !*var) return;
		
	TVLCCONFIG *vlc = getConfig(vp);

	const int state = getPlayState(vp);
	if (state && state != 8){		// is playing but not at the end
		double tlen = vlc->length;
		double tpos = (double)stringToTime(var, strlen(var));
		double pos = 1.0/(tlen/tpos);
		clipFloat(pos);
		vlc->position = pos;
		vlc_setPosition(vlc, vlc->position);
	}
}

static inline void cmd_position (TVLCPLAYER *vp, const char *var, const char *var2)
{
	if (!var || !*var) return;
		
	TVLCCONFIG *vlc = getConfig(vp);

	const int state = getPlayState(vp);
	if (state && state != 8){		// is playing but not at the end
		double pos = 0.0;
		sscanf(var, "%lf", &pos);
		pos /= 100.0;
		clipFloat(pos);
		vlc->position = pos;
		vlc_setPosition(vlc, vlc->position);
	}
}

static inline void cmd_randomTrack (TVLCPLAYER *vp, const char *var, const char *var2)
{
	wakeup(vp);
	
	if (var){
		if (!stricmp(var, "on"))
			setPlaybackRandom(vp, 1, 1);
		else if (!stricmp(var, "off"))
			setPlaybackRandom(vp, 0, 1);
		else
			return;
	}else{
		setPlaybackRandom(vp, -1, 1);
	}
}

static inline void cmd_mediaVolUp (TVLCPLAYER *vp, const char *var, const char *var2)
{
	wakeup(vp);
	timerSet(vp, TIMER_VOL_APP_UP, 0);
}

static inline void cmd_mediaVolDown (TVLCPLAYER *vp, const char *var, const char *var2)
{
	wakeup(vp);
	timerSet(vp, TIMER_VOL_APP_DN, 0);
}

static inline void cmd_winVolUp (TVLCPLAYER *vp, const char *var, const char *var2)
{
	timerSet(vp, TIMER_VOL_MASTER_UP, 0);
}

static inline void cmd_winVolDown (TVLCPLAYER *vp, const char *var, const char *var2)
{
	timerSet(vp, TIMER_VOL_MASTER_DN, 0);
}

static inline void cmd_idle (TVLCPLAYER *vp, const char *var, const char *var2)
{
	page2Set(vp->pages, PAGE_NONE, 0);
	  	
	if (getPlayState(vp)) timerSet(vp, TIMER_STOP, 0);
	timerSet(vp, TIMER_SETIDLEA, 300);
	renderSignalUpdate(vp);
}

static inline void cmd_wake (TVLCPLAYER *vp, const char *var, const char *var2)
{
	wakeup(vp);
}

static inline void cmd_clock (TVLCPLAYER *vp, const char *var, const char *var2)
{
	wakeup(vp);
	page2Set(vp->pages, PAGE_CLOCK, 0);
}

static inline void cmd_ss (TVLCPLAYER *vp, const char *var, const char *var2)
{
	vp->gui.snapshot.save = 2;
	renderSignalUpdate(vp);
}

static inline void cmd_resync (TVLCPLAYER *vp, const char *var, const char *var2)
{
	lDISPLAY did = sbuiGetLibmylcdDID(vp->ml->hw);
	if (did){
		wakeup(vp);
		page2Set(vp->pages, PAGE_HOME, 1);
		sbuiResync(vp, did);
	}
}

static inline void playlistUIJumpToUID (TVLCPLAYER *vp, int uid, int trk)
{

	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	if (!plc/* || playlistGetTotal(plc) < trk*/) return;
				
	setDisplayPlaylistByUID(vp, uid);
	uid = getDisplayPlaylistUID(vp);

	if (page2RenderGetState(vp->pages, PAGE_PLY_SHELF)){
		if (playlistGetTotal(plc) < trk) return;
		if (trk < 1) trk = 0; else trk--;	
		
		setDisplayPlaylistByUID(vp, uid);
		//setSelectedItem(vp, trk);
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		spl->from = spl->to = trk;
		timerSet(vp, TIMER_PLYALB_REFRESH, 0);
	}
	if (page2RenderGetState(vp->pages, PAGE_PLY_PANEL)){
		plyPanSetCurrentUID(vp, uid);
		timerSet(vp, TIMER_PLYPAN_REBUILD, 0);
	}
	if (page2RenderGetState(vp->pages, PAGE_PLY_PANE)){
		setDisplayPlaylistByUID(vp, uid);
		setSelectedItem(vp, 0);
#if 1
		plypaneSetPlaylist(vp, uid);
#else
		TPLYPANE *plypane = pageGetPtr(vp, PAGE_PLY_PANE);
		plypaneStackEmpty(plypane);
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
		if (plc)
			buildPlaylistRoute(plypane->uidStack, plc);
		plypaneSetPaneUID(plypane, uid);
		
		timerSet(vp, TIMER_PLYPANE_REFRESH, 0);
#endif
	}
	//if (page2RenderGetState(vp->pages, PAGE_PLY_TV))
	//	timerSet(vp, TIMER_PLYTV_REFRESH, 0);
	if (page2RenderGetState(vp->pages, PAGE_PLY_FLAT)){
		int idx = playlistManagerGetIndexByUID(vp->plm, uid);
		setDisplayPlaylistByUID(vp, uid);
		setSelectedItem(vp, idx);
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_FLAT);
		swipeReset(&spl->drag);
		invalidateShelf(vp, spl, idx);
	}
}

static inline void cmd_playlistRename (TVLCPLAYER *vp, const char *var1, const char *var2)
{
	if (!var1 || strlen(var1) < 4) return;	// uid is minimum 4 chars
	if (!var2 || !*var2) return;			// there must be a new title to set
				
	int uid = 0, trk = -1;
	hexIntToInt2(var1, &uid, &trk);
	if (!uid) return;
	
	//printf("cmd_playlistRename %X %i '%s'\n", uid, trk, var2);
	
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	if (trk < 1){
		if (playlistSetName(plc, (char*)var2))
			dbprintf(vp, "Playlist %X renamed to: '%s'", uid, var2);
	}else{
		if (playlistRenameItem(vp->tagc, plc, trk-1, (char*)var2))
			dbprintf(vp, "Track #%i renamed to: %s", trk, var2);
	}
}

static inline void cmd_playlistGoTo (TVLCPLAYER *vp, const char *var, const char *var2)
{
	wakeup(vp);
	
	if (!var || strlen(var) < 4)	// uid is minimum 4 chars
		return;
				
	int uid = 0, trk = -1;
	
	if (!stricmp(var, "next")){
		uid = getDisplayPlaylistUID(vp) + 1;
	}else if (!stricmp(var, "prev") || !stricmp(var, "back")){
		uid = getDisplayPlaylistUID(vp) - 1;
	}else if (!stricmp(var, "root") || !stricmp(var, "first")){
		uid = getRootPlaylistUID(vp);
	}else if (!stricmp(var, "queued") || !stricmp(var, "playing")){
		uid = getQueuedPlaylistUID(vp);
	}else if (!stricmp(var, "right")){		// walk the playlist tree horizontally from within (a) branch
		uid = getDisplayPlaylistUID(vp);
		
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
		int pos = playlistGetPlaylistIndex(plc->parent, plc);
		pos = playlistGetNextItem(plc->parent, PLAYLIST_OBJTYPE_PLC, pos);
		uid = playlistGetPlaylistUID(plc->parent, pos);
		
	}else{
		hexIntToInt2(var, &uid, &trk);
	}
	
	if (uid)
		playlistUIJumpToUID(vp, uid, trk);

}

static inline void cmd_mediaVol (TVLCPLAYER *vp, const char *var, const char *var2)
{
	int volume = decToInt(var);
	if (volume > 100) volume = 100;
	else if (volume < 0) volume = 0;

	page2Set(vp->pages, PAGE_OVERLAY, 0);
	setVolumeDisplay(vp, setVolume(vp, volume, VOLUME_APP));
}

static inline void cmd_sbdkPress (TVLCPLAYER *vp, const char *var, const char *var2)
{
	if (isSBUIEnabled(vp)){
		const int dk = decToInt(var);
		if (dk >= SBUI_DK_1 && dk <= SBUI_DK_10)
			sbuiSimulateDk(dk, vp);
	}
}

static inline void cmd_eqProfileSet (TVLCPLAYER *vp, const char *var, const char *var2)
{
	const int ptotal = eqProfileGetTotal();
	for (int i = 0; i < ptotal; i++){
		const char *name = eqGetProfileName(i);
		if (!stricmp(name, var)){
			eqApplyPreset(pageGetPtr(vp,PAGE_EQ), i);
			return;
		}
	}
}

static inline void cmd_eqBandSet (TVLCPLAYER *vp, const char *var, const char *var2)
{
	int band;
	double amp;
	
	if (decToIntDouble(var, &band, &amp)){
		if (band >= 0 && band <= eqBandGetTotal())
			eqBandSet(pageGetPtr(vp, PAGE_EQ), band, amp);
	}
}

static inline void cmd_dvdNav (TVLCPLAYER *vp, const char *var, const char *var2)
{
	if (!var || !*var) return;
	
	TVLCCONFIG *vlc = getConfig(vp);
	
	if (!stricmp(var, "up"))
		vlc_navigate(vlc, libvlc_navigate_up);
	else if (!stricmp(var, "down") || !stricmp(var, "dn"))
		vlc_navigate(vlc, libvlc_navigate_down);
	else if (!stricmp(var, "left"))
		vlc_navigate(vlc, libvlc_navigate_left);
	else if (!stricmp(var, "right"))
		vlc_navigate(vlc, libvlc_navigate_right);
	else if (!stricmp(var, "go") || !stricmp(var, "ok") || !stricmp(var, "view") || !stricmp(var, "start") || !stricmp(var, "enter") || !stricmp(var, "play"))
		vlc_navigate(vlc, libvlc_navigate_activate);
}

static inline void cmd_title (TVLCPLAYER *vp, const char *var1, const char *var2)
{
	if (!var1 || !*var1) return;
	
	TVLCCONFIG *vlc = getConfig(vp);
	
	if (!stricmp(var1, "next")){
		vlc_nextTitle(vlc);
		
	}else if (!stricmp(var1, "previous") || !stricmp(var1, "prev") || !stricmp(var1, "back")){
		vlc_prevTitle(vlc);
		
	}else{
		TCHAPTER *chapt = pageGetPtr(vp, PAGE_CHAPTERS);
		int title = decToInt(var1);
		if (title > 0 && title <= 100 && title <= chapt->ttitles){
			//dbwprintf(vp, L"Setting title %i of %i\n", title, chapt->ttitles);
			vlc_setTitle(vp->vlc, title-1);
		}
	}
}

static inline void cmd_chapter (TVLCPLAYER *vp, const char *var1, const char *var2)
{
	if (!var1 || !*var1) return;
	
	TVLCCONFIG *vlc = getConfig(vp);
	
	if (!stricmp(var1, "next")){
		vlc_nextChapter(vlc);
		
	}else if (!stricmp(var1, "previous") || !stricmp(var1, "prev") || !stricmp(var1, "back")){
		vlc_previousChapter(vlc);
		
	}else{
		int chapter = decToInt(var1);
		if (chapter > 0 && chapter <= MAX_CHAPTERS){
			TCHAPTER *chapt = pageGetPtr(vp, PAGE_CHAPTERS);
			//dbwprintf(vp, L"Setting chapter %i of %i\n", chapter, chapt->tchapters);
			chapt->schapter = chapter;
			vlc_setChapter(vlc, chapter-1);
		}
	}
}

static inline void cmd_botMorbid (TVLCPLAYER *vp, const char *var1, const char *var2)
{
#if ENABLE_CMDFUNSTUFF
	wakeup(vp);
	botQuoteRandom(vp, vp->bot.sheets, BOT_MORBID);
	renderSignalUpdate(vp);
#endif
}

static inline void cmd_botBOFH (TVLCPLAYER *vp, const char *var1, const char *var2)
{
#if ENABLE_CMDFUNSTUFF
	wakeup(vp);
	botQuoteRandom(vp, vp->bot.sheets, BOT_BOFH);
	renderSignalUpdate(vp);
#endif
}

static inline void cmd_botDubya (TVLCPLAYER *vp, const char *var1, const char *var2)
{
#if ENABLE_CMDFUNSTUFF
	wakeup(vp);
	botQuoteRandom(vp, vp->bot.sheets, BOT_DUBYA);
	renderSignalUpdate(vp);
#endif
}

static inline void cmd_botFacts (TVLCPLAYER *vp, const char *var1, const char *var2)
{
#if ENABLE_CMDFUNSTUFF
	wakeup(vp);
	botQuoteRandom(vp, vp->bot.sheets, BOT_FACTS);
	renderSignalUpdate(vp);
#endif
}

static inline void cmd_taskbarupdate (TVLCPLAYER *vp, const char *var1, const char *var2)
{
	//printf("cmd_taskbarupdate\n");
#if 0
	wakeup(vp);
	timerSet(vp, TIMER_TASKBARTITLE_UPDATE, 0);
#else
	timer_drawTaskbarTrackTitle(vp);
#endif
}

static inline void cmd_flush (TVLCPLAYER *vp, const char *var1, const char *var2)
{
	timerSet(vp, TIMER_FLUSH, 0);
	timerSet(vp, TIMER_TASKBARTITLE_UPDATE, 0);
}
		
static inline void cmd_ply_mvq_up (TVLCPLAYER *vp, const char *var1, const char *var2)
{
	int orig = getQueuedPlaylistUID(vp);
	int uid = setQueuedPlaylistByUID(vp, getQueuedPlaylistParent(vp));
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	char *title = playlistGetNameDup(plc);
	printf("up: %X -> %X '%s'\n", orig, uid, title);
	if (title) my_free(title);

	playlistChangeEvent(vp, plc, getPlaylistFirstTrack(vp, uid));
}

static inline void cmd_ply_mvq_left (TVLCPLAYER *vp, const char *var1, const char *var2)
{
	int orig = getQueuedPlaylistUID(vp);
	int uid = setQueuedPlaylistByUID(vp, getQueuedPlaylistLeft(vp));
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	char *title = playlistGetNameDup(plc);
	printf("left: %X -> %X '%s'\n", orig, uid, title);
	if (title) my_free(title);

	playlistChangeEvent(vp, plc, getPlaylistFirstTrack(vp, uid));
}

static inline void cmd_ply_mvq_right (TVLCPLAYER *vp, const char *var1, const char *var2)
{
	int orig = getQueuedPlaylistUID(vp);
	int uid = setQueuedPlaylistByUID(vp, getQueuedPlaylistRight(vp));
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	char *title = playlistGetNameDup(plc);
	printf("right: %X -> %X '%s'\n", orig, uid, title);
	if (title) my_free(title);

	playlistChangeEvent(vp, plc, getPlaylistFirstTrack(vp, uid));
}

static inline void cmd_inputCopy (TVLCPLAYER *vp, const char *var1, const char *var2)
{
}

static inline void cmd_inputPaste (TVLCPLAYER *vp, const char *var1, const char *var2)
{
}

static const TEXTCMD funcs[] = {
	FUNCCMDCBS
};

int extCommandFunc (TVLCPLAYER *vp, const int op, const int varInt, const char *varStr1, const char *varStr2)
{

	const int tFuncs = sizeof(funcs)/sizeof(TEXTCMD);
	for (int i = 0; i < tFuncs; i++){
		if (funcs[i].op == op){
			funcs[i].funcCb(vp, varStr1, varStr2);
			return 1;
		}
	}
	return 0;
}

void extReceiveCdsPath (TVLCPLAYER *vp, const int to, const unsigned int hash, const char *pathIn8, const int pathInLen)
{
	if (!hash) return;
	
	if (hash == generateHash(pathIn8, pathInLen) && pathInLen == strlen(pathIn8)+1){
		//if (1 || !vp->gui.awake){
			if (pageGet(vp) == PAGE_CLOCK){
				void *ptr = page2PageStructGet(vp->pages, PAGE_CLOCK);
				page2SetPrevious(ptr);
			}

			setAwake(vp);
			vp->gui.frameCt = 0;
			sbuiWoken(vp);
			renderSignalUpdate(vp);
		//}
		dbprintf(vp, "Importing '%s'", pathIn8);

		if (to == WM_CDS_ADDTRACK_DSP)
			playlistImportPath(vp, getDisplayPlaylist(vp), (char*)pathIn8);
		else if (to == WM_CDS_ADDTRACK_PLY)
			playlistImportPath(vp, getQueuedPlaylist(vp), (char*)pathIn8);
		else
			playlistImportPath(vp, getPrimaryPlaylist(vp), (char*)pathIn8);
	}
}

void extReceiveCdsCmd (TVLCPLAYER *vp, const int op, const char *var1)
{

	if (!var1){
		extCommandFunc(vp, op, 0, NULL, NULL);
		return;
	}
	
	char buffer[MAX_PATH_UTF8+1];
	char *var2 = strstr(var1, "<|>");
	if (var2){
		*var2 = 0;
		var2 += 3;
		
		if (!*var2){
			var2 = NULL;
		}else{
			char *pbuffer = buffer;
			*pbuffer = 0;

			while (*var2){
				char *varSpace = strstr(var2, "<|>");
				if (!varSpace){
					strcpy(pbuffer, var2);
					break;
				}
				
				int len = (varSpace-var2);
				strncpy(pbuffer, var2, len);
				pbuffer += len;
				strncpy(pbuffer++, " ", 1);
				*pbuffer = 0;
				var2 = varSpace + 3;
			};
			var2 = buffer;
		}
	}	
	
	//printf("extReceiveCdsCmd: '%s' '%s'\n", var1, var2);
	extCommandFunc(vp, op, 0, var1, var2);
}

