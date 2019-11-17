
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



int chaptersLock (TCHAPTER *chap)
{
	return lockWait(chap->titles.lock, INFINITE);
}

void chaptersUnlock (TCHAPTER *chap)
{
	lockRelease(chap->titles.lock);
}

// lock must be held
static inline char *chaptersGetChapterName (TCHAPTER *chap, const int titleIdx, const int chapIdx)
{
	if (titleIdx >= 0 && titleIdx < chap->titles.total && chap->titles.list){
		if (chapIdx < chap->titles.list[titleIdx].tSeekPoints)
			return chap->titles.list[titleIdx].seekPoints[chapIdx].name;
	}
	return NULL;
}

// lock must be held
static inline char *chaptersGetChapterNameByTime (TCHAPTER *chap, const int titleIdx, int64_t timeOffset)
{
	char *name = NULL;
	if (titleIdx >= 0 && titleIdx < chap->titles.total && chap->titles.list){
		int idx = -1;
		for (int i = 0; i < chap->titles.list[titleIdx].tSeekPoints; i++){
			if (timeOffset >= chap->titles.list[titleIdx].seekPoints[i].timeOffset)
				idx = i;
			else
				break;
		}
		if (idx >= 0)
			name = my_strdup(chap->titles.list[titleIdx].seekPoints[idx].name);
	}
	//if (name == NULL)
		//name = my_strdup("-");
	return name;
}

// lock must be held
static inline int chaptersGetChapterTotal (TCHAPTER *chap, const int titleIdx)
{
	if (titleIdx >= 0 && titleIdx < chap->titles.total && chap->titles.list)
		return chap->titles.list[titleIdx].tSeekPoints;
	return 0;
}

// lock must be held
static inline int _chaptersGetTotal (TCHAPTER *chap)
{
	int total = 0;
	if (chap->tchapters)
		total = chaptersGetChapterTotal(chap, chap->ctitle-1);

	chap->displayMode = 0;		 // display chapters
	if (total < 1){
		if (chap->titles.total > 1 && chap->titles.list && !chap->titles.list->seekPoints){
			total = chap->titles.total;
			chap->displayMode = 1; // display titles
		}else{
			return 0;
		}
	}
	return total;
}

int chaptersGetTotal (TCHAPTER *chap)
{
	int ret = 0;
	if (chaptersLock(chap)){
		ret = _chaptersGetTotal(chap);
		chaptersUnlock(chap);
	}
	return ret;
}

int chaptersPaneAddChapters (TCHAPTER *chap, TPANE *pane)
{
	const int total = chaptersGetTotal(chap);
	if (!total) return 0;
			
	if (ccLock(pane)){
		paneRemoveAll(pane);
	
		lSetCharacterEncoding(pane->cc->vp->ml->hw, CMT_UTF8);
		
		const int titleIdx = (chap->ctitle-1)<<20;
		char *chapter;
		char buffer[128];
		int highlightThis = 0;
	
		
		if (chaptersLock(chap)){
			for (int i = 0; i < total; i++){
				if (!chap->displayMode){
					chapter = chaptersGetChapterName(chap, chap->ctitle-1, i);
					if (!chapter){
						chapter = buffer;
						__mingw_snprintf(buffer, sizeof(buffer)-1, "Chapter %i", i+1);
					}
					highlightThis = (i == chap->cchapter && chap->title+1 == chap->ctitle);
				}else{
					chapter = chap->titles.list[i].name;
					//printf("title %i:'%s'\n", i, chapter);
					highlightThis = (i == chap->ctitle-1);
				}
				
				int icon;
				if (highlightThis)
					icon = chap->icons.play;
				else
					icon = chap->icons.chapters;
					
				//printf("addchapter %i '%s'\n", i, chapter);
				paneTextAdd(pane, icon, 1.0, chapter, CHAPTERS_FONT, titleIdx|i);
			}
			chaptersUnlock(chap);
		}

		ccUnlock(pane);
	}
	
	return total;
}

static inline void chapterFreeTitles (TTITLE *titles, const int tTitles)
{
	TTITLE *title = titles;
	if (title){
		for (int i = 0; i < tTitles; title++,i++){
			if (title->name)
				my_free(title->name);

			if (title->seekPoints){
				for (int j = 0; j < title->tSeekPoints; j++)
					my_free(title->seekPoints[j].name);
				my_free(title->seekPoints);
			}
		}
		my_free(titles);
	}
}


void chapterChaptersRefresh (TCHAPTER *chap)
{
	if (ccLock(chap->pane)){
		chaptersPaneAddChapters(chap, chap->pane);
		ccUnlock(chap->pane);
	}
}

static inline int chapButtonPress (TCHAPTER *chap, TCCBUTTON *btn, const int id, const TTOUCHCOORD *pos)
{
	TVLCPLAYER *vp = btn->cc->vp;
	chap->btns->t0 = getTickCount();


	switch (id){
	  case CHPBUTTON_LEFT:
	  	if (!chap->ttitles) break;
		if (--chap->ctitle < 1) chap->ctitle = 1;

		chap->schapter = -1;
		chap->tchapters = vlc_getTitleChapterCount(vp->vlc, chap->ctitle-1);
		if (chap->tchapters > MAX_CHAPTERS) chap->tchapters = MAX_CHAPTERS;
		
		// if we've no chapters in title then attempt to play title
		if (!chap->tchapters)
			vlc_setTitle(vp->vlc, chap->ctitle-1);
		
		paneScrollReset(chap->pane);
		chapterChaptersRefresh(chap);
	  	break;
	  	
	  case CHPBUTTON_RIGHT:
	  	if (!chap->ttitles) break;
	  	if (++chap->ctitle > chap->ttitles)
	  		chap->ctitle = chap->ttitles;

		chap->schapter = -1;
		chap->tchapters = vlc_getTitleChapterCount(vp->vlc, chap->ctitle-1);
		if (chap->tchapters > MAX_CHAPTERS) chap->tchapters = MAX_CHAPTERS;

		// if we've no chapters in title then attempt to play title
		if (!chap->tchapters)
			vlc_setTitle(vp->vlc, chap->ctitle-1);
		
		paneScrollReset(chap->pane);
		chapterChaptersRefresh(chap);
	  	break;

	  case CHPBUTTON_BACK:
		page2SetPrevious(chap);
		break;
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

	if (msg == BUTTON_MSG_SELECTED_PRESS)
		return chapButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	return 1;
}

// lock must be held
static inline char *chaptersGetTitleName (TCHAPTER *chap, const int titleIdx)
{
	//printf("%i %i %p\n", titleIdx, chap->tTitles, chap->titles);
	
	if (titleIdx >= 0 && titleIdx < chap->titles.total && chap->titles.list)
		return chap->titles.list[titleIdx].name;
	return NULL;
}

// lock must be held
uint64_t chaptersGetChapterTimeOffset (TCHAPTER *chap, const int titleIdx, const int chapIdx)
{
	//printf("chaptersGetChapterTimeOffset\n");
	
	if (titleIdx >= 0 && titleIdx < chap->titles.total && chap->titles.list){
		if (chapIdx < chap->titles.list[titleIdx].tSeekPoints)
			return chap->titles.list[titleIdx].seekPoints[chapIdx].timeOffset;
	}
	return -1;
}

char *getChapterNameByTime (TVLCPLAYER *vp, int64_t timeOffset)
{
	
	//printf("getChapterNameByTime\n");
	
	TCHAPTER *chap = pageGetPtr(vp, PAGE_CHAPTERS);
	timeOffset *= 1000 * 1000;
	//printf("ctitle %i %I64d\n", chap->ctitle-1, timeOffset);
	
	char *name = NULL;
	if (chaptersLock(chap)){
		name = chaptersGetChapterNameByTime(chap, chap->ctitle-1, timeOffset);
		chaptersUnlock(chap);
	}
	return name;
}

char *getChapterNameByIndex (TVLCPLAYER *vp, const int chapIdx)
{
	//printf("getChapterNameByIndex\n");
	
	TCHAPTER *chap = pageGetPtr(vp, PAGE_CHAPTERS);

	char *name = NULL;
	if (chaptersLock(chap)){
		name = chaptersGetChapterName(chap, chap->ctitle-1, chapIdx);
		if (name) name = my_strdup(name);
		chaptersUnlock(chap);
	}

	return name;
}

int getTotalChapters (TVLCPLAYER *vp)
{
	//printf("getTotalChapters\n");
	
	if (!getPlayState(vp) || getPlayState(vp) == 8)
		return 0;

	TCHAPTER *chap = pageGetPtr(vp, PAGE_CHAPTERS);
	return chap->tchapters;
	//return chaptersGetTotal(chap);
}

int getPlayingChapter (TVLCPLAYER *vp)
{
	//printf("getPlayingChapter\n");
	
	TCHAPTER *chap = pageGetPtr(vp, PAGE_CHAPTERS);
	return chap->cchapter; //(chap->cchapter && chap->title+1 == chap->ctitle);	
}

void chapterGetDetails (TVLCPLAYER *vp, TVLCCONFIG *vlc, TCHAPTER *chapt)
{

	//printf("chapterGetDetails \n");
	//printf("chapterGetDetails count:%i\n", vlc_getTitleCount(vlc));

	chapt->ttitles = vlc_getTitleCount(vlc);
	if (chapt->ttitles < 0) chapt->ttitles = 0;

	if (!chapt->ttitles){
		chapt->title = 0;
		chapt->ctitle = 0;
		chapt->tchapters = 0;
		chapt->cchapter = 0;
	}else{
		chapt->title = vlc_getTitle(vlc);
		chapt->ctitle = chapt->title+1;

		chapt->tchapters = vlc_getTitleChapterCount(vlc, chapt->ctitle-1);
		if (chapt->tchapters > MAX_CHAPTERS || chapt->tchapters < 0) chapt->tchapters = 0;
		else if (chapt->tchapters > MAX_CHAPTERS) chapt->tchapters = MAX_CHAPTERS;

		chapt->cchapter = vlc_getChapter(vlc);
		if (chapt->cchapter > MAX_CHAPTERS-1) chapt->cchapter = MAX_CHAPTERS-1;
	}

#if 0
	int vtracks = vlc_getVideoTrackCount(vlc)-1;
	int atracks = vlc_getAudioTrackCount(vlc)-1;
	printf("titles %i chapters %i video tracks %i audio tracks %i\n", chapt->ttitles, chapt->tchapters, vtracks, atracks);
	printf("current title %i current chapter %i\n", chapt->ctitle, chapt->cchapter);
#endif
}

// TIMER_CHAPTER_UPDATE
void chaptersGetUpdate (TVLCPLAYER *vp)
{
	if (!getPlayState(vp)) return;
		
	char *path = getPlayingPath(vp);
	if (!path) return;
	
	int canNotHaveChapters = (isMediaDVB(path) || !isMediaVideo8(path)) && !isAyFile8(path);
	my_free(path);
	
	//printf("chaptersGetUpdate canNotHaveChapters: %i %i\n", canNotHaveChapters, isAyFile8(path));
	
	if (canNotHaveChapters){
		TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
		if (buttonsStateGet(plyctrl->btns, VBUTTON_CHAPPREV)){
			buttonsStateSet(plyctrl->btns, VBUTTON_CHAPPREV, 0);
			buttonsStateSet(plyctrl->btns, VBUTTON_CHAPNEXT, 0);
		}

		if (hasPageBeenAccessed(vp, PAGE_CHAPTERS)){
			TCHAPTER *chap = pageGetPtr(vp, PAGE_CHAPTERS);
			if (chap->titles.total)
				chapterFreeTitles(chap->titles.list, chap->titles.total);
			chap->titles.total = 0;
			chap->titles.list = NULL;
		}
		return;
	}
	
	//printf("TIMER_CHAPTER_UPDATE %i\n", getPlayState(vp));


	TCHAPTER *chap = pageGetPtr(vp, PAGE_CHAPTERS);
	
	if (chaptersLock(chap)){
		TTITLE *oldTitles = chap->titles.list;
		int oldtTitles = chap->titles.total;

		TVLCCONFIG *vlc = getConfig(vp);
		chapterGetDetails(vp, vlc, chap);

		int tTitles = chap->titles.total;
		TTITLE *titles = getTitles(vlc, &tTitles);
	
		chap->titles.list = titles;
		chap->titles.total = tTitles;
		chapterFreeTitles(oldTitles, oldtTitles);

		chaptersUnlock(chap);
	}

	if (ccGetState(chap->pane))
		chaptersPaneAddChapters(chap, chap->pane);

	TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);
	if (chap->tchapters > 1){
		buttonsStateSet(plyctrl->btns, VBUTTON_CHAPPREV, 1);
		buttonsStateSet(plyctrl->btns, VBUTTON_CHAPNEXT, 1);
	}else{
		buttonsStateSet(plyctrl->btns, VBUTTON_CHAPPREV, 0);
		buttonsStateSet(plyctrl->btns, VBUTTON_CHAPNEXT, 0);
	}
	
	if (chap->ttitles > 1){
		buttonsStateSet(chap->btns, CHPBUTTON_LEFT, 1);
		buttonsStateSet(chap->btns, CHPBUTTON_RIGHT, 1);
	}else{
		buttonsStateSet(chap->btns, CHPBUTTON_LEFT, 0);
		buttonsStateSet(chap->btns, CHPBUTTON_RIGHT,0);
	}
	
	//if (contextMenuIsChaptersVisable(vp))
	//	taskbarPostMessage(vp, WM_TRACKCHAPTERUPDATE, chap->cchapter, chap->tchapters);
}

static inline int page_chapRender (TCHAPTER *chapt, TVLCPLAYER *vp, TFRAME *frame)
{
	if (!vp->vlc->isMediaLoaded || !chaptersGetTotal(chapt)){
		page2SetPrevious(chapt);
		return 1;
	}
	
	/*if (chapt->ttitles > 1){
		buttonsStateSet(chapt->btns, CHPBUTTON_LEFT, 1);
		buttonsStateSet(chapt->btns, CHPBUTTON_RIGHT, 1);
	}else{
		buttonsStateSet(chapt->btns, CHPBUTTON_LEFT, 0);
		buttonsStateSet(chapt->btns, CHPBUTTON_RIGHT,0);
	}*/

	if (!chapt->displayMode)
		buttonsRenderAll(chapt->btns, frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);
	
	if (!chapt->titles.total || chapt->titles.list == NULL)
		return 0;

	const unsigned int *col = swatchGetPage(vp, PAGE_CHAPTERS);
	lSetForegroundColour(vp->ml->hw, col[SWH_CHP_TITLE]);

	int x = chapt->rect.x1 + 156;
	int y = chapt->rect.y1 + 1;

	if (chaptersLock(chapt)){
		if (chapt->titles.total > 1){
			char *title = chaptersGetTitleName(chapt, chapt->ctitle-1);
			if (title && *title){
				lPrintf(frame, x-3, y, MFONT, LPRT_CPY, "%i: %s",chapt->ctitle, title);
			}else{
				char *titleName = vlc_getTitleDescription(vp->vlc, chapt->ctitle-1);
				if (titleName && *titleName){
					lPrintf(frame, x, y, MFONT, LPRT_CPY, "%i: %s", chapt->ctitle, titleName);
					my_free(titleName);
				}else{
					lPrintf(frame, x, y, MFONT, LPRT_CPY, "Title %i/%i", chapt->ctitle, chapt->ttitles);
				}
			}
		}
    	
		if (!chapt->displayMode){
			TFRAME *str = lNewString(vp->ml->hw, LFRM_BPP_32, PF_RIGHTJUSTIFY, MFONT, "[%i of %i] ", chapt->cchapter+1, chapt->tchapters);
			if (str){
				drawImage(str, frame, chapt->rect.x2 - str->width - 16, frame->height - str->height - 4, str->width-1, str->height-1);
				lDeleteFrame(str);
			}
		}
		chaptersUnlock(chapt);
	}
	
	ccRender(chapt->pane, frame);
	
#if DRAWTOUCHRECTS
	lDrawRectangle(frame, chapt->rect.x1, chapt->rect.y1, chapt->rect.x2, chapt->rect.y2, DRAWTOUCHRECTCOL);
#endif

	return 1;
}

static inline int page_chapRenderBegin (TCHAPTER *chap, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	lSetCharacterEncoding(vp->ml->hw, CMT_UTF8);
	return 1;
}

static inline void page_chapRenderEnd (TCHAPTER *chap, TVLCPLAYER *vp, int64_t destId, int64_t data2, void *opaquePtr)
{

}

void setChapter (TVLCPLAYER *vp, const int chapterIdx)
{
	TCHAPTER *chapt = pageGetPtr(vp, PAGE_CHAPTERS);
	if (!chapt->displayMode)
		chapt->schapter = chapterIdx;

	TVLCCONFIG *vlc = getConfig(vp);
	vlc_setChapter(vlc, chapterIdx);
}

static inline int64_t chapters_pane_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT || msg == CC_MSG_HOVER) return 1;
	
	TPANE *pane = (TPANE*)object;
	//printf("chapters_pane_cb in %p, %i %I64d %I64d %p\n", pane, msg, data1, data2, dataPtr);
	
	if (msg == PANE_MSG_TEXT_SELECTED || msg == PANE_MSG_IMAGE_SELECTED){
		TCHAPTER *chapt = ccGetUserData(pane);
		int title = data2>>20;
		int chapter = data2&0xFFFFF;

		TVLCCONFIG *vlc = getConfig(pane->cc->vp);
		if (!chapt->displayMode){
			chapt->schapter = chapter;
			vlc_setTitle(vlc, title);
			vlc_setChapter(vlc, chapter);
		}else if (chapt->displayMode == 1){
			vlc_setTitle(vlc, chapter);
		}
		
	}
	return 1;
}

static inline int page_chapStartup (TCHAPTER *chapt, TVLCPLAYER *vp, const int fw, const int fh)
{
	int width = fw * 0.994;
	int x = (fw - width)/2;
	int height = fh * 0.78;
	int y = (fh - height)/2;
		
	chapt->displayMode = 0;		// 0:chapters, 1:titles
	chapt->titles.lock = lockCreate("titlesLock");

	chapt->btns = buttonsCreate(vp->cc, PAGE_CHAPTERS, CHPBUTTON_TOTAL, ccbtn_cb);
	TCCBUTTON *btn = buttonsCreateButton(chapt->btns, L"chapters/left.png", L"chapters/left.png", CHPBUTTON_LEFT, 1, 1, x, 0);
	buttonsCreateButton(chapt->btns, L"chapters/right.png", L"chapters/righthl.png", CHPBUTTON_RIGHT, 1, 1, ccGetPositionX(btn)+ccGetWidth(btn)+48, 0);
	btn = buttonsCreateButton(chapt->btns, L"common/back_right96.png", NULL, CHPBUTTON_BACK, 1, 0, 0, 0);
	ccSetPosition(btn, fw - ccGetWidth(btn) - 16, 0);

	chapt->rect.x1 = x - 8;	
	chapt->rect.y1 = y - 24;
	chapt->rect.x2 = x + width + 8;
	chapt->rect.y2 = y + height + 8;
	chapt->ttitles = 0;
	chapt->tchapters = 0;
	chapt->ctitle = 0;
	chapt->schapter = -1;
	chapt->cchapter = -1;
	
	return 1;
}

static inline int page_chapInitalize (TCHAPTER *chap, TVLCPLAYER *vp, const int width, const int height)
{

	setPageAccessed(vp, PAGE_CHAPTERS);

	TPANE *pane = ccCreateEx(vp->cc, PAGE_CHAPTERS, CC_PANE, chapters_pane_cb, NULL, width, height-64, chap);
	chap->pane = pane;
	paneSetLayout(pane, PANE_LAYOUT_VERTCENTER);
	ccSetPosition(pane, 0, 64);
	paneSetAcceleration(pane, 1.0, 1.7);
	ccEnable(chap->pane);
	wchar_t bufferw[MAX_PATH+1];
	chap->icons.chapters = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/chapters32.png"));
	chap->icons.play = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/play32.png"));

	return 1;
}

static inline int page_chapShutdown (TCHAPTER *chap, TVLCPLAYER *vp)
{
	chaptersLock(chap);
	ccDelete(chap->pane);
	
	if (chap->titles.list)
		chapterFreeTitles(chap->titles.list, chap->titles.total);

	buttonsDeleteAll(chap->btns);	
	lockClose(chap->titles.lock);
	return 1;
}

static inline int page_chapInput (TCHAPTER *chap, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_TOUCH_DOWN:
		if (pos->y < ccGetPositionY(chap->pane))
	  		page2SetPrevious(chap);	
		break;
	  case PAGE_IN_WHEEL_FORWARD:
	  	paneScroll(chap->pane, CHAPTER_SCROLL_DELTA);
		break;
	  case PAGE_IN_WHEEL_BACK:
	  	paneScroll(chap->pane, -CHAPTER_SCROLL_DELTA);
	  	break;
	}
		
	return 1;
}

int page_chapCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TCHAPTER *chap = (TCHAPTER*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		 //printf("# page_chapCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_chapRender(chap, chap->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		return page_chapRenderBegin(chap, chap->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		page_chapRenderEnd(chap, chap->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_chapInput(chap, chap->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_chapStartup(chap, chap->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_chapInitalize(chap, chap->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_chapShutdown(chap, chap->com->vp);
		
	}
	
	return 1;
}

