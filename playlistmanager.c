
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




static inline int _playlistManagerLock (TPLAYLISTMANAGER *plm)
{
	return lockWait(plm->hMutex, INFINITE);
}

static inline void _playlistManagerUnlock (TPLAYLISTMANAGER *plm)
{
	lockRelease(plm->hMutex);
}

int playlistManagerLock (TPLAYLISTMANAGER *plm)
{
	return _playlistManagerLock(plm);
}

void playlistManagerUnlock (TPLAYLISTMANAGER *plm)
{
	_playlistManagerUnlock(plm);
}

static inline PLAYLISTCACHE *_playlistManagerGetPlaylistByName (TPLAYLISTMANAGER *plm, const char *name)
{
	if (name){
		for (int i = 0; i < plm->p_total; i++){
			if (plm->plc[i]){
				if (!stricmp(plm->plc[i]->title, name))
					return plm->plc[i];
			}
		}
	}
	return NULL;
}

PLAYLISTCACHE *playlistManagerGetPlaylistByName (TPLAYLISTMANAGER *plm, const char *name)
{
	//if (_playlistManagerLock(plm)){
		PLAYLISTCACHE *ret = _playlistManagerGetPlaylistByName(plm, name);
	//	_playlistManagerUnlock(plm);
		return ret;
	//}
	//return NULL;
}

static inline int _playlistManagerGetUIDByName (TPLAYLISTMANAGER *plm, const char *name)
{
	for (int i = 0; i < plm->p_total; i++){
		if (plm->plc[i]){
			if (!stricmp(plm->plc[i]->title, name))
				return plm->plc[i]->uid;
		}
	}
	return 0;
}

int playlistManagerGetUIDByName (TPLAYLISTMANAGER *plm, const char *name)
{
	if (!name) return 0;
	//if (_playlistManagerLock(plm)){
		int uid = _playlistManagerGetUIDByName(plm, name);
	//	_playlistManagerUnlock(plm);
		return uid;
	//}
	//return 0;
}

static inline char *_playlistManagerGetName (TPLAYLISTMANAGER *plm, const int uid)
{
	for (int i = 0; i < plm->p_total; i++){
		if (plm->plc[i]){
			if (plm->plc[i]->uid == uid)
				return playlistGetNameDup(plm->plc[i]);
		}
	}

	return NULL;
}
	
char *playlistManagerGetName (TPLAYLISTMANAGER *plm, const int uid)
{
	if (uid <= PLAYLIST_UID_BASE) return NULL;
	char *name = NULL;
	
	//if (_playlistManagerLock(plm)){
		name = _playlistManagerGetName(plm, uid);
	//	_playlistManagerUnlock(plm);
	//}
	return name;
}


static inline int _playlistManagerGetTotal (TPLAYLISTMANAGER *plm)
{
	int ret = 0;
	for (int i = 0; i < plm->p_total; i++)
		if (plm->plc[i]) ret++;
	return ret;
}

// return number of playlists'
int playlistManagerGetTotal (TPLAYLISTMANAGER *plm)
{
	int ret = 0;
	//if (_playlistManagerLock(plm)){
		ret = _playlistManagerGetTotal(plm);
	//	_playlistManagerUnlock(plm);
	//}
	return ret;
}

static inline int _playlistManagerGetPlaylistPrev (TPLAYLISTMANAGER *plm, const int plIdx)
{
	for (int i = plIdx-1; i >= 0; i--)
		if (plm->plc[i]) return i;
	return -1;
}

int playlistManagerGetPlaylistPrev (TPLAYLISTMANAGER *plm, const int plIdx)
{
	//if (_playlistManagerLock(plm)){
		int ret = _playlistManagerGetPlaylistPrev(plm, plIdx);
	//	_playlistManagerUnlock(plm);
		return ret;
	//}
	//return -1;
}

static inline int _playlistManagerGetPlaylistNext (TPLAYLISTMANAGER *plm, const int plIdx)
{
	for (int i = plIdx+1; i < plm->p_total; i++)
		if (plm->plc[i]) return i;
	return -1;
}

int playlistManagerGetPlaylistNext (TPLAYLISTMANAGER *plm, const int plIdx)
{
	//if (_playlistManagerLock(plm)){
		int ret = _playlistManagerGetPlaylistNext(plm, plIdx);
	//	_playlistManagerUnlock(plm);
		return ret;
	//}
	//return -1;
}

int playlistManagerGetPlaylistFirst (TPLAYLISTMANAGER *plm)
{
	return playlistManagerGetPlaylistNext(plm, -1);
}

static inline PLAYLISTCACHE *_playlistManagerGetPlaylist (TPLAYLISTMANAGER *plm, const int plIdx)
{
	if (plIdx >= 0 && plIdx < plm->p_total)
		return plm->plc[plIdx];
	else
		return NULL;
}

PLAYLISTCACHE *playlistManagerGetPlaylist (TPLAYLISTMANAGER *plm, const int plIdx)
{
	//if (_playlistManagerLock(plm)){
		PLAYLISTCACHE *ret = _playlistManagerGetPlaylist(plm, plIdx);
		/*if (ret)
			printf("%s:%i: playlistManagerGetPlaylist: %i -> '%s'\n", func, line, plIdx, ret->title);
		else
			printf("%s:%i: playlistManagerGetPlaylist: %i -> %p\n", func, line, plIdx, ret);*/
	//	_playlistManagerUnlock(plm);
		return ret;
	//}
	//return NULL;
}

static inline PLAYLISTCACHE *_playlistManagerGetPlaylistBaseByUID (TPLAYLISTMANAGER *plm, const int uid)
{
	for (int i = 0; i < plm->p_total; i++){
		if (plm->plc[i]){
			if (plm->plc[i]->uid == uid)
				return plm->plc[i];
		}
	}
	return NULL;
}


static inline int _playlistManagerGetPlaylistParentByUID (TPLAYLISTMANAGER *plm, const int uid)
{
	if (uid <= PLAYLIST_UID_BASE)
		return 0;
	
	for (int i = 0; i < plm->p_total; i++){
		if (plm->plc[i]){
			if (plm->plc[i]->uid == uid && plm->plc[i]->parent)
				return plm->plc[i]->parent->uid;
		}
	}
	return 0;
}

int playlistManagerGetPlaylistParentByUID (TPLAYLISTMANAGER *plm, const int uid)
{
	//if (_playlistManagerLock(plm)){
		int ret = _playlistManagerGetPlaylistParentByUID(plm, uid);
	//	_playlistManagerUnlock(plm);
		return ret;
	//}
	//return 0;
}

static inline PLAYLISTCACHE *_playlistManagerGetPlaylistByUID (TPLAYLISTMANAGER *plm, const int uid)
{
	if (uid <= PLAYLIST_UID_BASE){
		//printf("%s:%i: ", func, line);
		//printf("playlistManagerGetPlaylistByUID: invalid UID supplied %i\n", uid);
		return NULL;
	}
	
	for (int i = 0; i < plm->p_total; i++){
		if (plm->plc[i]){
			if (plm->plc[i]->uid == uid)
				return plm->plc[i];
		}
	}

	//printf("%s:%i: ", func, line);
	//printf("playlistManagerGetPlaylistByUID: no playlist found for UID %i\n", uid);

	return NULL;
}

PLAYLISTCACHE *playlistManagerGetPlaylistByUID (TPLAYLISTMANAGER *plm, const int uid)
{
	//printf("playlistManagerGetPlaylistByUID: %i\n", (int)GetCurrentThreadId());
	
	//if (_playlistManagerLock(plm)){
		PLAYLISTCACHE *ret = _playlistManagerGetPlaylistByUID(plm, uid/*, func, line*/);
	//	_playlistManagerUnlock(plm);
		return ret;
	//}
	//return NULL;
}

static inline int _playlistManagerGetPlaylistArtIdByUID (TPLAYLISTMANAGER *plm, const int uid)
{
	if (uid <= PLAYLIST_UID_BASE) return 0;
	
	for (int i = 0; i < plm->p_total; i++){
		if (plm->plc[i]){
			if (plm->plc[i]->uid == uid)
				return plm->plc[i]->artId;
		}
	}

	return 0;
}

int playlistManagerGetPlaylistArtIdByUID (TPLAYLISTMANAGER *plm, const int uid)

{
	//if (_playlistManagerLock(plm)){
		int ret = _playlistManagerGetPlaylistArtIdByUID(plm, uid);
	//	_playlistManagerUnlock(plm);
		return ret;
	//}
	//return 0;
}

static inline int _playlistManagerGetPlaylistUID (TPLAYLISTMANAGER *plm, const PLAYLISTCACHE *plc)
{
	if (plc){
		return plc->uid;
		/*
		for (int i = 0; i < plm->p_total; i++){
			if (plm->plc[i] == plc)
				return plm->plc[i]->uid;
		}*/
	}
	return 0;
}

int playlistManagerGetPlaylistUID (TPLAYLISTMANAGER *plm, const PLAYLISTCACHE *plc)
{
	//printf("playlistManagerGetPlaylistUID: %i\n", (int)GetCurrentThreadId());
	
	//if (_playlistManagerLock(plm)){
		int ret = _playlistManagerGetPlaylistUID(plm, plc);
		//_playlistManagerUnlock(plm);
		return ret;
	//}
	//return 0;
}

static inline int _playlistManagerGetIndexByUID (TPLAYLISTMANAGER *plm, const int uid)
{
	if (uid <= PLAYLIST_UID_BASE){
		//printf("playlistManagerGetIndexByUID: invalid UID supplied %i\n", uid);
		return 0;
	}
	
	for (int i = 0; i < plm->p_total; i++){
		if (plm->plc[i]){
			if (plm->plc[i]->uid == uid)
				return i;
		}
	}
	return 0;
}

int playlistManagerGetIndexByUID (TPLAYLISTMANAGER *plm, const int uid)
{
	//if (_playlistManagerLock(plm)){
		int ret = _playlistManagerGetIndexByUID(plm, uid);
	//	_playlistManagerUnlock(plm);
		return ret;
	//}
	//return 0;
}

static inline int _playlistManagerFindPathUID (TPLAYLISTMANAGER *plm, plm_search *plms, const char *searchFor, const int uid)
{
	int ct = 0;
	
	PLAYLISTCACHE *plc = _playlistManagerGetPlaylistByUID(plm, uid);
	if (plc){
		//printf("_playlistManagerFindPathUID %X '%s'\n", uid, plc->title);
		
		if (playlistLock(plc)){
			const int total = plc->total;
			for (int rec = 0; *plms->activeState && rec < total; rec++){
				const int itemType = g_playlistGetItemType(plc, rec);
				
				if (itemType == PLAYLIST_OBJTYPE_TRACK){
					TPLAYLISTITEM *item = g_playlistGetItem(plc, rec);
					if (item->obj.track.path){
						ct++;
						if (plms->funcCb && *plms->activeState){
							unsigned int hash = 0;
							if (plms->wantHash) hash = item->obj.track.hash;
							if (!plms->funcCb(plms->uptr, searchFor, plc->uid, rec, MTAG_PATH, hash, itemType, ct)){
								playlistUnlock(plc);
								return ct;
							}
						}
					}
				}else if (itemType == PLAYLIST_OBJTYPE_PLC){
					TPLAYLISTITEM *item = g_playlistGetItem(plc, rec);
					if (item->obj.plc->title && *item->obj.plc->title){
						ct++;
						if (plms->funcCb && *plms->activeState){
							if (!plms->funcCb(plms->uptr, searchFor, plc->uid, rec, MTAG_Title, 0, itemType, ct)){
								playlistUnlock(plc);
								return ct;
							}
						}
					}
				}
			}
			playlistUnlock(plc);
		}
		if (ct || !*plms->activeState)
			return ct;
	}else{
		return 0;
	}
	return ct;
}

// test string: #f024+654+086+E23+ 788 + 345 +234 + ++
static inline int _playlistManagerSearchComputed (TPLAYLISTMANAGER *plm, plm_search *plms, const char *searchFor)
{
	int ct = 0;
	const int slen = strlen(searchFor);

	// search for artwork image
	if (slen == 2 && searchFor[1] == '$' && plms->data > 0){
		int quit = 1;
		const int imgId = plms->data;
		const int plmTotal = plm->p_total;
		
		//printf("playlistManagerSearchComputed imgId %X\n", imgId);
		
		for (int i = 0; *plms->activeState && i < plmTotal; i++){
			PLAYLISTCACHE *plc = plm->plc[i];
			if (!plc) continue;

			if (playlistLock(plc)){
				const int plcTotal = plc->total;
				for (int rec = 0; *plms->activeState && rec < plcTotal; rec++){
					TPLAYLISTITEM *item = g_playlistGetItem(plc, rec);

					//printf("item->obj.track.artId %X %i %i 0x%X,\n", plc->uid, rec, item->objType, item->obj.track.artId);
					
					if (item->objType == PLAYLIST_OBJTYPE_TRACK){
						if (item->obj.track.artId == imgId){
							ct++;
							if (plms->funcCb && *plms->activeState)
								quit = plms->funcCb(plms->uptr, searchFor, plc->uid, rec, MTAG_Title, item->obj.track.hash, item->objType, ct);
						}
					}else if (item->objType == PLAYLIST_OBJTYPE_PLC){
						if (item->obj.plc->artId == imgId){
							ct++;
							if (plms->funcCb && *plms->activeState)
								quit = plms->funcCb(plms->uptr, searchFor, plc->uid, rec, MTAG_Title, 0, item->objType, ct);
						}
					}
				}
				
				playlistUnlock(plc);
			}
			//if (!quit) printf("!quit for %X\n", imgId);
			if (!quit) return ct;
		}	
	
	// find and add all playlists
	}else if (slen == 2 && searchFor[1] == '*'){
		int quit = 1;
		for (int i = 0; *plms->activeState && i < plm->p_total; i++){
			PLAYLISTCACHE *plc = plm->plc[i];
			if (!plc) continue;

			if (playlistLock(plc)){
				for (int rec = 0; *plms->activeState && rec < plc->total; rec++){
					const int itemType = g_playlistGetItemType(plc, rec);
					if (itemType == PLAYLIST_OBJTYPE_PLC){
						TPLAYLISTITEM *item = g_playlistGetItem(plc, rec);
						if (item->obj.plc->title && *item->obj.plc->title){
							ct++;
							if (plms->funcCb && *plms->activeState)
								quit = plms->funcCb(plms->uptr, searchFor, plc->uid, rec, MTAG_Title, 0, itemType, ct);
						}
					}
				}
				
				playlistUnlock(plc);
			}
			if (!quit) return ct;
		}

	// find and add all tracks
	}else if (slen == 2 && searchFor[1] == ':'){
		int quit = 1;
		for (int i = 0; *plms->activeState && i < plm->p_total; i++){
			PLAYLISTCACHE *plc = plm->plc[i];
			if (!plc) continue;

			if (playlistLock(plc)){
				for (int rec = 0; *plms->activeState && rec < plc->total; rec++){
					const int itemType = g_playlistGetItemType(plc, rec);
					
					if (itemType == PLAYLIST_OBJTYPE_TRACK){
						ct++;
						if (plms->funcCb && *plms->activeState)
							quit = plms->funcCb(plms->uptr, searchFor, plc->uid, rec, MTAG_Title, g_playlistGetHash(plc,rec), itemType, ct);
					}
				}
				playlistUnlock(plc);
			}
			if (!quit) return ct;
		}

	// search for and add playlists then tracks
	}else if (slen == 3 && searchFor[1] == '*' && searchFor[2] == ':'){
		ct += _playlistManagerSearchComputed(plm, plms, "#*");
		if (*plms->activeState)
			ct += _playlistManagerSearchComputed(plm, plms, "#:");
	
	// search for and add tracks then playlists
	}else if (slen == 3 && searchFor[1] == ':' && searchFor[2] == '*'){
		ct += _playlistManagerSearchComputed(plm, plms, "#:");
		if (*plms->activeState)
			ct += _playlistManagerSearchComputed(plm, plms, "#*");

	// find and add all tracks and playlists (ie; everything)
	}else if (slen == 2 &&  searchFor[1] == '.'){
		int quit = 1;
		for (int i = 0; *plms->activeState && i < plm->p_total; i++){
			PLAYLISTCACHE *plc = plm->plc[i];
			if (!plc) continue;

			if (playlistLock(plc)){
				for (int rec = 0; *plms->activeState && rec < plc->total; rec++){
					const int itemType = g_playlistGetItemType(plc, rec);
					
					if (itemType == PLAYLIST_OBJTYPE_TRACK){
						ct++;
						if (plms->funcCb && *plms->activeState)
							quit = plms->funcCb(plms->uptr, searchFor, plc->uid, rec, MTAG_Title, g_playlistGetHash(plc,rec), itemType, ct);
					}else if (itemType == PLAYLIST_OBJTYPE_PLC){
						TPLAYLISTITEM *item = g_playlistGetItem(plc, rec);
						if (item->obj.plc->title && *item->obj.plc->title){
							ct++;
							if (plms->funcCb && *plms->activeState)
								quit = plms->funcCb(plms->uptr, searchFor, plc->uid, rec, MTAG_Title, 0, itemType, ct);
						}
					}
				}
				playlistUnlock(plc);
			}
			if (!quit) return ct;
		}

	// search for playlist #xxx
	}else if (slen > 3){		// 3 for #nnn
		int *vals = NULL;
		int total = hexToInts(&searchFor[1], '+', &vals);
		if (total){
			for (int i = 0; i < total; i++){
				int uid = vals[i];
				//printf("hexToInts %i %X\n", i, uid);
				if (uid > PLAYLIST_UID_BASE)
					ct += _playlistManagerFindPathUID(plm, plms, searchFor, uid);
			}
			if (total) my_free(vals);
		}else{
			const int uid = hexToInt(&searchFor[1]);
			if (uid > PLAYLIST_UID_BASE)
				ct += _playlistManagerFindPathUID(plm, plms, searchFor, uid);
		}
		//if (ct) return ct;
	}
	return ct;
}

static inline int _playlistManagerSearchPostResult (plm_search *plms, const char *searchFor, const int uid, const int trackIdx)
{
	unsigned int hash = 0;
	if (plms->wantHash) hash = g_playlistGetHash(plms->plc, trackIdx);
	return plms->funcCb(plms->uptr, searchFor, uid, trackIdx, plms->mtag, hash, g_playlistGetItemType(plms->plc,trackIdx), plms->count);
}

static inline int _playlistManagerSearch (TPLAYLISTMANAGER *plm, plm_search *plms, const char *searchFor, const int from, const int to, const int searchHow)
{
	//printf("_playlistManagerSearch '%s' %i %i\n", searchFor, from, to);

	if (searchFor[0] == '#' && strlen(searchFor) >= 2)
		return _playlistManagerSearchComputed(plm, plms, searchFor);

	plms->count = 0;

	for (int i = from; *plms->activeState && i <= to && i < plm->p_total; i++){
		if (!plm->plc[i]) continue;
		
		plms->plc = plm->plc[i];
		if (playlistLock(plms->plc)){
			const int total = plms->plc->total;
			for (int rec = 0; *plms->activeState && rec < total; rec++){
				const int trackIdx = playlistSearchEx(plms->plc, plms->tagc, searchFor, rec, &plms->mtag, searchHow);
				if (trackIdx >= 0){
					rec = trackIdx;
					plms->count++;
					if (plms->funcCb && *plms->activeState){
						if (!_playlistManagerSearchPostResult(plms, searchFor, plms->plc->uid, trackIdx)){
							playlistUnlock(plms->plc);
							return plms->count;
						}
					}
				}else{
					rec = total;
				}
			}
			playlistUnlock(plms->plc);
		}
	}
	return plms->count;
}

int playlistManagerSearchEx (TPLAYLISTMANAGER *plm, plm_search *plms, int from, int to)
{
	int found = 0;
	if (from == -2) from = (plm->p_total/2)+1;
	if (from < 0) from = 0;
	if (to == -1) to = plm->p_total;
	if (to == -2) to = plm->p_total/2;
	if (from > to) return 0;

	//if (_playlistManagerLock(plm)){
		found = _playlistManagerSearch(plm, plms, plms->string, from, to, plms->searchHow);
	//	_playlistManagerUnlock(plm);
	//}
	return found;
}

int playlistManagerSearch (TPLAYLISTMANAGER *plm, plm_search *plms)
{
	//if (_playlistManagerLock(plm)){		
		int ret = _playlistManagerSearch(plm, plms, plms->string, 0, plm->p_total, plms->searchHow);
	//	_playlistManagerUnlock(plm);
		return ret;
	//}
	//return 0;
}

int _playlistManagerGetPlaylistByTrackHash (TPLAYLISTMANAGER *plm, const unsigned int hash)
{
	for (int i = 0; i < plm->p_total; i++){
		PLAYLISTCACHE *plc = plm->plc[i];
		if (!plc) continue;
		
		if (playlistGetTrackIdByHash(plc, hash))
			return plc->uid;
	}
	return 0;
}

int playlistManagerGetPlaylistByTrackHash (TPLAYLISTMANAGER *plm, const unsigned int hash)
{
	//if (_playlistManagerLock(plm)){
		int ret = _playlistManagerGetPlaylistByTrackHash(plm, hash);
	//	_playlistManagerUnlock(plm);
		return ret;
	//}
	//return 0;
}

static inline int _playlistManagerIsValidUID (TPLAYLISTMANAGER *plm, const int uid)
{
	if (uid <= PLAYLIST_UID_BASE) return 0;
	
	for (int i = 0; i < plm->p_total; i++){
		if (plm->plc[i] && plm->plc[i]->uid == uid)
			return 1;
	}
	return 0;
}

int playlistManagerIsValidUID (TPLAYLISTMANAGER *plm, const int uid)
{
	//if (_playlistManagerLock(plm)){
		int ret = _playlistManagerIsValidUID(plm, uid);
	//	_playlistManagerUnlock(plm);
		return ret;
	//}
	//return 0;
}

static inline int _playlistManagerGetUIDByIndex (TPLAYLISTMANAGER *plm, const int index)
{
	if (index >= 0 && index < plm->p_total){
		if (plm->plc[index])
			return plm->plc[index]->uid;
	}
	return -1;
}

int playlistManagerGetUIDByIndex (TPLAYLISTMANAGER *plm, const int index)
{
	//if (_playlistManagerLock(plm)){
		int ret = _playlistManagerGetUIDByIndex(plm, index);
	//	_playlistManagerUnlock(plm);
		return ret;
	//}
	//return 0;
}

int playlistManagerCreateUID (TPLAYLISTMANAGER *plm)
{
	if (_playlistManagerLock(plm)){
		int uid = ++plm->uidSrc;
		_playlistManagerUnlock(plm);
		return uid;
	}
	return -1;
}

static inline int _playlistManagerGetPlaylistIndex (TPLAYLISTMANAGER *plm, const PLAYLISTCACHE *plc)
{
	if (plc){
		for (int i = 0; i < plm->p_total; i++){
			if (plm->plc[i] == plc)
				return i;
		}
	}
	return -1;
}

int playlistManagerGetPlaylistIndex (TPLAYLISTMANAGER *plm, const PLAYLISTCACHE *plc)
{
	//if (_playlistManagerLock(plm)){
		int ret = _playlistManagerGetPlaylistIndex(plm, plc);
	//	_playlistManagerUnlock(plm);
		return ret;
	//}
	//return -1;
}

static inline PLAYLISTCACHE *_playlistManagerAlloc (TPLAYLISTMANAGER *plm, const char *name)
{
	if (name){
		for (int i = 0; i < plm->p_total; i++){
			if (!plm->plc[i]){
				plm->plc[i] = playlistNew(plm, name);
				return plm->plc[i];
			}
		}
	}

	PLAYLISTCACHE **tmp = (PLAYLISTCACHE**)my_realloc(plm->plc, (plm->p_total+32) * sizeof(PLAYLISTCACHE**));
	if (tmp){
		plm->plc = tmp;
		
		for (int i = plm->p_total; i < plm->p_total+32; i++)
			plm->plc[i] = NULL;

		PLAYLISTCACHE *plc = NULL;
		if (name)
			plc = playlistNew(plm, name);

		plm->plc[plm->p_total] = plc;
		plm->p_total += 32;
		return plc;
	}else{
		return NULL;
	}
}

PLAYLISTCACHE *playlistManagerCreatePlaylist (TPLAYLISTMANAGER *plm, const char *name, const int single)
{
	PLAYLISTCACHE *ret = NULL;
	if (_playlistManagerLock(plm)){
		if (single)
			ret = _playlistManagerGetPlaylistByName(plm, name);
		if (!ret)
			ret = _playlistManagerAlloc(plm, name);
		_playlistManagerUnlock(plm);
	}
	return ret;
}

static inline int _playlistManagerCreatePlaylistUID (TPLAYLISTMANAGER *plm, const char *name, const int single)
{
	PLAYLISTCACHE *plc = playlistManagerCreatePlaylist(plm, name, single);
	if (plc)
		return plc->uid;
	else
		return 0;
}

int playlistManagerCreatePlaylistUID (TPLAYLISTMANAGER *plm, const char *name, const int single)
{
	int ret = 0;
	//if (_playlistManagerLock(plm)){
		ret = _playlistManagerCreatePlaylistUID(plm, name, single);
	//	_playlistManagerUnlock(plm);
	//}
	return ret;
}

static inline int _playlistManagerAddPlaylistUID (TPLAYLISTMANAGER *plm, const int parentUID, const int childUID)
{
	int pos = -1;

	PLAYLISTCACHE *parent = NULL;
	if (parentUID <= PLAYLIST_UID_BASE){
		parent = _playlistManagerGetPlaylistBaseByUID(plm, PLAYLIST_UID_BASE+1);
	}else{
		parent = _playlistManagerGetPlaylistByUID(plm, parentUID);
	}
		
	if (parent){
		PLAYLISTCACHE *child = playlistManagerGetPlaylistByUID(plm, childUID);
		if (child){
			pos = playlistAddPlc(parent, child);
			child->parent = parent;
		}
	}
	return pos;
}

int playlistManagerAddPlaylistUID (TPLAYLISTMANAGER *plm, const int parentUID, const int childUID)
{
	int pos = -1;
	//if (_playlistManagerLock(plm)){
		pos = _playlistManagerAddPlaylistUID(plm, parentUID, childUID);
	//	_playlistManagerUnlock(plm);
	//}
	return pos;
}

static inline int _playlistManagerInsertPlaylist (TPLAYLISTMANAGER *plm, PLAYLISTCACHE *plc, const int plIdx)
{
	// grow the plm if need be
	if (_playlistManagerGetTotal(plm) == plm->p_total)
		_playlistManagerAlloc(plm, NULL);
	
	// move playlist entires right by one then inesrt plc left most
	for (int i = plm->p_total-1; i > plIdx; i--)
		plm->plc[i] = plm->plc[i-1];

	plm->plc[plIdx] = plc;
	return plm->p_total;
}

static inline int _playlistManagerRemovePlaylist (TPLAYLISTMANAGER *plm, PLAYLISTCACHE *plc)
{
	for (int i = 0; i < plm->p_total; i++){
		if (plm->plc[i] == plc){
			my_memcpy(&plm->plc[i], &plm->plc[i+1], ((plm->p_total-1)-i)*sizeof(PLAYLISTCACHE*));
			plm->plc[plm->p_total-1] = NULL;
			return 1;
		}
	}
	return 0;
}

static inline int playlistManagerMovePlaylist (TPLAYLISTMANAGER *plm, const int from, const int to)
{
	if (from == to) return 1;

	if (_playlistManagerLock(plm)){
		int ret = 0;
		PLAYLISTCACHE *plc = _playlistManagerGetPlaylist(plm, from);
		if (plc){
			if (_playlistManagerRemovePlaylist(plm, plc))
				ret = _playlistManagerInsertPlaylist(plm, plc, to);
		}
		_playlistManagerUnlock(plm);
		return ret;
	}
	return 0;
}

static inline int _playlistManagerSwapPlaylists (TPLAYLISTMANAGER *plm, int plA, int plB)
{
	if (plA == plB) return 1;
	if (plA < plB){
		plB ^= plA;
		plA ^= plB;
		plB ^= plA;
	}
	
	PLAYLISTCACHE *plcA = _playlistManagerGetPlaylist(plm, plA);
	PLAYLISTCACHE *plcB = _playlistManagerGetPlaylist(plm, plB);
	if (plcA && plcB){
		_playlistManagerRemovePlaylist(plm, plcA);
		_playlistManagerRemovePlaylist(plm, plcB);
		_playlistManagerInsertPlaylist(plm, plcA, plB);
		return _playlistManagerInsertPlaylist(plm, plcB, plA);
	}
	return 0;
}

static inline int playlistManagerSwapPlaylists (TPLAYLISTMANAGER *plm, const int plA, const int plB)
{
	if (_playlistManagerLock(plm)){
		int ret = _playlistManagerSwapPlaylists(plm, plA, plB);
		_playlistManagerUnlock(plm);
		return ret;
	}
	return 0;
}

static inline int _playlistManagerMovePlaylistInto (TPLAYLISTMANAGER *plm, const int from, const int to)
{
	if (from == to) return 0;

	PLAYLISTCACHE *plcFrom = _playlistManagerGetPlaylistByUID(plm, from);
	PLAYLISTCACHE *plcTo = _playlistManagerGetPlaylistByUID(plm, to);
	if (!plcFrom || !plcTo) return 0;

	//printf("plyManagerMvInto %X %X %i %i\n", plcFrom->uid, plcTo->uid, plcFrom->total, plcTo->total);
	
	int ret = 0;
	
	if (playlistLock(plcFrom)){
		if (playlistLock(plcTo)){
			PLAYLISTCACHE *plcFromParent = plcFrom->parent;

			if (playlistLock(plcFromParent)){
				int fromPosition = playlistGetPlaylistIndex(plcFromParent, plcFrom);
				if (fromPosition >= 0){

					TPLAYLISTRECORD *fromRec = playlistRemoveRecord(plcFromParent, fromPosition);
					if (fromRec){
						plcFromParent->total--;
						int dest = plcTo->total+1;		// insert last
						if (dest < 0) dest = 0;
				
						if (playlistInsert(plcTo, fromRec, dest)){
							plcTo->total++;
							plcFrom->parent = plcTo;
							ret = 1;
						}
					}
				}
				playlistUnlock(plcFromParent);
			}
			playlistUnlock(plcTo);
		}
		playlistUnlock(plcFrom);
	}
	return ret;
}

int playlistManagerMovePlaylistInto (TPLAYLISTMANAGER *plm, const int from, const int to)
{
	if (_playlistManagerLock(plm)){
		int ret = _playlistManagerMovePlaylistInto(plm, from, to);
		_playlistManagerUnlock(plm);
		return ret;
	}
	return 0;
}

static inline int _playlistManagerMovePlaylistTo (TPLAYLISTMANAGER *plm, const int from, const int to)
{
	PLAYLISTCACHE *plcFrom = _playlistManagerGetPlaylistByUID(plm, from);
	PLAYLISTCACHE *plcTo = _playlistManagerGetPlaylistByUID(plm, to);
	if (!plcFrom || !plcTo) return 0;
	
	int ret = 0;
	
	if (playlistLock(plcFrom)){
		if (playlistLock(plcTo)){
			PLAYLISTCACHE *plcFromParent = plcFrom->parent;
			PLAYLISTCACHE *plcToParent = plcTo->parent;

			if (plcFromParent && plcToParent){
				if (playlistLock(plcFromParent)){
					if (playlistLock(plcToParent)){
						int fromPosition = playlistGetPlaylistIndex(plcFromParent, plcFrom);
						if (fromPosition >= 0){

							TPLAYLISTRECORD *fromRec = playlistRemoveRecord(plcFromParent, fromPosition);
							if (fromRec){
								plcFromParent->total--;

								int toPosition = playlistGetPlaylistIndex(plcToParent, plcTo);
								if (toPosition >= 0){
									if (playlistInsert(plcToParent, fromRec, toPosition)){
										plcToParent->total++;
										plcFrom->parent = plcToParent;
										ret = 1;
									}
								}
							}
						}
						playlistUnlock(plcToParent);
					}
					playlistUnlock(plcFromParent);
				}
			}
			playlistUnlock(plcTo);
		}
		playlistUnlock(plcFrom);
	}
	return ret;
}

int playlistManagerMovePlaylistTo (TPLAYLISTMANAGER *plm, const int from, const int to)
{
	if (_playlistManagerLock(plm)){
		int ret = _playlistManagerMovePlaylistTo(plm, from, to);
		_playlistManagerUnlock(plm);
		return ret;
	}
	return 0;
}

static inline int _playlistManagerDeletePlaylist (TPLAYLISTMANAGER *plm, PLAYLISTCACHE *plc, const int doFreePlaylist)
{
	for (int i = 0; i < plm->p_total; i++){
		if (plm->plc[i] == plc){
			my_memcpy(&plm->plc[i], &plm->plc[i+1], ((plm->p_total-1)-i)*sizeof(PLAYLISTCACHE*));
			plm->plc[plm->p_total-1] = NULL;				

			for (int i = 0; i < plc->total; i++){
				TPLAYLISTITEM *item = playlistGetItem(plc, i);
				if (item && item->objType == PLAYLIST_OBJTYPE_PLC)
					_playlistManagerDeletePlaylist(plm, item->obj.plc, 0);
			}
				
			if (doFreePlaylist) playlistFree(plc);
			return 1;
		}
	}
	return 0;
}

int playlistManagerDeletePlaylist (TPLAYLISTMANAGER *plm, PLAYLISTCACHE *plc, const int doFreePlaylist)
{
	if (_playlistManagerLock(plm)){
		int ret = _playlistManagerDeletePlaylist(plm, plc, doFreePlaylist);
		//if (ret) plm->p_total--;
		_playlistManagerUnlock(plm);
		return ret;
	}
	return 0;
}

int playlistManagerDeletePlaylistByUID (TPLAYLISTMANAGER *plm, const int uid, const int doFreePlaylist)
{
	if (_playlistManagerLock(plm)){
		PLAYLISTCACHE *plc = _playlistManagerGetPlaylistByUID(plm, uid);
		
		int ret = _playlistManagerDeletePlaylist(plm, plc, doFreePlaylist);
		_playlistManagerUnlock(plm);
		return ret;
	}
	return 0;
}

static inline void _playlistManagerFree (TPLAYLISTMANAGER *plm)
{
	if (!plm) return;

	if (plm->plc){
		for (int i = 0; i < plm->p_total; i++){
			if (plm->plc[i])
			 	_playlistManagerDeletePlaylist(plm, plm->plc[i], 1);
		}
		my_free(plm->plc);
	}
	
	plm->plc = NULL;
	plm->p_total = 0;
}

static inline void _playlistManagerDelete (TPLAYLISTMANAGER *plm)
{
	_playlistManagerFree(plm);
	lockClose(plm->hMutex);
	my_free(plm->pr);
	my_free(plm);
}

void playlistManagerDelete (TPLAYLISTMANAGER *plm)
{
	_playlistManagerLock(plm);
	_playlistManagerDelete(plm);
}

TPLAYLISTMANAGER *playlistManagerNew ()
{
	TPLAYLISTMANAGER *plm = my_calloc(1, sizeof(TPLAYLISTMANAGER));
	if (plm){
		plm->hMutex = lockCreate("playlistManagerNew");
		plm->p_total = 8;
		plm->uidSrc = PLAYLIST_UID_BASE;
		plm->plc = (PLAYLISTCACHE**)my_calloc(plm->p_total, sizeof(PLAYLISTCACHE**));
		
		if (plm->plc){
			plm->pr = my_calloc(1, sizeof(PLAYLISTRENDER));
			if (plm->pr){
				plm->pr->selectedItem = -1;
				plm->pr->playingItem = -1;
				plm->pr->playingItemPrevious = -2;
			}
		}
	}
	return plm;
}

