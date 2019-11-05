
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


#ifndef _PLAYLISTC_H_
#define _PLAYLISTC_H_



#define PLAYLIST_OBJTYPE_PLC				(1)
#define PLAYLIST_OBJTYPE_TRACK				(2)

#define PLAYLIST_TRACK_BASEIDENT			100

#define PLAYLIST_SEARCH_CASE_INSENSITIVE	1
#define PLAYLIST_SEARCH_CASE_SENSITIVE		2
#define PLAYLIST_SEARCH_CASE_DEFAULT		PLAYLIST_SEARCH_CASE_INSENSITIVE


typedef struct TPLAYLISTRECORD TPLAYLISTRECORD;
typedef struct PLAYLISTCACHE PLAYLISTCACHE;


typedef struct {
	int selectedItem;			// current selection in playlist screen (selected via buttons prev/next, touch or mouse)
	int selectedItemId;
	int playingItem;			// currently playing track [index] and/or selection via browser screen
	int playingItemId;
	int playingItemPrevious;	// last played by index
	
	T2POINT itemOffset;			// used by panel control
	int64_t lbFocus;
	TPOINT pane;
}PLAYLISTRENDER;

typedef struct{
	unsigned int objType;
	int tid;						// unique item id for this playlist, will never change
	
	struct{
		PLAYLISTCACHE *plc;
		struct{
			char *path;				// complete utf8 path to media
			char *title;			// name of playlist/track title
			char *opts;				// per item vlc/media options
			unsigned int hash;		// path and meta tagc lookup key
			int artId;
		}track;
	}obj;

	int metaRetryCt;
	int artRetryCt;
}TPLAYLISTITEM;

struct TPLAYLISTRECORD{
	TPLAYLISTITEM *item;
	TPLAYLISTRECORD *next;
};

struct PLAYLISTCACHE {
	int uid;		// a unique id
	TMLOCK *hMutex;
	
	TPLAYLISTRECORD *first;
	TPLAYLISTRECORD *last;
	int idSrc;		// item id source
		
	int total;
	char *title;				// give the playlist a name
	uint64_t artRetryTime;		// when did last check happen
	int artId;

	// each playlist has its own independent selection and rendering detail
	PLAYLISTRENDER *pr;
	PLAYLISTCACHE *parent;
};


#if 1
int playlistLock (PLAYLISTCACHE *plc);
void playlistUnlock (PLAYLISTCACHE *plc);
#else
int _playlistLock (PLAYLISTCACHE *plc, const char *func, const int line);
void _playlistUnlock (PLAYLISTCACHE *plc, const char *func, const int line);
#define playlistLock(a) _playlistLock((a),__FILE__,__LINE__)
#define playlistUnlock(a) _playlistUnlock((a),__FILE__,__LINE__)
#endif

// create an empty playlist
PLAYLISTCACHE *playlistNew (TPLAYLISTMANAGER *plm, const char *name);

// flush playlist and all allocated memory
void playlistFree (PLAYLISTCACHE *plc);

// flush playlist items
void playlistDeleteRecords (PLAYLISTCACHE *plc);

// get number of records in list
int playlistGetTotal (PLAYLISTCACHE *plc);

// create and add a new record
int playlistAdd (PLAYLISTCACHE *plc, const char *path);

// add a title to a preexisiting record
int playlistSetTitle (PLAYLISTCACHE *plc, const int pos, const char *title, const int overwrite);

// add per track VLC options
int playlistSetOptions (PLAYLISTCACHE *plc, const int pos, const char *opts, const int overwrite);

TPLAYLISTRECORD *playlistRemoveRecord (PLAYLISTCACHE *plc, const int pos);
int playlistInsert (PLAYLISTCACHE *plc, TPLAYLISTRECORD *newRec, const int pos);

int playlistDeleteRecord (PLAYLISTCACHE *plc, const int pos);

int playlistMove (PLAYLISTCACHE *plcFrom, PLAYLISTCACHE *plcTo, const int fromIdx, const int toIdx);
int playlistCopyTrack (PLAYLISTCACHE *plcFrom, const int pos, PLAYLISTCACHE *plcTo);

TPLAYLISTITEM *playlistGetItem (PLAYLISTCACHE *plc, const int pos);
TPLAYLISTITEM *g_playlistGetItem (PLAYLISTCACHE *plc, const int pos);
PLAYLISTCACHE *playlistGetPlaylistByName (PLAYLISTCACHE *plc, const char *title, const int from);
int playlistGetItemType (PLAYLISTCACHE *plc, const int pos);		// is playlist, track or other
int g_playlistGetItemType (PLAYLISTCACHE *plc, const int pos);		// as above but lock must be held before calling
int playlistSetPaths (PLAYLISTCACHE *plc, const char *path);
int playlistSetPath (PLAYLISTCACHE *plc, const int pos, const char *path);
char *playlistGetPath (PLAYLISTCACHE *plc, const int pos, char *buffer, const size_t len);
char *playlistGetTitle (PLAYLISTCACHE *plc, const int pos, char *buffer, const size_t len);
int playlistGetOptions (PLAYLISTCACHE *plc, const int pos, char *buffer, const size_t len);
char *playlistGetOptionsDup (PLAYLISTCACHE *plc, const int trackId);

unsigned int playlistGetHash (PLAYLISTCACHE *plc, const int pos);		// return track path hash, as used with the meta tagc cache
unsigned int g_playlistGetHash (PLAYLISTCACHE *plc, const int pos);		// as above but lock must be held before calling
int playlistGetPositionByHash (PLAYLISTCACHE *plc, const unsigned int hash);
int playlistGetTrackIdByHash (PLAYLISTCACHE *plc, const unsigned int hash);

int playlistGetArtId (PLAYLISTCACHE *plc, const int pos);
int playlistSetArtId (PLAYLISTCACHE *plc, const int pos, const int artId, const int overwrite);


// debugging only
int playlistRunList (PLAYLISTCACHE *plc);

// global search in all meta data
int playlistSearch (PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, const char *str, const int fromIdx);

// search in specific tags only
int playlistSearchTag (PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, const int mtag, const char *str, const int fromIdx);

// lock mmust be helf before entering
int playlistSearchEx (PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, const char *str, const int fromIdx, int *foundInTag, const int searchHow);

PLAYLISTCACHE *playlistGetParent (PLAYLISTCACHE *plc);

int playlistPrune (PLAYLISTCACHE *plc, const int remove);

char *playlistGetName (PLAYLISTCACHE *plc, char *buffer, const size_t len);
int playlistSetName (PLAYLISTCACHE *plc, char *buffer);

int playlistAddPlc (PLAYLISTCACHE *plc, const PLAYLISTCACHE *childPlaylist);
int playlistGetCount (PLAYLISTCACHE *plc, const int type);
int playlistGetPlaylistUID (PLAYLISTCACHE *plc, const int pos);		// return uid of child playlist at position
PLAYLISTCACHE *playlistGetPlaylist (PLAYLISTCACHE *plc, const int pos);
int playlistGetPlaylistIndex (PLAYLISTCACHE *plc, PLAYLISTCACHE *child);	// if a child of this, return index of playlist


// returns the idx of next/prev objType (track or playlist entry) from pos
int playlistGetNextItem (PLAYLISTCACHE *plc, const unsigned int objType, const int pos);
int playlistGetPrevItem (PLAYLISTCACHE *plc, const unsigned int objType, const int pos);


int playlistIsChild (PLAYLISTCACHE *plcA, PLAYLISTCACHE *plcB);

char *playlistGetNameDup (PLAYLISTCACHE *plc);
char *playlistGetTitleDup (PLAYLISTCACHE *plc, const int pos);
char *playlistGetPathDup (PLAYLISTCACHE *plc, const int pos);

wchar_t *playlistGetPathDupW (PLAYLISTCACHE *plc, const int pos);


// returns unique constant id per playlist record
int playlistGetId (PLAYLISTCACHE *plc, const int idx);
int playlistIdToIdx (PLAYLISTCACHE *plc, const int tid);
int playlistMoveById (PLAYLISTCACHE *plcFrom, PLAYLISTCACHE *plcTo, const int fromId, const int toId);
int playlistCopyTrackById (PLAYLISTCACHE *plcFrom, const int fromId, PLAYLISTCACHE *plcTo, const int toId);
unsigned int playlistGetHashById (PLAYLISTCACHE *plc, const int trackId);
int playlistGetArtIdById (PLAYLISTCACHE *plc, const int trackId);
int playlistGetPlaylistUIDById (PLAYLISTCACHE *plc, const int trackId);		// return uid of child playlist at position
char *playlistGetTitleDupById (PLAYLISTCACHE *plc, const int trackId);


#define playlistIdxToId(a,b) playlistGetId((a),(b))


#endif

