
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




// keep this in sync with tags.h:enum _tags_meta and meta.c:tagStrLookup[]
static const char *tagStrTable[] = {
	"title",
    "artist",
    "genre",
    "copyright",
    "album",
    "track",
    "description",
    "rating",
    "date",
    "setting",
    "url",
    "language",
    "nowplaying",
    "publisher",
    "encodedby",
    "artpath",
    "trackid",

#if (0 || LIBVLC_VERSION_MAJOR >= 2 && LIBVLC_VERSION_MINOR >= 2)
    "tracktotal",
    "director",
    "season",
    "episode",
    "showname",
    "actors",
#endif
    
    "length",
    "filename",
    "playlist",
    "path",
    ""
};



const char *tagToString (const int mtag)
{
	if (mtag >= 0 && mtag < MTAG_TOTAL)
		return tagStrTable[mtag];
	return NULL;
}

int tagLookup (const char *str)
{
	for (int mtag = 0; mtag < MTAG_TOTAL; mtag++){
		if (!stricmp(str, tagStrTable[mtag]))
			return mtag;
	}
	return -1;
}

static inline int _tagLock (TMETATAGCACHE *tagc)
{
	return lockWait(tagc->hMutex, INFINITE);
}

static inline void _tagUnlock (TMETATAGCACHE *tagc)
{
	lockRelease(tagc->hMutex);
}

int tagLock (TMETATAGCACHE *tagc)
{
	return _tagLock(tagc);
}

void tagUnlock (TMETATAGCACHE *tagc)
{
	_tagUnlock(tagc);
}

static inline TMETARECORD *_tagGetFirst (TMETATAGCACHE *tagc)
{
	return tagc->first;
}

static inline TMETARECORD *_tagGetLast (TMETATAGCACHE *tagc)
{
	return tagc->lastCreated;
}
	
static inline TMETAITEM *_tagFindEntryByHash (TMETATAGCACHE *tagc, const unsigned int hash)
{
	if (_tagGetLast(tagc) && _tagGetLast(tagc)->item->hash == hash)
		return _tagGetLast(tagc)->item;

	TMETARECORD *tag = _tagGetFirst(tagc);
	if (tag){
		do{
			if (tag->item && tag->item->hash == hash)
				return tag->item;
		}while((tag=tag->next));
	}
	return NULL;
}

TMETAITEM *g_tagFindEntryByHash (TMETATAGCACHE *tagc, const unsigned int hash)
{
	return _tagFindEntryByHash(tagc, hash);
}

static inline TMETAITEM *_tagFindEntry (TMETATAGCACHE *tagc, const char *path)
{
	if (!path || !*path) return NULL;

	return _tagFindEntryByHash(tagc, getHash(path));
}

static inline TMETAITEM *_tagCreateItem (TMETATAGCACHE *tagc, const unsigned int hash/*, const char *path*/)
{
	if (!hash) return NULL;

	TMETAITEM *item = (TMETAITEM*)my_calloc(1, sizeof(TMETAITEM));
	if (item) item->hash = hash;
	return item;
}

static inline TMETARECORD *_tagCreateRecord (TMETATAGCACHE *tagc)
{
	return (TMETARECORD*)my_malloc(sizeof(TMETARECORD));
}

// there may only be one instance of a tag per path
// if path exists then modify it, otherwise create a new entry.
static inline int _tagAddByHash (TMETATAGCACHE *tagc, const unsigned int hash, const int tagid, const char *tag, const int overwrite)
{
	// search for an existing record
	TMETAITEM *item = _tagFindEntryByHash(tagc, hash);
	if (!item){

		// does not exist so create a new record
		item = _tagCreateItem(tagc, hash/*, path*/);
		TMETARECORD *newrec = _tagCreateRecord(tagc);
		//if (!item || !newrec){
			//printf("_tagAddByHash() out of memory %p %p. %i '%s'\n", item, newrec, tagid, tag);
		//	return 0;
		//}
		newrec->item = item;
		newrec->next = NULL;
		
		TMETARECORD *rec = _tagGetLast(tagc);
		if (rec)
			rec->next = newrec;
		else
			tagc->first = newrec;
		tagc->lastCreated = newrec;
	}
	
	if (!item) return 0;
	if (tag){
		if (tagid == MTAG_Title)
			item->hasTitle = 1;
		else if (tagid == MTAG_FILENAME)
			item->hasFilename = 1;

		if (item->tag[tagid]){
			if (!overwrite)
				return 1;
			else
				my_free(item->tag[tagid]);
		}
		item->tag[tagid] = my_strdup(tag);
	}
	return 1;
}

TMETAITEM *g_tagCreateNew (TMETATAGCACHE *tagc, const unsigned int hash)
{
	if (_tagAddByHash(tagc, hash, 0, NULL, 0)){
		//if (tagc->lastCreated->item->hash == hash)
		//	return tagc->lastCreated->item;
		//else
			return _tagFindEntryByHash(tagc, hash);
	}
	return NULL;
}

/*
TMETAITEM *tagFindEntryByHash (TMETATAGCACHE *tagc, const unsigned int hash)
{
	TMETAITEM *item = NULL;
	if (_tagLock(tagc)){
		item = _tagFindEntryByHash(tagc, hash);
		_tagUnlock(tagc);
	}
	return item;
}
*/
int g_tagAddByHash (TMETATAGCACHE *tagc, const unsigned int hash, const int tagid, const char *tag, const int overwrite)
{
	//if (!tag || !hash || tagid < 0 || tagid >= MTAG_TOTAL)
	//	return 0;

	int ret = 0;
	//if (_tagLock(tagc)){
		if (*tag)
			ret = _tagAddByHash(tagc, hash, tagid, tag, overwrite);
		//_tagUnlock(tagc);
	//}
	return ret;
}

int tagAddByHash (TMETATAGCACHE *tagc, const unsigned int hash, const int tagid, const char *tag, const int overwrite)
{
	if (!tag || !hash || tagid < 0 || tagid >= MTAG_TOTAL)
		return 0;

	int ret = 0;
	if (_tagLock(tagc)){
		if (*tag)
			ret = _tagAddByHash(tagc, hash, tagid, tag, overwrite);
		_tagUnlock(tagc);
	}
	return ret;
}

int tagAdd (TMETATAGCACHE *tagc, const char *path, const int tagid, const char *tag, const int overwrite)
{
	if (!path || !tag) return 0;
	return tagAddByHash(tagc, getHash(path), tagid, tag, overwrite);
}

int tagIsFilenameAvailableByHash (TMETATAGCACHE *tagc, const unsigned int hash)
{
	int ret = 0;
	if (_tagLock(tagc)){
		TMETAITEM *item = _tagFindEntryByHash(tagc, hash);
		ret = (item && item->hasFilename);
		_tagUnlock(tagc);
	}
	return ret;
}

int tagIsTitleAvailableByHash (TMETATAGCACHE *tagc, const unsigned int hash)
{
	int ret = 0;
	if (_tagLock(tagc)){
		TMETAITEM *item = _tagFindEntryByHash(tagc, hash);
		ret = (item && item->hasTitle);
		_tagUnlock(tagc);
	}
	return ret;
}

char *tagRetrieveByHash (TMETATAGCACHE *tagc, const unsigned int hash, const int tagid, char *buffer, const size_t len)
{
	*buffer = 0;
	if (!len || !hash || tagid < 0 || tagid >= MTAG_TOTAL) return NULL;

	if (_tagLock(tagc)){
		TMETAITEM *item = _tagFindEntryByHash(tagc, hash);
		if (item && item->tag[tagid])
			strncpy(buffer, item->tag[tagid], len);
		_tagUnlock(tagc);
	}
	return buffer;
}

char *tagRetrieveDup (TMETATAGCACHE *tagc, const unsigned int hash, const int tagid)
{
	if (!hash || tagid < 0 || tagid >= MTAG_TOTAL) return NULL;

	char *tag = NULL;
	if (_tagLock(tagc)){
		TMETAITEM *item = _tagFindEntryByHash(tagc, hash);
		if (item && item->tag[tagid])
			tag = strdup(item->tag[tagid]);
		_tagUnlock(tagc);
	}
	return tag;
}

char *tagRetrieve (TMETATAGCACHE *tagc, const char *path, const int tagid, char *buffer, const size_t len)
{
	*buffer = 0;
	if (!path) return buffer;
	return tagRetrieveByHash(tagc, getHash(path), tagid, buffer, len);
}

static inline void _tagFreeItem (TMETAITEM *item)
{
	for (int i = 0; i < MTAG_TOTAL; i++){
		if (item->tag[i])
			my_free(item->tag[i]);
	}

	my_free(item);
}

static inline void _tagFreeRecord (TMETARECORD *rec)
{
	if (rec->item)
		_tagFreeItem(rec->item);
	my_free(rec);
}

static inline int plmGetTrackTotal (TPLAYLISTMANAGER *plm)
{
	int total = 0;
	for (int i = 0; i < plm->p_total; i++){
		if (plm->plc[i])
			total += plm->plc[i]->total;
	}
	return total;
}

static inline int *createHashList (TPLAYLISTMANAGER *plm, const int hlistTotal)
{
	int *hlist = my_malloc((hlistTotal+1) * sizeof(int));
	if (!hlist) return NULL;
	
	int hashIdx = 0;
	const int total = plm->p_total;

	for (int i = 0; i < total; i++){
		if (plm->plc[i]){
			TPLAYLISTRECORD *rec = plm->plc[i]->first;
			while(rec){
				if (rec->item && rec->item->obj.track.hash)
					hlist[hashIdx++] = rec->item->obj.track.hash;
				rec = rec->next;
			}
		}
	}
	
	hlist[hashIdx] = 0;
	return hlist;
}

static inline int isHashInList (const int * restrict list, const int total, const int hash)
{
	for (int i = 0; i < total; i++){
		if (list[i] == hash) return 1;
	}
	return 0;
}

static inline int _tagFlushOrfhensPlm (TMETATAGCACHE *tagc, TPLAYLISTMANAGER *plm)
{
	TMETARECORD *rec = _tagGetFirst(tagc);
	if (!rec) return 0;

	const int hlistTotal = plmGetTrackTotal(plm);
	int *hlist = createHashList(plm, hlistTotal);
	if (!hlist) return 0;

	int totalPruned = 0;
	do{
		if (rec->item && rec->item->hash){
			if (!isHashInList(hlist, hlistTotal, rec->item->hash)){
				totalPruned++;
				//printf("got orfhen for %i: %X\n", totalPruned, rec->item->hash);
	
				rec->item->hash = 0;
				rec->item->hasTitle = 0;
				rec->item->hasFilename = 0;
				rec->item->isParsed = 0;
			}
		}
	}while((rec=rec->next));
	my_free(hlist);
	
	return totalPruned;
}

// remove what is about to become orfhened
static inline int _tagFlushOrfhensPlc (TMETATAGCACHE *tagc, PLAYLISTCACHE *plc)
{
	int totalPruned = 0;
	
	TMETARECORD *rec = _tagGetFirst(tagc);
	if (rec){
		do{
			if (rec->item && rec->item->hash){
				if (playlistGetPositionByHash(plc, rec->item->hash) >= 0){
					totalPruned++;
					//printf("got orfhen for %i: %X\n", totalPruned++, rec->item->hash);
					
					rec->item->hash = 0;
					rec->item->hasTitle = 0;
					rec->item->hasFilename = 0;
					rec->item->isParsed = 0;
				}
			}
		}while((rec=rec->next));
	}
	return totalPruned;
}

int tagFlushOrfhensPlc (TMETATAGCACHE *tagc, PLAYLISTCACHE *plc)
{
	int ret = 0;
	if (_tagLock(tagc)){
		if (playlistLock(plc)){
			ret = _tagFlushOrfhensPlc(tagc, plc);
			playlistUnlock(plc);
		}
		_tagUnlock(tagc);
	}
	return ret;
}

int tagFlushOrfhensPlm (TMETATAGCACHE *tagc, TPLAYLISTMANAGER *plm)
{
	int ret = 0;
	if (_tagLock(tagc)){
		if (playlistManagerLock(plm)){
			ret = _tagFlushOrfhensPlm(tagc, plm);
			playlistManagerUnlock(plm);
		}
		_tagUnlock(tagc);
	}
	return ret;
}

static inline void _tagFlush (TMETATAGCACHE *tagc)
{
	tagc->lastCreated = NULL;
	TMETARECORD *rec = _tagGetFirst(tagc);
	if (rec){
		TMETARECORD *next;
		do{
			next = rec->next;
			_tagFreeRecord(rec);
		}while((rec=next));
	}
	tagc->first = NULL;
}

void tagFlush (TMETATAGCACHE *tagc)
{
	if (_tagLock(tagc)){
		_tagFlush(tagc);
		_tagUnlock(tagc);
	}
}

void tagcFree (TMETATAGCACHE *tagc)
{
	_tagLock(tagc);
	_tagFlush(tagc);
	lockClose(tagc->hMutex);
	tagc->hMutex = NULL;
	my_free(tagc);
}

TMETATAGCACHE *tagcNew (/*TVLCPLAYER *vp, */THWD *hw)
{
	TMETATAGCACHE *tagc = my_calloc(1, sizeof(TMETATAGCACHE));
	if (tagc){
		tagc->hw = hw;
		//tagc->vp = vp;
		tagc->hMutex = lockCreate("tagNew");
	}
	return tagc;
}
