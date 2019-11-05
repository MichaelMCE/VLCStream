
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


extern int SHUTDOWN;





static inline int overlayPlaylistListboxFill (TVIDEOOVERLAY *plyctrl, PLAYLISTCACHE *plc);









static inline void computeEquationParts (TTEXTSCALE *scale)
{
	scale->expMultiplier = sqrt(2.0 * M_PI) / scale->sigma / scale->rho;
	scale->expMember = 4.0 * scale->sigma * scale->sigma;
}

static inline double computeModifierUnprotected (TTEXTSCALE *scale, const double x)
{
	return scale->expMultiplier * exp((-x * x) / scale->expMember);
}

static inline double computeModifier (TTEXTSCALE *scale, const double x)
{
	double result = computeModifierUnprotected(scale, x);
	if (result > 1.0)
		result = 1.0;
	else if (result < -1.0)
		result = -1.0;
	return result;
}

static inline void setSigma (TTEXTSCALE *scale, const double sigma)
{
	scale->sigma = sigma;
	scale->rho = 1.1;
	computeEquationParts(scale);
	scale->rho = computeModifierUnprotected(scale, 0.0);
	computeEquationParts(scale);
}

void ctrlPlayback (TVLCPLAYER *vp, const int func)
{

	switch (func){
	  case VBUTTON_PLAY:
	  	//printf("getPlayState(vp) %i\n", getPlayState(vp));

		if (getPlayState(vp) == 2){ 		// is paused so unpause
			trackPlay(vp);
			return;

	  	}else if (getPlayState(vp) != 1){
	  		if (getQueuedPlaylistUID(vp) <= PLAYLIST_UID_BASE){
	  			PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
				if (plc){
					if (playlistGetCount(plc, PLAYLIST_OBJTYPE_TRACK))
						//vp->queuedPlaylist = vp->displayPlaylist;
						setQueuedPlaylist(vp, plc);
					else
						return;
				}else{
					return;
				}
			}

			// load a media item if none already
			if (!vp->vlc->m || !vp->vlc->mp){
				PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
				if (plc){
					int track = plc->pr->playingItem;
					if (track < 0){
						track = plc->pr->selectedItem;
						if (track < 0)
							track = plc->pr->selectedItem = 0;
					}
					startPlaylistTrack(vp, plc, track);
				}
				return;
			}
			trackPlay(vp);
		}
		//break;
		ALLOW_FALLTHROUGH;
	  case VBUTTON_PAUSE:
	  	trackPlayPause(vp);
		break;

	  case VBUTTON_STOP:
		trackStop(vp);
		break;

	  case VBUTTON_PRETRACK:
	  	trackPrev(vp);
	  	break;

	  case VBUTTON_NEXTTRACK:
	  	trackNext(vp);
	  	break;
	};
}

#if 0
static inline void drawButtonPanel (TVLCPLAYER *vp, TFRAME *frame, const int x, const int y, const int width, const int height)
{
	const unsigned int col = swatchGetColour(vp, PAGE_OVERLAY, SWH_OVR_PANELEDGE);

	// top
	lDrawLine(frame, x+2, y, x+width-3, y, col);

	// left
	lDrawLine(frame, x, y+2, x, y+height-1, col);
	lDrawLine(frame, x+1, y+1, x+7, y+7, col);
	lDrawLine(frame, x+1, y+2, x+2, y+1, (col&0xFF000000)|COL_BLUE_SEA_TINT);

	// right
	lDrawLine(frame, x+width-1, y+2, x+width-1, y+height-1, col);
	lDrawLine(frame, x+width-2, y+1, x+width-8, y+7, col);
	lDrawLine(frame, x+width-2, y+2, x+width-3, y+1, (col&0xFF000000)|COL_BLUE_SEA_TINT);

#if 0
	lDrawRectangleFilled(frame, x, y, x+width-1, y+height-1, 0xFF000000);
#endif
}
#endif

static inline int64_t labelCcObject_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;

	//if (msg == CC_MSG_RENDER || obj->type != CC_BUTTON) return 1;
	//printf("labelCcObject_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);

	if (msg == CC_MSG_RENDER){
		/*if (obj->type == CC_LABEL){	// draw an outline around label before the blur is applied
			TLABEL *label = (TLABEL*)obj;
			drawButtonPanel(label->cc->vp, dataPtr, label->metrics.x+1, label->metrics.y+1, label->metrics.width-3, label->metrics.height-3);
		}*/
		return 1;
	}

	if (msg == BUTTON_MSG_SELECTED_PRESS){
		TCCBUTTON *btn = (TCCBUTTON*)obj;
		if (!ccGetState(ccGetUserData(btn)))	// is label disabled
			return 1;
		else
			return plyctrlButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	}
	return 1;
}

static inline int64_t ctrlCcObject_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	if (msg == CC_MSG_RENDER) return 1;


	//if (obj->type == CC_SLIDER_HORIZONTAL){
		TSLIDER *slider = (TSLIDER*)obj;
		TVLCPLAYER *vp = (TVLCPLAYER*)obj->cc->vp;

		//if (slider->id == ccGetId(vp, CCID_SLIDER_TRACKPOSITION)){
			switch (msg){
			  case SLIDER_MSG_VALSET:
		  	  case SLIDER_MSG_VALCHANGED:
		  		if (getPlayState(vp) && getPlayState(vp) /*!= 8*/){
					vp->vlc->position = sliderGetValueFloat(slider);
					timerSet(vp, TIMER_CTRL_UPDATETIMESTAMP, 1);
					vlc_setPosition(vp->vlc, vp->vlc->position);
					overlayActivateOverlayResetTimer(vp);
				}
				break;
			}
		//}
	//}
	return 1;
}

static inline int volumeRenderStringGetWidth (TVIDEOOVERLAY *plyctrl, TFRAME **digit, const char *str)
{
	int w = 0;

	while (*str){
		if (*str >= '0' && *str <= '9'){
			int chr = *str - '0';
			w += (digit[chr]->width - plyctrl->volume.charOverlap);
		}
		str++;
	}
	return w;
}

static inline int volumeRenderStringGetHeight (TFRAME **digit)
{
	int h = 100;
	if (digit[0]) h = digit[0]->height;
	return h;
}

static inline void volumeRenderString (TVIDEOOVERLAY *plyctrl, TFRAME **digit, TFRAME *frame, int x, int y, const char *str)
{
	while (*str){
		if (*str >= '0' && *str <= '9'){
			int chr = *str - '0';
			TFRAME *d = digit[chr];
			//if (x+d->width < frame->width)
				drawImage(d, frame, x, y, d->width-1, d->height-1);

#if DRAWBUTTONRECTS
			lDrawRectangle(frame, x, y, x+d->width-1, y+d->height-1, DRAWTOUCHRECTCOL);
#endif
			x += (d->width - plyctrl->volume.charOverlap);
		}
		str++;
	}
}

static inline void volumeRender (TVIDEOOVERLAY *plyctrl, TFRAME *frame, const int volume)
{
	for (int i = 0; i < 10; i++){
		plyctrl->volume.images[i] = imageManagerImageAcquire(plyctrl->com->vp->im, plyctrl->volume.imageIds[i]);
		//printf("imageManagerImageAcquire %i\n", plyctrl->volume.imageIds[i]);
	}

	char buffer[8];
	__mingw_snprintf(buffer, sizeof(buffer), "%i", volume);

	int w = volumeRenderStringGetWidth(plyctrl, plyctrl->volume.images, buffer) + plyctrl->volume.charOverlap;
	int x = abs(frame->width - w)/2;
	if (x < 0) x = 0;
	int y = ((frame->height - volumeRenderStringGetHeight(plyctrl->volume.images))/2)-plyctrl->volume.verticalOffset;
	//printf("clockRenderDigitStringDGetWidth %i %i\n", w, x);

	volumeRenderString(plyctrl, plyctrl->volume.images, frame, x, y, buffer);

	for (int i = 0; i < 10; i++){
		imageManagerImageRelease(plyctrl->com->vp->im, plyctrl->volume.imageIds[i]);
		//printf("imageManagerImageRelease %i\n", plyctrl->volume.imageIds[i]);
	}
}

void setVolumeDisplay (TVLCPLAYER *vp, const int volume)
{
	TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
	plyctrl->volume.val = volume;
	plyctrl->volume.doRender = 1;
	timerSet(vp, TIMER_CTRL_DISPLAYVOLRESET, 500);
}

int plyctrlButtonPress (TVIDEOOVERLAY *plyctrl, TCCBUTTON *btn, const int btnId, const TTOUCHCOORD *pos)
{
	TVLCPLAYER *vp = plyctrl->com->vp;
	overlayActivateOverlayResetTimer(vp);
	plyctrl->btns->t0 = getTickCount();


	switch (btnId){
	  case VBUTTON_PRETRACK:
	  case VBUTTON_PLAY:
	  case VBUTTON_PAUSE:
	  case VBUTTON_STOP:
	  case VBUTTON_NEXTTRACK:
		ctrlPlayback(vp, btnId);
		break;

	  case VBUTTON_HOME:
	  	//pageSet(vp, PAGE_HOME);
	  	page2Set(vp->pages, PAGE_HOME, 1);
	  	break;

	  case VBUTTON_EPG_PROGRAMMES:{
	  	TEPG *epg = pageGetPtr(vp, PAGE_EPG);
	  	page2Set(vp->pages, PAGE_EPG, 1);
	  	ccEnable(epg->guide.paneChannels);
		ccEnable(epg->guide.paneContents);
	  	ccDisable(epg->programme.listbox);
		break;
	  }
	  case VBUTTON_TIMESTMP:{
	  	//printf("VBUTTON_TIMESTMP: %i %i %i\n", pos->id, pos->pen, pos->dt);


	  	if (!ccGetState(plyctrl->listbox.lb))
	  		ccEnable(plyctrl->listbox.lb);
	  	else
	  		ccDisable(plyctrl->listbox.lb);
	    break;
	  }

	  case VBUTTON_MHOOK:
		if (!pos->pen){
			TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
	  		buttonsStateSet(plyctrl->btns, VBUTTON_MHOOK, 0);

			if (mHookGetState()){
				captureMouse(vp, 0);
				mHookUninstall();
			}
		}
	  	break;

	  case VBUTTON_CHAPPREV:
	  	vlc_previousChapter(vp->vlc);
	  	break;

	  case VBUTTON_CHAPNEXT:
	  	vlc_nextChapter(vp->vlc);
	  	break;

	  case VBUTTON_VOLUMEMODE:
	  	if (buttonFaceActiveGet(btn) == BUTTON_PRI){
	  		buttonFaceActiveSet(btn, BUTTON_SEC);

	  		buttonFaceActiveSet(buttonsButtonGet(plyctrl->btns, VBUTTON_VOLUME), BUTTON_SEC);
			buttonFaceActiveSet(buttonsButtonGet(plyctrl->btns, VBUTTON_VOLUMEBASE), BUTTON_SEC);
	  	}else{
	  		buttonFaceActiveSet(btn, BUTTON_PRI);

	  		buttonFaceActiveSet(buttonsButtonGet(plyctrl->btns, VBUTTON_VOLUME), BUTTON_PRI);
			buttonFaceActiveSet(buttonsButtonGet(plyctrl->btns, VBUTTON_VOLUMEBASE), BUTTON_PRI);
		}
		break;

	  case VBUTTON_VOLUMEBASE:
	  case VBUTTON_VOLUME:{
  		#define BORDERTHICKNESS 1.0
		double bheight = ccGetHeight(btn);
		//calculate the volume based from icon metrics and touch position
		const int volume = 100.0 - ((100.0/(bheight - BORDERTHICKNESS)) * (double)(pos->y - BORDERTHICKNESS));
#if 1
		if (buttonFaceActiveGet(btn) == BUTTON_PRI)
			setVolumeDisplay(vp, setVolume(vp, volume, VOLUME_APP));
		else
			setVolumeDisplay(vp, setVolume(vp, volume, VOLUME_MASTER));
#else
		if (buttonFaceActiveGet(btn) == BUTTON_PRI)
			vp->gui.tmpVolumeCtrl = VOLUME_APP;
		else
			vp->gui.tmpVolumeCtrl = VOLUME_MASTER;

		vp->gui.tmpVolume = setVolume(vp, volume, vp->gui.tmpVolumeCtrl);
		timerSet(vp, TIMER_testingonly, 0);
#endif
		break;
	  }
	}
	return 0;
}

static inline int plyctrlButtonSlide (TVIDEOOVERLAY *plyctrl, TCCBUTTON *btn, const int btnId, const TTOUCHCOORD *pos)
{
	TVLCPLAYER *vp = btn->cc->vp;
	overlayActivateOverlayResetTimer(vp);
	plyctrl->btns->t0 = getTickCount();

	switch (btnId){
	  case VBUTTON_VOLUMEBASE:
	  case VBUTTON_VOLUME:{
	  	//if (flags != 0x03){			// don't fire on touch up: down and drag only
	  		#define BORDERTHICKNESS 1.0
			double bheight = ccGetHeight(btn);

			//calculate the volume based from icon metrics and touch position
			int volume = 100.0 - ((100.0/(bheight - BORDERTHICKNESS)) * (double)(pos->y - BORDERTHICKNESS));
#if 1
			if (buttonFaceActiveGet(btn) == BUTTON_PRI)
				setVolumeDisplay(vp, setVolume(vp, volume, VOLUME_APP));
			else
				setVolumeDisplay(vp, setVolume(vp, volume, VOLUME_MASTER));
#else
			if (buttonFaceActiveGet(btn) == BUTTON_PRI)
				vp->gui.tmpVolumeCtrl = VOLUME_APP;
			else
				vp->gui.tmpVolumeCtrl = VOLUME_MASTER;

			vp->gui.tmpVolume = setVolume(vp, volume, vp->gui.tmpVolumeCtrl);
			timerSet(vp, TIMER_testingonly, 5);
#endif
			break;
		//}
		}
	}
	return 0;
}


static inline void drawTrackChapterMarks (TVLCPLAYER *vp, TFRAME *frame, TCHAPTER *chapt, TSLIDER *slider)
{

	TLPOINTEX rt;
	sliderGetSliderRect(slider, &rt);

	const double w = (rt.x2 - rt.x1) + 1;
	int playingPosX2 = rt.x2;
	int playingPosX1 = 0;
	int playingPos = 0;
	const int posy = rt.y1+2;

	TVLCCONFIG *vlc = getConfig(vp);
	TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
	const int markOffset = 1;//plyctrl->chapterMark->width>>1;
	uint64_t tOffset, cs;


	if (chaptersLock(chapt)){
		for (int i = -1; i < chapt->tchapters; i++){
			if (i == -1){
				tOffset = 0;
				cs = 0;
			}else{
				tOffset = chaptersGetChapterTimeOffset(chapt, chapt->ctitle-1, i);
				if ((int64_t)tOffset == (int64_t)-1) continue;
				cs = tOffset/1000/1000;
			}

			int posx = rt.x1 + (w / ((double)vlc->length) * (double)cs);
#if DRAWMISCDETAIL
			lDrawLine(frame, posx, posy-20, posx, posy+40, 0xFFFF0000); /* ideal placement */
#endif
			drawImage(plyctrl->chapterMark, frame, posx-markOffset, posy, plyctrl->chapterMark->width-1, plyctrl->chapterMark->height-1);

			if (playingPos != 2){
				if ((double)vlc->length*(double)vlc->position > cs){
					playingPos = 1;
					playingPosX1 = posx;

					// vlc v1.1.11 doesn't seem to dispatch an event to signal a chapter change
					// so hack something manually
					chapt->cchapter = i;
				}else if (playingPos == 1){
					playingPos = 2;
					playingPosX2 = posx-1;
				}
			}
		}
		chaptersUnlock(chapt);
	}

	if (playingPosX1 && playingPosX2 > playingPosX1){
		for (int x = playingPosX1; x <= playingPosX2; x++)
			drawImage(plyctrl->chapterMarkFilled, frame, x, posy, plyctrl->chapterMarkFilled->width-1, plyctrl->chapterMarkFilled->height-1);
	}
}

static inline void drawButtonsVolume (TVLCPLAYER *vp, TCCBUTTONS *btns, TVIDEOOVERLAY *playctrl, TFRAME *frame)
{

	if (buttonsStateGet(btns, VBUTTON_VOLUME)){
		TCCBUTTON *bvol = buttonsButtonGet(btns, VBUTTON_VOLUME);

		//TFRAME *frm = labelImageGetSrc(bvol->active->label, bvol->active->itemId);
		int imgId = labelImgcGetSrc(bvol->active->label, bvol->active->itemId);
		TFRAME *img = imageManagerImageAcquire(vp->im, imgId);


		TVLCCONFIG *vlc = getConfig(vp);
		double volume;
		if (buttonFaceActiveGet(buttonsButtonGet(btns, VBUTTON_VOLUME)) == BUTTON_PRI)
			volume = vlc->volume;
		else
			volume = getVolume(vp, VOLUME_MASTER);

		if (volume > 0.0001){ // is not muted
			const double volFactor = (img->height-4.0)/100.0;
			drawVolume(img, frame, ccGetPositionX(bvol), ccGetPositionY(bvol), \
	  			0, (100.0 - volume)*volFactor, img->width, img->height);
		}
		imageManagerImageRelease(vp->im, imgId);
	}
}

static inline void drawTrackPositionText (TVIDEOOVERLAY *plyctrl, TVLCPLAYER *vp, TFRAME *frame, TGUI *gui)
{

	const int mx = gui->cursor.dx;
	const int my = gui->cursor.dy;
	const int minx = 2;

	TMETRICS metrics;
	metrics.x = 6;
	metrics.y = 0;
	metrics.width = 0;
	metrics.height = 0;
	TLPOINTEX rt;

	TSLIDER *slider = plyctrl->trackSlider;
	sliderGetSliderRect(slider, &rt);

	if (mx >= rt.x1 && mx <= rt.x2){
		if (my >= rt.y1 && my <= rt.y2){
			double w = (rt.x2 - rt.x1)+1;
			double position = (1.0/w) * ((double)(mx - rt.x1));

			if (position >= 0.0 && position <= 1.0){
				char title[MAX_PATH_UTF8+1];
				double timeX = (double)vp->vlc->length * position;
				char *chapname = getChapterNameByTime(vp, timeX);
				if (!chapname){
					chapname = getPlayingProgramme(vp);
					if (!chapname)
						chapname = getPlayingTitle(vp);
				}
				if (chapname){
					strcpy(title, chapname);
					my_free(chapname);
				}else{
					strcpy(title, " ");
				}

				char buffer[64];
				timeToString(timeX, buffer, sizeof(buffer)-1);
				if (*buffer){
					lSetBackgroundColour(frame->hw, 0);
					setForegroundColourIdx(vp, PAGE_OVERLAY, SWH_OVR_TRKBUBBLE);
					const int colBk = swatchGetColour(vp, PAGE_OVERLAY, SWH_OVR_TRKBUBBLEBK);
					const int colBor = swatchGetColour(vp, PAGE_OVERLAY, SWH_OVR_TRKBUBBLEBOR);

					lSetRenderEffect(frame->hw, LTR_SHADOW);
					lSetFilterAttribute(frame->hw, LTR_SHADOW, 0, LTRA_SHADOW_N|LTRA_SHADOW_S|LTRA_SHADOW_E|LTRA_SHADOW_W | LTRA_SHADOW_S5 | LTRA_SHADOW_OS(0) | LTRA_SHADOW_TR(90));
					lSetFilterAttribute(frame->hw, LTR_SHADOW, 1, /*0xFFFFFF*/0);

					TFRAME *cname = newStringEx(frame->hw, &metrics, LFRM_BPP_32, 0, CTRLOVR_SLIDER_FONT, title, frame->width-(minx<<1), NSEX_RIGHT);
					TFRAME *ttime = lNewString(frame->hw, LFRM_BPP_32, 0, CTRLOVR_SLIDER_FONT, " %s ", buffer);
					lSetRenderEffect(frame->hw, LTR_DEFAULT);

					if (ttime){
						int tx = (mx-MOFFSETX) - (ttime->width/2) + minx;
						int ty = rt.y1 - ttime->height - 3;

						int x, bx;
						int w = MAX(ttime->width, cname->width);
						x = (cname->width - ttime->width)/2;
						if (x >= 0){
							x = tx - abs(x);
							x = MIN(x, frame->width-w-minx);
							if (x < minx) x = minx;
							bx = x;
						}else{
							x = tx + abs(x);
							x = MIN(x, frame->width-w-minx);
							if (x < minx) x = minx;
							bx = tx;
						}

						int h = (ttime->height + cname->height) - 2;
						int y = (ty - cname->height) + 2;

						lDrawRectangleFilled(frame, bx, y, bx+w-1, y+h-1, colBk);
						drawImage(cname, frame, x, y, cname->width-1, cname->height-1);
						drawImage(ttime, frame, tx, ty-1, ttime->width-1, ttime->height-1);

						// draw speech box around time
						lSetPixel(frame, bx, y, colBor);					// corner points
						lSetPixel(frame, bx+w-1, y, colBor);
						lSetPixel(frame, bx, y+h-1, colBor);
						lSetPixel(frame, bx+w-1, y+h-1, colBor);

						lDrawLine(frame, bx-1, y+1, bx-1, y+h-2, colBor);	// left
						lDrawLine(frame, bx+w, y+1, bx+w, y+h-2, colBor);	// right
						lDrawLine(frame, bx+1, y-1, bx+w-2, y-1, colBor);	// top

						int mid = gui->cursor.dx-1;
						lDrawLine(frame, bx+1, y+h, mid-5, y+h, colBor);	// bottom left
						lDrawLine(frame, mid+5, y+h, bx+w-2, y+h, colBor); 	// bottom right

						lDrawLine(frame, mid-4, y+h, mid, y+h+4, colBor);	// arrow, left stroke
						lDrawLine(frame, mid+4, y+h, mid, y+h+4, colBor);	// arrow, right stroke

						//fill arrow with background colour
						lDrawTriangleFilled(frame, mid-3, y+h, mid+3, y+h, mid, y+h+3, colBk);

						lDeleteFrame(ttime);
						lDeleteFrame(cname);
					}
				}
			}
		}
	}
}

static inline int ctrlPanSetImage (TLABEL *label, wchar_t *pri, const int udata, const int canAnimate, const int x, const int y)
{
	wchar_t buffer[MAX_PATH+1];

	TCCBUTTON2 *btn = ccCreate(label->cc, label->pageOwner, CC_BUTTON2, labelCcObject_cb, NULL, 0, 0);
	ccSetUserDataInt(btn, udata);
	ccSetUserData(btn, label);
	button2FaceImgcSet(btn, artManagerImageAdd(btn->cc->vp->am, buildSkinD(label->cc->vp, buffer, pri)), 0, 0.0, 0, 0);
	button2FaceHoverSet(btn, 1, COL_HOVER, 0.8);
	button2AnimateSet(btn, canAnimate);
	btn->isChild = 1;

	int itemId = labelCCCreate(label, btn, x, y);
	ccEnable(btn);

	return itemId;
}

int ctrlPanGetEnabledTotal (TBTNPANEL *btnpan, const int total)
{
	int ct = 0;
	for (int i = 0; i < total; i++){
		if (btnpan->itemIds[i])
			ct += labelItemGetEnabledStatus(btnpan->base, btnpan->itemIds[i]);
	}
	return ct;
}

void ctrlPanCalcPositions (TBTNPANEL *btnpan, const int set)
{
	if (!btnpan->base->cc->vp->gui.drawControlIcons) return;

	// enable everything, disable what we don't need then center justify whats left

	for (int i = 0; i < VBUTTON_TOTAL; i++){
		if (btnpan->itemIds[i])
			labelItemEnable(btnpan->base, btnpan->itemIds[i]);
	}

	if (set == BTNPANEL_SET_PLAY){	// is playing so disable play button
		labelItemDisable(btnpan->base, btnpan->itemIds[VBUTTON_PLAY]);
	}else if (set == BTNPANEL_SET_PAUSE){	// is playing so disable pause button
		labelItemDisable(btnpan->base, btnpan->itemIds[VBUTTON_PAUSE]);
	}else if (set == BTNPANEL_SET_STOP){	// is stopped so disable botth pause and stop buttons
		labelItemDisable(btnpan->base, btnpan->itemIds[VBUTTON_PAUSE]);
		labelItemDisable(btnpan->base, btnpan->itemIds[VBUTTON_STOP]);
	}

	const int fw = getFrontBuffer(btnpan->base->cc->vp)->width;
	int btotal = ctrlPanGetEnabledTotal(btnpan, VBUTTON_TOTAL);

	const int x_gap =  btnpan->bwidth / 2;
	btnpan->width = ((btotal+1)*x_gap) + (btotal*btnpan->bwidth);
	ccSetMetrics(btnpan->base, -1, -1, btnpan->width, -1);
	ccSetPosition(btnpan->base, abs(fw - ccGetWidth(btnpan->base))/2, ccGetPositionY(btnpan->base));

	int x = x_gap + 1;
	const int y = ((btnpan->height - btnpan->bheight)/2) + 2;

	for (int i = 0; i < VBUTTON_TOTAL; i++){
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

	if (ccGetUserDataInt(btn) == VBUTTON_TOUCHPAD){
		//printf("VBUTTON_TOUCHPAD %i\n",msg);

		if (msg == BUTTON_MSG_SELECTED_PRESS){
			//printf("BUTTON_MSG_SELECTED_PRESS\n");

			TTOUCHCOORD *pos = (TTOUCHCOORD*)dataPtr;
			if (!pos) return 1;

			TPADSIPE *swipe = ccGetUserData(btn);
			swipe->loc.x1 = pos->x;
			swipe->loc.y1 = pos->y;
			swipe->state = 1;
			overlayActivateOverlayResetTimer(btn->cc->vp);

		}else if (msg == BUTTON_MSG_SELECTED_RELEASE){
			//printf("BUTTON_MSG_SELECTED_RELEASE\n");

			TTOUCHCOORD *pos = (TTOUCHCOORD*)dataPtr;
			if (!pos) return 1;

			TPADSIPE *swipe = ccGetUserData(btn);
			if (swipe->state == 1){
				swipe->loc.x2 = pos->x;
				swipe->loc.y2 = pos->y;

				#define DEADZONE (20)
				const int dx = swipe->loc.x2 - swipe->loc.x1;
				const int dy = swipe->loc.y2 - swipe->loc.y1;

				int dir = 0;
				if (abs(dx) > abs(dy))
					dir = dx;
				else
					dir = dy;
				//printf("dir %i, %i %i\n", dir, dx, dy);

				if (dir > DEADZONE){
					timerSet(btn->cc->vp, TIMER_PREVTRACK, 0);
				}else if (dir < -DEADZONE){
					timerSet(btn->cc->vp, TIMER_NEXTTRACK, 0);
				}else{
					if (!getPlayState(btn->cc->vp))
						timerSet(btn->cc->vp, TIMER_PLAY, 0);
					else
						timerSet(btn->cc->vp, TIMER_PLAYPAUSE, 0);
				}
			}
			swipe->state = 0;
			overlayActivateOverlayResetTimer(btn->cc->vp);

		}else if (msg == CC_MSG_DELETE){
			TPADSIPE *swipe = ccGetUserData(btn);
			if (swipe) my_free(swipe);
		}
	}else if (msg == BUTTON_MSG_SELECTED_PRESS){
		return plyctrlButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	}else if (msg == BUTTON_MSG_SELECTED_SLIDE){
		return plyctrlButtonSlide(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	}

	return 1;
}


// TODO: UPDATE for new listbox
void ctrlLbMetaCb (TVLCPLAYER *vp, const int itemId, const int uid, const int track, void *dataPtr1, void *dataPt2)
{
	//printf("metalbcb: %i, %X %i\n", itemId, uid, track);

#if 1
	if (SHUTDOWN) return;

	char buffer[MAX_PATH_UTF8+1];

	tagRetrieveByHash(vp->tagc, (unsigned int)(intptr_t)dataPt2, MTAG_Title, buffer, MAX_PATH_UTF8);
	if (!*buffer){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
		if (!plc) return;

		playlistGetTitle(plc, track, buffer, MAX_PATH_UTF8);
		if (!*buffer)
			playlistGetPath(plc, track, buffer, MAX_PATH_UTF8);
	}

	if (*buffer){
		TLISTBOX *listbox = dataPtr1;
		listboxUpdateItem(listbox, itemId, buffer);
		//listboxSetFocus(listbox, listboxGetFocus(listbox));
	}
#endif
}

static inline int _fillPlaylistListbox (TVIDEOOVERLAY *plyctrl, TLISTBOX *listbox, PLAYLISTCACHE *plc)
{
	if (!plc) return 0;

	TVLCPLAYER *vp = plyctrl->com->vp;


	const int highlightedItem = plc->pr->playingItem;
	const int uid = playlistManagerGetPlaylistUID(vp->plm, plc);
	const int plmIdx = (uid - PLAYLIST_UID_BASE)<<18;
	TMETACOMPLETIONCB mcb = {0};
	int ct = 0;


	int focus;
	if (plyctrl->lbUID != plc->uid)
		focus = plc->pr->lbFocus;
	else
		focus = listboxGetFocus(listbox);
	listboxRemoveAll(listbox);

 	int artId = playlistManagerGetPlaylistArtIdByUID(vp->plm, uid);
	if (!artId){
		getItemPlaylistImage(vp, plc, &artId);
		if (!artId){
			initiateAlbumArtRetrieval(vp, plc, 0, 12, vp->gui.artSearchDepth);
			playlistMetaGetMeta(vp, plc, 0, 16, NULL);
		}
	}
	listboxSetUnderlsy(listbox, artId, 0.80, 90, 0, 20, SHADOW_BLACK);


	if (plc->parent){
		int parent = (playlistManagerGetPlaylistUID(vp->plm, plc->parent) - PLAYLIST_UID_BASE)<<18;
		listboxAddItem(listbox, "    . .    ", 0, CTRL_TRACK_COLOUR, parent|0x3FFFF);
	}

	const int total = playlistGetTotal(plc);
	if (!total) return 0;

	int highlightedItemId = 0;
	char buffer[MAX_PATH_UTF8+1];
	const int uidQ = getQueuedPlaylistUID(vp);

	for (int i = 0; i < total; i++){
		TPLAYLISTITEM *item = playlistGetItem(plc, i);
		if (!item) break;

		if (item->objType == PLAYLIST_OBJTYPE_PLC){
			int child = (playlistManagerGetPlaylistUID(vp->plm, item->obj.plc)-PLAYLIST_UID_BASE)<<18;
			unsigned int colour;
			if (item->obj.plc->uid == uidQ)
				colour = CTRL_HIGHLIGHT_COLOUR;
			else
				colour = CTRL_PLAYLIST_COLOUR;
			ct += listboxAddItem(listbox, item->obj.plc->title, 0, colour, child|0x3FFFF) > 0;

		}else if (item->objType == PLAYLIST_OBJTYPE_TRACK){
			//tagRetrieveByHash(vp->tagc, item->obj.track.hash, MTAG_Title, buffer, MAX_PATH_UTF8);

			if (item->obj.track.title && item->obj.track.title[0]){
				mcb.itemId = listboxAddItem(listbox, item->obj.track.title, 0/*item->obj.track.artId*/, CTRL_TRACK_COLOUR, plmIdx|(i&0x3FFFF));
			}else{
				playlistGetPath(plc, i, buffer, MAX_PATH_UTF8);
				mcb.itemId = listboxAddItem(listbox, buffer, 0, CTRL_TRACK_COLOUR, plmIdx|(i&0x3FFFF));
			}
			if (!mcb.itemId) continue;
			ct += mcb.itemId > 0;

			if (i == highlightedItem /*|| (highlightedItem == -1 && i == plc->pr->selectedItem)*/)
				highlightedItemId = mcb.itemId;

			item->metaRetryCt = 0;
			mcb.cb = ctrlLbMetaCb;
			mcb.dataInt1 = uid;
			mcb.dataInt2 = i;
			mcb.dataPtr1 = listbox;
			mcb.dataPtr2 = (void*)(intptr_t)item->obj.track.hash;
			playlistMetaGetTrackMeta(vp, plc, item->obj.track.path, i, &mcb);
		}
	}

	listboxSetHighlightedItem(listbox, highlightedItemId);
	plc->pr->lbFocus = focus;
	listboxSetFocus(listbox, focus);
	return ct;
}

static inline int overlayPlaylistListboxFill (TVIDEOOVERLAY *plyctrl, PLAYLISTCACHE *plc)
{
	int ret = 0;
	if (ccLock(plyctrl->listbox.lb)){
		ret = _fillPlaylistListbox(plyctrl, plyctrl->listbox.lb, plc);
		if (ret > 0)
			plyctrl->lbUID = plc->uid;
		ccUnlock(plyctrl->listbox.lb);
	}

	return ret;
}

static inline void timeStampSetTime (TVLCPLAYER *vp, const double position)
{
	char str[132];
	char buffer[2][64];

	timeToString(position*(double)vp->vlc->length, buffer[0], sizeof(buffer[0])-1);
	timeToString(vp->vlc->length, buffer[1], sizeof(buffer[1])-1);


	if (*buffer[0] && *buffer[1])
		__mingw_snprintf(str, sizeof(str)-1, "%s/%s", buffer[0], buffer[1]);
	else if (*buffer[0])
		__mingw_snprintf(str, sizeof(str)-1, "%s/0:00", buffer[0]);
	else if (*buffer[1])
		__mingw_snprintf(str, sizeof(str)-1, "0:00/%s", buffer[1]);
	else
		strncpy(str, "0:00/0:00", sizeof(str)-1);

	TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
	TCCBUTTON *btn = buttonsButtonGet(plyctrl->btns, VBUTTON_TIMESTMP);
	buttonFaceTextUpdate(btn, BUTTON_PRI, str);
	buttonFaceTextUpdate(btn, BUTTON_SEC, str);
	//printf("timeStampSetTime: str '%s', '%s', '%s'\n", str, buffer[0], buffer[1]);
}

void overlayActivateOverlayResetTimer (TVLCPLAYER *vp)
{
	timerReset(vp, TIMER_CTRL_OVERLAYRESET);
	timerSet(vp, TIMER_CTRL_OVERLAYRESET, vp->gui.mOvrTime);
}

void overlaySetOverlay (TVLCPLAYER *vp)
{
	//printf("overlaySetOverlay\n");
	if (SHUTDOWN) return;

	overlayActivateOverlayResetTimer(vp);
	page2Set(vp->pages, PAGE_OVERLAY, 0);
}

static inline void _overlaySetOverlay (void *pageStruct)
{
	TVIDEOOVERLAY *plyctrl = (TVIDEOOVERLAY*)pageStruct;
	overlaySetOverlay(plyctrl->com->vp);
}


// TIMER_CTRL_OVERLAYRESET
void overlayResetOverlay (TVLCPLAYER *vp)
{
	//printf("@@@ TIMER_CTRL_OVERLAYRESET\n");

	if (vp->applState){
		if ((page2RenderGetState(vp->pages, PAGE_OVERLAY) || pageGet(vp) == PAGE_OVERLAY) && !kHookGetState() /*&& pageGetSec(vp) != PAGE_CHAPTERS*/){

			TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
			if (!ccGetState(plyctrl->listbox.lb)){
				page2Set(vp->pages, PAGE_NONE, 1);
				page2RenderDisable(vp->pages, PAGE_OVERLAY);

				vp->gui.frameCt = 0;
				renderSignalUpdate(vp);
			}
		}
	}
}

// TIMER_CTRL_PLAYLISTLBREFRESH
void timer_playlistListboxRefresh (TVLCPLAYER *vp)
{

	//printf("@@@ timer_playlistListboxRefresh\n");

	timerSet(vp, TIMER_PLYPANE_REFRESH, 20);

	TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
	TLISTBOX *listbox = plyctrl->listbox.lb;
	if (!ccGetState(listbox)){
		return;
	}

	if (ccLock(listbox)){
		const int state = ccGetState(listbox);
		ccDisable(listbox);
		ccEnable(listbox);
		if (!state) ccDisable(listbox);
		ccUnlock(listbox);
	}
}

// TIMER_CTRL_UPDATETIMESTAMP
void overlayTimeStampSetTime (TVLCPLAYER *vp)
{
	//printf("overlayTimeStampSetTime in\n");

	//printf("overlayTimeStampSetTime %f %i \n", vp->vlc->position, (int)vp->vlc->length);
	if (pageGet(vp) == PAGE_OVERLAY)
		timeStampSetTime(vp, vp->vlc->position);
	else if (pageGet(vp) == PAGE_PLY_PANE)
		plypaneUpdateTimestamp(pageGetPtr(vp, PAGE_PLY_PANE));

	if (contextMenuIsTrackbarVisable(vp))
		taskbarPostMessage(vp, WM_TRACKTIMESTAMPNOTIFY, 0, 0);

	//printf("overlayTimeStampSetTime out\n");
}

// TIMER_CTRL_DISPLAYVOLRESET
void overlayDisplayVolReset (TVLCPLAYER *vp)
{
	//printf("overlayDisplayVolReset\n");

	TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
	plyctrl->volume.doRender = 0;
}

static inline int scaleTextScaleSet (TLABEL *label, TFRAME *frame, TTEXTSCALE *scale, const double t1)
{
	double dt = t1 - scale->time0;
	dt *= scale->accel;

	int died = 0;
	if (dt > 2000.0){
		dt = 2000.0;
		died = 1;
	}

	dt /= 1000.0;	// scale
	dt -= 1.0;		// normalize
	//dt *= 0.90;		// set velocity factor

	const double mod = computeModifier(scale, dt);
	const double fw = frame->width;
	const double fh2 = frame->height/2.0;

	double y;
	if (dt < 0.0)
		y = fh2 * mod;
	else if (dt > 0.0)
		y = (fh2 * (1.0-mod)) + fh2;
	else
		y = fh2;

	const int ty = abs((label->metrics.height+8) - (scale->textHeight*mod)) / 2;
	labelItemPositionSet(label, scale->lblIdText, 0, ty);

	ccSetPosition(label, (fw-label->metrics.width)/2, y-(scale->textHeight/2.0));
	if (scale->displayType != 2)
		labelRenderScaleSet(label, scale->lblIdText, mod);

	return died;
}

static inline int scaleTextCount (TTEXTSCALELLIST *scaleList)
{
	//printf("scaleTextCount %i\n", scaleList->count);
	return scaleList->count;
	//return listCount(scaleList->head.next);
}

static inline void scaleTextDelete (TTEXTSCALE *scale)
{
	if (scale->text){
		my_free(scale->text);
		scale->text = NULL;
	}
	ccDelete(scale->label);
}

static inline int scaleTextRender (TTEXTSCALELLIST *scaleList, TFRAME *frame, const double t1)
{
	int ct = 0;

	TLISTITEM *sItem = listGetNext(&scaleList->head);
	while(sItem){
		TTEXTSCALE *scale = listGetStorage(sItem);
		if (!scale) break;

		scaleList->timeLast = getTime(scale->label->cc->vp);
		int died = scaleTextScaleSet(scale->label, frame, scale, scaleList->timeLast);
		ccRender(scale->label, frame);

		TLISTITEM *next = listGetNext(sItem);
		if (died){
			listRemove(sItem);
			scaleTextDelete(scale);
			listDestroy(sItem);
			scaleList->count--;

			if (scaleList->head.next == sItem)
				scaleList->head.next = next;
		}
		sItem = next;
		ct++;
	}

	return ct;
}

static inline int page_plyctrlRender (TVIDEOOVERLAY *plyctrl, TVLCPLAYER *vp, TFRAME *frame, const double t1)
{
	TLISTBOX *listbox = plyctrl->listbox.lb;
	int listboxState = 0;

	if (ccLock(listbox)){
		listboxState = ccGetState(listbox) && listboxGetTotal(listbox);
		ccUnlock(listbox);
	}

	if (listboxState){
		ccRender(listbox, frame);
	}else{
		sliderSetTipPosition(plyctrl->trackSlider, vp->vlc->position);
		if (scaleTextCount(&plyctrl->scale))
			scaleTextRender(&plyctrl->scale, frame, t1);
	}

	const int playState = getPlayState(vp);
	const int isMarQuDrawn = marqueeDraw(vp, frame, plyctrl->marquee, 2, 2);
	if (isMarQuDrawn){
		buttonsStateSet(plyctrl->btns, VBUTTON_MHOOK, 0);
		//buttonsStateSet(plyctrl->btns, VBUTTON_EPG_PROGRAMMES, 0);
		buttonsStateSet(plyctrl->btns, VBUTTON_TIMESTMP, 0);
		buttonsStateSet(plyctrl->btns, VBUTTON_HOME, 0);
		buttonsStateSet(plyctrl->btns, VBUTTON_CHAPPREV, 0);
		buttonsStateSet(plyctrl->btns, VBUTTON_CHAPNEXT, 0);
		buttonsStateSet(plyctrl->btns, VBUTTON_VOLUMEMODE, 0);

	}

	if (!listboxState /* || !listboxGetTotalItems(lb)*/){
		if (vp->vlc->bufferPos > 0.0 && vp->vlc->bufferPos < 1.0){		// draw media buffer[ing] bar
			TSLIDER *s = plyctrl->trackSlider;
			TLPOINTEX rt;
			sliderGetSliderRect(s, &rt);
			const int bw = ((rt.x2-rt.x1)+1) * vp->vlc->bufferPos;
			lDrawRectangleFilled(frame, rt.x1, rt.y1+16, rt.x1+bw, rt.y2-16, 180<<24|COL_GREEN_TINT/*COL_BLUE_SEA_TINT*/);

			if (vp->vlc->bufferPos >= 1.0){
				vp->vlc->bufferPos = 0.0;
				renderSignalUpdate(vp);
			}
		}

		if (hasPageBeenAccessed(vp, PAGE_CHAPTERS)){
			TCHAPTER *chapt = pageGetPtr(vp, PAGE_CHAPTERS);
			if (chapt->ttitles && chapt->tchapters > 0)
				drawTrackChapterMarks(vp, frame, chapt, plyctrl->trackSlider);
		}
		ccRender(plyctrl->trackSlider, frame);

		if (playState){
			labelStrRender(plyctrl->title, frame);
			labelStrRender(plyctrl->album, frame);
		}
	}

	if (vp->gui.drawControlIcons) ccRender(plyctrl->ctrlpan.base, frame);

	if (hasPageBeenAccessed(vp, PAGE_EPG)){
		TEPG *epg = pageGetPtr(vp, PAGE_EPG);
		if (playState && !isMarQuDrawn && epgProgrammeLbGetTotal(epg) > 1)
			buttonsStateSet(plyctrl->btns, VBUTTON_EPG_PROGRAMMES, 1);
		else
			buttonsStateSet(plyctrl->btns, VBUTTON_EPG_PROGRAMMES, 0);
	}

	if (plyctrl->volume.doRender)
		volumeRender(plyctrl, frame, plyctrl->volume.val);

	drawButtonsVolume(vp, plyctrl->btns, plyctrl, frame);
	buttonsRenderAll(plyctrl->btns, frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);

	if (isMarQuDrawn){
		buttonsStateSet(plyctrl->btns, VBUTTON_TIMESTMP, 1);
		buttonsStateSet(plyctrl->btns, VBUTTON_HOME, 1);
		buttonsStateSet(plyctrl->btns, VBUTTON_VOLUMEMODE, 1);

		if (mHookGetState())
			buttonsStateSet(plyctrl->btns, VBUTTON_MHOOK, 1);

		if (hasPageBeenAccessed(vp, PAGE_CHAPTERS)){
			TCHAPTER *chapt = pageGetPtr(vp, PAGE_CHAPTERS);
			if (chapt->tchapters > 1){
				buttonsStateSet(plyctrl->btns, VBUTTON_CHAPPREV, 1);
				buttonsStateSet(plyctrl->btns, VBUTTON_CHAPNEXT, 1);
			}
		}
	}

	if (!listboxState){
		if (vp->gui.cursor.isHooked || isVirtual(vp)){
			if (ccIsHovered(plyctrl->trackSlider) || ccPositionIsOverlapped(plyctrl->trackSlider, vp->gui.cursor.dx, vp->gui.cursor.dy))
				drawTrackPositionText(plyctrl, vp, frame, &vp->gui);
		}
		if ((t1 - plyctrl->scale.timeLast < 500.0) || scaleTextCount(&plyctrl->scale))
			setTargetRate(vp, 25.0);
	}

	//ccRender(listbox, frame);

	//printf("ctrlRender %i %i %i\n", listbox->enabled, listbox->pane->enabled, listboxGetTotal(listbox));

	lSetRenderEffect(frame->hw, LTR_DEFAULT);
	return 1;
}

void ctrlNewTrackEvent (TVIDEOOVERLAY *plyctrl, unsigned int uid, const int trackIdx)
{
	//printf("ctrlNewTrackEvent in\n");

	char *title = getPlayingTitle(plyctrl->com->vp);
	if (title){
		char buffer[MAX_PATH_UTF8+1];
		snprintf(buffer, MAX_PATH_UTF8, "%i: %s", trackIdx+1, title);
		my_free(title);

		labelStrUpdate(plyctrl->title, buffer);

		char *album = getPlayingAlbum(plyctrl->com->vp);
		if (album){
			snprintf(buffer, MAX_PATH_UTF8, "%s ", album);
			my_free(album);

			labelStrUpdate(plyctrl->album, buffer);
		}else{
			labelStrUpdate(plyctrl->album, "  ");
		}
	}else{
		labelStrUpdate(plyctrl->title, " : ");
		labelStrUpdate(plyctrl->album, "  ");
	}

	//printf("ctrlNewTrackEvent out\n");
}

static inline int64_t ctrl_listbox_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (msg == CC_MSG_HOVER || msg == CC_MSG_RENDER || msg == CC_MSG_INPUT || msg == CC_MSG_SETPOSITION) return 1;
	//if (msg == CC_MSG_HOVER || msg == CC_MSG_RENDER) return 1;

	//printf("ctrl_listbox_cb. msg:%i, data1:%I64d, data2:%I64d,  ptr:%p\n", msg, data1, data2, dataPtr);

	TLISTBOX *listbox = (TLISTBOX*)object;

	switch (msg){
	  case CC_MSG_RENDER:{
	  	TVIDEOOVERLAY *plyctrl = ccGetUserData(listbox);
	  	TVLCPLAYER *vp = listbox->cc->vp;
	  	TFRAME *frame = (TFRAME*)dataPtr;

	  	int x, y;
	  	ccGetPosition(listbox, &x, &y);
	  	outlineTextEnable(vp->ml->hw, 240<<24 | COL_BLACK);
	  	drawHex(frame, x+64, y-58, LFONT, plyctrl->lbUID, 0x00<<24|COL_WHITE);
	  	shadowTextDisable(vp->ml->hw);

	  	break;
	  }
	  /*case LISTBOX_MSG_VALIDATED:
	  	printf("LISTBOX_MSG_VALIDATED total:%i sbEnabled:%i\n", (int)data1, (int)data2);
	  	break;*/

  	  case CC_MSG_ENABLED:{
  	  	TVLCPLAYER *vp = listbox->cc->vp;

		PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
		if (!plc){
			plc = getDisplayPlaylist(vp);
			if (!plc){
				//printf("listbox playlist unavailable, Q:%i D:%i\n", vp->playlist.queued, vp->playlist.display);
				break;
			}
		}

		if (renderLock(vp)){
			TVIDEOOVERLAY *plyctrl = ccGetUserData(listbox);
			if (plc){
				listbox->cb.disable(listbox);
				overlayPlaylistListboxFill(plyctrl, plc);
				if (!listboxGetTotal(listbox))
					ccDisable(listbox);
				else
					listbox->cb.enable(listbox);
			}

  	  		buttonsStateSet(plyctrl->btns, VBUTTON_TOUCHPAD, 0);
  	  		buttonsStateSet(plyctrl->btns, VBUTTON_VOLUMEMODE, 0);
  	  		buttonsStateSet(plyctrl->btns, VBUTTON_VOLUME, 0);
			buttonsStateSet(plyctrl->btns, VBUTTON_VOLUMEBASE, 0);

			ccDisable(plyctrl->trackSlider);
			if (vp->gui.drawControlIcons) ccDisable(plyctrl->ctrlpan.base);
			renderUnlock(vp);
		}
		break;
	  }
  	  case CC_MSG_DISABLED:{
  	  	TVLCPLAYER *vp = listbox->cc->vp;

  	  	//if (renderLock(vp)){
  	  		TVIDEOOVERLAY *plyctrl = ccGetUserData(listbox);
  	  		buttonsStateSet(plyctrl->btns, VBUTTON_VOLUMEMODE, 1);
  	  		buttonsStateSet(plyctrl->btns, VBUTTON_TOUCHPAD, 1);
  	  		buttonsStateSet(plyctrl->btns, VBUTTON_VOLUME, 1);
			buttonsStateSet(plyctrl->btns, VBUTTON_VOLUMEBASE, 1);

  	    	ccEnable(plyctrl->trackSlider);
  	    	if (vp->gui.drawControlIcons) ccEnable(plyctrl->ctrlpan.base);
		//	renderUnlock(vp);
		//}
	  	break;
  	  }
	case LISTBOX_MSG_ITEMSELECTED:{
  	  	//printf("LISTBOX_MSG_ITEMSELECTED %i %X\n", (int)data1, (int)data2);

		TVIDEOOVERLAY *plyctrl = ccGetUserData(listbox);
		TVLCPLAYER *vp = listbox->cc->vp;

  	  	// if its a playlist item then enter and fill that playlist instead
  	  	if ((data2&0x3FFFF) == 0x3FFFF){
  	  		int uid = ((data2&0xFFFC0000)>>18)+PLAYLIST_UID_BASE;

			PLAYLISTCACHE *plcCurrent = playlistManagerGetPlaylistByUID(vp->plm, plyctrl->lbUID);
			if (plcCurrent){
				if (playlistLock(plcCurrent)){
					plcCurrent->pr->lbFocus = listboxGetFocus(listbox);
					playlistUnlock(plcCurrent);
				}
			}

			PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
			if (plc){
				if (renderLock(vp)){
					if (overlayPlaylistListboxFill(plyctrl, plc)){
						if (plc->parent && plc->parent->pr){
							if (playlistLock(plc)){
								plc->parent->pr->selectedItem = playlistGetPlaylistIndex(plc->parent, plc);
								playlistUnlock(plc);
							}
						}
					}
					renderUnlock(vp);
					listboxSetFocus(listbox, plc->pr->lbFocus);
				}
			}
  	  		break;
  	  	}

  	  	// a track was selected
  	  	int track = data2&0x3FFFF;
  	  	int playlist = ((data2&0xFFFC0000)>>18)+PLAYLIST_UID_BASE;
		int itemId = data1;

		PLAYLISTCACHE *plc = NULL;
		if (playlist >= 0)
  	  		plc = playlistManagerGetPlaylistByUID(vp->plm, playlist);

  		if (plc && playlistGetTotal(plc)){
  			int ret = 0;
  			if (renderLock(vp)){
  				ret = startPlaylistTrack(vp, plc, track);
  				renderUnlock(vp);
  			}
  			if (ret){
  	  			vp->playlist.display = playlist;
  				plc->pr->selectedItem = track;
  				plc->pr->playingItem = track;
  	  			//invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), track);

  	  			// ensure selected item is highlighted immediately
  	  			//overlayPlaylistListboxFill(plyctrl, plc);
  	  			listboxSetHighlightedItem(listbox, itemId);
  	  			pageUpdate(plyctrl);
  	  		}
  	  	}
  	  }
	  break;
	}

	return 1;
}

static inline int page_plyctrlStartup (TVIDEOOVERLAY *plyctrl, TVLCPLAYER *vp, const int fw, const int fh)
{
	//printf("@ page_plyctrlStartup\n");

	plyctrl->chapterMark = NULL;
	plyctrl->chapterMarkFilled = NULL;
	plyctrl->lbUID = 0;

	plyctrl->ctrlpan.itemIds = my_calloc(VBUTTON_TOTAL, sizeof(int));
	if (!plyctrl->ctrlpan.itemIds) return 0;
	plyctrl->ctrlpan.width = fw;
	if (plyctrl->ctrlpan.width > 400) plyctrl->ctrlpan.width = 400;
	plyctrl->ctrlpan.height = 90;


	TLISTBOX *listbox = ccCreateEx(vp->cc, PAGE_OVERLAY, CC_LISTBOX2, ctrl_listbox_cb, NULL, fw, fh - 55, plyctrl);

	TLABEL *label = ccCreate(vp->cc, PAGE_OVERLAY, CC_LABEL, labelCcObject_cb, &vp->gui.ccIds[CCID_LABEL_CTRLPAN], plyctrl->ctrlpan.width, plyctrl->ctrlpan.height);
	plyctrl->ctrlpan.base = label;
	ccSetPosition(label, (fw - ccGetWidth(label))/2, ((fh - ccGetHeight(label))/2)+96);
	labelRenderFlagsSet(label, LABEL_RENDER_CCOBJ/*|LABEL_RENDER_BASE|LABEL_RENDER_BLUR*/);
	//labelBaseColourSet(label, 20<<24 | COL_WHITE);

	settingsGet(vp, "device.virtual.onScreenCtrlIcons", &vp->gui.drawControlIcons);
	vp->gui.drawControlIcons |= isVirtual(vp);
	if (!vp->gui.drawControlIcons)
		ccDisable(label);
	else
		ccEnable(label);
	plyctrl->ctrlpan.itemIds[VBUTTON_PRETRACK] = ctrlPanSetImage(label, L"ctrlovr/pretrack.png", VBUTTON_PRETRACK, 1, 0, 0);
	plyctrl->ctrlpan.itemIds[VBUTTON_PLAY] = ctrlPanSetImage(label, L"ctrlovr/play.png", VBUTTON_PLAY, 0, 0, 0);
	plyctrl->ctrlpan.itemIds[VBUTTON_PAUSE] = ctrlPanSetImage(label, L"ctrlovr/pause.png", VBUTTON_PAUSE, 0, 0, 0);
	plyctrl->ctrlpan.itemIds[VBUTTON_STOP] = ctrlPanSetImage(label, L"ctrlovr/stop.png", VBUTTON_STOP, 0, 0, 0);
	plyctrl->ctrlpan.itemIds[VBUTTON_NEXTTRACK] = ctrlPanSetImage(label, L"ctrlovr/nexttrack.png", VBUTTON_NEXTTRACK, 1, 0, 0);
	plyctrl->ctrlpan.bwidth = ccGetWidth(labelCCGet(label, plyctrl->ctrlpan.itemIds[VBUTTON_PRETRACK]));
	plyctrl->ctrlpan.bheight = ccGetHeight(labelCCGet(label, plyctrl->ctrlpan.itemIds[VBUTTON_PRETRACK]));
	plyctrl->ctrlpan.height = plyctrl->ctrlpan.bheight + 10;
	ctrlPanCalcPositions(&plyctrl->ctrlpan, BTNPANEL_SET_STOP);


	int y = fh * 0.01;
	TCCBUTTONS *btns = plyctrl->btns = buttonsCreate(vp->cc, PAGE_OVERLAY, VBUTTON_TOTAL, ccbtn_cb);
	TCCBUTTON *btn = buttonsCreateButton(btns, L"ctrlovr/mhook.png", NULL, VBUTTON_MHOOK, 1, 0, 0, 0);
	btn->flags.disableRender = 1;

	btn = buttonsCreateButton(btns, L"ctrlovr/touchpad.png", NULL, VBUTTON_TOUCHPAD, 1, 0, 0, 0);
	ccSetPosition(btn, (abs(fw - ccGetWidth(btn))/2)-32, (abs(fh - ccGetHeight(btn))/2)-32);
	buttonFaceHoverSet(btn, 0, 0, 0);
	ccSetUserData(btn, my_calloc(1, sizeof(TPADSIPE)));
	btn->flags.disableRender = 1;


	btn = buttonsCreateButton(btns, L"ctrlovr/timestamp.png", L"ctrlovr/timestamp.png", VBUTTON_TIMESTMP, 1, 0, 0, 0);
	ccSetPosition(btn, abs((ccGetWidth(btn) - fw)/2.0), y+2);
	buttonFaceTextSet(btn, BUTTON_PRI, "0:00/0:00", PF_CLIPWRAP|PF_MIDDLEJUSTIFY, CTRLOVR_TIMESTAMP_FONT, 0, 0);
	buttonFaceTextSet(btn, BUTTON_SEC, "0:00/0:00", PF_CLIPWRAP|PF_MIDDLEJUSTIFY, CTRLOVR_TIMESTAMP_FONT, 0, 0);
	buttonFaceTextColourSet(btn, BUTTON_PRI, 255<<24|COL_WHITE, 255<<24|COL_BLACK, 200<<24|COL_BLUE_SEA_TINT);
	buttonFaceTextColourSet(btn, BUTTON_SEC, 255<<24|COL_WHITE, 255<<24|COL_GREEN_TINT, 120<<24|COL_GREEN_TINT);
	buttonFaceHoverSet(btn, 0, COL_GREEN_TINT, 0.2);
	buttonFaceAutoSwapEnable(btn, BUTTON_FACE_SWAP_SELECTED);


	btn = buttonsCreateButton(btns, L"ctrlovr/chapterprev.png", NULL, VBUTTON_CHAPPREV, 0, 1, 0, 0);
	ccSetPosition(btn, buttonsPosXGet(btns, VBUTTON_TIMESTMP) - ccGetWidth(btn) + 14, y+2);

	btn = buttonsCreateButton(btns, L"ctrlovr/chapternext.png", NULL, VBUTTON_CHAPNEXT, 0, 1, 0, 0);
	ccSetPosition(btn, buttonsPosXGet(btns, VBUTTON_TIMESTMP) + buttonsWidthGet(btns, VBUTTON_TIMESTMP) - 13, y+2);

	btn = buttonsCreateButton(btns, L"ctrlovr/volume_app.png", L"ctrlovr/volume_master.png", VBUTTON_VOLUMEMODE, 1, 0, 0, 0);
	ccSetPosition(btn, buttonsPosXGet(btns, VBUTTON_CHAPPREV) - ccGetWidth(btn) - 24, y-2);

	btn = buttonsCreateButton(btns, L"ctrlovr/home.png", NULL, VBUTTON_HOME, 1, 0, 0, 0);
	ccSetPosition(btn, fw-1 - ccGetWidth(btn), y);

	btn = buttonsCreateButton(btns, L"ctrlovr/dvb.png", NULL, VBUTTON_EPG_PROGRAMMES, 0, 0, 0, 0);
	ccSetPosition(btn, buttonsPosXGet(btns, VBUTTON_HOME) - ccGetWidth(btn) - 6, y);


	btn = buttonsCreateButton(btns, L"ctrlovr/volumebase.png", L"ctrlovr/volumebasealt.png", VBUTTON_VOLUMEBASE, 1, 0, 0, 0);
	btn->canDrag = 1;
	btn->flags.acceptDrag = 1;
	ccSetPosition(btn, abs(fw - ccGetWidth(btn)), (abs(fh - ccGetHeight(btn))/2)-24);

	btn = buttonsCreateButton(btns, L"ctrlovr/volumelevel.png", L"ctrlovr/volumelevelalt.png", VBUTTON_VOLUME, 1, 0, 0, 0);
	btn->flags.disableRender = 1;
	btn->canDrag = 1;
	btn->flags.acceptDrag = 1;
	ccSetPosition(btn, buttonsPosXGet(btns, VBUTTON_VOLUMEBASE), buttonsPosYGet(btns, VBUTTON_VOLUMEBASE));


	y = fh * 0.17;
	if (y < 44) y = 44;
	TSLIDER *slider = ccCreate(vp->cc, PAGE_OVERLAY, CC_SLIDER_HORIZONTAL, ctrlCcObject_cb, &vp->gui.ccIds[CCID_SLIDER_TRACKPOSITION], fw - 32, 32);
	plyctrl->trackSlider = slider;
	slider->pad.top = 16;
	slider->pad.btm = 16;
	sliderFaceSet(slider, SLIDER_FACE_LEFT,  L"cc/slider_h_solid2_left.png");
	sliderFaceSet(slider, SLIDER_FACE_RIGHT, L"cc/slider_h_solid2_right.png");
	sliderFaceSet(slider, SLIDER_FACE_MID,   L"cc/slider_h_solid2_mid.png");
	sliderFaceSet(slider, SLIDER_FACE_TIP,   L"cc/slider_h_solid2_tip.png");
	sliderFacesApply(slider);
	sliderHoverDisable(slider);
	ccSetMetrics(slider, (fw - ccGetWidth(slider))/2, fh - ccGetHeight(slider) - 48, -1, -1);
	sliderSetRange(slider, 0, 10000000);
	sliderSetValue(slider, 0);
	ccEnable(slider);


	int x = ccGetPositionX(slider);
	y = ccGetPositionY(slider) - 2;
	plyctrl->title = labelStrCreate(plyctrl, " ", CTRLOVR_META_FONT, x, y - 32, fw - x, plyctrl, NSEX_LEFT);
	plyctrl->album = labelStrCreate(plyctrl, " ", CTRLOVR_META_FONT, 0, y + ccGetHeight(slider) - 2, fw - x, plyctrl, NSEX_RIGHT);
	plyctrl->marquee = marqueeNew(16, MARQUEE_CENTER, TRACKPLAYMARQ_FONT);

	// set listbox position and height relative to timestamp position
	x = 2;
	y = buttonsPosYGet(btns, VBUTTON_TIMESTMP) + buttonsHeightGet(btns, VBUTTON_TIMESTMP) + 8;

	plyctrl->listbox.lb = listbox;
	listboxSetFont(listbox, CTRLOVR_LISTBOX_FONT);
	
	
	listboxSetLineHeight(listbox, 41);
	listboxSetScrollbarWidth(listbox, SCROLLBAR_VERTWIDTH);
	listboxSetHighlightedColour(listbox, CTRL_HIGHLIGHT_COLOUR, 255<<24|COL_BLACK, 177<<24|COL_BLUE_SEA_TINT);
	ccSetMetrics(listbox, 0, y+1, fw, fh-1-y);

	return 1;
}

static inline int64_t lbl_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT || msg == CC_MSG_SETPOSITION) return 1;

	//printf("lbl_cb %i\n", msg);

	TLABEL *label = (TLABEL*)object;

	if (msg == CC_MSG_CREATE){
		TTEXTSCALE *scale = ccGetUserData(label);
		if (!scale) return 1;

		scale->lblIdText = labelTextCreate(label, scale->text, PF_MIDDLEJUSTIFY, scale->font, 0, 0);

		labelRenderFilterSet(label, scale->lblIdText, scale->displayType);
		labelRenderBlurRadiusSet(label, scale->lblIdText, 4);
		labelRenderColourSet(label, scale->lblIdText, 255<<24 | COL_WHITE, 255<<24 | COL_BLUE_SEA_TINT, 80<<24 | COL_BLUE);
		labelRenderScaleSet(label, scale->lblIdText, 1.0);
		labelStringGetMetrics(label, scale->lblIdText, NULL, NULL, &scale->textWidth, &scale->textHeight);
		ccSetMetrics(label, -1, -1, scale->textWidth+16, scale->textHeight+1);
		labelRenderFlagsSet(label, LABEL_RENDER_TEXT/*|LABEL_RENDER_BORDER_PRE|LABEL_RENDER_BORDER_POST*/);

		ccEnable(label);
		//my_free(scale->text);
		//scale->text = NULL;

	}else if (msg == CC_MSG_DELETE){
		my_free(ccGetUserData(label));
	}

	return 1;
}

static inline TTEXTSCALE *scaleTextCreate (TVLCPLAYER *vp, const int displayType, const char *text)
{
	TTEXTSCALE *scale = my_calloc(1, sizeof(TTEXTSCALE));
	if (!scale) return NULL;

	scale->text = my_strdup(text);
	if (!scale->text){
		my_free(scale);
		return NULL;
	}

	const double sigma = 0.450;
	scale->sigma = sigma;
	scale->rho = 0.0;
    scale->expMultiplier = 0.0;
    scale->expMember = 0.0;
    scale->accel = 1.0;
    scale->displayType = displayType;
	scale->font = CTRLOVR_NEWTITLE_FONT;
	scale->time0 = getTime(vp)+300.0;
	setSigma(scale, scale->sigma);

	scale->label = ccCreateEx(vp->cc, PAGE_OVERLAY, CC_LABEL, lbl_cb, &scale->lblId, 200, 32, scale);

	//printf("addScale %i\n", scale->label->id);

	//my_free(scale->text);
	//scale->text = NULL;

	return scale;
}

void overlayAddTitle (TVIDEOOVERLAY *plyctrl, const char *text)
{
	overlayActivateOverlayResetTimer(plyctrl->com->vp);
	renderSignalUpdate(plyctrl->com->vp);

	// accelerate all other titles' so as they don't interfre with this
	TLISTITEM *sItem = plyctrl->scale.head.next;
	while(sItem){
		TTEXTSCALE *scale = listGetStorage(sItem);
		if (!scale) break;
		scale->accel = 1.85;
		scale->expMember *= 0.85;
		//scale->displayType = 0;
		//labelRenderFilterSet(scale->label, scale->lblIdText, scale->displayType);
		sItem = listGetNext(sItem);
	}


	TTEXTSCALE *scale = scaleTextCreate(plyctrl->com->vp, 1, text);
	if (scale){
		plyctrl->scale.count++;

		TLISTITEM *last = listGetLast(&plyctrl->scale.head);
		listInsert(last, NULL, listNewItem(scale));
	}

	if (page2RenderGetState(plyctrl->com->pages, PAGE_OVERLAY))
		setTargetRate(plyctrl->com->vp, 25);
	renderSignalUpdate(plyctrl->com->vp);

	//printf("addScale count %i\n", listCount(plyctrl->scale.list->head.next));
}

static inline int page_plyctrlInitalize (TVIDEOOVERLAY *plyctrl, TVLCPLAYER *vp, const int width, const int height)
{

	vp->gui.image[IMGC_CHAPTERMARK] = imageManagerImageAdd(vp->im, L"ctrlovr/chaptermark.png");
	vp->gui.image[IMGC_CHAPTERMARKFILLED] = imageManagerImageAdd(vp->im, L"ctrlovr/chaptermarkfilled.png");

	wchar_t *digitsFolder;
	settingsGetW(vp, "volume.digits", &digitsFolder);
	if (!digitsFolder) return 0;

	wchar_t buffer[MAX_PATH+1];
	for (int i = 0; i < 10; i++){
		__mingw_snwprintf(buffer, MAX_PATH, L"common/digits/%ls/%i.png", digitsFolder, i);
		plyctrl->volume.imageIds[i] = imageManagerImageAdd(vp->im, buffer);
	}
	my_free(digitsFolder);

	plyctrl->volume.verticalOffset = 20;
	plyctrl->volume.charOverlap = 0;
	plyctrl->volume.doRender = 0;
	settingsGet(vp, "volume.changeDelta", &plyctrl->volume.changeDelta);

	//plyctrl->scale = my_calloc(1, sizeof(TTEXTSCALELLIST));
	//if (plyctrl->scale.list)
	plyctrl->scale.timeLast = getTime(vp);

	setPageAccessed(vp, PAGE_OVERLAY);
	return 1;
}

static inline int page_plyctrlShutdown (TVIDEOOVERLAY *plyctrl, TVLCPLAYER *vp)
{

	ccDelete(plyctrl->listbox.lb);

	marqueeDelete(plyctrl->marquee);

	buttonsDeleteAll(plyctrl->btns);
	ccDelete(plyctrl->trackSlider);
	labelStrDelete(plyctrl->title);
	labelStrDelete(plyctrl->album);

	my_free(plyctrl->ctrlpan.itemIds);
	ccDelete(plyctrl->ctrlpan.base);

	TLISTITEM *sItem = plyctrl->scale.head.next;
	while(sItem){
		scaleTextDelete(listGetStorage(sItem));
		sItem = listGetNext(sItem);
	}

	listDestroyNext(plyctrl->scale.head.next);

	return 1;
}

static inline int page_plyctrlRenderBegin (TVIDEOOVERLAY *plyctrl, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	//printf("page_plyctrlRenderBegin\n");

	plyctrl->chapterMark = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_CHAPTERMARK]);
	plyctrl->chapterMarkFilled = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_CHAPTERMARKFILLED]);
	plyctrl->lbUnderlayImgId = 0;// not used

	/*for (int i = 0; i < 10; i++){
		plyctrl->volume.images[i] = imageManagerImageAcquire(vp->im, plyctrl->volume.imageIds[i]);
		printf("imageManagerImageAcquire %i\n", plyctrl->volume.imageIds[i]);
	}*/

	lSetCharacterEncoding(frame->hw, CMT_UTF8);

	if (playlistManagerGetTotal(vp->plm) <= 1){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, 0);
		if (!plc || playlistGetCount(plc, PLAYLIST_OBJTYPE_TRACK) < 1){
			//printf("page_plyctrlRenderBegin\n");

			page2Set(vp->pages, PAGE_HOME, 0);
			page2RenderDisable(vp->pages, PAGE_OVERLAY);
			timerReset(vp, TIMER_CTRL_OVERLAYRESET);
			return 0;
		}
	}else{
		timerSet(vp, TIMER_CTRL_UPDATETIMESTAMP, 0);
		overlayActivateOverlayResetTimer(vp);
	}

	ccHoverRenderSigEnable(vp->cc, 20.0);
	return 1;
}

static inline void page_plyctrlRenderEnd (TVIDEOOVERLAY *plyctrl, TVLCPLAYER *vp, int64_t destId, int64_t data2, void *opaquePtr)
{
	//printf("page_plyctrlRenderEnd\n");

	ccHoverRenderSigDisable(vp->cc);

	if (playlistManagerGetTotal(vp->plm) <= 1){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, 0);
		if (!plc || playlistGetCount(plc, PLAYLIST_OBJTYPE_TRACK) < 1)
			page2Set(vp->pages, PAGE_HOME, 0);
	}else{
		timerSet(vp, TIMER_CTRL_UPDATETIMESTAMP, 0);
		overlayActivateOverlayResetTimer(vp);
	}

	/*for (int i = 0; i < 10; i++){
		imageManagerImageRelease(vp->im, plyctrl->volume.imageIds[i]);
		printf("imageManagerImageRelease %i\n", plyctrl->volume.imageIds[i]);
	}*/

	imageManagerImageRelease(vp->im, vp->gui.image[IMGC_CHAPTERMARK]);
	imageManagerImageRelease(vp->im, vp->gui.image[IMGC_CHAPTERMARKFILLED]);
	artManagerFlush(vp->im);
	artManagerFlush(vp->am);
	//vp->gui.frameCt = 0;
}

static inline int page_plyctrlInput (TVIDEOOVERLAY *plyctrl, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	//printf("@ page_plyctrlInput msg:%i %i %p\n", msg, flags, pos);

	static unsigned int lastId = 0;

	switch(msg){
	  case PAGE_IN_TOUCH_DOWN:
		if (!flags){		// pen down
			if (lastId >= pos->id)
				return 0;
			lastId = pos->id;
		}else if (lastId != pos->id){
			return 0;
		}
		break;

	  case PAGE_IN_WHEEL_FORWARD:
	  	if (ccGetState(plyctrl->listbox.lb)){
	  		listboxScrollUp(plyctrl->listbox.lb);
	  	}else{
	  		const int x = flags >> 16;
	  		const int y = flags & 0xFFFF;
	  		TSLIDER *slider = plyctrl->trackSlider;

	  		if (ccPositionIsOverlapped(slider, x, y)){
				int64_t max = 1;
				sliderGetRange(slider, NULL, &max);
				int64_t diff = max * 0.01;
				//printf("forward: %I64d %I64d %I64d\n", max, diff, sliderGetValue(slider) + diff);
				sliderSetValue(slider, sliderGetValue(slider) + diff);
	  		}else{
	  			TCCBUTTON *btn = buttonsButtonGet(plyctrl->btns, VBUTTON_VOLUME);
	  			if (ccPositionIsOverlapped(btn, x, y)){
	  				if (buttonFaceActiveGet(btn) == BUTTON_PRI)
	  					setVolumeDisplay(vp, setVolume(vp, getVolume(vp, VOLUME_APP)+plyctrl->volume.changeDelta, VOLUME_APP));
	  				else
	  					setVolumeDisplay(vp, setVolume(vp, getVolume(vp, VOLUME_MASTER)+plyctrl->volume.changeDelta, VOLUME_MASTER));
	  			}
	  		}
	  	}
	  	break;

	  case PAGE_IN_WHEEL_BACK:
	  	if (ccGetState(plyctrl->listbox.lb)){
	  		listboxScrollDown(plyctrl->listbox.lb);
	  	}else{
	  		const int x = flags >> 16;
	  		const int y = flags & 0xFFFF;
	  		TSLIDER *slider = plyctrl->trackSlider;

	  		if (ccPositionIsOverlapped(slider, x, y)){
				int64_t max = 1;
				sliderGetRange(slider, NULL, &max);
				int64_t diff = max * 0.01;
				//printf("back: %I64d %I64d %I64d\n", max, diff, sliderGetValue(slider) - diff);
				sliderSetValue(slider, sliderGetValue(slider) - diff);
	  		}else{
	  			TCCBUTTON *btn = buttonsButtonGet(plyctrl->btns, VBUTTON_VOLUME);
	  			if (ccPositionIsOverlapped(btn, x, y)){
	  				if (buttonFaceActiveGet(btn) == BUTTON_PRI)
	  					setVolumeDisplay(vp, setVolume(vp, getVolume(vp, VOLUME_APP)-plyctrl->volume.changeDelta, VOLUME_APP));
	  				else
	  					setVolumeDisplay(vp, setVolume(vp, getVolume(vp, VOLUME_MASTER)-plyctrl->volume.changeDelta, VOLUME_MASTER));
	  			}
	  		}
	  	}
		break;
	};

	_overlaySetOverlay(plyctrl);
	return 1;
}

static inline int page_plyctrlRenderInit (TVIDEOOVERLAY *plyctrl, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	return 1;
}

int page_plyctrlCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TVIDEOOVERLAY *plyctrl = (TVIDEOOVERLAY*)pageStruct;

	// if (msg != PAGE_CTL_RENDER)
	//	 printf("# page_plyctrlCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);

	if (msg == PAGE_CTL_RENDER){
		return page_plyctrlRender(plyctrl, plyctrl->com->vp, dataPtr, dataInt1/1000.0);

	}else if (msg == PAGE_CTL_RENDER_START){
		return page_plyctrlRenderBegin(plyctrl, plyctrl->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);

	}else if (msg == PAGE_CTL_RENDER_END){
		page_plyctrlRenderEnd(plyctrl, plyctrl->com->vp, dataInt1, dataInt2, opaquePtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_plyctrlInput(plyctrl, plyctrl->com->vp, dataInt1, dataInt2, dataPtr);

	}else if (msg == PAGE_CTL_STARTUP){
		return page_plyctrlStartup(plyctrl, plyctrl->com->vp, dataInt1, dataInt2);

	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_plyctrlRenderInit(plyctrl, plyctrl->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);

	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_plyctrlInitalize(plyctrl, plyctrl->com->vp, dataInt1, dataInt2);

	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_plyctrlShutdown(plyctrl, plyctrl->com->vp);

	}else if (msg == PAGE_MSG_INPUT_MCAP){
		buttonsStateSet(plyctrl->btns, VBUTTON_MHOOK, dataInt1);

#if ENABLE_ANTPLUS
	}else if (msg == PAGE_MSG_DEVICE_ARRIVE){
		int vid = dataInt1;
		int pid = dataInt2;
		int cfgvid, cfgpid;

		antConfigGetDeviceIds(plyctrl, &cfgvid, &cfgpid);
		if (vid == cfgvid && pid == cfgpid){
			wchar_t *img = L"home/antpluson.png";
			picQueueAdd(plyctrl->com->vp->gui.picQueue, img, img, getTickCount()+8000);
		}
	}else if (msg == PAGE_MSG_DEVICE_DEPART){
		int vid = dataInt1;
		int pid = dataInt2;
		int cfgvid, cfgpid;

		antConfigGetDeviceIds(plyctrl, &cfgvid, &cfgpid);
		if (vid == cfgvid && pid == cfgpid){
			wchar_t *img = L"home/antplusoff.png";
			picQueueAdd(plyctrl->com->vp->gui.picQueue, img, img, getTickCount()+8000);
		}
#endif
	}

	return 1;
}

