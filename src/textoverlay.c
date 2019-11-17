
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


static inline TTEXTOVERLAY *thisPage (void *pageObj)
{
	TPAGE2COMOBJ *comPage = (TPAGE2COMOBJ*)pageObj;
	if (comPage->com->page->id != PAGE_TEXTOVERLAY)
		return pageGetPtr(comPage->com->vp, PAGE_TEXTOVERLAY);
	else
		return (TTEXTOVERLAY*)comPage;
}

static inline TLABELSTRITEM *lsItemNew ()
{
	TLABELSTRITEM *ls = my_calloc(1, sizeof(TLABELSTRITEM));
	return ls;
}

static inline void lsItemFree (TLABELSTRITEM *ls)
{
	labelStrDelete(ls->string);
	my_free(ls);
}

static inline int lsItemAdd (TTEXTOVERLAY *txtovr, TLABELSTRITEM *ls)
{
	TLISTITEM *item = listNewItem(ls);
	listAdd(txtovr->listRoot, item);
	return item != NULL;
}

static inline TLABELSTRITEM *lsListGetItem (TLISTITEM *item)
{
	TLABELSTRITEM *ls = (TLABELSTRITEM*)listGetStorage(item);
	return ls;
}

static inline TLABELSTRITEM *lsGetItem (TTEXTOVERLAY *txtovr, const int id)
{
	TLISTITEM *item = txtovr->listRoot; 
	while(item){
		TLABELSTRITEM *ls = lsListGetItem(item);
		if (ls->id == id) return ls;
		item = listGetNext(item);
	}
	return NULL;
}

void lsStrReplace (void *pageObj, const int id, const char *text)
{
	TLABELSTRITEM *ls = lsGetItem(thisPage(pageObj), id);
	if (ls)
		labelStrUpdate(ls->string, text);
}

void lsItemRemove (void *pageObj, const int id)
{
	TTEXTOVERLAY *txtovr = thisPage(pageObj);
	
	TLISTITEM *item = txtovr->listRoot; 
	while(item){
		TLABELSTRITEM *ls = lsListGetItem(item);
		if (ls->id == id){
			listRemove(item);
			if (txtovr->listRoot == item)
				txtovr->listRoot = item->next;
				
			lsItemFree(ls);
			listDestroy(item);
			return;
		}
		item = listGetNext(item);
	}
}

int lsItemCreate (void *pageObj, const char *text, const int font, int x, int y, const int justify)
{
	TTEXTOVERLAY *txtovr = thisPage(pageObj);
	TLABELSTRITEM *ls = lsItemNew();
	if (!ls) return 0;
	
	const int width = txtovr->com->vp->ml->width - x - 2;
	if (justify == NSEX_RIGHT) x = 0;

	ls->string = labelStrCreate(txtovr, text, font, x, y, width, txtovr, justify);
	ls->id = ls->string->ccId;

	lsItemAdd(txtovr, ls);
	return ls->id;
}

static inline void lsFree (TTEXTOVERLAY *txtovr)
{
	TLISTITEM *item = txtovr->listRoot; 
	while(item){
		lsItemFree(lsListGetItem(item));
		item = listGetNext(item);
	}
	listDestroyAll(txtovr->listRoot);
}

static inline int lsItemRender (TLABELSTRITEM *ls, TFRAME *frame)
{
	return labelStrRender(ls->string, frame);
}

static inline int lsRender (TTEXTOVERLAY *txtovr, TFRAME *frame)
{
	int renderedCt = 0;
	
	TLISTITEM *item = txtovr->listRoot; 
	while(item){
		renderedCt += lsItemRender(lsListGetItem(item), frame);
		item = listGetNext(item);
	}
	return renderedCt;
}

static inline int page_textRender (TTEXTOVERLAY *txtovr, TVLCPLAYER *vp, TFRAME *frame)
{
	lsRender(txtovr, frame);
	return 1;
}

static inline int page_textRenderBegin (TTEXTOVERLAY *txtovr, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	return 1;
}

static inline int page_textRenderEnd (TTEXTOVERLAY *txtovr, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, void *opaquePtr)
{
	return 1;
}

static inline int page_textRenderInit (TTEXTOVERLAY *txtovr, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	//lSetFontCharacterSpacing(vp->ml->hw, TEXTOVERLAY_FONT, lGetFontCharacterSpacing(vp->ml->hw, TEXTOVERLAY_FONT)+1);
	return 1;
}

static inline int page_textStartup (TTEXTOVERLAY *txtovr, TVLCPLAYER *vp, const int width, const int height)
{
	page2InputDisable(vp->pages, PAGE_TEXTOVERLAY);
	return 1;
}

static inline int page_textInitalize (TTEXTOVERLAY *txtovr, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_TEXTOVERLAY);
	return 1;
}

static inline int page_textShutdown (TTEXTOVERLAY *txtovr, TVLCPLAYER *vp)
{
	lsFree(txtovr);
	return 1;
}

int page_textCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TTEXTOVERLAY *txtovr = (TTEXTOVERLAY*)pageStruct;
	
	// printf("# page_textCallback: %p %i %I64d %I64d %p %p\n", txtovr, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_textRender(txtovr, txtovr->com->vp, dataPtr);
	
	}else if (msg == PAGE_CTL_RENDER_START){
		return page_textRenderBegin(txtovr, txtovr->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		return page_textRenderEnd(txtovr, txtovr->com->vp, dataInt1, dataInt2, opaquePtr);

	}else if (msg == PAGE_CTL_INPUT){
		return 0;

	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_textRenderInit(txtovr, txtovr->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_textStartup(txtovr, txtovr->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_textInitalize(txtovr, txtovr->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_textShutdown(txtovr, txtovr->com->vp);
	}
	
	return 1;
}
