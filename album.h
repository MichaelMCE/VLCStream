
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



#ifndef _ALBUM_H_
#define _ALBUM_H_


#define ALBUM_KP_HOLDPERIOD	700			// length of (press) time in ms before keypad is appears for editing



int page_plyAlbCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

void resetAlbumPosition (TVLCPLAYER *vp, TSPL *spl, int track);
void invalidateShelfAlbum (TVLCPLAYER *vp, TSPL *spl, int start);

int initiateAlbumArtRetrieval (TVLCPLAYER *vp, PLAYLISTCACHE *plc, int from, int to, int depth);
int initiateAlbumArtRetrievalEx (TVLCPLAYER *vp, PLAYLISTCACHE *plc, int from, int to, int depth, TMETACOMPLETIONCB *mccb);


int getItemImageCB (void *ptr, const int idx, int *freedBy);
int renderThisShelfItemCB (void *ptr, const int idx, TFRAME *src, TFRAME *des, const int x, const int y, const int w, const int h, double modifier);

//TFRAME *getItemPlaylistImage (TVLCPLAYER *vp, PLAYLISTCACHE *plc, unsigned int *hash);
int getItemPlaylistImage (TVLCPLAYER *vp, PLAYLISTCACHE *plc, int *artcId);

void plyAlbRefresh (TVLCPLAYER *vp);

void albPanCalcPositions (TBTNPANEL *btnpan, const int set);


#endif


