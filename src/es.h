
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



#ifndef _ES_H_
#define _ES_H_



typedef struct {
	TPAGE2COM *com;
	
	TMARQUEE *marquee;	// a general purpose marquee
	
	int tCategories;
	TCATEGORY *categories;
	int selected;		// selected cat index
	
	TAVTRACKS *atracks;
	int currentAudioTrack;	// (index to TSTREAMINFO:atracks->track[n])
	
	TAVTRACKS *vtracks;
	int currentVideoTrack;	// (index to TSTREAMINFO:atracks->track[n])
	
	TCCBUTTONS *btns;
}TSTREAMINFO;


int page_esCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

void esFreeCategories (TCATEGORY *cats, int tCat);
void esFreeAVTracks (TAVTRACKS *ats);

void esGetUpdate (TVLCPLAYER *vp);

#endif


