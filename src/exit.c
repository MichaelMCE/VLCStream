
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


const int border = 18;
	

void exitInitShutdown (TVLCPLAYER *vp)
{
  	//pageSetSec(vp, -1);
  	
	if (mHookGetState()){
		captureMouse(vp, 0);
		mHookUninstall();
	}

	if (kHookGetState()){
		kHookOff();
		kHookUninstall();
	}
  	timerSet(vp, TIMER_SHUTDOWN, 100);
  	renderSignalUpdate(vp);
}


static inline int exitButtonPress (TEXIT *exit, const int id, const TTOUCHCOORD *pos)
{
	TVLCPLAYER *vp = exit->com->vp;
	exit->btns->t0 = getTickCount();


	switch (id){
	  case EXITBUTTON_YES:
	  	pageDisable(vp, PAGE_EXIT);
		exitInitShutdown(vp);
	  	break;

	  case EXITBUTTON_NO:
	  	page2Set(vp->pages, PAGE_EXIT, 0);		// remove concurrent
	  	page2SetPrevious(exit);
	  	renderSignalUpdate(vp);
	  	break;
	  	
	  case EXITBUTTON_GOIDLE:
	  	page2Set(vp->pages, PAGE_EXIT, 0);		// remove concurrent
	  	pageDisable(vp, PAGE_EXIT);
	  	
	  	if (getPlayState(vp)) trackStop(vp);
	  	timerSet(vp, TIMER_SETIDLEA, 300);
	  	renderSignalUpdate(vp);
	  	break;
	}
	return 1;
}	

static inline int64_t ccbtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;
		
	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//printf("ccbtn_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);

	TCCBUTTON *btn = (TCCBUTTON*)object;
	//const int id = (int)data2;

	if (msg == BUTTON_MSG_SELECTED_PRESS){
		//printf("exit ccbtn_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);
		return exitButtonPress(ccGetUserData(btn), ccGetUserDataInt(btn), dataPtr);
	}

	return 1;
}

static inline int page_exitRender (TEXIT *exit, TVLCPLAYER *vp, TFRAME *frame)
{
	TFRAME *et = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_EXIT]);
	TFRAME *eb = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_EXITBOX]);
	
	const int x = (frame->width - et->width)/2;
	const int y = exit->pos.y + border;
	exit->pos.x = (frame->width - eb->width)/2;
	exit->pos.y = (frame->height - eb->height)/2;
	
	const int boxOffset = 4;
	lBlurArea(frame, exit->pos.x+boxOffset+2, exit->pos.y+boxOffset+2, exit->pos.x+eb->width-(boxOffset<<1), exit->pos.y+eb->height-(boxOffset<<1)+3, 2);
	drawImage(eb, frame, exit->pos.x, exit->pos.y, eb->width-1, eb->height-1);
	drawImage(et, frame, x, y, et->width-1, et->height-1);
	
	buttonsRenderAll(exit->btns, frame, BUTTONS_RENDER_HOVER);
	
	imageManagerImageRelease(vp->im, vp->gui.image[IMGC_EXIT]);
	imageManagerImageRelease(vp->im, vp->gui.image[IMGC_EXITBOX]);
	
	return 1;
}

static inline int page_exitInput (TEXIT *exit, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	/*switch(msg){
	  case PAGE_IN_TOUCH_DOWN:
	  case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  	break;
	}*/
		
	return 1;
}

static inline int page_exitStartup (TEXIT *exit, TVLCPLAYER *vp, const int fw, const int fh)
{
	
	return 1;
}

static inline int page_exitInitalize (TEXIT *exit, TVLCPLAYER *vp, const int fw, const int fh)
{
	
	vp->gui.image[IMGC_EXIT] = imageManagerImageAdd(vp->im, L"exit/exit.png");
	vp->gui.image[IMGC_EXITBOX] = imageManagerImageAdd(vp->im, L"exit/exitbox.png");
	
	int w,h;
	if (!imageManagerImageGetMetrics(vp->im, vp->gui.image[IMGC_EXITBOX], &w, &h))
		return 0;
	
	exit->pos.x = (fw - w)/2;
	exit->pos.y = (fh - h)/2;

	exit->btns = buttonsCreate(vp->cc, PAGE_EXIT, EXITBUTTON_TOTAL, ccbtn_cb);

	TCCBUTTON *btn = buttonsCreateButton(exit->btns, L"exit/yes.png", NULL, EXITBUTTON_YES, 1, 0, 0, 0);
	ccSetUserData(btn, exit);
	int x = exit->pos.x + border;
	int y = (exit->pos.y + h) - ccGetHeight(btn) - border - 2;
	ccSetPosition(btn, x, y);

	btn = buttonsCreateButton(exit->btns, L"exit/no.png", NULL, EXITBUTTON_NO, 1, 0, 0, 0);
	ccSetUserData(btn, exit);
	x = (exit->pos.x + w) - ccGetWidth(btn) - border;
	y = (exit->pos.y + h) - ccGetHeight(btn) - border - 2;
	ccSetPosition(btn, x, y);

	btn = buttonsCreateButton(exit->btns, L"exit/idle.png", NULL, EXITBUTTON_GOIDLE, 1, 0, 0, 0);
	ccSetUserData(btn, exit);
	x = exit->pos.x + ((w - ccGetWidth(btn))/2);
	y = (exit->pos.y + h) - ccGetHeight(btn) - border - 2;
	ccSetPosition(btn, x, y);
	
	return 1;
}

static inline int page_exitShutdown (TEXIT *exit, TVLCPLAYER *vp)
{
	buttonsDeleteAll(exit->btns);
	return 1;
}

int page_exitCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TEXIT *exit = (TEXIT*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_exitCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_exitRender(exit, exit->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){

	}else if (msg == PAGE_CTL_RENDER_END){
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_exitInput(exit, exit->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_exitStartup(exit, exit->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_exitInitalize(exit, exit->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_exitShutdown(exit, exit->com->vp);
		
	}
	
	return 1;
}


