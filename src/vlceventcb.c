
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



#define vlibvlc_free(x) libvlc_free(x)


enum _eventClass{
	EVENTC_DURATION = 1,
	EVENTC_META,
};

static const libvlc_event_type_t vlcEventsM_Duration[] = {
    libvlc_MediaDurationChanged,
    libvlc_MediaParsedChanged
};


static const libvlc_event_type_t vlcEventsM_Meta[] = {
	//libvlc_MediaMetaChanged,		// doesn't return anything useful
	libvlc_MediaParsedChanged
};



extern volatile int SHUTDOWN;
static TVLCEVENTCB eventlist = {NULL};


void vlcEventsDetach (TVLCEVENTCB_OPAQUE *vlc, void *udata);



void vlcEventsInit ()
{
	memset(&eventlist, 0, sizeof(eventlist));
}

void vlcEventsClose ()
{
	SHUTDOWN = 1;
}

int vlcEventCount ()
{
	return listCount(eventlist.root);
}

int vlcEventsLock (TVLCCONFIG *vlc)
{
	//printf("events lock waiting %i\n", (int)GetCurrentThreadId());
	int ret = lockWait(vlc->hLockLengths, INFINITE);
	//printf("events lock got %i\n", (int)GetCurrentThreadId());
	return ret;
}

void vlcEventsUnlock (TVLCCONFIG *vlc)
{
	//printf("events lock release %i, %i\n", (int)GetCurrentThreadId(), vlc->hLockLengths->lockCount);
	lockRelease(vlc->hLockLengths);
}

void vlcEventListInvalidate (TVLCCONFIG *vlc)
{
	if (vlcEventsLock(vlc)){

		TLISTITEM *item = eventlist.root;
		while(item){
			TVLCEVENTCB_OPAQUE *opaque = listGetStorage(item);
			if (opaque){
				opaque->status = 3;
				if (opaque->mccb && opaque->mccb->cb)
					opaque->mccb->cb = NULL;
			}
			item = item->next;
		}
		vlcEventsUnlock(vlc);
	}
}

void vlcEvent_MediaMetaParseChanged (TVLCEVENTCB_OPAQUE *vlc, const int data)
{
	TMETATAGCACHE *tagc = vlc->tagc;
	int hash = 0;

	for (int i = 0; i < MTAG_LENGTH; i++){
		char *tag = NULL;
		
		if (i == MTAG_Title && isMediaVideo8(vlc->path))		// video media generally doesn't contain metadata
			continue;

		if (vlc->m)
			tag = libvlc_media_get_meta(vlc->m, i);

		if (!tag) continue;
		if (!hash) hash = getHash(vlc->path);

		if (i == MTAG_Title){
			//if (!tagRetrieveByHash(meta->vp->tagc, meta->hash, i)){
				strchrreplace(tag, '_', ' ');
#if 1
				char *ay = stristr(tag, ".ay");
				if (ay) *ay = 0;
#endif
				tagAddByHash(tagc, hash, i, tag, 1);
				PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vlc->vp->plm, vlc->pid);
				if (plc)
					playlistSetTitle(plc, vlc->position, tag, (*tag != 0));
			//}
			if (vlc->mccb && vlc->mccb->cb){
				if (vlc->status == 1)
					vlc->mccb->cb(vlc->vp, vlc->mccb->itemId, vlc->mccb->dataInt1, vlc->mccb->dataInt2, vlc->mccb->dataPtr1, vlc->mccb->dataPtr2);
				vlc->mccb->cb = NULL;
			}
		}else if (i == MTAG_ArtworkPath){
#if 1

			//printf("MTAG_ArtworkPath: #%s#\n", tag);
			
			char *out;
			if (!strncmp(tag, "file:///", 8))
				out = decodeURI(tag, strlen(tag));
			else
				out = decodeURI_noprefix(tag, strlen(tag));
				
			if (out){
				PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vlc->vp->plm, vlc->pid);
				if (plc){
					wchar_t *tagw = converttow(out);
					if (tagw){
						int artId = artManagerImageAddEx(vlc->vp->am, tagw, tagc->maxArtWidth, tagc->maxArtHeight);
						//printf("vlcEvent: %i %i '%s'\n", artId>>16, artId&0xFFFF, out);
						if (artId){
#if (RELEASEBUILD)							
							playlistSetArtId(plc, vlc->position, artId, 1);
#else
							if (playlistSetArtId(plc, vlc->position, artId, 0) < 1)
								artManagerImageDelete(vlc->vp->am, artId);
#endif
						}
						my_free(tagw);
					}
				}
				my_free(out);
			}
#endif
		}else{
			tagAddByHash(tagc,/*vlc->path,*/ hash, i, tag, 1);
		}

		vlibvlc_free(tag);
	}
	if (hash){
		TMETAITEM *item = g_tagFindEntryByHash(tagc, hash);
		if (item) item->isParsed = 1;
	}
}

void vlcEvent_MediaMetaChanged (TVLCEVENTCB_OPAQUE *vlc, const int metaType)
{
	char *tag = NULL;
	if (vlc->m)
		tag = libvlc_media_get_meta(vlc->m, metaType);
	//printf("vlcEvent_MediaMetaChanged %i %p\n", metaType, tag);

	if (tag){
		//printf("%i, '%s' '%s'\n", metaType, tag, vlc->path);
		vlibvlc_free(tag);
	}
}


static inline void vlcEventSetTrackLength (TMETATAGCACHE *tagc, const char *path, libvlc_time_t length)
{
	char buffer[64];
	timeToString(length, buffer, sizeof(buffer)-1);
	if (*buffer)
		tagAdd(tagc, path, MTAG_LENGTH, buffer, 1);
}
/*
static inline void freeItem (TLISTITEM *item)
{
	listRemove(item);
	listDestroy(item);
	if (item == eventlist.root) eventlist.root = item->next;
	else if (item == eventlist.last) eventlist.last = item->prev;
}*/

/*
void freeHandleLocked (TVLCEVENTCB_OPAQUE *opaque, TLISTITEM *item)
{
	if (vlcEventsLock(opaque->vp->vlc)){
		freeHandle(opaque, 1);
		vlcEventsUnlock(opaque->vp->vlc);
	}
}*/

static inline int freeHandle (TVLCEVENTCB_OPAQUE *vlc/*, TLISTITEM *item*/, const int freeMode)
{
	if (!vlc) return 1;

	if (freeMode){				// we're shutting down so free everything
		vlc->status = 0;
		//vlcEventsDetach(vlc, vlc);					// VLC dead locks when attempting to call this
		//if (vlc->m) libvlc_media_release(vlc->m);		// and this
		if (vlc->mccb) my_free(vlc->mccb);
		my_free(vlc->path);
		my_free(vlc);
		return 1;

	}else if (vlc->status == 2){	// only free whats been signalled as ready to free
		vlc->status = 0;
		//printf("detach %p '%s'\n", vlc, vlc->path);
		vlcEventsDetach(vlc, vlc);
		//printf("release %p\n", vlc);
		if (vlc->m) libvlc_media_release(vlc->m);
		if (vlc->mccb) my_free(vlc->mccb);
		my_free(vlc->path);
		my_free(vlc);
		return 1;
	}
	return 0;
}


// we need to free the handles from outside the callback thread otherwise libVLC throws a whobbler
static inline void freeHandles (const int freeMode)
{
	if (freeMode){
		SHUTDOWN = 1;
		lSleep(10);
	}

	TLISTITEM *item = eventlist.root;
	while(item){
		TLISTITEM *next = item->next;
		TLISTITEM *prev = item->prev;

		if (freeHandle(listGetStorage(item), freeMode)){
			listSetStorage(item, NULL);
			if (!freeMode){
				listRemove(item);
				listDestroy(item);
				if (eventlist.root == item)
					eventlist.root = next;
				if (eventlist.last == item)
					eventlist.last = prev;
			}
		}
		item = next;
	}
}

void vlcEventFreeHandles (TVLCPLAYER *vp, const int mode)
{
	if (vlcEventsLock(vp->vlc)){
		freeHandles(mode);
		if (mode){
			listDestroyAll(eventlist.root);
			eventlist.root = NULL;
			eventlist.last = NULL;
		}
		
		//printf("vlcEventFreeHandles list size %i\n", listCount(eventlist.root));
		vlcEventsUnlock(vp->vlc);
	}
}

// TIMER_VLCEVENTS_CLEANUP
void vlcEventsCleanup (TVLCPLAYER *vp)
{
	//printf("## vlcEventsCleanup ##\n");
	vlcEventListInvalidate(vp->vlc);
	vlcEventFreeHandles(vp, 0);
}

void vlcEventCB (const libvlc_event_t *event, void *udata)
{
	if (SHUTDOWN) return;
	TVLCEVENTCB_OPAQUE *vlc = (TVLCEVENTCB_OPAQUE*)udata;
	if (!vlc || !vlc->status  || !getApplState(vlc->vp)) return;

	if (vlc->status == 3){
		vlc->status = 2;
		return;
	}

#if 0
   	printf("vlcEventCB: %i, event %i: '%s' %p, %p\n", vlc->eventClass, event->type, vlc_EventTypeToName(event->type), udata, libvlc_media_get_user_data(event->p_obj));
#endif


	const int eventType = event->type;
	switch (eventType){

	  case libvlc_MediaMetaChanged:
	  	vlcEvent_MediaMetaChanged(vlc, event->u.media_meta_changed.meta_type);
	  	break;

	  case libvlc_MediaDurationChanged:
	  	vlc->length = (int)(event->u.media_duration_changed.new_duration / 1000);
	  	if (vlc->length && vlc->path){
	  		vlcEventSetTrackLength(vlc->tagc, vlc->path, vlc->length);
	  		//dbprintf(vlc->vp, "%is - %s", vlc->length, vlc->path);
	  	}
		break;

      case libvlc_MediaParsedChanged:
      	if (vlc->eventClass == EVENTC_META){
      		vlcEvent_MediaMetaParseChanged(vlc, 0);
		}
		vlc->status = 2;		// ready to free
		timerSet(vlc->vp, TIMER_VLCEVENTS_CLEANUP, 20000);
		break;

	}

}

void vlcEventsDetach (TVLCEVENTCB_OPAQUE *vlc, void *udata)
{
	if (vlc->em){
		for (int i = 0; i < vlc->eventsTotal; i++)
			libvlc_event_detach(vlc->em, vlc->events[i], vlcEventCB, udata);
		vlc->em = NULL;
	}
}

static inline int vlcEventsAttach (TVLCEVENTCB_OPAQUE *vlc, const libvlc_event_type_t events[], const int total)
{
	if (total <= 0) return 0;

	vlc->eventsTotal = total;
	vlc->em = libvlc_media_event_manager(vlc->m);
	if (vlc->em){
		for (int i = 0; i < total && total < 31; i++){
			vlc->events[i] =  events[i];
			libvlc_event_attach(vlc->em, events[i], vlcEventCB, vlc);
		}
	}
	vlc->events[total] = 0;
	return total;
}

static inline int vlcEventIsInList (TLISTITEM *item, const int hash)
{
	while(item){
		TVLCEVENTCB_OPAQUE *vlc = listGetStorage(item);
		if (/*vlc &&*/ vlc->hash == hash) return 1;
		item = item->next;
	}
	return 0;
}

static inline int vlcEventIsAdded (TVLCPLAYER *vp, const int hash)
{
	int ret = 0;
	//if (vlcEventsLock(vp->vlc)){
		ret = vlcEventIsInList(eventlist.root, hash);
		//vlcEventsUnlock(vp->vlc);
	//}
	//if (ret){
	//	printf("%i %x\n", listCount(item), hash);
	//}

	return ret;
}

static inline TVLCEVENTCB_OPAQUE *vlcCreateEventsInstance (libvlc_instance_t *hLib, TVLCPLAYER *vp, TMETATAGCACHE *tagc, const char *path, TMETACOMPLETIONCB *mccb)
{
	TVLCEVENTCB_OPAQUE *vlc	= my_calloc(1, sizeof(TVLCEVENTCB_OPAQUE));
	if (!vlc) return NULL;

	vlc->path = my_strdup(path);
	if (!vlc->path){
		my_free(vlc);
		return NULL;
	}

	vlc->vp = vp;
	vlc->tagc = tagc;
	vlc->m = libvlc_media_new_path(hLib, path);
	if (vlc->m){
		libvlc_media_set_user_data(vlc->m, vlc);

		libvlc_media_add_option(vlc->m, "no-audio");
		libvlc_media_add_option(vlc->m, "no-video");
		libvlc_media_add_option(vlc->m, "album-art=2");
		libvlc_media_add_option(vlc->m, "ignore-config");

		//if (vlcEventsLock(vp->vlc)){
			TLISTITEM *item = listNewItem(vlc);
			if (eventlist.root)				//insert first
				listInsert(item, NULL, eventlist.root);
			else
				eventlist.last = item;
			eventlist.root = item;

			vlc->mccb = mccbDup(mccb);
			vlc->item = item;
			vlc->status = 1;				// in use, do not free

			//vlcEventsUnlock(vp->vlc);
			return vlc;
		//}
		//libvlc_media_release(vlc->m);
	}
	my_free(vlc->path);
	my_free(vlc);
	return NULL;
}

void vlcEventGetLength (libvlc_instance_t *hLib, TVLCPLAYER *vp, TMETATAGCACHE *tagc, const char *path, const int hash, TMETACOMPLETIONCB *mccb)
{
	//if (vlcEventIsAdded(vp, hash))
	//	return;

	if (vlcEventsLock(vp->vlc)){
		TVLCEVENTCB_OPAQUE *vlc = vlcCreateEventsInstance(hLib, vp, tagc, path, mccb);
		if (vlc){
			vlc->eventClass = EVENTC_DURATION;
			libvlc_media_parse_async(vlc->m);
			vlcEventsAttach(vlc, vlcEventsM_Duration, sizeof(vlcEventsM_Duration)/sizeof(*vlcEventsM_Duration));
		}
		vlcEventsUnlock(vp->vlc);
	}
}

void vlcEventGetMeta (libvlc_instance_t *hLib, TVLCPLAYER *vp, TMETATAGCACHE *tagc, const char *path, const int hash, TMETACOMPLETIONCB *mccb, const int pid, const int position)
{
	if (!vlcEventsLock(vp->vlc)) return;

	if (vlcEventIsAdded(vp, hash)){
		vlcEventsUnlock(vp->vlc);
		return;
	}

	TVLCEVENTCB_OPAQUE *vlc = vlcCreateEventsInstance(hLib, vp, tagc, path, mccb);
	if (vlc){
		vlc->pid = pid;
		vlc->position = position;
		vlc->eventClass = EVENTC_META;
				
		vlcEventsAttach(vlc, vlcEventsM_Meta, sizeof(vlcEventsM_Meta)/sizeof(*vlcEventsM_Meta));
		libvlc_media_parse_async(vlc->m);
		
		char *tag = libvlc_media_get_meta(vlc->m, MTAG_ArtworkPath);
		//if (tag) printf("vlcEventGetMeta: #%s#\n", tag);
		if (tag) libvlc_free(tag);
	}
	vlcEventsUnlock(vp->vlc);
}

TMETACOMPLETIONCB *mccbDup (const TMETACOMPLETIONCB *mccb1)
{
	if (!mccb1) return NULL;

	TMETACOMPLETIONCB *mccb2 = my_malloc(sizeof(TMETACOMPLETIONCB));
	if (mccb2)
		my_memcpy(mccb2, mccb1, sizeof(TMETACOMPLETIONCB));
	return mccb2;
}
