
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


// keep this in sync with meta.c:ctagStrLookup[]
// and libvlc_media.h:libvlc_meta_t

#ifndef _TAGS_H_
#define _TAGS_H_


typedef struct TMETARECORD TMETARECORD;

// keep this in sync with meta.c::tagStrLookupp[] and cmdparser.c:wtagStrLookup[]
enum _tags_meta {
    MTAG_Title,
    MTAG_Artist,
    MTAG_Genre,
    MTAG_Copyright,
    MTAG_Album,
    MTAG_TrackNumber,
    MTAG_Description,
    MTAG_Rating,
    MTAG_Date,
    MTAG_Setting,
    MTAG_URL,
    MTAG_Language,
    MTAG_NowPlaying,		/*	used with streaming media such as DVB-t*/
    MTAG_Publisher,
    MTAG_EncodedBy,
    MTAG_ArtworkPath,
    MTAG_TrackID,	// end of libvlcs' tags
    
#if (0 || LIBVLC_VERSION_MAJOR >= 2 && LIBVLC_VERSION_MINOR >= 2)
    MTAG_TrackTotal,
    MTAG_Director,
    MTAG_Season,
    MTAG_Episode,
    MTAG_ShowName,
    MTAG_Actors,
#endif

	MTAG_LENGTH,	// beginning of additional tags to libvlc's, in all block
	MTAG_FILENAME,	// filename.ext without the path
	MTAG_POSITION,	// position in playlist
	MTAG_PATH,		// complete track path+filename+ext
    MTAG_TOTAL
};


typedef struct{
	char *tag[MTAG_TOTAL+1];	// as of libvlc 1.1 there are 17 tags defined in libvlc_media.h, and several more in 2.2.x
								// use enum MTAG_ to index
	unsigned int hash;			// path based lookup key for which these tags belong
	int hasTitle;				// MTAG_Title has been set
	int hasFilename;			// MTAG_FILENAME has been set
	int isParsed;
}TMETAITEM;

struct TMETARECORD{
	TMETAITEM *item;
	TMETARECORD *next;
};

typedef struct{
	TMLOCK *hMutex;
	TMETARECORD *first;				// tags are here
	TMETARECORD *lastCreated;

	int maxArtWidth;
	int maxArtHeight;
	THWD *hw;
}TMETATAGCACHE;

// tagid refers to MTAG_xxx from tags.h
int tagAdd (TMETATAGCACHE *tagc, const char *path, const int tagid, const char *tag, const int overwrite);
int tagAddByHash (TMETATAGCACHE *tagc, const unsigned int hash, const int tagid, const char *tag, const int overwrite);
char *tagRetrieve (TMETATAGCACHE *tagc, const char *path, const int tagid, char *buffer, const size_t len);
char *tagRetrieveByHash (TMETATAGCACHE *tagc, const unsigned int hash, const int tagid, char *buffer, const size_t len);
char *tagRetrieveDup (TMETATAGCACHE *tagc, const unsigned int hash, const int tagid);

TMETATAGCACHE *tagcNew (THWD *hw);
void tagcFree (TMETATAGCACHE *tagc);
void tagFlush (TMETATAGCACHE *tagc);

int tagIsTitleAvailableByHash (TMETATAGCACHE *tagc, const unsigned int hash);
int tagIsFilenameAvailableByHash (TMETATAGCACHE *tagc, const unsigned int hash);


int tagLock (TMETATAGCACHE *tagc);
void tagUnlock (TMETATAGCACHE *tagc);

TMETAITEM *g_tagFindEntryByHash (TMETATAGCACHE *tagc, const unsigned int hash);

int g_tagAddByHash (TMETATAGCACHE *tagc, const unsigned int hash, const int tagid, const char *tag, const int overwrite);
TMETAITEM *g_tagCreateNew (TMETATAGCACHE *tagc, const unsigned int hash);

typedef struct TPLAYLISTMANAGER TPLAYLISTMANAGER;
typedef struct PLAYLISTCACHE PLAYLISTCACHE;

int tagFlushOrfhensPlm (TMETATAGCACHE *tagc, TPLAYLISTMANAGER *plm);
int tagFlushOrfhensPlc (TMETATAGCACHE *tagc, PLAYLISTCACHE *plc);


int tagLookup (const char *str);
const char *tagToString (const int mtag);

#endif
