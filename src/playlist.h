
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



#ifndef _PLAYLIST_H_
#define _PLAYLIST_H_


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))




typedef void (*TMetaCbMsg_t) (TVLCPLAYER *vp, const int itemId, const int dataInt1, const int dataInt2, void *dataPtr1, void *dataPt2);

typedef struct{
	TMetaCbMsg_t cb;
	int dataInt1;
	int dataInt2;
	void *dataPtr1;
	void *dataPtr2;
	int itemId;
}TMETACOMPLETIONCB;




void trackLoadEvent (TVLCPLAYER *vp, PLAYLISTCACHE *plc, int trackIdx);
void playlistChangeEvent (TVLCPLAYER *vp, PLAYLISTCACHE *plc, int trackIdx);

int playlistMetaGetMeta (TVLCPLAYER *vp, PLAYLISTCACHE *plc, int from, int to, TMETACOMPLETIONCB *mccb);
int playlistMetaGetTrackMeta (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const char *path, const int position, TMETACOMPLETIONCB *mccb);
int playlistMetaGetTrackMetaByHash (TVLCPLAYER *vp, const unsigned int hash, PLAYLISTCACHE *plc, const char *path, const int position, TMETACOMPLETIONCB *mccb);

int importPlaylistUIDW (TPLAYLISTMANAGER *plm, TMETATAGCACHE *tagc, TARTMANAGER *am, const wchar_t *path, const int uid, TFILEPANE *filepane);
int importPlaylistW (TPLAYLISTMANAGER *plm, PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, TARTMANAGER *am, const wchar_t *path, TFILEPANE *filepane);
int importPlaylist (TPLAYLISTMANAGER *plm, PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, TARTMANAGER *am, const char *path, TFILEPANE *filepane);

int playlistGetTrackLengths (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int recursive, const int log);

void savePlaylistAsync (TVLCPLAYER *vp);

char *getPlayingTitle (TVLCPLAYER *vp);
char *getPlayingAlbum (TVLCPLAYER *vp);
char *getPlayingPath (TVLCPLAYER *vp);
char *getPlayingProgramme (TVLCPLAYER *vp);
char *getPlayingLengthStr (TVLCPLAYER *vp);
char *getPlayingArtworkPath (TVLCPLAYER *vp);
char *getPlayingArtist (TVLCPLAYER *vp);
char *getPlayingDescription (TVLCPLAYER *vp);
char *getPlayingGenre (TVLCPLAYER *vp);
char *getPlayingTag (TVLCPLAYER *vp, const int mtag);

wchar_t *getPlayingPathW (TVLCPLAYER *vp);
wchar_t *getPlayingTitleW (TVLCPLAYER *vp);
wchar_t *getPlayingAlbumW (TVLCPLAYER *vp);
wchar_t *getPlayingLengthStrW (TVLCPLAYER *vp);
wchar_t *getPlayingArtworkPathW (TVLCPLAYER *vp);
wchar_t *getPlayingArtistW (TVLCPLAYER *vp);
wchar_t *getPlayingDescriptionW (TVLCPLAYER *vp);
wchar_t *getPlayingProgrammeW (TVLCPLAYER *vp);

int getPlayingItem (TVLCPLAYER *vp);
char *getSelectedPath (TVLCPLAYER *vp);
int getSelectedItem (TVLCPLAYER *vp);
int isPlayingItem (TVLCPLAYER *vp, const PLAYLISTCACHE *plc, const int pos);
void setSelectedItem (TVLCPLAYER *vp, const int item);
unsigned int getPlayingHash (TVLCPLAYER *vp);
unsigned int getSelectedHash (TVLCPLAYER *vp);

int getPlayingItemId (TVLCPLAYER *vp);
int getSelectedItemId (TVLCPLAYER *vp);
int isPlayingItemId (TVLCPLAYER *vp, const int uid, const int id);

void playlistResetRetries (TVLCPLAYER *vp);

int setDisplayPlaylist (TVLCPLAYER *vp, PLAYLISTCACHE *plc);
int setQueuedPlaylist (TVLCPLAYER *vp, PLAYLISTCACHE *plc);
int setDisplayPlaylistByUID (TVLCPLAYER *vp, const int uid);
int setQueuedPlaylistByUID (TVLCPLAYER *vp, const int uid);

int getRootPlaylistUID (TVLCPLAYER *vp);
int getDisplayPlaylistUID (TVLCPLAYER *vp);
int getQueuedPlaylistUID (TVLCPLAYER *vp);

int getQueuedPlaylistParent (TVLCPLAYER *vp);	// return UID, up a level
int getQueuedPlaylistLeft (TVLCPLAYER *vp);		// return UID, same level but left, if possible
int getQueuedPlaylistRight (TVLCPLAYER *vp);	// return UID, same level but right, if possible

int getPlaylistFirstTrack (TVLCPLAYER *vp, const int uid);

PLAYLISTCACHE *getPrimaryPlaylist (TVLCPLAYER *vp);
PLAYLISTCACHE *getQueuedPlaylist (TVLCPLAYER *vp);
PLAYLISTCACHE *getDisplayPlaylist (TVLCPLAYER *vp);

int playlistUIDToIdx (TPLAYLISTMANAGER *plm, const int uid);
int setPlaylistPlayingItem (TVLCPLAYER *vp, PLAYLISTCACHE *plc, int trk, const unsigned int hash);


void playlistTimerStartTrackUID (TVLCPLAYER *vp, const unsigned int uid, const int trk);
void playlistTimerStartTrackPLC (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int trk);

void timer_regTrackInfoUpdate (TVLCPLAYER *vp);


int playlistRenameItem (TMETATAGCACHE *tagc, PLAYLISTCACHE *plc, const int pos, char *newName);

#endif

