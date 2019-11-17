
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



/*

Note:
Underscored functions (_playlist####) do not pass through a lock, either directly or indirectly nor
may they call a locked function either directly or indirectly. Has local file scope only

g_### functions as above but with global scope

Non-underscored functions will contain the lock
*/

#include "common.h"



/*
static inline int matchWildcardsW (wchar_t *String, wchar_t *Pattern, const int IgnoreCase)
{
    wchar_t *s, *p;
    int star = FALSE;

    // Code is from http://xoomer.virgilio.it/acantato/dev/wildcard/wildmatch.html

LoopStart:
    for (s = String, p = Pattern; *s; s++, p++){
        switch (*p){
        case L'?':
            break;
        case L'*':
            star = TRUE;
            String = s;
            Pattern = p;

            do{
                Pattern++;
            }while (*Pattern == '*');

            if (!*Pattern) return TRUE;

            goto LoopStart;
            
        default:
            if (!IgnoreCase){
                if (*s != *p)
                    goto StarCheck;
            }else{
                if (towupper(*s) != towupper(*p))
                    goto StarCheck;
            }
            break;
        }
    }

    while (*p == L'*')
        p++;

    return (!*p);

StarCheck:
    if (!star)
        return FALSE;

    String++;
    goto LoopStart;
}
*/


static inline int matchWildcards8IgnoreCase (const char *_String, const char *_Pattern)
{
    char *s, *p;
    int star = FALSE;
    char *Pattern = (char*)_Pattern;
    char *String = (char*)_String;

LoopStart:
    for (s = String, p = Pattern; *s; s++, p++){
        switch (*p){
        case '?':
            break;
        case '*':
            star = TRUE;
            String = s;
            Pattern = p;

            do{
                Pattern++;
            }while (*Pattern == '*');

            if (!*Pattern) return TRUE;

            goto LoopStart;
            
        default:
			if (toupper(*s) != toupper(*p))
				goto StarCheck;
            break;
        }
    }

    while (*p == '*')
        p++;

    return (!*p);

StarCheck:
    if (!star)
        return FALSE;

    String++;
    goto LoopStart;
}

static inline int matchWildcards8WithCase (const char *_String, const char *_Pattern)
{
    char *s, *p;
    int star = FALSE;
    char *Pattern = (char*)_Pattern;
    char *String = (char*)_String;

LoopStart:
    for (s = String, p = Pattern; *s; s++, p++){
        switch (*p){
        case '?':
            break;
        case '*':
            star = TRUE;
            String = s;
            Pattern = p;

            do{
                Pattern++;
            }while (*Pattern == '*');

            if (!*Pattern) return TRUE;

            goto LoopStart;
            
        default:
			if (*s != *p)
				goto StarCheck;
            break;
        }
    }

    while (*p == '*')
        p++;

    return (!*p);

StarCheck:
    if (!star)
        return FALSE;

    String++;
    goto LoopStart;
}

static inline int matchWildcards8 (const char *string, const char *pattern, const int ignoreCase)
{
	//printf("matchWildcards8 %i\n", ignoreCase);
	
	if (!ignoreCase)
		return matchWildcards8WithCase(string, pattern);
	else
		return matchWildcards8IgnoreCase(string, pattern);
}

static inline int strMatch (const char *haystack, const char *needle, const int caseInsensitive)
{
	return !matchWildcards8(haystack, needle, caseInsensitive);
}

static inline int strFind (const char *haystack, const char *needle, const int searchHow)
{
	//printf("strFind: '%s' '%s' %i\n", haystack, needle, searchHow);

	if (searchHow == PLAYLIST_SEARCH_CASE_SENSITIVE)
		return !strMatch(haystack, needle, 0);
	else
		return !strMatch(haystack, needle, 1);
}


int playlistLock (PLAYLISTCACHE *plc)
//int _playlistLock (PLAYLISTCACHE *plc, const char *func, const int line)
{
	//printf("playlistLock: WAIT %X, %i, %i, (%s:%i)\n", plc->uid, plc->hMutex->lockCount, (int)GetCurrentThreadId(), func, line);
	const int ret = lockWait(plc->hMutex, INFINITE);
	/*if (ret > 0)
		printf("playlistLock: GOT: %X, %i, %i, (%s:%i)\n", plc->uid, plc->hMutex->lockCount, (int)GetCurrentThreadId(), func, line);
	else
		printf("playlistLock: FAILED: %X, %i, %i, (%s:%i)\n", plc->uid, plc->hMutex->lockCount, (int)GetCurrentThreadId(), func, line);*/
	
	return ret;
}

void playlistUnlock (PLAYLISTCACHE *plc)
//void _playlistUnlock (PLAYLISTCACHE *plc, const char *func, const int line)
{
	//printf("playlistUnlock: RELEASE %X, %i, %i, (%s:%i)\n", plc->uid, plc->hMutex->lockCount, (int)GetCurrentThreadId(), func, line);
	lockRelease(plc->hMutex);
}

static inline TPLAYLISTRECORD *_playlistGetRoot (PLAYLISTCACHE *plc)
{
	return plc->first;
}

static inline TPLAYLISTRECORD *_playlistGetLast (PLAYLISTCACHE *plc)
{
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	if (rec){
		while(rec->next)
			rec = rec->next;
	}
	return rec;
}

static inline int _playlistInsertLast (PLAYLISTCACHE *plc, TPLAYLISTITEM *item)
{
	TPLAYLISTRECORD *newrec = (TPLAYLISTRECORD*)my_malloc(sizeof(TPLAYLISTRECORD));
	if (!newrec) return -1;

	//printf("_playlistInsertLast %X %i %p, %p, %p %p\n", plc->uid, plc->total, item, newrec, plc->first, plc->last);
	
	newrec->next = NULL;
	newrec->item = item;
	
	TPLAYLISTRECORD *rec = _playlistGetLast(plc);
	//TPLAYLISTRECORD *rec = plc->last;
	
	//if (rec != _playlistGetLast(plc))
	//	printf("%p %p\n", rec, _playlistGetLast(plc));
	
	if (!plc->first){
		plc->first = newrec;
		plc->last = newrec;
		plc->total = 1;
		return plc->total-1;
	}

	if (rec)
		rec->next = newrec;
	else
		plc->first = newrec;

	plc->last = newrec;
	return plc->total++;
}

static inline TPLAYLISTITEM *_playlistGetItemById (PLAYLISTCACHE *plc, const int id)
{
	if (id < PLAYLIST_TRACK_BASEIDENT) return 0;

	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	while(rec){
		if (rec->item && rec->item->tid == id)
			return rec->item;
		rec = rec->next;
	}
	return NULL;
}

static inline TPLAYLISTITEM *_playlistGetItem (PLAYLISTCACHE *plc, const int pos)
{
	int idx = 0;
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	while(rec){
		if (idx++ == pos) return rec->item;
		rec = rec->next;
	}
	return NULL;
}

TPLAYLISTITEM *g_playlistGetItem (PLAYLISTCACHE *plc, const int pos)
{
	return _playlistGetItem(plc, pos);
}

TPLAYLISTITEM *playlistGetItem (PLAYLISTCACHE *plc, const int pos)
{
	TPLAYLISTITEM *ret = NULL;
	if (playlistLock(plc)){
		ret = _playlistGetItem(plc, pos);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistGetItemType (PLAYLISTCACHE *plc, const int pos)
{
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	if (item)
		return item->objType;
	else
		return 0;
}

int g_playlistGetItemType (PLAYLISTCACHE *plc, const int pos)
{
	return _playlistGetItemType(plc, pos);
}

int playlistGetItemType (PLAYLISTCACHE *plc, const int pos)
{
	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistGetItemType(plc, pos);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistIsChildOfParent (const PLAYLISTCACHE *child, const PLAYLISTCACHE *parent)
{
	if (child){
		if (child == parent)
			return 1;
		else
			return _playlistIsChildOfParent(child->parent, parent);
	}
	return 0;
}

int playlistIsChild (PLAYLISTCACHE *plcA, PLAYLISTCACHE *plcB)
{
	if (!plcA || !plcB) return 0;
	
	int ret = 0;
	if (playlistLock(plcA)){
		if (playlistLock(plcB)){
			ret = _playlistIsChildOfParent(plcA, plcB);
			playlistUnlock(plcB);
		}
		playlistUnlock(plcA);
	}
/*
	if (plcA && plcB)
		printf("playlistIsChild %i %X %X\n", ret, plcA->uid, plcB->uid);
	else if (plcA)
		printf("playlistIsChild %i plcA %X\n", ret, plcA->uid);
	else if (plcB)
		printf("playlistIsChild %i plcB %X\n", ret, plcB->uid);
*/
	return ret;
}

static inline TPLAYLISTITEM *_playlistCreateItem (PLAYLISTCACHE *plc, const void *objVar, const int type)
{
	TPLAYLISTITEM *item = (TPLAYLISTITEM*)my_calloc(1, sizeof(TPLAYLISTITEM));
	if (item){
		item->objType = type;
		item->tid = (plc->uid<<16)|(++plc->idSrc);
		
		if (item->objType == PLAYLIST_OBJTYPE_TRACK){
			item->obj.track.path = my_strdup((char*)objVar);
			item->obj.track.hash = getHash((char*)objVar);
			//item->obj.track.title = NULL;
			//item->obj.track.opts = NULL;
			//item->obj.track.artId = 0;
			//item->metaRetryCt = 0;
			//item->artRetryCt = 0;
			
		}else if (item->objType == PLAYLIST_OBJTYPE_PLC){
			item->obj.plc = (PLAYLISTCACHE*)objVar;
			item->obj.plc->parent = plc;
			//item->obj.plc->idSrc = PLAYLIST_TRACK_BASEIDENT;
			//item->metaRetryCt = 0;
			//item->artRetryCt = 0;
		}else{
			my_free(item);
			return NULL;
		}
	}
	return item;
}

static inline void _playlistFreeItem (TPLAYLISTITEM *item)
{
	if (item->objType == PLAYLIST_OBJTYPE_TRACK){
		if (item->obj.track.path) my_free(item->obj.track.path);	
		if (item->obj.track.title) my_free(item->obj.track.title);
		if (item->obj.track.opts) my_free(item->obj.track.opts);

	}else if (item->objType == PLAYLIST_OBJTYPE_PLC){
		playlistFree(item->obj.plc);
	}
	my_free(item);
}

static inline int _playlistAddPlc (PLAYLISTCACHE *plc, const PLAYLISTCACHE *childPlaylist)
{
	if (plc == childPlaylist) return 0;

	TPLAYLISTITEM *item = _playlistCreateItem(plc, childPlaylist, PLAYLIST_OBJTYPE_PLC);
	if (item)
		return _playlistInsertLast(plc, item);
	else
		return -1;
}

int playlistAddPlc (PLAYLISTCACHE *plc, const PLAYLISTCACHE *childPlaylist)
{
#if 0
	if (playlistIsChild(plc,(PLAYLISTCACHE*)childPlaylist) || playlistIsChild((PLAYLISTCACHE*)childPlaylist,plc)){
		printf("playlistAddPlc: cannot double link a playlist: %X %X\n", plc->uid, childPlaylist->uid);
		return 0;
	}
#endif

	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistAddPlc(plc, childPlaylist);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistGetCount (PLAYLISTCACHE *plc, const int type)
{
	int ct = 0;
	for (int i = 0; i < plc->total; i++){
		if (_playlistGetItemType(plc, i) == type)
			ct++;
	}
	return ct;
}

int playlistGetCount (PLAYLISTCACHE *plc, const int type)
{
	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistGetCount(plc, type);
		playlistUnlock(plc);
	}
	return ret;
}

// there can be multiple instances of the same track within a playlist
static inline int _playlistAddTrack (PLAYLISTCACHE *plc, const char *path)
{
	TPLAYLISTITEM *item = _playlistCreateItem(plc, path, PLAYLIST_OBJTYPE_TRACK);
	if (item)
		return _playlistInsertLast(plc, item);
	else
		return -1;
}

int playlistAdd (PLAYLISTCACHE *plc, const char *path)
{
	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistAddTrack(plc, path);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistGetTotal (PLAYLISTCACHE *plc)
{
	if (plc)
		return plc->total;
	else
		return 0;
}

// just print everything
/*
int playlistRunList (PLAYLISTCACHE *plc)
{
	if (!playlistLock(plc))
		return 0;
		
	int total = 0;
	char *path;
	char *title;
	
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	if (rec){
		do{
			path = rec->item->path;
			if (!path) path = "no path";
			title = rec->item->title;
			if (!title) title = "no title";
			
			printf("%i: %X '%s' '%s'\n", total++, rec->item->hash, path, title);
		}while((rec=rec->next));
	}
	
	playlistUnlock(plc);
	return total;
}
*/
static inline int _playlistIdToIdx (PLAYLISTCACHE *plc, const int tid)
{
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	if (rec){
		int idx = 0;
		
		do{
			if (rec->item && rec->item->tid == tid)
				return idx;
			idx++;
		}while((rec=rec->next));
	}
	return -1;
}

static inline TPLAYLISTRECORD * _playlistGetRecordById (PLAYLISTCACHE *plc, const int tid)
{
	if (tid < PLAYLIST_TRACK_BASEIDENT) return NULL;
	
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	if (rec){
		do{
			if (rec->item && rec->item->tid == tid)
				return rec;
		}while((rec=rec->next));
	}
	return NULL;
}

static inline TPLAYLISTRECORD * _playlistGetRecord (PLAYLISTCACHE *plc, const int pos)
{
	if (pos >= 0){
		TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
		if (rec){
			int idx = 0;
			do{
				if (idx == pos)
					return rec;
				idx++;
			}while((rec=rec->next));
		}
	}
	return NULL;
}

static inline int _playlistInsertById (PLAYLISTCACHE *plc, TPLAYLISTRECORD *newRec, const int tid)
{
	if (tid < PLAYLIST_TRACK_BASEIDENT) return 0;
	
	const int pos = _playlistIdToIdx(plc, tid);
	
	// special case for first
	if (!pos){
		//printf("insert first\n");
		TPLAYLISTRECORD *first = _playlistGetRoot(plc);
		plc->first = newRec;
		newRec->next = first;
		return 1;
	}
	
	// special case for last
	if (pos > plc->total-1){
		//printf("insert last %i\n", plc->total);
		newRec->next = NULL;
		TPLAYLISTRECORD *rec = _playlistGetLast(plc);
		//TPLAYLISTRECORD *rec = plc->last;
		
		if (rec)
			rec->next = newRec;
		else
			plc->first = newRec;
		plc->last = newRec;
		return 1;
	}

	TPLAYLISTRECORD *recPre = _playlistGetRecord(plc, pos-1);	
	TPLAYLISTRECORD *rec = _playlistGetRecord(plc, pos);

	if (rec){
		newRec->next = rec;
		recPre->next = newRec;
		return 1;
	}
	return 0;
}

static inline int _playlistInsert (PLAYLISTCACHE *plc, TPLAYLISTRECORD *newRec, int pos)
{
	//printf("_playlistInsert %X '%s' %i\n", plc->uid, plc->title, pos);
	//if (pos >= plc->total) pos = plc->total-1;
	
	// special case for first
	if (!pos){
		//printf("_playlistInsert (First) %p %p\n", plc->first, plc->last);
		TPLAYLISTRECORD *first = _playlistGetRoot(plc);
		plc->first = newRec;
		newRec->next = first;
		return 1;
	}
	
	// special case for last
	if (pos > plc->total-1){
		//printf("_playlistInsert (Last) %i %p %p %p\n", plc->total, plc->first, plc->first->next, plc->last);
				
		newRec->next = NULL;
		TPLAYLISTRECORD *rec = _playlistGetLast(plc);
		//TPLAYLISTRECORD *rec = plc->last;
		
		if (rec)
			rec->next = newRec;
		else
			plc->first = newRec;
		plc->last = newRec;
		return 1;
	}

	TPLAYLISTRECORD *recPre = _playlistGetRecord(plc, pos-1);	
	TPLAYLISTRECORD *rec = _playlistGetRecord(plc, pos);

	if (rec){
		newRec->next = rec;
		recPre->next = newRec;
		return 1;
	}
	return 0;
}

int playlistInsert (PLAYLISTCACHE *plc, TPLAYLISTRECORD *newRec, const int pos)
{
	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistInsert(plc, newRec, pos);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistUnlinkRecord (PLAYLISTCACHE *plc, TPLAYLISTRECORD *recunlink)
{
	//printf("_playlistUnlinkRecord %X %p\n", plc->uid, recunlink);
	
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	if (rec){
		if (rec == recunlink){	// handle special case that be root
			plc->first = rec->next;
			return 1;
		}

		do{
			// remove the record by relinking (skipping) right past it
			if (rec->next == recunlink){
				rec->next = recunlink->next;
				
				if (plc->last == recunlink){
					plc->last = rec;
				}
				return 1;
			}
		}while((rec=rec->next));
	}
	return 0;
}

static inline TPLAYLISTRECORD * _playlistRemoveRecord (PLAYLISTCACHE *plc, const int pos)
{
	TPLAYLISTRECORD *rec = _playlistGetRecord(plc, pos);
	if (rec)
		_playlistUnlinkRecord(plc, rec);
	return rec;
}


TPLAYLISTRECORD * playlistRemoveRecord (PLAYLISTCACHE *plc, const int pos)
{
	TPLAYLISTRECORD *ret = NULL;
	if (playlistLock(plc)){
		ret = _playlistRemoveRecord(plc, pos);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistMoveById (PLAYLISTCACHE *plcFrom, PLAYLISTCACHE *plcTo, const int fromId, const int toId)
{
	TPLAYLISTRECORD *recFrom = _playlistGetRecordById(plcFrom, fromId);
	if (!recFrom){
		//printf("playlistMove(): invalid rec %i\n", fromIdx);
		return 0;
	}

	if (_playlistUnlinkRecord(plcFrom, recFrom)){
		//printf("_playlistUnlinkRecord\n");

		//if (plcFrom == plcTo)
			plcFrom->total--;

		if (_playlistInsertById(plcTo, recFrom, toId)){
			//printf("playlist item %i:'%s' moved to %i:'%s'\n", fromIdx, plcFrom->title, toIdx, plcTo->title);
			
			//if (plcFrom != plcTo){
			//	plcFrom->total--;
				plcTo->total++;
			//}else{
			//	plcTo->total++;
			//}
			return 1;
		}else{
			//printf("_playlistInsert failed\n");
		}
	}
	return 0;
}

int playlistMoveById (PLAYLISTCACHE *plcFrom, PLAYLISTCACHE *plcTo, const int fromId, const int toId)
{
	if (plcFrom == plcTo && fromId == toId)
		return 1;

	int ret = 0;
	if (playlistLock(plcFrom)){
		if (playlistLock(plcTo)){
			ret = _playlistMoveById(plcFrom, plcTo, fromId, toId);
			playlistUnlock(plcTo);
		}
		playlistUnlock(plcFrom);
	}
	return ret;
}

static inline int _playlistMove (PLAYLISTCACHE *plcFrom, PLAYLISTCACHE *plcTo, int fromIdx, int toIdx)
{
	TPLAYLISTRECORD *recFrom = _playlistGetRecord(plcFrom, fromIdx);
	if (!recFrom){
		//printf("playlistMove(): invalid rec %i\n", fromIdx);
		return 0;
	}

	if (_playlistUnlinkRecord(plcFrom, recFrom)){
		//printf("_playlistUnlinkRecord\n");

		//if (plcFrom == plcTo)
			plcFrom->total--;

		if (_playlistInsert(plcTo, recFrom, toIdx)){
			//printf("playlist item %i:'%s' moved to %i:'%s'\n", fromIdx, plcFrom->title, toIdx, plcTo->title);
			
			//if (plcFrom != plcTo){
			//	plcFrom->total--;
				plcTo->total++;
			//}else{
			//	plcTo->total++;
			//}
			return 1;
		}else{
			//printf("_playlistInsert failed\n");
		}
	}
	return 0;
}

int playlistMove (PLAYLISTCACHE *plcFrom, PLAYLISTCACHE *plcTo, const int fromIdx, const int toIdx)
{
	if (plcFrom == plcTo && fromIdx == toIdx)
		return 1;

	int ret = 0;
	if (playlistLock(plcFrom)){
		if (playlistLock(plcTo)){
			ret = _playlistMove(plcFrom, plcTo, fromIdx, toIdx);
			playlistUnlock(plcTo);
		}
		playlistUnlock(plcFrom);
	}
	return ret;
}

static inline int _playlistCopyTrackById (PLAYLISTCACHE *plcFrom, const int fromId, PLAYLISTCACHE *plcTo, const int toId)
{
	//if (tid < PLAYLIST_TRACK_BASEIDENT) return 0;
	int destId = 0;
	
	TPLAYLISTITEM *item = _playlistGetItemById(plcFrom, fromId);
	if (item && item->objType == PLAYLIST_OBJTYPE_TRACK){
		TPLAYLISTITEM *itemNew = _playlistCreateItem(plcTo, item->obj.track.path, PLAYLIST_OBJTYPE_TRACK);
		if (itemNew){
			itemNew->obj.track.hash = item->obj.track.hash;
			itemNew->obj.track.artId = item->obj.track.artId;
			if (item->obj.track.title)
				itemNew->obj.track.title = my_strdup(item->obj.track.title);
			if (item->obj.track.opts)
				itemNew->obj.track.opts = my_strdup(item->obj.track.opts);
				
			TPLAYLISTRECORD *rec = my_calloc(1, sizeof(TPLAYLISTRECORD));
			if (rec){
				rec->item = itemNew;
				if (_playlistInsertById(plcTo, rec, toId) >= 0){
					plcTo->total++;
					destId = itemNew->tid;
				}
			}
			if (!destId)
				_playlistFreeItem(itemNew);
		}
	}
	return destId;
}

int playlistCopyTrackById (PLAYLISTCACHE *plcFrom, const int fromId, PLAYLISTCACHE *plcTo, const int toId)
{
	if (/*plcFrom == plcTo ||*/ fromId < PLAYLIST_TRACK_BASEIDENT || toId < PLAYLIST_TRACK_BASEIDENT)
		return -1;

	int ret = -1;
	if (playlistLock(plcFrom)){
		if (playlistLock(plcTo)){
			ret = _playlistCopyTrackById(plcFrom, fromId, plcTo, toId);
			playlistUnlock(plcTo);
		}
		playlistUnlock(plcFrom);
	}
	return ret;
}

static inline int _playlistCopyTrack (PLAYLISTCACHE *plcFrom, const int pos, PLAYLISTCACHE *plcTo)
{
	int ret = -1;
	
	TPLAYLISTITEM *item = _playlistGetItem(plcFrom, pos);
	if (item && item->objType == PLAYLIST_OBJTYPE_TRACK){
		TPLAYLISTITEM *itemNew = _playlistCreateItem(plcTo, item->obj.track.path, PLAYLIST_OBJTYPE_TRACK);
		if (itemNew){
			itemNew->obj.track.hash = item->obj.track.hash;
			itemNew->obj.track.artId = item->obj.track.artId;
			if (item->obj.track.title)
				itemNew->obj.track.title = my_strdup(item->obj.track.title);
			if (item->obj.track.opts)
				itemNew->obj.track.opts = my_strdup(item->obj.track.opts);
			ret = _playlistInsertLast(plcTo, itemNew);
		}
	}
	return ret;
}

int playlistCopyTrack (PLAYLISTCACHE *plcFrom, const int pos, PLAYLISTCACHE *plcTo)
{
	if (plcFrom == plcTo || pos < 0 || pos >= plcFrom->total)
		return -1;

	int ret = -1;
	if (playlistLock(plcFrom)){
		if (playlistLock(plcTo)){
			ret = _playlistCopyTrack(plcFrom, pos, plcTo);
			playlistUnlock(plcTo);
		}
		playlistUnlock(plcFrom);
	}
	return ret;
}

static inline int _playlistSearch_tag (PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, const int tag, const char *str, const int from, const int searchHow)
{
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	if (rec){
		char buffer[MAX_PATH_UTF8];
		int idx = 0;
		
		if (from > 0){
			do{
				if (idx == from) break;
				idx++;
			}while((rec=rec->next));
		}
		
		// TODO, add deep search of playlists' (plc->obj.plc)

		do{
			if (rec->item && rec->item->objType == PLAYLIST_OBJTYPE_TRACK){
				tagRetrieveByHash(tagc, rec->item->obj.track.hash, tag, buffer, MAX_PATH_UTF8);
				if (*buffer){
					if (strFind(buffer, str, searchHow))
						return idx;
				}
				//idx++;
			}
			idx++;
		}while((rec=rec->next));
	}
	return -1;
}

static inline int _playlistSearch_path (PLAYLISTCACHE *plc, const char *str, const int from, const int searchHow)
{
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	if (rec){
		int idx = 0;
		
		if (from > 0){
			do{
				if (idx == from) break;
				idx++;
			}while((rec=rec->next));
		}

		do{
			if (rec->item && rec->item->objType == PLAYLIST_OBJTYPE_TRACK){
				if (strFind(rec->item->obj.track.path, str, searchHow))
					return idx;
				//idx++;
			}
			idx++;
		}while((rec=rec->next));
	}
	return -1;
}

static inline PLAYLISTCACHE *_playlistSearch_title_plc (PLAYLISTCACHE *plc, const char *str, const int from, const int searchHow)
{
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	if (rec){
		int idx = 0;
		
		if (from > 0){
			do{
				if (idx == from) break;
				idx++;
			}while((rec=rec->next));
		}

		do{
			if (rec->item && rec->item->objType == PLAYLIST_OBJTYPE_PLC){
				if (rec->item->obj.plc->title && strFind(rec->item->obj.plc->title, str, searchHow))
					return rec->item->obj.plc;
				//idx++;
			}
			idx++;
		}while((rec=rec->next));
	}
	return NULL;
}

PLAYLISTCACHE *playlistGetPlaylistByName (PLAYLISTCACHE *plc, const char *title, const int from)
{
	PLAYLISTCACHE *ret = NULL;
	if (playlistLock(plc)){
		if (from >= 0 && from < _playlistGetTotal(plc))
			ret = _playlistSearch_title_plc(plc, title, from, PLAYLIST_SEARCH_CASE_DEFAULT);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistSearch_title (PLAYLISTCACHE *plc, const char *str, const int from, const int searchHow)
{
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	if (rec){
		int idx = 0;
		
		if (from > 0){
			do{
				if (idx == from) break;
				idx++;
			}while((rec=rec->next));
		}

		do{
			if (rec->item && rec->item->objType == PLAYLIST_OBJTYPE_TRACK){
				if (rec->item->obj.track.title && strFind(rec->item->obj.track.title, str, searchHow))
					return idx;
			}else if (rec->item && rec->item->objType == PLAYLIST_OBJTYPE_PLC){
				if (rec->item->obj.plc->title && strFind(rec->item->obj.plc->title, str, searchHow))
					return idx;
			}
			idx++;
		}while((rec=rec->next));
	}
	return -1;
}

int playlistSearchEx (PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, const char *str, const int from, int *foundIn, const int searchHow)
{
	int ret = -1;
	*foundIn = -1;

	if (from >= 0 && from < _playlistGetTotal(plc)){
		*foundIn = MTAG_Title;
		ret = _playlistSearch_title(plc, str, from, searchHow);
#if 1
		if (ret == -1){
			*foundIn = MTAG_PATH;
			ret = _playlistSearch_path(plc, str, from, searchHow);
		}
		if (ret == -1){
			*foundIn = MTAG_Artist;
			ret = _playlistSearch_tag(plc, tagc, MTAG_Artist, str, from, searchHow);
		}
		if (ret == -1){
			*foundIn = MTAG_Album;
			ret = _playlistSearch_tag(plc, tagc, MTAG_Album, str, from, searchHow);
		}
		if (ret == -1){
			*foundIn = MTAG_Genre;
			ret = _playlistSearch_tag(plc, tagc, MTAG_Genre, str, from, searchHow);
		}
		if (ret == -1){
			*foundIn = MTAG_Description;
			ret = _playlistSearch_tag(plc, tagc, MTAG_Description, str, from, searchHow);
		}
		if (ret == -1){
			*foundIn = MTAG_Date;
			ret = _playlistSearch_tag(plc, tagc, MTAG_Date, str, from, searchHow);
		}
		if (ret == -1){
			*foundIn = MTAG_LENGTH;
			ret = _playlistSearch_tag(plc, tagc, MTAG_LENGTH, str, from, searchHow);
		}
		if (ret == -1){
			*foundIn = MTAG_NowPlaying;
			ret = _playlistSearch_tag(plc, tagc, MTAG_NowPlaying, str, from, searchHow);
		}
#endif
	}
	if (ret == -1) *foundIn = -1;
	return ret;
}

int playlistSearch (PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, const char *str, const int from)
{
	int ret = -1;
	if (playlistLock(plc)){
		if (from >= 0 && from < _playlistGetTotal(plc)){
			ret = _playlistSearch_title(plc, str, from, PLAYLIST_SEARCH_CASE_DEFAULT);
			if (ret == -1)
				ret = _playlistSearch_path(plc, str, from, PLAYLIST_SEARCH_CASE_DEFAULT);
			if (ret == -1)
				ret = _playlistSearch_tag(plc, tagc, MTAG_Artist, str, from, PLAYLIST_SEARCH_CASE_DEFAULT);
			if (ret == -1)
				ret = _playlistSearch_tag(plc, tagc, MTAG_Album, str, from, PLAYLIST_SEARCH_CASE_DEFAULT);
			if (ret == -1)
				ret = _playlistSearch_tag(plc, tagc, MTAG_Genre, str, from, PLAYLIST_SEARCH_CASE_DEFAULT);
			if (ret == -1)
				ret = _playlistSearch_tag(plc, tagc, MTAG_Description, str, from, PLAYLIST_SEARCH_CASE_DEFAULT);	
			if (ret == -1)
				ret = _playlistSearch_tag(plc, tagc, MTAG_Date, str, from, PLAYLIST_SEARCH_CASE_DEFAULT);
			if (ret == -1)
				ret = _playlistSearch_tag(plc, tagc, MTAG_LENGTH, str, from, PLAYLIST_SEARCH_CASE_DEFAULT);
		}
		playlistUnlock(plc);
	}
	return ret;
}

int playlistSearchTag (PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, const int mtag, const char *str, const int from)
{
	int ret = -1;
	if (playlistLock(plc)){
		if (from >= 0 && from < _playlistGetTotal(plc)){
			if (mtag == MTAG_PATH)
				ret = _playlistSearch_path(plc, str, from, PLAYLIST_SEARCH_CASE_DEFAULT);
			else if (mtag == MTAG_Title)
				ret = _playlistSearch_title(plc, str, from, PLAYLIST_SEARCH_CASE_DEFAULT);
			else if (mtag >= 0 && mtag < MTAG_TOTAL)
				ret = _playlistSearch_tag(plc, tagc, mtag, str, from, PLAYLIST_SEARCH_CASE_DEFAULT);
		}			
		playlistUnlock(plc);
	}
	return ret;
}
 
int playlistGetTotal (PLAYLISTCACHE *plc)
{
	int ret = 0;

	if (plc){
		if (playlistLock(plc)){
			ret = _playlistGetTotal(plc);
			playlistUnlock(plc);
		}
	}
	return ret;
}

static inline int _playlistGetPlaylistUIDById (PLAYLISTCACHE *plc, const int trackId)
{
	TPLAYLISTITEM *item = _playlistGetItemById(plc, trackId);
	if (item && item->objType == PLAYLIST_OBJTYPE_PLC)
		return item->obj.plc->uid;
	else
		return 0;
}

int playlistGetPlaylistUIDById (PLAYLISTCACHE *plc, const int trackId)
{
	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistGetPlaylistUIDById(plc, trackId);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistGetPlaylistUID (PLAYLISTCACHE *plc, const int position)
{
	TPLAYLISTITEM *item = _playlistGetItem(plc, position);
	if (item && item->objType == PLAYLIST_OBJTYPE_PLC)
		return item->obj.plc->uid;
	else
		return 0;
}

int playlistGetPlaylistUID (PLAYLISTCACHE *plc, const int position)
{
	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistGetPlaylistUID(plc, position);
		playlistUnlock(plc);
	}
	return ret;
}

static inline PLAYLISTCACHE *_playlistGetPlaylist (PLAYLISTCACHE *plc, const int position)
{
	TPLAYLISTITEM *item = playlistGetItem(plc, position);
	if (item && item->objType == PLAYLIST_OBJTYPE_PLC)
		return item->obj.plc;
	else
		return NULL;
}

PLAYLISTCACHE *playlistGetPlaylist (PLAYLISTCACHE *plc, const int position)
{
	PLAYLISTCACHE *ret = NULL;
	if (playlistLock(plc)){
		ret = _playlistGetPlaylist(plc, position);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistGetPlaylistIndex (PLAYLISTCACHE *plc, PLAYLISTCACHE *child)
{
	if (child){
		TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
		if (rec){
			int idx = 0;
			
			do{
				if (rec->item && rec->item->objType == PLAYLIST_OBJTYPE_PLC){
					if (rec->item->obj.plc == child)
						return idx;
				}
				idx++;
			}while((rec=rec->next));
		}
	}
	return -1;
}

int playlistGetPlaylistIndex (PLAYLISTCACHE *plc, PLAYLISTCACHE *child)
{
	int ret = -1;
	if (playlistLock(plc)){
		ret = _playlistGetPlaylistIndex(plc, child);
		playlistUnlock(plc);
	}
	return ret;
}

PLAYLISTCACHE *playlistGetParent (PLAYLISTCACHE *plc)
{
	PLAYLISTCACHE *parent = NULL;
	
	if (playlistLock(plc)){
		parent = plc->parent;
		playlistUnlock(plc);
	}
	return parent;
}

static inline int _playlistSetOptions (PLAYLISTCACHE *plc, const int pos, const char *opts, const int overwrite)
{
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	if (item && item->objType == PLAYLIST_OBJTYPE_TRACK){
		if (item->obj.track.opts){
			if (overwrite){
				my_free(item->obj.track.opts);
				item->obj.track.opts = NULL;
			}else{
				return 1;
			}
		}
		if (opts)
			item->obj.track.opts = my_strdup(opts);
		else
			item->obj.track.opts = NULL;
		return 1;
	}
	return 0;
}

int playlistSetOptions (PLAYLISTCACHE *plc, const int pos, const char *opts, const int overwrite)
{
	int ret = 0;
	if (opts){
		if (playlistLock(plc)){
			ret = _playlistSetOptions(plc, pos, opts, overwrite);
			playlistUnlock(plc);
		}
	}
	return ret;
}

static inline char *_playlistGetOptionsDup (PLAYLISTCACHE *plc, const int trackId)
{
	TPLAYLISTITEM *item = _playlistGetItemById(plc, trackId);
	if (item && item->objType == PLAYLIST_OBJTYPE_TRACK && item->obj.track.opts)
		return my_strdup(item->obj.track.opts);
	return NULL;
}

char *playlistGetOptionsDup (PLAYLISTCACHE *plc, const int trackId)
{
	char *ret = NULL;
	if (playlistLock(plc)){
		ret = _playlistGetOptionsDup(plc, trackId);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistGetOptions (PLAYLISTCACHE *plc, const int pos, char *buffer, const size_t len)
{
	*buffer = 0;
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	if (item && item->objType == PLAYLIST_OBJTYPE_TRACK && item->obj.track.opts){
		strncpy(buffer, item->obj.track.opts, len);
		return 1;
	}else{
		return 0;
	}
}

int playlistGetOptions (PLAYLISTCACHE *plc, const int pos, char *buffer, const size_t len)
{
	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistGetOptions(plc, pos, buffer, len);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistSetPath (PLAYLISTCACHE *plc, const int pos, const char *path)
{
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	if (item && item->objType == PLAYLIST_OBJTYPE_TRACK){
		if (item->obj.track.path)
			my_free(item->obj.track.path);
		item->obj.track.path = my_strdup(path);
		item->obj.track.hash = getHash(path);
		return 1;
	}
	return 0;
}

int playlistSetPath (PLAYLISTCACHE *plc, const int pos, const char *path)
{
	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistSetPath(plc, pos, path);
		playlistUnlock(plc);
	}
	return ret;
}

int playlistSetPaths (PLAYLISTCACHE *plc, const char *path)
{
	int ret = 0;
	
	if (playlistLock(plc)){
		char newpath[MAX_PATH_UTF8+1];
		char file[MAX_PATH_UTF8+1];
		char ext[MAX_PATH_UTF8+1];
		
		for (int i = 0; i < plc->total; i++){
			TPLAYLISTITEM *item = _playlistGetItem(plc, i);
			if (item && item->objType == PLAYLIST_OBJTYPE_TRACK){
				char *oldpath = item->obj.track.path;
				if (!oldpath) continue;
				
				_splitpath(oldpath, NULL, NULL, file, ext);
				__mingw_snprintf(newpath, MAX_PATH_UTF8, "%s%s%s", path, file, ext);
				ret += _playlistSetPath(plc, i, newpath);
			}
		}
		
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistSetTitle (PLAYLISTCACHE *plc, const int pos, const char *title, const int overwrite)
{
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	if (item && item->objType == PLAYLIST_OBJTYPE_TRACK){
		//printf("## _playlistSetTitle %X %i '%s' %i\n", plc->uid, pos, title, overwrite);
		if (item->obj.track.title){
			if (overwrite)
				my_free(item->obj.track.title);
			else
				return 1;
		}
		item->obj.track.title = my_strdup(title);
		return 1;
	}
	return 0;
}

int playlistSetTitle (PLAYLISTCACHE *plc, const int pos, const char *title, const int overwrite)
{
	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistSetTitle(plc, pos, title, overwrite);
		playlistUnlock(plc);
	}
	return ret;
}

static inline char *_playlistGetTitle (PLAYLISTCACHE *plc, const int pos, char *buffer, const size_t len)
{
	*buffer = 0;
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	if (item){
		if (item->objType == PLAYLIST_OBJTYPE_TRACK && item->obj.track.title)
			strncpy(buffer, item->obj.track.title, len);
		else
			if (item->objType == PLAYLIST_OBJTYPE_PLC && item->obj.plc){
				if (item->obj.plc->title && *item->obj.plc->title)
					strncpy(buffer, item->obj.plc->title, len);
			}
	}
	return buffer;
}

char *playlistGetTitle (PLAYLISTCACHE *plc, const int pos, char *buffer, const size_t len)
{
	char *ret = NULL;
	if (playlistLock(plc)){
		ret = _playlistGetTitle(plc, pos, buffer, len);
		playlistUnlock(plc);
	}
	return ret;
}

static inline char *_playlistGetTitleDupById (PLAYLISTCACHE *plc, const int trackId)
{
	TPLAYLISTITEM *item = _playlistGetItemById(plc, trackId);
	if (item){
		if (item->objType == PLAYLIST_OBJTYPE_TRACK && item->obj.track.title){
			if (*item->obj.track.title)
				return my_strdup(item->obj.track.title);
			else
				return my_strdup(" ");
		}else if (item->objType == PLAYLIST_OBJTYPE_PLC && item->obj.plc){
			if (item->obj.plc->title && *item->obj.plc->title)
				return my_strdup(item->obj.plc->title);
			else
				return my_strdup(" ");
		}
	}
	return NULL;
}

char *playlistGetTitleDupById (PLAYLISTCACHE *plc, const int trackId)
{
	char *ret = NULL;
	if (playlistLock(plc)){
		ret = _playlistGetTitleDupById(plc, trackId);
		playlistUnlock(plc);
	}
	return ret;
}

static inline char *_playlistGetTitleDup (PLAYLISTCACHE *plc, const int pos)
{
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	if (item){
		if (item->objType == PLAYLIST_OBJTYPE_TRACK && item->obj.track.title){
			if (*item->obj.track.title)
				return my_strdup(item->obj.track.title);
			else
				return my_strdup(" ");
		}else if (item->objType == PLAYLIST_OBJTYPE_PLC && item->obj.plc){
			if (item->obj.plc->title && *item->obj.plc->title)
				return my_strdup(item->obj.plc->title);
			else
				return my_strdup(" ");
		}
	}
	return NULL;
}

char *playlistGetTitleDup (PLAYLISTCACHE *plc, const int pos)
{
	char *ret = NULL;
	if (playlistLock(plc)){
		ret = _playlistGetTitleDup(plc, pos);
		playlistUnlock(plc);
	}
	return ret;
}

static inline char *_playlistGetPath (PLAYLISTCACHE *plc, const int pos, char *buffer, const size_t len)
{
	*buffer = 0;
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	
	if (item && item->objType == PLAYLIST_OBJTYPE_TRACK && item->obj.track.path)
		strncpy(buffer, item->obj.track.path, len);
	return buffer;
}

char *playlistGetPath (PLAYLISTCACHE *plc, const int pos, char *buffer, const size_t len)
{
	char *ret = NULL;
	if (playlistLock(plc)){
		ret = _playlistGetPath(plc, pos, buffer, len);
		playlistUnlock(plc);
	}
	return ret;
}

static inline char *_playlistGetPathDup (PLAYLISTCACHE *plc, const int pos)
{
	char *path = NULL;
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	
	if (item && item->objType == PLAYLIST_OBJTYPE_TRACK && item->obj.track.path)
		path = my_strdup(item->obj.track.path);
	return path;
}

char *playlistGetPathDup (PLAYLISTCACHE *plc, const int pos)
{
	char *ret = NULL;
	if (playlistLock(plc)){
		ret = _playlistGetPathDup(plc, pos);
		playlistUnlock(plc);
	}
	return ret;
}

static inline wchar_t *_playlistGetPathDupW (PLAYLISTCACHE *plc, const int pos)
{
	wchar_t *path = NULL;
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	
	if (item && item->objType == PLAYLIST_OBJTYPE_TRACK && item->obj.track.path)
		path = converttow(item->obj.track.path);
	return path;
}

wchar_t *playlistGetPathDupW (PLAYLISTCACHE *plc, const int pos)
{
	wchar_t *ret = NULL;
	if (playlistLock(plc)){
		ret = _playlistGetPathDupW(plc, pos);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistSetName (PLAYLISTCACHE *plc, char *buffer)
{
	if (plc->title)
		my_free(plc->title);
	plc->title = my_strdup(buffer);
	return plc->title != NULL;
}

int playlistSetName (PLAYLISTCACHE *plc, char *buffer)
{
	int ret = 0;
	if (playlistLock(plc)){
		if (buffer && strlen(buffer) > 0)
			ret = _playlistSetName(plc, buffer);
		playlistUnlock(plc);
	}
	return ret;
}

static inline char *_playlistGetName (PLAYLISTCACHE *plc, char *buffer, const size_t len)
{
	*buffer = 0;
	strncpy(buffer, plc->title, len);
	return buffer;
}

char *playlistGetName (PLAYLISTCACHE *plc, char *buffer, const size_t len)
{
	char *ret = NULL;
	if (playlistLock(plc)){
		ret = _playlistGetName(plc, buffer, len);
		playlistUnlock(plc);
	}
	return ret;
}

char *playlistGetNameDup (PLAYLISTCACHE *plc)
{
	char *ret = NULL;
	if (playlistLock(plc)){
		ret = my_strdup(plc->title);
		playlistUnlock(plc);
	}
	return ret;
}

static inline unsigned int _playlistGetHashById (PLAYLISTCACHE *plc, const int trackId)
{
	TPLAYLISTITEM *item = _playlistGetItemById(plc, trackId);
	if (item && item->objType == PLAYLIST_OBJTYPE_TRACK)
		return item->obj.track.hash;
	else
		return 0;
}

unsigned int playlistGetHashById (PLAYLISTCACHE *plc, const int trackId)
{
	unsigned int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistGetHashById(plc, trackId);
		playlistUnlock(plc);
	}
	return ret;
}

static inline unsigned int _playlistGetHash (PLAYLISTCACHE *plc, const int pos)
{
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	if (item && item->objType == PLAYLIST_OBJTYPE_TRACK)
		return item->obj.track.hash;
	else
		return 0;
}

unsigned int g_playlistGetHash (PLAYLISTCACHE *plc, const int pos)
{
	return _playlistGetHash(plc, pos);
}

unsigned int playlistGetHash (PLAYLISTCACHE *plc, const int pos)
{
	unsigned int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistGetHash(plc, pos);
		playlistUnlock(plc);
	}
	return ret;
}
 
static inline int _playlistGetArtIdById (PLAYLISTCACHE *plc, const int trackId)
{
	TPLAYLISTITEM *item = _playlistGetItemById(plc, trackId);
	if (item){
		if (item->objType == PLAYLIST_OBJTYPE_TRACK)
			return item->obj.track.artId;
		else if (item->objType == PLAYLIST_OBJTYPE_PLC)
			return item->obj.plc->artId;
	}
	return 0;
}

int playlistGetArtIdById (PLAYLISTCACHE *plc, const int trackId)
{
	unsigned int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistGetArtIdById(plc, trackId);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistGetArtId (PLAYLISTCACHE *plc, const int pos)
{
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	if (item){
		if (item->objType == PLAYLIST_OBJTYPE_TRACK)
			return item->obj.track.artId;
		else if (item->objType == PLAYLIST_OBJTYPE_PLC)
			return item->obj.plc->artId;
	}
	return 0;
}

int playlistGetArtId (PLAYLISTCACHE *plc, const int pos)
{
	unsigned int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistGetArtId(plc, pos);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistSetArtId (PLAYLISTCACHE *plc, const int pos, const int artId, const int overwrite)
{
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	if (!item) return 0;

	if (item->objType == PLAYLIST_OBJTYPE_TRACK){
		if (!item->obj.track.artId || overwrite){
			item->obj.track.artId = artId;
			return 1;
		}else{
			return -1;
		}
		
	}else if (item->objType == PLAYLIST_OBJTYPE_PLC){
		item->obj.plc->artId = artId;
		return 1;
	}
	return 0;
}

int playlistSetArtId (PLAYLISTCACHE *plc, const int pos, const int artId, const int overwrite)
{
	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistSetArtId(plc, pos, artId, overwrite);
		if (ret > 0){
			if (!plc->artId)
				plc->artId = artId;
			if (plc->parent && !plc->parent->artId)
				plc->parent->artId = artId;
		}
		playlistUnlock(plc);
	}
	return ret;
}

int playlistIdToIdx (PLAYLISTCACHE *plc, const int tid)
{
	unsigned int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistIdToIdx(plc, tid);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistGetId (PLAYLISTCACHE *plc, const int pos)
{
	TPLAYLISTITEM *item = _playlistGetItem(plc, pos);
	if (item)
		return item->tid;
	return 0;
}

int playlistGetId (PLAYLISTCACHE *plc, const int pos)
{
	unsigned int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistGetId(plc, pos);
		playlistUnlock(plc);
	}
	return ret;
}

int _playlistGetPrevItem (PLAYLISTCACHE *plc, const unsigned int objType, const int pos)
{
	if (pos < 1) return -1;
	int idx = pos-1;
	
	TPLAYLISTRECORD *rec = _playlistGetRecord(plc, idx);
	while(rec){
		if (rec->item){
			if (rec->item->objType == objType)
				return idx;
		}
		rec = _playlistGetRecord(plc, --idx);
	}
	return -1;
}

int playlistGetPrevItem (PLAYLISTCACHE *plc, const unsigned int objType, const int pos)
{
	int ret = -1;
	if (playlistLock(plc)){
		ret = _playlistGetPrevItem(plc, objType, pos);
		playlistUnlock(plc);
	}
	return ret;
}

int _playlistGetNextItem (PLAYLISTCACHE *plc, const unsigned int objType, const int pos)
{
	int idx = pos+1;
	TPLAYLISTRECORD *rec = _playlistGetRecord(plc, idx);
	
	while(rec){
		if (rec->item){
			if (rec->item->objType == objType)
				return idx;
		}
		idx++;
		rec = rec->next;
	}
	return -1;
}

int playlistGetNextItem (PLAYLISTCACHE *plc, const unsigned int objType, const int pos)
{
	int ret = -1;
	if (playlistLock(plc)){
		ret = _playlistGetNextItem(plc, objType, pos);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistGetPositionByHash (PLAYLISTCACHE *plc, const unsigned int hash)
{
	int idx = 0;
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	while(rec){
		if (rec->item && rec->item->obj.track.hash == hash && rec->item->objType == PLAYLIST_OBJTYPE_TRACK)
			return idx;
		idx++;
		rec = rec->next;
	}
	return -1;
}

int playlistGetPositionByHash (PLAYLISTCACHE *plc, const unsigned int hash)
{
	int ret = -1;
	if (playlistLock(plc)){
		ret = _playlistGetPositionByHash(plc, hash);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistGetTrackIdByHash (PLAYLISTCACHE *plc, const unsigned int hash)
{
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	
	while(rec){
		TPLAYLISTITEM *item = rec->item;
		if (item && item->obj.track.hash == hash && item->objType == PLAYLIST_OBJTYPE_TRACK)
			return item->tid;

		rec = rec->next;
	}
	return 0;
}

int playlistGetTrackIdByHash (PLAYLISTCACHE *plc, const unsigned int hash)
{
	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistGetTrackIdByHash(plc, hash);
		playlistUnlock(plc);
	}
	return ret;
}

static inline void _playlistFreeRecord (TPLAYLISTRECORD *rec)
{
	if (rec->item)
		_playlistFreeItem(rec->item);
	my_free(rec);
}

static inline int _playlistDeleteRecord (PLAYLISTCACHE *plc, const int pos)
{
	TPLAYLISTRECORD *rec = _playlistGetRecord(plc, pos);
	if (rec){
		if (_playlistUnlinkRecord(plc, rec)){
			_playlistFreeRecord(rec);
			plc->total--;
			return 1;
		}
	}
	return 0;
}

int playlistDeleteRecord (PLAYLISTCACHE *plc, const int pos)
{
	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistDeleteRecord(plc, pos);
		playlistUnlock(plc);
	}
	return ret;
}

static inline int _playlistPrune (PLAYLISTCACHE *plc, const int remove)
{
	int ct = 0;
	TPLAYLISTRECORD tmp;
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	
	if (rec){
		do{
			if (rec && rec->item){
				if (rec->item->objType == PLAYLIST_OBJTYPE_TRACK){
					if (!doesFileExist8(rec->item->obj.track.path)){
						if (remove){
							tmp.next = rec->next;
							if (_playlistUnlinkRecord(plc, rec)){
								_playlistFreeRecord(rec);
								plc->total--;
							}
							rec = &tmp;
						}
						ct++;
					}
				}
			}
		}while((rec=rec->next));
	}
	return ct;
}

int playlistPrune (PLAYLISTCACHE *plc, const int remove)
{
	int ret = 0;
	if (playlistLock(plc)){
		ret = _playlistPrune(plc, remove);
		playlistUnlock(plc);
	}
	return ret;
}

static inline void _playlistDeleteRecords (PLAYLISTCACHE *plc)
{
	if (!plc->total) return;
		
	TPLAYLISTRECORD *rec = _playlistGetRoot(plc);
	if (rec){
		TPLAYLISTRECORD *next;
		do{
			next = rec->next;
			_playlistFreeRecord(rec);
		}while((rec=next));
	}

	plc->first = NULL;
	plc->last = NULL;
	plc->total = 0;
}

void playlistDeleteRecords (PLAYLISTCACHE *plc)
{
	if (playlistLock(plc)){
		_playlistDeleteRecords(plc);
		memset(plc->pr, 0, sizeof(PLAYLISTRENDER));
		plc->pr->selectedItem = -1;
		plc->pr->playingItem = -1;
		plc->pr->selectedItemId = 0;
		plc->pr->playingItemId = 0;
		plc->pr->playingItemPrevious = -2;
		plc->first = NULL;
		plc->last = NULL;
		plc->total = 0;
		plc->artRetryTime = 0;
		plc->artId = 0;
		playlistUnlock(plc);
	}
}

void playlistFree (PLAYLISTCACHE *plc)
{
	if (playlistLock(plc)){
		//printf("## playlistFree: '%s'\n", plc->title);
		
		_playlistDeleteRecords(plc);
		my_free(plc->pr);
		my_free(plc->title);
		lockClose(plc->hMutex);
		my_free(plc);
	}
}

PLAYLISTCACHE *playlistNew (TPLAYLISTMANAGER *plm, const char *name)
{
	PLAYLISTCACHE *plc = my_calloc(1, sizeof(PLAYLISTCACHE));
	if (plc){
		plc->title = my_strdup(name);
		if (plc->title){
			plc->pr = my_calloc(1, sizeof(PLAYLISTRENDER));
			if (plc->pr){
				plc->pr->selectedItem = -1;
				plc->pr->playingItem = -1;
				plc->pr->playingItemPrevious = -2;
				plc->hMutex = lockCreate("playlistNew");
				plc->first = NULL;
				plc->last = NULL;
				plc->total = 0;
				plc->parent = NULL;
				plc->uid = playlistManagerCreateUID(plm);
				plc->idSrc = PLAYLIST_TRACK_BASEIDENT;
				return plc;
			}
		}
		my_free(plc);
	}
	return NULL;
}
