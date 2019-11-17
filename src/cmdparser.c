// playlist media control overlay
// (next trk, play, art size, etc..)
//
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
#if (0&&ENABLE_ANTPLUS)
#include "antplus/anthrm.h"
#endif


#define SYSTEMCMDS_Lock				1
#define SYSTEMCMDS_Reboot			2
#define SYSTEMCMDS_Logoff			3
#define SYSTEMCMDS_Shutdown			4
#define SYSTEMCMDS_ShutdownAbort	5
#define SYSTEMCMDS_ExplorerRestart	6


// keep this in sync with tags.h:enum _tags_meta and meta.c:tagStrLookup[]
static const wchar_t *wtagStrTable[] = {
	L"title",
    L"artist",
    L"genre",
    L"copyright",
    L"album",
    L"track",
    L"description",
    L"rating",
    L"date",
    L"setting",
    L"url",
    L"language",
    L"nowplaying",
    L"publisher",
    L"encodedby",
    L"artpath",
    L"trackid",

#if (LIBVLC_VERSION_MAJOR >= 2 && LIBVLC_VERSION_MINOR >= 2)
    L"tracktotal",
    L"director",
    L"season",
    L"episode",
    L"showname",
    L"actors",
#endif
    
    L"length",
    L"filename",
    L"playlist",
    L"path",
    L""
};


static inline int tagLookupW (const wchar_t *str)
{
	for (int mtag = 0; mtag < MTAG_TOTAL; mtag++){
		if (!wcsicmp(str, wtagStrTable[mtag]))
			return mtag;
	}
	return -1;
}

static inline const wchar_t *getTagW (const int mtag)
{
	if (mtag >= 0 && mtag < MTAG_TOTAL)
		return wtagStrTable[mtag];
	else
		return L" ";
}

static inline wchar_t *strGetString (wchar_t *str, const wchar_t *sep)
{
	return wcstok(str, sep);
}

static inline wchar_t *strGetStringNext (const wchar_t *sep)
{
	return strGetString(NULL, sep);
}

static inline int strGetInt32 (wchar_t *str)
{
	wchar_t *digits = strGetString(str, L" :\0");
	if (digits){
		if (*digits == CMDPARSER_NUMIDENT || *digits == CMDPARSER_CMDIDENT || *digits == CMDPARSER_WILDIDENT)
			digits++;

		if (iswdigit(*digits)){
			wchar_t *end = L" \0\0";
			return (int)wcstol(digits, &end, 0);
		}
	}
	
	//wprintf(L"strGetInt32: not a number or invalid input #%s# #%s#\n", str, digits);
	return 0;
}

static inline int strGetInt32Next ()
{
	return strGetInt32(NULL);
}

static inline uint64_t strGetInt32Pair (wchar_t *str, int *_v1, int *_v2)
{
	int v1 = strGetInt32(str);
	int v2 = strGetInt32Next();
	if (_v1) *_v1 = v1;
	if (_v2) *_v2 = v2;
	return ((uint64_t)v1<<32)|(uint64_t)v2;
}

static inline uint64_t strGetInt32PairNext (int *_v1, int *_v2)
{
	return strGetInt32Pair(NULL, _v1, _v2);
}

static inline double strGetFloat (wchar_t *str)
{
	wchar_t *digits = strGetString(str, L" :\0");
	if (digits){
		if (*digits == CMDPARSER_NUMIDENT || *digits == CMDPARSER_CMDIDENT || *digits == CMDPARSER_WILDIDENT)
			digits++;

		if (iswdigit(*digits) || *digits == L'-'){
			wchar_t *end = L" \0\0";
			return (double)wcstod(digits, &end);
		}
	}
	
	__mingw_wprintf(L"strGetInt32: not a number or invalid input #%ls# #%ls#\n", str, digits);
	return 0.0;
}

static inline double strGetFloatNext ()
{
	return strGetFloat(NULL);
}

static inline int playlistSetTrackPaths (TPLAYLISTMANAGER *plm, const int uid, char *path)
{
	char *dir = removeLeadingSpaces(path);
	if (!dir) return -1;
		
	removeTrailingSpaces(dir);
	int len = strlen(dir);

	// perform a few sanity checks
	// verify that we're dealing with a legal logicial path
	if (len < strlen("c:\\") || !isalpha(path[0]) || path[1] != ':')
		return -1;
	if (path[2] != '\\' && path[2] != '/')
		return -1;

	// replace double ending slashes with a single
	if ((dir[len-1] == '\\' || dir[len-1] == '/') && (dir[len-2] == '\\' || dir[len-2] == '/')){
		dir[len-1] = 0;
		len--;
	}
	// need a trailing backslash so add if it doesn't exist
	if (dir[len-1] == '/'){
		dir[len-1] = '\\';
	}
	if (dir[len-1] != '\\'){
		dir[len] = '\\';		// we can do this because we know the buffer length is MAX_PATH_UTF8+2
		dir[len+1] = 0;
	}

	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(plm, uid);
	if (plc)
		return playlistSetPaths(plc, dir);
	else
		return -2;
}


void playlistsForceRefresh (TVLCPLAYER *vp, const int when)
{
	timerSet(vp, TIMER_PLYALB_REFRESH, when);
	timerSet(vp, TIMER_PLYTV_REFRESH, when);
	timerSet(vp, TIMER_PLYPAN_REBUILD, when);
	timerSet(vp, TIMER_PLYPLM_REFRESH, when);
}

static inline int searchPlaylist (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int mtag, const char *search, const int from)
{
	//if (mtag >= 0)
		//dbprintf(vp, "searching for '%s = %s' in '%s'", getTag(mtag), search, plc->name);
	//else
	//	dbprintf(vp, "searching for '%s' in '%s'", search, plc->name);

	int trk = -1;
	if (mtag == -1){
		if (from >= 0)
			trk = playlistSearch(plc, vp->tagc, search, from);
		else
			trk = playlistSearch(plc, vp->tagc, search, 0);
	}else{
		if (from >= 0)
			trk = playlistSearchTag(plc, vp->tagc, mtag, search, from);
		else
			trk = playlistSearchTag(plc, vp->tagc, mtag, search, 0);
	}

	return trk;
}

static inline int editBoxDoSearchPlaylist (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int mtag, const wchar_t *searchForW, const int searchFrom, const int play)
{
	int trk = -1;
	
	if (playlistGetTotal(plc)){
		char *searchFor8 = convertto8(searchForW);
		if (searchFor8){
			trk = searchPlaylist(vp, plc, -1, searchFor8, searchFrom);
			my_free(searchFor8);
		}

		if (trk >= 0){
			plc->pr->selectedItem = trk;

			char *searchFor8 = convertto8(searchForW);
			if (searchFor8){
				char *title = playlistGetTitleDup(plc, trk);
				if (title){
					dbprintf(vp, "found '%s' in  %s:#%i '%s'\n", searchFor8, plc->title, trk+1, title);
					my_free(title);
				}else{
					dbprintf(vp, "found '%s' in track %i of %s\n", searchFor8, trk+1, plc->title);
				}
				my_free(searchFor8);
					
				if (plc->parent)
					plc->parent->pr->selectedItem = playlistGetPlaylistIndex(plc->parent, plc);
				plc->pr->selectedItem = trk;
				//vp->displayPlaylist = playlistManagerGetPlaylistIndex(vp->plm, plc);
				setDisplayPlaylist(vp, plc);
				invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), plc->pr->selectedItem);
			}
			if (play)
				playlistTimerStartTrackPLC(vp, plc, trk);
		}else{
			if (playlistGetCount(plc, PLAYLIST_OBJTYPE_PLC)){
				const int ptotal = playlistGetTotal(plc);
				for (int i = searchFrom; i < ptotal; i++){
					TPLAYLISTITEM *item = playlistGetItem(plc, i);
					if (item && item->objType == PLAYLIST_OBJTYPE_PLC){
						trk = editBoxDoSearchPlaylist(vp, item->obj.plc, mtag, searchForW, 0, play);
						if (trk >= 0) return trk;
					}
				}
			}
		}
	}
	return trk;
}

static inline int editBoxDoSearch (TVLCPLAYER *vp, const int mtag, const wchar_t *searchFor, const int searchType)
{
	//printf("editBoxDoSearch\n");
	
	int searchFrom = searchType;
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	
	if (plc){
		if (searchType == SEARCH_CURRENT || searchType == SEARCH_CURRENT_PLAY){
			searchFrom = plc->pr->selectedItem;

		}else if (searchType == -2){
			TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
			searchFrom = spl->from;

		}else if (searchType == SEARCH_NEXT || searchType == SEARCH_NEXT_PLAY){
			searchFrom = plc->pr->selectedItem+1;

		}else if (searchType == SEARCH_RESET || searchType == SEARCH_RESET_PLAY){
			searchFrom = 0;

		}else if (searchType == SEARCH_ROOT || searchType == SEARCH_ROOT_PLAY){
			plc = getPrimaryPlaylist(vp);
			searchFrom = 0;
		}
		
		int play;
		switch (searchType){
		  case SEARCH_ROOT_PLAY:
		  case SEARCH_NEXT_PLAY:
		  case SEARCH_RESET_PLAY:
		  case SEARCH_CURRENT_PLAY:
		  	play = 1;
			break;
		  default:
		  	play = 0;
		  	break;
		}

		return editBoxDoSearchPlaylist(vp, plc, mtag, searchFor, searchFrom, play);
	}
	return 0;
}

static inline TEDITBOXCMD *editBoxGetCmd (TEDITBOX *input, wchar_t *cmdName)
{
	TEDITBOXCMD *cmd = input->registeredCmds;

	for (int i = 0; i < EDITBOXCMD_MAXCMDS && cmd->state; i++){
		if (!wcsncmp(cmd->name, cmdName, EDITBOXCMD_MAXCMDLEN))
			return cmd;
		else if (*cmd->alias && !wcsncmp(cmd->alias, cmdName, EDITBOXCMD_MAXCMDLEN))
			return cmd;
		cmd++;
	}
	return NULL;
}

static inline wchar_t *editBoxGetCmdDesc (TEDITBOX *input, wchar_t *cmdName)
{
	TEDITBOXCMD *cmd = input->registeredCmds;

	for (int i = 0; i < EDITBOXCMD_MAXCMDS && cmd->state; i++){
		if (!wcsncmp(cmd->name, cmdName, EDITBOXCMD_MAXCMDLEN))
			return cmd->desc;
		else if (*cmd->alias && !wcsncmp(cmd->alias, cmdName, EDITBOXCMD_MAXCMDLEN))
			return cmd->desc;
		cmd++;
	}
	return NULL;
}

static inline int editBoxCmdExecute (TEDITBOX *input, wchar_t *cmdName, int clen, wchar_t *var, int vlen)
{
	TEDITBOXCMD *cmd = editBoxGetCmd(input, cmdName);
	if (cmd){
		//wprintf(L"found cmd '%s'\n", cmd->name);
		cmd->pfunc(var, vlen, cmd->uptr, cmd->data1, 0/*do something with this*/);
		return 1;
	}
	return 0;
}

static inline int editboxParseCommands (TEDITBOX *input, TVLCPLAYER *vp, wchar_t *txt, int ilen)
{
	if (ilen < 1) return -1;


	/*
	#n == jump to track/list n
	#abc == a command
	*/

	/*
	plm->pr->selectedItem = item->playlistPosition;
	vp->displayPlaylist = plm->pr->selectedItem;
	*/

	if (iswdigit(*txt)){		// jump to #nnn
		//wchar_t *end = L"\0\0";
		int trk = strGetInt32(txt);
		const int page = pageGet(vp);
		
		if (page == PAGE_PLY_SHELF || page == PAGE_OVERLAY){
			PLAYLISTCACHE *plcD = getDisplayPlaylist(vp);
			if (trk > 0 && trk <= playlistGetTotal(plcD)){
				plcD->pr->selectedItem = --trk;
				invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), trk);

				if (pageGet(vp) == PAGE_OVERLAY)
					playlistTimerStartTrackPLC(vp, plcD, trk);
				else
					playlistMetaGetMeta(vp, plcD, trk-5, trk+5, NULL);
			}
		}else if (page == PAGE_PLY_FLAT){
			if (trk > 0 && trk <= playlistManagerGetTotal(vp->plm)){
				setDisplayPlaylistByUID(vp, playlistManagerGetUIDByIndex(vp->plm, --trk));
				invalidateShelf(vp, pageGetPtr(vp, PAGE_PLY_FLAT), trk);
			}
		}else if (page == PAGE_PLY_PANEL){
			if (trk > 0 && trk <= playlistManagerGetTotal(vp->plm)){
				setDisplayPlaylistByUID(vp, playlistManagerGetUIDByIndex(vp->plm, --trk));
				plyPanSetCurrentUID(vp, getDisplayPlaylistUID(vp));
				timerSet(vp, TIMER_PLYPAN_REBUILD, 0);
			}
		}
	}else{					// execute command
		wchar_t *cmd = strGetString(txt, L" ;");
		if (cmd && *cmd){
			int vlen = 0;
			const int clen = wcslen(cmd);
			wchar_t *var = strGetStringNext(L"\0");
			if (var && *var){
				var = removeLeadingSpacesW(var);
				vlen = wcslen(var);
			}
			//wprintf(L"#%s# #%s#\n",cmd, var);
			editBoxCmdExecute(input, cmd, clen, var, vlen);
		}
	}

	return -1;
}

int editboxProcessString (TEDITBOX *input, wchar_t *txt, int ilen, void *ptr)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;

	// check if its a command, if not then assume a search query

	if (ilen){
		txt = removeLeadingSpacesW(txt);
		txt = removeTrailingSpacesW(txt);
	}
	//wprintf(L"%i '%s'\n", ilen, txt);

	if (*txt == CMDPARSER_CMDIDENT && ilen > 0){
		editboxParseCommands(input, vp, ++txt, --ilen);
		return 1;
	}else if (*txt == CMDPARSER_COMMENT && ilen == 1){
		return 0;
	}else if (ilen > 3 && (txt[0] == CMDPARSER_SEARCHNEXT) && (txt[1] == CMDPARSER_SEARCHPLAY)){
		editBoxDoSearch(vp, -1, &txt[2], SEARCH_NEXT_PLAY);
		return 1;
	}else if (ilen > 3 && (txt[0] == CMDPARSER_SEARCHROOT) && (txt[1] == CMDPARSER_SEARCHPLAY)){
		editBoxDoSearch(vp, -1, &txt[2], SEARCH_ROOT_PLAY);
		return 1;
	}else if (ilen > 3 && (txt[0] == CMDPARSER_SEARCHRESET) && (txt[1] == CMDPARSER_SEARCHPLAY)){
		editBoxDoSearch(vp, -1, &txt[2], SEARCH_RESET_PLAY);
		return 1;
	}else if (ilen > 3 && *txt == CMDPARSER_SEARCHNEXT){
		editBoxDoSearch(vp, -1, ++txt, SEARCH_NEXT);
		return 1;
	}else if (ilen > 3 && *txt == CMDPARSER_SEARCHROOT){
		editBoxDoSearch(vp, -1, ++txt, SEARCH_ROOT);
		return 1;
	}else if (ilen > 3 && *txt == CMDPARSER_SEARCHPLAY){
		editBoxDoSearch(vp, -1, ++txt, SEARCH_CURRENT_PLAY);
		return 1;
	}else if (ilen > 3 && *txt == CMDPARSER_SEARCHRESET){
		editBoxDoSearch(vp, -1, ++txt, SEARCH_RESET);
		return 1;
	}else{
		editBoxDoSearch(vp, -1, txt, -2);
		return 1;
	}
}

static inline int cmd_help (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	TEDITBOX *input = (TEDITBOX*)&vp->input;
	
	if (!vlen) return 0;
	wchar_t *cmd = strGetString(var, L" \0");
	if (!cmd) return 0;
	
	TEDITBOXCMD *ebcmd = editBoxGetCmd(input, cmd);
	if (!ebcmd){
		dbwprintf(vp, L"Usage: %s", editBoxGetCmdDesc(input, L"help"));
		return 0;
	}
	
	wchar_t *info = editBoxGetCmdDesc(input, cmd);
	if (!info || (info && !wcslen(info))){
		dbwprintf(vp, L"No help for '%s'", cmd);
		return 0;
	}
	
	//wprintf(L"cmd_help: '%s' '%s'\n", ebcmd->name, info);
	
	dbwprintfEx(vp, MARQUEE_WRAP, L"%s: '%s'", ebcmd->name, info);
	return 1;
}

static inline int editBoxRegisterCmdFunc (TEDITBOX *input, wchar_t *_cmdName, void *pfunc, void *uptr, int data1, const wchar_t *info)
{
	TEDITBOXCMD *cmd = &input->registeredCmds[input->registeredCmdTotal++];
	if (input->registeredCmdTotal >= EDITBOXCMD_MAXCMDS){
		//printf("editBoxRegisterCmdFunc failed due to lack of cmd space (%i)\n", EDITBOXCMD_MAXCMDS);
		return 0;
	}

	wchar_t *cmdName = my_wcsdup(_cmdName);
	if (!cmdName) return 0;

	wchar_t *name = strGetString(cmdName, L" ,");
	if (!name){
		my_free(cmdName);
		return 0;
	}
	wcsncpy(cmd->name, name, EDITBOXCMD_MAXCMDLEN);
	cmd->name[EDITBOXCMD_MAXCMDLEN-1] = 0;

	wchar_t *alias = strGetStringNext(L" ,");
	if (alias && *alias){
		wcsncpy(cmd->alias, alias, EDITBOXCMD_MAXCMDLEN);
		cmd->alias[EDITBOXCMD_MAXCMDLEN-1] = 0;
	}else{
		cmd->alias[0] = 0;
	}

	cmd->pfunc = pfunc;
	cmd->uptr = uptr;
	cmd->data1 = data1;
	cmd->state = 1;
	if (info)
		wcsncpy(cmd->desc, info, EDITBOXCMD_MAXDESCLEN);
	my_free(cmdName);
	return 1;
}

static inline void cmd_sort (wchar_t *var, int vlen, void *uptr, int direction, int unused)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	PLAYLISTCACHE *plcD = getDisplayPlaylist(vp);

	var = strGetString(var, L" ");
	if (!var) return;
	vlen = wcslen(var);
	if (vlen < 4) return;

	//wprintf(L"editboxCmdSort: #%s# %i %i\n", var, vlen, direction);
	playlistSort(plcD, vp->tagc, tagLookupW(var), direction);
	playlistsForceRefresh(vp, 0);
}

static inline void cmd_mctrl (wchar_t *var, int vlen, void *uptr, int op, int unused)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	
	switch (op){
	  case VBUTTON_PRETRACK:
	  	timerSet(vp, TIMER_PREVTRACK, 0);
	  	break;

	  case VBUTTON_NEXTTRACK:
	  	timerSet(vp, TIMER_NEXTTRACK, 0);
	  	break;

	  case VBUTTON_STOP:
	  	timerSet(vp, TIMER_STOP, 0);
	  	break;
	  	
	  case VBUTTON_PAUSE:
	  	timerSet(vp, TIMER_PLAYPAUSE, 0);
		break;

	  case VBUTTON_PLAY:{
	  	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	  	if (!plc) break;
	  	
	  	int trk = strGetInt32(var);
	  	if (trk-- > 0){
			if (trk >= 0 && trk < playlistGetTotal(plc)){
				plc->pr->playingItem = trk;
				timerSet(vp, TIMER_STOPPLAY, 0);
			}
		}else{
			timerSet(vp, TIMER_PLAY, 0);
		}

	  	break;
	  }
	}
}

static inline void closeEditbox (TVLCPLAYER *vp)
{
	PostMessage(vp->gui.hMsgWin, WM_KEYDOWN, 27, 0);
	renderSignalUpdate(vp);
}

static inline void cmd_shutdown (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	if (mHookGetState()){
		captureMouse(vp, 0);
		mHookUninstall();
	}

	closeEditbox(vp);
	//pageSet(vp, PAGE_NONE);
  	//pageSetSec(vp, -1);
  	//printf("cmd_shutdown: TODO: add close all\n");
  	timerSet(vp, TIMER_SHUTDOWN, 150);
  	renderSignalUpdate(vp);
}

static inline int displayartW (const wchar_t *pathw)
{
	int success = 0;
		
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	if (pathw){
		const wchar_t *cl = L"cmd /c \"%ls\"";
		int len = __mingw_snwprintf(NULL, 0, cl, pathw);
		if (len > 1){
			wchar_t cmdline[++len];
			__mingw_snwprintf(cmdline, len, cl, pathw);
			success = CreateProcessW(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
		}
	}
	return success;
}

// TODO: update me using artId's
static inline int openArtwork (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int pos)
{
	const unsigned int hash = playlistGetHash(plc, pos);
	if (!hash) return 0;

	int ret = 0;
	char mrl[MAX_PATH_UTF8+1];
	char artpath[MAX_PATH_UTF8+1];

	tagRetrieveByHash(vp->tagc, hash, MTAG_ArtworkPath, mrl, MAX_PATH_UTF8);
	if (!*mrl){
		dbprintf(vp, "Art MRL not found");
		return 0;
	}
	
	if (decodeURIEx(mrl, strlen(mrl), artpath)){
		wchar_t *path = converttow(artpath);
		if (path){
			if (doesFileExistW(path)){
				if (displayartW(path)){
					if (mHookGetState()){
						captureMouse(vp, 0);
						mHookUninstall();
					}
					ret = 1;
				}else{
					dbprintf(vp, "Could not open '%s'", artpath);
				}
			}else{
				dbprintf(vp, "File not found '%s'", artpath);
			}
			my_free(path);
		}
	}
	return ret;
}

static inline void shelfGoToFirst (TSPL *spl, PLAYLISTCACHE *plc)
{
	bringAlbumToFocus(spl, 0);
}

static inline void shelfGoToLast (TSPL *spl, PLAYLISTCACHE *plc)
{
	bringAlbumToFocus(spl, playlistGetTotal(plc)-1);
}

static inline void cmd_firstlast (wchar_t *var, int vlen, void *uptr, int action, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	if (!plc) return;
	
	if (!action)
		shelfGoToFirst(pageGetPtr(vp, PAGE_PLY_SHELF), plc);
	else if (action == 1)
		shelfGoToLast(pageGetPtr(vp, PAGE_PLY_SHELF), plc);
}

static inline void cmd_eq (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	
	if (!var || !*var){
		page2Set(vp->pages, PAGE_EQ, 1);
	}else{
		const int preset = strGetInt32(var);
		TEQ *eq = pageGetPtr(vp, PAGE_EQ);
		if (eqApplyPreset(eq, preset))
			renderSignalUpdate(vp);
	}
}

static inline void cmd_clock (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	page2Set(vp->pages, PAGE_CLOCK, 1);
}

static inline void cmd_track (wchar_t *var, int vlen, void *uptr, int mode, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	if (mode == 1){			// playing
		PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
		plc->pr->selectedItem = plc->pr->playingItem;
		//vp->displayPlaylist = vp->queuedPlaylist;
		setDisplayPlaylist(vp, plc);
		vp->plm->pr->selectedItem = playlistManagerGetPlaylistIndex(vp->plm, plc);
		
		if (!page2RenderGetState(vp->pages, PAGE_PLY_SHELF)) page2Set(vp->pages, PAGE_PLY_SHELF, 1);
		invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), plc->pr->selectedItem);
		
	}else if (mode == 2){	// selected
		PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
		if (plc->pr->selectedItem >= 0){
			//if (pageGet(vp) != PAGE_PLY_SHELF) pageSet(vp, PAGE_PLY_SHELF);
			if (!page2RenderGetState(vp->pages, PAGE_PLY_SHELF)) page2Set(vp->pages, PAGE_PLY_SHELF, 1);
			invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), plc->pr->selectedItem);
		}
	}
}

static inline void cmd_config (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	//pageSetSec(vp, -1);
	if (!page2RenderGetState(vp->pages, PAGE_CFG))
		page2Set(vp->pages, PAGE_CFG, 1);
}

static inline void cmd_escodec (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	//pageSetSec(vp, -1);
	if (!page2RenderGetState(vp->pages, PAGE_ES))
		page2Set(vp->pages, PAGE_ES, 1);
}

static inline void cmd_back (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	//page2SetPrevious(vp);
	//pageSetSec(vp, -1);
	void *ptr = page2PageStructGet(vp->pages, pageRenderGetTop(vp->pages));
	if (ptr) page2SetPrevious(ptr);
}

static inline void cmd_dvdnav (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	
	wchar_t *state = strGetString(var, L"\0");
	if (state && *state){
		if (!wcscmp(state, L"up")){
			vlc_navigate(vp->vlc, libvlc_navigate_up);
		}else if (!wcscmp(state, L"down") || !wcscmp(state, L"dn")){
			vlc_navigate(vp->vlc, libvlc_navigate_down);
		}else if (!wcscmp(state, L"left")){
			vlc_navigate(vp->vlc, libvlc_navigate_left);
		}else if (!wcscmp(state, L"right")){
			vlc_navigate(vp->vlc, libvlc_navigate_right);
		}else if (!wcscmp(state, L"go") || !wcscmp(state, L"ok") || !wcscmp(state, L"view") || !wcscmp(state, L"start") || !wcscmp(state, L"enter") || !wcscmp(state, L"play")){
			vlc_navigate(vp->vlc, libvlc_navigate_activate);
		}
	}
}

static inline void cmd_dvb (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	
	wchar_t *state = strGetString(var, L"\0");
	if (state && *state){
		if (!wcscmp(state, L"genepg")){
			timerSet(vp, TIMER_EPG_GENDVBPLAYLIST, 10);
			return;
		}
	}else{
		//pageSetSec(vp, -1);
		if (!page2RenderGetState(vp->pages, PAGE_EPG))
			page2Set(vp->pages, PAGE_EPG, 1);
	}
}

static inline int savePlaylist (TVLCPLAYER *vp, PLAYLISTCACHE *plc, wchar_t *name)
{
	wchar_t wbuffer[MAX_PATH+1];
	wchar_t drive[_MAX_DRIVE+1];
	wchar_t dir[_MAX_DIR+1];
	wchar_t filename[MAX_PATH+1];

	_wsplitpath(name, drive, dir, filename, NULL);
	__mingw_snwprintf(wbuffer, MAX_PATH, L"%ls%ls%ls%ls", drive, dir, filename, VLCSPLAYLISTEXTW);
	dbwprintf(vp, L"Generating playlist '%s'", wbuffer);

	TM3U *m3u = m3uNew();
	if (m3u){
		if (m3uOpen(m3u, wbuffer, M3U_OPENWRITE)){
			
			int cret = m3uWritePlaylist(m3u, plc, vp->tagc, vp->am, plc != getPrimaryPlaylist(vp));
			m3uClose(m3u);
			dbwprintf(vp, L"%i tracks written to '%s'", cret, wbuffer);
		}
		m3uFree(m3u);
		return 1;
	}
	return 0;
}

static inline int savePlaylistUID (TVLCPLAYER *vp, const int uid, wchar_t *name)
{
	return savePlaylist(vp, playlistManagerGetPlaylistByUID(vp->plm, uid), name);
}

static inline void cmd_meta (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	if (pageGet(vp) != PAGE_META){
		PLAYLISTCACHE *plc = NULL;

		if (pageGet(vp) == PAGE_OVERLAY)
			plc = getQueuedPlaylist(vp);

		if (!plc)
			plc = getDisplayPlaylist(vp);

		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		TMETA *meta = pageGetPtr(vp, PAGE_META);
		TMETADESC *desc = &meta->desc;
		
		if (playlistGetItemType(plc, spl->from) == PLAYLIST_OBJTYPE_TRACK)
			desc->trackPosition = spl->from;
		else if (plc->pr->playingItem >= 0)
			desc->trackPosition = plc->pr->playingItem;
		else
			desc->trackPosition = spl->from;

		desc->uid = plc->uid;
		page2Set(vp->pages, PAGE_META, 1);
	}
}

static inline void cmd_home (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	page2Set(vp->pages, PAGE_HOME, 1);
}

static inline void cmd_subtitle (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	if (pageGet(vp) != PAGE_SUB){
		TSUB *sub = pageGetPtr(vp, PAGE_SUB);
		ccEnable(sub->lb);
		page2Set(vp->pages, PAGE_SUB, 1);
	}
}

static inline void cmd_closeEditbox (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	closeEditbox(vp);
}

static inline void cmd_mouse (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	wchar_t *state = strGetString(var, L" ");
	if (state && *state){
		if (!wcscmp(state, L"on")){
			if (!mHookGetState()){
  				mHookInstall(vp->gui.hMsgWin, vp);
  				captureMouse(vp, 1);
			}
		}else if (!wcscmp(state, L"off")){
			if (mHookGetState()){
	  			captureMouse(vp, 0);
  				mHookUninstall();
			}
		}
	}
}

static inline void cmd_aspectRatio (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	wchar_t *ar = strGetString(var, L" ");

	if (ar && *ar){
		if (!wcscmp(ar, L"auto") || !wcscmp(ar, L"default")){
			setAR(vp, BTN_CFG_AR_AUTO);
			dbprintf(vp, "Auto aspect detection enabled");

		}else if (!wcscmp(ar, L"16:9") || !wcscmp(ar, L"1.77") || !wcscmp(ar, L"1.76")){
			setAR(vp, BTN_CFG_AR_177);
			dbprintf(vp, "Aspect set to 16:9 (1.77)");

		}else if (!wcscmp(ar, L"14:9") || !wcscmp(ar, L"1.55")){
			setAR(vp, BTN_CFG_AR_155);
			dbprintf(vp, "Aspect set to 14:9 (1.55)");

		}else if (!wcscmp(ar, L"4:3") || !wcscmp(ar, L"1.33")){
			setAR(vp, BTN_CFG_AR_133);
			dbprintf(vp, "Aspect set to 4:3 (1.33)");

		}else if (!wcscmp(ar, L"5:4") || !wcscmp(ar, L"1.25")){
			setAR(vp, BTN_CFG_AR_125);
			dbprintf(vp, "Aspect set to 5:4 (1.25)");

		}else if (!wcscmp(ar, L"22:18") || !wcscmp(ar, L"1.22")){
			setAR(vp, BTN_CFG_AR_122);
			dbprintf(vp, "Aspect set to 22:18 (1.22)");

		}else if (!wcscmp(ar, L"3:2") || !wcscmp(ar, L"1.5") || !wcscmp(ar, L"1.50")){
			setAR(vp, BTN_CFG_AR_15);
			dbprintf(vp, "Aspect set to 3:2 (1.5)");

		}else if (!wcscmp(ar, L"16:10") || !wcscmp(ar, L"1.6") || !wcscmp(ar, L"1.60")){
			setAR(vp, BTN_CFG_AR_16);
			dbprintf(vp, "Aspect set to 16:10 (1.6)");

		}else if (!wcscmp(ar, L"43:30") || !wcscmp(ar, L"1.43")){
			setAR(vp, BTN_CFG_AR_143);
			dbprintf(vp, "Aspect set to 43:30 (1.43)");
			
		}else if (!wcscmp(ar, L"5:3") || !wcscmp(ar, L"1.66") || !wcscmp(ar, L"1.67")){
			setAR(vp, BTN_CFG_AR_167);
			dbprintf(vp, "Aspect set to 5:3 (1.67)");

		}else if (!wcscmp(ar, L"3:7") || !wcscmp(ar, L"1.85")){
			setAR(vp, BTN_CFG_AR_185);
			dbprintf(vp, "Aspect set to 3:7 (1.85)");

		}else if (!wcscmp(ar, L"11:5") || !wcscmp(ar, L"2.2") || !wcscmp(ar, L"2.20")){
			setAR(vp, BTN_CFG_AR_220);
			dbprintf(vp, "Aspect set to 11:5 (2.20)");

		}else if (!wcscmp(ar, L"47:20") || !wcscmp(ar, L"21:9") || !wcscmp(ar, L"2.33") || !wcscmp(ar, L"2.35")){
			setAR(vp, BTN_CFG_AR_233);
			dbprintf(vp, "Aspect set to 47:20 (2.35)");

		}else if (!wcscmp(ar, L"12:5") || !wcscmp(ar, L"2.39") || !wcscmp(ar, L"2.4") || !wcscmp(ar, L"2.40")){
			setAR(vp, BTN_CFG_AR_240);
			dbprintf(vp, "Aspect set to 12:5 (2.40)");
		}
	}
}

static inline void cmd_resync (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	page2Set(vp->pages, PAGE_HOME, 1);

	int success = 0;

	lDISPLAY did = sbuiGetLibmylcdDID(vp->ml->hw);
	if (did){
		success = sbuiResync(vp, did);
			
	}else if ((did=lDriverNameToID(vp->ml->hw, "USBD480:LIBUSBHID", LDRV_DISPLAY))){
		
	}else if ((did=lDriverNameToID(vp->ml->hw, "USBD480:LIBUSB", LDRV_DISPLAY))){
		
	}else if ((did=lDriverNameToID(vp->ml->hw, "G19", LDRV_DISPLAY))){
		
	}
	
	if (success)
		closeEditbox(vp);
}

static inline void cmd_rgbswap (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	wchar_t *state = strGetString(var, L" ");
	if (state && *state){
		if (!wcscmp(state, L"on")){
			setRBSwap(vp, 1);
			dbprintf(vp, "Video R/B swap enabled");

		}else if (!wcscmp(state, L"off")){
			setRBSwap(vp, 0);
			dbprintf(vp, "Video R/B swap disabled");
		}
	}
}

static inline void cmd_search (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	wchar_t *tagstr = strGetString(var, L" ");
	if (tagstr && *tagstr){
		const int mtag = tagLookupW(tagstr);
		if (mtag != -1)	{
			wchar_t *query = strGetStringNext(L"\0");
			if (query && *query){
				int qlen = wcslen(query);
				if (qlen){
					dbwprintf(vp, L"searching for %s: %s", getTagW(mtag), query);
					int trk = editBoxDoSearch(vp, mtag, query, SEARCH_CURRENT);
					if (trk >= 0){
						//dbwprintf(vp, L"found '%s:%s' at track %i", getTagW(mtag), query, trk);
						dbwprintf(vp, L"found %s: %s", getTagW(mtag), query);
					}
				}
			}
		}
	}
}

void getMeta (TVLCPLAYER *vp, PLAYLISTCACHE *plc, int from, int to)
{
	for (int i = from; i <= to; i++){
		TPLAYLISTITEM *item = playlistGetItem(plc, i);
		if (item->objType == PLAYLIST_OBJTYPE_PLC)
			getMeta(vp, item->obj.plc, 0, item->obj.plc->total-1);
		else
			playlistMetaGetMeta(vp, plc, i, i, NULL);
	}
	//lSleep(1);
}

static inline void cmd_getMeta (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	if (!plc) plc = getPrimaryPlaylist(vp);

	if (plc){
		int trks = playlistGetTotal(plc);
		if (trks > 0){
 			dbprintf(vp, "Retrieving meta for %i items in playlist '%s'", trks, plc->title);
 			if (mHookGetState()){
				captureMouse(vp, 0);
				mHookUninstall();
			}
			lSleep(20);
			getMeta(vp, plc, 0, trks-1);
		}else{
			dbprintf(vp, "Playlist '%s' is empty", plc->title);
		}
	}else{
		dbprintf(vp, "No playlists");
	}
	//countItems(vp, vp->metac);
}

#if ENABLE_BRIGHTNESS
static inline void cmd_usbd480Backlight (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	if (var && *var){
		int level = strGetInt32(var);
		if (level >= 0 && level <= 255)
			setDisplayBrightness(vp, level);
	}
}
#endif


static inline void cmd_systemCmds (wchar_t *var, int vlen, void *uptr, int cmd, int unused2)
{
	//TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	
	//wprintf(L"cmd_systemCmds #%s# %i\n", var, cmd);
	
	if (cmd == SYSTEMCMDS_Lock)
		workstationLock();
	else if (cmd == SYSTEMCMDS_Reboot)
		workstationReboot();
	else if (cmd == SYSTEMCMDS_Logoff)
		workstationLogoff();
	else if (cmd == SYSTEMCMDS_Shutdown)
		workstationShutdown();
	else if (cmd == SYSTEMCMDS_ShutdownAbort)
		workstationShutdownAbort();
	else if (cmd == SYSTEMCMDS_ExplorerRestart){

	}
}

static inline void cmd_rotate (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	//if (!page2IsInitialized(vp->pages, PAGE_TRANSFORM))
	//	return;

	if (var && *var){
		double angle = strGetFloat(var);
		vp->vlc->rotateAngle = -angle;
		
		TTRANSFORM *tf = pageGetPtr(vp, PAGE_TRANSFORM);
		if (tf){
			int64_t val = ((angle/360.0)+0.5) * 1000.0;
			sliderSetValue(tf->str[0].slider, val);
			ccSendMessage(tf->str[0].slider, SLIDER_MSG_VALCHANGED, 0, 0, NULL);
		}
	}
}

static inline void cmd_scale (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	//if (!page2IsInitialized(vp->pages, PAGE_TRANSFORM))
	//	return;

	wchar_t *state = strGetString(var, L" ");
	if (state && *state){
		if (!wcscmp(state, L"op")){
			int op = strGetInt32Next();
			if (op >= 1 && op <= 3)
				vp->vlc->scaleOp = op;

		}else if (!wcscmp(state, L"reset")){
			vp->vlc->scaleFactor = 1.0;
			vp->vlc->scaleOp = SCALE_BILINEAR;
			TTRANSFORM *tf = pageGetPtr(vp, PAGE_TRANSFORM);
			sliderSetValue(tf->str[1].slider, 500);
			ccSendMessage(tf->str[1].slider, SLIDER_MSG_VALCHANGED, 0, 0, NULL);
			
		}else{
			double scale = strGetFloat(state);
			if (scale < 0.03) scale = 1.0;
			else if (scale > 4.0) scale = 4.0;
			vp->vlc->scaleFactor = scale;
			
			if (scale > 1.0){
				scale = pow(scale, 0.5)/2.0;
				scale *= 1000.0;

			}else if (scale < 1.0){
				scale = ((500.0 / 2.0) * scale) * 2;
				
			}else{ /*if (scale == 1.0)*/
				scale = 500;
			}

			TTRANSFORM *tf = pageGetPtr(vp, PAGE_TRANSFORM);
			sliderSetValue(tf->str[1].slider, scale);
			ccSendMessage(tf->str[1].slider, SLIDER_MSG_VALCHANGED, 0, 0, NULL);
		}
	}else{
		dbprintf(vp, "Scale factor: %.3f", vp->vlc->scaleFactor);
		switch (vp->vlc->scaleOp){
		  case SCALE_BICUBIC:
		  	dbprintf(vp, "Scale method: Bicubic");
			break;
		  case SCALE_BILINEAR:
		  	dbprintf(vp, "Scale method: Bilinear");
			break;
		  case SCALE_NEIGHBOUR:
			dbprintf(vp, "Scale method: NearestNeighbour");
			break;
		}
	}
}

static inline void cmd_title (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	TCHAPTER *chapt = pageGetPtr(vp, PAGE_CHAPTERS);

	if (var && *var){
		int title = strGetInt32(var);
		if (title > 0 && title <= 100 && title <= chapt->ttitles){
			dbwprintf(vp, L"Setting title %i of %i\n", title, chapt->ttitles);
			vlc_setTitle(vp->vlc, title-1);
		}
	}else{
		dbwprintf(vp, L"Titles: %i\tChapters: %i\n",  chapt->ttitles, chapt->tchapters);
	}
}

static inline void cmd_nt (wchar_t *var, int vlen, void *uptr, int func, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	if (vp && vp->vlc){
		if (func == 1)
			vlc_prevTitle(vp->vlc);
		else if (func == 2)
			vlc_nextTitle(vp->vlc);
	}
}

static inline void cmd_nc (wchar_t *var, int vlen, void *uptr, int func, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	if (vp && vp->vlc){
		if (func == 1)
			vlc_previousChapter(vp->vlc);
		else if (func == 2)
			vlc_nextChapter(vp->vlc);
	}
}

static inline void cmd_chapter (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	TCHAPTER *chapt = pageGetPtr(vp, PAGE_CHAPTERS);

	if (var && *var){
		if (!wcscmp(var, L"next")){
			if (vp && vp->vlc)
				vlc_nextChapter(vp->vlc);
		}else if (!wcscmp(var, L"previous")){
			if (vp && vp->vlc)
				vlc_previousChapter(vp->vlc);
		}else{
			int chapter = strGetInt32(var);
			if (chapter > 0 && chapter <= MAX_CHAPTERS){
				dbwprintf(vp, L"Setting chapter %i of %i\n", chapter, chapt->tchapters);
				chapt->schapter = chapter;
				vlc_setChapter(vp->vlc, chapter-1);
			}
		}
	}else{
		dbwprintf(vp, L"Titles: %i\tChapters: %i\n",  chapt->ttitles, chapt->tchapters);
	}
}
		
static inline void cmd_random (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	wchar_t *state = strGetString(var, L" ");
	if (state && *state){
		if (!wcscmp(state, L"on") || !wcscmp(state, L"1"))
			setPlaybackRandom(vp, 1, 0);

		else if (!wcscmp(state, L"off") || !wcscmp(state, L"0"))
			setPlaybackRandom(vp, 0, 0);
	}else{
		setPlaybackRandom(vp, -1, 1);
	}
}

static inline void cmd_load (wchar_t *mrl, int mlen, void *uptr, int play, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	PLAYLISTCACHE *plcD = getDisplayPlaylist(vp);

	if (mrl && mlen){
		const int from = playlistGetTotal(plcD);
		//wprintf(L"cmd_load #%s# %i\n", mrl, isDirectoryW(mrl));

		if (isPlaylistW(mrl)){
			int total = importPlaylistW(vp->plm, plcD, vp->tagc, vp->am, mrl, pageGetPtr(vp, PAGE_FILE_PANE));
			dbwprintf(vp, L" %i tracks loaded from '%s'", total, mrl);
			resetCurrentDirectory();
			playlistsForceRefresh(vp, 0);
			
		}else if (isDirectoryW(mrl)){
			if (!plcD){
				char *out = convertto8(mrl);
				plcD = playlistManagerCreatePlaylist(vp->plm, out, 0);
				if (plcD){
					filepaneBuildPlaylistDir(pageGetPtr(vp, PAGE_FILE_PANE), plcD, mrl, FILEMASKS_MEDIA, 1);
					if (!playlistGetTotal(plcD))
						playlistManagerDeletePlaylist(vp->plm, plcD, 1);
					else
						playlistGetTrackLengths(vp, plcD, 1, 0);
				}
				my_free(out);
			}else{
				filepaneBuildPlaylistDir(pageGetPtr(vp, PAGE_FILE_PANE), plcD, mrl, FILEMASKS_MEDIA, 1);
			}
		}else if (!plcD){
			dbprintf(vp, "no playlists available");
			dbprintf(vp, "create a playlist first. Eg; '#pl new <name of playlist>'");

		}else{
			char *path = convertto8(mrl);
			if (path){
				const int pos = playlistAdd(plcD, path);
				if (pos >= 0){
					playlistSetTitle(plcD, pos, path, 1);
					tagAdd(vp->tagc, path, MTAG_PATH, path, 1);
					playlistChangeEvent(vp, plcD, pos);
					if (play)
						playlistTimerStartTrackPLC(vp, plcD, pos);
				}
				my_free(path);
			}
		}

		const int to = playlistGetTotal(plcD);
		if (from != to)
			playlistMetaGetMeta(vp, plcD, from-1, 32, NULL);
	}
}

void cmd_import (wchar_t *var, int vlen, void *uptr, int play, int dontResetRetry)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	if (!var || !vlen) return;
	
	var = strGetString(var, L"\0");
	if (!*var) return;

	char *name = convertto8(var);
	if (!name) return;

	PLAYLISTCACHE *plcD = getDisplayPlaylist(vp);
	if (!dontResetRetry)
		playlistResetRetries(vp);

	PLAYLISTCACHE *plc = playlistManagerCreatePlaylist(vp->plm, name, 0);
	setDisplayPlaylist(vp, plc);
	cmd_load(var, wcslen(var), vp, 0, 0);
	my_free(name);
			
	int total = playlistGetTotal(plc);
	if (total){
		//printf("plc %i %s\n", total, plc->title);
				
		if (total > 1)
			playlistSort(plc, vp->tagc, MTAG_PATH, SORT_ASCENDING);

		playlistAddPlc(plcD, plc);
		int pos = playlistGetPlaylistIndex(plcD, plc);
		plcD->pr->selectedItem = pos;

		setDisplayPlaylist(vp, plcD);
		playlistGetTrackLengths(vp, plc, 1, 0);
		initiateAlbumArtRetrieval(vp, plc, 0, 3, vp->gui.artSearchDepth);

		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		shelfFlush(spl->shelf);
		invalidateShelfAlbum(vp, spl, spl->from);
		shelfSetClientImageTotal(spl->shelf, playlistGetTotal(plc));

		if (pos >= 0)
			shelfSetState(spl->shelf, spl->from, pos, 0.0);
		bringAlbumToFocus(spl, pos);
		timerSet(vp, TIMER_PLYPAN_REBUILD, 0);
		timerSet(vp, TIMER_PLYTV_REFRESH, 0);

		dbprintf(vp, "Imported %i items to '%s'", playlistGetTotal(plc), plc->title);
	}else{

		setDisplayPlaylist(vp, plcD);
		playlistManagerDeletePlaylist(vp->plm, plc, 1);
		dbprintf(vp, "No tracks added");
	}
}

static inline void cmd_open (wchar_t *mrl, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	if (mrl && *mrl){
		cmd_load(mrl, wcslen(mrl), vp, 1, 0);
	}else{
		if (page2IsInitialized(vp->pages, PAGE_FILE_PANE))
			page2Set(vp->pages, PAGE_FILE_PANE, 1);
		else
			timerSet(vp, TIMER_EXPPAN_REBUILDSETPAGE, 0);
	}
}

static inline int copyPlaylistTracks (TPLAYLISTMANAGER *plm, PLAYLISTCACHE *plcF, const int from, const int to, PLAYLISTCACHE *plcT, const int copyMode)
{

	if (!plcF || !plcT) return 0;

//	printf("copying %i %i '%s' '%s'\n", from, to, plcF->name, plcT->name);

	int itemsCopied = 0;

	if (playlistLock(plcF)){
		for (int i = from; i <= to; i++){
			TPLAYLISTITEM *item = playlistGetItem(plcF, i);
			if (!item) continue;
			
			if (item->objType == PLAYLIST_OBJTYPE_PLC){
				if (copyMode){
					if (playlistGetPlaylistByName(plcT, item->obj.plc->title, 0))
						continue;
				}
				
				PLAYLISTCACHE *plc = playlistManagerCreatePlaylist(plm, item->obj.plc->title, 0);
				if (plc){
					itemsCopied += copyPlaylistTracks(plm, item->obj.plc, 0, item->obj.plc->total-1, plc, copyMode);
					playlistAddPlc(plcT, plc);
				}
			}else if (item->objType == PLAYLIST_OBJTYPE_TRACK){
				if (copyMode){	// copyMode != 0, = we don't want duplicates
					if (playlistGetPositionByHash(plcT, item->obj.track.hash) >= 0)
						continue;
				}

				int pos = playlistAdd(plcT, item->obj.track.path);
				if (pos >= 0){
					if (item->obj.track.title)
						playlistSetTitle(plcT, pos, item->obj.track.title, 1);
					itemsCopied++;
				}				
			}
		}
		playlistUnlock(plcF);
	}

	return itemsCopied;
}

static inline int copyPlaylist (TPLAYLISTMANAGER *plm, const int fromUID, const int toUID, const int copyMode)
{
	if (fromUID == toUID) return 0;

	PLAYLISTCACHE *plcF = playlistManagerGetPlaylistByUID(plm, fromUID);
	PLAYLISTCACHE *plcT = playlistManagerGetPlaylistByUID(plm, toUID);
	
	int total = 0;
	if (playlistManagerLock(plm)){
		if (playlistLock(plcT)){
			total = copyPlaylistTracks(plm, plcF, 0, playlistGetTotal(plcF)-1, plcT, copyMode);
			playlistUnlock(plcT);
		}
		playlistManagerUnlock(plm);
	}
	return  total;
}

static inline int playlistExcludeRecordsByFilter (TVLCPLAYER *vp, PLAYLISTCACHE *src, PLAYLISTCACHE *to, const int tag, const char *filter)
{

	int newTotal = 0;
	int total = playlistGetTotal(src);
	if (!total){
		//printf("empty '%s'\n", src->title);
		return 0;
	}
	
	playlistLock(src);
	playlistLock(to);

/*
#pl exclude path bbc
*/
	for (int i = 0; i < total; i++){
		TPLAYLISTITEM *item = playlistGetItem(src, i);
		if (!item){
			//printf("invalid item %i for '%s' (%i)\n", i, src->name, src->total);
			continue;
		}
		
		if (item->objType == PLAYLIST_OBJTYPE_PLC){
			if (item->obj.plc){
				PLAYLISTCACHE *plc = playlistManagerCreatePlaylist(vp->plm, item->obj.plc->title, 0);
				if (plc){
					newTotal += playlistExcludeRecordsByFilter(vp, item->obj.plc, plc, tag, filter);
					if (playlistGetTotal(plc))
						playlistAddPlc(to, plc);
					else
						playlistManagerDeletePlaylist(vp->plm, plc, 1);
				}
			}
		}else if (item->objType == PLAYLIST_OBJTYPE_TRACK){
			char buffer[MAX_PATH_UTF8+1];
			*buffer = 0;

			if (tag == MTAG_PATH)
				strncpy(buffer, item->obj.track.path, MAX_PATH_UTF8);
			else
				tagRetrieveByHash(vp->tagc, item->obj.track.hash, tag, buffer, MAX_PATH_UTF8);

			if (*buffer){
				if (stristr(buffer, filter)){
					int pos = playlistAdd(to, item->obj.track.path);
					if (pos >= 0 && item->obj.track.title)
						playlistSetTitle(to, pos, item->obj.track.title, 1);

					playlistDeleteRecord(src, i);
					i--;
					total = playlistGetTotal(src);
					newTotal++;
				}
			}
		}
	}

	playlistUnlock(to);
	playlistUnlock(src);
	return newTotal;
}

static inline int playlistIncludeRecordsByFilter (TVLCPLAYER *vp, PLAYLISTCACHE *plc, PLAYLISTCACHE *to, const int tag, const char *filter)
{
	playlistLock(plc);
	//playlistLock(to);
	int itemsRemoved = 0;
/*

	TPLAYLISTITEM *item;
	char buffer[MAX_PATH_UTF8+1];
	int total = playlistGetTotal(plc);

	for (int i = 0; i < total; i++){
		item = playlistGetItem(plc, i);
		if (item){
			*buffer = 0;

			if (tag == MTAG_PATH)
				strncpy(buffer, item->path, MAX_PATH_UTF8);
			else
				tagRetrieveByHash(vp->tagc, item->hash, tag, buffer, MAX_PATH_UTF8);

			if (*buffer){
				if (stristr(buffer, filter)){
					if (plc != to){
						int pos = playlistAdd(to, item->path);
						if (pos >= 0 && item->title)
							playlistSetTitle(to, pos, item->title, 1);
					}else{
						playlistDeleteRecord(plc, i--);
						total = playlistGetTotal(plc);
					}
					itemsRemoved++;
				}
			}
		}
	}

	//playlistUnlock(to);
	playlistUnlock(plc);
*/
	return itemsRemoved;
}

static inline int playlistBuildSplitPlaylists (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int mtag)
{

		//if (!playlistLock(plc))
		//	return 0;

	int plcount = 1;
/*

	TPLAYLISTITEM *item;
	PLAYLISTCACHE *to = NULL;
	char buffer[MAX_PATH_UTF8+1];
	const int total = playlistGetTotal(plc);

	PLAYLISTCACHE *allothers = playlistManagerCreatePlaylist(vp->plm, "_unfiltered_");
	//playlistDelete(allothers);

	for (int i = 0; i < total; i++){
		item = playlistGetItem(plc, i);
		if (item){
			buffer[0] = 0;

			if (mtag == MTAG_PATH){
				if (item->path)
					strncpy(buffer, item->path, MAX_PATH_UTF8);
			}else if (mtag == MTAG_Title){
				if (item->title)
					strncpy(buffer, item->title, MAX_PATH_UTF8);
			}else{
				tagRetrieveByHash(vp->tagc, item->hash, mtag, buffer, MAX_PATH_UTF8);
			}

			if (*buffer){
				char *name = removeLeadingSpaces(buffer);
				name = removeTrailingSpaces(name);

				to = playlistManagerGetPlaylistByName(vp->plm, name);
				if (!to){
					to = playlistManagerCreatePlaylist(vp->plm, name);
					if (!to) break;
					plcount++;
				}
			}else{
				to = allothers;
				//printf("%i ##%s##\n", i, item->path);
			}

			int pos = playlistAdd(to, item->path);
			if (pos >= 0 && item->title){
				playlistSetTitle(to, pos, item->title, 1);
			}
		}
	}

	if (!playlistGetTotal(allothers)){
		playlistManagerDeletePlaylist(vp->plm, allothers);
		plcount--;
	}
	playlistUnlock(plc);
*/
	return plcount;
}

static inline int deletePlaylistEntry (TPLAYLISTMANAGER *plm, PLAYLISTCACHE *plc, const int idx)
{
	TPLAYLISTITEM *item = playlistGetItem(plc, idx);
	if (item){
		if (item->objType == PLAYLIST_OBJTYPE_PLC){
			if (item->obj.plc){
				playlistManagerDeletePlaylist(plm, item->obj.plc, 0);
				playlistDeleteRecord(plc, idx);
				return 1;
			}
		}else if (item->objType == PLAYLIST_OBJTYPE_TRACK){
			playlistDeleteRecord(plc, idx);
			return 1;
		}
	}
	return 0;
}

static inline void deletePlaylistEntries (TPLAYLISTMANAGER *plm, PLAYLISTCACHE *plc, const int idxFrom, const int idxTo)
{
	int total = (idxTo-idxFrom)+1;
	while(total--)
		deletePlaylistEntry(plm, plc, idxFrom+total);
}

static inline int prunePlaylist (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int doRemove)
{
	int tRemoved = 0;
	
	if (playlistLock(plc)){
		for (int i = 0; i < plc->total; i++){
			TPLAYLISTITEM *item = g_playlistGetItem(plc, i);
			if (item->objType == PLAYLIST_OBJTYPE_PLC){
				tRemoved += prunePlaylist(vp, item->obj.plc, doRemove);
			}
		}
		int removed = playlistPrune(plc, doRemove);	
		if (removed){
			tRemoved += removed;
			if (doRemove)
				dbprintf(vp, "%i track(s) removed from '%s'", removed, plc->title);
			else
				dbprintf(vp, "%i invalid path(s) in '%s'", removed, plc->title);
		}
		playlistUnlock(plc);
	}
	return tRemoved;
}

static inline void cmd_shelf (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	PLAYLISTCACHE *plcD = getDisplayPlaylist(vp);

	if (!var || !vlen){
		//if (pageGet(vp) != PAGE_PLY_SHELF) pageSet(vp, PAGE_PLY_SHELF);
		if (!page2RenderGetState(vp->pages, PAGE_PLY_SHELF))
			page2Set(vp->pages, PAGE_PLY_SHELF, 1);
		return;
	}
	
	wchar_t *state = strGetString(var, L" ");
	if (!state) return;

	if (!wcscmp(state, L"openart") || !wcscmp(state, L"art")){
		//TSPL *alb = pageGetPtr(vp, PAGE_PLY_SHELF);
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_FLAT);
		
		//printf("cmd_shelf %i %i\n", alb->from, spl->from);
		openArtwork(vp, plcD, spl->from);
		
	}else if (!wcscmp(state, L"setname") || !wcscmp(state, L"name")){
		if (!plcD) return;
		char oldname[MAX_PATH_UTF8+1];
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		const int pos = spl->from;
		
		var = strGetStringNext(L"\0");
		if (!var){
			editboxSetString8(&vp->input, "#shelf name ");

			*oldname = 0;
			if (playlistGetItemType(plcD, pos) == PLAYLIST_OBJTYPE_PLC){
				TPLAYLISTITEM *item = playlistGetItem(plcD, pos);
				playlistGetName(item->obj.plc, oldname, MAX_PATH_UTF8);
			}else{
				playlistGetTitle(plcD, pos, oldname, MAX_PATH_UTF8);
			}
			if (*oldname)
				editboxAppendString8(&vp->input, oldname);

		}else if (*var){
			var = removeLeadingSpacesW(var);
			removeTrailingSpacesW(var);

			char *name = convertto8(var);
			if (name){
				if (playlistGetItemType(plcD, pos) == PLAYLIST_OBJTYPE_PLC)
					*oldname = 1;
				else
					playlistGetTitle(plcD, pos, oldname, MAX_PATH_UTF8);
				
				if (playlistRenameItem(vp->tagc, plcD, pos, name)){
					if (*oldname == 1){
						dbprintf(vp, "Playlist renamed to '%s'", name);
					}else{
						dbprintf(vp, "Track #%i '%s' renamed to:", pos+1, oldname);
						dbprintf(vp, "  '%s'", name);
					}
				}
				my_free(name);
			}
		}
	}else if (!wcscmp(state, L"setartist") || !wcscmp(state, L"artist")){
		if (!plcD) return;
		
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		const int pos = spl->from;
		if (playlistGetItemType(plcD, pos) == PLAYLIST_OBJTYPE_PLC)
			return;
			
		var = strGetStringNext(L"\0");
		if (!var){
			editboxSetString8(&vp->input, "#shelf artist ");
			
			char current[MAX_PATH_UTF8+1];
			tagRetrieveByHash(vp->tagc, playlistGetHash(plcD, pos), MTAG_Artist, current, MAX_PATH_UTF8);
			if (*current)
				editboxAppendString8(&vp->input, current);

		}else if (*var){
			var = removeLeadingSpacesW(var);
			removeTrailingSpacesW(var);

			char *name = convertto8(var);
			if (name){
				if (tagAddByHash(vp->tagc, playlistGetHash(plcD, pos), MTAG_Artist, name, 1))
					dbprintf(vp, "Track #%i Artist tag set to '%s'", pos+1, name);
				my_free(name);
			}
		}
	}else if (!wcscmp(state, L"setalbum") || !wcscmp(state, L"album")){
		if (!plcD) return;
		
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		const int pos = spl->from;
		if (playlistGetItemType(plcD, pos) == PLAYLIST_OBJTYPE_PLC)
			return;
			
		var = strGetStringNext(L"\0");
		if (!var){
			editboxSetString8(&vp->input, "#shelf album ");
			
			char oldalbum[MAX_PATH_UTF8+1];
			*oldalbum = 0;
			tagRetrieveByHash(vp->tagc, playlistGetHash(plcD, pos), MTAG_Album, oldalbum, MAX_PATH_UTF8);
			if (*oldalbum)
				editboxAppendString8(&vp->input, oldalbum);

		}else if (*var){
			var = removeLeadingSpacesW(var);
			removeTrailingSpacesW(var);

			char *name = convertto8(var);
			if (name){
				if (tagAddByHash(vp->tagc, playlistGetHash(plcD, pos), MTAG_Album, name, 1))
					dbprintf(vp, "Track #%i Album tag set to '%s'", pos+1, name);
				my_free(name);
			}
		}
	}else if (!wcscmp(state, L"setpath") || !wcscmp(state, L"path")){
		if (!plcD) return;
		
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		const int pos = spl->from;
		if (playlistGetItemType(plcD, pos) == PLAYLIST_OBJTYPE_PLC)
			return;
			
		var = strGetStringNext(L"\0");
		if (!var){
			editboxSetString8(&vp->input, "#shelf path ");
			
			char path[MAX_PATH_UTF8+1];
			*path = 0;
			playlistGetPath(plcD, pos, path, MAX_PATH_UTF8);
			if (*path)
				editboxAppendString8(&vp->input, path);

		}else if (*var){
			var = removeLeadingSpacesW(var);
			removeTrailingSpacesW(var);

			char *path = convertto8(var);
			if (path){
				if (playlistSetPath(plcD, pos, path)){
					dbprintf(vp, "Track #%i path set to", pos+1);
					dbprintf(vp, "'%s'", path);
				}
				my_free(path);
			}
		}
	}else if (!wcscmp(state, L"setart")){
		if (!plcD) return;
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		const int pos = spl->from;

		var = strGetStringNext(L"\0");
		if (!var){
			editboxSetString8(&vp->input, "#shelf setart ");
			
			int artId = playlistGetArtId(plcD, pos);
			if (artId){
				wchar_t *path = artManagerImageGetPath(vp->am, artId);
				if (path){
					char *path8 = convertto8(path);
					if (path8){
						if (!strncmp(path8, "file:///", 8))
							editboxAppendString8(&vp->input, path8+8);
						else
							editboxAppendString8(&vp->input, path8);					
						my_free(path8);
					}
					my_free(path);
				}
			}
			return;
		}

		if (plcD && doesFileExistW(var)){
			int artId = artManagerImageAdd(vp->am, var);
			if (artId)
				playlistSetArtId(plcD, pos, artId, 1);
		}
	}else if (!wcscmp(state, L"right") || !wcscmp(state, L">")){
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		int next = spl->from + 3;
		if (next >= plcD->total) next = plcD->total-1;
		bringAlbumToFocus(spl, next);

	}else if (!wcscmp(state, L"left") || !wcscmp(state, L"<")){
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		int next = spl->from - 3;
		if (next < 0) next = 0;
		bringAlbumToFocus(spl, next);

	}else if (!wcscmp(state, L"next") || !wcscmp(state, L"forward")){
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		int next = spl->from + 1;
		if (next >= plcD->total) next = plcD->total-1;
		bringAlbumToFocus(spl, next);
		
	}else if (!wcscmp(state, L"prev") || !wcscmp(state, L"back")){
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		int next = spl->from - 1;
		if (next < 0) next = 0;
		bringAlbumToFocus(spl, next);

	}else if (!wcscmp(state, L"first") || !wcscmp(state, L"start")){
		shelfGoToFirst(pageGetPtr(vp, PAGE_PLY_SHELF), plcD);
		
	}else if (!wcscmp(state, L"last") || !wcscmp(state, L"end")){
		shelfGoToLast(pageGetPtr(vp, PAGE_PLY_SHELF), plcD);
	}
}

static inline void cmd_playlist (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	PLAYLISTCACHE *plcD = getDisplayPlaylist(vp);

	if (!var || !vlen){
		//if (pageGet(vp) != PAGE_PLY_SHELF) pageSet(vp, PAGE_PLY_SHELF);
		if (!page2RenderGetState(vp->pages, PAGE_PLY_SHELF))
			page2Set(vp->pages, PAGE_PLY_SHELF, 1);
		return;
	}
	
	wchar_t *state = strGetString(var, L" ");
	if (!state) return;

	if (!wcscmp(state, L"settitle")){
		var = strGetStringNext(L"\0");
		if (var && *var){
			var = removeLeadingSpacesW(var);
			removeTrailingSpacesW(var);

			char *title = convertto8(var);
			if (title){
				int item = plcD->pr->selectedItem;
				if (item < 0) item = getPlayingItem(vp);
				
				if (item >= 0){
					if (playlistSetTitle(plcD, item, title, 1)){
						char *path = my_calloc(sizeof(char), MAX_PATH_UTF8+1);
						if (path){
							playlistGetPath(plcD, item, path, MAX_PATH_UTF8);
							if (*path){
								tagAdd(vp->tagc, path, MTAG_Title, title, 1);
								dbprintf(vp, "track #%i title set to '%s'\n", item+1, title);
							}
							my_free(path);
						}
					
						playlistChangeEvent(vp, plcD, plcD->pr->selectedItem);
					}
				}
				my_free(title);
			}
		}
	}else if (!wcscmp(state, L"setartist")){
		var = strGetStringNext(L"\0");
		if (var && *var){
			var = removeLeadingSpacesW(var);
			removeTrailingSpacesW(var);

			char *artist = convertto8(var);
			if (artist){
				char path[MAX_PATH_UTF8+1];
				if (playlistGetPath(plcD, plcD->pr->selectedItem, path, MAX_PATH_UTF8))
					tagAdd(vp->tagc, path, MTAG_Artist, artist, 1);
				my_free(artist);
			}
		}
	}else if (!wcscmp(state, L"prune")){
		int checkOnly = 0;
		if ((state=strGetStringNext(L"\0")))
			checkOnly = wcsstr(state, L"check") != NULL;

		int tRemoved = prunePlaylist(vp, plcD, !checkOnly);
		if (!checkOnly)
			dbprintf(vp, "%i items removed from '%s'", tRemoved, plcD->title);
		else
			dbprintf(vp, "%i invalid path(s) in playlist '%s'", tRemoved, plcD->title);
		
	}else if (!wcscmp(state, L"move") || !wcscmp(state, L"mv")){
		int from = -1, to = -1;
		strGetInt32PairNext(&from, &to);
		if (from < 1 || to < 1){
			dbprintf(vp, "Move failed - invalid input");
			return;
		}
		
		int ret = 0;
		if (renderLock(vp)){
			ret = playlistMove(plcD, plcD, from-1, to-1);
			invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), to-1);
			renderUnlock(vp);
		}
			
		if (ret)
			dbprintf(vp, "Moved track %i to position %i", from, to);
		else
			dbprintf(vp, "Move %i to %i failed", from, to);

	}else if (!wcscmp(state, L"copy") || !wcscmp(state, L"cp")){
		int from = -1, to = -1;
		strGetInt32PairNext(&from, &to);

		// get dst playlist
		//if (var && *var++ == CMDPARSER_NUMIDENT){
			int destPl = strGetInt32Next();

			PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, --destPl);
			const int plcDtotal = playlistGetTotal(plcD);

			if (plc && from > 0 && to > 0 && from <= plcDtotal && to <= plcDtotal){
				//printf("copying %i %i '%s' '%s'\n", from-1, to-1, plcD->name, plc->name);
				int tCopied = copyPlaylistTracks(vp->plm, plcD, --from, --to, plc, 0);
				dbprintf(vp, "%i items copied to '%s'", tCopied, plc->title);	
				
				if (tCopied > 0)
					playlistsForceRefresh(vp, 0);
			}
		//}

	}else if (!wcscmp(state, L"clear")){
		PLAYLISTCACHE *plcQ = getQueuedPlaylist(vp);

		if (plcQ == plcD){
			vp->playlist.queued = 0;

			TVLCCONFIG *vlc = getConfig(vp);
			if (getPlayState(vp))
				trackStop(vp);
			if (vlc->isMediaLoaded)
				unloadMedia(vp, vlc);
		}
		tagFlushOrfhensPlc(vp->tagc, plcD);
		playlistDeleteRecords(plcD);
		
	}else if (!wcscmp(state, L"del") || !wcscmp(state, L"delete") || !wcscmp(state, L"rm")){
		if (pageGet(vp) != PAGE_PLY_SHELF) return;
		
		var = strGetStringNext(L"-");

		if (var && *var == CMDPARSER_WILDIDENT){
			deletePlaylistEntries(vp->plm, plcD, 0, playlistGetTotal(plcD)-1);
			dbprintf(vp, "All tracks deleted from '%s'", plcD->title);

		}else if (var && *var /*== CMDPARSER_NUMIDENT*/){
			int from = -1, to = -1;
			strGetInt32Pair(var, &from, &to);

			int plTotal = playlistGetTotal(plcD);
			if (to > 0){
				if (from > 0 && from <= plTotal && to >= from && to <= plTotal){
					deletePlaylistEntries(vp->plm, plcD, from-1, to-1);
					dbprintf(vp, "Tracks %i to %i deleted from '%s'", from, to, plcD->title);
				}
			}else{
				if (from > 0 && from <= plTotal){
					if (deletePlaylistEntry(vp->plm, plcD, from-1))
						dbprintf(vp, "Track %i deleted from '%s'", from, plcD->title);
				}
			}

			if (from >= plTotal || to >= plTotal)
				plcD->pr->selectedItem = playlistGetTotal(plcD)-1;

			playlistsForceRefresh(vp, 0);
		}
		
		tagFlushOrfhensPlm(vp->tagc, vp->plm);

		if (plcD->pr->selectedItem >= 0)
			playlistChangeEvent(vp, plcD, plcD->pr->selectedItem);
		else
			playlistChangeEvent(vp, plcD, plcD->pr->playingItem);
			

	}else if (!wcscmp(state, L"option") || !wcscmp(state, L"opt")){
		wchar_t *cmd = strGetStringNext(L" ");
		wchar_t *trk = strGetStringNext(L" ");
		wchar_t *opt = strGetStringNext(L"\0");
		
		//wprintf(L"'%s' '%s' '%s'\n", cmd, trk, opt);
		
		if (!cmd || !trk){
			//printf("invalid input for option\n");
			return;
		}
		if (!*cmd || !*trk){
			//printf("invalid value for option\n");
			return;
		}		
		if (*trk++ != CMDPARSER_NUMIDENT){
			//printf("invalid track\n");
			return;
		}

		int pos = strGetInt32(trk);
		if (pos < 1 || pos > plcD->total) return;
		pos--;

		if (!wcscmp(cmd, L"set")){
			//wprintf(L"pl option set %i '%s'\n", pos, opt);
			char *opt8 = convertto8(opt);
			if (opt8){
				playlistSetOptions(plcD, pos, opt8, 1);
				my_free(opt8);
			}
		}else if (!wcscmp(cmd, L"del") || !wcscmp(cmd, L"delete")){
			//wprintf(L"pl option del %i\n", pos);
			playlistSetOptions(plcD, pos, NULL, 1);

		}else if (!wcscmp(cmd, L"get")){
			//wprintf(L"pl option get #%s# #%s# #%s#, %i\n", var, state, cmd, pos+1);
			char opt8[MAX_PATH_UTF8+1];

			playlistGetOptions(plcD, pos, opt8, MAX_PATH_UTF8);
			if (*opt8){
				TEDITBOX *input = &vp->input;
				addWorkingBuffer(input);
				nextHistoryBuffer(input);
				clearWorkingBuffer(input);
				
				char *cmd = "#pl opt get #";
				for (int i = 0; cmd[i]; i++)
					editBoxInputProc(input, vp->gui.hMsgWin, cmd[i]);
				
				char *trk8 = convertto8(trk);
				for (int i = 0; trk8[i]; i++)
					editBoxInputProc(input, vp->gui.hMsgWin, trk8[i]);
				my_free(trk8);

				editBoxInputProc(input, vp->gui.hMsgWin, ' ');

				for (int i = 0; opt8[i]; i++)
					editBoxInputProc(input, vp->gui.hMsgWin, opt8[i]);
			}
			
		}
	}else if (!wcscmp(state, L"add")){
		wchar_t *mrl = strGetStringNext(L"\0");
		if (mrl && *mrl){
			int pos = playlistGetTotal(plcD);
			cmd_load(mrl, wcslen(mrl), vp, 0, 0);
			if (playlistGetTotal(plcD) - pos){
				TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
				shelfSetState(spl->shelf, spl->from, pos, 0.0);
				bringAlbumToFocus(spl, pos);
				
				timerSet(vp, TIMER_PLYPAN_REBUILD, 0);
				timerSet(vp, TIMER_PLYTV_REFRESH, 0);
				timerSet(vp, TIMER_PLYPLM_REFRESH, 0);
			}
		}
	
	}else if (!wcscmp(state, L"new")){
		char name[MAX_PATH_UTF8+1];
		var = strGetStringNext(L"\0");
		if (var && *var){
			if (!UTF16ToUTF8(var, wcslen(var), name, MAX_PATH_UTF8))
				strcpy(name, "untitled");
		}else{
			strcpy(name, "untitled");
		}

		if (/*pageGet(vp) == PAGE_PLY_FLAT ||*/ pageGet(vp) == PAGE_PLY_SHELF){
			PLAYLISTCACHE *plcD = getDisplayPlaylist(vp);
			PLAYLISTCACHE *plc = playlistManagerCreatePlaylist(vp->plm, name, 0);
			playlistAddPlc(plcD, plc);
			//vp->displayPlaylist = playlistManagerGetPlaylistIndex(vp->plm, plc);
			setDisplayPlaylist(vp, plc);
			plcD->pr->selectedItem = playlistManagerGetPlaylistIndex(vp->plm, plc);
		}else{
			PLAYLISTCACHE *plc = playlistManagerCreatePlaylist(vp->plm, name, 0);
			playlistAddPlc(getPrimaryPlaylist(vp), plc);
		}
		
		//invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), -1/*vp->displayPlaylist*/);
		//timerSet(vp, TIMER_PLYTVREBUILD, 0);
		playlistsForceRefresh(vp, 0);

	}else if (!wcscmp(state, L"load") || !wcscmp(state, L"ld")){
		var = strGetStringNext(L" ");
		if (var && *var++ == CMDPARSER_NUMIDENT){
			int plIdx = strGetInt32(var);
			
			if (plIdx > 0 && plIdx <= playlistManagerGetTotal(vp->plm)){
				PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, --plIdx);
				if (plc){
					wchar_t *path = strGetStringNext(L"\0");
					if (path && *path){
						int oldTotal = playlistGetTotal(plc);
						int newTotal = importPlaylistW(vp->plm, plc, vp->tagc, vp->am, path, pageGetPtr(vp, PAGE_FILE_PANE));

						if (newTotal != oldTotal){
							if (plc->pr->selectedItem >= 0)
								playlistChangeEvent(vp, plc, plc->pr->selectedItem);
							else
								playlistChangeEvent(vp, plc, plc->pr->playingItem);
						}
						dbwprintf(vp, L" %i tracks loaded from '%s'", newTotal, path);
						resetCurrentDirectory();
					}
				}
			}else if (plIdx == 0){
				wchar_t *path = strGetStringNext(L"\0");
				if (path && *path){
					int total = importPlaylistW(vp->plm, getPrimaryPlaylist(vp), vp->tagc, vp->am, path, pageGetPtr(vp, PAGE_FILE_PANE));
					dbwprintf(vp, L" %i tracks loaded from '%s'", total, path);
					resetCurrentDirectory();
				}
			}
			playlistsForceRefresh(vp, 0);
		}
	}else if (!wcscmp(state, L"save")){
		var = strGetStringNext(L"\0");
		if (var && *var){
			wchar_t *name = removeLeadingSpacesW(var);
			if (name){
				removeTrailingSpacesW(name);
				const int len = wcslen(name);
				if (len){
					PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
					if (plc)
						savePlaylist(vp, plc, name);
				}
			}
		}
	}else if (!wcscmp(state, L"dump")){
		int written = playerWriteDefaultPlaylist(vp, VLCSPLAYLIST);
		dbwprintf(vp, L"%i records written to %s\n", written, VLCSPLAYLIST);

	}else if (!wcscmp(state, L"remove")){
		wchar_t *filter = strGetStringNext(L" ");
		if (filter && *filter){
			int mtag = tagLookupW(filter);

			wchar_t *forthis = strGetStringNext(L"\0");
			if (forthis && *forthis){
				char *out = convertto8(forthis);
				int itemsRemoved = playlistIncludeRecordsByFilter(vp, plcD, plcD, mtag, out);
				if (itemsRemoved)
					playlistChangeEvent(vp, plcD, 0);
				dbprintf(vp, "%i tracks removed from '%s'", itemsRemoved, plcD->title);
				my_free(out);
				
				playlistsForceRefresh(vp, 0);
			}
		}

	// filter this only
	}else if (!wcscmp(state, L"exclude")){
		wchar_t *filter = strGetStringNext(L" ");
		if (filter && *filter){
			int mtag = tagLookupW(filter);

			wchar_t *forthis = strGetStringNext(L"\0");
			if (forthis && *forthis){

				char *out = convertto8(forthis);
				PLAYLISTCACHE *to = playlistManagerCreatePlaylist(vp->plm, out, 0);
				int newTotal = playlistExcludeRecordsByFilter(vp, plcD, to, mtag, out);
				my_free(out);

				if (playlistGetTotal(to)){
					playlistAddPlc(getPrimaryPlaylist(vp), to);
					//int plIdx = playlistManagerGetPlaylistIndex(vp->plm, to);
					//vp->displayPlaylist = plIdx;
					playlistChangeEvent(vp, to, 0);
					playlistsForceRefresh(vp, 0);

					dbprintf(vp, "%i tracks extracted to '%s' from '%s'", newTotal, to->title, plcD->title);

				}else /*if !playlistGetTotal(to))*/{
					playlistManagerDeletePlaylist(vp->plm, to, 1);
				}
			}
		}

	// filter everything which is not this (everything but this)
	}else if (!wcscmp(state, L"extract")){
		wchar_t *filter = strGetStringNext(L" ");
		if (filter && *filter){
			int mtag = tagLookupW(filter);

			wchar_t *forthis = strGetStringNext(L"\0");
			if (forthis && *forthis){
				//wprintf(L"filter include #%s# (%i) for #%s#\n", filter, mtag, forthis);

				char *out = convertto8(forthis);
				PLAYLISTCACHE *to = playlistManagerCreatePlaylist(vp->plm, out, 0);
				int newTotal = playlistIncludeRecordsByFilter(vp, plcD, to, mtag, out);
				my_free(out);

				if (to->total){
					//int plIdx = playlistManagerGetPlaylistIndex(vp->plm, to);
					//vp->displayPlaylist = plIdx;
					setDisplayPlaylist(vp, to);
					playlistChangeEvent(vp, to, 0);
					playlistsForceRefresh(vp, 0);

				}else /*if (!to->total)*/{
					playlistManagerDeletePlaylist(vp->plm, to, 1);
				}
				dbprintf(vp, "%i tracks extracted to '%s' from '%s'", newTotal, to->title, plcD->title);
			}
		}

	}else if (!wcscmp(state, L"decompose") || !wcscmp(state, L"decom") || !wcscmp(state, L"split")){
		wchar_t *filter = strGetStringNext(L" ");
		if (filter && *filter){
			int mtag = tagLookupW(filter);
			//wprintf(L"filter split #%s# (%i)\n", filter, mtag);
			int total = playlistBuildSplitPlaylists(vp, plcD, mtag);
			if (total){
				dbwprintf(vp, L"%i playlists created from '%s'", total, filter);
				playlistsForceRefresh(vp, 0);
			}
		}
	}
	//invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), -1);
}

static inline void cmd_volume (wchar_t *var, int vlen, void *uptr, int vtype, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	wchar_t *str = strGetString(var, L" ");
	if (str && *str){
		if (!wcscmp(str, L"on")){
  			setVolume(vp, 50, vtype);
			dbprintf(vp, "Volume on (50%)");

		}else if (!wcscmp(str, L"off") || !wcscmp(str, L"mute")){
			setVolume(vp, 0, vtype);
			dbprintf(vp, "Volume muted");
			
		}else{
			int vol = strGetInt32(str);
			if (vol > 100) vol = 100;
			else if (vol < 0) vol = 0;

			setVolumeDisplay(vp, setVolume(vp, vol, vtype));
			dbprintf(vp, "Volume set to %i%%", vol);
		}
	}else{
		char buffer[128];
		int volume = getVolume(vp, vtype);

		if (vtype == VOLUME_MASTER)
			snprintf(buffer, sizeof(buffer), "#mv %i", volume);
		else if (vtype == VOLUME_APP)
			snprintf(buffer, sizeof(buffer), "#vol %i", volume);
		editboxSetString8(&vp->input, buffer);	
	}
}

#if (LIBVLC_VERSION_MAJOR  < 2)
static inline void cmd_visual (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	wchar_t *state = strGetString(var, L" ");
	if (state && *state){
		if (!wcscmp(state, L"on")){
	  		//buttonCfg(NULL, NULL, CFGBUTTON_VIS_GOOM_Q3-1, 0, vp);
	  		//buttonCfg(NULL, NULL, vp->gui.visual+CFGBUTTON_VIS_DISABLED, 0, vp);
		}else if (!wcscmp(state, L"off")){
			cfgButton(NULL, NULL, CFGBUTTON_VIS_OFF, 0, vp);

		}else{
			int vis = strGetInt32(state);
			if (vis >= 0 && vis < VIS_TOTAL)
				cfgButton(NULL, NULL, CFGBUTTON_VIS_DISABLED+vis-1, 0, vp);
		}
	}
}
#endif

void cmd_snapshot (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	vp->gui.snapshot.save = 1;
	renderSignalUpdate(vp);
}

static inline void cmd_list (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	TEDITBOX *input = &vp->input;
	TEDITBOXCMD *cmd = input->registeredCmds;
	
	wchar_t comment[2] = {CMDPARSER_COMMENT, 0};
	editboxSetString(input, comment);
	editboxAppendString(input, L" ");
	for (int i = 0; i < EDITBOXCMD_MAXCMDS && cmd->state; cmd++, i++){
		editboxAppendString(input, cmd->name);
		if (*cmd->alias){
			editboxAppendString(input, L"(");
			editboxAppendString(input, cmd->alias);
			editboxAppendString(input, L")");
		}
		editboxAppendString(input, L" ");
	}
}

static inline void printImageStats (TVLCPLAYER *vp)
{
	dbprintf(vp, "Images registered: %i", artManagerCount(vp->am)+artManagerCount(vp->im));
	dbprintf(vp, "Images loaded: %i", artManagerSurfaceCount(vp->am)+artManagerSurfaceCount(vp->im));
	dbprintf(vp, "Images locked: %i", artManagerUnreleasedCount(vp->am)+artManagerUnreleasedCount(vp->im));
	dbprintf(vp, "Mem used: %.0fK", (artManagerMemUsage(vp->am)+artManagerMemUsage(vp->im))/1024.0);
	dbprintf(vp, "branchTotal: %i", vp->am->branchTotal+vp->im->branchTotal);
	dbprintf(vp, "hashTableSize: %i/%i", vp->am->hashTableSize, vp->im->hashTableSize);
}

static inline void cmd_gui (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	
	wchar_t *cmd = strGetString(var, L" ");
	if (!wcscmp(cmd, L"overlay")){
		int msperiod = strGetInt32Next();
		if (msperiod > 0){
			vp->gui.mOvrTime = msperiod;
			overlayActivateOverlayResetTimer(vp);
			dbprintf(vp, "Overlay period set to %ims", vp->gui.mOvrTime);
		}else{
			dbprintf(vp, "Overlay period: %ims", vp->gui.mOvrTime);
		}
	}else if (!wcscmp(cmd, L"idle")){
		cmd = strGetStringNext(L" \0");
		if (cmd && (!wcscmp(cmd, L"go") || !wcscmp(cmd, L"set") || !wcscmp(cmd, L"enter"))){
			pageDisable(vp, PAGE_EXIT);
	  		page2Set(vp->pages, PAGE_NONE, 0);
	  	
	  		if (getPlayState(vp)) trackStop(vp);
	  		timerSet(vp, TIMER_SETIDLEA, 300);
	  		renderSignalUpdate(vp);
	  		
	  	}else if (cmd && (!wcscmp(cmd, L"off") || !wcscmp(cmd, L"disabled"))){
	  		vp->gui.idleDisabled = 1;
	  		
		}else if (cmd && (!wcscmp(cmd, L"fps") || !wcscmp(cmd, L"rate"))){
			double fps = strGetFloatNext();
			if (fps >= 0.001){
				vp->gui.idleFPS = fps;
				dbprintf(vp, "Idle update rate set to %.3f fps", vp->gui.idleFPS);
			}else{
				dbprintf(vp, "Idle update rate: %.3f fps", vp->gui.idleFPS);
			}
		}else{
			int period = strGetInt32(cmd);
			if (period > 0){
				vp->gui.idleTime = period*1000;

				dbprintf(vp, "Idle period set to %i seconds", vp->gui.idleTime/1000);
			}else{
				dbprintf(vp, "Idle period: %i seconds", vp->gui.idleTime/1000);
			}
		}
	}else if (!wcscmp(cmd, L"reload")){
		int back = 0;
		wchar_t *all;
		if ((all=strGetStringNext(L"\0"))){
			back = (wcscmp(all, L"bg") == 0);
			if (!back){
				back = strGetInt32(all);
				if (back == 0)
					back = 1;			// set next background
				else if (back > 0)
					vp->gui.skin.currentIdx = back-2;		// set previous background to selected background so it will be the next selected background
			}
		}
		reloadSkin(vp, back);
		
	}else if (!wcscmp(cmd, L"stats")){
		vp->gui.displayStats = msGetStatDisplayState(vp);

		cmd = strGetStringNext(L"\0");
		if (!cmd)
			msSetStatDisplayState(vp, vp->gui.displayStats ^= 1);
		else if (!wcscmp(cmd, L"on"))
			msSetStatDisplayState(vp, 1);
		else if (!wcscmp(cmd, L"off"))
			msSetStatDisplayState(vp, 0);

		dbprintf(vp, "media stats %s", vp->gui.displayStats ? "enabled":"disabled");
			
	}else if (!wcscmp(cmd, L"rate") || !wcscmp(cmd, L"fps")){
		cmd = strGetStringNext(L"\0");
		if (!cmd){
			renderStatsToggle(vp);
			
		}else if (!wcscmp(cmd, L"on")){
			renderStatsEnable(vp);

		}else if (!wcscmp(cmd, L"off")){
			renderStatsDisable(vp);

		}else{
			double fps = strGetFloat(cmd);
			if (fps > 5.0 && fps < 200.0)
				updateTickerStart(vp, fps);
		}
	}else if (!wcscmp(cmd, L"targetrate")){
		double rate = strGetFloatNext();
		if (rate > 1.0 && rate < 200.0){
			setTargetRate(vp, rate);
			dbprintf(vp, "targetRate %.2f\n", rate);
		}
	}else if (!wcscmp(cmd, L"baserate")){
		double rate = strGetFloatNext();
		if (rate > 1.0 && rate < 200.0){
			setBaseUpdateRate(rate);
			dbprintf(vp, "baseUpdateRate %.2f\n", rate);
		}
	}else if (!wcscmp(cmd, L"metatrackbar")){
		vp->gui.drawMetaTrackbar ^= 1;

	}else if (!wcscmp(cmd, L"controlicons")){
		vp->gui.drawControlIcons ^= 1;
		
		TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
		if (!vp->gui.drawControlIcons)
			ccDisable(plyctrl->ctrlpan.base);
		else
			ccEnable(plyctrl->ctrlpan.base);
			
	}else if (!wcscmp(cmd, L"imflush") || !wcscmp(cmd, L"amflush")){
		int ct = imageManagerFlush(vp->im);
		ct += imageManagerFlush(vp->am);
		dbprintf(vp, "Images flushed: %i", ct);
		invalidateShadows(vp->gui.shadow);
		ccLabelFlushAll(vp->cc);
		dbprintf(vp, "Glyphs flushed: %i", libmylcd_FlushFonts(vp->ml->hw));
		
		
	}else if (!wcscmp(cmd, L"amstats") || !wcscmp(cmd, L"imstats")){
		printImageStats(vp);

		// probably shouldn't do this..
		int ct = 0;
		TCCOBJ *obj = vp->cc->objs;
		while((obj=obj->next)) if (obj->obj) ct++;
		
		dbprintf(vp, "Widget objects: table size:%i, built:%i", vp->cc->ccIdIdx, ct);
	}
}

static inline void cmd_time (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	if (var && *var){
		const int state = getPlayState(vp);
		if (state && state != 8){		// is playing but not at the end
			double tlen = vp->vlc->length;
			double tpos = (double)stringToTimeW(var, vlen);
			double pos = 1.0/(tlen/tpos);
			clipFloat(pos);
			vp->vlc->position = pos;
			vlc_setPosition(vp->vlc, vp->vlc->position);
		}
	}
}

static inline void cmd_timeJump (wchar_t *var, int vlen, void *uptr, int direction, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	if (var && *var){
		if (getPlayState(vp) && getPlayState(vp) /*!= 8*/){
			const double tskip = strGetFloat(var);
			if (tskip > 0.00000){
				double pos = 0.0;
				double tlen = vp->vlc->length;
				double dt = (1.0/tlen) * tskip;
				
				if (direction == TIMRSKIP_FORWARD)
					pos = vp->vlc->position + dt;
				else if (direction == TIMRSKIP_BACK)
					pos = vp->vlc->position - dt;

				clipFloat(pos);
				vp->vlc->position = pos;
				vlc_setPosition(vp->vlc, vp->vlc->position);
			}
		}
	}
}

/*
void dumpPlaylists (TVLCPLAYER *vp, TPLAYLISTMANAGER *plm)
{
	if (playlistManagerLock(plm)){
		PLAYLISTCACHE *plc;
		const int total = playlistManagerGetTotal(plm);
		
		for (int i = 0; i < total; i++){
			plc = playlistManagerGetPlaylist(plm, i);
			if (plc){
				if (playlistLock(plc)){
					dbprintf(vp, "%i (%i) '%s'\n", i, plc->total, plc->title);
					playlistUnlock(plc);
				}
			}
		}
		playlistManagerUnlock(plm);
	}
}
*/


static inline void cmd_plm (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	TPLAYLISTMANAGER *plm = vp->plm;

	if (!var || !vlen) return;

	const int nodup = (wcsistr(var, L"nodup") != NULL);
	wchar_t *state = strGetString(var, L" ");
	if (!state) return;

	if (!wcscmp(state, L"new") || !wcscmp(state, L"create")){
		var = strGetStringNext(L" ");
		if (!var) return;
		
		int parentId = hexToIntW(var);
		if (parentId <= PLAYLIST_UID_BASE)
			parentId = PLAYLIST_UID_BASE+1;
		
		char name[MAX_PATH_UTF8+1];
		var = strGetStringNext(L"\0");
		if (var && *var){
			if (!UTF16ToUTF8(var, wcslen(var), name, MAX_PATH_UTF8))
				strcpy(name, "untitled");
		}else{
			return;
		}
		
		const int uid = playlistManagerCreatePlaylistUID(plm, name, 0);
		if (uid){
			int pos = playlistManagerAddPlaylistUID(plm, parentId, uid);
			if (pos >= 0){
				dbprintf(vp, "Playlist '%s' (%X) created in %X", name, uid, parentId);
				playlistsForceRefresh(vp, 0);
			}else{
				dbprintf(vp, "Playlist %X not found", parentId);
			}
		}
		
	}else if (!wcscmp(state, L"setpaths") || !wcscmp(state, L"sp")){
		var = strGetStringNext(L" ");
		if (!var) return;
		
		const int uid = hexToIntW(var);
		if (uid <= PLAYLIST_UID_BASE+1){
			dbprintf(vp, "Invalid playlist Id");
			return;
		}
			
		char path[MAX_PATH_UTF8+2];
		var = strGetStringNext(L"\0");
		
		if (var && *var){
			if (UTF16ToUTF8(var, wcslen(var), path, MAX_PATH_UTF8)){
				int ret = playlistSetTrackPaths(plm, uid, path);
				if (ret == 0)
					dbprintf(vp, "Nothing changed in %X", uid);
				else if (ret > 0)
					dbprintf(vp, "%i tracks changed in %X", ret, uid);
				else if (ret == -1)
					dbprintf(vp, "Path entered is not valid");
				else if (ret == -2)
					dbprintf(vp, "Playlist not found (%X)", uid);
				return;
			}
		}
		dbprintf(vp, "Path not entered");

	}else if (!wcscmp(state, L"load") || !wcscmp(state, L"ld") || !wcscmp(state, L"import")){
		var = strGetStringNext(L" ");
		if (!var) return;
		
		int uid = hexToIntW(var);
		if (uid == 0) uid = PLAYLIST_UID_BASE+1;
		
		if (uid > PLAYLIST_UID_BASE){
			var = strGetStringNext(L"\0");
			if (var && *var){
				wchar_t *path = removeLeadingSpacesW(var);
				if (path){
					removeTrailingSpacesW(path);
					if (wcslen(path)){
						int total = 0;

						if (isDirectoryW(path))
							//browserImportPlaylistByDirUIDW(vp, path, uid, 1, &total);
							filepaneImportPlaylistByDirUID(pageGetPtr(vp, PAGE_FILE_PANE), path, uid, 1);
							
						else if (isPlaylistW(path) && doesFileExistW(path))
							total = importPlaylistUIDW(plm, vp->tagc, vp->am, path, uid, pageGetPtr(vp, PAGE_FILE_PANE));
						else
							dbwprintf(vp, L"'%s' is not a valid path or playlist", path);

						if (total > 0){
							dbprintf(vp, "%i items imported in to %X", total, uid);
							playlistsForceRefresh(vp, 0);
						}else{
							dbprintf(vp, "Import failed");
						}
					}
				}
			}
		}
	}else if (!wcscmp(state, L"save") || !wcscmp(state, L"write")){
		var = strGetStringNext(L" ");
		if (!var) return;
		
		int uid = hexToIntW(var);
		if (uid == 0) uid = PLAYLIST_UID_BASE+1;

		if (uid > PLAYLIST_UID_BASE){
			var = strGetStringNext(L"\0");
			if (var && *var){
				wchar_t *name = removeLeadingSpacesW(var);
				if (name){
					removeTrailingSpacesW(name);
					const int len = wcslen(name);
					if (len)
						savePlaylistUID(vp, uid, name);
				}
			}
		}
	}else if (!wcscmp(state, L"copy") || !wcscmp(state, L"cp")){
		int from, to;
		
		if (hexToInt2W(strGetStringNext(L"\0"), &from, &to)){
			if (from <= PLAYLIST_UID_BASE) from = PLAYLIST_UID_BASE+1;
			if (to <= PLAYLIST_UID_BASE) to = PLAYLIST_UID_BASE+1;
			
			if (from > PLAYLIST_UID_BASE && to > PLAYLIST_UID_BASE){
				int tCopied = copyPlaylist(plm, from, to, nodup);
				if (tCopied > 0){
					dbprintf(vp, "%i items from playlist %X copied in to %X", tCopied, from, to);
					playlistsForceRefresh(vp, 0);
					
				}else{
					if (from == to)
						dbprintf(vp, "Can not copy to self (%X) as bad things will happen", from);
					else if (nodup)
						dbprintf(vp, "Nothing to copy or duplicates found (%X to %X)", from, to);
					else
						dbprintf(vp, "Invalid playlist(s) (%X to %X)", from, to);
				}
			}
		}		
	}else if (!wcscmp(state, L"move") || !wcscmp(state, L"mv")){
		int from, to;

		if (hexToInt2W(strGetStringNext(L"\0"), &from, &to)){
			if (from <= PLAYLIST_UID_BASE) from = PLAYLIST_UID_BASE+1;
			if (to <= PLAYLIST_UID_BASE) to = PLAYLIST_UID_BASE+1;
			
			if (from > PLAYLIST_UID_BASE && to > PLAYLIST_UID_BASE){
				int ret = playlistManagerMovePlaylistTo(plm, from, to);
				if (ret){
					dbprintf(vp, "Playlist %X moved to %X", from, to);
					playlistsForceRefresh(vp, 0);
					
				}else{
					if (from == to)
						dbprintf(vp, "Can not move to self (%X)", from);
					else
						dbprintf(vp, "Invalid playlist(s) (%X %X)", from, to);
				}
			}
		}
	}else if (!wcscmp(state, L"movein") || !wcscmp(state, L"mi") || !wcscmp(state, L"mvin")){
		int from, to;

		if (hexToInt2W(strGetStringNext(L"\0"), &from, &to)){
			if (from <= PLAYLIST_UID_BASE) from = PLAYLIST_UID_BASE+1;
			if (to <= PLAYLIST_UID_BASE) to = PLAYLIST_UID_BASE+1;
			
			if (from > PLAYLIST_UID_BASE && to > PLAYLIST_UID_BASE){
				int ret = playlistManagerMovePlaylistInto(plm, from, to);
				if (ret){
					dbprintf(vp, "Playlist %X moved in to %X", from, to);
					playlistsForceRefresh(vp, 0);
				}else{
					if (from == to)
						dbprintf(vp, "Can not move to self (%X)", from);
					else
						dbprintf(vp, "Invalid playlist(s) (%X %X)", from, to);
				}				
			}
		}
	}else if (!wcscmp(state, L"delete") || !wcscmp(state, L"del")){
		var = strGetStringNext(L" ");
		if (!var) return;

		int uid = hexToIntW(var);
		if (uid > PLAYLIST_UID_BASE+1){
			
			PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(plm, uid);
			if (plc && plc->parent){
				int pos = playlistGetPlaylistIndex(plc->parent, plc);
				if (pos >= 0){
					playlistManagerDeletePlaylistByUID(plm, uid, 0);
					playlistDeleteRecord(plc->parent, pos);
					
					dbprintf(vp, "Playlist %X removed", uid);
					playlistsForceRefresh(vp, 0);
					return;
				}
			}
		}
		if (uid == PLAYLIST_UID_BASE+1)
			dbprintf(vp, "Playlist %X may not be removed", uid);
		else
			dbprintf(vp, "Playlist %X is invalid or can not be removed", uid);
		
	}else if (!wcscmp(state, L"getid") || !wcscmp(state, L"id")){
		PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		const int pos = spl->from;
		const int page = pageGet(vp);
		
		if (page == PAGE_PLY_SHELF){
			if (playlistGetItemType(plc, pos) == PLAYLIST_OBJTYPE_PLC)
				dbprintf(vp, "Viewing %X at position %i (is a playlist with ID: %X)", plc->uid, pos+1, playlistGetPlaylistUID(plc, pos));
			else
				dbprintf(vp, "Viewing %X at position %i", plc->uid, pos+1);

		}else if (page == PAGE_PLY_FLAT){
			dbprintf(vp, "Looking at %X", plc->uid);
			
		}else if (page == PAGE_PLY_PANEL){
			TPLYPANEL *plypan = pageGetPtr(vp, PAGE_PLY_PANEL);
			dbprintf(vp, "Viewing %X", plypan->currentPlcUID);
			
		}else if (page == PAGE_OVERLAY){
			dbprintf(vp, "Queued %X", plc->uid);
		}else{
			dbprintf(vp, "Viewing %X", plc->uid);

		}
	}else{
		int id  = hexToIntW(var);
		if (id < PLAYLIST_UID_BASE) id = PLAYLIST_UID_BASE+1;
		
		setDisplayPlaylistByUID(vp, id);
		id = getDisplayPlaylistUID(vp);
		plyPanSetCurrentUID(vp, id);

		timerSet(vp, TIMER_PLYPAN_REBUILD, 0);
		timerSet(vp, TIMER_PLYALB_REFRESH, 0);
		timerSet(vp, TIMER_PLYPANE_REFRESH, 0);
		
		invalidateShelf(vp, pageGetPtr(vp, PAGE_PLY_FLAT), playlistManagerGetIndexByUID(plm, id));
		// todo: jump to and open plytv item where tree branch id == uid

		if (!page2RenderGetState(vp->pages, PAGE_PLY_SHELF) && pageGet(vp) != PAGE_PLY_FLAT && pageGet(vp) != PAGE_PLY_PANEL)
			page2Set(vp->pages, PAGE_PLY_SHELF, 1);
	}
}

static inline void cmd_getlengths (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	if (mHookGetState()){
		captureMouse(vp, 0);
		mHookUninstall();
	}
	//vlcEventFreeHandles(vp, 0);
	
	char buffer[32];
	char path[MAX_PATH_UTF8+1];
	int scanCt = 0;

	const int ptotal = playlistManagerGetTotal(vp->plm);
	for (int i = 0; i < ptotal && vp->applState; i++){
		PLAYLISTCACHE *plcD = playlistManagerGetPlaylist(vp->plm, i);
		int total = playlistGetTotal(plcD);
		if (!total) continue;
		
		int trk = 0;
		int trklen = 0;
		
		//dbprintf(vp, "processing: %i of %i: '%s'", i, ptotal, plcD->title);
		
		while(trk < total && vp->applState){
			if (playlistGetItemType(plcD, trk) == PLAYLIST_OBJTYPE_PLC){
				trk++;
				continue;
			}

			plcD->pr->selectedItem = trk;
			unsigned int hash = playlistGetHash(plcD, trk);
			if (hash){
				trklen = 0;
				tagRetrieveByHash(vp->tagc, hash, MTAG_LENGTH, buffer, sizeof(buffer)-1);
				if (*buffer)
					trklen = (int)stringToTime(buffer, sizeof(buffer)-1);
	
				if (trklen <= 0){
					playlistGetPath(plcD, trk, path, MAX_PATH_UTF8);
					if (*path){
						vlcEventGetLength(vp->vlc->hLibTmp, vp, vp->tagc, path, hash, NULL);
						dbprintf(vp, "track %i:%i", i, trk+1);
						scanCt++;
					}
				}
			}
			trk++;
		}
	}
	dbprintf(vp, "%i tracks scanned", scanCt);

}

// render lock must be held before entering
int artworkFlush (TVLCPLAYER *vp, TARTMANAGER *am)
{

	if (hasPageBeenAccessed(vp, PAGE_PLY_SHELF)){
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		shelfFlush(spl->shelf);
		invalidateShelfAlbum(vp, spl, spl->from);
	}
	if (hasPageBeenAccessed(vp, PAGE_PLY_FLAT)){
		TSPL *spl = pageGetPtr(vp, PAGE_PLY_FLAT);
		shelfFlush(spl->shelf);
		invalidateShelf(vp, spl, spl->from);
	}
	
	if (playlistManagerLock(vp->plm)){
		const int pltotal = playlistManagerGetTotal(vp->plm);
		for (int i = 0; i < pltotal; i++){
			PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, i);
			if (plc){
				if (playlistLock(plc)){
					//plc->artId = 0;
					plc->artRetryTime = 0;
						
					for (int j = 0; j < plc->total; j++){
						TPLAYLISTITEM *item = g_playlistGetItem(plc, j);
						if (item){
							item->metaRetryCt = 0;
							item->artRetryCt = 0;
						}
					}
					playlistUnlock(plc);
				}
			}
		}
		playlistManagerUnlock(vp->plm);
	}
	
	return artManagerFlush(am);
}	
/*
static inline void cmd_flushArt (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	if (renderLock(vp)){
		artworkFlush(vp, vp->am);
		//playlistResetRetries(vp);
		renderUnlock(vp);
	}
	//countItems(vp, vp->metac);
}

static inline void cmd_flushMeta (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;

	if (renderLock(vp)){
		tagFlush(vp->tagc);
		playlistResetRetries(vp);
		renderUnlock(vp);
	}
	//countItems(vp, vp->metac);
}
*/
void printAbout (TVLCPLAYER *vp)
{
	char buffer[256] = {0};
	dbprintf(vp, " VlcStream, by %s", mySELF);
	dbprintf(vp, " web: mylcd.sourceforge.net");
	dbprintf(vp, " email: okio@users.sourceforge.net");
	dbprintf(vp, " %s-%s", PLAYER_VERSION, libmylcdVERSION);
	dbprintf(vp, " libvlc v%s", libvlc_get_version());
	//dbprintf(vp, " Compiler: %s", libvlc_get_compiler());
	dbprintf(vp, " Compiler: gcc version %i.%i (GCC)", __GNUC__, __GNUC_MINOR__);
#ifdef DISABLE_ME_SBUISDK_VERSION
	dbprintf(vp, " SBUI SDK v%s", SBUISDK_VERSION);
#endif
	
	int len = 0;
	
#ifdef USE_MMX 
	strncat(buffer, "MMX", sizeof(buffer)-1);
	len += 3;
#endif
#ifdef USE_MMX2 
	if (len)
		strncat(buffer, " ", sizeof(buffer)-1);
	strncat(buffer, "MMXext", sizeof(buffer)-1);
	len += 7;
#endif
#ifdef USE_SSE
	if (len)
		strncat(buffer, " ", sizeof(buffer)-1);
	strncat(buffer, "SSE", sizeof(buffer)-1);
	len += 3;
#endif
#ifdef USE_SSE2
	if (len)
		strncat(buffer, " ", sizeof(buffer)-1);
	strncat(buffer, "SSE2", sizeof(buffer)-1);
	len += 4;
#endif
#ifdef USE_3DNOW
	if (len)
		strncat(buffer, " ", sizeof(buffer)-1);
	strncat(buffer, "3DNOW", sizeof(buffer)-1);
	len += 5;
#endif

	if (len) dbprintf(vp, " Enabled: %s", buffer);
		
	dbprintf(vp, " CPU supports: %s", cpu_getCapabilityString(buffer, sizeof(buffer)-1));	
	dbprintf(vp, " &#169; %s", mySELF);
}

static inline void cmd_about (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	printAbout(vp);
}

static inline void cmd_trackpad (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	
	wchar_t *state = strGetString(var, L" ");
	if (state && *state){
		if (!wcscmp(state, L"on") || !wcsicmp(state, L"app"))
			setPadControl(vp, BTN_CFG_PADCTRL_ON);
		else if (!wcscmp(state, L"off") || !wcsicmp(state, L"os"))
			setPadControl(vp, BTN_CFG_PADCTRL_OFF);
	}
}

static inline void cmd_keypad (wchar_t *var, int vlen, void *uptr, int unused1, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	
	TKEYBOARD *vkey = pageGetPtr(vp, PAGE_VKEYBOARD);
	TKEYPAD *kp = vkey->kp;
	keypadListenerRemoveAll(kp);
	keypadEditboxSetUserData(&kp->editbox, 0);	// if nobody is listening then there shouldn't be a cc Id
	ccEnable(kp);
	pageSet(vp, PAGE_VKEYBOARD);
}

#if ENABLE_CMDFUNSTUFF
static inline void cmd_bots (wchar_t *var, int vlen, void *uptr, const int bot, int unused2)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	botQuoteRandom(vp, vp->bot.sheets, bot);
}
#endif


void editboxDoCmdRegistration (TEDITBOX *input, void *vp)
{
	editBoxRegisterCmdFunc(input, L"help,info",		cmd_help, vp, 0, L"#help cmd");
	//editBoxRegisterCmdFunc(input, L"flushart,fa",	cmd_flushArt, vp, 0, L"Dump all artwork image(s) from memory, reacquire new MRL then load if required");
	//editBoxRegisterCmdFunc(input, L"flushmeta,fm",	cmd_flushMeta, vp, 0, L"Clear all playlist item metadata (unwise) then reacquire when requested");
	editBoxRegisterCmdFunc(input, L"getlengths,gl",	cmd_getlengths, vp, 0, L"Get play length (time) of any zero length track from current playlist (Album playlist page) location");
	editBoxRegisterCmdFunc(input, L"plm",			cmd_plm, vp, 0, L"Jump to playlist manager page, which is a flat playlist view");
	editBoxRegisterCmdFunc(input, L"list",			cmd_list, vp, 0, L"Display available commands");
	editBoxRegisterCmdFunc(input, L"playing",		cmd_track, vp, 1, L"Jump to whichever playlist item is playing (Album playlist page)");		// 1:go to playing
	editBoxRegisterCmdFunc(input, L"selected",		cmd_track, vp, 2, L"Jump to whichever playlist item is selected (Album playlist page)");		// 2:go to selected
	editBoxRegisterCmdFunc(input, L"back",			cmd_back, vp, 0, L"Return to previously displayed page");
	editBoxRegisterCmdFunc(input, L"fastforward,ff",cmd_timeJump, vp, TIMRSKIP_FORWARD, L"Fast forward by n seconds");
	editBoxRegisterCmdFunc(input, L"rewind,rw",		cmd_timeJump, vp, TIMRSKIP_BACK, L"Go back by n seconds");
	editBoxRegisterCmdFunc(input, L"time",			cmd_time, vp, 0, L"Jump to n seconds of the playback time");
	editBoxRegisterCmdFunc(input, L"snapshot,ss",	cmd_snapshot, vp, 1, L"Save a .PNG image of the framebuffer minus command prompt");
	editBoxRegisterCmdFunc(input, L"pause",			cmd_mctrl, vp, VBUTTON_PAUSE, L"Page playback");
	editBoxRegisterCmdFunc(input, L"load,ld",		cmd_load, vp, 1, L"Import directory/track in to selected playlist location (eg; #load c:\\location\\ or c:\\music\\track.mp3)");
	editBoxRegisterCmdFunc(input, L"import",		cmd_import, vp, 0, L"Import a directory (eg; c:\\location)");
	editBoxRegisterCmdFunc(input, L"swaprgb,rgb",	cmd_rgbswap, vp, 0, L"Swap around red (0xFF0000) and blue (0x0000FF) colour components of video playback");
	editBoxRegisterCmdFunc(input, L"playlist,pl",	cmd_playlist, vp, 0, L"General playlist control. See readme.txt for details");
	editBoxRegisterCmdFunc(input, L"pc",			cmd_nc, vp, 1, L"Jump to previous chapter");	// previous chapter
	editBoxRegisterCmdFunc(input, L"nc",			cmd_nc, vp, 2, L"Go to next chapter");	// next chapter
	editBoxRegisterCmdFunc(input, L"pt",			cmd_nt, vp, 1, L"Jump to previous title");	// previous title
	editBoxRegisterCmdFunc(input, L"nt",			cmd_nt, vp, 2, L"Go to next title");	// next title
	


	
	editBoxRegisterCmdFunc(input, L"lock",			cmd_systemCmds, vp, SYSTEMCMDS_Lock, L"Lock the computer. Identical to WindowsKey+L");
	editBoxRegisterCmdFunc(input, L"reboot",		cmd_systemCmds, vp, SYSTEMCMDS_Reboot, L"Reboots workstation");
	editBoxRegisterCmdFunc(input, L"logoff",		cmd_systemCmds, vp, SYSTEMCMDS_Logoff, L"Logoff current user");
	editBoxRegisterCmdFunc(input, L"shutdown",		cmd_systemCmds, vp, SYSTEMCMDS_Shutdown, L"Power off workstation");
	editBoxRegisterCmdFunc(input, L"abortshutdown,shutdownabort", cmd_systemCmds, vp, SYSTEMCMDS_ShutdownAbort, L"Abort current shutdown procedure");
	//editBoxRegisterCmdFunc(input, L"restartexplorer",cmd_systemCmds, vp, SYSTEMCMDS_ExplorerRestart, L" ");	
	
	editBoxRegisterCmdFunc(input, L"volume,vol",	cmd_volume, vp, VOLUME_APP, L"Set playback volume (#vol 72)");
	editBoxRegisterCmdFunc(input, L"mvolume,mv",	cmd_volume, vp, VOLUME_MASTER, L"Set system/master volume (#mv 83) 0-100, 0 = mute. (Windows Vista and later)");
	editBoxRegisterCmdFunc(input, L"getmeta,gm",	cmd_getMeta, vp, 0, L"Manually initiate metatag retrieval for playlist contents, but is generally not required. Excludes length");
	editBoxRegisterCmdFunc(input, L"search,find",	cmd_search, vp, 0, L"Metatag data search. eg; '#find album under rug' will search for any track from current playlist containing 'under rug' anywhere in the album tag.\n(Tags: Title, Artist, Genre, Copyright, Album, Track, Description, Rating, Date, Setting, URL, Language, NowPlaying, Publisher, EncodedBy, artPath, TrackID, Length, Filename, Playlist and Path)");
	editBoxRegisterCmdFunc(input, L"aspect,ar",		cmd_aspectRatio, vp, 0, L"Manually set aspect ratio of video playback. Eg; #aspect 16:9, or, #aspect 1.77\n(auto, 16:9/1.77, 14:9/1.55, 4:3/1.33, 5:4/1.25, 22:18/1.22, 3:2/1.5, 16:10/1.6, 43:30/1.43, 5:3/1.66, 3:7/1.85, 11:5/2.2, 47:20/21:9/2.33/2.35, 12:5/2.39/2.40");
	editBoxRegisterCmdFunc(input, L"mouse,cursor",	cmd_mouse, vp, 0, L"Toggle mouse/cursor control");
	editBoxRegisterCmdFunc(input, L"close",			cmd_closeEditbox, vp, 0, L"Close input prompt (or press Escape)");
	editBoxRegisterCmdFunc(input, L"quit,exit",		cmd_shutdown, vp, 0, L"(o o)\n        (-)");
	editBoxRegisterCmdFunc(input, L"sorta,sa",		cmd_sort, vp, SORT_ASCENDING, L"Sort playlist in to ascending order by metatag data; Eg; #sorta year, #sorta path\n(Tags: Title, Artist, Genre, Copyright, Album, Track, Description, Rating, Date, Setting, URL, Language, NowPlaying, Publisher, EncodedBy, artPath, TrackID, Length, Filename, Playlist and Path)");
	editBoxRegisterCmdFunc(input, L"sortd,sd",		cmd_sort, vp, SORT_DESCENDING, L"Sort playlist in to descending order by metatag data; Eg; #sorta year, #sorta path\n(Tags: Title, Artist, Genre, Copyright, Album, Track, Description, Rating, Date, Setting, URL, Language, NowPlaying, Publisher, EncodedBy, artPath, TrackID, Length, Filename, Playlist and Path)");
	editBoxRegisterCmdFunc(input, L"play",			cmd_mctrl, vp, VBUTTON_PLAY, L"Start playback or play selected track. Eg; #play, #play 6");
	editBoxRegisterCmdFunc(input, L"stop",			cmd_mctrl, vp, VBUTTON_STOP, L"Stop playback");
	editBoxRegisterCmdFunc(input, L"meta",			cmd_meta, vp, 0, L"Go to metadata page. Displays whatever tags are found");
	editBoxRegisterCmdFunc(input, L"home",			cmd_home, vp, 0, L"Go home.");
	editBoxRegisterCmdFunc(input, L"scale",			cmd_scale, vp, 0, L"Set zoom level of video playback (#zoom 1.2)");
	editBoxRegisterCmdFunc(input, L"prev",			cmd_mctrl, vp, VBUTTON_PRETRACK, L"Play previous track (in current playlist)");
	editBoxRegisterCmdFunc(input, L"next",			cmd_mctrl, vp, VBUTTON_NEXTTRACK, L"Go to next track");
	editBoxRegisterCmdFunc(input, L"open",			cmd_open, vp, 0, L"Import directory/track in to selected playlist location and initiate playback (eg; #load c:\\location\\ or c:\\music\\track.mp3)");
	editBoxRegisterCmdFunc(input, L"clock",			cmd_clock, vp, 0, L"Display time");
	editBoxRegisterCmdFunc(input, L"eq",			cmd_eq, vp, 0, L"Go to EQ page");
	editBoxRegisterCmdFunc(input, L"first",			cmd_firstlast, vp, 0, L"In Album View, scroll to first item");
	editBoxRegisterCmdFunc(input, L"last",			cmd_firstlast, vp, 1, L"In Album View, scroll to last item");
	editBoxRegisterCmdFunc(input, L"config,cfg",	cmd_config, vp, 0, L"Go to config page");
	editBoxRegisterCmdFunc(input, L"stream,es",		cmd_escodec, vp, 0, L"Go to Elementry Stream(s) description page");
	editBoxRegisterCmdFunc(input, L"dvb,epg",		cmd_dvb, vp, 0, L"View electronic programme guide where available (DVB-c/s/t)");
	editBoxRegisterCmdFunc(input, L"chapter",		cmd_chapter, vp, 0, L"Go to Chapter selection page");
	editBoxRegisterCmdFunc(input, L"title",			cmd_title, vp, 0, L"Go to Title playback selection page");
	editBoxRegisterCmdFunc(input, L"random",		cmd_random, vp, 0, L"Set random playback state (On/Off or 1/0)");
	editBoxRegisterCmdFunc(input, L"about",			cmd_about, vp, 0, L"Display general program information");
	editBoxRegisterCmdFunc(input, L"subtitle,subs",	cmd_subtitle, vp, 0, L"Go to Subtitle selection page");
	editBoxRegisterCmdFunc(input, L"shelf,alb",		cmd_shelf, vp, 0, L"Go to album page");
	editBoxRegisterCmdFunc(input, L"keypad,kp",		cmd_keypad, vp, 0, L"Display keypad (is used when renaming items)");
	editBoxRegisterCmdFunc(input, L"gui,ui",		cmd_gui, vp, 0, L"overlay n mseconds, idle n seconds, idle set, idle fps n, reload, stats on/off, fps n seconds, fps on/off, targetrate n, baserate n, metatrackbar, controlicons");
	editBoxRegisterCmdFunc(input, L"trackpad,tp",	cmd_trackpad, vp, 0, L"Decide who has trackpad focus; This appl. or the OS. (#trackpad app/os)");
	editBoxRegisterCmdFunc(input, L"resync",		cmd_resync, vp, 0, L"Reestablish connection to the Switchblade device (may have to type this blind..)");
	editBoxRegisterCmdFunc(input, L"dvdnav,dvd",	cmd_dvdnav, vp, 0, L"Navigate through dvd menus. (#dvd up/down/left/right)");

#if ENABLE_BRIGHTNESS
	editBoxRegisterCmdFunc(input, L"backlight,bl",	cmd_usbd480Backlight, vp, 0, L"0-100");
#endif

	editBoxRegisterCmdFunc(input, L"rotate",		cmd_rotate, vp, 0, L"Rotate playing video image by n degress (#rotate 13.4)");

#if (LIBVLC_VERSION_MAJOR < 2)
	editBoxRegisterCmdFunc(input, L"visual,vis",	cmd_visual, vp, 0, L"");
#endif

#if ENABLE_CMDFUNSTUFF
	editBoxRegisterCmdFunc(input, L"bofh",			cmd_bots, vp, BOT_BOFH, L"Quotes from the b'tard operator from hell");
	editBoxRegisterCmdFunc(input, L"dubya",			cmd_bots, vp, BOT_DUBYA, L"Actual quotes from you know who :) while still a govener");
	editBoxRegisterCmdFunc(input, L"fact",			cmd_bots, vp, BOT_FACTS, L"Display a random fact");
	editBoxRegisterCmdFunc(input, L"morbid",		cmd_bots, vp, BOT_MORBID, L"Prints a random but morbid fact");
#endif

}
