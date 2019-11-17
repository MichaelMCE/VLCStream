
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



#ifndef _PLAYLISTMANAGER_H_
#define _PLAYLISTMANAGER_H_




#define PLAYLIST_UID_BASE	(0x100-1)	// first/root playlist will be uid:0x100

typedef struct TPLAYLISTMANAGER TPLAYLISTMANAGER;
 
struct TPLAYLISTMANAGER {
	PLAYLISTCACHE **plc;
	int p_total;				// total slots available
	
	TMLOCK *hMutex;
	PLAYLISTRENDER *pr;
	
	unsigned int uidSrc;
};


typedef struct{
	char *string;
	int64_t data;
	
	TMETATAGCACHE *tagc;
	int (*funcCb) (void *uptr, const char *searchFor, const int uid, const int trackIdx, const int foundInMTag, const unsigned int hash, const int recType, const int count);
	void *uptr;
	volatile int *activeState;
	
	int wantHash;
	int from;
	int to;
	int searchHow;		// PLAYLIST_SEARCH_xxxx
	
	int mtag;
	PLAYLISTCACHE *plc;
	int count;
}plm_search;



void playlistManagerUnlock (TPLAYLISTMANAGER *plm);
int playlistManagerLock (TPLAYLISTMANAGER *plm);

PLAYLISTCACHE *playlistManagerCreatePlaylist (TPLAYLISTMANAGER *plm, const char *name, const int single);
int playlistManagerDeletePlaylist (TPLAYLISTMANAGER *plm, PLAYLISTCACHE *plc, const int freePlaylist);
int playlistManagerDeletePlaylistByUID (TPLAYLISTMANAGER *plm, const int uid, const int doFreePlaylist);

TPLAYLISTMANAGER *playlistManagerNew ();
void playlistManagerDelete (TPLAYLISTMANAGER *plm);
int playlistManagerGetTotal (TPLAYLISTMANAGER *plm);


/*
search string modifiers: (plm_search::string)
		
		#nnn	add tracks from playlist nnn (eg; #2C9)
		#:		all files, only
		#*		all playlists, only
		#:*		all files then playlists
		#*:		all playlists then files
		#.		everything, unordered as and when found
		#$		artwork when :data is the imgId
*/
int playlistManagerSearch (TPLAYLISTMANAGER *plm, plm_search *plms);

// first playlist is index 0
// if from == -2, begin search at index half playlist total (playlistTotal/2)
// to -2 == end at half way mark
// to == -1, search until last playlist
int playlistManagerSearchEx (TPLAYLISTMANAGER *plm, plm_search *plms, int from, int to);

int playlistManagerGetUIDByName (TPLAYLISTMANAGER *plm, const char *name);
PLAYLISTCACHE *playlistManagerGetPlaylistByName (TPLAYLISTMANAGER *plm, const char *name);
int playlistManagerGetPlaylistFirst (TPLAYLISTMANAGER *plm);
int playlistManagerGetPlaylistPrev (TPLAYLISTMANAGER *plm, const int plIdx);
int playlistManagerGetPlaylistNext (TPLAYLISTMANAGER *plm, const int plIdx);
int playlistManagerGetPlaylistIndex (TPLAYLISTMANAGER *plm, const PLAYLISTCACHE *plc);
PLAYLISTCACHE *playlistManagerGetPlaylist (TPLAYLISTMANAGER *plm, const int plIdx);

int playlistManagerGetPlaylistParentByUID (TPLAYLISTMANAGER *plm, const int uid);
int playlistManagerCreateUID (TPLAYLISTMANAGER *plm);
int playlistManagerGetPlaylistUID (TPLAYLISTMANAGER *plm, const PLAYLISTCACHE *plc);
int playlistManagerGetUIDByIndex (TPLAYLISTMANAGER *plm, const int index);
int playlistManagerGetIndexByUID (TPLAYLISTMANAGER *plm, const int uid);
int playlistManagerGetPlaylistArtIdByUID (TPLAYLISTMANAGER *plm, const int uid);
char *playlistManagerGetName (TPLAYLISTMANAGER *plm, const int uid);
PLAYLISTCACHE *playlistManagerGetPlaylistByUID (TPLAYLISTMANAGER *plm, const int uid);
int playlistManagerCreatePlaylistUID (TPLAYLISTMANAGER *plm, const char *name, const int single);
int playlistManagerAddPlaylistUID (TPLAYLISTMANAGER *plm, const int parentUID, const int childUID);
int playlistManagerIsValidUID (TPLAYLISTMANAGER *plm, const int uid);

// same track [path] may appear in multiple playlists', this will just return the first it finds
int playlistManagerGetPlaylistByTrackHash (TPLAYLISTMANAGER *plm, const unsigned int hash);


// delink playlist fromUID from its parent then insert in to the position occupied by toUID, moving toUID to the right
int playlistManagerMovePlaylistTo (TPLAYLISTMANAGER *plm, const int fromUID, const int toUID);
int playlistManagerMovePlaylistInto (TPLAYLISTMANAGER *plm, const int fromUID, const int toUID);



#endif

