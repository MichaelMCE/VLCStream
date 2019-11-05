
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


// playlsit support utils


#define RETRYCOUNT_ART	(5)
#define RETRYCOUNT_META (5)




static inline int writePlaylists (TVLCPLAYER *vp, TPLAYLISTMANAGER *plm, wchar_t *buffer, size_t len)
{
	int ret = 0;

	if (playlistManagerGetTotal(plm) > 0){
		wchar_t buffertime[64];
		time_t t = time(0);
		struct tm *tdate = localtime(&t);

		if (wcsftime(buffertime, sizeof(buffertime), L"%d%m%y-%H%M%S", tdate) > 1){
			if (__mingw_snwprintf(buffer, len, L"vlcs-%ls.m3u8", buffertime) > 1){
				TM3U *m3u = m3uNew();
				if (m3u){
					if (m3uOpen(m3u, buffer, M3U_OPENWRITE)){
						ret = m3uWritePlaylists(m3u, plm, vp->tagc, vp->am);
						m3uClose(m3u);
					}
					m3uFree(m3u);
				}
			}
		}
	}
	return ret;
}

static inline unsigned int __stdcall writePlaylistsCB (void *dwUser)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)dwUser;
	wchar_t buffer[MAX_PATH+1];
	
	dbwprintf(vp, L"Writting playlist...");
	int totalWritten = writePlaylists(vp, vp->plm, buffer, sizeof(buffer)-1);
	if (totalWritten)
		dbwprintf(vp, L"%i records written to %s", totalWritten, buffer);
	else
		dbwprintf(vp, L"Playlist write failed [%s]", buffer);
	
	_endthreadex(1);
	return 1;
}

void savePlaylistAsync (TVLCPLAYER *vp)
{
	unsigned int threadId;
	_beginthreadex(NULL, THREADSTACKSIZE, writePlaylistsCB, vp, 0, &threadId);
}

int playlistGetTrackLengths (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int recursive, const int log)
{
	
	const int total = playlistGetTotal(plc);
	//printf("playlistGetTrackLengths %i, '%s'\n", total, plc->title);
	if (!total) return 0;

	char buffer[32];
	char path[MAX_PATH_UTF8+1];
	int scanCt = 0;
	int trk = -1;
	int trklen = 0;

		
	while(++trk < total && vp->applState){
		if (playlistGetItemType(plc, trk) == PLAYLIST_OBJTYPE_PLC){
			if (recursive){
				TPLAYLISTITEM *item = playlistGetItem(plc, trk);
				if (item)
					scanCt += playlistGetTrackLengths(vp, item->obj.plc, recursive, log);
			}
			continue;
		}

		plc->pr->selectedItem = trk;
		unsigned int hash = playlistGetHash(plc, trk);
		if (hash){
			trklen = 0;
			tagRetrieveByHash(vp->tagc, hash, MTAG_LENGTH, buffer, sizeof(buffer)-1);
			if (*buffer)
				trklen = (int)stringToTime(buffer, sizeof(buffer)-1);
	
			if (trklen <= 0){
				playlistGetPath(plc, trk, path, MAX_PATH_UTF8);
				if (*path && !isMediaDVB(path)){
					vlcEventGetLength(vp->vlc->hLibTmp, vp, vp->tagc, path, hash, NULL);
					//if (log) dbprintf(vp, "track %i:%i", scanCt, trk+1);
					scanCt++;
				}
			}
		}
		//trk++;
	}
	
	if (log && scanCt) dbprintf(vp, "%X, %i tracks scanned", plc->uid, scanCt);
	return scanCt;
}


/*
void vlcEventGetArt (job_controller *jc, TARTMANAGER *am, const char *path, const TMETACOMPLETIONCB *mccb, const unsigned int uid, const int position)
{
	//printf("vlcEventGetArt %X:'%s'\n", hash, path);
}
*/


int playlistMetaGetTrackMetaByHash (TVLCPLAYER *vp, const unsigned int hash, PLAYLISTCACHE *plc, const char *path, const int position, TMETACOMPLETIONCB *mccb)
{
	
	int ret = 0;
	int isTitleReady = 0;
	//int isArtReady = 0;
	TPLAYLISTITEM *item = playlistGetItem(plc, position);

	if (tagLock(vp->tagc)){
		TMETAITEM *tagItem = g_tagFindEntryByHash(vp->tagc, hash);
		if (tagItem){
			if (item->artRetryCt > RETRYCOUNT_ART){
				//isArtReady = 1;
			}else{
				item->artRetryCt++;
				//isArtReady = playlistGetArtId(plc, position) != 0;
			}
			isTitleReady = tagItem->isParsed;
		}
		tagUnlock(vp->tagc);
	}

	if ((!isTitleReady && item->metaRetryCt <= RETRYCOUNT_META) || item->metaRetryCt == -1){
		vlcEventGetMeta(vp->vlc->hLib, vp, vp->tagc, path, hash, mccb, plc->uid, position);
		item->metaRetryCt++;
		ret = 1;
	}

#if 0
	if (!isArtReady && isAudioFile8(path)){
		const int uid = playlistManagerGetPlaylistUID(vp->plm, plc);
		vlcEventGetArt(vp->jc, vp->am, path, mccb, uid, position);
	}
#endif
	return ret;
}

int playlistMetaGetTrackMeta (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const char *path, const int position, TMETACOMPLETIONCB *mccb)
{
	if (!*path) return -1;
	return playlistMetaGetTrackMetaByHash(vp, getHash(path), plc, path, position, mccb);
}

int playlistMetaGetMeta (TVLCPLAYER *vp, PLAYLISTCACHE *plc, int from, int to, TMETACOMPLETIONCB *mccb)
{
	
	int ret = 0;
	char fname[MAX_PATH_UTF8+1];
	char ext[MAX_PATH_UTF8+1];
	char buffer[MAX_PATH_UTF8+1];
	char path[MAX_PATH_UTF8+1];
	const char *extTags[] = {EXTAUDIOA, EXTVIDEOA, ""};
	
	from = MAX(0, from);
	to = MIN(playlistGetTotal(plc)-1, to);
	//printf("playlistMetaGetMeta %X %i %i\n", plc->uid, from, to);
	
	for (int i = from; i <= to; i++){
		playlistGetPath(plc, i, path, MAX_PATH_UTF8);
		if (!*path || isMediaDVB(path)) continue;

		switch(hasPathExtA(path, extTags)){
		  case 1:
		  	if ((ret=playlistMetaGetTrackMeta(vp, plc, path, i, mccb)) > 0)
		  		break;
		  	if (ret < 0) break;
		  	 //__attribute__ ((fallthrough)); // C and C++03
		  	 ALLOW_FALLTHROUGH;
		  	//[[gnu::fallthrough]];
		  	// else continue to next, set a title
		  case 0:
		  	*fname = 0; *ext = 0;
			_splitpath(path, NULL, NULL, fname, ext);
			if (__mingw_snprintf(buffer, MAX_PATH_UTF8, "%s%s", fname, ext) > 0)
				playlistSetTitle(plc, i, buffer, 0);
		};
	}
	return ret;
}

int setPlaylistPlayingItem (TVLCPLAYER *vp, PLAYLISTCACHE *plc, int trk, const unsigned int hash)
{
	//printf("setPlaylistPlayingItem '%s' %X %i\n", plc->title, hash, trk);

	if (!hash) return -1;
	TPLAYLISTITEM *item;
	int pos = -1;

	while ((item=playlistGetItem(plc, trk))){
		//printf("%i, %i %X\n", trk, item->objType, item->obj.track.hash);

		if (item->objType == PLAYLIST_OBJTYPE_TRACK && item->obj.track.hash == hash){
			pos = plc->pr->playingItem = trk;
			playlistChangeEvent(vp, plc, trk);
			trackLoadEvent(vp, plc, trk);
			break;
		}
		trk++;
	}
	return pos;
}


int importPlaylistW (TPLAYLISTMANAGER *plm, PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, TARTMANAGER *am, const wchar_t *inpath, TFILEPANE *filepane)
{
	wchar_t drive[MAX_PATH+1];
	wchar_t dir[MAX_PATH+1];
	wchar_t path[MAX_PATH+1];
	wchar_t name[MAX_PATH+1];
	wchar_t cpath[MAX_PATH+1];
	wchar_t ext[MAX_PATH+1];
	*drive = 0;
	*dir = 0;
	*cpath = 0;


	if (!isPlaylistW(inpath) || !doesFileExistW(inpath))
		return -1;

	_wsplitpath(inpath, drive, dir, name, ext);
				
	if (!*drive && *dir){
		GetCurrentDirectoryW(MAX_PATH, cpath);
		__mingw_swprintf(path, L"%ls\\%ls", cpath, dir);
		SetCurrentDirectoryW(path);
		__mingw_swprintf(path, L"%ls%ls", name, ext);
		
	}else if (*drive){
		__mingw_swprintf(path, L"%ls%ls", drive, dir);
		SetCurrentDirectoryW(path);
		wcscpy(path, inpath);
		
	}else{
		wcscpy(path, inpath);
	}

	int total = 0;

	TM3U *m3u = m3uNew();
	if (m3u){
		if (m3uOpen(m3u, path, M3U_OPENREAD)){
			//wprintf(L"reading #%s#\n", path);
			total = m3uReadPlaylist(m3u, plm, plc, tagc, am, filepane);
			//wprintf(L"importPlaylist: '%s', %i tracks read\n", path, total);
			m3uClose(m3u);
		}
		m3uFree(m3u);
	}
	
	if (*cpath)
		SetCurrentDirectoryW(cpath);

	return total;
}

int importPlaylist (TPLAYLISTMANAGER *plm, PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, TARTMANAGER *am, const char *path, TFILEPANE *filepane)
{
	wchar_t *out = converttow(path);
	if (out){
		int total = importPlaylistW(plm, plc, tagc, am, out, filepane);
		my_free(out);
		return total;
	}
	return 0;
}

int importPlaylistUIDW (TPLAYLISTMANAGER *plm, TMETATAGCACHE *tagc, TARTMANAGER *am, const wchar_t *path, const int uid, TFILEPANE *filepane)
{
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(plm, uid);
	
	//printf("importPlaylistUIDW %p\n", plc);
	
	if (plc)
		return importPlaylistW(plm, plc, tagc, am, path, filepane);
	else
		return -1;
}

void playlistChangeEvent (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int trackIdx)
{

	if (plc->pr->selectedItem >= playlistGetTotal(plc))
		plc->pr->selectedItem = -1;

	playlistMetaGetMeta(vp, plc, trackIdx, trackIdx+4, NULL);
	if (hasPageBeenAccessed(vp, PAGE_PLY_SHELF))
		invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), trackIdx);
	timerSet(vp, TIMER_CTRL_PLAYLISTLBREFRESH, 250);

	TPLYPANEL *plypan = pageGetPtr(vp, PAGE_PLY_PANEL);
	PLAYLISTCACHE *panelPlc = playlistManagerGetPlaylistByUID(vp->plm, plypan->currentPlcUID);
	
	if (panelPlc){
		plypan->currentPlcUID = getPrimaryPlaylist(vp)->uid;
		plypan->panel->itemOffset = &panelPlc->pr->itemOffset;
		timerSet(vp, TIMER_PLYPAN_REBUILD, 0);
		
	}else if (panelPlc == plc){
		if (!playlistGetTotal(plc)){
			if (plc->parent)
				plypan->currentPlcUID = plc->parent->uid;
			timerSet(vp, TIMER_PLYPAN_REBUILD, 0);
		}else{
			timerSet(vp, TIMER_PLYPAN_REBUILD, 0);
		}
	}
}

static inline void trackSetRegMeta (TVLCPLAYER *vp, const char *metaStr, const int metaTag)
{
	char *str = getPlayingTag(vp, metaTag);
	if (str){
		regSetString8(metaStr, str);
		my_free(str);
	}else{
		regSetString8(metaStr, " ");
	}
}

void trackLoadEvent (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int trackIdx)
{
	
	//printf("trackLoadEvent in\n");
	//printf("trackLoadEvent %i %i '%s'\n", trackIdx, plc->uid, plc->title);
	
	TMETA *meta = pageGetPtr(vp, PAGE_META);
	TMETADESC *desc = &meta->desc;
	
	// make meta page follow current track when it's the currently highlighted track
	if (desc->trackPosition == plc->pr->playingItemPrevious)
		desc->trackPosition = trackIdx;

	// allow playlist to follow playing track when nothing else is selected
	if (plc->pr->selectedItem == -1){
		if (pageGet(vp) == PAGE_PLY_SHELF){
			if (vp->playlist.queued == vp->playlist.display)
				bringAlbumToFocus(pageGetPtr(vp, PAGE_PLY_SHELF), trackIdx);
		}
	}else{
		if (hasPageBeenAccessed(vp, PAGE_PLY_SHELF))
			invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), -1);
	}

	plc->pr->playingItem = trackIdx;
	plc->pr->playingItemPrevious = plc->pr->playingItem;
	plc->pr->playingItemId = playlistGetId(plc, trackIdx);

	if (desc->trackPosition == -1 && plc->pr->playingItem >= 0)
		desc->trackPosition = plc->pr->playingItem;
	
	timerSet(vp, TIMER_REG_TRACK_UPDATE, 1500);
	timerSet(vp, TIMER_CTRL_PLAYLISTLBREFRESH, 250);
	playlistMetaGetMeta(vp, plc, plc->pr->playingItem-CACHEREADAHEAD, plc->pr->playingItem+CACHEREADAHEAD, NULL);
	ctrlNewTrackEvent(pageGetPtr(vp,PAGE_OVERLAY), plc->uid, trackIdx);

	taskbarPostMessage(vp, WM_TRACKPLAYNOTIFY, plc->uid, trackIdx);

	//printf("trackLoadEvent out\n");

}

void playlistResetRetries (TVLCPLAYER *vp)
{

	if (playlistManagerLock(vp->plm)){
		vlcEventListInvalidate(vp->vlc);
		const int pltotal = playlistManagerGetTotal(vp->plm);
		for (int i = 0; i < pltotal; i++){
			PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, i);
			if (plc){
				if (playlistLock(plc)){
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
		vlcEventListInvalidate(vp->vlc);
		playlistManagerUnlock(vp->plm);
	}
}


// return first playlist found (should never fail)
PLAYLISTCACHE *getPrimaryPlaylist (TVLCPLAYER *vp)
{
	return vp->playlist.root;
}

PLAYLISTCACHE *getDisplayPlaylist (TVLCPLAYER *vp)
{
	//printf("getDisplayPlaylist %i %i\n", vp->displayPlaylist, vp->queuedPlaylist);
	//printf("getDisplayPlaylist: %i\n", (int)GetCurrentThreadId());
	
	PLAYLISTCACHE *plcD = playlistManagerGetPlaylistByUID(vp->plm, vp->playlist.display);
	if (!plcD){
		int idx = playlistManagerGetPlaylistNext(vp->plm, -1);
		vp->playlist.display = playlistManagerGetUIDByIndex(vp->plm, idx);
		plcD = playlistManagerGetPlaylistByUID(vp->plm, vp->playlist.display);
	}
	//if (!plcD)
	//	printf("void display playlist %i\n", vp->displayPlaylist);

	return plcD;
}

PLAYLISTCACHE *getQueuedPlaylist (TVLCPLAYER *vp)
{
	//printf("getQueuedPlaylist: %i\n", (int)GetCurrentThreadId());
	return playlistManagerGetPlaylistByUID(vp->plm, vp->playlist.queued);
}

wchar_t *getPlayingPathW (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char *path = my_calloc(sizeof(char), MAX_PATH_UTF8+1);
		if (path){
			playlistGetPath(plc, plc->pr->playingItem, path, MAX_PATH_UTF8);
			if (*path){
				wchar_t *pathw = converttow(path);
				my_free(path);
				return pathw;
			}else{
				my_free(path);
			}
		}
	}
	return NULL;
}

char *getPlayingPath (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char *path = my_calloc(sizeof(char), MAX_PATH_UTF8+1);
		if (path){
			playlistGetPath(plc, plc->pr->playingItem, path, MAX_PATH_UTF8);
			if (*path)
				return path;
			else
				my_free(path);
		}
	}
	return NULL;
}

wchar_t *getPlayingArtworkPathW (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		int artId = playlistGetArtId(plc, plc->pr->playingItem);
		if (artId)
			return artManagerImageGetPath(vp->am, artId);
	}
	return NULL;
}

char *getPlayingArtworkPath (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		int artId = playlistGetArtId(plc, plc->pr->playingItem);
		if (artId){
			wchar_t *path = artManagerImageGetPath(vp->am, artId);
			if (path){
				char *path8 = convertto8(path);
				my_free(path);	
				return path8;
			}
		}
	}
	return NULL;
}

char *getPlayingTag (TVLCPLAYER *vp, const int mtag)
{
	if (mtag == MTAG_PATH)
		return getPlayingPath(vp);
	else if (mtag == MTAG_ArtworkPath)
		return getPlayingArtworkPath(vp);
	
		
	char buffer[MAX_PATH_UTF8+1];
	
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char path[MAX_PATH_UTF8+1];
		playlistGetPath(plc, plc->pr->playingItem, path, MAX_PATH_UTF8);
		tagRetrieve(vp->tagc, path, mtag, buffer, MAX_PATH_UTF8);
		if (*buffer)
			return my_strdup(buffer);
	}
	return NULL;
}

unsigned int getPlayingHash (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc && plc->pr->playingItem >= 0)
		return playlistGetHash(plc, plc->pr->playingItem);
	return 0;
}

unsigned int getSelectedHash (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc && plc->pr->selectedItem >= 0)
		return playlistGetHash(plc, plc->pr->selectedItem);
	return 0;
}

char *getPlayingTitle (TVLCPLAYER *vp)
{
	//printf("getPlayingTitle: %i\n", (int)GetCurrentThreadId());
	
	char title[MAX_PATH_UTF8+1];
	
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char path[MAX_PATH_UTF8+1];
		playlistGetPath(plc, plc->pr->playingItem, path, MAX_PATH_UTF8);
		tagRetrieve(vp->tagc, path, MTAG_Title, title, MAX_PATH_UTF8);
		if (*title){
			return my_strdup(title);
		}else{
			playlistGetTitle(plc, plc->pr->playingItem, title, MAX_PATH_UTF8);
			if (*title)
				return my_strdup(title);
		}
	}
	return NULL;
}

wchar_t *getPlayingTitleW (TVLCPLAYER *vp)
{
	char title[MAX_PATH_UTF8+1];
	
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char path[MAX_PATH_UTF8+1];
		playlistGetPath(plc, plc->pr->playingItem, path, MAX_PATH_UTF8);
		tagRetrieve(vp->tagc, path, MTAG_Title, title, MAX_PATH_UTF8);
		if (*title){
			return converttow(title);
		}else{
			playlistGetTitle(plc, plc->pr->playingItem, title, MAX_PATH_UTF8);
			if (*title)
				return converttow(title);
		}
	}
	return NULL;
}

wchar_t *getPlayingAlbumW (TVLCPLAYER *vp)
{
	char album[MAX_PATH_UTF8+1];
	
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char path[MAX_PATH_UTF8+1];
		playlistGetPath(plc, plc->pr->playingItem, path, MAX_PATH_UTF8);
		tagRetrieve(vp->tagc, path, MTAG_Album, album, MAX_PATH_UTF8);
		if (*album)
			return converttow(album);
	}
	return NULL;
}

char *getPlayingAlbum (TVLCPLAYER *vp)
{
	char album[MAX_PATH_UTF8+1];
	
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char path[MAX_PATH_UTF8+1];
		playlistGetPath(plc, plc->pr->playingItem, path, MAX_PATH_UTF8);
		tagRetrieve(vp->tagc, path, MTAG_Album, album, MAX_PATH_UTF8);
		if (*album)
			return my_strdup(album);
	}
	return NULL;
}

char *getPlayingArtist (TVLCPLAYER *vp)
{
	char artist[MAX_PATH_UTF8+1];
	
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char path[MAX_PATH_UTF8+1];
		playlistGetPath(plc, plc->pr->playingItem, path, MAX_PATH_UTF8);
		tagRetrieve(vp->tagc, path, MTAG_Artist, artist, MAX_PATH_UTF8);
		if (*artist)
			return my_strdup(artist);
	}
	return NULL;
}

wchar_t *getPlayingArtistW (TVLCPLAYER *vp)
{
	char artist[MAX_PATH_UTF8+1];
	
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char path[MAX_PATH_UTF8+1];
		playlistGetPath(plc, plc->pr->playingItem, path, MAX_PATH_UTF8);
		tagRetrieve(vp->tagc, path, MTAG_Artist, artist, MAX_PATH_UTF8);
		if (*artist)
			return converttow(artist);
	}
	return NULL;
}

char *getPlayingDescription (TVLCPLAYER *vp)
{
	char buffer[MAX_PATH_UTF8+1];
	
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char path[MAX_PATH_UTF8+1];
		playlistGetPath(plc, plc->pr->playingItem, path, MAX_PATH_UTF8);
		tagRetrieve(vp->tagc, path, MTAG_Description, buffer, MAX_PATH_UTF8);
		if (*buffer)
			return my_strdup(buffer);
	}
	return NULL;
}

wchar_t *getPlayingDescriptionW (TVLCPLAYER *vp)
{
	char buffer[MAX_PATH_UTF8+1];
	
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char path[MAX_PATH_UTF8+1];
		playlistGetPath(plc, plc->pr->playingItem, path, MAX_PATH_UTF8);
		tagRetrieve(vp->tagc, path, MTAG_Description, buffer, MAX_PATH_UTF8);
		if (*buffer)
			return converttow(buffer);
	}
	return NULL;
}

char *getPlayingLengthStr (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char buffer[64] = {0};
		unsigned int hash = playlistGetHash(plc, plc->pr->playingItem);
		if (hash){
			tagRetrieveByHash(vp->tagc, hash, MTAG_LENGTH, buffer, sizeof(buffer)-1);
			if (*buffer)
				return my_strdup(buffer);
		}
	}
	return NULL;
}

wchar_t *getPlayingLengthStrW (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char buffer[64] = {0};
		unsigned int hash = playlistGetHash(plc, plc->pr->playingItem);
		if (hash){
			tagRetrieveByHash(vp->tagc, hash, MTAG_LENGTH, buffer, sizeof(buffer)-1);
			if (*buffer)
				return converttow(buffer);
		}
	}
	return NULL;
}

char *getPlayingProgramme (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char *title = my_calloc(sizeof(char), MAX_PATH_UTF8+1);
		if (title){
			//playlistGetTitle(plc, plc->pr->playingItem, title, MAX_PATH_UTF8);
			char *path = getPlayingPath(vp);
			if (path){
				tagRetrieve(vp->tagc, path, MTAG_NowPlaying, title, MAX_PATH_UTF8);
				my_free(path);
			}

			if (*title)
				return title;
			else
				my_free(title);
		}
	}
	return NULL;
}

wchar_t *getPlayingProgrammeW (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc){
		char *title = my_calloc(sizeof(char), MAX_PATH_UTF8+1);
		if (title){
			//playlistGetTitle(plc, plc->pr->playingItem, title, MAX_PATH_UTF8);
			char *path = getPlayingPath(vp);
			if (path){
				tagRetrieve(vp->tagc, path, MTAG_NowPlaying, title, MAX_PATH_UTF8);
				my_free(path);
			}

			if (*title){
				wchar_t *titlew = converttow(title);
				my_free(title);
				return titlew;
			}else{
				my_free(title);
			}
		}
	}
	return NULL;
}

char *getSelectedPath (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	if (plc){
		char *path = my_calloc(sizeof(char), MAX_PATH_UTF8+1);
		if (path){
			playlistGetPath(plc, plc->pr->selectedItem, path, MAX_PATH_UTF8);
			if (*path)
				return path;
			else
				my_free(path);
		}
	}
	return NULL;
}

int getPlayingItem (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc)
		return plc->pr->playingItem;
	else
		return -1;
}

int getPlayingItemId (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (plc)
		return plc->pr->playingItemId;
	else
		return 0;
}

int getSelectedItemId (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	if (plc){
		if (!plc->pr->selectedItemId)
			plc->pr->selectedItemId = playlistGetId(plc, plc->pr->selectedItem);
		return plc->pr->selectedItemId;
	}else{
		return 0;
	}
}

int isPlayingItemId (TVLCPLAYER *vp, const int uid, const int id)
{
	if (id >= PLAYLIST_TRACK_BASEIDENT && uid == getQueuedPlaylistUID(vp)){
		PLAYLISTCACHE *plcQ = getQueuedPlaylist(vp);
		const PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
		return (plcQ == plc && plc && plc->pr->playingItemId == id);
	}
	return 0;
}

int isPlayingItem (TVLCPLAYER *vp, const PLAYLISTCACHE *plc, const int pos)
{
	PLAYLISTCACHE *plcQ = getQueuedPlaylist(vp);
	return (plcQ && plcQ == plc && pos >= 0 && plc->pr->playingItem == pos);
}

int getSelectedItem (TVLCPLAYER *vp)
{
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	if (plc)
		return plc->pr->selectedItem;
	else
		return -1;
}

void setSelectedItem (TVLCPLAYER *vp, const int item)
{
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	if (plc)
		plc->pr->selectedItem = item;
}

int setDisplayPlaylist (TVLCPLAYER *vp, PLAYLISTCACHE *plc)
{
	vp->playlist.display = playlistManagerGetPlaylistUID(vp->plm, plc);
	return vp->playlist.display;
}

int setQueuedPlaylist (TVLCPLAYER *vp, PLAYLISTCACHE *plc)
{
	vp->playlist.queued = playlistManagerGetPlaylistUID(vp->plm, plc);
	return vp->playlist.queued;
}

int setQueuedPlaylistByUID (TVLCPLAYER *vp, const int uid)
{
	if (uid > PLAYLIST_UID_BASE)
		vp->playlist.queued = uid;
	return vp->playlist.queued;
}

int setDisplayPlaylistByUID (TVLCPLAYER *vp, const int uid)
{
	if (uid > PLAYLIST_UID_BASE)
		vp->playlist.display = uid;
	else
		vp->playlist.display = getPrimaryPlaylist(vp)->uid;	//getPrimaryPlaylist() can not fail
	return vp->playlist.display;
}

int getRootPlaylistUID (TVLCPLAYER *vp)
{
	if (vp->playlist.root)
		return vp->playlist.root->uid;
	else
		return 0;
}

int getDisplayPlaylistUID (TVLCPLAYER *vp)
{
	if (vp->playlist.display > PLAYLIST_UID_BASE)
		return vp->playlist.display;
	else
		return 0;
}

int getQueuedPlaylistUID (TVLCPLAYER *vp)
{
	if (vp->playlist.queued > PLAYLIST_UID_BASE)
		return vp->playlist.queued;
	else
		return 0;
}

int getQueuedPlaylistParent (TVLCPLAYER *vp)
{
	if (vp->playlist.queued > PLAYLIST_UID_BASE)
		return playlistManagerGetPlaylistParentByUID(vp->plm, vp->playlist.queued);
	else
		return 0;
}

// returns playlist left off and from the queued playlist
int getQueuedPlaylistLeft (TVLCPLAYER *vp)
{
	if (vp->playlist.queued > PLAYLIST_UID_BASE){
		int idx = playlistManagerGetIndexByUID(vp->plm, vp->playlist.queued);
		idx = playlistManagerGetPlaylistPrev(vp->plm, idx);
		return playlistManagerGetUIDByIndex(vp->plm, idx);		
	}else{
		return 0;
	}
}

// returns playlist right off and from the queued playlist
int getQueuedPlaylistRight (TVLCPLAYER *vp)
{
	if (vp->playlist.queued > PLAYLIST_UID_BASE){
		int idx = playlistManagerGetIndexByUID(vp->plm, vp->playlist.queued);
		idx = playlistManagerGetPlaylistNext(vp->plm, idx);
		int uid = playlistManagerGetUIDByIndex(vp->plm, idx);		
		if (uid <= PLAYLIST_UID_BASE) uid = vp->playlist.queued;
		return uid;
	}else{
		return 0;
	}
}

// first item in a playlist need not be a [playable] track. eg; could be a child playlist
// returns -1 if no tracks
int getPlaylistFirstTrack (TVLCPLAYER *vp, const int uid)
{
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	int idx = playlistGetNextItem(plc, PLAYLIST_OBJTYPE_TRACK, -1);
	
	return idx;
}

static inline int uidToIdxLocked (PLAYLISTCACHE *plc, const int uid, int *idx)
{
	if (plc->uid == uid)
		return *idx;
	else
		(*idx)++;
	
	int trk = 0;
	TPLAYLISTITEM *item = playlistGetItem(plc, trk);
	while (item){
		if (item->objType == PLAYLIST_OBJTYPE_PLC){
			if (playlistGetTotal(item->obj.plc)){
				if (uidToIdxLocked(item->obj.plc, uid, idx) >= 0)
					return *idx;
			}
		}
		item = playlistGetItem(plc, ++trk);
	}
	
	return -1;
}

int playlistUIDToIdx (TPLAYLISTMANAGER *plm, const int uid)
{
	if (playlistManagerLock(plm)){
		const int total = playlistManagerGetTotal(plm);

		int  i = 0;
		int idx = 0;
		
		while(i < total){
			PLAYLISTCACHE *plc = playlistManagerGetPlaylist(plm, i++);
			if (plc && !plc->parent){
				idx = uidToIdxLocked(plc, uid, &idx);
				if (idx >= 0){
					playlistManagerUnlock(plm);
					return idx;
				}
			}
		}
		playlistManagerUnlock(plm);
	}
	return -1;
}

// TIMER_REG_TRACK_UPDATE
void timer_regTrackInfoUpdate (TVLCPLAYER *vp)
{
#if ENABLE_REGMETAUPDATE
	//printf("timer_regTrackInfoUpdate start\n");

	char *str = getPlayingTitle(vp);
	if (str){
		regSetString8("track_title", str);
		my_free(str);

		trackSetRegMeta(vp, "track_album", MTAG_Album);
		trackSetRegMeta(vp, "track_length", MTAG_LENGTH);
		trackSetRegMeta(vp, "track_artist", MTAG_Artist);
		trackSetRegMeta(vp, "track_date", MTAG_Date);
		trackSetRegMeta(vp, "track_path", MTAG_PATH);
		trackSetRegMeta(vp, "track_filename", MTAG_FILENAME);
		trackSetRegMeta(vp, "track_description", MTAG_Description);
		trackSetRegMeta(vp, "track_nowPlaying", MTAG_NowPlaying);
		trackSetRegMeta(vp, "track_genre", MTAG_Genre);
		trackSetRegMeta(vp, "track_artwork", MTAG_ArtworkPath);
		
		regSetDword(L"track_number", getPlayingItem(vp)+1);
		regSetQword(L"track_added", getTickCount());
	}
	
	//printf("timer_regTrackInfoUpdate end\n");
#endif
}

void playlistTimerStartTrackUID (TVLCPLAYER *vp, const unsigned int uid, const int trk)
{
	if (uid){
		TTIMERPLAYTRACK *tpt = &vp->gui.playtrack;
		tpt->uid = uid;
		tpt->track = trk;
		timerSet(vp, TIMER_PLAYTRACK, 0);
	}
}

void playlistTimerStartTrackPLC (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int trk)
{
	playlistTimerStartTrackUID(vp, playlistManagerGetPlaylistUID(vp->plm, plc), trk);
}

int playlistRenameItem (TMETATAGCACHE *tagc, PLAYLISTCACHE *plc, const int pos, char *newName)
{
	if (playlistGetItemType(plc, pos) == PLAYLIST_OBJTYPE_PLC){
		TPLAYLISTITEM *item = playlistGetItem(plc, pos);
		return playlistSetName(item->obj.plc, newName);
		
	}else if (playlistGetItemType(plc, pos) == PLAYLIST_OBJTYPE_TRACK){
		char path[MAX_PATH_UTF8+1];
		playlistGetPath(plc, pos, path, MAX_PATH_UTF8);
		tagAdd(tagc, path, MTAG_Title, newName, 1);
		return playlistSetTitle(plc, pos, newName, 1);
	}
	return 0;
}
