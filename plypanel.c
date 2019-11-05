
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



#define IMGARTSIZE	(156.0)


static const char *extImage8[] = {
	EXTIMAGEA,
	""
};

static const char *extPlaylists8[] = {
	EXTPLAYLISTSA,
	""
};


enum _panelbtn {
	PANEL_BUTTON_1 = 0,
	PANEL_BUTTON_2,
	PANEL_BUTTON_3,
	PANEL_BUTTON_4,
/*	PANEL_BUTTON_5,
	PANEL_BUTTON_6,
	PANEL_BUTTON_7,*/
	PANEL_BUTTON_TOTAL
};

#define PANEL_BUTTON_BACK		PANEL_BUTTON_1
#define PANEL_BUTTON_UP			PANEL_BUTTON_2
#define PANEL_BUTTON_RELOAD		PANEL_BUTTON_3
#define PANEL_BUTTON_ROOT		PANEL_BUTTON_4


extern volatile int SHUTDOWN;





#if 0
void metaCbMsgCb (TVLCPLAYER *vp, const int msg, const int dataInt1, const int dataInt2, void *dataPtr1, void *dataPtr2)
{
	if (SHUTDOWN) return;
	if (!vp || !getApplState(vp)) return;
	
	//printf("metaCbMsgCb %i\n", msg);
	
	/*if (msg)*/ return;
	/*
	if (dataPtr1 == dataPtr2){
		if (pageGet(vp) == PAGE_PLY_PANEL)
			timerSet(vp, TIMER_PLYPAN_REBUILD, 30);
		else
			timerSet(vp, TIMER_PLYPAN_REBUILD, 2000);
		if (fireonce){
			fireonce = 0;
			timerSet(vp, TIMER_PLYPAN_REBUILDCLNMETA, 20000);
		}
	}*/
}


TMETACOMPLETIONCB *panelImgArtBuildCB (TVLCPLAYER *vp, TMetaCbMsg_t cb, void *dataPtr1, int dataInt, void *dataPtr2, TMETACOMPLETIONCB *mccb)
{
	memset(mccb, 0, sizeof(TMETACOMPLETIONCB));
	mccb->cb = cb;
	mccb->dataPtr1 = dataPtr1;
	mccb->dataInt1 = dataInt;
	mccb->dataPtr2 = dataPtr2;
	return mccb;
}
#endif

static inline int plypanButtonPress (TPLYPANEL *plypan, TCCBUTTON *btn, const int btn_id, const TTOUCHCOORD *pos)
{
	TPANEL *panel = plypan->panel;
	if (!panel) return 0;
	panel->btns->t0 = getTickCount();
		
	TVLCPLAYER *vp = panel->cc->vp;

	switch (btn_id){
	  case PANEL_BUTTON_BACK:
		page2SetPrevious(plypan);
		return 1;

	  case PANEL_BUTTON_UP:{
	  	int missing = 0;
	  	int idx = 0;
	  	
	  	TPANELIMG *item = panelImgGetFirst(panel, &idx);
	  	if (item){
			PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm,(intptr_t)panelImgStorageGet(panel, item->id));
			if (plc){			
				//printf("panelImgStorageGet(panel, item->id) %X %X %X %X\n", (int)panelImgStorageGet(panel, item->id), plypan->currentPlcUID, plc->parent->uid, getPrimaryPlaylist(vp)->uid);
				//if (plc->parent->uid == getPrimaryPlaylist(vp)->uid)
				//	return plypanButtonPress(plypan, btn, PANEL_BUTTON_ROOT, pos);
					
				PLAYLISTCACHE *parent = plc->parent;
				if (parent){
					setDisplayPlaylist(vp, parent);
					missing = plyPanelBuild(vp, panel, parent);
					invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), 0);
				}
			}
	  	}else{
	  		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, plypan->currentPlcUID);
	  		if (plc){
		  		PLAYLISTCACHE *parent = plc->parent;
	  			if (parent){
	  				setDisplayPlaylist(vp, parent);
	  				missing = plyPanelBuild(vp, panel, parent);
					invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), 0);
				}
			}
	  	}
	  	if (missing)
	  		timerSet(vp, TIMER_PLYPAN_REBUILD, 3000);
	    break;
	  }
	  case PANEL_BUTTON_RELOAD:
	  case PANEL_BUTTON_ROOT:{
	  	PLAYLISTCACHE *plc = getPrimaryPlaylist(vp);
	  	plc->pr->itemOffset.y = 0;
	  	
		plyPanelBuild(vp, panel, plc);
		invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), 0);
		break;
	  }
	}
	return 0;
}


static inline int64_t ccbtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;
		
	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//printf("ccbtn_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);

	TCCBUTTON *btn = (TCCBUTTON*)object;
	//const int id = (int)data2;

	if (msg == BUTTON_MSG_SELECTED_PRESS){
		//printf("ccbtn_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);
		return plypanButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	}
	return 1;
}

int64_t plypan_panel_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;
	
	TPANEL *panel = (TPANEL*)object;
	TVLCPLAYER *vp = panel->cc->vp;
	//printf("plypan_panel_cb in %p, %p, %i %I64d %I64d %p\n", panel, vp, msg, data1, data2, dataPtr);
	
	if (msg == PANEL_MSG_ITEMSELECTED){
		const int uid = (intptr_t)panelImgStorageGet(panel, data1);
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
		if (playlistManagerGetPlaylistIndex(vp->plm, plc) < 0){
			//printf("plypan_panel_cb out a %p, %p, %i %I64d %I64d %p\n", panel, vp, msg, data1, data2, dataPtr);
			return 1;
		}

		const intptr_t plcItem = (intptr_t)dataPtr;
		if (playlistGetItemType(plc, plcItem) == PLAYLIST_OBJTYPE_PLC){
			//printf("plypan_panel_cb %I64d %I64d %p\n", data1, data2, dataPtr);
			
			TPLAYLISTITEM *item = playlistGetItem(plc, plcItem);
			if (item){
				invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), 0);
				setDisplayPlaylist(panel->cc->vp, item->obj.plc);
				
				//printf("plypan_panel_cb Build '%s' %i\n", item->obj.plc->title, item->obj.plc->uid);
				
				if (plyPanelBuild(vp, panel, item->obj.plc))
					timerSet(vp, TIMER_PLYPAN_REBUILD, 2000);
			}
		}else if (playlistGetItemType(plc, plcItem) == PLAYLIST_OBJTYPE_TRACK){
			if (getPlayState(vp) && isPlayingItem(vp, plc, plcItem)){
				timerSet(vp, TIMER_STOP, 0);
			}else{
				TTIMERPLAYTRACK *tpt = &vp->gui.playtrack;
				tpt->uid = uid;
				tpt->track = plcItem;
				timerSet(vp, TIMER_PLAYTRACK, 0);
			}
		}
	}
	
	//printf("plypan_panel_cb out b %p, %p, %i %I64d %I64d %p\n", panel, vp, msg, data1, data2, dataPtr);
	return 1;
}

static inline int _plyPanelBuild (TVLCPLAYER *vp, TPANEL *panel, PLAYLISTCACHE *plc)
{
	
#if 0
	static double lastBuildTime = 0.0;
	double t1 = getTime(vp);
	if (t1 - lastBuildTime < 2) return 1;
#endif	

	TPLYPANEL *plypan = (TPLYPANEL*)ccGetUserData(panel);

	//printf("_plyPanelBuild %i %X '%s'\n", plypan->imgArtSize, plc->artId, plc->title);

	plypan->currentPlcUID = plc->uid;

	//const double scale = plypan->imgArtSize/(double)vp->gui.artMaxHeight;	
	int missingArtCt = 0;
	const int hoverCol = swatchGetColour(vp, PAGE_NONE, SWH_PLY_ISELECTED);	
	char path[MAX_PATH_UTF8+1];
	const int total = playlistGetTotal(plc);

	for (intptr_t i = 0; i < total; i++){
		if (playlistGetItemType(plc, i) == PLAYLIST_OBJTYPE_PLC){
			TPLAYLISTITEM *item = playlistGetItem(plc, i);
			if (!item) continue;

			PLAYLISTCACHE *plcChild = item->obj.plc;
			if (plcChild){
				if (playlistLock(plcChild)){
					int artId = plcChild->artId;
					if (!artId){
						getItemPlaylistImage(vp, plcChild, &artId);
						if (!artId){
							artId = plcChild->artId;
							if (!artId){
								//TMETACOMPLETIONCB mccb;
								//panelImgArtBuildCB(vp, metaCbMsgCb, (void*)plc->uid, i, (void*)plypan->currentPlcUID, &mccb);
								
								initiateAlbumArtRetrievalEx(vp, plc, i, i, vp->gui.artSearchDepth,NULL/* &mccb*/);
								playlistMetaGetMeta(vp, plc, i, i, NULL/*&mccb*/);
								initiateAlbumArtRetrievalEx(vp, plcChild, 0, 4, vp->gui.artSearchDepth, NULL/*&mccb*/);
								//playlistMetaGetMeta(vp, plcChild, 0, 4, &mccb);
							}
						}
					}
					
					if (artId){
						if (!artManagerImageGetMetrics(vp->am, artId, NULL, NULL))
							artId = 0;
					}
					
					if (artId){
						if (!plc->artId) plc->artId = artId;
						if (!plcChild->artId) plcChild->artId = artId;
					}

					char *title;
					char bufferTitle[MAX_PATH_UTF8+1];
					tagRetrieveByHash(vp->tagc, item->obj.track.hash, MTAG_Title, bufferTitle, MAX_PATH_UTF8);
					if (!*bufferTitle)
						title = plcChild->title;
					else
						title = bufferTitle;


					int id;
					if (!artId){
						missingArtCt++;
						artId = plypan->iconId[PANEL_ICON_FOLDER];
						id = panelImgAddEx(panel, artId, 0, 1.0, title, (void*)i);
					}else{
						double scale = plypan->imgArtSize / (double)vp->gui.artMaxHeight;
						id = panelImgAddEx(panel, artId, 0, scale, title, (void*)i);
					}
						

					char buffer[64];
					const int total = playlistGetTotal(plcChild);
					__mingw_snprintf(buffer, sizeof(buffer)-1, "(%i)", total);

					panelImgAddSubtext(panel, id, buffer, 240<<24|COL_YELLOW);
					panelImgStorageSet(panel, id, (void*)(intptr_t)plc->uid);
					panelImgAttributeSet(panel, id, PANEL_ITEM_GENERAL);

					if (artId){
						TPANELIMG *pitem = panelImgGetItem(panel, id);
						if (pitem){
							button2AnimateSet(pitem->btn, 0);
							button2FaceConstraintsSet(pitem->btn, panel->metrics.x, panel->metrics.y, panel->metrics.x + panel->metrics.width-1, panel->metrics.y + panel->metrics.height-1);
							button2FaceConstraintsEnable(pitem->btn);
							button2FaceAutoSwapDisable(pitem->btn);
							button2FaceHoverSet(pitem->btn, 1, hoverCol, 0.8);
							ccEnable(pitem->btn);

							if (plcChild == getQueuedPlaylist(vp)){
								//const unsigned int colour[] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF};
								//buttonBorderProfileSet(pitem->btn, BUTTON_BORDER_POST, colour, 3);
								button2BorderEnable(pitem->btn);
							}
						}
					}else{
						missingArtCt++;
					}
					playlistUnlock(plcChild);
				}
			}
		}else if (playlistGetItemType(plc, i) == PLAYLIST_OBJTYPE_TRACK){
			char name[MAX_PATH_UTF8+1];
			
			tagRetrieveByHash(vp->tagc, playlistGetHash(plc, i), MTAG_Title, name, MAX_PATH_UTF8);
			if (!name[0]){
				char *title = playlistGetTitleDup(plc, i);
				if (title){
					//__mingw_snprintf(name, MAX_PATH_UTF8, "%s", title);
					strncpy(name, title, MAX_PATH_UTF8);
					my_free(title);
				}else{
					__mingw_snprintf(name, MAX_PATH_UTF8, "#%i", i);
				}
			}

			if (*name){
				int id = -1;
				int artId = playlistGetArtId(plc, i);

				if (!artManagerImageGetMetrics(vp->am, artId, NULL, NULL))
					artId = 0;

				double scale;
				if (artId){
					//printf("@@@@ plypanel: b found art for %X, %f\n", artId, scale);
					scale = plypan->imgArtSize / (double)vp->gui.artMaxHeight;
					id = panelImgAddEx(panel, artId, 0, scale, name, (void*)i);
				}else{
					missingArtCt++;
					playlistGetPath(plc, i, path, MAX_PATH_UTF8);	

					
					if (isMediaVideo8(path))
						id = panelImgAdd(panel, plypan->iconId[PANEL_ICON_VIDEO], name, (void*)i);
					else if (hasPathExtA(path, extImage8))
						id = panelImgAdd(panel, plypan->iconId[PANEL_ICON_IMAGE], name, (void*)i);
					else if (hasPathExtA(path, extPlaylists8))
						id = panelImgAdd(panel, plypan->iconId[PANEL_ICON_PLAYLIST], name, (void*)i);
					else
						id = panelImgAdd(panel, plypan->iconId[PANEL_ICON_AUDIO], name, (void*)i);
					scale = 1.0;
				}
					
				char bufferLen[64];
				char buffer[64];
				tagRetrieveByHash(vp->tagc, playlistGetHash(plc, i), MTAG_LENGTH, bufferLen, sizeof(bufferLen));
				if (!bufferLen[0] || !bufferLen[1]) strcpy(bufferLen, "0:00");
				__mingw_snprintf(buffer, 64, "%s", bufferLen);
					
				panelImgAddSubtext(panel, id, buffer, 240<<24|COL_WHITE);
				panelImgStorageSet(panel, id, (void*)(intptr_t)plc->uid);
				panelImgAttributeSet(panel, id, PANEL_ITEM_GENERAL);

				if (artId){
					TPANELIMG *pitem = panelImgGetItem(panel, id);
					if (pitem){
						button2AnimateSet(pitem->btn, 1);
						button2FaceConstraintsSet(pitem->btn, panel->metrics.x, panel->metrics.y, panel->metrics.x + panel->metrics.width-1, panel->metrics.y + panel->metrics.height-1);
						button2FaceConstraintsEnable(pitem->btn);
						button2FaceAutoSwapDisable(pitem->btn);
						button2FaceHoverSet(pitem->btn, 1, hoverCol, 0.8);
						ccEnable(pitem->btn);

						// draw a band around image to indicate selection
						if (isPlayingItem(vp, plc, i)){
							const unsigned int colour[] = {80<<24 | COL_WHITE,
														   255<<24 | COL_BLUE_SEA_TINT,
														   180<<24 | COL_BLUE_SEA_TINT,
														   100<<24 | COL_BLUE_SEA_TINT,
														    85<<24 | COL_BLUE_SEA_TINT,
														    40<<24 | COL_BLUE_SEA_TINT};
							button2BorderProfileSet(pitem->btn, BUTTON_BORDER_POST, colour, 6);
							button2BorderEnable(pitem->btn);
						}
					}
				}else{
					TPANELIMG *pitem = panelImgGetItem(panel, id);
					if (pitem)
						button2AnimateSet(pitem->btn, 1);
					//TMETACOMPLETIONCB mccb;
					//panelImgArtBuildCB(vp, metaCbMsgCb, (void*)plc->uid, i, (void*)plypan->currentPlcUID, &mccb);
					playlistMetaGetMeta(vp, plc, i, i,NULL/* &mccb*/);
				}
			}
		}
	}

	TMETRICS metrics = {0, 0, panel->metrics.width, panel->metrics.height};	
	panel->vHeight = panelImgPositionMetricsCalc(panel->list, panel->listSize, &metrics, panel->itemHoriSpace, panel->itemVertSpace);
	//lastBuildTime = getTime(vp);
	return missingArtCt;
}

int plyPanelBuild (TVLCPLAYER *vp, TPANEL *panel, PLAYLISTCACHE *plc)
{
	int ret = 0;
	//static int ct = 0;

	if (playlistManagerGetPlaylistIndex(vp->plm, plc) < 0){
		//printf("plyPanel invalid for %p\n", plc);
		return -1;	
	}

	//printf("plyPanel built for '%s' (%i)\n", plc->title, plc->uid);

	if (ccLock(panel)){
		if (playlistLock(plc)){
			const int total = playlistGetTotal(plc);
			if (total){
				//lSetCharacterEncoding(vp->ml->hw, CMT_UTF8);

				panelListResize(panel, total, 0);
				panel->itemOffset = &plc->pr->itemOffset;
				ret = _plyPanelBuild(vp, panel, plc);
				panelInvalidate(panel);

				if (plc == getPrimaryPlaylist(vp)){
					buttonsStateSet(panel->btns, PANEL_BUTTON_UP, 0);
					buttonsStateSet(panel->btns, PANEL_BUTTON_RELOAD, 1);
				}else{
					buttonsStateSet(panel->btns, PANEL_BUTTON_UP, 1);
					buttonsStateSet(panel->btns, PANEL_BUTTON_RELOAD, 0);
				}
			}else{
				if (pageGet(vp) == PAGE_PLY_PANEL)
					dbprintf(vp, "'%s' is empty", plc->title);
			}
			playlistUnlock(plc);
		}
		ccUnlock(panel);
	}
	return ret;
}

// TIMER_PLYPAN_REBUILD
void plyPanRebuild (TVLCPLAYER *vp)
{
	//printf("\n\n########################## plyPanRebuild ########\n");
	
	TPLYPANEL *plypan = pageGetPtr(vp, PAGE_PLY_PANEL);
	
	if (ccLock(plypan->panel)){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, plypan->currentPlcUID);
		if (!plc) plc = getPrimaryPlaylist(vp);
		
		if (plc){
			const int y = plypan->panel->itemOffset->y;
			//panelListResize(plypan->panel, 1, 0);
			//printf("plyPanelRebuild\n");
			plyPanelBuild(vp, plypan->panel, plc);
			plypan->panel->itemOffset->y = y;
			
			renderSignalUpdate(vp);
		}
		ccUnlock(plypan->panel);
	}
}


// TIMER_PLYPAN_REBUILDCLNMETA
void plyPanRebuildCleanMeta (TVLCPLAYER *vp)
{
	//printf("plyPanRebuildCleanMeta\n");
	vlcEventListInvalidate(vp->vlc);
	vlcEventsCleanup(vp);
	//if (vp->jt)
	//	artQueueFlush(artThreadGetWork(vp->jt));
	//artworkFlush(vp, 0);
	//if (vp->jt)
	//	artQueueFlush(artThreadGetWork(vp->jt));
	
	plyPanRebuild(vp);
}

void plyPanSetCurrentUID (TVLCPLAYER *vp, const int uid)
{
	TPLYPANEL *plypan = pageGetPtr(vp, PAGE_PLY_PANEL);
	if (ccLock(plypan->panel)){
		plypan->currentPlcUID = uid;
		ccUnlock(plypan->panel);
	}
}

void plyRenderTitle (TVLCPLAYER *vp, PLAYLISTCACHE *plc, TFRAME *frame)
{
	int x = 0;
	int y = 8;
	const int font = LFTW_B28;
	const int colour = 241<<24|COL_WHITE;
	lSetForegroundColour(frame->hw, colour);

	char buffer[MAX_PATH_UTF8+1];
	playlistGetName(plc, buffer, MAX_PATH_UTF8);
	if (!*buffer) return;

	unsigned int shash = colour^generateHash(buffer, strlen(buffer)*sizeof(char));
	TFRAME *str = strcFindString(vp->strc, shash);
	if (!str){
		TMETRICS metrics;
		metrics.x = 1;
		metrics.y = 0;
		metrics.width = 0;
		metrics.height = 0;
		
		str = newStringEx(frame->hw, &metrics, frame->bpp, 0, font, (char*)buffer, frame->width, NSEX_RIGHT);
		if (str) strcAddString(vp->strc, str, shash);
	}
	if (str){
		x = (frame->width - str->width)/2;
		drawShadowedImage(str, frame, x, y, 0, 2, 1, 0);
	}
}

static inline int page_plypanRender (TPLYPANEL *plypan, TVLCPLAYER *vp, TFRAME *frame)
{

//	printf("%i\n", plypan->panel->itemOffset->y);

	ccRender(plypan->panel, frame);

	TVIDEOOVERLAY *playctrl = pageGetPtr(vp, PAGE_OVERLAY);
	const int wasDrawn = marqueeDraw(vp, frame, playctrl->marquee, 2, 8);
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, plypan->currentPlcUID);
	
	if (!wasDrawn && plc && plc != getPrimaryPlaylist(vp))
		plyRenderTitle(vp, plc, frame);

	return 1;
}

static inline int page_plypanInput (TPLYPANEL *plypan, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
	  case PAGE_IN_WHEEL_BACK:{
	  	TTOUCHSWIPE *swipe = &plypan->panel->swipe;
	  	swipeReset(swipe);
	  	
	  	swipe->t0 = getTime(vp)-100.0;
	  	swipe->i32value = plypan->panel->itemOffset->y;
	  	swipe->sx = 10;
	  	swipe->sy = 150;
	  	swipe->ex = 10;
		swipe->ey = swipe->sy-25;
		swipe->dx = swipe->ex - swipe->sx;
		if (msg == PAGE_IN_WHEEL_FORWARD)
			swipe->dy = -(swipe->ey - swipe->sy);
		else
			swipe->dy = swipe->ey - swipe->sy;
		
		swipe->state = 0;
	  	swipe->adjust = swipe->dy;
	  	swipe->dt = 100.0;
		swipe->decayAdjust = abs(swipe->dy)/(double)swipe->dt;
		swipe->decayAdjust *= swipe->velocityFactor;
	  	swipe->state = 0;

	  	break;
	  }
	  /*case PAGE_IN_TOUCH_DOWN:
	  case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  	break;*/
	}
		
	return 1;
}

static inline int page_plypanStartup (TPLYPANEL *plypan, TVLCPLAYER *vp, const int width, const int height)
{
	
	TPANEL *panel = ccCreate(vp->cc, PAGE_PLY_PANEL, CC_PANEL, plypan_panel_cb, &vp->gui.ccIds[CCID_PANEL_PLAYLIST], PANEL_BUTTON_TOTAL, 0);
	plypan->panel = panel;
	plypan->currentPlcUID = 0;
	plypan->imgArtSize = IMGARTSIZE;
	
	panel->itemOffset = &plypan->itemOffset;
	panel->itemOffset->x = 0;
	panel->itemOffset->y = 0;
	panel->itemMaxWidth = width * 0.30;
	panel->font = PLAYLIST_PANEL_FONT;
	
	ccSetUserData(panel, plypan);
	int y = (height * 0.122)+1;
	
	panelSetBoundarySpace(panel, 10, 24);
	ccSetMetrics(panel, 0, y, width, height - y);
	ccEnable(panel);
	
	TCCBUTTON *btn = panelSetButton(panel, PANEL_BUTTON_BACK, L"common/back_right96.png", NULL, ccbtn_cb);
	ccSetPosition(btn, width - ccGetWidth(btn)-1, 0);

	btn = panelSetButton(panel, PANEL_BUTTON_UP, L"common/back_left96.png", NULL, ccbtn_cb);
	btn->flags.canAnimate = 1;
	ccSetPosition(btn, 1, 0);
		
	btn = panelSetButton(panel, PANEL_BUTTON_ROOT, L"plypanel/root.png", NULL, ccbtn_cb);
	ccSetPosition(btn, (width - ccGetWidth(btn))/2, 0);
	btn->flags.disableRender = 1;

	btn = panelSetButton(panel, PANEL_BUTTON_RELOAD, L"plypanel/refresh.png", NULL, ccbtn_cb);
	btn->flags.canAnimate = 1;
	ccSetPosition(btn, (width - ccGetWidth(btn))/2, 0);
	
	return 1;
}

static inline int page_plypanInitalize (TPLYPANEL *plypan, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_PLY_PANEL);
	
	TPANEL *panel = plypan->panel;
	plypan->iconId[PANEL_ICON_AUDIO] = genAMId(panel, L"plypanel/audio.png");
	plypan->iconId[PANEL_ICON_FOLDER] = genAMId(panel, L"plypanel/folder.png");
	plypan->iconId[PANEL_ICON_VIDEO] = genAMId(panel, L"plypanel/video.png");
	plypan->iconId[PANEL_ICON_IMAGE] = genAMId(panel, L"plypanel/image.png");
	plypan->iconId[PANEL_ICON_PLAYLIST] = genAMId(panel, L"plypanel/playlist.png");


	settingsGet(vp, "artwork.panelImgSize", &plypan->imgArtSize);
	if (plypan->imgArtSize < 10) plypan->imgArtSize = 110;
	
	return 1;
}

static inline int page_plypanShutdown (TPLYPANEL *plypan, TVLCPLAYER *vp)
{
	ccDelete(plypan->panel);
	return 1;
}


static inline int page_plypanRenderBegin (TPLYPANEL *plypan, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	lSetCharacterEncoding(frame->hw, CMT_UTF8);
		
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	if (!plc) plc = getPrimaryPlaylist(vp);
	
	plypan->currentPlcUID = plc->uid;
	plyPanelBuild(vp, plypan->panel, plc);

	return 1;
}

static inline void page_plypanRenderEnd (TPLYPANEL *plypan, TVLCPLAYER *vp, int64_t destId, int64_t data2, void *opaquePtr)
{
	artManagerFlush(vp->am);
}

int page_plyPanCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TPLYPANEL *plypan = (TPLYPANEL*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_plyPanCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_plypanRender(plypan, plypan->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		return page_plypanRenderBegin(plypan, plypan->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		page_plypanRenderEnd(plypan, plypan->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_plypanInput(plypan, plypan->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_plypanStartup(plypan, plypan->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_plypanInitalize(plypan, plypan->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_plypanShutdown(plypan, plypan->com->vp);
		
	}
	
	return 1;
}
