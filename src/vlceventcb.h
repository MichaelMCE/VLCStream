
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



#ifndef _VLCEVENTCB_H_
#define _VLCEVENTCB_H_


typedef struct {
	int status;
	libvlc_media_t *m;
	libvlc_event_manager_t *em;
	
	TMETATAGCACHE *tagc;
	char *path;
	int length;
	
	TVLCPLAYER *vp;
	TLISTITEM *item;
	
	libvlc_event_type_t events[128];
	int eventsTotal;
	int eventClass;
	
	TMETACOMPLETIONCB *mccb;
	
	//PLAYLISTCACHE *plc;
	int pid;			// playlist id
	int position;
	int hash;
}TVLCEVENTCB_OPAQUE;

typedef struct {
	TLISTITEM *root;
	TLISTITEM *last;
}TVLCEVENTCB;




void vlcEventGetLength (libvlc_instance_t *hLib, TVLCPLAYER *vp, TMETATAGCACHE *tagc, const char *path, const int hash, TMETACOMPLETIONCB *mccb);
void vlcEventGetMeta (libvlc_instance_t *hLib, TVLCPLAYER *vp, TMETATAGCACHE *tagc, const char *path, const int hash, TMETACOMPLETIONCB *mccb, const int pid, const int position);



void vlcEventFreeHandles (TVLCPLAYER *vp, const int mode);
void vlcEventsUnlock (TVLCCONFIG *vlc);
int vlcEventsLock (TVLCCONFIG *vlc);
void vlcEventsCleanup (TVLCPLAYER *vp);
void vlcEventsInit ();
void vlcEventsClose ();
void vlcEventListInvalidate (TVLCCONFIG *vlc);
int vlcEventCount ();

TMETACOMPLETIONCB *mccbDup (const TMETACOMPLETIONCB *mccb1);



#endif
