
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



static inline int64_t subCcObject_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	if (msg == CC_MSG_RENDER) return 1;
	
	
	if (obj->type == CC_LISTBOX){
		TLB *lb = (TLB*)object;
		TVLCPLAYER *vp = (TVLCPLAYER*)obj->cc->vp;

		if (lb->id == ccGetId(vp, CCID_LISTBOX_SUBTITLES)){
			switch (msg){
			  case CC_MSG_ENABLED:{
			  	TVLCCONFIG *vlc = getConfig(vp);
			  	subtitleListboxFill(pageGetPtr(vp, PAGE_SUB), lb, &vlc->spu);
				break;
			  }
		  	  case CC_MSG_DISABLED:
				break;

		  	  case LISTBOX_MSG_ITEMSELECTED:{
		  	  	TVLCCONFIG *vlc = getConfig(vp);

		  	  	vlc_setSubtitle(vlc, data2);
		  	  	vlc->spu.selected = vlc_getSubtitle(vlc);
				//printf("vlc_getSubtitle %i, %i %i\n",vlc->spu.selected, (int)data1, (int)data2);
		  	  	subtitleListboxFill(pageGetPtr(vp, PAGE_SUB), lb, &vlc->spu);
		  	  	break;
			  }
			};
		}
	}
	return 1;
}

static inline void _subtitleListboxFill (TSUB *sub, TLB *lb, TVLCSPU *spu)
{
	if (!spu) return;
	int total = spu->total;
	if (total < 1) return;

	lbRemoveItems(lb);

	char buffer[2048];
	libvlc_track_description_t *st = spu->desc;
	const int selected = spu->selected;
	
	sub->totalSubtitles = 0;
	
	for (int i = 0; i < total && st; i++, st=st->p_next){
		char *subName = st->psz_name;
		if (!subName){
			__mingw_snprintf(buffer, sizeof(buffer), "Subtitle %i", i+1);
			subName = buffer;
		}

		int colour = (0xFF<<24)|COL_WHITE;
		if (st->i_id == selected) colour = (0xFF<<24)|COL_GREEN;
		lbAddItemEx(lb, LISTBOX_ITEM_STRING, subName, NULL, 0, colour, st->i_id);
		sub->totalSubtitles++;
	}
}

void subtitleListboxFill (TSUB *sub, TLB *lb, TVLCSPU *spu)
{
	if (ccLock(lb)){
		_subtitleListboxFill(sub, lb, spu);
		ccUnlock(lb);
	}
}

void subtitleListboxRefresh (TSUB *sub)
{
	if (ccGetState(sub->lb)){
		ccDisable(sub->lb);
		ccEnable(sub->lb);
	}
}

int subtitleGetDetails (TVLCPLAYER *vp, TVLCCONFIG *vlc)
{
	TSUB *sub = pageGetPtr(vp, PAGE_SUB);
	TLB *lb = sub->lb;
	int total = 0;
	
	if (ccLock(lb)){
		if (vlc->spu.desc) libvlcTrackDescriptionRelease(vlc->spu.desc);
		vlc->spu.desc = vlc_getSubtitleDescriptions(vlc);
		vlc->spu.selected = -1;
		total = vlc->spu.total = vlc_getSubtitleCount(vlc);
		ccUnlock(lb);
	}
	return total;
}

// TIMER_SUB_UPDATE
void subtitleGetUpdate (TVLCPLAYER *vp)
{

	char *path = getPlayingPath(vp);
	if (!path) return;
	int canNotHaveSubs = !isMediaVideo8(path);
	my_free(path);
	if (canNotHaveSubs) return;

	//printf("TIMER_SUB_UPDATE\n");
	
	TSUB *sub = pageGetPtr(vp, PAGE_SUB);
	TVLCCONFIG *vlc = vp->vlc;
	
	if (subtitleGetDetails(vp, vlc) > 0){
		vlc->spu.selected = vlc_getSubtitle(vlc);
		if (ccGetState(sub->lb))
			subtitleListboxFill(sub, sub->lb, &vlc->spu);
	}
}

static inline int page_subRender (TSUB *sub, TVLCPLAYER *vp, TFRAME *frame)
{
	TVLCCONFIG *vlc = getConfig(vp);
	if (vp->vlc->isMediaLoaded && vlc->spu.total)
		ccRender(sub->lb, frame);
	else
		//pageSetPrevious(vp);
		page2SetPrevious(sub);
		
	return 1;
}

static inline int page_subInput (TSUB *sub, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_TOUCH_DOWN:{
	  	// check if touch is within the faded area
		if (pos->x >= ccGetPositionX(sub->lb) && pos->x < ccGetPositionX(sub->lb)+ccGetWidth(sub->lb)+16){
			if (pos->y >= ccGetPositionY(sub->lb) && pos->y < ccGetPositionY(sub->lb)+ccGetHeight(sub->lb)+16){
				//ccDisable(sub->lb);
				return 0;
			}
		}
		page2SetPrevious(sub);
		break;
	  }
	  case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  	break;
	}
		
	return 1;
}

static inline int page_subStartup (TSUB *sub, TVLCPLAYER *vp, const int fw, const int fh)
{
	
	int width = fw * 0.994;
	int x = (fw - width)/2;
	int height = fh * 0.70;
	int y = (fh - height)/2;
	
	sub->lb = ccCreate(vp->cc, PAGE_SUB, CC_LISTBOX, subCcObject_cb, &vp->gui.ccIds[CCID_LISTBOX_SUBTITLES], width, height);
	sub->lb->font = CHAPTERS_FONT;
	sub->lb->verticalPadding += 6;
	sub->lb->scrollbar.offset = 24;
	sub->lb->flags.drawBaseGfx = 0;
	sub->lb->flags.drawBaseBlur = 0;
	
	ccSetMetrics(sub->lb, x, y, -1, -1);
	ccEnable(sub->lb);

	lbScrollbarSetWidth(sub->lb, SCROLLBAR_VERTWIDTH);
	//lbSetFocus(sub->lb, 0);
	//ccEnable(sub->lb);

	return 1;
}

int getSubtitleTotal (TVLCPLAYER *vp)
{
	if (!getPlayState(vp) || getPlayState(vp) == 8)
		return 0;
	if (hasPageBeenAccessed(vp, PAGE_SUB)){
		TSUB *sub = pageGetPtr(vp, PAGE_SUB);
		return sub->totalSubtitles;
	}
	
	return 0;
}

static inline int page_subInitalize (TSUB *sub, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_SUB);
	sub->totalSubtitles = 0;
	return 1;
}

static inline int page_subShutdown (TSUB *sub, TVLCPLAYER *vp)
{
	ccDelete(sub->lb);	
	return 1;
}

int page_subCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TSUB *sub = (TSUB*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_subCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_subRender(sub, sub->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){

	}else if (msg == PAGE_CTL_RENDER_END){
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_subInput(sub, sub->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_subStartup(sub, sub->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_subInitalize(sub, sub->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_subShutdown(sub, sub->com->vp);
		
	}
	
	return 1;
}


