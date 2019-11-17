
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


extern volatile double UPDATERATE_BASE;



static inline int64_t albumCcObject_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	if (msg == CC_MSG_RENDER) return 1;
	//printf("albumCcObject_cb. id:%i, objType:%i, msg:%i, data1:%i, data2:%i\n", obj->id, obj->type, msg, data1, data2);


	if (obj->type == CC_SLIDER_HORIZONTAL){
		TSLIDER *slider = (TSLIDER*)object;
		TVLCPLAYER *vp = (TVLCPLAYER*)obj->cc->vp;
		
		if (slider->id == ccGetId(vp, CCID_SLIDER_ALBUM)){
			switch (msg){
			 // case SLIDER_MSG_VALSET:
		  	  case SLIDER_MSG_VALCHANGED:{
		  	  	TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
		  	  	swipeReset(&spl->drag);
				invalidateShelfAlbum(vp, spl, sliderGetValue(slider));
				break;
			  }
			}
		}
	}else if (obj->type == CC_LABEL){
		TLABEL *label = (TLABEL*)object;
		TVLCPLAYER *vp = (TVLCPLAYER*)obj->cc->vp;
		
		if (label->id == ccGetId(vp, CCID_LABEL_ALBTITLE)){
			 if (msg == KP_MSG_PAD_OPENED){
				//printf("album KP_MSG_PAD_OPENED: %I64d %X %p\n", data1, (int)data2, dataPtr);
				
				TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
				swipeReset(&spl->drag);
				invalidateShelf(vp, spl, sliderGetValue(spl->slider));
				
			}else if (msg == KP_MSG_PAD_CLOSED){
				//printf("album KP_MSG_PAD_CLOSED: %I64d %X %p\n", data1, (int)data2, dataPtr);
				
				TKEYBOARD *vkey = pageGetPtr(vp, PAGE_VKEYBOARD);
				keypadListenerRemoveAll(vkey->kp);
				
			}else if (msg == KP_MSG_PAD_ENTER){
				int dataType = data1;
				if (dataType == KP_INPUT_COMPLETE8){		// UTF8 only
					//printf("album KP_MSG_PAD_ENTER: %I64d %X %p: '%s'\n", data1, (int)data2, dataPtr, (char*)dataPtr);
					
					if (strlen(dataPtr) < 1) return 0;
					int id = (int)data2;
					int uid = id>>16;
					int pos = (id&0xFFFF)-1;
					
					PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
					//printf("%x %x\n", uid, pos);
					
					int objType = playlistGetItemType(plc, pos);
					if (objType == PLAYLIST_OBJTYPE_TRACK){
						//printf("a track\n");
						tagAddByHash(vp->tagc, playlistGetHash(plc, pos), MTAG_Title, dataPtr, 1);
						playlistSetTitle(plc, pos, dataPtr, MAX_PATH_UTF8);
						
						int saved = 0;
						char *path = playlistGetPathDup(plc, pos);
						if (path){
							libvlc_media_t *m = libvlc_media_new_path(vp->vlc->hLib, path);
							if (m){
								libvlc_media_set_meta(m, MTAG_Title, dataPtr);
								saved = libvlc_media_save_meta(m);
								libvlc_media_release(m);
							}
							my_free(path);
						}
						if (!saved)
							dbprintf(vp, "Could not save title to %X %i", uid, pos+1);
						//else
						//	dbprintf(vp, "Title saved for %X %i", uid, pos+1);
						
					}else if (objType == PLAYLIST_OBJTYPE_PLC){
						//printf("a playlist\n");
						int childUid = playlistGetPlaylistUID(plc, pos);
						if (childUid){
							plc = playlistManagerGetPlaylistByUID(vp->plm, childUid);
							if (plc)
								playlistSetName(plc, dataPtr);
						}
					}
				}
			}else if (msg == LABEL_MSG_BASE_SELECTED_PRESS){
				TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
				swipeReset(&spl->drag);
				invalidateShelf(vp, spl, sliderGetValue(spl->slider));
				
				TTOUCHCOORD *pos = (TTOUCHCOORD*)dataPtr;
				ccSetUserDataInt(label, pos->time * 1000.0);

			}else if (msg == LABEL_MSG_BASE_SELECTED_RELEASE){
				TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
				swipeReset(&spl->drag);
				invalidateShelf(vp, spl, sliderGetValue(spl->slider));
				
				TTOUCHCOORD *pos = (TTOUCHCOORD*)dataPtr;
				double t0 = (double)ccGetUserDataInt(label) / 1000.0;
				//printf("albumCcObject_cb label %i %i %i %i, %i, %i %I64d: %I64d\n", label->id, msg, (int)data1, touchState, pos->pen, pos->dt, pos->time, pos->time-t0);
				
				if (pos->time - t0 >= ALBUM_KP_HOLDPERIOD){
					if (/*pageGetSec(vp) != PAGE_VKEYBOARD && */pageGet(vp) != PAGE_VKEYBOARD){
						TKEYPAD *kp = keyboardGetKeypad(spl);

						char title[MAX_PATH_UTF8+1];
	  					PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	  					playlistGetTitle(plc, spl->from, title, MAX_PATH_UTF8);

	  					int id = (playlistManagerGetPlaylistUID(vp->plm, plc)<<16) | (spl->from+1);
	  					ccEnable(kp);	// ensure pad is built by enabling it before use
	  					keypadListenerRemoveAll(kp);
	  					keypadListenerAdd(kp, label->id, KP_INPUT_OPENED|KP_INPUT_CLOSED|KP_INPUT_COMPLETE8, id);
	  					keypadEditboxSetBuffer8(&kp->editbox, title);
	  					keypadEditboxSetUndoBuffer8(&kp->editbox, title);
	  					keypadEditboxSetUserData(&kp->editbox, id); // only sets the hex value displayed
						pageSet(vp, PAGE_VKEYBOARD);
					}
				}
			}
		}
	}
	
	return 1;
}

void invalidateShelfAlbum (TVLCPLAYER *vp, TSPL *spl, int start)
{
	//printf("invalidateShelfAlbum %i\n",start);
	
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	int lastIdx = 0;
	if (plc)
		lastIdx = playlistGetTotal(plc)-1;

	if (/*spl->from*/start  > lastIdx) start = lastIdx;
	spl->resetTrack = start;
	spl->reset = 1;
}

static inline void albumGetPlaylistArtSingle (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int idx)
{
	//printf("albumGetPlaylistArtSingle %i '%s'\n",idx, plc->name);
	
	TPLAYLISTITEM *item = playlistGetItem(plc, idx);
	if (!item) return;

	if (item->objType == PLAYLIST_OBJTYPE_PLC){
		if (!item->obj.plc->artId)
			initiateAlbumArtRetrieval(vp, plc, idx, idx, vp->gui.artSearchDepth);
	}else{
		playlistMetaGetMeta(vp/*, vp->metac*/, plc, idx, idx, NULL);
	}
}

int renderThisShelfItemCB (void *ptr, const int idx, TFRAME *src, TFRAME *des, const int x, const int y, const int w, const int h, double modifier)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;

	if (src->udata_int == SHELF_IMGTYPE_NOART){
		PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
		if (plc)
			albumGetPlaylistArtSingle(vp, plc, idx);

		int noArtImgId = 0;
		if (idx == getSelectedItem(vp))
			noArtImgId = vp->gui.image[IMGC_NOART_SHELF_SELECTED];
		else if (idx == getPlayingItem(vp) && (vp->playlist.queued == vp->playlist.display))
			noArtImgId = vp->gui.image[IMGC_NOART_SHELF_PLAYING];

		if (noArtImgId){
			TFRAME *img = imageManagerImageAcquire(vp->im, noArtImgId);
			if (img){
				drawImageScaledOpacity(img, des, 0, 0, img->width, img->height, (int)x, (int)y, w, h, modifier, modifier*0.95);
				imageManagerImageRelease(vp->im, noArtImgId);
			}
		}
		return 1;
	}else{
		if (idx == getSelectedItem(vp))
			drawShadowUnderlay(vp, des, x-1, y-1, w+2, h+2, SHADOW_GREEN);
		else if (idx == getPlayingItem(vp) && (vp->playlist.queued == vp->playlist.display))
			drawShadowUnderlay(vp, des, x-1, y-1, w+2, h+2, SHADOW_BLUE);
		else
			drawShadowUnderlay(vp, des, x-1, y-1, w+2, h+2, SHADOW_BLACK);
	}
	return 1;
}

int getItemPlaylistImage (TVLCPLAYER *vp, PLAYLISTCACHE *plc, int *artcId)
{

	if (plc->artId){
		*artcId = plc->artId;
		return plc->artId;
	}

	const int playlistTotal = playlistGetTotal(plc);
	for (int i = 0; i < playlistTotal && i < 4; i++){
		*artcId = playlistGetArtId(plc, i);
		if (*artcId){
			if (!plc->artId) plc->artId = *artcId;
			return plc->artId;
		}
	}
	
	for (int i = playlistTotal-1; i >= 0 && i >= playlistTotal-4; i--){
		*artcId = playlistGetArtId(plc, i);
		if (*artcId){
			if (!plc->artId) plc->artId = *artcId;
			return plc->artId;
		}
	}

	for (int i = 0; i < 4; i++){
		TPLAYLISTITEM *item = playlistGetItem(plc, i);
		if (item){
			if (item->objType == PLAYLIST_OBJTYPE_PLC && !*artcId){
				if (getItemPlaylistImage(vp, item->obj.plc, artcId)){
					if (!plc->artId) plc->artId = *artcId;
					return plc->artId;
				}
			}
		}else{
			break;
		}
	}
	
	if (playlistTotal > 4){
		for (int i = playlistTotal-1; i > playlistTotal-5; i++){
			TPLAYLISTITEM *item = playlistGetItem(plc, i);
			if (item){
				if (item->objType == PLAYLIST_OBJTYPE_PLC && !*artcId){
					if (getItemPlaylistImage(vp, item->obj.plc, artcId)){
						if (!plc->artId) plc->artId = *artcId;
						return plc->artId;
					}
				}
			}else{
				break;
			}
		}
	}
	return 0;
}

int getItemImageCB (void *ptr, const int idx, int *artcId)
{
	if (idx < 0) return 0;

	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	
	if (!plc) return 0;
	if (idx >= playlistGetTotal(plc)) return 0;

	TPLAYLISTITEM *item = playlistGetItem(plc, idx);
	if (item){
		if (item->objType == PLAYLIST_OBJTYPE_PLC){
			if (getItemPlaylistImage(vp, item->obj.plc, artcId)){
				if (!plc->artId) plc->artId = *artcId;
				if (!*artcId) *artcId = plc->artId;
				return *artcId;
			}
		}else if (item->objType == PLAYLIST_OBJTYPE_TRACK){
			int artId = playlistGetArtId(plc, idx);
			if (artId){
				if (!plc->artId) plc->artId = artId;
				if (!*artcId) *artcId = artId;
				return artId;
			}
		}
	}

	*artcId = vp->gui.shelfNoArtId;
	return *artcId;
}

static inline int _getItemImageCB (void *ptr, const int idx, int *artcId)
{
	*artcId = 0;
	return getItemImageCB(ptr, idx, artcId);
	//printf("_getItemImageCB: %p %i %X\n", ptr, idx, *artcId);
	//return *artcId;
}

int albButtonPress (TSPL *spl, TCCBUTTON *btn, const int btnId, const TTOUCHCOORD *pos)
{
	TVLCPLAYER *vp = btn->cc->vp;
	spl->btns->t0 = getTickCount();
	
	switch (btnId){
	  case ALBBUTTON_PREV:
	  	ctrlPlayback(vp, VBUTTON_PRETRACK);
		break;
	  case ALBBUTTON_STOP:
	  	ctrlPlayback(vp, VBUTTON_STOP);
		break;
	  case ALBBUTTON_NEXT:
	  	ctrlPlayback(vp, VBUTTON_NEXTTRACK);
		break;
	  case ALBBUTTON_PLAY:
	    swipeReset(&spl->drag);
	  	if (getPlayState(vp) == 2){		// check is paused
			trackPlay(vp);
			
	  	}else if (!getPlayState(vp)){	// check if stopped
	  		PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
			if (plc){
				if (playlistGetCount(plc, PLAYLIST_OBJTYPE_TRACK)){
					//vp->queuedPlaylist = vp->displayPlaylist;	
				}else{
					break;
				}
			}else{
				break;
			}
	  		//plc = getQueuedPlaylist(vp);
	  		
	  		int trk = 0;
	  		if (spl->from/*plc->pr->selectedItem*/ >= 0)
	  			trk = spl->from/*plc->pr->selectedItem*/;
	  		else
	  			trk = plc->pr->playingItem;

			if (trk < 0){
				trk = 0;
				plc->pr->playingItem = trk;
			}

	  		if (pageGet(vp) == PAGE_PLY_SHELF)
				bringAlbumToFocus(spl, trk);
	  		if (startPlaylistTrack(vp, plc, trk))
	  			vp->playlist.queued = vp->playlist.display;

	  	}
		break;

	  case ALBBUTTON_PAUSE:
	    swipeReset(&spl->drag);
	  	trackPlayPause(vp);
		break;
		
	  case ALBBUTTON_META:{
		swipeReset(&spl->drag);
	  	PLAYLISTCACHE *plcD = getDisplayPlaylist(vp);
	  	
		const int pos = spl->from;
	  	if (playlistGetItemType(plcD, pos) != PLAYLIST_OBJTYPE_TRACK)
	  		break;
	  			  	
		TMETA *meta = pageGetPtr(vp, PAGE_META);
		TMETADESC *desc = &meta->desc;
		if (pos >= 0)
			desc->trackPosition = pos;
		else
			desc->trackPosition = 0;
		desc->uid = plcD->uid;

		playlistMetaGetMeta(vp, plcD, desc->trackPosition, desc->trackPosition+1, NULL);
		page2Set(vp->pages, PAGE_META, 1);
		break;
	  }
	  case ALBBUTTON_GOTOPLAYING:{
	  	swipeReset(&spl->drag);
		PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
		if (plc){
			plc->pr->selectedItem = -1; //plc->pr->playingItem;
			vp->playlist.display = vp->playlist.queued;
			
			
			//vp->plm->pr->selectedItem = vp->queuedPlaylist;
			invalidateShelfAlbum(vp, spl, plc->pr->playingItem);
		}
		break;
	  }
	  case ALBBUTTON_BACK:
	  	swipeReset(&spl->drag);
	  	page2SetPrevious(spl);
	  	break;

	  case ALBBUTTON_PLAYLISTBACK:{
	  	swipeReset(&spl->drag);
	  	PLAYLISTCACHE *plcD = getDisplayPlaylist(vp);
	  	if (plcD != getPrimaryPlaylist(vp)){
	  		setDisplayPlaylist(vp, plcD->parent);
			plcD->parent->pr->selectedItem = playlistGetPlaylistIndex(plcD->parent, plcD);
			
			if (plcD->parent->pr->selectedItem == -1) plcD->parent->pr->selectedItem = 0;
			invalidateShelfAlbum(vp, spl, plcD->parent->pr->selectedItem);
	  	}
		break;
	  }
	}
	return 1;
}

static inline int64_t labelCcObject_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	if (msg == CC_MSG_RENDER/* || obj->type != CC_BUTTON*/) return 1;
	//printf("labelCcObject_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);


	if (msg == LABEL_MSG_IMAGE_SELECTED_PRESS){
		//printf("labelCcObject_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);
		ccDisable(obj);	// close the button panel
		ccEnable(ccGetUserData(obj));	// enable other one
		return 1;
	}else if (msg == BUTTON_MSG_SELECTED_PRESS){
		TCCBUTTON *btn = (TCCBUTTON*)obj;
		if (!ccGetState(ccGetUserData(btn)))	// is label disabled
			return 1;
		else
			return albButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	}
	return 1;
}

static inline int albPanSetImage (TLABEL *label, wchar_t *pri, const int udata, const int canAnimate, const int x, const int y)
{
	wchar_t buffer[MAX_PATH+1];

	TCCBUTTON2 *btn = ccCreate(label->cc, PAGE_PLY_SHELF, CC_BUTTON2, labelCcObject_cb, NULL, 0, 0);
	ccSetUserDataInt(btn, udata);
	ccSetUserData(btn, label);
	const int imgId = artManagerImageAdd(btn->cc->vp->am, buildSkinD(label->cc->vp, buffer, pri));
	button2FaceImgcSet(btn, imgId, 0, 0.0, 0, 0);
	button2FaceHoverSet(btn, 1, COL_HOVER, 0.8);
	button2AnimateSet(btn, canAnimate);
	btn->isChild = 1;

	int itemId = labelCCCreate(label, btn, x, y);
	ccEnable(btn);

	return itemId;
}

static inline int albPanGetEnabledTotal (TBTNPANEL *btnpan)
{
	int ct = 0;
	for (int i = 0; i < ALBBUTTON_TOTAL; i++){
		if (btnpan->itemIds[i])
			ct += labelItemGetEnabledStatus(btnpan->base, btnpan->itemIds[i]);
	}
	return ct;
}

void albPanCalcPositions (TBTNPANEL *btnpan, const int set)
{
	// enable everything, disable what we don't need then center justify whats left
	
	for (int i = 0; i < ALBBUTTON_TOTAL; i++){
		if (btnpan->itemIds[i])
			labelItemEnable(btnpan->base, btnpan->itemIds[i]);
	}
	
	if (set == BTNPANEL_SET_PLAY){			// is playing so disable play button
		labelItemDisable(btnpan->base, btnpan->itemIds[ALBBUTTON_PLAY]);
	}else if (set == BTNPANEL_SET_PAUSE){	// is playing so disable pause button
		labelItemDisable(btnpan->base, btnpan->itemIds[ALBBUTTON_PAUSE]);
	}else if (set == BTNPANEL_SET_STOP){	// is stopped so disable both pause and stop
		labelItemDisable(btnpan->base, btnpan->itemIds[ALBBUTTON_PAUSE]);
		labelItemDisable(btnpan->base, btnpan->itemIds[ALBBUTTON_STOP]);
	}
	
	const int fw = getFrontBuffer(btnpan->base->cc->vp)->width;
	int btotal = albPanGetEnabledTotal(btnpan);

	int x_gap;
	if (btotal == 6)
		x_gap = 4.9816; //fw * 0.006227;
	else if (btotal == 5)
		x_gap = 21.6; //fw * 0.027;
	else
		x_gap = 50;//fw * 0.0625;
		
	//btnpan->width = ((btotal+1)*x_gap) + (btotal*btnpan->bwidth);
	ccSetPosition(btnpan->base, abs(fw - ccGetWidth(btnpan->base))/2, ccGetPositionY(btnpan->base));

	int x = 18;
	const int y = ((btnpan->height - btnpan->bheight)/2) + 2;
	
	for (int i = 0; i < ALBBUTTON_TOTAL; i++){
		if (btnpan->itemIds[i]){
			if (labelItemGetEnabledStatus(btnpan->base, btnpan->itemIds[i])){
				labelItemPositionSet(btnpan->base, btnpan->itemIds[i], x, y);
				x += btnpan->bwidth + x_gap;
			}
		}
	}
}

static inline int64_t ccbtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;
		
	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//printf("ccbtn_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);

	TCCBUTTON *btn = (TCCBUTTON*)object;
	//const int id = (int)data2;

	if (msg == BUTTON_MSG_SELECTED_PRESS)
		return albButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	return 1;
}

static inline void renderShelfItemDetail (TVLCPLAYER *vp, TSPL *spl, TFRAME *frame, PLAYLISTCACHE *plc, const int pos, const double cx, double cy)
{

	TPLAYLISTITEM *item = playlistGetItem(plc, pos);
	if (!item) return;

	TMETRICS metrics = {2, 0, 0, 0};
	lSetCharacterEncoding(vp->ml->hw, CMT_UTF8);
	lSetForegroundColour(vp->ml->hw, 0xFFFFFFFF);
	lSetBackgroundColour(vp->ml->hw, 0x00000000);
	//outlineTextEnable(vp->ml->hw, 100<<24 | 0x101010);
	
	int colour = COL_BLUE_SEA_TINT;		// 24bit only
	int alpha = 0.90 * 1000.0;	// alpha range of 0 to 1000
	int radius = 3;			// 0 - 255
	int blur = LTR_BLUR5;
	//lRenderEffectReset(frame->hw, ALBUM_FONT, blur);	// reset previous blur4 state
	lSetRenderEffect(frame->hw, blur);
	lSetFilterAttribute(frame->hw, blur, LTRA_BLUR_COLOUR, colour);
	lSetFilterAttribute(frame->hw, blur, LTRA_BLUR_RADIUS, radius);
	lSetFilterAttribute(frame->hw, blur, LTRA_BLUR_SETTOP, 1);
	lSetFilterAttribute(frame->hw, blur, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, blur, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, blur, LTRA_BLUR_ALPHA, alpha);
		
		
	if (item->objType == PLAYLIST_OBJTYPE_TRACK){
		char bufferTitle[MAX_PATH_UTF8+1];
		char bufferAlbum[MAX_PATH_UTF8+1];
		char bufferLen[64];

		unsigned int hash = playlistGetHash(plc, pos);
		tagRetrieveByHash(vp->tagc, hash, MTAG_Title, bufferTitle, MAX_PATH_UTF8);
		if (!*bufferTitle){
			playlistGetTitle(plc, pos, bufferTitle, MAX_PATH_UTF8);
			if (!*bufferTitle) strcpy(bufferTitle, " ");
		}
		tagRetrieveByHash(vp->tagc, hash, MTAG_Album, bufferAlbum, MAX_PATH_UTF8);
		//if (!*bufferAlbum) strcpy(bufferAlbum, " ");
		//if (!*bufferAlbum) cy += frame->height * (textVSpace/3.0);
		tagRetrieveByHash(vp->tagc, hash, MTAG_LENGTH, bufferLen, sizeof(bufferLen));
		if (!bufferLen[0] || !bufferLen[1]) strcpy(bufferLen, "0:00");


		// album
		if (*bufferAlbum){
			unsigned int shash = getHash(bufferAlbum) ^ ((PAGE_PLY_SHELF << 16) | PAGE_PLY_SHELF);
			TFRAME *strAlb = strcFindString(vp->strc, shash);
			if (!strAlb){
				strAlb = newStringEx(vp->ml->hw, &metrics, LFRM_BPP_32A, PF_FORCEAUTOWIDTH, ALBUM_FONT, bufferAlbum, frame->width, NSEX_RIGHT);
				strcAddString(vp->strc, strAlb, shash);
			}
			if (strAlb){
				int x = (frame->width - strAlb->width)/2;
				drawShadowedImage(strAlb, frame, x, frame->height * 0.150, COL_BLUE_SEA_TINT, 3, 0, 0);
			}
		}
			
		// track title
		unsigned int shash = getHash(bufferTitle) ^ ((PAGE_PLY_SHELF << 16) | PAGE_PLY_SHELF);
		TFRAME *strTitle = strcFindString(vp->strc, shash);
		if (!strTitle){
			strTitle = newStringEx(vp->ml->hw, &metrics, LFRM_BPP_32A, PF_FORCEAUTOWIDTH, ALBUM_FONT, bufferTitle, frame->width, NSEX_RIGHT);
			strcAddString(vp->strc, strTitle, shash);
		}
		if (strTitle){
			int x = (frame->width - strTitle->width)/2;
			drawShadowedImage(strTitle, frame, x, cy, COL_BLUE_SEA_TINT, 3, 0, 0);


			// track length and playlist position
			char bufferLenPos[128];
			__mingw_snprintf(bufferLenPos, sizeof(bufferLenPos), "#%i: %s", pos+1, bufferLen);
			shash = getHash(bufferLenPos) ^ ((PAGE_PLY_SHELF << 16) | PAGE_PLY_SHELF);
			TFRAME *strSubtext = strcFindString(vp->strc, shash);
			if (!strSubtext){
				strSubtext = lNewStringEx(vp->ml->hw, &metrics, LFRM_BPP_32A, PF_FORCEAUTOWIDTH|PF_IGNOREFORMATTING, ALBUM_FONT, bufferLenPos);
				strcAddString(vp->strc, strSubtext, shash);
			}
			if (strSubtext){
				cy += frame->height * spl->cfg.textVSpace + frame->height * 0.01;
				drawShadowedImage(strSubtext, frame, abs(frame->width - strSubtext->width)/2, cy, COL_BLUE_SEA_TINT, 3, 0, 0);
			}
		}
	}else if (item->objType == PLAYLIST_OBJTYPE_PLC){
		if ((plc=item->obj.plc)){
			
			// draw playlist id
			drawHex(frame, 2, frame->height*0.15, PLAYLISTPLM_PATH_FONT, playlistManagerGetPlaylistUID(vp->plm, plc), 0);
			
			const int flags = /*PF_CLIPWRAP|*/PF_MIDDLEJUSTIFY|PF_FORCEAUTOWIDTH;
			char bufferTitle[MAX_PATH_UTF8+1];
			playlistGetName(plc, bufferTitle, MAX_PATH_UTF8);
			if (!*bufferTitle) strcpy(bufferTitle, " ");
		
			unsigned int shash = getHash(bufferTitle) ^ ((PAGE_PLY_SHELF << 16) | PAGE_PLY_SHELF);
			TFRAME *strTitle = strcFindString(vp->strc, shash);
			if (!strTitle){
				//metrics.width = frame->width;
				strTitle = newStringEx(vp->ml->hw, &metrics, LFRM_BPP_32A, flags, ALBUM_FONT, bufferTitle, frame->width, NSEX_RIGHT);
				//printf("strTitle %p '%s'\n", strTitle, bufferTitle);
				strcAddString(vp->strc, strTitle, shash);
				//if (strTitle)
				//	printf("strTitle %i %i '%s'\n", strTitle->width, strTitle->height, bufferTitle);
			}
			
			if (strTitle){
				char buffer[128];
				
				// title
				int x = (frame->width - strTitle->width)/2;
				drawShadowedImage(strTitle, frame, x, cy, COL_BLUE_SEA_TINT, 3, 0, 0);
				
				// total items in playlist
				__mingw_snprintf(buffer, sizeof(buffer), "%i", playlistGetTotal(plc));
				shash = getHash(buffer) ^ ((PAGE_PLY_SHELF << 16) | PAGE_PLY_SHELF);
				
				TFRAME *strSubtext = strcFindString(vp->strc, shash);
				if (!strSubtext){
					strSubtext = lNewString(vp->ml->hw, LFRM_BPP_32A, flags|PF_IGNOREFORMATTING, ALBUM_FONT, buffer);
					strcAddString(vp->strc, strSubtext, shash);
				}
				if (strSubtext){
					cy += frame->height * spl->cfg.textVSpace;
					drawShadowedImage(strSubtext, frame, abs(frame->width - strSubtext->width)/2, cy, COL_BLUE_SEA_TINT, 3, 0, 0);
				}

				// position of playlist
				__mingw_snprintf(buffer, sizeof(buffer), "#%i", pos+1);
				shash = getHash(buffer) ^ ((PAGE_PLY_SHELF << 16) | PAGE_PLY_SHELF);
				
				strSubtext = strcFindString(vp->strc, shash);
				if (!strSubtext){
					strSubtext = lNewString(vp->ml->hw, LFRM_BPP_32A, flags|PF_IGNOREFORMATTING, ALBUM_FONT, buffer);
					strcAddString(vp->strc, strSubtext, shash);
				}
				if (strSubtext){
					cy += frame->height * spl->cfg.textVSpace;
					drawShadowedImage(strSubtext, frame, abs(frame->width - strSubtext->width)/2, cy, COL_BLUE_SEA_TINT, 3, 0, 0);
				}
			}
		}
	}
	lSetRenderEffect(frame->hw, LTR_DEFAULT);
	//outlineTextDisable(vp->ml->hw);
}

static inline void renderThisShelf (TVLCPLAYER *vp, TFRAME *frame, TSHELF *shelf, TSPL *spl, const unsigned int buttonDt)
{
	//printf("renderThisShelf %p %p\n", shelf, spl->shelf);
	
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	if (!shelfGetClientImageTotal(shelf)){
		shelfSetClientImageTotal(shelf, playlistGetTotal(plc));
		shelfSetState(shelf, spl->from, spl->to, 0.0);
	}else{
		shelfSetClientImageTotal(shelf, playlistGetTotal(plc));
	}
	
	if (shelf->ichanged){
		shelf->ichanged = 0;
		shelfCalcZDepth(shelf);
		shelfSortZDepth(shelf);
	}
	
	shelfRender(shelf, frame, 3);
	
	if (!plc){
		spl->from = shelfAnimateNext(shelf);
		return;
	}

	const int idx = shelf->simg[shelf->sortlist[shelf->totalImg-1]].imgSrcIndex;
	if (idx < 0){
		invalidateShelfAlbum(vp, spl, 0);
		spl->from = shelfAnimateNext(shelf);
		return;
	}

	renderShelfItemDetail(vp, spl, frame, plc, idx, 0, frame->height * spl->cfg.titleY);

	if (spl->from != spl->to || buttonDt < 1000)
		setTargetRate(vp, 25);
	else
		setTargetRate(vp, UPDATERATE_BASE);

	spl->from = shelfAnimateNext(shelf);
}

void resetAlbumPosition (TVLCPLAYER *vp, TSPL *spl, int track)
{
	//printf("resetAlbumPosition %i %p\n", track, spl->shelf);
	
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	shelfSetClientImageTotal(spl->shelf, playlistGetTotal(plc));

	if (track == -1) track = spl->from;
	shelfSetState(spl->shelf, track, track, 0.0);
	shelfSetAnimationStepRate(spl->shelf, spl->cfg.rate);
	spl->from = spl->to = track;
	
	sliderSetRange(spl->slider, 0, shelfGetClientImageTotal(spl->shelf)-1);
	sliderSetValue(spl->slider, spl->from);
	
	if (shelfGetClientImageTotal(spl->shelf) > 3)
		ccEnable(spl->slider);
	else
		ccDisable(spl->slider);
}

// TIMER_PLYALB_REFRESH
void plyAlbRefresh (TVLCPLAYER *vp)
{
#if 0
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	if (plc){
		if (plc->pr->selectedItem > playlistGetTotal(plc)-1)
			plc->pr->selectedItem = playlistGetTotal(plc)-1;
		invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), plc->pr->selectedItem);
		bringAlbumToFocus(pageGetPtr(vp, PAGE_PLY_SHELF), plc->pr->selectedItem);
	}
#else
	TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
	//spl->from = 0;
	//spl->to = 0;
	shelfFlush(spl->shelf);
	invalidateShelfAlbum(vp, spl, -1);
	bringAlbumToFocus(spl, -1);
#endif
}

int initiateAlbumArtRetrievalEx (TVLCPLAYER *vp, PLAYLISTCACHE *plc, int from, int to, int depth, TMETACOMPLETIONCB *mccb)
{
	for (int i = from; i <= to; i++){
		TPLAYLISTITEM *item = playlistGetItem(plc, i);
		if (item && item->objType == PLAYLIST_OBJTYPE_PLC){
			PLAYLISTCACHE *plcC = item->obj.plc;
			if (plcC){
				uint64_t t0 = getTickCount();
				if (t0 - plcC->artRetryTime < 1500)
					return --depth;
				plcC->artRetryTime = t0;

				if (depth > 0){
					if (!plcC->artId){
						int rightMost = 7;
						if (rightMost > playlistGetTotal(plcC)-1)
							rightMost = playlistGetTotal(plcC)-1;
						depth = initiateAlbumArtRetrievalEx(vp, plcC, 0, rightMost, depth-1, mccb);
					}
				}else{
					return depth;
				}
			}
		}else{
			playlistMetaGetMeta(vp, plc, i, i, mccb);
		}
	}
	
	playlistMetaGetMeta(vp, plc, 0, 0, mccb);
	playlistMetaGetMeta(vp, plc, playlistGetTotal(plc)-4, playlistGetTotal(plc)-1, mccb);
	return --depth;
}

int initiateAlbumArtRetrieval (TVLCPLAYER *vp, PLAYLISTCACHE *plc, int from, int to, int depth)
{
	return initiateAlbumArtRetrievalEx(vp, plc, from, to, depth, NULL);
}

int albumTouch (TSPL *spl, TTOUCHCOORD *pos, int flags, TVLCPLAYER *vp)
{
//	static unsigned int lastId = 0;
	
	// we don't want drag reports
	//if (pos->dt < 80 || flags) return -2;

	//if (flags == 3) return 0;
/*
	if (!flags){		// pen down
		if (lastId >= pos->id)
			return 0;
		lastId = pos->id;
	}else if (lastId != pos->id){
		return 0;	
	}
*/

	TSHELF *shelf = spl->shelf;

#if 1
	TTOUCHSWIPE *drag = &spl->drag;
	
	//printf("%i, %i %i %i %i\n", pos->count, pos->x, pos->dt, pos->pen, flags);
	
	if (!pos->pen && flags == 0 && pos->dt > 80){			// pen down
		//printf("  @@@ pen down %i\n", pos->dt);
		drag->state = 1;
		drag->t0 = 0.0;//getTime(vp);
		drag->sx = pos->x;
		drag->sy = pos->y;
			
		if (shelf->direction == 1)
			spl->to = shelf->to = spl->from + 1;
		else if (shelf->direction == 2)
			spl->to = shelf->to = spl->from - 1;

		shelf->ichanged = 1;
		shelfSetAnimationStepRate(shelf, spl->cfg.rate);
		//invalidateShelfAlbum(vp, spl, spl->to);
		
	}else if (!pos->pen && drag->state == 1 && flags == 1){	// dragging
		if (drag->t0 < 1.0)
			drag->t0 = getTime(vp);
		drag->ex = pos->x;
		drag->ey = pos->y;
		drag->dx = drag->ex - drag->sx;
		drag->dy = drag->ey - drag->sy;

		//printf("  @@@ pen drag %i %i, %i\n", drag->dx, drag->dy, pos->dt);

		if (abs(drag->dx) >= drag->dragMinH)
			return 0;
		
	}else if (pos->pen && drag->state == 1 && flags == 3){	// pen up
		drag->ex = pos->x;
		drag->ey = pos->y;
		drag->dx = drag->ex - drag->sx;
		drag->dy = drag->ey - drag->sy;

		//printf("  @@@ pen up %i %i, %i\n", drag->dx, drag->dy, pos->dt);
		
		if (abs(drag->dx) > drag->dragMinH){
			drag->dt = getTime(vp) - drag->t0;
			drag->state = 0;
			
			double adjustment = drag->dx * (drag->dt/1000.0) * drag->velocityFactor;
			int to = shelf->to - adjustment;
			if (to < 0) to = 0;
			
			if (to > shelf->clientImageTotal-1) to = shelf->clientImageTotal-1;
			spl->to = shelf->to = to;

			//printf("drag %i,%i %i %.1fms: %.1f %i->%i %i\n", drag->ex, drag->ey, drag->dx, drag->dt, adjustment, shelf->from, shelf->to, shelf->totalImg);

			if (spl->from < shelf->to)
				shelf->direction = 1;
			else if (spl->from > shelf->to)
				shelf->direction = 2;
			shelf->ichanged = 1;
			
			double aniRate = abs(shelf->from - shelf->to)* (spl->cfg.rate/2.00);
			//printf("   @@@ adjust %i, %i %i, %f\n", to, shelf->from, shelf->to, aniRate);
			
			shelfSetAnimationStepRate(shelf, aniRate);
			renderSignalUpdate(vp);
			return 0;
		}
	}
#endif

#if 1	
	// check if album is pressed. if so bring that album to focus
	if (!flags){
		TSHELFIMG *simg = shelf->simg;
		// copy out the zOrder so we have something consistent
		// then rearrange so closest image is first in list
		int zOrder[shelf->totalImg];
		for (int i = 0, j = shelf->totalImg-1; i < shelf->totalImg; i++, j--)
			zOrder[i] = shelf->sortlist[j];

		// check if an image is pressed (selected), is so then bring it forward to center screen
		for (int i = 0; i < shelf->totalImg; i++){
			int idx = zOrder[i];
			TLPOINTEX *ipos = &simg[idx].pos;
			
			if (pos->y > ipos->y1 && pos->y < ipos->y1+ipos->y2){
				if (pos->x > ipos->x1 && pos->x < ipos->x1+ipos->x2){

					if (!i && simg[idx].imgSrcIndex != shelf->to){
						invalidateShelfAlbum(vp, spl, simg[idx].imgSrcIndex);
						return 0;
						
					}else if (!i && simg[idx].imgSrcIndex == shelf->to && simg[idx].imgSrcIndex >= 0){
						PLAYLISTCACHE *plcD = getDisplayPlaylist(vp);
						TPLAYLISTITEM *item = playlistGetItem(plcD, simg[idx].imgSrcIndex);
						if (item){
							if (item->objType == PLAYLIST_OBJTYPE_PLC){
								PLAYLISTCACHE *plc = item->obj.plc;
								//int plidx = playlistManagerGetPlaylistIndex(vp->plm, plc);
					  			//vp->displayPlaylist = plidx;
					  			setDisplayPlaylist(vp, plc);
					  			
					  			
								//vp->plm->pr->selectedItem = plidx;
								plcD->pr->selectedItem = simg[idx].imgSrcIndex;
								invalidateShelfAlbum(vp, spl, 0);
								return 0;
							}
						}else{
							return 0;
						}
						if (plcD->pr->selectedItem != simg[idx].imgSrcIndex)
							plcD->pr->selectedItem = simg[idx].imgSrcIndex;
						else
							plcD->pr->selectedItem = -1;
						return 0;

					}else if (simg[idx].imgSrcIndex >= 0 && simg[idx].imgSrcIndex < shelfGetClientImageTotal(shelf)){
						shelf->ichanged = 1;
						spl->to = shelf->to = simg[idx].imgSrcIndex;
						if (shelf->from < shelf->to)
							shelf->direction = 1;
						else if (shelf->from > shelf->to)
							shelf->direction = 2;
						//else (shelf->from == shelf->to)	
							//shelf->direction = 0;
					
						shelfSetAnimationStepRate(shelf, abs(shelf->from - shelf->to)* spl->cfg.rate);
						return 0;
					}
				}
			}
		}
	}
#endif

	if (!flags)
		shelfSetAnimationStepRate(shelf, spl->cfg.rate);
	return 0;
}


static inline int page_albumRender (TSPL *spl, TVLCPLAYER *vp, TFRAME *frame)
{
	if (spl->reset){
		spl->reset = 0;
		resetAlbumPosition(vp, spl, spl->resetTrack);
	}

	//albumGetPlaylistArt(vp, getDisplayPlaylist(vp), spl);

	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	if (plc == getPrimaryPlaylist(vp))
		buttonsStateSet(spl->btns, ALBBUTTON_PLAYLISTBACK, 0);
	else
		buttonsStateSet(spl->btns, ALBBUTTON_PLAYLISTBACK, 1);

  	if (playlistGetItemType(plc, spl->from) != PLAYLIST_OBJTYPE_TRACK)
  		labelItemDisable(spl->albpan.base, spl->albpan.itemIds[ALBBUTTON_META]);
	else if (ccGetState(spl->albpan.base))
		labelItemEnable(spl->albpan.base, spl->albpan.itemIds[ALBBUTTON_META]);


	ccRender(spl->albpan.base, frame);
	ccRender(spl->labPanClosed, frame);
	
	const unsigned int dt = buttonsRenderAll(spl->btns, frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);
	renderThisShelf(vp, frame, spl->shelf, spl, dt);

	//int marqueeY = 8;
	//if (ccGetState(spl->albpan.base))
	//	marqueeY = 64;

	sliderSetValue(spl->slider, spl->from);
	ccRender(spl->slider, frame);

	if (vp->gui.cursor.isHooked && ccGetState(spl->slider)){
		TLPOINTEX rt;
		sliderGetSliderRect(spl->slider, &rt);
		int w = (rt.x2 - rt.x1) + 1;
		int h = (rt.y2 - rt.y1) + 1;		
		int mx = vp->gui.cursor.dx - rt.x1;
		int my = vp->gui.cursor.dy - rt.y1;

		if (mx >= -4 && mx <= w+4){
			if (my >= -4 && my <= h+4){
				if (mx < 0) mx = 0;
				else if (mx > w) mx = w;
				
				int64_t value = sliderGetValueLookup(spl->slider, mx);
				if (value < 0) value = 0;
				drawAlbumPositionText(vp, frame, spl->cfg.sliderY, value, vp->gui.cursor.dx, vp->gui.cursor.dy);
			}
		}
	}

#if DRAWTOUCHRECTS
	ccRender(spl->title, frame);	// is used for touch input only so nothing to render
#endif

	//TVIDEOOVERLAY *playctrl = pageGetPtr(vp, PAGE_OVERLAY);
	//marqueeDraw(vp, frame, playctrl->marquee, 2, marqueeY);
	
	return 1;
}

static inline int page_albumInput (TSPL *spl, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_TOUCH_DOWN:
	  case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  	return albumTouch(spl, pos, flags, vp);
#if 0
	  case PAGE_IN_WHEEL_FORWARD:
	  case PAGE_IN_WHEEL_BACK:{
		TSLIDER *slider = spl->slider;
		int64_t max = 1;
		sliderGetRange(slider, NULL, &max);
		int64_t diff = 1;
		int64_t val = sliderGetValue(slider);
				
		if (msg == PAGE_IN_WHEEL_FORWARD)
			val -= diff;
		else
			val += diff;
		sliderSetValue(slider, val);
		return 1;
	  }
#else
	  case PAGE_IN_WHEEL_FORWARD:{
		int next = spl->to - 1;
		if (next < 0) next = 0;
		bringAlbumToFocus(spl, next);
		return 1;
	  }
	  case PAGE_IN_WHEEL_BACK:{
	  	const int total = playlistGetTotal(getDisplayPlaylist(vp));
		if (!total) return 0;

		int next = spl->to + 1;
		if (next >= total) next = total-1;
		bringAlbumToFocus(spl, next);
		return 1;
	  }
#endif
	}
	
	return 0;
}

static inline int page_albumStartup (TSPL *spl, TVLCPLAYER *vp, const int fw, const int fh)
{
	spl->reset = 1;
	spl->from = 0;
	spl->to = spl->from;

	spl->drag.state = 0;
	spl->drag.t0 = 0.0;		// drag state time
	spl->drag.dt = 0.0;		// drag end time
	spl->drag.sx = 0;		// start
	spl->drag.sy = 0;
	spl->drag.ex = 0;		// end
	spl->drag.ey = 0;
	spl->drag.dx = 0;		// delta
	spl->drag.dy = 0;
	spl->drag.dragMinH = fw * 0.025;		// (0.05 = 24 pixels @ 480 width)
	spl->drag.dragMinV = spl->drag.dragMinH;
	spl->drag.velocityFactor = 0.30;

	settingsGet(vp, "album.shelf.sigma", &spl->cfg.sigma);
	settingsGet(vp, "album.shelf.rho",  &spl->cfg.rho);
	settingsGet(vp, "album.shelf.expMult", &spl->cfg.expMult);
	settingsGet(vp, "album.shelf.rate", &spl->cfg.rate);
	settingsGet(vp, "album.shelf.spacing", &spl->cfg.spacing);
	settingsGet(vp, "album.shelf.art.scaleMult", &spl->cfg.artScaleMult);
	settingsGet(vp, "album.shelf.art.opacityMult", &spl->cfg.artOpacityMult);
	settingsGet(vp, "album.slider.x", &spl->cfg.sliderX);
	settingsGet(vp, "album.slider.y", &spl->cfg.sliderY);
	settingsGet(vp, "album.slider.width", &spl->cfg.sliderW);
	settingsGet(vp, "album.titleY", &spl->cfg.titleY);
	settingsGet(vp, "album.subTextSpace", &spl->cfg.textVSpace);

	
	spl->albpan.itemIds = my_calloc(ALBBUTTON_TOTAL, sizeof(int));
	spl->albpan.width = fw;
	if (spl->albpan.width > 400) spl->albpan.width = 400;
	spl->albpan.height = 70;
	
	
	// create a panel base to contain media (control) buttons
	TLABEL *label = ccCreate(vp->cc, PAGE_PLY_SHELF, CC_LABEL, labelCcObject_cb, &vp->gui.ccIds[CCID_LABEL_ALBPANOPEN], spl->albpan.width, spl->albpan.height);
	int imgId = imageManagerImageAdd(vp->im, L"shelf/dropmenuopen.png");
	labelImgcCreate(label, imgId, 1, 0, 0);

	spl->albpan.base = label;
	ccSetPosition(label, abs(fw - ccGetWidth(label))/2, 0);
	labelRenderFlagsSet(label, LABEL_RENDER_CCOBJ|LABEL_RENDER_IMAGE);
	
	
	spl->albpan.itemIds[ALBBUTTON_GOTOPLAYING] = albPanSetImage(label, L"shelf/splplaying.png", ALBBUTTON_GOTOPLAYING, 1, 0, 0);
	spl->albpan.itemIds[ALBBUTTON_PREV] = albPanSetImage(label, L"shelf/prev.png", ALBBUTTON_PREV, 1, 0, 0);
	spl->albpan.itemIds[ALBBUTTON_PLAY] = albPanSetImage(label, L"shelf/play.png", ALBBUTTON_PLAY, 0, 0, 0);
	spl->albpan.itemIds[ALBBUTTON_PAUSE] = albPanSetImage(label,L"shelf/pause.png", ALBBUTTON_PAUSE, 0, 0, 0);
	spl->albpan.itemIds[ALBBUTTON_STOP] = albPanSetImage(label, L"shelf/stop.png", ALBBUTTON_STOP, 0, 0, 0);
	spl->albpan.itemIds[ALBBUTTON_NEXT] = albPanSetImage(label, L"shelf/next.png", ALBBUTTON_NEXT, 1, 0, 0);
	spl->albpan.itemIds[ALBBUTTON_META] = albPanSetImage(label, L"shelf/meta.png", ALBBUTTON_META, 0, 0, 0);
	spl->albpan.bwidth = ccGetWidth(labelCCGet(label, spl->albpan.itemIds[ALBBUTTON_GOTOPLAYING]));
	spl->albpan.bheight = ccGetHeight(labelCCGet(label, spl->albpan.itemIds[ALBBUTTON_GOTOPLAYING]));

	albPanCalcPositions(&spl->albpan, BTNPANEL_SET_STOP);
	ccDisable(label);

	// create closed panel
	label = ccCreate(vp->cc, PAGE_PLY_SHELF, CC_LABEL, labelCcObject_cb, &vp->gui.ccIds[CCID_LABEL_ALBPANCLOSED], 8, 8);
	imgId = imageManagerImageAdd(vp->im, L"shelf/dropmenuclosed.png");
	labelImgcCreate(label, imgId, 1, 0, 0);
	
	spl->labPanClosed = label;
	labelRenderFlagsSet(label, LABEL_RENDER_IMAGE);
	ccSetPosition(label, abs(fw - ccGetWidth(label))/2, 0);
	ccEnable(label);
	
	ccSetUserData(spl->labPanClosed, spl->albpan.base);		// if one label is enabled then other must be disabled
	ccSetUserData(spl->albpan.base, spl->labPanClosed);
	
	
	spl->btns = buttonsCreate(vp->cc, PAGE_PLY_SHELF, ALBBUTTON_TOTAL, ccbtn_cb);
	spl->btns->t0 = 0;
	
	TCCBUTTON *btn = buttonsCreateButton(spl->btns, L"common/back_left92.png", NULL, ALBBUTTON_PLAYLISTBACK, 1, 1, 2, 2);
	btn = buttonsCreateButton(spl->btns, L"common/back_right92.png", NULL, ALBBUTTON_BACK, 1, 0, 0, 0);
	ccSetPosition(btn, fw - ccGetWidth(btn), 2);


	spl->shelf = shelfNew(7, spl->cfg.sigma, spl->cfg.spacing, spl->cfg.rate, spl->cfg.rho, spl->cfg.expMult, spl->cfg.artScaleMult, spl->cfg.artOpacityMult);
	shelfSetClientImageLookupCB(spl->shelf, (void*)vp, _getItemImageCB);
	shelfSetClientFreeImageCB(spl->shelf, freeImageCB, vp->am);
	shelfSetClientRenderCB(spl->shelf, renderThisShelfItemCB, (void*)vp);


	spl->slider = ccCreate(vp->cc, PAGE_PLY_SHELF, CC_SLIDER_HORIZONTAL, albumCcObject_cb, &vp->gui.ccIds[CCID_SLIDER_ALBUM], 0, 0);
	sliderFaceSet(spl->slider, SLIDER_FACE_LEFT, L"cc/slider_h_thin_left.png");
	sliderFaceSet(spl->slider, SLIDER_FACE_RIGHT, L"cc/slider_h_thin_right.png");
	sliderFaceSet(spl->slider, SLIDER_FACE_MID, L"cc/slider_h_thin_mid.png");
	sliderFaceSet(spl->slider, SLIDER_FACE_TIP, L"cc/slider_h_thin_tip.png");
	sliderFacesApply(spl->slider);
	ccSetMetrics(spl->slider, fw*spl->cfg.sliderX, fh*spl->cfg.sliderY, fw*spl->cfg.sliderW, 16);
	sliderSetRange(spl->slider, 0, 10);
	sliderSetValue(spl->slider, 0);
	ccEnable(spl->slider);
	
	
	// title change underlay (keypad activation zone)
	spl->title = ccCreate(vp->cc, PAGE_PLY_SHELF, CC_LABEL, albumCcObject_cb, &vp->gui.ccIds[CCID_LABEL_ALBTITLE], 400, 80);
	labelRenderFlagsSet(spl->title, LABEL_RENDER_INV);
	ccSetPosition(spl->title, (fw-400)/2, fh*spl->cfg.titleY);
	ccEnable(spl->title);
	
	return 1;
}

static inline int page_albumInitalize (TSPL *spl, TVLCPLAYER *vp, const int width, const int height)
{
	//printf("page_albumInitalize %x %x\n", spl->noartId, vp->gui.shelfNoArtId);
	setPageAccessed(vp, PAGE_PLY_SHELF);
	page2InputDragEnable(vp->pages, PAGE_PLY_SHELF);
	
	return 1;
}   
    
static inline int page_albumShutdown (TSPL *spl, TVLCPLAYER *vp)
{   
	my_free(spl->albpan.itemIds);
	
	ccDelete(spl->albpan.base);
	ccDelete(spl->labPanClosed);
	ccDelete(spl->title);
	ccDelete(spl->slider);
	shelfDelete(spl->shelf);
	
	buttonsDeleteAll(spl->btns);	
	return 1;
}

static inline int page_albumRenderInit (TSPL *spl, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	//printf("page_albumRenderInit %x %x\n", spl->noartId, vp->gui.shelfNoArtId);
	
	if (vp->gui.shelfNoArtId){
		spl->noartId = vp->gui.shelfNoArtId;
		
	}else{
		wchar_t buffer[MAX_PATH+1];
		spl->noartId = artManagerImageAdd(vp->am, buildSkinD(vp,buffer,L"shelf/noart.png"));
		vp->gui.shelfNoArtId = spl->noartId;
	}
	
	spl->shelf->am = vp->am;
	spl->shelf->imgNoArtId = spl->noartId;
	return 1;
}

static inline int page_albumRenderBegin (TSPL *spl, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	TFRAME *img = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_NOART_SHELF_SELECTED]);
	if (img){
		img->udata_int = SHELF_IMGTYPE_NOART;
		imageManagerImageRelease(vp->im, vp->gui.image[IMGC_NOART_SHELF_SELECTED]);
	}
	
	img = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_NOART_SHELF_PLAYING]);
	if (img){
		img->udata_int = SHELF_IMGTYPE_NOART;
		imageManagerImageRelease(vp->im, vp->gui.image[IMGC_NOART_SHELF_PLAYING]);
	}
	
	return 1;
}

int page_plyAlbCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TSPL *album = (TSPL*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_plyAlbCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_albumRender(album, album->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_albumInput(album, album->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_START){
		return page_albumRenderBegin(album, album->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		
	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_albumRenderInit(album, album->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_albumStartup(album, album->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_albumInitalize(album, album->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_albumShutdown(album, album->com->vp);
		
	}
	
	return 1;
}


