
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


#define MAKEID(a,b)			((a)<<16|((b)&0xFFFF))
#define TEXT_MAX_HEIGHT		(44)
#define ARTSIZE				(110.0)

static int g_imgArtSize = ARTSIZE;
extern volatile int SHUTDOWN;
extern volatile double UPDATERATE_BASE;



#if 0
// this can only work after passing nodes have been built, otherrwise nothing exists to open
void playlistExpandTo (TTV *tv, const int uid)
{
	int stack[1024] = {0};
	int sp = 0;

	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(tv->cc->vp->plm, uid);

	// generate list of playlist parents'
	while (plc && plc->uid){
		if (plc->uid > PLAYLIST_UID_BASE+1)
			stack[sp++] = plc->uid;
		
		plc = plc->parent;
	}
	
	if (sp){
		for (int i = sp-1; i >= 0; i--){
			//printf("%i %X\n", i, stack[i]);
	
			TTV_ITEM *item = tvTreeGetItem(tv, stack[i], NULL);
			if (item){
				tvInputDoAction(tv, item->id, TV_ACTION_EXPANDER_OPEN, NULL, item);
				tvTreeFreeItem(item);
			}
		}
		tvBuildPrerender(tv, tv->rootId);
	}
}
#endif



static inline int opaqueGetType (TTV_NODE_DESC_OPAQUE *opaque)
{
	return opaque->objType;
}

static inline int opaqueGetPlaylist (TTV_NODE_DESC_OPAQUE *opaque)
{
	if (opaque->objType == NODE_OBJTYPE_PLC)
		return opaque->playlist.pid;
	else if (opaque->objType == NODE_OBJTYPE_TRACK)
		return opaque->track.pid;
	else if (opaque->objType == NODE_OBJTYPE_IMAGE)
		return opaque->image.pid;
	else if (opaque->objType == NODE_OBJTYPE_DETAIL)
		return opaque->detail.pid;
	else
		return 0;
}

static inline int opaqueGetTrack (TTV_NODE_DESC_OPAQUE *opaque)
{
	if (opaque->objType == NODE_OBJTYPE_PLC)
		return opaque->playlist.position;
	else if (opaque->objType == NODE_OBJTYPE_TRACK)
		return opaque->track.position;
	else if (opaque->objType == NODE_OBJTYPE_IMAGE)
		return opaque->image.position;
	else if (opaque->objType == NODE_OBJTYPE_DETAIL)
		return opaque->detail.position;
	else
		return 0;
}

static inline TTV_ITEM_DESC_IMAGE *tvItemImageGet (TTV_ITEM_DESC *desc, const int imgNumber)
{
	if (desc->image)
		return &desc->image[imgNumber-1];
	else
		return NULL;
}

static inline void tvItemImageSetState (TTV_ITEM_DESC *desc, const int imgNumber, const int drawState)
{
	TTV_ITEM_DESC_IMAGE *image = tvItemImageGet(desc, imgNumber);
 	image->drawState = drawState;
}

static inline TTV_ITEM_DESC_IMAGE *tvItemImageSetImg (TTV *tv, TTV_ITEM_DESC *desc, const int imgNumber, const int artcId, const int offsetX, const int offsetY)
{
	TTV_ITEM_DESC_IMAGE *image = tvItemImageGet(desc, imgNumber);
	image->drawImage = TV_DRAWIMAGE_ENABLED;
	image->offsetX = offsetX;
	image->offsetY = offsetY;
	image->artId = artcId;
	
	int width=8, height=8;
	artManagerImageGetMetrics(tv->cc->vp->am, image->artId, &width, &height);
	
	int w = width;
	if (w < MIN_IMAGE_WIDTH) w = MIN_IMAGE_WIDTH;
	int h = height;
	if (h < MIN_IMAGE_HEIGHT) h = MIN_IMAGE_HEIGHT;
	
	if (image->offsetX + w > desc->metrics.width)
		desc->metrics.width = image->offsetX + w;
	if (image->offsetY + h > desc->metrics.height)
		desc->metrics.height = image->offsetY + h;

	return image;
}

static inline void *tvItemImageOpaqueGet (TTV_ITEM_DESC_IMAGE *image)
{
	return image->opaque;
}

static inline void *tvItemImageOpaqueCreate (TTV_ITEM_DESC_IMAGE *image, const size_t len)
{
	return image->opaque = my_calloc(1, len);
}

static inline TTV_ITEM_DESC_IMAGE *tvItemImageCreate (TTV_ITEM_DESC *desc, const int total)
{
	desc->imageTotal = total;
	desc->image = my_calloc(total, sizeof(TTV_ITEM_DESC_IMAGE));
	if (desc->image){
		for (int i = 0; i < total; i++){
			desc->image[i].drawImage = TV_DRAWIMAGE_DISABLED;
			desc->image[i].drawState = TV_DRAWIMAGE_DISABLED;
			desc->image[i].scale = 1.0;
		}
	}
	return desc->image;
}

static inline TTV_ITEM_DESC *tvTreeBuildDescPlc (TTV *tv, const char *name, const int pid, const int position)
{
	TTV_ITEM_DESC *desc = my_calloc(1, sizeof(TTV_ITEM_DESC));
	if (desc){
		lGetTextMetrics(tv->cc->vp->ml->hw, name, 0, LFONT, &desc->metrics.width, &desc->metrics.height);
		desc->metrics.height += 6;
		if (desc->metrics.height > TEXT_MAX_HEIGHT)
			desc->metrics.height = TEXT_MAX_HEIGHT;
		
		desc->enabled = 1;
		desc->objType = TV_TYPE_LABEL;
		desc->expander.drawExpander = TV_EXPANDER_ENABLED;
		desc->expander.expanderState = TV_EXPANDER_CLOSED;
		desc->checkbox.drawCheckbox = 1;
		
		desc->label.colour = 255<<24 | COL_WHITE;
		desc->label.font = PLAYLIST_TV_FONT;
		desc->label.flags = PF_IGNOREFORMATTING;		
		
		TTV_NODE_DESC_OPAQUE *opaque = my_calloc(1, sizeof(TTV_NODE_DESC_OPAQUE));
		opaque->objType = NODE_OBJTYPE_PLC;
		opaque->playlist.pid = pid;
		opaque->playlist.position = position;
		desc->opaque = opaque;

	}
	return desc;
}

static inline TTV_ITEM_DESC *tvTreeBuildDescTrack (TTV *tv, const char *name, const int pid, const int track)
{
	TTV_ITEM_DESC *desc = my_calloc(1, sizeof(TTV_ITEM_DESC));
	if (!desc) return NULL;
	
	lGetTextMetrics(tv->cc->vp->ml->hw, name, 0, LFONT, &desc->metrics.width, &desc->metrics.height);
	desc->metrics.height += 6;
	if (desc->metrics.height > TEXT_MAX_HEIGHT)
		desc->metrics.height = TEXT_MAX_HEIGHT;
		
	desc->enabled = 1;
	desc->objType = TV_TYPE_LABEL;
	desc->expander.drawExpander = TV_EXPANDER_DISABLED;
	desc->expander.expanderState = TV_EXPANDER_DONTRENDER;
	desc->checkbox.drawCheckbox = 1;
		
	desc->label.colour = 255<<24 | COL_WHITE;
	desc->label.font = PLAYLIST_TV_FONT;
	desc->label.flags = PF_IGNOREFORMATTING;
		
	TTV_NODE_DESC_OPAQUE *opaque = my_calloc(1, sizeof(TTV_NODE_DESC_OPAQUE));
	opaque->objType = NODE_OBJTYPE_TRACK;
	opaque->track.pid = pid;
	opaque->track.position = track;
	desc->opaque = opaque;

	return desc;
}

static inline TTV_ITEM_DESC *tvTreeBuildDescDetail (TPLYTV *plytv, TTV *tv, const char *length, TPLAYLISTITEM *playlistItem, const int pid, const int track)
{
	TVLCPLAYER *vp = tv->cc->vp;
	PLYTVPLCDEPENDENTS dep;
	
	dep.tvplayId = plytv->artc.play;
	dep.tvpauseId = plytv->artc.pause;
	dep.tvstopId = plytv->artc.stop;
	dep.noartId = plytv->artc.noart;
	dep.hw = vp->ml->hw;
	dep.tagc = vp->tagc;
	dep.vp = vp;
	
	
	const int flags = 0;
	const int font = PLAYLIST_TV_FONT;

	TTV_ITEM_DESC *descDetail = my_calloc(1, sizeof(TTV_ITEM_DESC));
	if (!descDetail) return NULL;

	descDetail->enabled = 1;
	descDetail->objType = TV_TYPE_LABEL;

	descDetail->label.flags |= flags;
	descDetail->label.font = font;
	descDetail->label.offsetX = 10;
	descDetail->label.offsetY = 16;
	descDetail->expander.drawExpander = TV_EXPANDER_DISABLED;
	descDetail->expander.expanderState = TV_EXPANDER_DONTRENDER;
	descDetail->checkbox.drawCheckbox = 0;


	lGetTextMetrics(dep.hw, length, flags, font, &descDetail->metrics.width, &descDetail->metrics.height);
	descDetail->metrics.height += 2;
	if (descDetail->metrics.height > TEXT_MAX_HEIGHT)
		descDetail->metrics.height = TEXT_MAX_HEIGHT;
	int timeHeight = descDetail->metrics.height;
	
	
	TTV_NODE_DESC_OPAQUE *desc = my_calloc(1, sizeof(TTV_NODE_DESC_OPAQUE));
	desc->objType = NODE_OBJTYPE_DETAIL;
	desc->detail.pid = pid;
	desc->detail.position = track;
	descDetail->opaque = desc;

	// add artwork as first item directly below track title
	tvItemImageCreate(descDetail, 4);		// create space for 4 images


	TTV_ITEM_DESC_IMAGE *image = tvItemImageSetImg(tv, descDetail, ITEM_DETAIL_IMAGE_ARTWORK, dep.noartId, 6, 4);
	image->cbId = tvGenImageCbId(tv);
	desc = tvItemImageOpaqueCreate(image, sizeof(TTV_NODE_DESC_OPAQUE));
	desc->objType = NODE_OBJTYPE_IMAGE;
	desc->image.pid = pid;
	desc->image.position = track;
	desc->image.type = ITEM_DETAIL_IMAGE_ARTWORK;
	image->drawState = TV_DRAWIMAGE_ENABLED;
	
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, pid);
	if (plc) playlistMetaGetTrackMetaByHash(tv->cc->vp, playlistItem->obj.track.hash, plc, playlistItem->obj.track.path, desc->image.position, NULL);

	
	const int timeGap = timeHeight + 26;
	image = tvItemImageSetImg(tv, descDetail, ITEM_DETAIL_IMAGE_PLAY, dep.tvplayId, -2, timeGap);
	desc = tvItemImageOpaqueCreate(image, sizeof(TTV_NODE_DESC_OPAQUE));
	desc->objType = NODE_OBJTYPE_IMAGE;
	desc->image.pid = pid;
	desc->image.position = track;
	desc->image.type = ITEM_DETAIL_IMAGE_PLAY;
	image->drawState = TV_DRAWIMAGE_ENABLED;

	image = tvItemImageSetImg(tv, descDetail, ITEM_DETAIL_IMAGE_PAUSE, dep.tvpauseId, -2, timeGap);
	desc = tvItemImageOpaqueCreate(image, sizeof(TTV_NODE_DESC_OPAQUE));
	desc->objType = NODE_OBJTYPE_IMAGE;
	desc->image.pid = pid;
	desc->image.position = track;
	desc->image.type = ITEM_DETAIL_IMAGE_PAUSE;
	image->drawState = TV_DRAWIMAGE_DONTRENDER;
				
	image = tvItemImageSetImg(tv, descDetail, ITEM_DETAIL_IMAGE_STOP, dep.tvstopId, 6, timeGap);
	desc = tvItemImageOpaqueCreate(image, sizeof(TTV_NODE_DESC_OPAQUE));
	desc->objType = NODE_OBJTYPE_IMAGE;
	desc->image.pid = pid;
	desc->image.position = track;
	desc->image.type = ITEM_DETAIL_IMAGE_STOP;
	image->drawState = TV_DRAWIMAGE_DONTRENDER;


	int width, height;
	artManagerImageGetMetrics(tv->cc->vp->am, dep.noartId, &width, &height);
	
	descDetail->metrics.width = width;
	if (descDetail->metrics.width < MIN_IMAGE_WIDTH)
		descDetail->metrics.width = MIN_IMAGE_WIDTH;
			
	descDetail->metrics.height = height;
	if (descDetail->metrics.height < MIN_IMAGE_HEIGHT)
		descDetail->metrics.height = MIN_IMAGE_HEIGHT;

	//descDetail->metrics.height -= descDetail->label.offsetY;

	descDetail->metrics.width += 4;
	descDetail->metrics.height += 5;

	return descDetail;
}

static inline int tvTreeAddPlc (TPLYTV *plytv, TTV *tv, const int id, PLAYLISTCACHE *plc, const int addBranches)
{
	const int total = playlistGetTotal(plc);
	//printf("tvTreeAddPlc %X '%s' %i %i\n", id, plc->title, addBranches, total);
	
	for (int i = 0; i < total; i++){
		TPLAYLISTITEM *item = playlistGetItem(plc, i);
		//printf("tvTreeAddPlc %i %p\n", i, item);
		if (!item) continue;

		if (item->objType == PLAYLIST_OBJTYPE_PLC){
			int subid = item->obj.plc->uid;
			
			TTV_ITEM_DESC *desc = tvTreeBuildDescPlc(tv, item->obj.plc->title, subid, i);
			tvTreeAddNode(tv, id, item->obj.plc->title, subid, desc);
			if (addBranches)
				tvTreeAddPlc(plytv, tv, subid, item->obj.plc, addBranches);
			
		}else if (item->objType == PLAYLIST_OBJTYPE_TRACK){
			const int trackId = MAKEID(plc->uid, i+1);
			TTV_ITEM_DESC *desc = tvTreeBuildDescTrack(tv, item->obj.track.title, plc->uid, i);
			//printf(":: %i, %X, '%s', %X\n", i, id, item->obj.track.title, trackId);
			tvTreeAddNode(tv, id, item->obj.track.title, trackId, desc);
			
			TPLAYLISTITEM *playlistItem = playlistGetItem(plc, i);
			
			char length[MAX_PATH_UTF8+1];
			tagRetrieveByHash(tv->cc->vp->tagc, playlistItem->obj.track.hash, MTAG_LENGTH, length, MAX_PATH_UTF8);
			if (!*length) strcpy(length, "0:00");
	
	
			/*TTV_ITEM_DESC *descDetail = my_calloc(1, sizeof(TTV_ITEM_DESC));
			descDetail->enabled = 1;
			descDetail->objType = TV_TYPE_LABEL;
			descDetail->label.flags = 0;
			descDetail->label.font = PLAYLIST_TV_FONT;
			descDetail->label.offsetX = 172;
			descDetail->label.offsetY = -160;
			descDetail->expander.drawExpander = TV_EXPANDER_DISABLED;
			descDetail->expander.expanderState = TV_EXPANDER_DONTRENDER;
			descDetail->checkbox.drawCheckbox = 0;
			lGetTextMetrics(tv->cc->vp->ml->hw, "testing", 0, descDetail->label.font, &descDetail->metrics.width, &descDetail->metrics.height);
			descDetail->metrics.height += 2;
			if (descDetail->metrics.height > TEXT_MAX_HEIGHT)
				descDetail->metrics.height = TEXT_MAX_HEIGHT;
				
			TTV_NODE_DESC_OPAQUE *desco = my_calloc(1, sizeof(TTV_NODE_DESC_OPAQUE));
			desco->objType = NODE_OBJTYPE_DETAIL;
			desco->detail.pid = plc->uid;
			desco->detail.position = i;
			descDetail->opaque = desco;*/
			
			desc = tvTreeBuildDescDetail(plytv, tv, length, playlistItem, plc->uid, i);
			tvTreeAddItem(tv, trackId, length, 0x8000 | trackId, desc);
			
			/*descDetail->label.offsetX += desc->metrics.width;
			descDetail->label.offsetY = -desc->metrics.height;
			tvTreeAddItem(tv, trackId, "testing", 0x4000 | trackId, descDetail);*/
		}
	}
	return total;
}

static inline void tvTreeResyncItems (TTV *tv, TTV_ITEM *item, const int pid)
{
	TTV_NODE_DESC_OPAQUE *desc, *data;
	int idx = 0;
	int child = 0;
	
	//printf("tvTreeResyncItems '%s'\n", item->name);
	
	while(item->children[child]){
		TTV_ITEM *subItem = tvTreeGetItem(tv, item->children[child], item->entry);
		if (subItem){

			//printf(":: %i :%s:\n", idx, subItem->name);

			desc = tvItemOpaqueGet(subItem);
			desc->track.position = idx++;
			desc->playlist.position = desc->track.position;
			if (pid) desc->track.pid = pid;
			

			if (desc->objType == NODE_OBJTYPE_TRACK){
				TTV_ITEM *detail = tvTreeGetItem(tv, subItem->children[0], subItem->entry);
				if (detail){
					data = tvItemOpaqueGet(detail);
					if (data && data->objType == NODE_OBJTYPE_DETAIL){
						data->detail.position = desc->track.position;
						data->detail.pid = desc->track.pid;
					}
				
					TTV_ITEM_DESC *descDetail = tvTreeItemGetDesc(detail);
				
					data = tvItemImageOpaqueGet(tvItemImageGet(descDetail, ITEM_DETAIL_IMAGE_ARTWORK));
					data->image.position = desc->track.position;
					data->image.pid = desc->track.pid;
					data = tvItemImageOpaqueGet(tvItemImageGet(descDetail, ITEM_DETAIL_IMAGE_PLAY));
					data->image.position = desc->track.position;
					data->image.pid = desc->track.pid;
					data = tvItemImageOpaqueGet(tvItemImageGet(descDetail, ITEM_DETAIL_IMAGE_PAUSE));
					data->image.position = desc->track.position;
					data->image.pid = desc->track.pid;
					data = tvItemImageOpaqueGet(tvItemImageGet(descDetail, ITEM_DETAIL_IMAGE_STOP));
					data->image.position = desc->track.position;
					data->image.pid = desc->track.pid;
				
					tvTreeFreeItem(detail);
				}
			}
		
			tvTreeFreeItem(subItem);
		}
		child++;
	};
}

static inline int plytvTreeRemoveCheckedItems (TTV *tv, TTV_ITEM *item)
{
	
	//printf("# plytvTreeRemoveCheckedItems %i '%s'\n", item->id, item->name);
	
	int recOffset = 0;
	int doResync = 0;
	int child = 0;
	
	while(item->children[child]){
		TTV_ITEM *subItem = tvTreeGetItem(tv, item->children[child], item->entry);
		if (subItem){
			if (subItem->type == TREE_TYPE_BRANCH){
				if (plytvTreeRemoveCheckedItems(tv, subItem)){
					TTV_ITEM *itemUpdated = tvTreeGetItem(tv, subItem->id, item->entry);
					tvTreeResyncItems(tv, itemUpdated, 0);
					tvTreeFreeItem(itemUpdated);
				}
			}
			
			TTV_ITEM_DESC *desc = tvTreeItemGetDesc(subItem);
			if (desc->checkbox.checkState == TV_CHECKBOX_CHECKED){
				//printf("tvTreeDelete %X\n", subItem->id);

				TTV_NODE_DESC_OPAQUE *opaque = tvItemOpaqueGet(subItem);
				if (opaque->objType == NODE_OBJTYPE_TRACK){
					PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(tv->cc->vp->plm, opaque->track.pid);
					playlistDeleteRecord(plc, opaque->track.position - recOffset++);
					doResync = 1;
				
				}else if (opaque->objType == NODE_OBJTYPE_PLC){
					PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(tv->cc->vp->plm, opaque->playlist.pid);
					if (plc){
						//printf("deleting playlist: %i/%i '%s'\n", opaque->playlist.pid, opaque->playlist.position, plc->title);
						playlistManagerDeletePlaylist(tv->cc->vp->plm, plc, 0);
						playlistDeleteRecord(plc->parent, opaque->playlist.position - recOffset++);
						doResync = 1;
					}
				}
				//if (item->id == plytv->current) plytv->current = 0;
			
				tvTreeDelete(tv, subItem->id);
			}

			tvTreeFreeItem(subItem);
		}
		child++;
	}
	if (doResync){
		//tvTreeResyncItems(tv, item, 0);
		TTV_ITEM *itemUpdated = tvTreeGetItem(tv, item->id, 0);
		tvTreeResyncItems(tv, itemUpdated, 0);
		tvTreeFreeItem(itemUpdated);
	}
	
	//if (recOffset)
	//	tvTreeResyncItems(tv, item, item->id);
	return recOffset;
}

static inline void plytvTreeDeleteCheckedItems (TTV *tv)
{
	TTV_ITEM *item = tvTreeGetItem(tv, tv->rootId, NULL);
	if (item){
		plytvTreeRemoveCheckedItems(tv, item);
		tvTreeFreeItem(item);
		//plytvTvRenderRebuild(tv);
	}

	//const double t0 = getTime(tv->cc->vp);
	tagFlushOrfhensPlm(tv->cc->vp->tagc, tv->cc->vp->plm);
	//const double t1 = getTime(tv->cc->vp);
	//printf("tagFlushOrfhensPlm %f\n", t1-t0);
	//
}

static inline void plytvDeleteChecked (TPLYTV *plytv, TTV *tv)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)tv->cc->vp;
		  	
	if (ccLock(tv)){
		plytvTreeDeleteCheckedItems(tv);
		//tvCheckClearAll(tv);
		ccUnlock(tv);
  	}	
	
	TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);
	shelfFlush(spl->shelf);
	invalidateShelfAlbum(vp, spl, spl->from);
	
	spl = pageGetPtr(vp, PAGE_PLY_FLAT);
	shelfFlush(spl->shelf);
	invalidateShelfAlbum(vp, spl, spl->from);
	
	
	TPLYPANEL *plypan = pageGetPtr(vp, PAGE_PLY_PANEL);
	PLAYLISTCACHE *panelPlc = playlistManagerGetPlaylistByUID(vp->plm, plypan->currentPlcUID);
	if (!panelPlc){
		panelPlc = getPrimaryPlaylist(vp);
		plypan->currentPlcUID = 0;
	}
		
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	if (plc == panelPlc && plc != getDisplayPlaylist(vp)){
		plc = panelPlc->parent;
		if (!plc) plc = getPrimaryPlaylist(vp);
	}else{
		plc = panelPlc;
	}
	//panelListResize(plypan->panel, playlistGetTotal(plc), 0);
	plyPanelBuild(vp, plypan->panel, plc);
}

// TIMER_PLYTV_REFRESH
void plytvRefresh (TVLCPLAYER *vp)
{
	//printf("plytvRefresh\n");
	
	if (hasPageBeenAccessed(vp, PAGE_PLY_TV)){
		TPLYTV *plytv = pageGetPtr(vp, PAGE_PLY_TV);
		if (plytv->buildNo)	{
			TTV *tv = plytv->tv;
  			swipeReset(&plytv->swipe);
  			plytvTvReload(plytv, tv, getPrimaryPlaylist(tv->cc->vp));
  		}
  	}
}

void plytvButtonsSetState (TPLYTV *plytv, TCCBUTTONS *btns, const int state)
{
	buttonsSetState(btns, state);
	if (state){
		if (plytv->tv->dragEnabled)
			buttonsStateSet(btns, PLYTV_LOCKED, 0);
		else
			buttonsStateSet(btns, PLYTV_UNLOCKED, 0);
	}
}

static inline int plytvButtonPress (TPLYTV *plytv, TCCBUTTON *btn, const int btnId)
{
	TTV *tv = plytv->tv;
	plytv->btns->t0 = getTickCount();
	
	switch (btnId){
	  case PLYTV_EXPAND_OPENALL:
  		if (ccLock(tv)){
			vlcEventListInvalidate(tv->cc->vp->vlc);
			vlcEventsCleanup(tv->cc->vp);
			
  			tvCollapseAll(tv);
  			tvExpandAll(tv);
  			plytvTvRenderRebuild(tv);
	  		ccUnlock(tv);
  		}
		break;

	  case PLYTV_EXPAND_CLOSEALL:
	  	if (ccLock(tv)){
	  		vlcEventListInvalidate(tv->cc->vp->vlc);
			vlcEventsCleanup(tv->cc->vp);
			
	  		tvCollapseAll(tv);
	  		plytvTvRenderRebuild(tv);
	  		ccUnlock(tv);
	  	}
		break;

	  case PLYTV_BACK:
	  	swipeReset(&plytv->swipe);
	  	page2SetPrevious(plytv);
	  	break;
	  	
	  case PLYTV_DELETE:
  		swipeReset(&plytv->swipe);
		plytvDeleteChecked(plytv, tv);
		plytvTvRenderRebuild(tv);
	  	break;

	  case PLYTV_RELOAD:
	  	swipeReset(&plytv->swipe);
	  	plytvTvReload(plytv, tv, getPrimaryPlaylist(btn->cc->vp));
	  	break;
	  	
  	  case PLYTV_LOCKED:
		tv->dragEnabled = 1;
		ccDisable(btn);
		ccEnable(buttonsButtonGet(plytv->btns, PLYTV_UNLOCKED));
		break;
		
  	  case PLYTV_UNLOCKED:
		tv->dragEnabled = 0;
		ccDisable(btn);
		ccEnable(buttonsButtonGet(plytv->btns, PLYTV_LOCKED));
		break;
			
  	  case PLYTV_SCROLLUP:
  		swipeReset(&plytv->swipe);
		tv->sbVertPositionRate = (3*39)+2;
		tvScrollUp(tv);
  		break;

  	  case PLYTV_SCROLLDOWN:
	  	swipeReset(&plytv->swipe);
		tv->sbVertPositionRate = (3*39)+2;
		tvScrollDown(tv);
	  	break;
	
	  case PLYTV_RENAME:
	  	if (!tvRenameGetState(tv)){
	  		tvRenameEnable(tv);
	  		plytvButtonsSetState(plytv, plytv->btns, 0);
			buttonsStateSet(plytv->btns, PLYTV_RENAME, 1);
	  	}else{
	  		plytvButtonsSetState(plytv, plytv->btns, 1);
	  		tvRenameDisable(tv);
	  	}
	};
	
	plytv->btns->t0 = getTickCount();
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
		return plytvButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn));
	else
		return 0;
}

static inline int plytvTreeCreateRoot (TPLYTV *plytv, TTV *tv, PLAYLISTCACHE *plc)
{
	//printf("plytvTreeCreateRoot\n");
	
	int ret = 0;
	if (ccLock(tv)){
		//tvTreeCreate(tv, plc->title, plc->uid);
		ret = tvTreeAddPlc(plytv, tv, plc->uid, plc, 0);
		if (ret)
			tvBuildPrerender(tv, plc->uid);
		ccUnlock(tv);
	}
	return ret;
}

int plytvTvReload (TPLYTV *plytv, TTV *tv, PLAYLISTCACHE *plc)
{
	//printf("plytvTvReload\n");
	
	
	int ret = 0;
	if (ccLock(tv)){
		plytv->buildNo++;
		
		vlcEventListInvalidate(tv->cc->vp->vlc);
		vlcEventsCleanup(tv->cc->vp);
		//if (tv->cc->vp->jt)
		//	artQueueFlush(artThreadGetWork(tv->cc->vp->jt));
		
		tv->sbVertPosition = 0;
		plytv->current = 0;

		scrollbarSetFirstItem(tv->sbVert, 0);

		tvTreeDeleteItems(tv, tv->rootId);
		ret = tvTreeAddPlc(plytv, tv, plc->uid, plc, 0);
		if (ret)
			tvBuildPrerender(tv, plc->uid);

		ccUnlock(tv);
	}
	return ret;
}

void plytvTvRenderRebuild (TTV *tv)
{
	//printf("plytvTvRenderRebuild\n");
	tvBuildPrerender(tv, tv->rootId);
}

static inline int isOverlap (const int x, const int y, const TLPOINTEX *box)
{
	if (x >= box->x1 && x <= box->x2){
		if (y >= box->y1 && y <= box->y2)
			return 1;
	}
	return 0;
}

static inline void drawRectangleConstrained (TFRAME *frame, const int x1, const int y1, const int x2, const int y2, const int colour, const int miny, const int maxy)
{
	if (y1 >= miny && y1 <= maxy){
		for (int x = x1; x <= x2; x++)			// top
			lSetPixel_NB(frame, x, y1, colour);
	}
	if (y2 >= miny && y2 <= maxy){
		for (int x = x1; x <= x2; x++)			// bottom
			lSetPixel_NB(frame, x, y2, colour);
	}
	for (int y = y1+1; y < y2; y++){			// left
		if (y >= miny && y <= maxy)
			lSetPixel_NB(frame, x1, y, colour);
	}
	for (int y = y1+1; y < y2; y++){			// right
		if (y >= miny && y <= maxy)
			lSetPixel_NB(frame, x2, y, colour);
	}
}

static inline TTV_RENDER_ITEM *plytvRenderItemHighlight (TTV *tv, TFRAME *frame, const int x, const int y)
{
	TTV_RENDER_ITEM *post = tv->postRender;

	for (int i = 0; i < tv->tPostItems; i++, post++){
		if (isOverlap(x, y, &post->pos)){
			TTV_ITEM *item = tvTreeGetItem(tv, post->id, NULL);
			if (!item) continue;

			TTV_NODE_DESC_OPAQUE *opaque = tvItemOpaqueGet(item);
			
			// highlight item being dragged 'on to'
			if (tv->dragEnabled && tv->drag.state == 2 && opaque->objType == NODE_OBJTYPE_PLC){
				const int x1 = post->pos.x1-2;
				const int x2 = post->pos.x2+1;
				
				drawRectangleConstrained(frame, x1-1, post->pos.y1-1, x2+1, post->pos.y2+1, 140<<24 | COL_WHITE, tv->metrics.y, tv->metrics.y + tv->metrics.height-1);
				drawRectangleConstrained(frame, x1,   post->pos.y1, x2, post->pos.y2, 255<<24 | COL_WHITE, tv->metrics.y, tv->metrics.y + tv->metrics.height-1);
				drawRectangleConstrained(frame, x1+1, post->pos.y1+1, x2-1, post->pos.y2-1, 140<<24 | COL_WHITE, tv->metrics.y, tv->metrics.y + tv->metrics.height-1);
					
				tvTreeFreeItem(item);
				return post;
				
			}else if (tv->drag.state != 2){		// highlight item under cursor
				const int x1 = post->pos.x1-1;
				const int x2 = post->pos.x2+1;
				const int y1 = post->pos.y1-1;
				const int y2 = post->pos.y2+1;
					
				drawRectangleConstrained(frame, x1-1, y1-1, x2+1, y2+1, 140<<24 | COL_AQUA, tv->metrics.y, tv->metrics.y + tv->metrics.height-1);
				drawRectangleConstrained(frame, x1, y1, x2, y2, 255<<24 | COL_AQUA, tv->metrics.y, tv->metrics.y + tv->metrics.height-1);
				drawRectangleConstrained(frame, x1+1, y1+1, x2-1, y2-1, 140<<24 | COL_AQUA, tv->metrics.y, tv->metrics.y + tv->metrics.height-1);
			}	
			tvTreeFreeItem(item);
			return NULL;
		}
	}
	return NULL;
}

static inline TTV_RENDER_ITEM *ptvRenderItemInsert (TTV *tv, TFRAME *frame, const int x, const int y)
{
	if (!tv->dragEnabled || tv->drag.state != 2) return NULL;
	TTV_RENDER_ITEM *post = tv->postRender;
		
	for (int i = 0; i < tv->tPostItems; i++, post++){
		if (y >= post->pos.y1 && y <= post->pos.y2){
			TTV_ITEM *item = tvTreeGetItem(tv, post->id, NULL);
			if (!item) continue;

			TTV_NODE_DESC_OPAQUE *opaque = tvItemOpaqueGet(item);
			
			if (opaque->objType != NODE_OBJTYPE_DETAIL){
				int w = ((post->pos.x2 - post->pos.x1)+1)/2;
				int x = post->pos.x1 + w;
				int y = post->pos.y1+1;

				drawRectangleConstrained(frame, post->pos.x1-1, y-1, post->pos.x2+1, y+1, 140<<24 | COL_WHITE, tv->metrics.y, tv->metrics.y + tv->metrics.height-1);
				drawRectangleConstrained(frame, post->pos.x1, y, post->pos.x2, y, 255<<24 | COL_WHITE, tv->metrics.y, tv->metrics.y + tv->metrics.height-1);
				
				drawRectangleConstrained(frame, x-3, y-3, x+3, y+3, 120<<24 | COL_WHITE, tv->metrics.y, tv->metrics.y + tv->metrics.height-1);
				drawRectangleConstrained(frame, x-2, y-2, x+2, y+2, 230<<24 | COL_WHITE, tv->metrics.y, tv->metrics.y + tv->metrics.height-1);
				drawRectangleConstrained(frame, x-1, y-1, x+1, y+1, 255<<24 | COL_WHITE, tv->metrics.y, tv->metrics.y + tv->metrics.height-1);
				drawRectangleConstrained(frame, x, y, x, y, 255<<24 | COL_WHITE, tv->metrics.y, tv->metrics.y + tv->metrics.height-1);

				tvTreeFreeItem(item);
				return post;
			}else{
			
				tvTreeFreeItem(item);
				return NULL;
			}
		}
	}
	return NULL;
}

static inline void plytvTreeRenderCutPaste (TTV *tv, TFRAME *frame, const int x, const int y)
{

	// render cursor highlights
	if (tv->dragEnabled && tv->inputEnabled){
		TTV_RENDER_ITEM *post = plytvRenderItemHighlight(tv, frame, x, y);
		if (post){
			if (tv->drag.dest.id != post->id){
				//tv->drag.dest = *post;
				my_memcpy(&tv->drag.dest, post, sizeof(TTV_RENDER_ITEM));
			}
			tv->drag.action = 1;
		}else{
			post = ptvRenderItemInsert(tv, frame, x, y);
			if (post){
				if (tv->drag.dest.id != post->id){
					//tv->drag.dest = *post;
					my_memcpy(&tv->drag.dest, post, sizeof(TTV_RENDER_ITEM));
				}
				tv->drag.action = 2;
			}else{
				tv->drag.dest.id = 0;
				tv->drag.action = 0;
			}
		}
			
		if (tv->drag.state && tv->drag.post.id){
			TTV_ITEM *item = tvTreeGetItem(tv, tv->drag.post.id, NULL);
			if (item){
				TTV_NODE_DESC_OPAQUE *opaque = tvItemOpaqueGet(item);
			
				if (opaque->objType == NODE_OBJTYPE_DETAIL){
					tv->drag.post.id = 0;
					tv->drag.state = 0;
					tv->drag.action = 0;
				}
				tvTreeFreeItem(item);
			}
		}
	}
}

static inline int plytvTreeRender (TVLCPLAYER *vp, TFRAME *frame, TPLYTV *plytv)
{
	TTV *tv = (TTV*)plytv->tv;

				
	//lSetForegroundColour(frame->hw, 0xFFFFFFFF);
	//shadowTextEnable(frame->hw, 255<<24|COL_BLUE_SEA_TINT, plytv->shadowPre);
	

	lDrawRectangleFilled(frame, 0, tv->metrics.y, frame->width-1, frame->height-1, (50<<24)|COL_BLACK);
	//plytvTreeRenderCutPaste(tv, frame, vp->gui.cursor.dx, vp->gui.cursor.dy);	

	
	// as a two pass renderer, we don't want the scroll position to change between passes
	tv->renderStartY = tv->sbVertPosition;
	
#if 0
	if (!ccGetState(plytv->imgovr))
		tv->renderFlags = TV_RENDER_LABELS/* | TV_RENDER_UNDERLAY */| TV_RENDER_CHECKBOX | TV_RENDER_EXPANDER;
	else
		tv->renderFlags = TV_RENDER_LABELS/* | TV_RENDER_UNDERLAY*/ | TV_RENDER_CHECKBOX | TV_RENDER_EXPANDER | TV_RENDER_IMAGES;

	ccRender(tv, frame);
#endif

	//plytvTreeRenderCutPaste(tv, frame, vp->gui.cursor.dx, vp->gui.cursor.dy);

	if (!tvRenameGetState(tv)){
		lSetForegroundColour(frame->hw, 255<<24 | COL_WHITE);
		shadowTextEnable(frame->hw, COL_BLUE_SEA_TINT, plytv->shadow);
	}else{
		lSetForegroundColour(frame->hw, 255<<24 | COL_BLUE_SEA_TINT);
		shadowTextEnable(frame->hw, COL_WHITE, 180);
	}

	if (!ccGetState(plytv->imgovr) && buttonsStateGet(plytv->btns,PLYTV_RENAME))
		tv->renderFlags = /*TV_RENDER_BLUR |*/ TV_RENDER_PASTEDEST|TV_RENDER_DRAGITEM|TV_RENDER_IDTEXT | TV_RENDER_LABELS | TV_RENDER_SCROLLWIDGETS | TV_RENDER_IMAGES | TV_RENDER_CHECKBOX | TV_RENDER_EXPANDER;
	else
		tv->renderFlags = /*TV_RENDER_BLUR |*/ TV_RENDER_PASTEDEST|TV_RENDER_DRAGITEM|TV_RENDER_IDTEXT;

	ccRender(tv, frame);

	plytvTreeRenderCutPaste(tv, frame, vp->gui.cursor.dx, vp->gui.cursor.dy);
	
	shadowTextEnable(frame->hw, 20<<8|20, plytv->shadow);		// reset attributes
	shadowTextDisable(frame->hw);

	return 1;
}

static inline void drawRectangle (TFRAME *frame, const int filled, const int x1, const int y1, const int x2, const int y2, const int colour)
{
	if (!filled)
		lDrawRectangle(frame, x1, y1, x2, y2, colour);
	else
		lDrawRectangleFilled(frame, x1, y1, x2, y2, colour);
}

static inline void drawTriangle (TFRAME *frame, const int filled, const int x1, const int y1, const int x2, const int y2, const int x3, const int y3, const int colour)
{
	if (!filled)
		lDrawTriangle(frame, x1, y1, x2, y2, x3, y3, colour);
	else
		lDrawTriangleFilled(frame, x1, y1, x2, y2, x3, y3, colour);
}

static inline void drawSwipeMarker (TFRAME *frame, const int cx, const int cy, const int cWidth, const int cHeight, const int colour, const int dx, const int dy, const int triHeight, const int filled)
{
	
	int x1 = cx - (cWidth>>1);
	int y1 = cy - (cHeight>>1);
	int x2 = x1 + cWidth;
	int y2 = y1 + cHeight;
	
	//drawRectangle(frame, 1, x1, y1, x2, y2, colour);
	
	const int y = cy - dy;
	if (y > cHeight){			// up
		drawRectangle(frame, filled, x1, y1-1, x2, dy, colour);
		
		int th = abs(y - cHeight);
		if (th > triHeight) th = triHeight;
		drawTriangle(frame, filled, x1+1+(triHeight-th), dy-2, x2-(triHeight-th), dy-2, cx, dy-th, colour);
		
		
	}else if (y < -cHeight){	// down
		drawRectangle(frame, filled, x1, y2+1, x2, dy, colour);	

		int th = abs(-y - cHeight);
		if (th > triHeight) th = triHeight;
		drawTriangle(frame, filled, x1+1+(triHeight-th), dy+1, x2-(triHeight-th), dy+1, cx, dy+th, colour);		
	}
}

static inline void tvTreeItemSelectTrack (TPLYTV *plytv, TTV *tv, TTV_ITEM *item, const int pid, const int position)
{
	TVLCPLAYER *vp = tv->cc->vp;
	//if (!item->tv) return;
	//printf("item %p %p\n", item, item->tv);
	
	// refresh artwork

	TTV_ITEM_DESC *desc = tvTreeItemGetDesc(item);	
	//printf(":tvTreeItemSelectTrack %X '%s' %p %i\n", item->id, item->name, desc->image, desc->imageTotal);

	TTV_ITEM_DESC_IMAGE *image = tvItemImageGet(desc, ITEM_DETAIL_IMAGE_ARTWORK);
	if (!image) return;
	
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, pid);
	//printf("tvTreeItemSelectTrack plc->name '%s'\n", plc->title);
	
	int noartId = plytv->artc.noart;
	if (!image->artId || image->artId == noartId){
		int id = playlistGetArtId(plc, position);
		if (id){
			image->artId = id;
			image->scale = g_imgArtSize / (double)vp->gui.artMaxHeight;
		}else{
			image->artId = noartId;
			image->scale = 1.0;
		}
	}
}

static inline void tvTreeImageSelectPlay (TTV *tv, TTV_ITEM *item, TTV_ITEM_DESC_IMAGE *image, const int pid, const int position)
{
	TVLCPLAYER *vp = tv->cc->vp;
	TPLYTV *plytv = pageGetPtr(vp, PAGE_PLY_TV);
	TTV_ITEM_DESC *desc = tvTreeItemGetDesc(item);
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, pid);

	//printf("@@@ playing %X %X\n", item->id, item->parentId);

	if (getPlayState(vp) == 2 && getQueuedPlaylist(vp) == plc && getPlayingItem(vp) == position){
		trackPlay(vp);		// is paused so unpause

		tvItemImageSetState(desc, ITEM_DETAIL_IMAGE_PLAY, TV_DRAWIMAGE_DONTRENDER);
		tvItemImageSetState(desc, ITEM_DETAIL_IMAGE_PAUSE, TV_DRAWIMAGE_ENABLED);
		tvItemImageSetState(desc, ITEM_DETAIL_IMAGE_STOP, TV_DRAWIMAGE_ENABLED);

		if (plytv->current && plytv->current != item->id){
			TTV_ITEM *citem = tvTreeGetItem(tv, plytv->current, NULL);
			if (citem){
				TTV_ITEM_DESC *cdesc = tvTreeItemGetDesc(citem);
				tvItemImageSetState(cdesc, ITEM_DETAIL_IMAGE_PLAY, TV_DRAWIMAGE_ENABLED);
				tvItemImageSetState(cdesc, ITEM_DETAIL_IMAGE_PAUSE, TV_DRAWIMAGE_DONTRENDER);
				tvItemImageSetState(cdesc, ITEM_DETAIL_IMAGE_STOP, TV_DRAWIMAGE_DONTRENDER);
				tvTreeFreeItem(citem);
			}
		}
		plytv->current = item->id;
			  	
	}else if (startPlaylistTrack(vp, plc, position)){
		if (plc){
	  		//vp->displayPlaylist = playlistManagerGetPlaylistIndex(vp->plm, plc);
	  		setDisplayPlaylist(vp, plc);
	  		plc->pr->selectedItem = position;
	  		invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), position);
	  	}else{
	  		vp->playlist.display = -1;
	  	}
		  	  			
	  	tvItemImageSetState(desc, ITEM_DETAIL_IMAGE_PLAY, TV_DRAWIMAGE_DONTRENDER);
		tvItemImageSetState(desc, ITEM_DETAIL_IMAGE_PAUSE, TV_DRAWIMAGE_ENABLED);
		tvItemImageSetState(desc, ITEM_DETAIL_IMAGE_STOP, TV_DRAWIMAGE_ENABLED);
	  	if (plytv->current && plytv->current != item->id){
	  		TTV_ITEM *citem = tvTreeGetItem(tv, plytv->current, NULL);
	  		if (citem){
				TTV_ITEM_DESC *cdesc = tvTreeItemGetDesc(citem);
	  			tvItemImageSetState(cdesc, ITEM_DETAIL_IMAGE_PLAY, TV_DRAWIMAGE_ENABLED);
				tvItemImageSetState(cdesc, ITEM_DETAIL_IMAGE_PAUSE, TV_DRAWIMAGE_DONTRENDER);
				tvItemImageSetState(cdesc, ITEM_DETAIL_IMAGE_STOP, TV_DRAWIMAGE_DONTRENDER);
				tvTreeFreeItem(citem);
			}
	  	}
		plytv->current = item->id;
  	}
}

static inline void tvTreeImageSelectPause (TTV *tv, TTV_ITEM *item, TTV_ITEM_DESC_IMAGE *image, const int pid, const int position)
{
	TVLCPLAYER *vp = tv->cc->vp;
	TPLYTV *plytv = pageGetPtr(vp, PAGE_PLY_TV);
	TTV_ITEM_DESC *desc = tvTreeItemGetDesc(item);
	
	tvItemImageSetState(desc, ITEM_DETAIL_IMAGE_PLAY, TV_DRAWIMAGE_ENABLED);
	tvItemImageSetState(desc, ITEM_DETAIL_IMAGE_PAUSE, TV_DRAWIMAGE_DONTRENDER);
	tvItemImageSetState(desc, ITEM_DETAIL_IMAGE_STOP, TV_DRAWIMAGE_ENABLED);
	
	if (plytv->current && plytv->current != item->id){
		TTV_ITEM *citem = tvTreeGetItem(tv, plytv->current, NULL);
		if (citem){
			TTV_ITEM_DESC *cdesc = tvTreeItemGetDesc(citem);
			tvItemImageSetState(cdesc, ITEM_DETAIL_IMAGE_PLAY, TV_DRAWIMAGE_ENABLED);
			tvItemImageSetState(cdesc, ITEM_DETAIL_IMAGE_PAUSE, TV_DRAWIMAGE_DONTRENDER);
			tvItemImageSetState(cdesc, ITEM_DETAIL_IMAGE_STOP, TV_DRAWIMAGE_DONTRENDER);
			tvTreeFreeItem(citem);
		}
	}
	plytv->current = item->id;
	
	trackPlayPause(vp);	
}

static inline void tvTreeImageSelectStop (TTV *tv, TTV_ITEM *item, TTV_ITEM_DESC_IMAGE *image, const int pid, const int position)
{
	TVLCPLAYER *vp = tv->cc->vp;
	TPLYTV *plytv = pageGetPtr(vp, PAGE_PLY_TV);
	TTV_ITEM_DESC *desc = tvTreeItemGetDesc(item);
	
  	tvItemImageSetState(desc, ITEM_DETAIL_IMAGE_PLAY, TV_DRAWIMAGE_ENABLED);
	tvItemImageSetState(desc, ITEM_DETAIL_IMAGE_PAUSE, TV_DRAWIMAGE_DONTRENDER);
	tvItemImageSetState(desc, ITEM_DETAIL_IMAGE_STOP, TV_DRAWIMAGE_DONTRENDER);
	
	if (plytv->current && plytv->current != item->id){
		TTV_ITEM *citem = tvTreeGetItem(tv, plytv->current, NULL);
		if (citem){
			TTV_ITEM_DESC *cdesc = tvTreeItemGetDesc(citem);
			tvItemImageSetState(cdesc, ITEM_DETAIL_IMAGE_PLAY, TV_DRAWIMAGE_ENABLED);
			tvItemImageSetState(cdesc, ITEM_DETAIL_IMAGE_PAUSE, TV_DRAWIMAGE_DONTRENDER);
			tvItemImageSetState(cdesc, ITEM_DETAIL_IMAGE_STOP, TV_DRAWIMAGE_DONTRENDER);
			tvTreeFreeItem(citem);
		}
	}
	plytv->current = 0;
	trackStop(vp);
}

static inline void tvTreeImageSelectArtwork (TTV *tv, TTV_ITEM *item, TTV_ITEM_DESC_IMAGE *image, const int pid, const int position)
{
	TVLCPLAYER *vp = tv->cc->vp;
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, pid);
	
	int artId = playlistGetArtId(plc, position);
	if (artId){
		//printf("plytv: found art for %X, %X, %X\n", item->id, artId, artId);
		tvInputDisable(tv);

		TPLYTV *plytv = pageGetPtr(vp, PAGE_PLY_TV);
		plytvButtonsSetState(plytv, plytv->btns, 0);
		labelArtcSet(plytv->imgovr, plytv->imgovrArtId, artId, 1);
		
		int x = (getFrontBuffer(vp)->width - ccGetWidth(plytv->imgovr))/2;
		int y = (getFrontBuffer(vp)->height - ccGetHeight(plytv->imgovr))/2;
		ccSetPosition(plytv->imgovr, x, y);

		labelItemEnable(plytv->imgovr, plytv->imgovrArtId);
		labelItemDisable(plytv->imgovr, plytv->imgovrImageId);
		ccEnable(plytv->imgovr);
		
		tvTreeItemSelectTrack(plytv, tv, item, pid, position);
	//}else{
	//	printf("noartwork for %i:%s\n", position, plc->title);
	}
	
}

static inline void tvTreeItemSelectDetail (TTV *tv, TTV_ITEM *item, const int pid, const int position)
{
	//printf("tvTreeItemSelectDetail\n");
	
	TVLCPLAYER *vp = tv->cc->vp;
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, pid);
	if (!plc) return;
	
	TPLYTV *plytv = pageGetPtr(vp, PAGE_PLY_TV);
	TMETA *meta = pageGetPtr(vp, PAGE_META);
	TMETADESC desc;
	metaCopyDesc(&desc, meta);

	desc.w = vp->ml->front->width - 16;
	desc.h = vp->ml->front->height - 16;
	desc.x = 0;
	desc.y = 0;
	desc.trackPosition = position;
	desc.textHeight = 0;
	desc.uid = pid;

	TFRAME *image = lNewFrame(vp->ml->hw, desc.w, desc.h, LFRM_BPP_32A);
	lClearFrameClr(image, 100<<24 | COL_BLACK);
	
	metaRender(vp, image, meta, &desc, META_FONT, -1);

	labelImageSet(plytv->imgovr, plytv->imgovrImageId, image, 1);
	lDeleteFrame(image);
		
	int x = (vp->ml->front->width - ccGetWidth(plytv->imgovr))/2;
	int y = (vp->ml->front->height - ccGetHeight(plytv->imgovr))/2;
	ccSetPosition(plytv->imgovr, x, y);
	
	labelItemDisable(plytv->imgovr, plytv->imgovrArtId);
	labelItemEnable(plytv->imgovr, plytv->imgovrImageId);
	ccEnable(plytv->imgovr);

	// stop the treeview control under the artwork image from responding to input
	// until artwork is closed
	tvInputDisable(tv);

	plytvButtonsSetState(plytv, plytv->btns, 0);

}

static inline void tvTreeItemInputCallback (TPLYTV *plytv, TTV *tv, TTV_ITEM *item, const int type, TTV_NODE_DESC_OPAQUE *opaque)
{
	int wantUpdate = 0;
	  	
	//printf("tvTreeItemInputCallback type: %i %p\n", type, opaque);
	  	
	switch (type){
	  case NODE_OBJTYPE_PLC:{
	  	//printf("NODE_OBJTYPE_PLC\n");
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(tv->cc->vp->plm, opaque->playlist.pid);
		
		if (plc){
			//printf("count: %i %i, %i %i\n", tvTreeCountItems(tv, item->id), playlistGetTotal(plc), plc->uid, item->id);
			if (playlistLock(plc)){
				const int total = tvTreeCountItems(tv, item->id);
				if (total < 1 || plc->total != total || plc->uid != item->id){
					tvTreeDeleteItems(tv, item->id);
					tvTreeAddPlc(plytv, tv, item->id, plc, 0);
				
					TTV_ITEM_DESC *desc = tvTreeItemGetDesc(item);
					if (desc->checkbox.checkState == TV_CHECKBOX_CHECKED){
						TTV_ITEM *itemUpdated = tvTreeGetItem(tv, item->id, NULL);
						if (itemUpdated){
							tvSetCheckNode(tv, itemUpdated, TV_CHECKBOX_CHECKED);
							tvTreeFreeItem(itemUpdated);
						}
					}
				}
				playlistUnlock(plc);
			}
		}
		break;
	  }

	  case NODE_OBJTYPE_TRACK:{
	  	//printf("NODE_OBJTYPE_TRACK %X\n", item->children[0]);
	  	TTV_ITEM *subItem = tvTreeGetItem(tv, item->children[0], item->entry);
	  	if (subItem){
	  		tvTreeItemSelectTrack(plytv, tv, subItem, opaque->track.pid, opaque->track.position);
	  		tvTreeFreeItem(subItem);
	  	}
		break;
	  }

	  case NODE_OBJTYPE_DETAIL:
	  	//printf("NODE_OBJTYPE_DETAIL\n");
		tvTreeItemSelectDetail(tv, item, opaque->detail.pid, opaque->detail.position);
		break;
	  
	  case NODE_OBJTYPE_IMAGE:{
	  	TTV_ITEM_DESC *desc = tvTreeItemGetDesc(item);
		TTV_ITEM_DESC_IMAGE *image = tvItemImageGet(desc, opaque->image.type);
	  	
		switch (opaque->image.type){
		  case ITEM_DETAIL_IMAGE_ARTWORK:
			tvTreeImageSelectArtwork(tv, item, image, opaque->image.pid, opaque->image.position);
			wantUpdate = 1;
			break;

		  case ITEM_DETAIL_IMAGE_PLAY:
		  	tvTreeImageSelectPlay(tv, item, image, opaque->image.pid, opaque->image.position);
		  	wantUpdate = 1;
			break;

		  case ITEM_DETAIL_IMAGE_PAUSE:
		  	tvTreeImageSelectPause(tv, item, image, opaque->image.pid, opaque->image.position);
		  	wantUpdate = 1;
			break;

		  case ITEM_DETAIL_IMAGE_STOP:
		  	tvTreeImageSelectStop(tv, item, image, opaque->image.pid, opaque->image.position);
		  	wantUpdate = 1;
			break;

		  default:
			//printf("node objtype image INVALID: %p %p %i\n", item, image, opaque->image.type);
		  	break;
		}

	  	//printf("node objtype image %p %p %i\n", item, image, opaque->image.type);
		break;
	  }
	}
	
	if (wantUpdate)
		renderSignalUpdate(tv->cc->vp);

}

static inline int tvTreeCutPaste (TTV *tv, TTV_ITEM *src, TTV_ITEM *dst, const int pasteType)
{
	const int from = src->id;
	const int to = dst->id;

	//printf("tvTreeCutPaste %X %X %i\n", from, to, pasteType);

	TTV_NODE_DESC_OPAQUE *srcData = tvItemOpaqueGet(src);
	PLAYLISTCACHE *plcSrc = playlistManagerGetPlaylistByUID(tv->cc->vp->plm, opaqueGetPlaylist(srcData));
	int srcIdx = opaqueGetTrack(srcData);
			
	TTV_NODE_DESC_OPAQUE *dstData = tvItemOpaqueGet(dst);
	PLAYLISTCACHE *plcDst = playlistManagerGetPlaylistByUID(tv->cc->vp->plm, opaqueGetPlaylist(dstData));
	int dstIdx = opaqueGetTrack(dstData);
		
	const int srcNodeType = srcData->objType;
	const int dstNodeType = dstData->objType;
	const int isChild = playlistIsChild(plcDst, plcSrc);


	if (srcNodeType == NODE_OBJTYPE_PLC && isChild){
		//printf("is child, returning\n");
		return 0;
	}


	// move treeview items
	if (pasteType == 2){		// drop above the playlist or track
		tvTreeEntryMove(tv, from, to);
		
		TTV_ITEM *srcParent = tvTreeGetItem(tv, src->parentId, NULL);
		if (srcParent){
			tvTreeResyncItems(tv, srcParent, 0);
			tvTreeFreeItem(srcParent);
		}
		
		if (srcNodeType == NODE_OBJTYPE_TRACK && dstNodeType == NODE_OBJTYPE_PLC){
			TTV_ITEM *dstParent = tvTreeGetItem(tv, dst->parentId, NULL);
			if (dstParent){
				tvTreeResyncItems(tv, dstParent, plcDst->parent->uid);
				tvTreeFreeItem(dstParent);
			}
		}else if (srcNodeType == NODE_OBJTYPE_PLC && dstNodeType == NODE_OBJTYPE_PLC){
			TTV_ITEM *dstParent = tvTreeGetItem(tv, dst->parentId, NULL);
			if (dstParent){
				tvTreeResyncItems(tv, dstParent, plcDst->parent->uid);
				tvTreeFreeItem(dstParent);
			}
		}else{
			TTV_ITEM *dstParent = tvTreeGetItem(tv, dst->parentId, NULL);
			if (dstParent){
				tvTreeResyncItems(tv, dstParent, plcDst->uid);
				tvTreeFreeItem(dstParent);
			}
		}
	}else if (pasteType == 1){	// drop in to the playlist
		//printf("'%s' : '%s'\n", plcSrc->title, plcDst->title);
		tvTreeEntryMoveToTail(tv, from, to);
		
		TTV_ITEM *srcParent = tvTreeGetItem(tv, src->parentId, NULL);
		if (srcParent){
			tvTreeResyncItems(tv, srcParent, 0);
			tvTreeFreeItem(srcParent);
		}
		TTV_ITEM *dstParent = tvTreeGetItem(tv, dst->parentId, NULL);
		if (dstParent){
			tvTreeResyncItems(tv, dstParent, plcDst->uid);
			tvTreeFreeItem(dstParent);		
		}

	}
	
	
	// move playlist items
	if (srcNodeType == NODE_OBJTYPE_PLC){
		if (dstNodeType == NODE_OBJTYPE_PLC){
			if (pasteType == 1){			// drop in to the playlist
				playlistMove(plcSrc->parent, plcDst, srcIdx, plcDst->total);
				srcData->playlist.position = plcDst->total-1;
				plcSrc->parent = plcDst;
				
			}else if (pasteType == 2){		// drop above the playlist
				if (plcSrc->parent == plcDst->parent && dstIdx > srcIdx)
					dstIdx--;

				playlistMove(plcSrc->parent, plcDst->parent, srcIdx, dstIdx);
				plcSrc->parent = plcDst->parent;
			}
		}else if (dstNodeType == NODE_OBJTYPE_TRACK){
			if (plcSrc->parent == plcDst && dstIdx > srcIdx)
				dstIdx--;

			playlistMove(plcSrc->parent, plcDst, srcIdx, dstIdx);
			plcSrc->parent = plcDst;
		}
	}else if (srcNodeType == NODE_OBJTYPE_TRACK){
		if (dstNodeType == NODE_OBJTYPE_PLC){
			if (pasteType == 1){			// drop in to the playlist
				playlistMove(plcSrc, plcDst, srcIdx, plcDst->total);
				srcData->track.pid = plcDst->uid;
							
			}else if (pasteType == 2){		// drop above the playlist
				if (plcSrc == plcDst->parent && dstIdx > srcIdx)
					dstIdx--;

				//int ptotal = playlistGetTotal(plcDst->parent);
				playlistMove(plcSrc, plcDst->parent, srcIdx, dstIdx);
				//if (dstIdx == ptotal-1)
				//	playlistMove(plcDst->parent, plcDst->parent, ptotal-1, ptotal);
				srcData->track.position = dstIdx;
				srcData->track.pid = plcDst->parent->uid;
			}
		}else if (dstNodeType == NODE_OBJTYPE_TRACK){	
			if (plcSrc == plcDst && dstIdx > srcIdx)
				dstIdx--;
			
			//int ptotal = playlistGetTotal(plcDst);
			playlistMove(plcSrc, plcDst, srcIdx, dstIdx);
			//if (dstIdx == ptotal-1)
			//	playlistMove(plcDst, plcDst, ptotal-1, ptotal);

			srcData->track.pid = dstData->track.pid;
		}
	}
	return 1;
}


static inline int64_t labelccobject_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER) return 1;

	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	//printf("labelccobject_cb. id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, data1, data2, dataPtr);
	
	if (msg == CC_MSG_ENABLED){
		TLABEL *label = (TLABEL*)obj;
		TPLYTV *plytv = pageGetPtr(label->cc->vp, PAGE_PLY_TV);
		if (plytv->tv){
			tvInputDisable(plytv->tv);
			ccDisable(plytv->tv->sbVert);
		}
		
	}else if (msg == CC_MSG_DISABLED){
		TLABEL *label = (TLABEL*)obj;
		TPLYTV *plytv = pageGetPtr(label->cc->vp, PAGE_PLY_TV);
		if (plytv->tv){
			plytv->tv->drag.post.id = 0;
			plytv->tv->drag.dest.id = 0;
			tvInputEnable(plytv->tv);
			ccEnable(plytv->tv->sbVert);
		}
		
	}else if (msg == LABEL_MSG_IMAGE_SELECTED_PRESS){
		TTOUCHCOORD *pos = (TTOUCHCOORD*)dataPtr;
		if (pos->dt > 140){
			TLABEL *label = (TLABEL*)obj;
			
			TPLYTV *plytv = pageGetPtr(label->cc->vp, PAGE_PLY_TV);
			swipeReset(&plytv->swipe);

			plytvButtonsSetState(plytv, plytv->btns, 1);
			ccDisable(label);
		}
	}
	return 1;
}

static inline int64_t plytvccobject_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER) return 1;

	TCCOBJECT *obj = (TCCOBJECT*)object;
	TTV *tv = (TTV*)obj;
	TVLCPLAYER *vp = tv->cc->vp;
	TPLYTV *plytv = pageGetPtr(vp, PAGE_PLY_TV);
	
	//printf("plytvccobject_cb. id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);

	
	switch (msg){
	  case KP_MSG_PAD_OPENED:{
	  	//printf("plytv KP_MSG_PAD_OPENED\n");
	  	
	  	tvInputDisable(tv);
	  	ccDisable(tv->sbVert);
	  	plytvButtonsSetState(plytv, plytv->btns, 0);

	  	break;
	  }
	  case KP_MSG_PAD_CLOSED:{
	  	//printf("plytv KP_MSG_PAD_CLOSED\n");
	  	
	  	if (!tvRenameGetState(tv))
	  		plytvButtonsSetState(plytv, plytv->btns, 1);

		buttonsStateSet(plytv->btns, PLYTV_RENAME, 1);
	  	ccEnable(tv->sbVert);
	  	tvInputEnable(tv);

	  	TKEYBOARD *vkey = pageGetPtr(vp, PAGE_VKEYBOARD);
	  	keypadListenerRemove(vkey->kp, tv->id);

		page2Set(vp->pages, PAGE_VKEYBOARD, 0);		// remove concurrent
	  	if (page2RenderGetState(vp->pages, PAGE_PLY_TV))
	  		page2RenderDisable(vp->pages, PAGE_VKEYBOARD);
	  	else
	  		page2SetPrevious(vkey);
	  	
	  	//tvRenameDisable(tv);
	  	break;
	  }
	  case KP_MSG_PAD_PRESS:
	  	//printf("KP_MSG_PAD_PRESS\n");
	  	break;
	  
	  case KP_MSG_PAD_ENTER:{
	  	//printf("KP_MSG_PAD_ENTER\n");
	  	
	 	if ((int)data1 != KP_INPUT_COMPLETE8)
	 		return 0;
	 	
		//printf("plytv  KP_MSG_PAD_ENTER: %I64d %I64d %p: '%s'\n", data1, data2, dataPtr, (char*)dataPtr);
		
		int id = (int)data2;
		char *newTitle = (char*)dataPtr;
		
		if (!newTitle) return 0;
		if (strlen(newTitle) < 1) return 0;

		TTV_ITEM *item = tvTreeGetItem(tv, id, NULL);
		//printf("item %X %p\n", id, item);
		
		if (item){
			TTV_NODE_DESC_OPAQUE *opaque = tvItemOpaqueGet(item);
			//printf("opaque %X %p\n", id, opaque);
			
			if (opaque){
				int uid = opaqueGetPlaylist(opaque);
				int track = opaqueGetTrack(opaque);
				int type = opaqueGetType(opaque);
				char *newTitle = (char*)dataPtr;

				if (type == NODE_OBJTYPE_PLC){
	  				PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	  				//printf("playlist %i: %X,%i '%s'\n", type, uid, track, plc->title);
	  				
	  				if (plc){
  						//printf("newTitle '%s'\n", newTitle);
  						if (playlistSetName(plc, newTitle))
  							treeRenameItem(tv, id, newTitle);
	  				}
	  			}else if (type == NODE_OBJTYPE_TRACK){
	  				PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	  				//printf("track %i: %X,%i '%s'\n", type, uid, track, plc->title);

	  				if (plc){
  						//printf("newTitle '%s'\n", newTitle);
  						treeRenameItem(tv, id, newTitle);

						int saved = 0;
						char *path = playlistGetPathDup(plc, track);
						if (path){
							playlistSetTitle(plc, track, newTitle, 1);
							tagAdd(tv->cc->vp->tagc, path, MTAG_Title, newTitle, 1);							
							
							libvlc_media_t *m = libvlc_media_new_path(vp->vlc->hLib, path);
							if (m){
								libvlc_media_set_meta(m, MTAG_Title, dataPtr);
								saved = libvlc_media_save_meta(m);
								libvlc_media_release(m);
							}
							my_free(path);
						}
						if (!saved)
							dbprintf(vp, "Title not written to %X %i", uid, track+1);
	  				}
	  			}
	  		}
	  		tvTreeFreeItem(item);
		}
	  	break;
	  	
	  }
#if 0
	  case CC_MSG_CREATE:
		//printf("TV_MSG_CREATE: %i, %i, %p\n", data1, data2, dataPtr);
		break;
	  case CC_MSG_INPUT:
		//printf("TV_MSG_INPUT: %i, %i, %p\n", data1, data2, dataPtr);
		break;
	  case CC_MSG_SETPOSITION:
		//printf("TV_MSG_SETPOSITION: %i, %i, %p\n", data1, data2, dataPtr);
		break;
	  case CC_MSG_ENABLED:
		//printf("TV_MSG_ENABLED: %i, %i, %p\n", data1, data2, dataPtr);
		break;
	  case CC_MSG_DISABLED:
		//printf("TV_MSG_DISABLED: %i, %i, %p\n", data1, data2, dataPtr);
		break;				
	  case TV_MSG_SB_INPUT:{
		//if (tree->renderStartY != tree->sbVertPosition)
		//	renderSignalUpdate(vp);
		//TPLYTV *plytv = pageGetPtr(tv->cc->vp, PAGE_PLY_TV);
		//swipeReset(&plytv->swipe);
		break;
	  }
#endif
	  case TV_MSG_ITEMDROP:
		//printf("TV_MSG_ITEMDROP: %i, %i, %p\n", data1, data2, dataPtr);
		
		if (ccLock(tv)){
			TTV_INPUT_DROP *tvid = (TTV_INPUT_DROP*)dataPtr;

			TTV_ITEM *src = tvTreeGetItem(tv, tvid->from, NULL);
			if (src){
				TTV_ITEM *dst = tvTreeGetItem(tv, tvid->to, NULL);
				if (dst){
					tvTreeCutPaste(tv, src, dst, tvid->action);
					tvTreeFreeItem(dst);

					timerSet(tv->cc->vp, TIMER_PLYALB_REFRESH, 100);
					timerSet(tv->cc->vp, TIMER_PLYPAN_REBUILD, 100);
					
				}
				tvTreeFreeItem(src);
				plytvTvRenderRebuild(tv);
			}
			ccUnlock(tv);
			//timerSet(vp, TIMER_PLYPAN_REBUILD, 0);
			renderSignalUpdate(tv->cc->vp);
		}
		break;
		
	  case TV_MSG_EXPANDERSTATE:
		//printf("TV_MSG_EXPANDERSTATE: %i, %X, %p\n", data1, data2, dataPtr);

		if (data1 == TV_EXPANDER_OPEN){
			//const int id = data2;
			//if (ccLock(tree)){
				
				TTV_ITEM *item = (TTV_ITEM*)dataPtr;// tvTreeGetItem(tv, id);
				if (item){
					TTV_NODE_DESC_OPAQUE *opaque = tvItemOpaqueGet(item);
				
					if (opaque->objType != NODE_OBJTYPE_DETAIL){
						tvTreeItemInputCallback(plytv, tv, item, opaque->objType, opaque);
					}else{
						TTV_ITEM_DESC *desc = tvTreeItemGetDesc(item);
						desc->expander.expanderState = TV_EXPANDER_CLOSED;
					}
					//tvTreeFreeItem(item);
				}
				//ccUnlock(tree);
			//}
		}else if (data1 == TV_EXPANDER_CLOSED){
			plytv->current = 0;
		}

		break;
	  //case TV_MSG_CHECKBOXSTATE:
		//printf("TV_MSG_CHECKBOXSTATE: %i, %i, %p\n", data1, data2, dataPtr);
		//break;
		
	  case TV_MSG_ITEMSELECT:{
		//printf("TV_MSG_ITEMSELECT: %i, %X, %p\n", data1, data2, dataPtr);

		if (ccLock(tv)){
			const int id = data2;
			TTV_ITEM *item = tvTreeGetItem(tv, id, NULL);
			if (item){
				TTV_NODE_DESC_OPAQUE *opaque = tvItemOpaqueGet(item);
				tvTreeItemInputCallback(plytv, tv, item, opaque->objType, opaque);
				tvTreeFreeItem(item);
			}
			ccUnlock(tv);
		}
		break;
	  }
	  case TV_MSG_IMAGESELECT:{
		//printf("TV_MSG_IMAGESELECT: %i, %X, %p\n", data1, data2, dataPtr);

		if (ccLock(tv)){
			const int id = data2;

			TTV_ITEM *subItem = tvTreeGetItem(tv, id, NULL);
			if (subItem){
				TTV_ITEM_DESC *desc = tvTreeItemGetDesc(subItem);
				TTV_ITEM_DESC_IMAGE *image = tvItemImageGet(desc, data1);
				TTV_NODE_DESC_OPAQUE *opaque = tvItemImageOpaqueGet(image);

				//TTV_ITEM_DESC *desc = tvTreeItemGetDesc(item);
				//printf("opaque->objType %i\n", opaque->objType);
				tvTreeItemInputCallback(plytv, tv, subItem, opaque->objType, opaque);
			
				tvTreeFreeItem(subItem);
			}
			ccUnlock(tv);
		}

		break;
	  }
	  //default:
	  	//printf("plytvccobject_cb. id:%i/%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, ccGetId(vp, CCID_TV_PLAYLIST), obj->type, msg, data1, data2, dataPtr);
	  //	break;
	}
	return 1;
}

int plytvTouch (TTOUCHCOORD *pos, int flags, TVLCPLAYER *vp)
{
	static unsigned int lastId = 0;

	if (!flags){		// pen down
		if (lastId >= pos->id)
			return 0;
		lastId = pos->id;
	}else if (lastId != pos->id){
		return 0;
	}
	
	
	TPLYTV *plytv = pageGetPtr(vp, PAGE_PLY_TV);
	TTV *tv = plytv->tv;
	TTOUCHSWIPE *swipe = &plytv->swipe;

	if (!tv->inputEnabled) return 0;

	//printf("plytv %i %i %i %i, %i %i %i\n", tv->dragEnabled, pos->pen, swipe->state, flags, pos->x, pos->y, ccPositionIsOverlapped(tv, pos->x, pos->y));

	if (ccLock(plytv->tv)){
		if (!tv->dragEnabled){
			if (!pos->pen && !flags && pos->dt > 0){			// pen down
				//printf("%i %i %i\n", (int)tv->sbVertPosition, (int)swipe->u64value, (int)scrollbarGetFirstItem(tv->sbVert));
				//swipeReset(swipe);
				//swipe->u64value = tv->sbVertPosition;
				
				if (ccPositionIsOverlapped(tv, pos->x, pos->y)){
					//swipeReset(swipe);
					swipe->sx = pos->x;
					swipe->sy = pos->y;
					swipe->u64value = scrollbarGetFirstItem(tv->sbVert);
					
					swipe->adjust = 0.0;
					swipe->decayAdjust = 0.0;
					swipe->t0 = 0.0;//getTime(vp);
					swipe->state = SWIPE_DOWN;
					//printf("\npen down\n\n");
				}else{
					swipe->state = SWIPE_UP;
					swipeReset(swipe);
					//swipe->u64value = tv->sbVertPosition;
				}
			}else if (!pos->pen && (swipe->state == SWIPE_DOWN || swipe->state == SWIPE_SLIDE) && flags == 1){	// slide/drag/move
				if (swipe->t0 < 0.0)
					swipe->t0 = getTime(vp);
				swipe->state = SWIPE_SLIDE;
				swipe->ex = pos->x;
				swipe->ey = pos->y;
				swipe->dx = swipe->ex - swipe->sx;
				swipe->dy = swipe->ey - swipe->sy;

				//printf("drag: %i\n", swipe->dy);
			
				if (abs(swipe->dy) >= swipe->dragMinV){
					swipe->adjust = -swipe->dy * swipe->velocityFactor;
					ccUnlock(plytv->tv);
					return 0;
				}
			}else if (pos->pen && (swipe->state == SWIPE_DOWN || swipe->state == SWIPE_SLIDE) && flags == 3){	// pen up
				swipe->state = SWIPE_UP;
				swipeReset(swipe);
				
				swipe->decayAdjust = 0.0;
				swipe->adjust = 0.0;
				swipe->dy = 0;
				swipe->dt = 0;
				swipe->u64value = scrollbarGetFirstItem(tv->sbVert);
				tv->sbVertPosition = swipe->u64value;
				
				//printf("up %i\n\n", (int)swipe->u64value);
				
				setTargetRate(vp, UPDATERATE_BASE);
				ccUnlock(plytv->tv);
				renderSignalUpdate(vp);
				return 0;
			}
		}else{
			swipeReset(swipe);
		}
		ccUnlock(plytv->tv);
	}

	return 1;
}

static inline void swapInt (int *a, int *b)
{
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}

static inline void blurMarker (TFRAME *frame, int x1, int y1, int x2, int y2)
{
	//if (x2 < x1) swapInt(&x1, &x2);

	if (y2 < y1){
		swapInt(&y1, &y2);
		y1 -= 30;
		y2 += 4;
	}else{
		y1 -= 4;
		y2 += 30;
	}

	lBlurArea(frame, x1, y1, x2, y2, 3);
}

static inline int page_plytvRender (TPLYTV *plytv, TVLCPLAYER *vp, TFRAME *frame)
{
	TTV *tv = (TTV*)plytv->tv;

	//int inputEnabled = 0;

	TTOUCHSWIPE *swipe = &plytv->swipe;
	if (swipe->state == SWIPE_SLIDE){
		int colour = 60<<24 | COL_WHITE;
		for (int i = 1; i <= 12; i++)
			drawSwipeMarker(frame, swipe->sx, swipe->sy, 56-i, 16-(i<<1), colour, swipe->ex, swipe->ey, 28, 0);

		colour = 157<<24 | COL_BLUE_SEA_TINT;
		for (int i = 1; i <= 14; i++)
			drawSwipeMarker(frame, swipe->sx, swipe->sy, 48-i, i, colour, swipe->ex, swipe->ey, 20, 0);

		colour = 157<<24 | COL_WHITE;
		for (int i = 0; i <= 32; i++)
			drawSwipeMarker(frame, swipe->sx, swipe->sy, i, 14+(32-i), colour, swipe->ex, swipe->ey, 12, 0);

		blurMarker(frame, swipe->sx-28, swipe->sy, swipe->sx+29, swipe->ey);
	}

	if (ccLock(tv)){
		plytvTreeRender(vp, frame, plytv);
		//inputEnabled = tv->inputEnabled;
		ccUnlock(tv);
	}

	const unsigned int dt = buttonsRenderAll(plytv->btns, frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);
	ccRender(plytv->imgovr, frame);

	TVIDEOOVERLAY *playctrl = pageGetPtr(vp, PAGE_OVERLAY);
	marqueeDraw(vp, frame, playctrl->marquee, 2, 0);


	if (swipe->state == SWIPE_DOWN || swipe->state == SWIPE_SLIDE){
		if (ccLock(tv)){
			volatile const int adjust = (int)swipe->adjust;
			if (abs(adjust) >= swipe->dragMinV>>1){
				int64_t value = swipe->u64value - adjust;
				tv->sbVertPosition = scrollbarSetFirstItem(tv->sbVert, value);
			
				//if (abs(tv->sbVertPosition - value)) swipeReset(swipe);
				value = tv->sbVertPosition;
				swipe->u64value = value;
			}
			ccUnlock(tv);
		}

		//if (swipe->decayAdjust <= 0.0) swipe->adjust = 0.0;
		if (swipe->adjust != 0.00 || dt < 1000)
			setTargetRate(vp, 25);
		else
			setTargetRate(vp, UPDATERATE_BASE);
	}
	return 1;
}

static inline int page_plytvInput (TPLYTV *plytv, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_TOUCH_DOWN:
	  case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  	plytvTouch(pos, flags, vp);
	  	break;
	  	
  	  case PAGE_IN_WHEEL_FORWARD:
  		swipeReset(&plytv->swipe);
		plytv->tv->sbVertPositionRate = (1*39);//+2;
		tvScrollUp(plytv->tv);
  		break;

  	  case PAGE_IN_WHEEL_BACK:
	  	swipeReset(&plytv->swipe);
		plytv->tv->sbVertPositionRate = (1*39);//+2;
		tvScrollDown(plytv->tv);
	  	break;
	}
	
	return 1;
}

static inline int page_plytvStartup (TPLYTV *plytv, TVLCPLAYER *vp, const int fw, const int fh)
{
	int x = 10;
	int y = fh * 0.1768;

	TTV *tv = ccCreate(vp->cc, PAGE_PLY_TV, CC_TV, plytvccobject_cb, &vp->gui.ccIds[CCID_TV_PLAYLIST], 0, 0);
	int width = fw - x;
	int height = fh - y;
	//if (height > 394) height = 394;
	
	tvScrollbarSetWidth(tv, SCROLLBAR_VERTWIDTH);
	ccSetMetrics(tv, x, y, width, height);
	tvTreeCreate(tv, PLAYLIST_PRIMARY, PLAYLIST_UID_BASE+1);
	tvRenderSetNodeIdLocation(tv, (fw/2)+96, tv->metrics.y-70);
	tvRenderSetNodeIdFont(tv, UPDATERATE_FONT);
	tvRenderSetNodeIdColour(tv, COL_WHITE);
	ccDisable(tv);

	plytv->tv = tv;
	plytv->buildNo = 0;
	plytv->current = 0;
	plytv->shadowPre = 200;
	plytv->shadow = 240;
	plytv->imgArtSize = g_imgArtSize;

	TTOUCHSWIPE *swipe = &plytv->swipe;
	swipe->state = SWIPE_UP;
	swipe->t0 = 0.0;	// drag state time
	swipe->dt = 0.0;	// drag end time
	swipe->sx = 0;		// start
	swipe->sy = 0;
	swipe->ex = 0;		// end
	swipe->ey = 0;
	swipe->dx = 0;		// delta
	swipe->dy = 0;
	swipe->adjust = 0.0;
	swipe->dragMinV = 8;
	swipe->dragMinH = swipe->dragMinV/2;
	swipe->velocityFactor = 1.00;
	swipe->decayRate = 1.0; //0.010;
	swipe->decayFactor = 1.0;//10.000;

	return 1;
}

static inline int page_plytvInitalize (TPLYTV *plytv, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_PLY_TV);
	settingsGet(vp, "artwork.panelImgSize", &g_imgArtSize);

	if (g_imgArtSize < 10) g_imgArtSize = 110;
	plytv->imgArtSize = g_imgArtSize;
	
	return 1;
}

void page_plytvRenderInit (TPLYTV *plytv, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	
	const int fw = frame->width;
	//const int fh = frame->height;
	
	int	y = 10;
	const int btnSpace = 16;

	TTV *tv = plytv->tv;
	plytv->btns = buttonsCreate(vp->cc, PAGE_PLY_TV, PLYTV_TOTAL, ccbtn_cb);

	TCCBUTTON *btn = buttonsCreateButton(plytv->btns, L"plytv/rename.png", NULL, PLYTV_RENAME, 1, 1, ccGetPositionX(tv) + 8, y);
	btn = buttonsCreateButton(plytv->btns, L"plytv/expand.png", NULL, PLYTV_EXPAND_OPENALL, 1, 1, ccButtonGetXWidth(btn) + btnSpace, y);
	btn = buttonsCreateButton(plytv->btns, L"plytv/collapse.png", NULL, PLYTV_EXPAND_CLOSEALL, 1, 1, ccButtonGetXWidth(btn) + btnSpace, y);
	btn = buttonsCreateButton(plytv->btns, L"plytv/delete.png", NULL, PLYTV_DELETE, 1, 1, ccButtonGetXWidth(btn) + btnSpace, y);
	btn = buttonsCreateButton(plytv->btns, L"plytv/refresh.png", NULL, PLYTV_RELOAD, 1, 1, ccButtonGetXWidth(btn) + btnSpace, y);
	btn = buttonsCreateButton(plytv->btns, L"plytv/locked.png", NULL, PLYTV_LOCKED, 1, 0, ccButtonGetXWidth(btn) + btnSpace, y);
	btn = buttonsCreateButton(plytv->btns, L"plytv/unlocked.png", NULL, PLYTV_UNLOCKED, 0, 0, ccGetPositionX(btn), y);
	btn = buttonsCreateButton(plytv->btns, L"plytv/scrollup.png", NULL, PLYTV_SCROLLUP, 1, 1, ccButtonGetXWidth(btn) + btnSpace + 32, y);
	btn = buttonsCreateButton(plytv->btns, L"plytv/scrolldown.png", NULL, PLYTV_SCROLLDOWN, 1, 1, ccButtonGetXWidth(btn) + btnSpace + 8, y);
	btn = buttonsCreateButton(plytv->btns, L"plytv/close.png", NULL, PLYTV_BACK, 1, 0, 0, 0);
	ccSetPosition(btn, fw - ccGetWidth(btn) - 8, y);
	
	
	wchar_t buffer[MAX_PATH+1];
	plytv->artc.play = artManagerImageAdd(vp->am, buildSkinD(vp,buffer,L"plytv/play.png"));
	plytv->artc.pause = artManagerImageAdd(vp->am, buildSkinD(vp,buffer,L"plytv/pause.png"));
	plytv->artc.stop = artManagerImageAdd(vp->am, buildSkinD(vp,buffer,L"plytv/stop.png"));
	plytv->artc.noart = artManagerImageAdd(vp->am, buildSkinD(vp,buffer,L"plytv/noart.png"));
	
	//create track item label object to contain meta details and artwork
	plytv->imgovr = ccCreate(vp->cc, PAGE_PLY_TV, CC_LABEL, labelccobject_cb, &vp->gui.ccIds[CCID_LABEL_IMGOVR], 8, 8);
	TFRAME *tmp = lNewFrame(getFrontBuffer(vp)->hw, 8, 8, getFrontBuffer(vp)->bpp);
	plytv->imgovrImageId = labelImageCreate(plytv->imgovr, tmp, 0, 0, 0);
	lDeleteFrame(tmp);
	plytv->imgovrArtId = labelArtcCreate(plytv->imgovr, plytv->artc.noart, 1.0, 0, 0, 0);

	labelRenderFlagsSet(plytv->imgovr, /*LABEL_RENDER_BORDER_POST |*/ LABEL_RENDER_IMAGE | LABEL_RENDER_HOVER);
	ccSetPosition(plytv->imgovr, 0, 0);
	ccDisable(plytv->imgovr);
	
	ccDisable(plytv->tv);
	PLAYLISTCACHE *plc = getPrimaryPlaylist(vp);
	if (plc && playlistGetTotal(plc)){
		plytv->buildNo++;
		if (plytvTreeCreateRoot(plytv, plytv->tv, plc))
			ccEnable(plytv->tv);
	}
}

static inline int page_plytvShutdown (TPLYTV *plytv, TVLCPLAYER *vp)
{
	buttonsDeleteAll(plytv->btns);
	if (plytv->imgovr) ccDelete(plytv->imgovr);
	ccDelete(plytv->tv);
	
	return 1;
}

void page_plytvRenderStart (TPLYTV *plytv, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	//printf("page_plytvRenderStart\n");
	
	/*TKEYBOARD *vkey = pageGetPtr(vp, PAGE_VKEYBOARD);
	TKEYPAD *kp = vkey->kp;
	if (ccGetState(kp)) ccDisable(kp);*/
	
	
	//plytv->imgc.play = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_TVPLAY]);
	//plytv->imgc.pause = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_TVPAUSE]);
	//plytv->imgc.stop = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_TVSTOP]);
	//plytv->imgc.noart = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_NOART_PLYTV]);
}

void page_plytvRenderEnd (TPLYTV *plytv, TVLCPLAYER *vp, int64_t destId, int64_t data2, void *opaquePtr)
{
	//printf("page_plytvRenderEnd\n");
	
	if (tvRenameGetState(plytv->tv)){
		tvInputEnable(plytv->tv);
		ccEnable(plytv->tv->sbVert);
		tvRenameDisable(plytv->tv);
		plytvButtonsSetState(plytv, plytv->btns, 1);
	}
	
	
	//imageManagerImageRelease(vp->im, vp->gui.image[IMGC_TVPLAY]);
	//imageManagerImageRelease(vp->im, vp->gui.image[IMGC_TVPAUSE]);
	//imageManagerImageRelease(vp->im, vp->gui.image[IMGC_TVSTOP]);
	//imageManagerImageRelease(vp->im, vp->gui.image[IMGC_NOART_PLYTV]);
		
	/*TKEYBOARD *vkey = pageGetPtr(vp, PAGE_VKEYBOARD);
	TKEYPAD *kp = vkey->kp;
	if (ccGetState(kp)) ccDisable(kp);*/
}

int page_plyTvCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TPLYTV *plytv = (TPLYTV*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_plyTvCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_plytvRender(plytv, plytv->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		page_plytvRenderStart(plytv, plytv->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		page_plytvRenderEnd(plytv, plytv->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_plytvInput(plytv, plytv->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_INIT){
		page_plytvRenderInit(plytv, plytv->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_plytvStartup(plytv, plytv->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_plytvInitalize(plytv, plytv->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_plytvShutdown(plytv, plytv->com->vp);
		
	}
	
	return 1;
}
