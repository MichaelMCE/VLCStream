
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



typedef struct {
	TVLCPLAYER *vp;
	TTOUCHCOORD pos;
	int flags;
	unsigned int id;
}TMBCLICK;

#define MBCLICKD_TOTAL (511)

typedef struct{
	TMBCLICK mbclick[MBCLICKD_TOTAL+1];
	int order;
	int listUpper;
}TMBDISPATCH;
static TMBDISPATCH mDispatch;





void touchIn (TTOUCHCOORD *pos, int flags, void *ptr)
{
	
	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;
	if (!vp || !getApplState(vp)) return;

	TTOUCHINPOS *tin = my_malloc(sizeof(TTOUCHINPOS));
	if (tin){
		tin->pos = *pos;
		tin->flags = flags;
		tin->ptr = ptr;
		PostMessage(vp->gui.hMsgWin, WM_TOUCHIN, (WPARAM)tin, (LPARAM)vp);
	}
}

static inline int touchDispatch (TTOUCHCOORD *pos, int flags, TVLCPLAYER *vp)
{

	static unsigned int prevId = 0;	//	... must remove these statics...
	static unsigned int id = 0;

	wakeup(vp);

	flags &= 0xFFF;			// remove TOUCH_VINPUT and anything thats not directly touch related
	if (!flags)				// we could use the libmylcd supplied Id but that blocks us from using TOUCH_VINPUT
		pos->id = ++id;		// so we generate our own id
	else
		pos->id = id;

	// each touch down to up stream is given a unique id
	// we try to use this id to detect invalid, corrupt or out of order data
	if (!flags){		// pen down
		if (prevId >= pos->id)
			return 0;
		prevId = pos->id;
	}else if (prevId != pos->id){
		return 0;
	}

	const int pageId = pageInputGetTop(vp->pages);

	//printf("touchDispatch in: page:%i, canDrag:%i, wantDrag:%i\n", pageId, ccCanDrag(vp->cc, pos, pageId), page2InputDragGetState(vp->pages, pageId));

	if (!page2InputDragGetState(vp->pages, pageId) && !ccCanDrag(vp->cc, pos, pageId)){
		if (flags == 1 || pos->dt < vp->gui.inputCallLength+80.0){
			vp->gui.inputCallLength = 0.0;
			return -2;
		}else{
			vp->gui.inputCallLength = 0.0;
		}
	}

	const double t0 = getTime(vp);
	//printf("ccHandleInputAll %i\n", pageId);
	int ret = ccHandleInputAll(vp->cc, pos, flags, pageId);
	if (!ret){
		//printf("page2Input %X\n", flags);
		ret = page2Input(vp->pages, PAGE_IN_TOUCH, pos, flags);
	}

	//printf("touchDispatch: page2Input() ret %i\n", ret);
	vp->gui.inputCallLength = getTime(vp) - t0;
	return ret;
}

int touchDispatchFilter (TTOUCHCOORD *pos, const int flags, TVLCPLAYER *vp)
{
	TGUI *gui = &vp->gui;
	TGUIINPUT *cursor = &vp->gui.cursor;
	const int touchState = flags&~TOUCH_VINPUT;
	
	if (touchState == 0)
		gui->cursor.LBState = 1;
	else if (touchState == 1)
		gui->cursor.LBState = 2;
	else if (touchState == 3)
		gui->cursor.LBState = 0;

	if (gui->cursor.LBState == 1){		// mouse down
		cursor->dragRect0.x = cursor->dx;
		cursor->dragRect0.y = cursor->dy;
		cursor->dragRect1.x = cursor->dragRect0.x;
		cursor->dragRect1.y = cursor->dragRect0.y;
		cursor->dragRectDelta.x = 0;
		cursor->dragRectDelta.y = 0;
		
	}else if (gui->cursor.LBState == 2){	// mouse drag
		cursor->dragRect1.x = cursor->dx;
		cursor->dragRect1.y = cursor->dy;
		cursor->dragRectDelta.x = cursor->dragRect1.x - cursor->dragRect0.x;
		cursor->dragRectDelta.y = cursor->dragRect1.y - cursor->dragRect0.y;
#if 0
	}else if (gui->cursor.dragRectIsEnabled && touchState/*gui->cursor.LBState*/ == 3){	// mouse up
		int ret = ccDumpRect(vp->cc->objs, pageGet(vp), cursor->dragRect0.x, cursor->dragRect0.y, cursor->dragRect1.x, cursor->dragRect1.y);
		printf("%i: %i,%i %i,%i - %i %i %i %i - %i,%i - %i\n", ret, gui->cursor.x, gui->cursor.y, gui->cursor.dx, gui->cursor.dy, touchState, gui->cursor.LBState, gui->cursor.MBState, gui->cursor.RBState, (int)cursor->dragRectDelta.x, (int)cursor->dragRectDelta.y, flags);
#endif
	}

	//printf("%i: %i , %i %i,%i - %i %i %i %i - %i,%i - %i\n", touchState, gui->cursor.x, gui->cursor.y, gui->cursor.dx, gui->cursor.dy, touchState, gui->cursor.LBState, gui->cursor.MBState, gui->cursor.RBState, (int)cursor->dragRectDelta.x, (int)cursor->dragRectDelta.y, flags);

	//void distort_mouse (int state, int x, int y);
	return touchDispatch(pos, flags, vp);
}

void touchDispatcherStart (TVLCPLAYER *vp, const void *fn, const void *ptr)
{
	memset(&mDispatch, 0, sizeof(TMBDISPATCH));
	mDispatch.listUpper = -1;
	
	
	if (sbuiGetLibmylcdDID(vp->ml->hw)){
		if (!sbuiGestureCBEnable(vp)){
		//	printf("sbui gesture cb initialization failed\n");
		}else if (!sbuiDKCBEnable(vp)){
		//	printf("sbui dk cb initialization failed\n");
		}
	}else{
		const char *device[] = {"USBD480:LIBUSBHID", "USBD480:LIBUSB"};
		for (int i = 0; i < 2; i++){
			lDISPLAY did = lDriverNameToID(vp->ml->hw, device[i], LDRV_DISPLAY);
			if (did){
				lSetDisplayOption(vp->ml->hw, did, lOPT_USBD480_TOUCHCB, (intptr_t*)fn);
				lSetDisplayOption(vp->ml->hw, did, lOPT_USBD480_TOUCHCBUSERPTR, (intptr_t*)ptr);
				break;
			}
		}
	}


#if MOUSEHOOKCAP
	startMouseCapture(vp);
#endif
}

void touchDispatcherStop (TVLCPLAYER *vp)
{
#if MOUSEHOOKCAP
	endMouseCapture(vp);
#endif

	if (sbuiGetLibmylcdDID(vp->ml->hw)){
		sbuiDKCBDisable(vp);
		sbuiGestureCBDisable(vp);
	}

	lDISPLAY did;	
	if ((did=lDriverNameToID(vp->ml->hw, "USBD480:LIBUSBHID", LDRV_DISPLAY))){
		lSetDisplayOption(vp->ml->hw, did, lOPT_USBD480_TOUCHCB, NULL);
	}else if ((did=lDriverNameToID(vp->ml->hw, "USBD480:LIBUSB", LDRV_DISPLAY))){
		lSetDisplayOption(vp->ml->hw, did, lOPT_USBD480_TOUCHCB, NULL);
	}	
}

static inline int waitForDispatchSignal (TVLCPLAYER *vp)
{
	return (WaitForSingleObject(vp->gui.hDispatchEvent, INFINITE) == WAIT_OBJECT_0);
}

static inline int dispatchLock (TVLCPLAYER *vp)
{
	return lockWait(vp->gui.hDispatchLock, INFINITE);
}

static inline void dispatchUnlock (TVLCPLAYER *vp)
{
	lockRelease(vp->gui.hDispatchLock);
}

unsigned int __stdcall inputDispatchThread (void *ptr)
{
	//printf("mouseDispatchThread %i\n", (int)GetCurrentThreadId());
	
	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;
	TMBCLICK *mbroot = my_calloc(MBCLICKD_TOTAL+1, sizeof(TMBCLICK));
	if (mbroot == NULL){
		_endthreadex(0);
		return 0;
	}
	
	do{
		if (waitForDispatchSignal(vp)){
			if (vp->applState){
				int upper = MBCLICKD_TOTAL;
				
				if (dispatchLock(vp)){
					upper = mDispatch.listUpper;
					memcpy(mbroot, mDispatch.mbclick, sizeof(TMBCLICK) * upper/*MBCLICKD_TOTAL*/);
					for (int i = 0; i < upper; i++)
						mDispatch.mbclick[i].id = 0;
					dispatchUnlock(vp);
				}else if (vp->applState){
					continue;
				}else{
					break;
				}

				for (int j = 0; j < upper; j++){
					unsigned int first = (unsigned int)-1;
					TMBCLICK *mbFirst = NULL;
					TMBCLICK *mb = mbroot;
							
					// (FIFO)
					// ensure events are fired in order they arrived, oldest first
					for (int i = 0; i < upper; i++, mb++){
						if (mb->id){
							if (mb->id < first){
								first = mb->id;
								mbFirst = mb;
							}
						}
					}
					if (mbFirst){
						mbFirst->id = 0;
	  					if (vp->applState){
	  						if (renderLock(vp)){
	  							if (vp->applState){
	  								if (mbFirst->flags&TOUCH_VINPUT){
	  									vp->gui.cursor.dx = mbFirst->pos.x;
										vp->gui.cursor.dy = mbFirst->pos.y;
									}
	  								
	  								//printf("touchDispatchFilter in\n");
									touchDispatchFilter(&mbFirst->pos, mbFirst->flags, mbFirst->vp);
									//printf("touchDispatchFilter out\n");
									const double t0 = getTime(vp);
 									if (t0 - vp->lastRenderTime > ((1.0/(double)(UPDATERATE_MAXUI+15.0))*1000.0))
  										renderSignalUpdate(vp);
								}
								renderUnlock(vp);
							}
						}
					}
				}
			}
		}
	}while(vp->applState);
	
	my_free(mbroot);
	_endthreadex(1);
	return 1;
}

void touchSimulate (const TTOUCHCOORD *pos, const int flags, TVLCPLAYER *vp)
{
	if (dispatchLock(vp)){
		if (vp->applState){
			TMBCLICK *mb = mDispatch.mbclick;

			for (int i = 0; i < MBCLICKD_TOTAL; i++, mb++){
				if (!mb->id){
					mb->id = ++mDispatch.order;
					mb->vp = vp;
					mb->flags = flags;
					
					//mb->pos = *pos;
					my_memcpy(&mb->pos, pos, sizeof(TTOUCHCOORD));
					
					if (i > mDispatch.listUpper) mDispatch.listUpper = i+1;
					dispatchUnlock(vp);
					SetEvent(vp->gui.hDispatchEvent);
					return;
				}
			}
		}
		dispatchUnlock(vp);
	}
}


void inputGetCursorPosition (TVLCPLAYER *vp, int *x, int *y)
{
	*x = vp->gui.cursor.dx;
	*y = vp->gui.cursor.dy;
}
