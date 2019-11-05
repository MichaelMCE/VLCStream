
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



int64_t plmCcObject_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	if (msg == CC_MSG_RENDER) return 1;
	//printf("plmCcObject_cb. id:%i, objType:%i, msg:%i, data1:%i, data2:%i\n", obj->id, obj->type, msg, data1, data2);


	if (obj->type == CC_SLIDER_HORIZONTAL){
		TSLIDER *slider = (TSLIDER*)object;
		TVLCPLAYER *vp = (TVLCPLAYER*)obj->cc->vp;
		
		if (slider->id == ccGetId(vp, CCID_SLIDER_PLAYLISTS)){
			switch (msg){
		  	  case SLIDER_MSG_VALCHANGED:{
		  	  	TSPL *spl = pageGetPtr(vp, PAGE_PLY_FLAT);
		  	  	swipeReset(&spl->drag);
				invalidateShelf(vp, spl, sliderGetValue(slider));
				break;
			  }
			}
		}
	}else if (obj->type == CC_LABEL){
		TLABEL *label = (TLABEL*)object;
		TVLCPLAYER *vp = (TVLCPLAYER*)obj->cc->vp;
		
		if (label->id == ccGetId(vp, CCID_LABEL_SPLTITLE)){
			 if (msg == KP_MSG_PAD_OPENED){
				//printf("spl KP_MSG_PAD_OPENED: %I64d %X %p\n", data1, (int)data2, dataPtr);
				TSPL *spl = pageGetPtr(vp, PAGE_PLY_FLAT);
				swipeReset(&spl->drag);
				invalidateShelf(vp, spl, sliderGetValue(spl->slider));
				
			}else if (msg == KP_MSG_PAD_CLOSED){
				//printf("spl KP_MSG_PAD_CLOSED: %I64d %X %p\n", data1, (int)data2, dataPtr);
				TKEYBOARD *vkey = pageGetPtr(vp, PAGE_VKEYBOARD);
				keypadListenerRemove(vkey->kp, data1);
				
			}else if (msg == KP_MSG_PAD_ENTER){
				//printf("spl KP_MSG_PAD_ENTER: %I64d %X %p\n", data1, (int)data2, dataPtr);
				
				int dataType = data1;
				if (dataType == KP_INPUT_COMPLETE8){		// UTF8 only
					//printf("album KP_MSG_PAD_ENTER: %I64d %X %p: '%s'\n", data1, (int)data2, dataPtr, (char*)dataPtr);
					if (strlen(dataPtr) < 1) return 0;
					int uid = data2;
					if (uid <= (PLAYLIST_UID_BASE+1)) return 0;	// do not adjust root playlist title
					
					PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
					if (plc)
						playlistSetName(plc, dataPtr);
				}
			}else if (msg == LABEL_MSG_BASE_SELECTED_PRESS){
				TSPL *spl = pageGetPtr(vp, PAGE_PLY_FLAT);
				swipeReset(&spl->drag);
				invalidateShelf(vp, spl, sliderGetValue(spl->slider));

				TTOUCHCOORD *pos = (TTOUCHCOORD*)dataPtr;
				ccSetUserDataInt(label, pos->time * 1000.0);
					
			}else if (msg == LABEL_MSG_BASE_SELECTED_RELEASE){
				TSPL *spl = pageGetPtr(vp, PAGE_PLY_FLAT);
				swipeReset(&spl->drag);
				invalidateShelf(vp, spl, sliderGetValue(spl->slider));

				TTOUCHCOORD *pos = (TTOUCHCOORD*)dataPtr;
				double t0 = (double)ccGetUserDataInt(label) / 1000.0;
				//printf("splCcObject_cb label %i %i %i %i, %i, %i %I64d: %I64d\n", label->id, msg, (int)data1, touchState, pos->pen, pos->dt, pos->time, pos->time-t0);
				
				if (pos->time - t0 >= ALBUM_KP_HOLDPERIOD){
					if (/*pageGetSec(vp) != PAGE_VKEYBOARD &&*/ pageGet(vp) != PAGE_VKEYBOARD){
						TKEYPAD *kp = keyboardGetKeypad(spl);

						char title[MAX_PATH_UTF8+1];
	  					PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	  					playlistGetName(plc, title, MAX_PATH_UTF8);

	  					//printf("%i %i #%s#\n", spl->from, spl->to, title);
	  			
	  					int id = playlistManagerGetPlaylistUID(vp->plm, plc);
	  					if (id <= PLAYLIST_UID_BASE+1) return 0;
	  					
	  					ccEnable(kp);	// ensure pad is built by enabling it before use
	  					keypadListenerRemove(kp, label->id);
	  					keypadListenerAdd(kp, label->id, KP_INPUT_OPENED|KP_INPUT_CLOSED|KP_INPUT_COMPLETE8, id);
	  					keypadEditboxSetBuffer8(&kp->editbox, title);
	  					keypadEditboxSetUndoBuffer8(&kp->editbox, title);
	  					keypadEditboxSetUserData(&kp->editbox, id); // only sets the displayed hex value 
						pageSet(vp, PAGE_VKEYBOARD);
					}
				}
				
			}
		}
	}
	return 1;
}

void swipeReset (TTOUCHSWIPE *swipe)
{
  	swipe->state = SWIPE_UP;
  	swipe->adjust = 0.0;
  	swipe->decayAdjust = 0.0;
}

void bringAlbumToFocus (TSPL *spl, const int idx)
{
	TSHELF *shelf = spl->shelf;

	//if (idx >= 0)
		//shelfSetState(shelf, spl->from, idx, 0.0);
	
	shelf->ichanged = 1;
	if (idx >= 0)
		spl->to = shelf->to = idx;
		
	if (shelf->from < shelf->to)
		shelf->direction = 1;
	else if (shelf->from > shelf->to)
		shelf->direction = 2;
	//else (shelf->from == shelf->to)
		//shelf->direction = 0;
					
	double seekRate = abs(shelf->from - shelf->to) * spl->cfg.rate;
	if (seekRate > shelf->spacing)
		seekRate = shelf->spacing;
	else if (seekRate < spl->cfg.rate)
		seekRate = spl->cfg.rate;
	shelfSetAnimationStepRate(shelf, seekRate);
}

// TIMER_PLYPLM_REFRESH
void plyPlmRefresh (TVLCPLAYER *vp)
{
	TSPL *spl = pageGetPtr(vp, PAGE_PLY_FLAT);
	shelfFlush(spl->shelf);
	invalidateShelfAlbum(vp, spl, -1);
}

void resetShelfPosition (TVLCPLAYER *vp, TSPL *spl, int track)
{
	//printf("resetShelfPosition %p\n", spl->shelf);
	
	shelfSetClientImageTotal(spl->shelf, playlistManagerGetTotal(vp->plm));

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

int getPlaylistImageCB (void *ptr, const int idx, int *artcId)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;
	
	PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, idx);
	if (!plc) return 0;

	if (plc->artId){
		*artcId = plc->artId;
		return plc->artId;
	}

	const int playlistTotal = playlistGetTotal(plc);
	for (int i = 0; i < playlistTotal && i < 10; i++){
		int artId = playlistGetArtId(plc, i);
		if (artId){
			plc->artId = artId;
			*artcId = artId;
			return artId;
		}
	}
	
	for (int i = playlistTotal-1; i >= 0 && i >= playlistTotal-4; i--){
		int artId = playlistGetArtId(plc, i);
		if (artId){
			plc->artId = artId;
			*artcId = artId;
			return artId;
		}
	}

	*artcId = vp->gui.shelfNoArtId;
	return *artcId;
}


void freeImageCB (void *ptr, const int idx, const int artId)
{
}


int renderShelfItemCB (void *ptr, const int idx, TFRAME *src, TFRAME *des, const int x, const int y, const int w, const int h, double modifier)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;	
	
	if (src->udata_int == SHELF_IMGTYPE_NOART){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, idx);
		if (plc){
			if (plc && !plc->artId)
				initiateAlbumArtRetrieval(vp, plc, idx, idx, vp->gui.artSearchDepth);

			if (idx == playlistManagerGetIndexByUID(vp->plm, getQueuedPlaylistUID(vp))){
				TFRAME *img = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_NOART_SHELF_PLAYING]);
				drawImageScaledOpacity(img, des, 0, 0, img->width, img->height, (int)x, (int)y, w, h, modifier, modifier*0.90);
				imageManagerImageRelease(vp->im, vp->gui.image[IMGC_NOART_SHELF_PLAYING]);
			}
		}
		return 1;
	}
	

	drawShadowUnderlay(vp, des, x-1, y-1, w+2, h+2, SHADOW_BLACK);
#if 1
	int a = (int)(modifier * 127.0)&0xFF;
	lDrawRectangle(des, x-1, y-1, x+w, y+h, (a<<24) | COL_BLACK);
#else
	int a = (int)(modifier * 255.0)&0xFF;
	int col;
	
	if (idx == playlistManagerGetIndexByUID(vp->plm, getQueuedPlaylistUID(vp))){
		col = COL_BLUE_SEA_TINT;
		a /= 1.5;
		lDrawRectangle(des, x-2, y-2, x+w+1, y+h+1, (a<<24) | col);
	}else if (idx == playlistManagerGetIndexByUID(vp->plm, getDisplayPlaylistUID(vp))){
		col = COL_GREEN_TINT;
		a /= 2.2;
		lDrawRectangle(des, x-2, y-2, x+w+1, y+h+1, (a<<24) | col);
	}else{
		col = 0xFFFFFF;
	}
	lDrawRectangle(des, x-1, y-1, x+w, y+h, (a<<24) | col);
#endif
	return 1;
}

int plmButtonPress (TSPL *spl, TCCBUTTON *btn, const int btnId, const TTOUCHCOORD *pos)
{
	//TVLCPLAYER *vp = spl->com->vp;
	//spl->btns->t0 = getTickCount();
	
	switch (btnId){
	  case SPLBUTTON_BACK:
	  	swipeReset(&spl->drag);
		page2SetPrevious(spl);
	}
	return 1;
}

static inline int64_t ccbtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;
		
	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//printf("ccbtn_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);

	TCCBUTTON *btn = (TCCBUTTON*)object;
	//const int id = (int)data2;

	if (msg == BUTTON_MSG_SELECTED_PRESS)
		return plmButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	return 1;
}

static inline void getPlaylistRoute (PLAYLISTCACHE *plc, char *buffer, const size_t len)
{
	*buffer = 0;
	if (plc->parent){
		getPlaylistRoute(plc->parent, buffer, len);
		strncat(buffer, plc->title, len);
	}else{
		strncat(buffer, "\\", len);
	}
	strncat(buffer, "\\", len);
}

static inline void renderShelf (TVLCPLAYER *vp, TFRAME *frame, TSHELF *shelf, TSPL *spl, const unsigned int buttonDt)
{

	//printf("renderShelf %p %p\n", shelf, spl->shelf);

	if (!shelfGetClientImageTotal(shelf)){
		shelfSetClientImageTotal(shelf, playlistManagerGetTotal(vp->plm));
		shelfSetState(shelf, spl->from, spl->to, 0.0);
	}else{
		shelfSetClientImageTotal(shelf, playlistManagerGetTotal(vp->plm));
	}
	
	if (shelf->ichanged){
		shelf->ichanged = 0;
		shelfCalcZDepth(shelf);
		shelfSortZDepth(shelf);
	}
	
	shelfRender(shelf, frame, 3);

	// draw the text
	const int idx = shelf->simg[shelf->sortlist[shelf->totalImg-1]].imgSrcIndex;
	PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, idx);
	if (plc){
		lSetCharacterEncoding(vp->ml->hw, CMT_UTF8);
		lSetForegroundColour(vp->ml->hw, 0xFFFFFFFF);
		lSetBackgroundColour(vp->ml->hw, 0x00000000);
		outlineTextEnable(vp->ml->hw, 100<<24 | 0x101010);
		
		const int flags = PF_CLIPWRAP|PF_MIDDLEJUSTIFY|PF_FORCEAUTOWIDTH|PF_IGNOREFORMATTING;
		char buffer[MAX_PATH_UTF8+1];
		TMETRICS metrics;
		metrics.x = 1;
		metrics.y = 0;
		metrics.width = 0;
		metrics.height = 0;
		
		// draw playlist UID
		drawHex(frame, 2, frame->height*0.085, PLAYLISTPLM_PATH_FONT, playlistManagerGetPlaylistUID(vp->plm, plc), 0);
		
		getPlaylistRoute(plc, buffer, MAX_PATH_UTF8);
		// TODO: remove double \\ from end of above path ^^
		
		unsigned int shash = getHash(buffer) ^ ((PAGE_PLY_SHELF << 16) | PAGE_PLY_FLAT);
		TFRAME *path = strcFindString(vp->strc, shash);
		if (!path){
			path = newStringEx(vp->ml->hw, &metrics, LFRM_BPP_32A, flags, PLAYLISTPLM_PATH_FONT, buffer, frame->width-2, NSEX_RIGHT);
			strcAddString(vp->strc, path, shash);
		}
		if (path)
			drawShadowedImage(path, frame, 2, frame->height*0.13, 0, 1, 1, 1);

		playlistGetName(plc, buffer, MAX_PATH_UTF8);
		if (!*buffer) strcpy(buffer, " ");
		
		shash = getHash(buffer) ^ ((PAGE_PLY_FLAT << 16) | PAGE_PLY_FLAT);
		TFRAME *strTitle = strcFindString(vp->strc, shash);
		if (!strTitle){
			strTitle = newStringEx(vp->ml->hw, &metrics, LFRM_BPP_32A, flags, PLAYLISTPLM_LABELS_FONT, buffer, frame->width, NSEX_RIGHT);
			strcAddString(vp->strc, strTitle, shash);
		}
		if (strTitle){
			// playlist name
			int cy = frame->height * spl->cfg.titleY;
			int x = (frame->width - strTitle->width)/2;
			drawShadowedImage(strTitle, frame, x, cy, 0, 1, 2, 1);

			// items in playlist
			__mingw_snprintf(buffer, sizeof(buffer), "%i", playlistGetTotal(plc));
			shash = getHash(buffer) ^ ((PAGE_PLY_SHELF << 16) | PAGE_PLY_FLAT);
			TFRAME *strSubtext = strcFindString(vp->strc, shash);
			if (!strSubtext){
				strSubtext = lNewString(vp->ml->hw, LFRM_BPP_32A, flags, PLAYLISTPLM_LABELS_FONT, buffer);
				strcAddString(vp->strc, strSubtext, shash);
			}
			if (strSubtext){
				cy += frame->height * 0.07;
				drawShadowedImage(strSubtext, frame, abs(frame->width - strSubtext->width)/2, cy, 0, 1, 2, 1);
			}

			// playlistmanager index/id
			__mingw_snprintf(buffer, sizeof(buffer), "#%i",idx+1);
			shash = getHash(buffer) ^ ((PAGE_PLY_SHELF << 16) | PAGE_PLY_FLAT);
			strSubtext = strcFindString(vp->strc, shash);
			if (!strSubtext){
				strSubtext = lNewString(vp->ml->hw, LFRM_BPP_32A, flags, PLAYLISTPLM_LABELS_FONT, buffer);
				strcAddString(vp->strc, strSubtext, shash);
			}
			if (strSubtext){
				cy += frame->height * 0.07;
				drawShadowedImage(strSubtext, frame, abs(frame->width - strSubtext->width)/2, cy, 0, 1, 2, 1);
			}
		}
		outlineTextDisable(vp->ml->hw);
	}
	
	if (spl->from != spl->to || buttonDt < 1000)
		setTargetRate(vp, 25);
	else
		setTargetRate(vp, UPDATERATE_BASE);
	
	spl->from = shelfAnimateNext(shelf);
}

void invalidateShelf (TVLCPLAYER *vp, TSPL *spl, int start)
{
	int lastIdx = playlistManagerGetTotal(vp->plm)-1;
	if (start > lastIdx) start = lastIdx;
	spl->resetTrack = start;
	spl->reset = 1;
}

void drawAlbumPositionText (TVLCPLAYER *vp, TFRAME *frame, double y, const double alb, const int mx, const int my)
{

	const unsigned int *col = swatchGetPage(vp, PAGE_OVERLAY);
	const int colTxt  = col[SWH_OVR_TRKBUBBLE];
	const int colBk = col[SWH_OVR_TRKBUBBLEBK];
	const int colBor = col[SWH_OVR_TRKBUBBLEBOR];
	lSetBackgroundColour(frame->hw, colBk);
	lSetForegroundColour(frame->hw, colTxt);
		
	TFRAME *str = lNewString(frame->hw, LFRM_BPP_32, 0, ALBUM_SLIDER_FONT, " %i ", (int)alb+1);
	if (str){
		int x = mx - (str->width/2)-1;
		y *= frame->height - 18;
		lDrawRectangleFilled(frame, x, y, x+str->width-1, y+str->height-1, colBk);
		drawImage(str, frame, x, y, str->width-1, str->height-1);
						
		// draw a speech box around string
		lSetPixel(frame, x, y, colBor);	// corner points
		lSetPixel(frame, x+str->width-1, y, colBor);
		lSetPixel(frame, x, y+str->height-1, colBor);
		lSetPixel(frame, x+str->width-1, y+str->height-1, colBor);
		
		lDrawLine(frame, x-1, y+1, x-1, y+str->height-2, colBor);	// left
		lDrawLine(frame, x+str->width, y+1, x+str->width, y+str->height-2, colBor);	// right
		lDrawLine(frame, x+1, y-1, x+str->width-2, y-1, colBor);					// top
		
		int mid = (str->width/2);
		lDrawLine(frame, x+1, y+str->height, x+mid-5, y+str->height, colBor);			 // bottom left
		lDrawLine(frame, x+mid+5, y+str->height, x+str->width-2, y+str->height, colBor); // bottom right
		
		lDrawLine(frame, x+mid-4, y+str->height, x+mid, y+str->height+4, colBor);	// arrow left stroke)
		lDrawLine(frame, x+mid+4, y+str->height, x+mid, y+str->height+4, colBor);	// arrow right stroke)
		
		//fill arrow with background colour
		lDrawTriangleFilled(frame, x+mid-3, y+str->height, x+mid+3, y+str->height, x+mid, y+str->height+3, colBk);

		lDeleteFrame(str);
	}
}

int plmTouch (TSPL *spl, TTOUCHCOORD *pos, int flags, TVLCPLAYER *vp)
{
	static unsigned int lastId = 0;
	
	// we don't want drag reports
	//if (pos->dt < 80 || flags) return -2;

	//if (flags == 3) return 0;

	if (!flags){		// pen down
		if (lastId >= pos->id)
			return 0;
		lastId = pos->id;
	}else if (lastId != pos->id){
		return 0;	
	}

	TSHELF *shelf = spl->shelf;


#if 1
	TTOUCHSWIPE *drag = &spl->drag;
	
	//printf("%i %i %i %i %i\n", pos->x, pos->dt, pos->pen, flags, drag->state);

	if (!pos->pen && flags == 0 && pos->dt > 80){			// pen down
		drag->t0 = 0.0;//getTime(vp);
		drag->sx = pos->x;
		drag->sy = pos->y;
		drag->state = 1;
			
		if (shelf->direction == 1)
			spl->to = shelf->to = spl->from + 1;
		else if (shelf->direction == 2)
		spl->to = shelf->to = spl->from - 1;

		shelf->ichanged = 1;
		shelfSetAnimationStepRate(shelf, spl->cfg.rate);
		//invalidateShelf(vp, spl, spl->to);
		
	}else if (!pos->pen && drag->state == 1 && flags == 1){	// dragging
		if (drag->t0 < 1.0)
			drag->t0 = getTime(vp);
		drag->ex = pos->x;
		drag->ey = pos->y;
		drag->dx = drag->ex - drag->sx;
		drag->dy = drag->ey - drag->sy;
		
		if (abs(drag->dx) >= drag->dragMinH)
			return 0;
		
	}else if (pos->pen && drag->state == 1 && flags == 3){	// pen up
		drag->ex = pos->x;
		drag->ey = pos->y;
		drag->dx = drag->ex - drag->sx;
		drag->dy = drag->ey - drag->sy;
		
		if (abs(drag->dx) > drag->dragMinH){
			drag->dt = getTime(vp) - drag->t0;
			drag->state = 0;
			
			double adjustment = drag->dx * (drag->dt/1000.0) * drag->velocityFactor;
			shelf->to -= adjustment;
			
			if (shelf->to < 0) shelf->to = 0;
			if (shelf->to > shelf->clientImageTotal-1) shelf->to = shelf->clientImageTotal-1;
			spl->to = shelf->to;
			//printf("drag %i,%i %i %.1fms: %i %i->%i %i\n", drag->ex, drag->ey, drag->dx, drag->dt, adjustment, shelf->from, shelf->to, shelf->totalImg);

			if (spl->from < shelf->to)
				shelf->direction = 1;
			else if (spl->from > shelf->to)
				shelf->direction = 2;
			shelf->ichanged = 1;
			
			shelfSetAnimationStepRate(shelf, abs(shelf->from - shelf->to)* (spl->cfg.rate/2.00));
			renderSignalUpdate(vp);
			return 0;
		}
	}
#endif
	
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
						invalidateShelf(vp, pageGetPtr(vp, PAGE_PLY_FLAT), simg[idx].imgSrcIndex);
						return 0;
					}
#if 1
					if (!i && simg[idx].imgSrcIndex == shelf->to && simg[idx].imgSrcIndex >= 0){
						vp->plm->pr->selectedItem = simg[idx].imgSrcIndex;
						//vp->playlist.display = vp->plm->pr->selectedItem;
						vp->playlist.display = playlistManagerGetUIDByIndex(vp->plm, vp->plm->pr->selectedItem);
						invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), 0);
						page2Set(vp->pages, PAGE_PLY_SHELF, 1);
						renderSignalUpdate(vp);
						return 0;
					}
#endif
					//printf("%i\n", simg[idx].imgSrcIndex);
					if (simg[idx].imgSrcIndex >= 0 && simg[idx].imgSrcIndex < shelfGetClientImageTotal(shelf)){
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

	if (!flags)
		shelfSetAnimationStepRate(shelf, spl->cfg.rate);
	return 0;
}

static inline int page_plyplmRenderInit (TSPL *spl, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	if (vp->gui.shelfNoArtId){
		spl->noartId = vp->gui.shelfNoArtId;
	}else{
		wchar_t buffer[MAX_PATH+1];
		spl->noartId = artManagerImageAdd(vp->am, buildSkinD(vp,buffer,L"shelf/noart.png"));
		vp->gui.shelfNoArtId = spl->noartId;
	}

	spl->shelf->am = vp->am;
	spl->shelf->imgNoArtId = spl->noartId;


	// ensure root playlist has artwork
	const int total = playlistGetTotal(getPrimaryPlaylist(vp));
	int artcId = spl->noartId;
	for (int i = 0; artcId && artcId == spl->noartId && i < total; i++)
		getPlaylistImageCB(vp, i, &artcId);

	return 1;
}

static inline int page_plyplmRenderBegin (TSPL *spl, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	return 1;
}

static inline int page_plyplmRender (TSPL *spl, TVLCPLAYER *vp, TFRAME *frame)
{
	if (spl->reset){
		spl->reset = 0;
		resetShelfPosition(vp, spl, spl->resetTrack);
	}
	
	const unsigned int dt = buttonsRenderAll(spl->btns, frame, BUTTONS_RENDER_HOVER);
	renderShelf(vp, frame, spl->shelf, spl, dt);
	
	TVIDEOOVERLAY *playctrl = pageGetPtr(vp, PAGE_OVERLAY);
	marqueeDraw(vp, frame, playctrl->marquee, 2, 2);

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

	//printf("%i %i %i\n", spl->shelf->direction, spl->from, spl->to);
#if DRAWTOUCHRECTS
	ccRender(spl->title, frame);
#endif

	if (spl->from == spl->to){
		vp->playlist.display = playlistManagerGetUIDByIndex(vp->plm, spl->from);
		invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), -1);
	}

	return 1;
}

static inline int page_plyplmInput (TSPL *spl, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  case PAGE_IN_TOUCH_DOWN:
		return plmTouch(spl, pos, flags, vp);

	  case PAGE_IN_WHEEL_FORWARD:{
		int next = spl->to - 1;
		if (next < 0) next = 0;
		bringAlbumToFocus(spl, next);
		return 1;
	  }
	  case PAGE_IN_WHEEL_BACK:{
	  	const int total = playlistManagerGetTotal(vp->plm);
	  	if (!total) return 0;

		int next = spl->to + 1;
		if (next >= total) next = total-1;
		bringAlbumToFocus(spl, next);
		return 1;
	  }
	}

	return 1;
}

static inline int page_plyplmStartup (TSPL *spl, TVLCPLAYER *vp, const int fw, const int fh)
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
	spl->drag.dragMinH = fw * 0.05;		// (24 pixels @ 480 width)
	spl->drag.dragMinV = spl->drag.dragMinH;
	spl->drag.velocityFactor = 1.25;

	settingsGet(vp, "plm.shelf.sigma", &spl->cfg.sigma);
	settingsGet(vp, "plm.shelf.rho",  &spl->cfg.rho);
	settingsGet(vp, "plm.shelf.expMult", &spl->cfg.expMult);
	settingsGet(vp, "plm.shelf.rate", &spl->cfg.rate);
	settingsGet(vp, "plm.shelf.spacing", &spl->cfg.spacing);
	settingsGet(vp, "plm.shelf.art.scaleMult", &spl->cfg.artScaleMult);
	settingsGet(vp, "plm.shelf.art.opacityMult", &spl->cfg.artOpacityMult);
	settingsGet(vp, "plm.slider.x", &spl->cfg.sliderX);
	settingsGet(vp, "plm.slider.y", &spl->cfg.sliderY);
	settingsGet(vp, "plm.slider.width", &spl->cfg.sliderW);
	settingsGet(vp, "plm.titleY", &spl->cfg.titleY);
	
		
	spl->shelf = shelfNew(7, spl->cfg.sigma, spl->cfg.spacing, spl->cfg.rate, spl->cfg.rho, spl->cfg.expMult, spl->cfg.artScaleMult, spl->cfg.artOpacityMult);
	shelfSetClientImageLookupCB(spl->shelf, (void*)vp, getPlaylistImageCB);
	shelfSetClientFreeImageCB(spl->shelf, freeImageCB, vp->am);
	shelfSetClientRenderCB(spl->shelf, renderShelfItemCB, (void*)vp);
	

	spl->btns = buttonsCreate(vp->cc, PAGE_PLY_FLAT, SPLBUTTON_TOTAL, ccbtn_cb);

	TCCBUTTON *btn = buttonsCreateButton(spl->btns, L"common/back_right92.png", NULL, SPLBUTTON_BACK, 1, 0, 0, 0);
	ccSetPosition(btn, fw-1 - ccGetWidth(btn), 2);
	
	spl->slider = ccCreate(vp->cc, PAGE_PLY_FLAT, CC_SLIDER_HORIZONTAL, plmCcObject_cb, &vp->gui.ccIds[CCID_SLIDER_PLAYLISTS], 0, 0);
	sliderFaceSet(spl->slider, SLIDER_FACE_LEFT, L"cc/slider_h_thin_left.png");
	sliderFaceSet(spl->slider, SLIDER_FACE_RIGHT, L"cc/slider_h_thin_right.png");
	sliderFaceSet(spl->slider, SLIDER_FACE_MID, L"cc/slider_h_thin_mid.png");
	sliderFaceSet(spl->slider, SLIDER_FACE_TIP, L"cc/slider_h_thin_tip.png");
	sliderFacesApply(spl->slider);
	ccSetMetrics(spl->slider, fw*spl->cfg.sliderX, fh*spl->cfg.sliderY, fw*spl->cfg.sliderW, 16);
	sliderSetRange(spl->slider, 0, 10);
	sliderSetValue(spl->slider, 0);
	ccEnable(spl->slider);

	
	// title change underlay (keypad activate zone)
	spl->title = ccCreate(vp->cc, PAGE_PLY_FLAT, CC_LABEL, plmCcObject_cb, &vp->gui.ccIds[CCID_LABEL_SPLTITLE], 300, 70);
	labelRenderFlagsSet(spl->title, LABEL_RENDER_INV);
	ccSetPosition(spl->title, (fw-300)/2, fh*spl->cfg.titleY);
	ccEnable(spl->title);

	return 1;
}

static inline int page_plyplmInitalize (TSPL *spl, TVLCPLAYER *vp, const int width, const int height)
{
	//printf("page_plyplmInitalize %x %x\n", spl->noartId, vp->gui.shelfNoArtId);

	setPageAccessed(vp, PAGE_PLY_FLAT);
	page2InputDragEnable(vp->pages, PAGE_PLY_FLAT);
	
	if (vp->gui.shelfNoArtId){
		spl->noartId = vp->gui.shelfNoArtId;
		
	}else{
		wchar_t buffer[MAX_PATH+1];
		spl->noartId = artManagerImageAdd(vp->am, buildSkinD(vp,buffer,L"shelf/noart.png"));
		vp->gui.shelfNoArtId = spl->noartId;
	}

	return 1;
}

static inline int page_plyplmShutdown (TSPL *spl, TVLCPLAYER *vp)
{
	ccDelete(spl->slider);
	ccDelete(spl->title);
	shelfDelete(spl->shelf);
	buttonsDeleteAll(spl->btns);
	return 1;
}

int page_plyPlmCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TSPL *spl = (TSPL*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_plyPlmCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_plyplmRender(spl, spl->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		return page_plyplmRenderBegin(spl, spl->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);

	}else if (msg == PAGE_CTL_RENDER_END){
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_plyplmInput(spl, spl->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_plyplmRenderInit(spl, spl->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_plyplmStartup(spl, spl->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_plyplmInitalize(spl, spl->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_plyplmShutdown(spl, spl->com->vp);
		
	}
	
	return 1;
}


