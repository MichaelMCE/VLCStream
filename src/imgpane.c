
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


/*
imgpane config needs:
	image scale
	image cache read ahead
	input accel
	
*/



#define IMGPANE_FONT	LFTW_B24



static  wchar_t *extImage[] = {
	EXTIMAGE,
	L""
};






static inline int imgPane_Input (TIMGPANE *imgpane, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	/*
	switch(msg){
	  case PAGE_IN_TOUCH_DOWN:
	  case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  	break;
	}
		*/
	return 1;
}

void imgpaneDoView (TVLCPLAYER *vp, wchar_t *path)

{
	TIOVR *iovr = pageGetPtr(vp, PAGE_IMGOVR);
					
	int pos = iOvrBuildImageList(iovr, vp, path);
	if (pos >= 0){
		iovr->currentImg = pos;
		const TFRAME *front = getFrontBuffer(vp);
		char imagePath[MAX_PATH_UTF8+1];

		playlistGetPath(iovr->plc, pos, imagePath, MAX_PATH_UTF8);
		imageOverSetImage(iovr, front->width, front->height, imagePath);

		const int imgTotal = playlistGetTotal(iovr->plc);
		sliderSetRange(iovr->slider, 0, imgTotal-1);
		sliderSetValue(iovr->slider, iovr->currentImg);
						
		if (imgTotal > 2){
			ccEnable(iovr->slider);
			buttonsStateSet(iovr->btns, IBUTTON_PREV, 1);
			buttonsStateSet(iovr->btns, IBUTTON_NEXT, 1);
		}else if (imgTotal < 2){
			ccDisable(iovr->slider);
			buttonsStateSet(iovr->btns, IBUTTON_PREV, 0);
			buttonsStateSet(iovr->btns, IBUTTON_NEXT, 0);
		}else{	// == 2
			ccDisable(iovr->slider);
			buttonsStateSet(iovr->btns, IBUTTON_PREV, 1);
			buttonsStateSet(iovr->btns, IBUTTON_NEXT, 1);
		}
		
		page2Set(vp->pages, PAGE_IMGOVR, 1);
		//pageSetSec(vp, -1);
	}
}

static inline int64_t img_pane_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT || msg == CC_MSG_HOVER) return 1;
	
	TPANE *pane = (TPANE*)object;
	//printf("img_pane_cb in %p, %i %I64d %I64d %p (%i %i)\n", pane, msg, data1, data2, dataPtr, artId>>16, artId&0xFFFF);


	if (msg == PANE_MSG_ITEM_ENABLED){
		int objType = data2;
		if (objType == PANE_OBJ_IMAGE){
			const int itemId = data1;
			TIMGPANE *imgpane = ccGetUserData(pane);
			if (!imgpane->imgLoader) return 1;
			
			TMETRICS *metrics = (TMETRICS*)dataPtr;
			//printf("%i: %i %i %i %i\n", itemId, metrics->x, metrics->y, metrics->width, metrics->height);

			if (metrics->x >= 0){			// moving left to right
				int artId = labelArtcGet(pane->base, itemId+6);
				if (artId) imgLoaderAddImage(imgpane->imgLoader, artId);
				artId = labelArtcGet(pane->base, itemId+4);
				if (artId) imgLoaderAddImage(imgpane->imgLoader, artId);
				artId = labelArtcGet(pane->base, itemId+2);
				if (artId) imgLoaderAddImage(imgpane->imgLoader, artId);
			}else{							// moving right to left
				int artId = labelArtcGet(pane->base, itemId-6);
				if (artId) imgLoaderAddImage(imgpane->imgLoader, artId);
				artId = labelArtcGet(pane->base, itemId-4);
				if (artId) imgLoaderAddImage(imgpane->imgLoader, artId);
				artId = labelArtcGet(pane->base, itemId-2);
				if (artId) imgLoaderAddImage(imgpane->imgLoader, artId);
			}
		}
	}else if (msg == PANE_MSG_IMAGE_SELECTED || msg == PANE_MSG_TEXT_SELECTED){
		const int artId = data2;
		if (artId){
			TIMGPANE *imgpane = ccGetUserData(pane);
			wchar_t *path = artManagerImageGetPath(imgpane->am, artId);
			if (path){
				imgpaneDoView(pane->cc->vp, path);
				my_free(path);
			}
		}
	}
	
	return 1;
}

static inline int toggleNameState (TIMGPANE *imgpane)
{
	if (++imgpane->nameToggleState > 2)
		imgpane->nameToggleState = 0;
	return imgpane->nameToggleState;
}

static inline int imgPaneButtonPress (TIMGPANE *imgpane, TCCBUTTON *btn, const int id, const TTOUCHCOORD *pos)
{
	//TVLCPLAYER *vp = btn->cc->vp;
	imgpane->btns->t0 = getTickCount();


	switch (id){
	case IMGPANE_NAMETOGGLE:{
	  	int start = imgpane->pane->firstEnabledImgIdx;
	  	
	  	int x = 0;
	  	labelItemPositionGet(imgpane->pane->base, imgpane->pane->firstEnabledImgId, &x, NULL);
	  	imgPaneEmpty(imgpane);
		imgPaneAddPath(imgpane, imgpane->pane, imgpane->path, NULL, toggleNameState(imgpane), 0.35);
		imgPaneSetImageFocus(imgpane, start);
		
		imgpane->pane->offset.x += x;
		break;
	}
	case IMGPANE_BACK:
		page2SetPrevious(imgpane);
		break;
	}
	return 1;
}	  

static inline int64_t ccbtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;
		
	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//printf("ccbtn_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);

	TCCBUTTON *btn = (TCCBUTTON*)object;
	//const int id = (int)data2;

	if (msg == BUTTON_MSG_SELECTED_PRESS){
		return imgPaneButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	}
	return 1;
}

static inline int imgPane_Startup (TIMGPANE *imgpane, TVLCPLAYER *vp, const int width, const int height)
{
	imgpane->nameToggleState = 1;
	imgpane->am = vp->am;
	imgpane->btns = buttonsCreate(vp->cc, PAGE_IMGPANE, IMGPANE_TOTAL, ccbtn_cb);


	
	TCCBUTTON *btn = buttonsCreateButton(imgpane->btns, L"common/back_left96.png", NULL, IMGPANE_BACK, 1, 0, 0, 0);
	btn = buttonsCreateButton(imgpane->btns, L"common/nametoggle.png", L"common/nametogglehl.png", IMGPANE_NAMETOGGLE, 1, 0, 0, 0);
	ccSetPosition(btn, width-1 - ccGetWidth(btn), 0);
	buttonFaceAutoSwapEnable(btn, BUTTON_FACE_SWAP_SELECTED);	


	TPANE *pane = ccCreate(vp->cc, PAGE_IMGPANE, CC_PANE, img_pane_cb, NULL, width, height);
	imgpane->pane = pane;
	ccSetUserData(pane, imgpane);
	ccSetPosition(pane, 0, 0);
	paneSetLayout(pane, PANE_LAYOUT_HORICENTER);
	paneSetAcceleration(pane, 2.0, 1.0);
		
	return 1;
}

void imgPaneSetImageFocus (TIMGPANE *imgpane, const int idx)
{
	if (idx && idx < imgpane->itemIdListTotal)
		paneFocusSet(imgpane->pane, imgpane->itemIdList[idx]);
}

void imgPaneEmpty (TIMGPANE *imgpane)
{
	paneRemoveAll(imgpane->pane);
	imgpane->pane->offset.x = 0;
	imgpane->pane->offset.y = 0;
}

int imgPaneAddPath (TIMGPANE *imgpane, TPANE *pane, const wchar_t *path, const char *file, const int addTitle, const double scale)
{

	TFB *fb = fbNew();
	fbInit(fb, "imgPane");

	wchar_t buffer[MAX_PATH+1];
	char title[MAX_PATH_UTF8+1];
	if (!addTitle) strcpy(title, "   ");
	
	int depth = 0;
	fbFindFiles(fb, fb->rootId, path, L"*.*", &depth, extImage);
	fbGetTotals(fb, fb->rootId, NULL, &imgpane->itemIdListTotal);
	if (!imgpane->itemIdListTotal){
		fbRelease(fb);
		return 0;
	}
	
	fbSort(fb, fb->rootId, SORT_NAME_A);
	if (imgpane->itemIdList) my_free(imgpane->itemIdList);
	imgpane->itemIdList = my_calloc(imgpane->itemIdListTotal+1, sizeof(int));

	const int width = ccGetWidth(pane) * scale;
	const int height = ccGetHeight(pane) * scale;
	int noIconId = 0;
	int addCount = 0;
	int found = -1;
	
	TFBITEM *item = fbGetFirst(fb);
	while (item){
		if (fbIsLeaf(fb, item)){
			char *name8 = fbGetName(fb,item);
			wchar_t *name = converttow(name8);
			if (name){
				__mingw_snwprintf(buffer, MAX_PATH, L"%ls%ls", path, name);
				int imgId = artManagerImageAddEx(imgpane->am, buffer, width, height);
				if (!imgId || !artManagerImageGetMetrics(imgpane->am, imgId, NULL, NULL)){
					if (!noIconId){
						wchar_t imgpath[MAX_PATH+1];
						buildSkinD(pane->cc->vp, imgpath, L"common\\icons\\fileicon.png");
						noIconId = artManagerImageAddEx(imgpane->am, imgpath, ccGetWidth(pane)*scale, ccGetHeight(pane)*scale);
					}
					imgId = noIconId;
				}
				if (imgId){
					if (1/*artManagerImageGetMetrics(imgpane->am, imgId, NULL, NULL)*/){
						if (addTitle == 1)
							_splitpath(name8, NULL, NULL, title, NULL);
						else if (addTitle == 2)
							__mingw_snprintf(title, MAX_PATH_UTF8, "(%i)", addCount+1);
							
						if (file && found == -1){
							if (!strcmp(file, name8))
								found = addCount;
						}
						imgpane->itemIdList[addCount++] = paneTextAdd(pane, imgId, 0.0, title, IMGPANE_FONT, imgId)-1;
					}
				}
				my_free(name);
			}
		}
		item = fbGetNext(fb, item);
	}

	fbRelease(fb);

	//imgpane->nameToggleState = addTitle;

	if (addCount){
		wchar_t *newPath = my_wcsdup(path);
		if (imgpane->path) my_free(imgpane->path);
		imgpane->path = newPath;
	}

	if (file)
		return found;
	else
		return addCount;
}

static inline int imgPane_Initalize (TIMGPANE *imgpane, TVLCPLAYER *vp, const int width, const int height)
{
	return 1;
}

static inline int imgPane_Render (TIMGPANE *imgpane, TVLCPLAYER *vp, TFRAME *frame)
{
	setPageAccessed(vp, PAGE_IMGPANE);
	
	buttonsRenderAll(imgpane->btns, frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);
	ccRender(imgpane->pane, frame);
	return 1;
}

static inline int imgPane_Shutdown (TIMGPANE *imgpane, TVLCPLAYER *vp)
{
	if (imgpane->imgLoader)
		imgLoaderShutdown(imgpane->imgLoader);

	if (imgpane->itemIdList) my_free(imgpane->itemIdList);
	if (imgpane->path) my_free(imgpane->path);
	ccDelete(imgpane->pane);
	buttonsDeleteAll(imgpane->btns);
	return 1;
}

static inline int imgPane_RenderInit (TIMGPANE *imgpane, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	imgpane->am = vp->am;
	imgpane->imgLoader = imgLoaderNew(imgpane->am, 4);
	return 1;
}

int imgPane_Callback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TIMGPANE *imgpane = (TIMGPANE*)pageStruct;
	
	// printf("# page_Callback: %p %i %I64d %I64d %p %p\n", imgpane, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return imgPane_Render(imgpane, imgpane->com->vp, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_START){
		ccEnable(imgpane->pane);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		ccDisable(imgpane->pane);
		
	}else if (msg == PAGE_CTL_RENDER_INIT){
		return imgPane_RenderInit(imgpane, imgpane->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_INPUT){
		return imgPane_Input(imgpane, imgpane->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return imgPane_Startup(imgpane, imgpane->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return imgPane_Initalize(imgpane, imgpane->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return imgPane_Shutdown(imgpane, imgpane->com->vp);
		
	}
	
	return 1;
}
