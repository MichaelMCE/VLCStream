
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







static inline void plyPlaylistPaneSetPosition (PLAYLISTCACHE *plc, const int x, const int y)
{
	if (plc){
		plc->pr->pane.x = x;
		plc->pr->pane.y = y;
	}
}

static inline void plyPlaylistPaneGetPosition (PLAYLISTCACHE *plc, int *x, int *y)
{
	if (plc){
		*x = plc->pr->pane.x;
		*y = plc->pr->pane.y;
	}
}

int plypaneGetPaneUID (TPLYPANE *plypane)
{
	intptr_t uid = 0;
	stackPeek(plypane->uidStack, &uid);	// current playlist will always be topmost
	return (int)uid;
}

int plypaneSetPaneUID (TPLYPANE *plypane, const int uid)
{
	if (plypaneGetPaneUID(plypane) != uid){
		stackPush(plypane->uidStack, uid);
		return 0;	
	}
	return 0;
}

void plypaneUpdateTimestamp (TPLYPANE *plypane)
{
	TVLCPLAYER *vp = plypane->com->vp;

	if (getPlayState(vp)){
		char str[132];
		char buffer[2][64];
	
		const double position = vp->vlc->position;
	
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
		
		labelStringSet(plypane->title, plypane->timestampItemId, str);
	}else{
		labelStringSet(plypane->title, plypane->timestampItemId, " ");
	}
}


static inline void buildTitleBar (TPLYPANE *plypane, TLABEL *title, const int uid)
{
	char buffer[MAX_PATH_UTF8+1];
	
	if (uid != getRootPlaylistUID(title->cc->vp)){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(title->cc->vp->plm, uid);
		if (plc)
			playlistGetName(plc, buffer, sizeof(buffer));
		else
			strcpy(buffer, "---");	
	}else{
		strcpy(buffer, "\\\\");
	}
	
	int itemId = ccGetUserDataInt(title);
	if (!itemId){
		itemId = labelTextCreate(title, buffer, 0, PANE_TITLE_FONT, 0, 0);
		ccSetUserDataInt(title, itemId);
		labelRenderFilterSet(title, itemId, 0);
		labelStringRenderFlagsSet(title, itemId, PF_MIDDLEJUSTIFY);
		labelRenderColourSet(title, itemId, 250<<24 | COL_WHITE, 200<<24 | COL_BLUE_SEA_TINT, 140<<24 | COL_BLACK);
	}else{
	 	labelStringSet(title, itemId, buffer);
	}
	
	
	if (!plypane->timestampItemId){
		int itemId = labelTextCreate(title, "0:00/0:00", 0, PANE_TIMESTAMP_FONT, 0, 3);
	 	labelRenderFilterSet(title, itemId, 0);
		labelStringRenderFlagsSet(title, itemId, PF_LEFTJUSTIFY);
		labelRenderColourSet(title, itemId, 255<<24 | COL_WHITE, 200<<24 | COL_BLUE_SEA_TINT, (100<<24) | COL_BLUE_SEA_TINT);
		plypane->timestampItemId = itemId;
	}else{
		plypaneUpdateTimestamp(plypane);
	}
	
	labelItemDataSet(title, itemId, uid);
}

static inline void buildLocBar (TPLYPANE *plypane, TLABEL *bar)
{
	labelItemsDelete(bar);
	
	int length;
	int *stack = stackCopyInt32(plypane->uidStack, &length);
	if (!stack || !length) return;


	char buffer[MAX_PATH_UTF8+1];
	const int rootUid = getRootPlaylistUID(bar->cc->vp);
	#define sepSingle "\\"
	#define sepDouble "\\\\"
	int x = 0;
	int y = -1;
	
			
	for (int i = 0; i < length-1; i++){
		//printf("bar %i %X\n", i, stack[i]);
			
		char *sep;
		if (stack[i] != rootUid){
			PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(bar->cc->vp->plm, stack[i]);
			if (!plc) continue;
			
			playlistGetName(plc, buffer, sizeof(buffer));
			sep = (char*)sepSingle;
		}else{
			strcpy(buffer, " ");
			if (length-1 == 1)
				sep = sepDouble"        ";
			else
				sep = sepDouble;
		}
			
		if (!*buffer) break;

		int width;
		int itemId = labelTextCreate(bar, buffer, 0, PANE_LOCBAR_FONT, 0, 0);
		labelRenderFilterSet(bar, itemId, 0);
		labelStringRenderFlagsSet(bar, itemId, PF_LEFTJUSTIFY);
		labelItemDataSet(bar, itemId, stack[i]);
		labelRenderColourSet(bar, itemId, 255<<24 | COL_WHITE, 255<<24 | COL_BLUE_SEA_TINT, (177<<24) | COL_BLUE_SEA_TINT);

		labelStringGetMetrics(bar, itemId, NULL, NULL, &width, NULL);
		labelItemPositionSet(bar, itemId, x, y);
		x += width;

		
		itemId = labelTextCreate(bar, sep, 0, PANE_LOCBAR_FONT, 0, 0);
		labelRenderFilterSet(bar, itemId, 0);
		labelStringRenderFlagsSet(bar, itemId, PF_LEFTJUSTIFY);
		labelItemDataSet(bar, itemId, stack[i]);
		labelRenderColourSet(bar, itemId, 255<<24 | COL_WHITE, 255<<24 | COL_BLUE_SEA_TINT, (177<<24) | COL_YELLOW);
		
		labelStringGetMetrics(bar, itemId, NULL, NULL, &width, NULL);
		labelItemPositionSet(bar, itemId, x, y-1);
		x += width;
	}
	my_free(stack);
	
}

static inline int page_plypaneRender (TPLYPANE *plypane, TVLCPLAYER *vp, TFRAME *frame)
{

//	printf("%i\n", plypan->pane->itemOffset->y);

	ccRender(plypane->locBar, frame);
	ccRender(plypane->title, frame);
	ccRender(plypane->pane, frame);
	
	return 1;
}

static inline int page_plypaneInput (TPLYPANE *plypane, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	//printf("page_plypaneInput %i %i %i %i\n", pos->x, pos->y, flags, pos->pen);

	
	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
		if (ccGetState(plypane->pane))
			paneScroll(plypane->pane, PLYPANE_SCROLL_DELTA);
		break;
		
	  case PAGE_IN_WHEEL_BACK:
		if (ccGetState(plypane->pane))
			paneScroll(plypane->pane, -PLYPANE_SCROLL_DELTA);
		break;
	}

	return 1;
}

static inline int plyPaneAddPlaylist (TPLYPANE *plypane, TPANE *pane, TPLAYLISTMANAGER *plm, const int pid)
{
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(plm, pid);
	if (!plc) return 0;
	
	//printf("### plyPaneAddPlaylist %X ###\n", pid);
	
	if (playlistLock(plc)){
		TVLCPLAYER *vp = pane->cc->vp;

		
		plyPlaylistPaneGetPosition(plc, &pane->offset.x, &pane->offset.y);
			
		// set view type
		// horizontal if there is a further playlist level, otherwise vertical when tracks' only
		const int total = playlistGetTotal(plc);
		int hasTracks = 0;
		for (int i = 0; !hasTracks && i < total; i++)
			hasTracks = (playlistGetItemType(plc, i) == PLAYLIST_OBJTYPE_TRACK);

		
		// set layout type and artwork image
		if (hasTracks){
			paneTextMulityLineEnable(pane);
			paneSetLayout(pane, PANE_LAYOUT_VERT);
			
			int artId = plc->artId;
			if (pid == getQueuedPlaylistUID(vp)){
				int id = playlistGetArtId(plc, getPlayingItem(vp));
				if (id > 0) artId = id;
			}

			if (!artId)
				getItemPlaylistImage(vp, plc, &artId);

			if (artId){
				paneImageAdd(pane, artId, 0.9, PANE_IMAGE_EAST, 0, 0, pid<<16);
			}else{
				artId = plypane->icons.art;
				paneTextMulityLineDisable(pane);
				paneSetLayout(pane, PANE_LAYOUT_HORI);
				paneImageAdd(pane, artId, 0.0, PANE_IMAGE_EAST, 0, 0, pid<<16);
			}
		}else{
			paneTextMulityLineDisable(pane);
			paneSetLayout(pane, PANE_LAYOUT_HORI);
		}
		
#if 0
		if (plc->parent){
			int64_t id = ((int64_t)1<<32) | (plc->parent->uid<<16);
			paneTextAdd(pane, plypane->icons.back, 0.0, " ", PANE_FONT, id);
		}
#endif

		// ensure folders are they're topmost, add first
		for (int i = 0; i < total; i++){
			if (playlistGetItemType(plc, i) == PLAYLIST_OBJTYPE_PLC){
				char *title = playlistGetTitleDup(plc, i);
				if (title){
					if (!playlistGetArtId(plc, i)){
						initiateAlbumArtRetrieval(vp, plc, i, i, vp->gui.artSearchDepth);
						playlistMetaGetMeta(vp, plc, i, i, NULL);
						
						TPLAYLISTITEM *item = playlistGetItem(plc, i);
						if (item->obj.plc)
							initiateAlbumArtRetrieval(vp, item->obj.plc, 0, 4, vp->gui.artSearchDepth);
					}
					
					int uid = playlistGetPlaylistUID(plc, i);
#if 0
					int artId = playlistGetArtId(plc, i);
					if (artId)
						paneTextAdd(pane, artId, 0.07, title, PANE_FONT, uid<<16);
					else
#endif
						paneTextAdd(pane, plypane->icons.folder, 0.0, title, PANE_FONT, uid<<16);
					
					
					my_free(title);
				}
			}
		}

		if (hasTracks){
			char buffer[MAX_PATH_UTF8+1];
			int playingItem = -1;
			int playingItemId = 0;
			
			if (pid == getQueuedPlaylistUID(vp))
				playingItem = getPlayingItem(vp);

			for (int i = 0; i < total; i++){
				if (playlistGetItemType(plc, i) == PLAYLIST_OBJTYPE_TRACK){
					unsigned int hash = playlistGetHash(plc, i);
					tagRetrieveByHash(vp->tagc, hash, MTAG_Title, buffer, MAX_PATH_UTF8);
					if (!*buffer)
						playlistGetTitle(plc, i, buffer, MAX_PATH_UTF8);

					if (*buffer){
						//int artId = playlistGetArtId(plc, i);
						//int itemId = paneTextAdd(pane, artId, 0.07, buffer, PANE_FONT, (plc->uid<<16) | (i+1));
						int itemId = paneTextAdd(pane, plypane->icons.audio, 0.0, buffer, PANE_FONT, (plc->uid<<16) | (i+1));
						
						if (i == playingItem) playingItemId = itemId;
					}
				}
			}
				
			if (playingItemId /*&& getPlayState(vp)*/){
				plypane->playingItemId = playingItemId;
				labelArtcSet(pane->base, playingItemId-1, plypane->icons.play, 0);
			}
		}
			
		if (plc->parent){
			int64_t id = ((int64_t)1<<32) | (plc->parent->uid<<16);
			paneTextAdd(pane, plypane->icons.back, 0.0, "   ", PANE_FONT, id);
		}

#if 1
		int64_t id = ((int64_t)1<<33)/* | (plc->parent->uid<<16)*/;
		paneTextAdd(pane, plypane->icons.closePane, 0.0, "       ", PANE_FONT, id);
#endif

		plypaneSetPaneUID(plypane, pid);
		playlistUnlock(plc);
	}
	
	return 1;
}

int plyPaneRefresh (TVLCPLAYER *vp)
{
	
	if (!page2IsInitialized(vp->pages, PAGE_PLY_PANE))
		return 0;
		
	TPLYPANE *plypane = pageGetPtr(vp, PAGE_PLY_PANE);
	TPANE *pane = plypane->pane;


	//printf("plyPaneRefresh %X\n", plypaneGetPaneUID(plypane));

	intptr_t uid = 0;
	
	if (ccLock(pane)){
		paneRemoveAll(pane);
			
		uid = plypaneGetPaneUID(plypane);
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
		if (plc){
			plyPlaylistPaneSetPosition(plc, pane->offset.x, pane->offset.y);
			plyPaneAddPlaylist(plypane, pane, vp->plm, uid);
			buildLocBar(plypane, plypane->locBar);
			buildTitleBar(plypane, plypane->title, uid);
		}
				
		ccUnlock(pane);
	}
	
	return uid;
}

// TIMER_PLYPANE_REFRESH
void timer_plyPaneRefresh (TVLCPLAYER *vp)
{
	//printf("@@@ timer_plyPaneRefresh\n");
	
	if (pageIsDisplayed(vp, PAGE_PLY_PANE)){
		plyPaneRefresh(vp);
		renderSignalUpdate(vp);
	}
}

int64_t plypane_titlebar_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT || msg == CC_MSG_HOVER) return 1;
	
	TLABEL *title = (TLABEL*)object;
	//printf("plypane_titlebar_cb in %p, %i %I64d %I64d %p\n", bar, msg, data1, data2, dataPtr);
	
	if (msg == LABEL_MSG_TEXT_SELECTED_PRESS){
		TPLYPANE *plypane = ccGetUserData(title);	
		const int uid = labelItemDataGet(title, data2);
		if (uid)
			plyPaneRefresh(plypane->com->vp);
	}

	return 1;
}

static inline void buildPlaylistRoute (TSTACK *stack, PLAYLISTCACHE *plc)
{
	if (plc){
		if (plc->parent)
			buildPlaylistRoute(stack, plc->parent);

		stackPush(stack, plc->uid);
	}
}

static inline void plypaneStackEmpty (TPLYPANE *plypane)
{
	while (stackPop(plypane->uidStack, NULL)){
	};
}

static inline void plypaneStackRewind (TPLYPANE *plypane, const int until)
{
	intptr_t data = 0;
	
	while (stackPeek(plypane->uidStack, &data)){
		if (data != (intptr_t)until){
			if (!stackPop(plypane->uidStack, &data))
				return;
		}else{
			return;
		}
	};
}

int64_t plypane_locbar_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT || msg == CC_MSG_HOVER) return 1;
	
	TLABEL *bar = (TLABEL*)object;
	//printf("plypane_lbl_cb in %p, %i %I64d %I64d %p\n", bar, msg, data1, data2, dataPtr);

	if (msg == LABEL_MSG_TEXT_SELECTED_PRESS){
		TPLYPANE *plypane = ccGetUserData(bar);	
		TPANE *pane = plypane->pane;
		
		int uid = labelItemDataGet(bar, data2);
		if (uid){
			// remember current position
			int id = plypaneGetPaneUID(plypane);
			PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(bar->cc->vp->plm, id);
			if (!plc){	// playlist was probably deleted so [re]set to something valid
				plypaneStackEmpty(plypane);
				uid = getQueuedPlaylistUID(bar->cc->vp);
				if (uid && uid != id){
					PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(bar->cc->vp->plm, uid);
					if (plc)
						buildPlaylistRoute(plypane->uidStack, plc);
					else
						uid = getRootPlaylistUID(bar->cc->vp);	
				}else{
					uid = getRootPlaylistUID(bar->cc->vp);
				}

				paneRemoveAll(pane);
				plyPaneAddPlaylist(plypane, pane, bar->cc->vp->plm, uid);
				buildLocBar(plypane, bar);
				buildTitleBar(plypane, plypane->title, uid);
				return 1;
			}

			plyPlaylistPaneSetPosition(plc, pane->offset.x, pane->offset.y);

			// rewind stack until we're at the selected playlist
			plypaneStackRewind(plypane, uid);

			// set new
			plc = playlistManagerGetPlaylistByUID(bar->cc->vp->plm, uid);
			if (!plc) return 1;
			plyPlaylistPaneGetPosition(plc, &pane->offset.x, &pane->offset.y);

			paneRemoveAll(pane);
			plyPaneAddPlaylist(plypane, pane, bar->cc->vp->plm, uid);
			buildLocBar(plypane, bar);
			buildTitleBar(plypane, plypane->title, uid);
		}
	}

	return 1;
}

int plypaneSetPlaylist (TVLCPLAYER *vp, const int uid)
{
	TPLYPANE *plypane = pageGetPtr(vp, PAGE_PLY_PANE);

	int ret = 0;
	if (ccLock(plypane->pane)){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
		if (plc){
			plypaneStackEmpty(plypane);
			buildPlaylistRoute(plypane->uidStack, plc);

			plypaneSetPaneUID(plypane, uid);
			//buildLocBar(plypane, plypane->locBar);
			//buildTitleBar(plypane, plypane->title, uid);
			
			timerSet(vp, TIMER_PLYPANE_REFRESH, 10);
			ret = uid;
		}
		ccUnlock(plypane->pane);
	}
	
	return ret;
}

static inline int64_t plypane_pane_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT || msg == CC_MSG_HOVER) return 1;
	
	TPANE *pane = (TPANE*)object;
	//printf("plypane_pane_cb in %p, %i %I64d %I64d %p\n", pane, msg, data1, data2, dataPtr);
	
	if (msg == PANE_MSG_TEXT_SELECTED || msg == PANE_MSG_IMAGE_SELECTED){
		TVLCPLAYER *vp = pane->cc->vp;
		TPLYPANE *plypane = pageGetPtr(vp, PAGE_PLY_PANE);	
		
		const int isBack = (int64_t)(data2>>32)&0x01;
		const int isClose = (int64_t)(data2>>33)&0x01;
		const int pid = (data2>>16)&0xFFFF;
		int track = (data2&0xFFFF);
		
		//printf("isBack:%i %i, %X %i %i\n", isBack, isClose, pid, track, pid);
		
		if (isClose){
			page2SetPrevious(plypane);
			
		}else if (!track){	// is a playlist
			// set current (soon to be old) pane position
			PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, plypaneGetPaneUID(plypane));
			if (!plc) return 1;
			plyPlaylistPaneSetPosition(plc, pane->offset.x, pane->offset.y);

			plc = playlistManagerGetPlaylistByUID(vp->plm, pid);
			if (plc){
				if (!plc->artId){
					int total = playlistGetTotal(plc);
					if (total < 100){
						initiateAlbumArtRetrieval(vp, plc, 0, total-1, vp->gui.artSearchDepth);
						playlistMetaGetMeta(vp, plc, 0, total-1, NULL);
					}
				}
				plyPlaylistPaneGetPosition(plc, &pane->offset.x, &pane->offset.y);
			}else{
				timerSet(vp, TIMER_PLYPANE_REFRESH, 0);
				return 1;
			}

			paneRemoveAll(pane);

			if (isBack)
				stackPop(plypane->uidStack, NULL);
			plyPaneAddPlaylist(plypane, pane, vp->plm, pid);
			buildLocBar(plypane, plypane->locBar);
			buildTitleBar(plypane, plypane->title, pid);
			
			// ensure title are accurate
			timerSet(vp, TIMER_PLYPANE_REFRESH, 1000);
			
		}else{
			track--;
			// is a track - selected/play it
			//printf("play track %i, playlist %X\n", track, pid);
			PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, pid);
			if (!plc) return 1;
			
			//printf("plypane_cb: data1 %i %i\n", (int)data1, (msg == PANE_MSG_IMAGE_SELECTED));
			int itemId = data1 + (msg == PANE_MSG_IMAGE_SELECTED);
				
			if (getPlayState(vp) && isPlayingItem(vp, plc, track)){
				timerSet(vp, TIMER_STOP, 0);
			}else{
				plypane->playingItemId = itemId;

				TTIMERPLAYTRACK *tpt = &vp->gui.playtrack;
				tpt->uid = pid;
				tpt->track = track;
				timerSet(vp, TIMER_PLAYTRACK, 0);
			}
		}
	}
	
	return 1;
}


static inline int page_plypaneInitalize (TPLYPANE *plypane, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_PLY_PANE);
	
	wchar_t bufferw[MAX_PATH+1];
	
	plypane->icons.audio = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/audio32.png"));
	plypane->icons.folder = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/folder32.png"));
	plypane->icons.play = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/play32.png"));
	plypane->icons.back = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/back96.png"));
	plypane->icons.art = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"shelf/noart.png"));
	plypane->icons.closePane = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/close96.png"));
	
	return plypane->icons.closePane > 0;
}

static inline int page_plypaneStartup (TPLYPANE *plypane, TVLCPLAYER *vp, const int width, const int height)
{
	plypane->uidStack = stackCreate(16);
		
	TPANE *pane = ccCreateEx(vp->cc, PAGE_PLY_PANE, CC_PANE, plypane_pane_cb, &vp->gui.ccIds[CCID_PANE_PLAYLIST], width, height-64, plypane);
	ccSetPosition(pane, 0, 64);
	paneSetAcceleration(pane, PANE_ACCELERATION_X, PANE_ACCELERATION_Y);
	plypane->pane = pane;
	pane->flags.readAhead.enabled = 0;
		
	TLABEL *locBar = ccCreate(vp->cc, PAGE_PLY_PANE, CC_LABEL, plypane_locbar_cb, &vp->gui.ccIds[CCID_LABEL_PANELOCBAR], width, 36);
	ccSetUserData(locBar, plypane);
	labelRenderFlagsSet(locBar, LABEL_RENDER_HOVER_OBJ | LABEL_RENDER_TEXT);
	ccSetPosition(locBar, 0, 0);
	plypane->locBar = locBar;


	TLABEL *title = ccCreate(vp->cc, PAGE_PLY_PANE, CC_LABEL, plypane_titlebar_cb, NULL, width, 34);
	ccSetUserData(title, plypane);
	labelRenderFlagsSet(title, LABEL_RENDER_TEXT);
	ccSetPosition(title, 0, 34);
	plypane->title = title;	

	
	return 1;
}

static inline int page_plypaneShutdown (TPLYPANE *plypane, TVLCPLAYER *vp)
{
	stackDestroy(plypane->uidStack);
	ccDelete(plypane->locBar);
	ccDelete(plypane->title);
	ccDelete(plypane->pane);
	
	return 1;
}

static inline int page_plypaneRenderInit (TPLYPANE *plypane, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	int uid = getQueuedPlaylistUID(vp);
	if (uid){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
		if (plc)
			buildPlaylistRoute(plypane->uidStack, plc);
		else
			uid = getRootPlaylistUID(vp);
	}else{
		uid = getRootPlaylistUID(vp);
	}

	plyPaneAddPlaylist(plypane, plypane->pane, vp->plm, uid);
	buildLocBar(plypane, plypane->locBar);
	buildTitleBar(plypane, plypane->title,  uid);
	return 1;
}

static inline int page_plypaneRenderBegin (TPLYPANE *plypane, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	ccEnable(plypane->pane);
	ccEnable(plypane->locBar);
	ccEnable(plypane->title);
	
	timerSet(vp, TIMER_PLYPANE_REFRESH, 100);
	
	return 1;
}

static inline void page_plypaneRenderEnd (TPLYPANE *plypane, TVLCPLAYER *vp, int64_t destId, int64_t data2, void *opaquePtr)
{
	ccDisable(plypane->pane);
	ccDisable(plypane->locBar);
	ccDisable(plypane->title);
}

int page_plyPaneCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TPLYPANE *plypane = (TPLYPANE*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
	//	 printf("# page_plyPaneCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_plypaneRender(plypane, plypane->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_plypaneInput(plypane, plypane->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_START){
		return page_plypaneRenderBegin(plypane, plypane->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		page_plypaneRenderEnd(plypane, plypane->com->vp, dataInt1, dataInt2, opaquePtr);

	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_plypaneRenderInit(plypane, plypane->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_plypaneStartup(plypane, plypane->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_plypaneInitalize(plypane, plypane->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_plypaneShutdown(plypane, plypane->com->vp);
		
	}
	
	return 1;
}

