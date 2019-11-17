
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


extern volatile double UPDATERATE_BASE;



static inline void page2StackPush (TPAGE2 *page, const int id)
{
#if 1
	stackPush(page->stack, id);
#else
	my_memcpy(&page->stack[0], &page->stack[1], sizeof(TPAG2EHISTORY)*(PAGES_STACKSIZE-1));
	page->stack[PAGES_STACKSIZE-1].id = id;
#endif
}

static inline int page2StackPop (TPAGE2 *page)
{

#if 1
	intptr_t id = 0;
	stackPop(page->stack, &id);
	if (!id) id = PAGE_NONE;
	
	return id;
#else
	const int previous = page->stack[PAGES_STACKSIZE-1].id;
	for (int i = PAGES_STACKSIZE-1; i > 0; i--)
		my_memcpy(&page->stack[i], &page->stack[i-1], sizeof(TPAG2EHISTORY));
		
	return previous;
#endif
}

static inline TPAGE2COM *page2ComAlloc (void *pageStruct)
{
	return (*(void**)pageStruct = my_calloc(1, sizeof(TPAGE2COM)));
}

static inline TPAGE2COM *page2ComGet (void *pageStruct)
{
	//printf("page2ComGet pageStruct:%p, com:%p\n", pageStruct, (TPAGE2COM*)*(void**)pageStruct);
	return (TPAGE2COM*)*(void**)pageStruct;
}

static inline TPAGE2COM *page2ComSet (void *pageStruct, TPAGE2 *page, TPAGES2 *pages, TVLCPLAYER *vp)
{
	TPAGE2COM *com = page2ComGet(pageStruct);
	if (!com) com = page2ComAlloc(pageStruct);

	if (com){
		com->page = page;
		com->pages = pages;
		com->vp = vp;
	}
	return com;
}

static inline TPAGES2 *page2PagesGet (void *pageStruct)
{
	return page2ComGet(pageStruct)->pages;
}

int page2Add (TPAGES2 *pages, TPAGE2 *page)
{
	if (!pages->tPages){
		pages->page = my_malloc(sizeof(TPAGE2*));
		if (!pages->page) return 0;
	}
	
	pages->page = my_realloc(pages->page, (pages->tPages+1) * sizeof(TPAGE2*));
	if (pages->page){
		pages->page[pages->tPages] = page;
		return ++pages->tPages;
	}else{
		return 0;
	}
}

static inline TPAGE2 *page2Get (TPAGES2 *pages, int id)
{
	id &= PAGE_PAGEMASK;
	
	for (int i = 0; i < pages->tPages; i++){
		if (pages->page[i]){
			if (pages->page[i]->id == id)
				return pages->page[i];
		}
	}
	return NULL;
}

static inline int page2SendMessage (TPAGE2 *page, const int msg, const int64_t dataInt1, const int64_t dataInt2, void *dataPtr)
{
	/*if (msg == PAGE_CTL_RENDER_START)
		printf("renderStart %s\n", page->title);
	else if (msg == PAGE_CTL_RENDER_END)
		printf("renderEnd %s\n", page->title);	
	else if (msg == PAGE_CTL_RENDER)
		printf("render %s\n", page->title);
	*/
	
	
	//printf("pageSendMessage() id:%i, msg:%i, d1:%I64d, d2:%I64d, dp:%p, op:%p\n", page->id, msg, dataInt1, dataInt2, dataPtr, page->opaquePtr);
	int ret = page->callback(page->pageStruct, msg, dataInt1, dataInt2, dataPtr, page->opaquePtr);
	//printf("pageSendMessage() ret:%i, id:%i, msg:%i, d1:%I64d, d2:%I64d, dp:%p, op:%p\n", ret, page->id, msg, dataInt1, dataInt2, dataPtr, page->opaquePtr);
	return ret;
}

// send message to everyone
int pageDispatchMessage (TPAGES2 *pages, const int msg, const int64_t dataInt1, const int64_t dataInt2, void *dataPtr)
{
	int ret = 0;

	for (int i = 0; i < pages->tPages; i++){
		TPAGE2 *page = pages->page[i];
		if (page && page->isEnabled){
			int status = page2SendMessage(page, msg, dataInt1, dataInt2, dataPtr);
			//printf("status %i %i %i\n", i, page->id, status);
			if (!status) break;
			ret += status;
		}
	}

	return ret;
}

// send message to single page only
int pageSendMessage (TPAGES2 *pages, const int pageId, const int msg, const int64_t dataInt1, const int64_t dataInt2, void *dataPtr)
{
	TPAGE2 *page = page2Get(pages, pageId&PAGE_PAGEMASK);
	return page2SendMessage(page, msg, dataInt1, dataInt2, dataPtr);
}

int page2OpaqueSet (TPAGES2 *pages, const int id, void *opaquePtr)
{
	TPAGE2 *page = page2Get(pages, id);
	if (page){
		page->opaquePtr = opaquePtr;
		return 1;
	}
	return 0;
}

void *page2OpaqueGet (TPAGES2 *pages, const int id)
{
	if (id < PAGE_BASEID){
		//printf("page2OpaqueGet. invalid page %i\n", id);
		return 0;
	}
	
	void *opaquePtr = NULL;
	
	TPAGE2 *page = page2Get(pages, id);
	if (page){
		opaquePtr = page->opaquePtr;
		//if (!opaquePtr)
		//	printf("pages2: ::opaquePtr requested but not set for page %i:'%s'\n", id, page->title);
	}
	return opaquePtr;
}

// shouldn't be needed
/*static inline int page2PageStructSet (TPAGES2 *pages, const int id, void *pageStruct)
{
	TPAGE2 *page = page2Get(pages, id);
	if (page){
		page->pageStruct = pageStruct;
		return 1;
	}
	return 0;
}*/

int page2InputDragGetState (TPAGES2 *pages, const int id)
{
	TPAGE2 *page = page2Get(pages, id);
	if (page)
		return page->inputWantDrag;
		
	return 0;
}

int page2InputDragEnable (TPAGES2 *pages, const int id)
{
	TPAGE2 *page = page2Get(pages, id);
	if (page)
		return (page->inputWantDrag = 1);
		
	return 0;
}

int page2InputDragDisable (TPAGES2 *pages, const int id)
{
	TPAGE2 *page = page2Get(pages, id);
	if (page)
		return !(page->inputWantDrag = 0);
		
	return 0;
}

static inline int page2StateGet (TPAGES2 *pages, const int id, const int stateFlag)
{
	int state = 0;
	TPAGE2 *page = page2Get(pages, id);
	if (page)
		state = page->initState[stateFlag]&0x01;

	return state;
}

// should probably put a lock around this
static inline int page2StateSet (TPAGES2 *pages, const int id, const int stateFlag, const int state)
{
	int oldState = 0;
	TPAGE2 *page = page2Get(pages, id);
	if (page){
		oldState = page->initState[stateFlag];
		page->initState[stateFlag] = (state != 0);
	}
	return oldState;
}

static inline int page2Startup (TPAGES2 *pages, const int id)
{
	//printf("page2Startup id:%i state:%i\n", id, page2StateGet(pages, id, PAGE_CTL_STARTUP));
	
	TPAGE2 *page = page2Get(pages, id);

	if (!page2StateGet(pages, id, PAGE_CTL_STARTUP)){
		page2StateSet(pages, id, PAGE_CTL_STARTUP, 1);
		
		TPAGE2COM *com = page2ComGet(page->pageStruct);
		TFRAME *front = getFrontBuffer(com->vp);
		
		//printf("page2Startup '%s'\n", page->title);
		
		int startupOk = page2SendMessage(page, PAGE_CTL_STARTUP, front->width, front->height, NULL);
		if (!startupOk)
			page->isEnabled = 0;

		//printf("page2Startup b startOK:%i\n", startupOk);

		page2StateSet(pages, id, PAGE_CTL_STARTUP, startupOk);
		return startupOk;
	}else{
		return 1;
	}
}

static inline int page2Initialize (TPAGES2 *pages, const int id)
{
	TPAGE2 *page = page2Get(pages, id);

	if (!page2StateGet(pages, id, PAGE_CTL_STARTUP))
		return 0;
	
	if (!page2StateGet(pages, id, PAGE_CTL_INITIALIZE)){
		TPAGE2COM *com = page2ComGet(page->pageStruct);
		TFRAME *front = getFrontBuffer(com->vp);

		//printf("page2Initialize %i, '%s'\n", page->id, page->title);
		
		int initializeOk = page2SendMessage(page, PAGE_CTL_INITIALIZE, front->width, front->height, front);
		if (!initializeOk)
			page->isEnabled = 0;
		else
			page->isEnabled = 1;
		page2StateSet(pages, id, PAGE_CTL_INITIALIZE, page->isEnabled);
		
	}
	return page->isEnabled;
}

int page2Enable (TPAGES2 *pages, const int id)
{
	//printf("page2Enable %i\n", id);
	
	int ret = 0;
	//double t0 = getTime(pages->vp);

	if (page2Startup(pages, id))
		ret = page2Initialize(pages, id);
	//double t1 = getTime(pages->vp);
	
	//TPAGE2 *page = page2Get(pages, id);
	//printf("  page2Enable %.2f '%s'\n" ,t1-t0, page->title);
	
	return ret;
}

void *page2PageStructGet (TPAGES2 *pages, const int id)
{
	//printf("page2PageStructGet %i\n", id);
	
	if (id < PAGE_BASEID){
		//printf("page2PageStructGet. invalid page %i\n", id);
		return 0;
	}
	
	void *pageStruct = NULL;
	
	TPAGE2 *page = page2Get(pages, id);
	if (page){
		pageStruct = page->pageStruct;
		//if (!pageStruct)
		//	printf("pages2: ::pageStruct requested but not set for page %i:'%s'\n", id, page->title);

		if (!page2StateGet(pages, page->id, PAGE_CTL_STARTUP)){
			if (!page2Enable(pages, id))
				pageStruct = NULL;
		}
	}
	
	//if (!pageStruct)
	//	printf("page2PageStructGet failed %i\n", id);
	
	return pageStruct;
}

static inline int page2RenderEnable (TPAGES2 *pages, int id)
{
	id &= PAGE_PAGEMASK;
	
	TPAGE2 *page = page2Get(pages, id);
	if (!page->readyToRender){
		page->zDepthId = ++pages->zDepthId;
		
		if (!page2StateGet(pages, id, PAGE_CTL_RENDER_INIT)){
			page2StateSet(pages, id, PAGE_CTL_RENDER_INIT, 1);
			page2SendMessage(page, PAGE_CTL_RENDER_INIT, getTime(pages->vp), page->zDepthId, getFrontBuffer(pages->vp));
		}

		page->readyToRender = page2SendMessage(page, PAGE_CTL_RENDER_START, getTime(pages->vp), page->zDepthId, getFrontBuffer(pages->vp));
		
		if (page->readyToRender){
			page2StateSet(pages, page->id, PAGE_CTL_RENDER_START, 1);
			page2StateSet(pages, page->id, PAGE_CTL_RENDER_END, 0);
		}else{
			page2StateSet(pages, page->id, PAGE_CTL_RENDER_START, 0);
			page2StateSet(pages, page->id, PAGE_CTL_RENDER_END, 1);
		}
	}

	return page->readyToRender;
}

static inline void page2RenderEnd (TPAGES2 *pages, const int endId, const int goingToId)
{
	int pageId = endId&PAGE_PAGEMASK;
	if (pageId == pages->basePageId && !(endId&PAGE_CLOSEBASE))
		return;
			
	TPAGE2 *page = page2Get(pages, pageId);
	//page->isConcurrent = 0;
	if (page->readyToRender){
		page->readyToRender = 0;
		page2SendMessage(page, PAGE_CTL_RENDER_END, goingToId, 0, NULL);
		page2StateSet(pages, page->id, PAGE_CTL_RENDER_START, 0);
		page2StateSet(pages, page->id, PAGE_CTL_RENDER_END, 1);
	}
}

void pageUpdate (void *pageStruct)
{
	TPAGE2COM *com = page2ComGet(pageStruct);
	renderSignalUpdate(com->vp);
}

void page2RenderDisable (TPAGES2 *pages, const int id)
{
	page2RenderEnd(pages, id, 0);
}


// is this page set to render
static inline unsigned int page2RenderActiveGet (TPAGES2 *pages, const int id)
{
	TPAGE2 *page = page2Get(pages, id);
	return page->readyToRender;
}

unsigned int page2RenderGetState (TPAGES2 *pages, const int id)
{
	return page2RenderActiveGet(pages, id);
}

static inline void page2InputSetState (TPAGE2 *page, const unsigned int state)
{
	page->inputEnabled = state&0x01;
}

unsigned int page2InputGetState (TPAGES2 *pages, const int id)
{
	TPAGE2 *page = page2Get(pages, id);
	return page->inputEnabled;
}

void page2InputEnable (TPAGES2 *pages, const int id)
{
	TPAGE2 *page = page2Get(pages, id);
	page2InputSetState(page, 1);
}

void page2InputDisable (TPAGES2 *pages, const int id)
{
	TPAGE2 *page = page2Get(pages, id);
	page2InputSetState(page, 0);
}

int page2IsInitialized (TPAGES2 *pages, const int id)
{
	return page2StateGet(pages, id, PAGE_CTL_RENDER_INIT);
}

// set this page as the only active render target, disabling all others	(once enabled ::basePageId can never be disabled)
static inline int page2RenderActiveSet (TPAGES2 *pages, const int id)
{
	const int pageId = id&PAGE_PAGEMASK;
		
	if (!(id&PAGE_RENDER_CONCURRENT)){
		// disable all
		for (int i = 0; i < pages->tPages; i++){
			TPAGE2 *page = pages->page[i];
		
			if (page->readyToRender && page->id != pageId)
				page2RenderEnd(pages, page->id, id);
		}
	}
	
	return page2RenderEnable(pages, pageId);
}

static inline int page2InputTouchCall (TPAGE2 *page, TTOUCHCOORD *pos, const int flags)
{
	int inType = 0;
	
	if (!pos->pen && flags == 0)
		inType = PAGE_IN_TOUCH_DOWN;
	else if (!pos->pen && flags == 1)
		inType = PAGE_IN_TOUCH_SLIDE;
	else if (pos->pen && flags == 3)
		inType = PAGE_IN_TOUCH_UP;
	//else
	//	printf("page2InputTouchCall input unhandled page:%i pen:%i flags:%X\n", page->id, pos->pen, flags);
			
	return page2SendMessage(page, PAGE_CTL_INPUT, inType, flags, pos);
}

static inline int page2InputMouseCall (TPAGE2 *page, const unsigned int *mpos, const int flags)
{
	return page2SendMessage(page, PAGE_CTL_INPUT, flags, *mpos, NULL);
}

static inline int pageInputGetTopMostId (TPAGES2 *pages)
{
	//printf("pageInputGetTopMostId tPages %i\n", pages->tPages);

	int id = 0;
	int zDepth = 0;
	
	for (int i = 0; i < pages->tPages; i++){
		TPAGE2 *page = pages->page[i];
		if (!page/* || page->isConcurrent*/) continue;

		//printf("pageInputGetTopMostId %i %i %p, %i %i\n", i, page->id, page, page->readyToRender, page->inputEnabled);

		if (page->readyToRender && page->inputEnabled){
			if (page->zDepthId > zDepth){
				zDepth = page->zDepthId;
				id = page->id;
			}
		}
	}
	return id;
}

static inline int pageRenderGetTopMostId (TPAGES2 *pages)
{
	//printf("pageRenderGetTopMostId tPages %i\n", pages->tPages);

	int id = 0;
	int zDepth = 0;
	
	for (int i = 0; i < pages->tPages; i++){
		TPAGE2 *page = pages->page[i];
		if (!page) continue;

		//printf("pageRenderGetTopMostId %i %p\n", i, page);

		if (page->readyToRender){
			if (page->zDepthId > zDepth){
				zDepth = page->zDepthId;
				id = page->id;
			}
		}
	}
	return id;
}

int pageInputGetTop (TPAGES2 *pages)
{
	TPAGE2 *page = page2Get(pages, pageInputGetTopMostId(pages));
	if (page)
		return page->id;
	else
		return 0;
}

static inline TPAGE2 *pageInputGetTopMostPage (TPAGES2 *pages)
{
	return page2Get(pages, pageInputGetTopMostId(pages));
}

int pageRenderGetTop (TPAGES2 *pages)
{
	TPAGE2 *page = page2Get(pages, pageRenderGetTopMostId(pages));
	if (page)
		return page->id;
	else
		return 0;
}

TPAGE2 *pageRenderGetTopMostPage (TPAGES2 *pages)
{
	return page2Get(pages, pageRenderGetTopMostId(pages));
}

int page2Input (TPAGES2 *pages, const int inType, void *ptr, const unsigned int flags)
{
	//printf("page2Input flags:%X\n", flags);
	TPAGE2 *page = pageInputGetTopMostPage(pages);
	if (!page){
		//printf("page2Input, no top most page\n");
		return 0;
	}
	
	
	if (inType == PAGE_IN_TOUCH){
		//TTOUCHCOORD *pos = (TTOUCHCOORD*)ptr;
		return page2InputTouchCall(page, ptr, flags);

	}else if (inType == PAGE_IN_MOUSE){
		if (flags == PAGE_IN_WHEEL_FORWARD || flags == PAGE_IN_WHEEL_BACK || flags == PAGE_IN_WHEEL_LEFT || flags == PAGE_IN_WHEEL_RIGHT){
			if (page2InputMouseCall(page, ptr, flags)){
				renderSignalUpdate(pages->vp);
				return 1;
			}
		}
	}

	return 0;
}

// render whats signaled as render ready, if id supplied then render topmost without having to set topmost
int page2Render (TPAGES2 *pages, TFRAME *frame, const int id)
{

	uint64_t t0 = getTime(pages->vp)*1000.0;
	int ct = 1;

	
	TPAGE2 *pageZList[pages->tPages];
	int zTotal = 0;
	
	// render anything thats set to renderable, apart from id which will be top rendered
	for (int i = 0; i < pages->tPages; i++){
		TPAGE2 *page = pages->page[i];
		
		//if (page->isConcurrent)
		//	printf("@@ %i %i '%s'\n", page->isConcurrent, page->readyToRender, page->title);
		
		if ((page->readyToRender || page->isConcurrent) && page->id != id)
			pageZList[zTotal++] = page;
	}
	
	// sort in to descending order by Z depth
	for (int i = 0; i < zTotal; i++){
		for (int j = 0; j < zTotal-1; j++){
			if (pageZList[j]->zDepthId < pageZList[j+1]->zDepthId){
				void *ptr = pageZList[j];
				pageZList[j] = pageZList[j+1];
				pageZList[j+1] = ptr;
			}
		}
	}

	// do the rendering from bottom up, non concurrent first (bottom most)
	int concurrentTotal = 0;
	int total = zTotal;
	while (total--){
		TPAGE2 *page = pageZList[total];
		if (!page->isConcurrent){
			int renderOk = page2SendMessage(page, PAGE_CTL_RENDER, t0, ct, frame);
			if (!renderOk && page2StateGet(pages, page->id, PAGE_CTL_RENDER_START))
				page2RenderEnd(pages, page->id, id);

			ct += renderOk;
		}else{
			concurrentTotal++;
		}
	}
	
	//printf("concurrentTotal %i\n", concurrentTotal);
	
	// now render whatever is set as concurrent	
	if (concurrentTotal){
		total = zTotal;
		while (total--){
			TPAGE2 *page = pageZList[total];
		
			if (page->isConcurrent){
				int renderOk = 0;
				if (!page->readyToRender)
					page->readyToRender = page2RenderEnable(pages, page->id);

				if (page->readyToRender){
					renderOk = page2SendMessage(page, PAGE_CTL_RENDER, t0, ct, frame);
					if (!renderOk && page2StateGet(pages, page->id, PAGE_CTL_RENDER_START))
						page2RenderEnd(pages, page->id, id);

					ct += renderOk;
				}
				if (!renderOk){
					page->isConcurrent = 0;
					page2RenderDisable(pages, page->id);
				}
			}
		}
	}
	
	// finally render top most the selected page
	if (id >= 0 && page2StateGet(pages, id, PAGE_CTL_STARTUP)){
		int renderOk = 0;
		TPAGE2 *page = page2Get(pages, id);
		if (!page->readyToRender)
			page->readyToRender = page2RenderEnable(pages, id);
		
		if (page->readyToRender){
			renderOk = page2SendMessage(page, PAGE_CTL_RENDER, t0, ct, frame);
			ct += renderOk;
		}
			
		if (!renderOk)
			page2RenderDisable(pages, id);
	}
	return ct;
}

int page2Set (TPAGES2 *pages, const int id, const int setStack)
{
	//printf("\n# page2Set id:%i, modStack:%i\n", id, setStack);
	
	if (id < PAGE_BASEID){
		//printf("page2Set: invalid page %i\n", id);
		//return 0;
		return page2Set(pages, PAGE_DEFAULT, 1);
	}

	TPAGE2 *pageCurrent = pageInputGetTopMostPage(pages);	// this is where we are
	TPAGE2 *pageTo = page2Get(pages, id);					// this is where we want to go
	if (!pageCurrent || !pageTo) return 0;

	// shouldn't be here
	if (kHookGetState()){
		kHookOff();
		kHookUninstall();
	}
	
	TVLCPLAYER *vp = pages->vp;
	int rate = pageTo->updateRate;
	if (rate)
		setTargetRate(vp, rate);
	else
		setTargetRate(vp, UPDATERATE_BASE);

	const float t0 = getTime(vp);
 	if (t0 - vp->lastRenderTime > ((1.0/(double)(UPDATERATE_MAXUI+10.0))*1000.0))
		renderSignalUpdate(vp);


	pageTo->isConcurrent = ((id&PAGE_RENDER_CONCURRENT) != 0);
	//printf("# page2Set id:%i, '%s', modStack:%i %i\n", id, pageTo->title, setStack, pageTo->isConcurrent);

	if (pageRenderGetTopMostId(pages) == id || pageCurrent->id == id)  // do nothing if we're already there
		return 1;

	if (!page2StateGet(pages, pageTo->id, PAGE_CTL_STARTUP)){
		if (!page2Enable(pages, pageTo->id))
			return 0;
	}
	
	// tell destination where to return to (the previous page)
	if (setStack)
		page2StackPush(pageTo, pageCurrent->id);
	
	// disable rendering of current/visible page
	if (!(id&PAGE_RENDER_CONCURRENT))
		page2RenderEnd(pages, pageCurrent->id, id);
	
	// switch render target to new page
	int ret = page2RenderActiveSet(pages, id);
	if (ret) renderSignalUpdate(pages->vp);
	return ret;
}

int page2SetPrevious (void *pageStruct)
{
	TPAGE2COM *com = page2ComGet(pageStruct);
	TPAGE2 *pageCurrent = com->page;
	
	int id = page2StackPop(pageCurrent);
	//printf("page2SetPrevious %i %i\n", id, pageCurrent->id);
	
	if (id == pageCurrent->id)
		return 1;

	if (page2Set(com->pages, id, 0))
		return id;
	return 0;
}

void page2Disable (TPAGES2 *pages, const int id)
{
	if (page2StateGet(pages, id, PAGE_CTL_STARTUP)){
		TPAGE2 *page = page2Get(pages, id);

		if (page2StateGet(pages, id, PAGE_CTL_RENDER_START))
			page2RenderDisable(pages, id|PAGE_CLOSEBASE);

		page2SendMessage(page, PAGE_CTL_SHUTDOWN, 0, 0, NULL);
		page2StateSet(pages, id, PAGE_CTL_STARTUP, 0);
		page2StateSet(pages, id, PAGE_CTL_SHUTDOWN, 1);
	}
}

int page2Register (TPAGES2 *pages, const char *title, const int id, const int updateRate, TPageCallback_t callback, const size_t dataStructSize)
{
	TPAGE2 *page = my_calloc(1, sizeof(TPAGE2));
	if (page){
		page->id = id;
		page->title = my_strdup(title);
		page->updateRate = updateRate;
		page->callback = callback;
		page->opaquePtr = NULL;
		page->stack = stackCreate(PAGES_STACKSIZE);
		page->pageStruct = my_calloc(1, dataStructSize);
		
		if (page->pageStruct){
			page2ComSet(page->pageStruct, page, pages, pages->vp);
			
			int ok = page2Add(pages, page);
			if (ok && id == pages->basePageId){	//	ensure the base page (fallback) is initialize asap
				if (page2Enable(pages, id))
					ok = page2RenderActiveSet(pages, id);
			}
			
			page2InputSetState(page, 1);
			return ok;
		}else{
			my_free(page);
		}
	}
	return 0;
}

void page2Free (TPAGE2 *page)
{
	TPAGE2COM *com = page2ComGet(page->pageStruct);
	if (com) my_free(com);
	if (page->pageStruct) my_free(page->pageStruct);
	if (page->stack) stackDestroy(page->stack);
	my_free(page->title);
	my_free(page);
}

TPAGES2 *pages2New (TVLCPLAYER *vp, const int basePageId)
{
	TPAGES2 *pages = my_calloc(1, sizeof(TPAGES2));
	if (pages){
		pages->vp = vp;
		pages->basePageId = basePageId;
		pages->tPages = 0;
		pages->page = NULL;
	}
	return pages;
}

void pages2Delete (TPAGES2 *pages)
{
	for (int i = 0; i < pages->tPages; i++){
		page2Disable(pages, pages->page[i]->id);
		page2Free(pages->page[i]);
		pages->page[i] = NULL;
	}

	my_free(pages->page);
	my_free(pages);
}
