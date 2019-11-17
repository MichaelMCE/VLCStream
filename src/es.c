
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


#define BORDERDEPTH	12




static inline int esButtonPress (TSTREAMINFO *sinfo, TCCBUTTON *btn, const int id, const TTOUCHCOORD *pos)
{
	TVLCPLAYER *vp = sinfo->com->vp;
	sinfo->btns->t0 = getTickCount();


	switch (id){
	  case SBUTTON_ATRACK:
		if (sinfo->atracks){
			if (++sinfo->currentAudioTrack > sinfo->atracks->totalTracks-1)
				sinfo->currentAudioTrack = 0;
				
			int id = sinfo->atracks->track[sinfo->currentAudioTrack].id;
			if (id > 0){
				const char *text = sinfo->atracks->track[sinfo->currentAudioTrack].name;
				marqueeAdd(vp, sinfo->marquee, text, getTime(vp)+5000);
				vlc_setAudioTrack(vp->vlc, id);
			}
		}
	  	break;

	  case SBUTTON_VTRACK:
		if (sinfo->vtracks){
			if (++sinfo->currentVideoTrack > sinfo->vtracks->totalTracks-1)
				sinfo->currentVideoTrack = 0;
				
			int id = sinfo->vtracks->track[sinfo->currentVideoTrack].id;
			if (id > 0){
				const char *text = sinfo->vtracks->track[sinfo->currentVideoTrack].name;
				marqueeAdd(vp, sinfo->marquee, text, getTime(vp)+5000);
				//printf("v id %i\n", id);
				vlc_setVideoTrack(vp->vlc, id);
			}
		}
	  	break;
	  	
	  case SBUTTON_SUBS:
	  	page2Set(vp->pages, PAGE_SUB, 1);
	  	TSUB *sub = pageGetPtr(vp, PAGE_SUB);
		ccEnable(sub->lb);
	  	break;

	  /*case SBUTTON_META:{
		PLAYLISTCACHE *plc = NULL;

	  	if (pageGet(vp) == PAGE_OVERLAY)
	  		plc = getQueuedPlaylist(vp);
	  	if (!plc)
	  		plc = getDisplayPlaylist(vp);
		if (!plc) break;	// this shouldn't happen

		TMETA *meta = pageGetPtr(vp, PAGE_META);
		if (plc->pr->playingItem >= 0)
			meta->trackPosition = plc->pr->playingItem;
		else
			meta->trackPosition = 0;

	  	pageSet(vp, PAGE_META);
	  }
		break;
		*/
	  case SBUTTON_NEXT:
		if (++sinfo->selected > sinfo->tCategories-1)
			sinfo->selected = 0;
		break;

	  case SBUTTON_PREV:
	  	if (sinfo->selected > sinfo->tCategories-1)
	  		sinfo->selected = 0;
	  	//else if (sinfo->selected > 0)
	  		//--sinfo->selected;
	  	else if (--sinfo->selected < 0)
	  		sinfo->selected = sinfo->tCategories-1;

		break;
	}
	return 0;
}	

void esFreeCategories (TCATEGORY *cats, const int tCat)
{
	if (tCat){
		for (int i = 0; i < tCat; i++){
			if (cats[i].name) 
				my_free(cats[i].name);
				
			if (cats[i].tInfos && cats[i].infos){
				for (int j = 0; j < cats[i].tInfos; j++){
					if (cats[i].infos[j].name)
						my_free(cats[i].infos[j].name);
					if (cats[i].infos[j].value)
						my_free(cats[i].infos[j].value);
				}
				my_free(cats[i].infos);
			}
		}
		my_free(cats);
	}
}

void esFreeAVTracks (TAVTRACKS *avts)
{
	if (avts){
		while(avts->totalTracks-- > 0)
			my_free(avts->track[avts->totalTracks].name);

		my_free(avts->track);
		my_free(avts);
	}
}

// TIMER_ES_UPDATE
void esGetUpdate (TVLCPLAYER *vp)
{
	if (!getPlayState(vp)) return;
		
	//printf("TIMER_ES_UPDATE playstate:%i\n", getPlayState(vp));

	TSTREAMINFO *sinfo = pageGetPtr(vp, PAGE_ES);
	TVLCCONFIG *vlc = vp->vlc;

	sinfo->selected = 0;
	esFreeCategories(sinfo->categories, sinfo->tCategories);
	sinfo->categories = getCategories(vlc, &sinfo->tCategories);

	buttonsStateSet(sinfo->btns, SBUTTON_ATRACK, 0);
	//buttonDisable(vp, PAGE_ES, SBUTTON_ATRACK);
	
	esFreeAVTracks(sinfo->atracks);
	sinfo->atracks = getAudioTracks(vlc, &sinfo->currentAudioTrack);
	if (sinfo->atracks){
		if (sinfo->atracks->totalTracks > 1)
			buttonsStateSet(sinfo->btns, SBUTTON_ATRACK, 1);
			//buttonEnable(vp, PAGE_ES, SBUTTON_ATRACK);
	}
		
	
	buttonsStateSet(sinfo->btns, SBUTTON_VTRACK, 0);
	//buttonDisable(vp, PAGE_ES, SBUTTON_VTRACK);
	
	esFreeAVTracks(sinfo->vtracks);
	sinfo->vtracks = getVideoTracks(vlc, &sinfo->currentVideoTrack);
	if (sinfo->vtracks){
		if (sinfo->vtracks->totalTracks > 1)
			buttonsStateSet(sinfo->btns, SBUTTON_VTRACK, 1);
			//buttonEnable(vp, PAGE_ES, SBUTTON_VTRACK);
		//printf("sinfo->vtracks %p, %i %i\n", sinfo->vtracks, sinfo->vtracks->totalTracks, sinfo->currentVideoTrack);
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
		return esButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	return 1;
}

int renderCategory (TVLCPLAYER *vp, TFRAME *frame, const int font, TCATEGORY *cats, const int tCat, const int selected)
{
	TCATEGORY *cat = &cats[selected];
	if (!cat) return -1;
	if (!cat->tInfos) return -1;
	
	TLPRINTR rt;
	memset(&rt, 0, sizeof(TLPRINTR));
	const int flags = PF_CLIPWRAP|PF_DONTFORMATBUFFER|PF_MIDDLEJUSTIFY|PF_WORDWRAP|PF_FORCEAUTOWIDTH;
	char *buffer = my_calloc(1, MAX_PATH_UTF8+1);
	if (!buffer){
		printf("renderCategory: error allocating memory %i\n", MAX_PATH_UTF8);
		return 0;
	}

	int len;
	if (cat->name)
		len = __mingw_snprintf(buffer, MAX_PATH_UTF8, "[%s]", cat->name);
	else
		len = __mingw_snprintf(buffer, MAX_PATH_UTF8, "[Stream %i]", selected+1);
		
	for (int i = 0; i < cat->tInfos && i < 12 && len < 512; i++){
		if (cat->infos[i].name && cat->infos[i].value)
			len += __mingw_snprintf(&buffer[len], MAX_PATH_UTF8-len, "\n%s: %s", cat->infos[i].name, cat->infos[i].value);
		else if (cat->infos[i].name)
			len += __mingw_snprintf(&buffer[len], MAX_PATH_UTF8-len, "\n%s: <>", cat->infos[i].name);
		else if (cat->infos[i].value)
			len += __mingw_snprintf(&buffer[len], MAX_PATH_UTF8-len, "\n<>: %s", cat->infos[i].value);
	}
		
	// get text height
	rt.bx1 = BORDERDEPTH;
	rt.bx2 = frame->width-1-BORDERDEPTH;
	lPrintEx(frame, &rt, font, flags|PF_GETTEXTBOUNDS|PF_DONTRENDER, LPRT_CPY, buffer);
	if (rt.sy > frame->height-3) rt.sy = frame->height-3;
	int h = rt.ey;
	if (h > frame->height-3) h = frame->height-3;

	memset(&rt, 0, sizeof(TLPRINTR));
	rt.bx1 = BORDERDEPTH;
	rt.bx2 = frame->width-1-BORDERDEPTH;
	rt.sy = ((frame->height - h) / 2);
	if (font == MFONT)
		h += lGetFontLineSpacing(frame->hw, font)+1;
	int y1 = rt.sy;

	const unsigned int *col = swatchGetPage(vp, PAGE_ES);
	lDrawLine(frame, BORDERDEPTH+2, rt.sy+1, frame->width-1-BORDERDEPTH-2, rt.sy+1, col[SWH_ES_BLURBORDER]);
	lDrawLine(frame, BORDERDEPTH+2, rt.sy+h-1, frame->width-1-BORDERDEPTH-2, rt.sy+h-1, col[SWH_ES_BLURBORDER]);
	lDrawLine(frame, BORDERDEPTH+1, rt.sy+2, BORDERDEPTH+1, rt.sy+h-2, col[SWH_ES_BLURBORDER]);
	lDrawLine(frame, frame->width-1-BORDERDEPTH-1, rt.sy+2, frame->width-1-BORDERDEPTH-1, rt.sy+h-2, col[SWH_ES_BLURBORDER]);
	lBlurArea(frame, BORDERDEPTH, rt.sy, frame->width-1-BORDERDEPTH, rt.sy+h, 4);
	lPrintEx(frame, &rt, font, flags, LPRT_CPY, buffer);

	my_free(buffer);
	return y1;
}

static inline int page_esRender (TSTREAMINFO *sinfo, TVLCPLAYER *vp, TFRAME *frame)
{
	
	if (!vp->vlc->isMediaLoaded){
		page2SetPrevious(sinfo);
		return 1;
	}

	int selected = sinfo->selected;
	if (selected > sinfo->tCategories-1)
		selected = 0;
	if (selected < 0)
		selected = sinfo->tCategories-1;

	if (!sinfo->tCategories){
		page2SetPrevious(sinfo);
		return 1;
	}

	const unsigned int *col = swatchGetPage(vp, PAGE_ES);
	lSetForegroundColour(vp->ml->hw, col[SWH_ES_TEXT]);
	lSetBackgroundColour(vp->ml->hw, col[SWH_ES_TEXTBK]);
	lSetRenderEffect(vp->ml->hw, LTR_SHADOW);
	lSetFilterAttribute(vp->ml->hw, LTR_SHADOW, 0, LTRA_SHADOW_S|LTRA_SHADOW_E | LTRA_SHADOW_S3 | LTRA_SHADOW_OS(1) | LTRA_SHADOW_TR(100));
	lSetCharacterEncoding(vp->ml->hw, CMT_UTF8);
		
	int y = renderCategory(vp, frame, ESTREAM_FONT, sinfo->categories, sinfo->tCategories, selected);
	if (y < 0) return 0;
	
	if (sinfo->tCategories > 1){
		if (y < 20) y = 20;
		lPrintf(frame, BORDERDEPTH+1, y-20, ESTREAM_FONT, LPRT_CPY, "[%i of %i]", selected+1, sinfo->tCategories);

		buttonsStateSet(sinfo->btns, SBUTTON_PREV, 1);
		buttonsStateSet(sinfo->btns, SBUTTON_NEXT, 1);
		//buttonEnable(vp, PAGE_ES, SBUTTON_PREV);
		//buttonEnable(vp, PAGE_ES, SBUTTON_NEXT);
	}else{
		buttonsStateSet(sinfo->btns, SBUTTON_PREV, 0);
		buttonsStateSet(sinfo->btns, SBUTTON_NEXT, 0);
		//buttonDisable(vp, PAGE_ES, SBUTTON_PREV);
		//buttonDisable(vp, PAGE_ES, SBUTTON_NEXT);		
	}
	lSetRenderEffect(vp->ml->hw, LTR_DEFAULT);
	marqueeDraw(vp, frame, sinfo->marquee, 2, 2);	
	
	if (vp->vlc->spu.total > 0)
		buttonsStateSet(sinfo->btns, SBUTTON_SUBS, 1);
		//buttonEnable(vp, PAGE_ES, SBUTTON_SUBS);
	else
		buttonsStateSet(sinfo->btns, SBUTTON_SUBS, 0);
		//buttonDisable(vp, PAGE_ES, SBUTTON_SUBS);
	
	buttonsRenderAll(sinfo->btns, frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);
	return 1;
}

static inline int page_esInput (TSTREAMINFO *sinfo, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_TOUCH_DOWN:
	  	page2SetPrevious(sinfo);
		//pageSetSec(vp, -1);
		break;

	 /* case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  	break;*/
	  	
	  case PAGE_IN_WHEEL_FORWARD:
	  	esButtonPress(sinfo, NULL, SBUTTON_PREV, NULL);
		break;
		
	  case PAGE_IN_WHEEL_BACK:
	  	esButtonPress(sinfo, NULL, SBUTTON_NEXT, NULL);
	  	break;
	  	
	}
		
	return 1;
}

static inline int page_esStartup (TSTREAMINFO *sinfo, TVLCPLAYER *vp, const int fw, const int fh)
{
	sinfo->tCategories = 0;
	sinfo->selected = 0;
	sinfo->currentAudioTrack = -1;
	sinfo->currentVideoTrack = -1;
	sinfo->marquee = marqueeNew(3, MARQUEE_CENTER, ESTREAM_FONT);

	sinfo->btns = buttonsCreate(vp->cc, PAGE_ES, SBUTTON_TOTAL, ccbtn_cb);
	
	TCCBUTTON *btn = buttonsCreateButton(sinfo->btns, L"meta/mright.png", L"meta/mrighthl.png", SBUTTON_NEXT, 1, 0, 0, 0);
	ccSetPosition(btn, fw-1 - ccGetWidth(btn), abs(fh - ccGetHeight(btn))/2);
	
	btn = buttonsCreateButton(sinfo->btns, L"meta/mleft.png", L"meta/mlefthl.png", SBUTTON_PREV, 1, 0, 0, 0);
	ccSetPosition(btn, 0, abs(fh - ccGetHeight(btn))/2);

	btn = buttonsCreateButton(sinfo->btns, L"es/essubs.png", NULL, SBUTTON_SUBS, 1, 0, 0, 0);
	ccSetPosition(btn, fw-8 - ccGetWidth(btn), 0);

	btn = buttonsCreateButton(sinfo->btns, L"es/atrack.png", NULL, SBUTTON_ATRACK, 0, 1, 1, 0);
	btn = buttonsCreateButton(sinfo->btns, L"es/vtrack.png", NULL, SBUTTON_VTRACK, 0, 1, ccGetPositionX(btn)+ccGetWidth(btn)+12, 0);

	return 1;
}

static inline int page_esInitalize (TSTREAMINFO *sinfo, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_ES);
	return 1;
}

static inline int page_esShutdown (TSTREAMINFO *sinfo, TVLCPLAYER *vp)
{
	marqueeDelete(sinfo->marquee);
	if (sinfo->tCategories)
		esFreeCategories(sinfo->categories, sinfo->tCategories);
	if (sinfo->atracks)
		esFreeAVTracks(sinfo->atracks);
	if (sinfo->vtracks)
		esFreeAVTracks(sinfo->vtracks);
		
	buttonsDeleteAll(sinfo->btns);
	
	return 1;
}

int page_esCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TSTREAMINFO *es = (TSTREAMINFO*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_esCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_esRender(es, es->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){

	}else if (msg == PAGE_CTL_RENDER_END){
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_esInput(es, es->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_esStartup(es, es->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_esInitalize(es, es->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_esShutdown(es, es->com->vp);
		
	}
	
	return 1;
}

