
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




static inline int64_t iOvrCcObject_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	if (msg == CC_MSG_RENDER) return 1;
	
	if (obj->type == CC_SLIDER_HORIZONTAL){
		TSLIDER *slider = (TSLIDER*)object;
		TVLCPLAYER *vp = (TVLCPLAYER*)obj->cc->vp;
	
		if (slider->id == ccGetId(vp, CCID_SLIDER_IMGOVR)){
			switch (msg){
			  //case SLIDER_MSG_VALSET:
		  	  case SLIDER_MSG_VALCHANGED:{
		  	  	TIOVR *iovr = pageGetPtr(vp, PAGE_IMGOVR);
				char imagePath[MAX_PATH_UTF8+1];
				iovr->currentImg = sliderGetValue(iovr->slider);
				playlistGetPath(iovr->plc, iovr->currentImg, imagePath, MAX_PATH_UTF8);
				imageOverSetImage(iovr, getFrontBuffer(vp)->width, getFrontBuffer(vp)->height, imagePath);
				break;
			  }
			}			
		}
	}
	return 1;
}

static inline int imgOvrNewImgLock (TIOVR *iovr)
{
	return lockWait(iovr->hNewImgLock, INFINITE);
}

static inline void imgOvrNewImgUnlock (TIOVR *iovr)
{
	lockRelease(iovr->hNewImgLock);
}

static inline void imgOvrNewImgSetSignal (TIOVR *iovr)
{
	SetEvent(iovr->hLoadEvent);
}

static inline int waitForNewImageLoadSignal (TIOVR *iovr)
{
	return (WaitForSingleObject(iovr->hLoadEvent, INFINITE) == WAIT_OBJECT_0);
}

static inline int imgOvrLoadLock (TIOVR *iovr)
{
	return lockWait(iovr->hLoadLock, INFINITE);
}

static inline void imgOvrLoadUnlock (TIOVR *iovr)
{
	lockRelease(iovr->hLoadLock);
}

static inline int imgOvrButtonPress (TIOVR *iovr, TCCBUTTON *btn, const int id, const TTOUCHCOORD *pos)
{
	TVLCPLAYER *vp = iovr->com->vp;
	TFRAME *frame = getFrontBuffer(vp);
	iovr->btns->t0 = getTickCount();


	switch (id){
	  case IBUTTON_ROTATE:
	  	if (iovr->plc){
	  		if (!iovr->rotateImg)
	  			iovr->rotateImg = IMGOVR_ROTATE_RIGHT;
	  		else if (iovr->rotateImg == IMGOVR_ROTATE_RIGHT)
	  			iovr->rotateImg = IMGOVR_ROTATE_180;
	  		else if (iovr->rotateImg == IMGOVR_ROTATE_180)
	  			iovr->rotateImg = IMGOVR_ROTATE_LEFT;
	  		else
	  			iovr->rotateImg = IMGOVR_ROTATE_NONE;
	  		
	  		char imagePath[MAX_PATH_UTF8+1];
			playlistGetPath(iovr->plc, iovr->currentImg, imagePath, MAX_PATH_UTF8);
			imageOverSetImage(iovr, frame->width, frame->height, imagePath);
		}
		break;

	  case IBUTTON_PREV:
	  	if (iovr->plc){
			if (--iovr->currentImg < 0)
				iovr->currentImg = playlistGetTotal(iovr->plc)-1;

			char imagePath[MAX_PATH_UTF8+1];
			playlistGetPath(iovr->plc, iovr->currentImg, imagePath, MAX_PATH_UTF8);
			imageOverSetImage(iovr, frame->width, frame->height, imagePath);
			sliderSetValue(iovr->slider, iovr->currentImg);
		}
		break;
		
	  case IBUTTON_NEXT:
		if (iovr->plc){
			if (++iovr->currentImg >= playlistGetTotal(iovr->plc))
				iovr->currentImg = 0;
		
			char imagePath[MAX_PATH_UTF8+1];
			playlistGetPath(iovr->plc, iovr->currentImg, imagePath, MAX_PATH_UTF8);
			imageOverSetImage(iovr, frame->width, frame->height, imagePath);
			sliderSetValue(iovr->slider, iovr->currentImg);
		}
		break;
	}
	return 0;
}

int iOvrBuildImageList (TIOVR *iovr, TVLCPLAYER *vp, wchar_t *mrl)
{
	wchar_t drive[_MAX_DRIVE+1];
	wchar_t dir[_MAX_DIR+1];
	wchar_t buffer[MAX_PATH+1];
	_wsplitpath(mrl, drive, dir, NULL, NULL);
	__mingw_snwprintf(buffer, MAX_PATH, L"%ls%ls", drive, dir);
		

	if (iovr->plc) playlistFree(iovr->plc);
	iovr->plc = playlistNew(vp->plm, "browser_image");

	filepaneBuildPlaylistDir(pageGetPtr(vp, PAGE_FILE_PANE), iovr->plc, buffer, FILEMASKS_IMAGE, 0);

	if (!playlistGetTotal(iovr->plc)){
		return -1;
	}else{
		if (playlistGetTotal(iovr->plc) > 2)
			ccEnable(iovr->slider);
		else
			ccDisable(iovr->slider);
		return playlistGetPositionByHash(iovr->plc, getHashW(mrl));
	}
}

static inline int imageOverSetImageW (TIOVR *iovr, TVLCPLAYER *vp, const int dwidth, const int dheight, const wchar_t *filename)
{
	TFRAME *tmp = newImage(vp, filename, LFRM_BPP_32A);
	if (!tmp){
		wchar_t path[MAX_PATH+1];
		buildSkinD(vp, path, L"common\\icons\\fileicon.png");
		tmp = newImage(vp, path, LFRM_BPP_32A);
	}
	
	if (tmp){
		if (iovr->rotateImg == IMGOVR_ROTATE_LEFT)
	 		rotateFrameL90(tmp);
	 	else if (iovr->rotateImg == IMGOVR_ROTATE_RIGHT)
	 		rotateFrameR90(tmp);
	 	else if (iovr->rotateImg == IMGOVR_ROTATE_180)
	 		rotateFrame180(tmp);

		int w, h;
		imageBestFit(iovr->width, iovr->height, tmp->width, tmp->height, &w, &h);
		lResizeFrame(iovr->img, w+2, h+2, 0);
		//printf("frameSize %i %i  %i %i\n", w, h, iovr->img->width, iovr->img->height);
		
		copyAreaScaled(tmp, iovr->img, 0, 0, tmp->width-1, tmp->height-1, 1, 1, w, h);
		//trans Scale(tmp, iovr->img, w, h, 0, 0, SCALE_BILINEAR);
		
		lDrawRectangle(iovr->img, 0, 0, iovr->img->width-1, iovr->img->height-1, swatchGetColour(vp, PAGE_IMGOVR, SWH_IOVR_IMGBORDER));
		iovr->x = (dwidth - iovr->img->width)/2.0;
		iovr->y = (dheight - iovr->img->height)/2.0;

		if (imgOvrLoadLock(iovr)){
			if (iovr->protectedImg)
				lDeleteFrame(iovr->protectedImg);
			iovr->enabled = (iovr->protectedImg=lCloneFrame(iovr->img)) != NULL;
			iovr->dwidth = tmp->width;
			iovr->dheight = tmp->height;
			imgOvrLoadUnlock(iovr);
		}else{
			iovr->enabled = 0;
		}
		
		lDeleteFrame(tmp);
		return 1;
	}else{
		//wprintf(L"imageOvr: can not load '%s'\n", filename);
		iovr->enabled = 0;
		return 0;
	}
}

int imageOverSetImage8 (TIOVR *iovr, TVLCPLAYER *vp, const int dwidth, const int dheight, char *filename)
{
	int ret = 0;
	wchar_t *path = converttow(filename);
	if (path){
		ret = imageOverSetImageW(iovr, vp, dwidth, dheight, path);
		my_free(path);
	}
	return ret;
}

void imageOverSetImage (TIOVR *iovr, const int dwidth, const int dheight, char *imgpath8)
{
	if (imgOvrNewImgLock(iovr)){
		iovr->nextWidth = dwidth;
		iovr->nextHeight = dheight;
		if (iovr->nextPath8)
			my_free(iovr->nextPath8);
		iovr->nextPath8 = my_strdup(imgpath8);
		iovr->nextIsReady = (iovr->nextPath8 != NULL);
		imgOvrNewImgSetSignal(iovr);
		imgOvrNewImgUnlock(iovr);
	}
}

static inline unsigned int __stdcall imgOvrLoadThread (void *ptr)
{
	//printf("imgOvrLoadThread start %i\n", (int)GetCurrentThreadId());
	
	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;
	TIOVR *iovr = pageGetPtr(vp, PAGE_IMGOVR);
	iovr->threadState = 1;
	
	int width = 0, height = 0, isReady = 0;
	char *path8 = NULL;
	
	do{
		if (waitForNewImageLoadSignal(iovr)){
			if (iovr->threadState && vp->applState){
				if (imgOvrNewImgLock(iovr)){
					if (iovr->nextIsReady){
						isReady = iovr->nextIsReady;
						iovr->nextIsReady = 0;
						width = iovr->nextWidth;
						height = iovr->nextHeight;
						if (path8)
							my_free(path8);
						path8 = my_strdup(iovr->nextPath8);
					}
					imgOvrNewImgUnlock(iovr);
				}
				if (isReady){
					isReady = 0;
					if (path8){
						imageOverSetImage8(iovr, vp, width, height, path8);
						renderSignalUpdate(vp);
						my_free(path8);
						path8 = NULL;
					}
				}
			}
		}
	}while(vp->applState && iovr->threadState);
	
	iovr->threadState = 0;
	//printf("imgOvrLoadThread end %i\n", (int)GetCurrentThreadId());
	
	_endthreadex(1);
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
		return imgOvrButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	return 1;
}

static void drawImgInfo (TFRAME *frame, const int foreCol, const char *path8, const int imgIdx, const int width, const int height, const int x, const int y)
{
	lSetCharacterEncoding(frame->hw, CMT_UTF8);
	lSetForegroundColour(frame->hw, foreCol);
	
	lSetRenderEffect(frame->hw, LTR_SHADOW);
	lSetFilterAttribute(frame->hw, LTR_SHADOW, 0, LTRA_SHADOW_S|LTRA_SHADOW_E | LTRA_SHADOW_S3 | LTRA_SHADOW_OS(1) | LTRA_SHADOW_TR(160));
	lSetBackgroundColour(frame->hw, 0x00000000);
	
	if (path8)
		lPrintf(frame, x, y, MFONT, LPRT_CPY, "%s", path8);

	lPrintf(frame, x, y+20, MFONT, LPRT_CPY, "%i", imgIdx+1);
	lPrintf(frame, x, y+40, MFONT, LPRT_CPY, "%ix%i", width, height);
	
	lSetRenderEffect(frame->hw, LTR_DEFAULT);
}

static inline int page_imgOvrRender (TIOVR *iovr, TVLCPLAYER *vp, TFRAME *frame)
{
	char *path8 = NULL;
	if (iovr->enabled){
		if (imgOvrLoadLock(iovr)){
			// faster but lower quality and doesn't consider the alpha channel
			//fastFrameCopy(iovr->img, frame, iovr->x, iovr->y);
			
			// alpha blend image
			if (iovr->protectedImg){
				path8 = my_strdup(iovr->nextPath8);
				drawImage(iovr->protectedImg, frame, iovr->x, iovr->y, iovr->protectedImg->width-1, iovr->protectedImg->height-1);
			}
			imgOvrLoadUnlock(iovr);
		}
	}


	drawImgInfo(frame, swatchGetColour(vp, PAGE_OVERLAY, SWH_OVR_FPSTEXT), path8, iovr->currentImg, iovr->dwidth, iovr->dheight, 5, 0);
	if (path8) my_free(path8);

	buttonsRenderAll(iovr->btns, frame, BUTTONS_RENDER_HOVER);
	ccRender(iovr->slider, frame);

	return 1;
}

static inline int page_imgOvrInput (TIOVR *iovr, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_TOUCH_DOWN:
		// check if image overlay was touched, if so then remove the overlay
		if ((pos->dt > 100 || flags == 0) && pos->y > 64 && pos->y < getFrontBuffer(vp)->height-130){
			//if (pos->x > iovr->x && pos->x < iovr->x+iovr->img->width){
				//if (pos->y > iovr->y && pos->y < iovr->y+iovr->img->height){
					TIOVR *iovr = pageGetPtr(vp, PAGE_IMGOVR);
				iovr	->enabled = 0;
					if (iovr->plc){
						playlistFree(iovr->plc);
						iovr->plc = NULL;
					}
					//pageSetPrevious(vp);
					page2SetPrevious(iovr);
					//pageSetSec(vp, -1);
					return 0;
				//}
			//}
		}
	  	break;
	  case PAGE_IN_WHEEL_FORWARD:
	  	imgOvrButtonPress(iovr, NULL, IBUTTON_PREV, NULL);
	  	return 1;
	  	
	  case PAGE_IN_WHEEL_BACK:
	  	imgOvrButtonPress(iovr, NULL, IBUTTON_NEXT, NULL);
	  	return 1;

	  case PAGE_IN_WHEEL_LEFT:
	  case PAGE_IN_WHEEL_RIGHT:
	  	imgOvrButtonPress(iovr, NULL, IBUTTON_ROTATE, NULL);
	  	return 1;
	}

	return 0;
}

static inline int page_imgOvrStartup (TIOVR *iovr, TVLCPLAYER *vp, const int fw, const int fh)
{
	iovr->x = 2;
	iovr->y = 2;
	iovr->width = fw - (iovr->x * 2);
	iovr->height = fh - (iovr->y * 2);
	iovr->img = lNewFrame(getFrontBuffer(vp)->hw, 8, 8, LFRM_BPP_32A);
	iovr->plc = NULL;
	iovr->currentImg = -1;
	iovr->rotateImg = 0;

	iovr->hLoadLock = lockCreate("imageOverlayLoad");
	iovr->hNewImgLock = lockCreate("imageOverlayNew");
	
	//iovr->hImgOvrLoadThread = _beginthreadex(NULL, THREADSTACKSIZE, imgOvrLoadThread, vp, 0, &iovr->loadLockThreadID);
	iovr->hLoadEvent = CreateEvent(NULL, 0, 0, NULL);
	iovr->nextWidth = 0;
	iovr->nextHeight = 0;
	iovr->nextPath8 = NULL;
	iovr->nextIsReady = 0;

	TCCBUTTONS *btns = iovr->btns = buttonsCreate(vp->cc, PAGE_IMGOVR, IBUTTON_TOTAL, ccbtn_cb);
	
	TCCBUTTON *btn = buttonsCreateButton(btns, L"imgview/inext.png", L"imgview/inexthl.png", IBUTTON_NEXT, 1, 0, 0, 0);
	ccSetPosition(btn, fw-1 - ccGetWidth(btn), 0);
	buttonFaceAutoSwapEnable(btn, BUTTON_FACE_SWAP_SELECTED);

	btn = buttonsCreateButton(btns, L"imgview/iprev.png", L"imgview/iprevhl.png", IBUTTON_PREV, 1, 0, 0, 0);
	int x = buttonsPosXGet(btns, IBUTTON_NEXT) - ccGetWidth(btn) - 8;
	ccSetPosition(btn, x, 0);
	buttonFaceAutoSwapEnable(btn, BUTTON_FACE_SWAP_SELECTED);

	btn = buttonsCreateButton(btns, L"imgview/irotate.png", L"imgview/irotatehl.png", IBUTTON_ROTATE, 1, 0, 0, 0);
	x = buttonsPosXGet(btns, IBUTTON_NEXT);
	int y = buttonsPosYGet(btns, IBUTTON_NEXT) + buttonsHeightGet(btns, IBUTTON_NEXT) + 32;
	ccSetPosition(btn, x, y);
	buttonFaceAutoSwapEnable(btn, BUTTON_FACE_SWAP_SELECTED);
	
	iovr->slider = ccCreate(vp->cc, PAGE_IMGOVR, CC_SLIDER_HORIZONTAL, iOvrCcObject_cb, &vp->gui.ccIds[CCID_SLIDER_IMGOVR], fw - 64, 16);
	sliderFaceSet(iovr->slider, SLIDER_FACE_LEFT, L"imgview/slider_h_square_left.png");
	sliderFaceSet(iovr->slider, SLIDER_FACE_RIGHT,L"imgview/slider_h_square_right.png");
	sliderFaceSet(iovr->slider, SLIDER_FACE_MID,  L"imgview/slider_h_square_mid.png");
	sliderFaceSet(iovr->slider, SLIDER_FACE_TIP,  L"imgview/slider_h_square_tip.png");
	sliderFacesApply(iovr->slider);

	ccSetPosition(iovr->slider, 32, fh - ccGetHeight(iovr->slider) + 1);
	sliderSetRange(iovr->slider, 0, 10);
	sliderSetValue(iovr->slider, 0);
	ccEnable(iovr->slider);
	
	return (iovr->img != NULL);
}

static inline int page_imgOvrInitalize (TIOVR *iovr, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_IMGOVR);
	return 1;
}

static inline int page_imgOvrShutdown (TIOVR *iovr, TVLCPLAYER *vp)
{
	imgOvrNewImgSetSignal(iovr);
	if (imgOvrNewImgLock(iovr)){
		iovr->nextIsReady = 0;
		if (iovr->nextPath8)
			my_free(iovr->nextPath8);
		iovr->nextPath8 = NULL;
		imgOvrNewImgUnlock(iovr);
	}
	
	
	WaitForSingleObject((HANDLE)iovr->hImgOvrLoadThread, INFINITE);
	CloseHandle((HANDLE)iovr->hImgOvrLoadThread);
	iovr->hImgOvrLoadThread = 0;
	
	CloseHandle(iovr->hLoadEvent);
	iovr->hLoadEvent = NULL;
	lockClose(iovr->hLoadLock);
	iovr->hLoadLock = NULL;
	lockClose(iovr->hNewImgLock);
	iovr->hNewImgLock = NULL;
	
	buttonsDeleteAll(iovr->btns);
	ccDelete(iovr->slider);
	
	if (iovr->protectedImg)
		lDeleteFrame(iovr->protectedImg);
	if (iovr->img)
		lDeleteFrame(iovr->img);
	if (iovr->plc)
		playlistFree(iovr->plc);
	return 1;
}

static inline void page_imgOvrRenderBegin (TIOVR *iovr, TVLCPLAYER *vp, int64_t destId, int64_t data2, void *opaquePtr)
{
	//ccEnable(iovr->slider);
	
	if (iovr->threadState && iovr->hImgOvrLoadThread){
		iovr->threadState = 0;
		iovr->nextIsReady = 0;
		imgOvrNewImgSetSignal(iovr);
		WaitForSingleObject((HANDLE)iovr->hImgOvrLoadThread, INFINITE);
		CloseHandle((HANDLE)iovr->hImgOvrLoadThread);
		iovr->hImgOvrLoadThread = 0;
		iovr->loadLockThreadID = 0;
		
		if (iovr->protectedImg){
			lDeleteFrame(iovr->protectedImg);
			iovr->protectedImg = NULL;
		}
		if (iovr->img)
			lResizeFrame(iovr->img, 8, 8, 0);
		if (iovr->plc){
			playlistFree(iovr->plc);
			iovr->plc = NULL;
		}
	}
	
	iovr->hImgOvrLoadThread = _beginthreadex(NULL, THREADSTACKSIZE, imgOvrLoadThread, vp, 0, &iovr->loadLockThreadID);
}

static inline void page_imgOvrRenderEnd (TIOVR *iovr, TVLCPLAYER *vp, int64_t destId, int64_t data2, void *opaquePtr)
{
	imgPaneSetImageFocus(pageGetPtr(vp, PAGE_IMGPANE), iovr->currentImg);
	
	if (iovr->hImgOvrLoadThread){
		iovr->threadState = 0;
		iovr->nextIsReady = 0;
		imgOvrNewImgSetSignal(iovr);
		WaitForSingleObject((HANDLE)iovr->hImgOvrLoadThread, INFINITE);
		CloseHandle((HANDLE)iovr->hImgOvrLoadThread);
		iovr->hImgOvrLoadThread = 0;
		iovr->loadLockThreadID = 0;
		
		if (iovr->protectedImg){
			lDeleteFrame(iovr->protectedImg);
			iovr->protectedImg = NULL;
		}
		if (iovr->img)
			lResizeFrame(iovr->img, 8, 8, 0);
		if (iovr->plc){
			playlistFree(iovr->plc);
			iovr->plc = NULL;
		}
	}
}

int page_imgOvrCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TIOVR *iovr = (TIOVR*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_imgOvrCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_imgOvrRender(iovr, iovr->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		page_imgOvrRenderBegin(iovr, iovr->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		page_imgOvrRenderEnd(iovr, iovr->com->vp, dataInt1, dataInt2, opaquePtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_imgOvrInput(iovr, iovr->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_imgOvrStartup(iovr, iovr->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_imgOvrInitalize(iovr, iovr->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_imgOvrShutdown(iovr, iovr->com->vp);
		
	}
	
	return 1;
}


