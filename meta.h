
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





#ifndef _META_H_
#define _META_H_


typedef struct{
	char *name;
	char *value;
}vlc_meta_str_t;

typedef struct{
	vlc_meta_str_t **meta;
	int total;
}vlc_meta_extra_t;


typedef struct{
	unsigned int hashTrack;		// hash of playlist item's track path entry
	unsigned int hashArt;		// hash of artwork path
	TFRAME *img;
	int artId;
}TMETAARTWORK;

typedef struct{
	TMETAARTWORK art;
	int x;
	int y;
	int w;
	int h;
	int textHeight;
	int trackPosition;
	unsigned int uid;			// playlist id
}TMETADESC;
	
typedef struct{
	TPAGE2COM *com;
	
	TMETADESC desc;
	vlc_meta_extra_t *extra;
	
	TCCBUTTONS *btns;
	TSLIDER *trackSlider;
}TMETA;



int page_metaCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);



// remove me
int metaRender (TVLCPLAYER *vp, TFRAME *frame, TMETA *meta, TMETADESC *desc, const int font, const int drawui);

void metaCopyDesc (TMETADESC *desc, TMETA *meta);
void metaGetUpdate (TVLCPLAYER *vp);

// returns a UTF8 string (free with my_free()) containing available meta data and all stream info (ES) from all channels
char *metaGetMetaAll (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int trackId, const char *lineBreak);


#endif


