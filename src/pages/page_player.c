
// https://github.com/MichaelMCE/VLCStream
// 

// okio@users.sourceforge.net

//  Copyright (c) 2005-2025  
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


#include "../common.h"


const int itemRowHeight = 117;			// vertical pitch of both fonts + 1 (80+36+1)
const int queueLeftColumnWidth = 72;
;


void playlistQueueAdjustPosition (TPLYQUEUE *plyqueue, const int x,  int y)
{
	TLISTITEM *item = plyqueue->playlistQueue.items;

	playlist_queue_item_t *obj = listGetStorage(item);
	if (!obj->ccCtrl) return;

	// clamp to 0
	int objY = ccGetPositionY(obj->ccCtrl);
	if (objY+y > 0) y = (y-objY) - y;

	while (item){
		playlist_queue_item_t *obj = listGetStorage(item);
		if (obj->ccCtrl){
			int posX = ccGetPositionX(obj->ccCtrl) + x;
			int posY = ccGetPositionY(obj->ccCtrl) + y;

			if (posY < -(itemRowHeight * (plyqueue->playlistQueueItemsAdded-1))){
				break;
			}

			ccSetPosition(obj->ccCtrl, posX, posY);
		}
		item = listGetNext(item);
	}
}

int queueSlide (TPLYQUEUE *plyqueue, const int x, const int y, TTOUCHCOORD *pos)
{
	playlistQueueAdjustPosition(plyqueue, 0, y*3);
	return 1;
}

int64_t queue_base_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	TLABEL *base = (TLABEL*)obj;

	if (msg == LABEL_MSG_BASE_SELECTED_PRESS){
		TPLYQUEUE *plyqueue = ccGetUserData(base);
		TTOUCHCOORD *pos = (TTOUCHCOORD*)dataPtr;

		plyqueue->input.baseYDelta = data1&0xFFFF;
		plyqueue->input.baseIsSliding = 0;
		plyqueue->input.baseIsPressed = 1;
		plyqueue->input.basePressedId = pos->id;
		plyqueue->input.time0 = getTime(base->cc->vp);
		return -1;
		
	}else if (msg == LABEL_MSG_BASE_SELECTED_SLIDE){
		TPLYQUEUE *plyqueue = ccGetUserData(base);
		int y = data1&0xFFFF;
		int delta = y-plyqueue->input.baseYDelta;
		if (delta){
			plyqueue->input.baseIsSliding++;
			queueSlide(plyqueue, (data1>>16)&0xFFFF, delta, (TTOUCHCOORD*)dataPtr);
		}
		plyqueue->input.baseYDelta = y;
		return -1;
		
	}else if (msg == LABEL_MSG_BASE_SELECTED_RELEASE){
		TPLYQUEUE *plyqueue = ccGetUserData(base);
		TTOUCHCOORD *pos = (TTOUCHCOORD*)dataPtr;

		plyqueue->input.baseIsHeld = 0;
		int ret = -1;
		
		if (pos->id == plyqueue->input.basePressedId && plyqueue->input.baseIsPressed && plyqueue->input.baseIsSliding < 2){
			double time0 = getTime(base->cc->vp) - plyqueue->input.time0;
			if (time0 < 350.0){
				ret = 2;			// pass on input
			}else if (time0 > 1000.0){
				printf("base RELEASE %i, %.0f\n", pos->id, time0);
				plyqueue->input.baseIsHeld = 1;
				ret = 2;
			}
		}
		plyqueue->input.baseIsSliding = 0;
		plyqueue->input.baseIsPressed = 0;
		plyqueue->input.basePressedId = 0;
		
		return ret;

	}else if (msg == KP_MSG_PAD_OPENED){

	}else if (msg == KP_MSG_PAD_CLOSED){

	}else if (msg == KP_MSG_PAD_ENTER){
		int dataType = data1;
		if (dataType == KP_INPUT_COMPLETE8){		// UTF8 only
			TVLCPLAYER *vp = (TVLCPLAYER*)obj->cc->vp;
			
					if (strlen(dataPtr) < 1) return 0;
					int id = (int)data2;
					int uid = id>>16;
					int pos = (id&0xFFFF)-1;
					
					PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
					
					int objType = playlistGetItemType(plc, pos);
					if (objType == PLAYLIST_OBJTYPE_TRACK){
						tagAddByHash(vp->tagc, playlistGetHash(plc, pos), MTAG_Title, dataPtr, 1);
						playlistSetTitle(plc, pos, dataPtr, MAX_PATH_UTF8);
						
						int saved = 0;
						char *path = playlistGetPathDup(plc, pos);
						if (path){
							libvlc_media_t *m = libvlc_media_new_path(vp->vlc->hLib, path);
							if (m){
								libvlc_media_set_meta(m, (int)MTAG_Title, dataPtr);
								saved = libvlc_media_save_meta(m);
								libvlc_media_release(m);
							}
							my_free(path);
						}
						if (!saved)
							dbprintf(vp, "title not saved: %X %i", uid, pos+1);
						
					}else if (objType == PLAYLIST_OBJTYPE_PLC){
					}
		}
	}

	return 1;
}

int64_t queue_playlistItem_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TLABEL *label = (TLABEL*)object;

	if (msg == LABEL_MSG_BASE_SELECTED_PRESS){
		
	}else if (msg == LABEL_MSG_BASE_SELECTED_SLIDE){
		
	}else if (msg == LABEL_MSG_BASE_SELECTED_RELEASE){
		playlist_queue_item_t *obj = ccGetUserData(label);

		TPLYQUEUE *plyqueue = obj->dataPtr;
		TVLCPLAYER *vp = label->cc->vp;
		
		PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
		int track = obj->track;

		if (0 && plyqueue->input.baseIsHeld && pageGet(vp) != PAGE_VKEYBOARD){
			TKEYPAD *kp = keyboardGetKeypad(plyqueue);
  			int firstBuild = buildKeypad(kp);

			char title[MAX_PATH_UTF8+1];
  			playlistGetTitle(plc, track, title, MAX_PATH_UTF8);
  			int id = (playlistManagerGetPlaylistUID(vp->plm, plc)<<16) | (track+1);

  			keypadListenerRemoveAll(kp);
  			keypadListenerAdd(kp, plyqueue->base->id, KP_INPUT_OPENED|KP_INPUT_CLOSED|KP_INPUT_COMPLETE8, id);
  			keypadEditboxSetBuffer8(&kp->editbox, title);
  			keypadEditboxSetUndoBuffer8(&kp->editbox, title);
  			keypadEditboxSetUserData(&kp->editbox, id); // only sets the hex value displayed
			if (firstBuild) ccEnable(kp);
			pageSet(vp, PAGE_VKEYBOARD);
			return 1;
		}

		if (getPlayState(vp) && isPlayingItem(vp, plc, track)){
			timerSet(vp, TIMER_STOP, 0);
			plyqueue->updateButtons = 1;

		}else{
			TTIMERPLAYTRACK *tpt = &vp->gui.playtrack;
			tpt->uid = getQueuedPlaylistUID(vp);
			tpt->track = track;
			timerSet(vp, TIMER_PLAYTRACK, 0);
		}
	}
	return 1;
}

static inline void queueListInsert (playlist_queue_t *queue, playlist_queue_item_t *obj)
{
	if (!queue->items){
		queue->items = listNewItem(obj);
		queue->last = queue->items;
		queue->last->prev = NULL;
		queue->last->next = NULL;

	}else{
		queue->last->next = listNewItem(obj);
		queue->last = queue->last->next;
	}
}

void playlistQueueBuild (TPLYQUEUE *plyqueue, TVLCPLAYER *vp, PLAYLISTCACHE *plc)
{
	char buffer[MAX_PATH_UTF8+8];
	char artist[MAX_PATH_UTF8+1];
	char length[64];

	const int total = playlistGetTotal(plc);
	const int itemWidth = ccGetWidth(plyqueue->base) - 4;
	const int itemHeight = itemRowHeight;
	const int itemX = queueLeftColumnWidth;
	
	plyqueue->playlistQueueItemsAdded = 0;

	for (int i = 0; i < total; i++){
		if (playlistGetItemType(plc, i) != PLAYLIST_OBJTYPE_TRACK){
			playlist_queue_item_t *obj = my_calloc(1, sizeof(playlist_queue_item_t));
			obj->dataPtr = plyqueue;
			obj->itemNumber = -1;
			obj->track = i;
			queueListInsert(&plyqueue->playlistQueue, obj);	
			continue;
		}
			
		TLABEL *label = ccCreate(vp->cc, PAGE_PLY_QUEUE, CC_LABEL, queue_playlistItem_cb, NULL, itemWidth, itemHeight);
		if (!label) break;
		
		playlist_queue_item_t *obj = my_calloc(1, sizeof(playlist_queue_item_t));
		obj->ccCtrl = label;
		obj->dataPtr = plyqueue;
		obj->track = i;
		obj->itemNumber = plyqueue->playlistQueueItemsAdded++;
		queueListInsert(&plyqueue->playlistQueue, obj);

		ccSetUserData(label, obj);
		ccSetPosition(label, itemX, obj->itemNumber * itemHeight);
		
		labelRenderFlagsSet(label, LABEL_RENDER_TEXT|LABEL_RENDER_IMAGE|LABEL_RENDER_BASE|/*LABEL_RENDER_HOVER|*/LABEL_RENDER_HOVER_OBJ);
		if (i&0x01)
			labelBaseColourSet(label, 90<<24|COL_BLACK);
		else
			labelBaseColourSet(label, 120<<24|COL_BLACK);


		int largePosX = 0;
		obj->itemId.art = playlistGetArtId(plc, i);
		if (obj->itemId.art){
			obj->itemId.art = labelArtcCreateEx(label, obj->itemId.art, 0.25, 0, 0, 0, 1, COL_WHITE, 1.0, i+1);
			//labelItemGet(label, obj->itemId.art);
			int width = 0;
			labelArtcGetMetrics(label, obj->itemId.art, &width, NULL);
			largePosX += width + 1;
		}


		const uint32_t hash = playlistGetHash(plc, i);
		tagRetrieveByHash(vp->tagc, hash, MTAG_Title, buffer, sizeof(buffer)-1);
		obj->itemId.large = labelTextCreate(label, buffer, PF_LEFTJUSTIFY, LFTW_UNICODE72, largePosX, -10);
			
		//labelRenderFilterSet(label, obj->itemId.large, 1);
		labelStringSetMaxWidth(label, obj->itemId.large, itemWidth-48);

		tagRetrieveByHash(plyqueue->com->vp->tagc, hash, MTAG_LENGTH, length, sizeof(length)-1);
		if (length[0] == 0) strcpy(length, "0:00");
		tagRetrieveByHash(vp->tagc, hash, MTAG_Artist, artist, sizeof(artist)-1);
		if (!artist[0])
			sprintf(buffer, " %s", length);
		else
			sprintf(buffer, " %s - %s", length, artist);
		
		obj->itemId.small = labelTextCreate(label, buffer, PF_LEFTJUSTIFY, LFTW_B34, 0, 80-6);
		labelStringSetMaxWidth(label, obj->itemId.small, itemWidth);

		ccEnable(label);
	}
}

void playlistQueueRender (TPLYQUEUE *plyqueue, TFRAME *frame)
{
	TMETRICS metrics;
	TLISTITEM *item = plyqueue->playlistQueue.items;
	

	while (item){
		playlist_queue_item_t *obj = listGetStorage(item);
		if (obj->ccCtrl){
			ccGetMetrics(obj->ccCtrl, &metrics);
			if (metrics.y < frame->height-1 && (metrics.y + metrics.height > 0))
				ccRender(obj->ccCtrl, frame);
		}
		item = listGetNext(item);
	}
}

void playlistQueueFree (TPLYQUEUE *plyqueue)
{
	TLISTITEM *item = plyqueue->playlistQueue.items;
	while (item){
		playlist_queue_item_t *obj = listGetStorage(item);
		
		if (obj){
			if (obj->ccCtrl) ccDelete(obj->ccCtrl);
			my_free(obj);
		}
		
		void *ptr = item;
		item = listGetNext(item);
		my_free(ptr);
	}
	
	plyqueue->playlistQueue.items = NULL;
	plyqueue->playlistQueue.last = NULL;
}


void queueReset (TPLYQUEUE *plyqueue)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(plyqueue->com->vp);

	const int total = playlistGetTotal(plc);
	TLISTITEM *item = plyqueue->playlistQueue.items;
	if (!total || !item) return;
	
	int itemCt = 0;
	
	for (int i = 0; i < total; i++){
		playlist_queue_item_t *obj = listGetStorage(item);
		if (obj->ccCtrl){
			ccSetPosition(obj->ccCtrl, queueLeftColumnWidth, itemCt * itemRowHeight);
			itemCt++;
		}
		item = listGetNext(item);
	}
	
}

void playlistQueueFocusItem (TPLYQUEUE *plyqueue, const int trk)
{
	PLAYLISTCACHE *plc = getQueuedPlaylist(plyqueue->com->vp);

	for (int i = 0; i < trk; i++){
		if (playlistGetItemType(plc, i) == PLAYLIST_OBJTYPE_TRACK)
			playlistQueueAdjustPosition(plyqueue, 0, -itemRowHeight);
	}
}

static inline int64_t queue_buttons_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCBUTTON *btn = (TCCBUTTON*)object;
	TVLCPLAYER *vp = btn->cc->vp;
	
	if (msg == BUTTON_MSG_SELECTED_PRESS){
		TCCBUTTONS *btns = ccGetUserData(btn);
		if (btns) btns->t0 = getTickCount();
		
		TPLYQUEUE *plyqueue = pageGetPtr(vp, ccGetOwner(btn));
		
		int btnId = ccGetUserDataInt(btn);
		if (btnId == BTN_PLYQUEUE_CLOSE){
			page2SetPrevious(plyqueue);
			
		}else if (btnId == BTN_PLYQUEUE_STOP){
			timerSet(vp, TIMER_STOP, 0);
			plyqueue->playingTrk = -1;
			
		}else if (btnId == BTN_PLYQUEUE_PLAY){
			TPLYQUEUE *plyqueue = pageGetPtr(vp, ccGetOwner(btn));
			TTIMERPLAYTRACK *tpt = &vp->gui.playtrack;
			
			tpt->uid = getQueuedPlaylistUID(vp);
			tpt->track = plyqueue->playingTrk;
			timerSet(vp, TIMER_PLAYTRACK, 0);
			
			plyqueue->playingTrk = -1;

		}else if (btnId == BTN_PLYQUEUE_EQ){
			page2Set(vp->pages, PAGE_EQ, 1);
		}
	}
	return 1;
}

// called at program startup
int page_queueStartup (TPLYQUEUE *plyqueue, TVLCPLAYER *vp, const int width, const int height)
{
	TCCBUTTONS *btns = buttonsCreate(vp->cc, PAGE_PLY_QUEUE, BTN_PLYQUEUE_TOTAL, queue_buttons_cb);
	plyqueue->btns = btns;
	
	int y = (height-72);
	buttonsCreateButton(btns, L"player/close.png",NULL, BTN_PLYQUEUE_CLOSE,1, 0, 0, y); y -= 72 + 14;
	buttonsCreateButton(btns, L"player/eq.png",   NULL, BTN_PLYQUEUE_EQ,   1, 0, 0, y); y -= 72 + 14;
	buttonsCreateButton(btns, L"player/stop.png", NULL, BTN_PLYQUEUE_STOP, 0, 0, 0, y); y -= 72 + 14;
	buttonsCreateButton(btns, L"player/play.png", NULL, BTN_PLYQUEUE_PLAY, 0, 0, 0, y); y -= 72 + 14;

	const int toolbarWidth = 72;
	TLABEL *base = ccCreate(vp->cc, PAGE_PLY_QUEUE, CC_LABEL, queue_base_cb, NULL, width-1, height);
	plyqueue->base = base;
	ccSetUserData(base, plyqueue);
	ccSetPosition(base, toolbarWidth, 0);
	ccInputDisable(base);
	//base->isChild = 1;
	base->canDrag = 1;
	labelRenderFlagsSet(base, 0);

	return 1;
}

// called when page is first accessed
int page_queueInitalize (TPLYQUEUE *plyqueue, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_PLY_QUEUE);

	plyqueue->playingTrk = -1;
	plyqueue->signalQueueRebuild = 0;
	plyqueue->updateButtons = 1;
	
	playlistQueueBuild(plyqueue, vp, getQueuedPlaylist(vp));
	playlistQueueFocusItem(plyqueue, getPlayingItem(vp));

	return 1;
}

// called at program shutdown
int page_queueShutdown (TPLYQUEUE *plyqueue)
{
	buttonsDeleteAll(plyqueue->btns);
	ccDelete(plyqueue->base);
	playlistQueueFree(plyqueue);


	return 1;
}

// called when first render is requsted
int page_queueRenderInit (TPLYQUEUE *plyqueue, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	return 1;
}

void plyQueueNewTrackEvent (TPLYQUEUE *plyqueue, unsigned int uid, const int trackIdx)
{
	plyqueue->signalQueueRebuild = 1;
	plyqueue->updateButtons = 1;
	
}

// called before each render
void page_queueRenderBegin (TPLYQUEUE *plyqueue, int64_t destId, int64_t data2, void *opaquePtr)
{
	plyqueue->trackTitleCharSpacing = lGetFontCharacterSpacing(plyqueue->com->vp->ml->hw, LFTW_UNICODE72);
	lSetFontCharacterSpacing(plyqueue->com->vp->ml->hw, LFTW_UNICODE72, plyqueue->trackTitleCharSpacing-4);
	
	if (plyqueue->signalQueueRebuild){
		if (plyqueue->playlistQueueUID != getQueuedPlaylistUID(plyqueue->com->vp)){
			plyqueue->playlistQueueUID = getQueuedPlaylistUID(plyqueue->com->vp);
					
			playlistQueueFree(plyqueue);
			playlistQueueBuild(plyqueue, plyqueue->com->vp, getQueuedPlaylist(plyqueue->com->vp));
		}
		plyqueue->signalQueueRebuild = 0;
	}
	
	plyqueue->updateButtons = 1;
	plyqueue->input.baseYDelta = -1;
	
	ccEnable(plyqueue->base);
	ccInputEnable(plyqueue->base);
	ccHoverRenderSigEnable(plyqueue->com->vp->cc, 16.0);
}

// called after each render
void page_queueRenderEnd (TPLYQUEUE *plyqueue, int64_t destId, int64_t data2, void *opaquePtr)
{
	ccHoverRenderSigDisable(plyqueue->com->vp->cc);
	ccInputDisable(plyqueue->base);
	ccDisable(plyqueue->base);
	lSetFontCharacterSpacing(plyqueue->com->vp->ml->hw, LFTW_UNICODE72, plyqueue->trackTitleCharSpacing);
}


void playlistQueueSetTrackHighlight (TPLYQUEUE *plyqueue, const int setIdx, const int unsetIdx)
{
	int idx = 0;

	TLISTITEM *item = plyqueue->playlistQueue.items;
	while (item){
		playlist_queue_item_t *obj = listGetStorage(item);
		if (obj->ccCtrl){
			if (idx == unsetIdx){
				TLABEL *label = obj->ccCtrl;
				if (idx&0x01)
					labelBaseColourSet(label, 90<<24|COL_BLACK);
				else
					labelBaseColourSet(label, 120<<24|COL_BLACK);

			}else if (idx == setIdx){
				TLABEL *label = obj->ccCtrl;
				labelBaseColourSet(label, 100<<24|COL_ORANGE);
			}
		}
		
		idx++;
		item = listGetNext(item);
	}
}

// do the render
int page_queueRender (TPLYQUEUE *plyqueue, TFRAME *frame)
{
	const int playingTrack = getPlayingItem(plyqueue->com->vp);
	if (playingTrack != plyqueue->playingTrk){
		playlistQueueSetTrackHighlight(plyqueue, playingTrack, plyqueue->playingTrk);
		plyqueue->playingTrk = playingTrack;
		plyqueue->updateButtons = 1;
	}

	if (plyqueue->updateButtons){
		plyqueue->updateButtons = 0;
		buttonsStateSet(plyqueue->btns, BTN_PLYQUEUE_STOP, getPlayState(plyqueue->com->vp) > 0);
	}
	
	playlistQueueRender(plyqueue, frame);
	buttonsRenderAll(plyqueue->btns, frame, BUTTONS_RENDER_HOVER/*|BUTTONS_RENDER_ANIMATE*/);
	return 1;
}

static void doRotary (TPLYQUEUE *plyqueue, rotary_t *enc)
{
	if (enc->encoderId == 1){
		if (enc->positionDelta > 0){
			playlistQueueAdjustPosition(plyqueue, 0, -itemRowHeight);
		}else if (enc->positionDelta < 0){
			playlistQueueAdjustPosition(plyqueue, 0, itemRowHeight);
		}else if (enc->buttonPressed){
			page2SetPrevious(plyqueue);
		}
	}
}

// receives any input attached to this page
int page_queueInput (TPLYQUEUE *plyqueue, const int msg, const int flags, void *dataPtr)
{
	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
		playlistQueueAdjustPosition(plyqueue, 0, itemRowHeight);
		break;
		
	  case PAGE_IN_WHEEL_BACK:
		playlistQueueAdjustPosition(plyqueue, 0, -itemRowHeight);
		break;
		
	  case PAGE_IN_ROTARY_CHANGE:
	  	doRotary(plyqueue, (rotary_t*)dataPtr);
	  	break;
	}
	return 1;
}

int page_queueCb (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TPLYQUEUE *plyqueue = (TPLYQUEUE*)pageStruct;


	if (msg == PAGE_CTL_RENDER){
		return page_queueRender(plyqueue, dataPtr);

	}else if (msg == PAGE_MSG_OBJ_HOVER){
		
	}else if (msg == PAGE_CTL_RENDER_START){
		page_queueRenderBegin(plyqueue, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		page_queueRenderEnd(plyqueue, dataInt1, dataInt2, opaquePtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_queueInput(plyqueue, dataInt1, dataInt2, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_queueRenderInit(plyqueue, dataInt1, dataInt2, dataPtr, opaquePtr);
				
	}else if (msg == PAGE_CTL_STARTUP){
		return page_queueStartup(plyqueue, plyqueue->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_queueInitalize(plyqueue, plyqueue->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_queueShutdown(plyqueue);
		
	}
	
	return 1;
}
