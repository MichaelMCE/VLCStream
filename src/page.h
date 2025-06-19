
// https://github.com/MichaelMCE/VLCStream
// okio@users.sourceforge.net
// Copyright (c) 2005-2025
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



#ifndef _PAGE_H_
#define _PAGE_H_


#define PAGES_STACKSIZE					4



enum _page2Ctrl {
	PAGE_CTL_STARTUP = 1,		// called at application startup
	PAGE_CTL_INITIALIZE,		// called once per startup but just before first PAGE_CTL_OPEN is sent
	PAGE_CTL_RENDER_START,		// called whenever page is to become visible, in focus, just before a PAGE_CTL_RENDER series
	PAGE_CTL_RENDER,			// called when in focus and visible
	PAGE_CTL_RENDER_END,		// called whenever page is nolonger visible, out of focus
	PAGE_CTL_INPUT,				// input generated while in focus
	PAGE_CTL_SHUTDOWN,			// called at application shutdown
	PAGE_CTL_RENDER_INIT,		// sent once per runtime and directly before first render, contains render context identical to PAGE_CTL_RENDER_START
	PAGE_CTL_TOTAL,
	
	PAGE_MSG_DEVICE_ARRIVE	= 101,
	PAGE_MSG_DEVICE_DEPART,

	PAGE_MSG_DRIVE_ARRIVE	= 110,
	PAGE_MSG_MEDIA_ARRIVE,
	PAGE_MSG_DRIVE_DEPART,
	PAGE_MSG_MEDIA_DEPART,
	
	PAGE_MSG_VOLUME_CHANGE	= 120,
	PAGE_MSG_WAKEUP,
	PAGE_MSG_INPUT_MCAP,
	PAGE_MSG_INPUT_KCAP,
	PAGE_MSG_OBJ_HOVER,
	PAGE_MSG_ALARM_FIRED,
	PAGE_MSG_IMAGE_FLUSH,

	PAGE_MSG_CHAR_DOWN,				// delivered via the virtual display
	PAGE_MSG_CHAR_UP,				// not implemented
	PAGE_MSG_KEY_DOWN,				// delivered via the virtual display
	PAGE_MSG_KEY_UP,				// not implemented
	PAGE_MSG_HOTKEY,				// a registered hotkey was activated/pressed. return 1 if handled, 0 to pass on
	
	PAGE_MSG_FILE_SELECTED	= 150,// msg dispatched upon file selected, return 0 if handled locally, otherwise 1 to continue loading (media) file
	PAGE_MSG_CFG_READ,
	PAGE_MSG_CFG_WRITE
};


//#define PAGE_RENDER_NONE	
#define PAGE_RENDER_CONCURRENT		0x10000			// enable page along with anything else thats (also) set renderable
#define PAGE_CLOSEBASE				0x20000
#define PAGE_PAGEMASK				0x00FFF


#define PAGE_IN_TOUCH				1
#define PAGE_IN_MOUSE				2
#define PAGE_IN_ROTARY				3

// PAGE_CTL_INPUT messages
#define PAGE_IN_TOUCH_DOWN			1
#define PAGE_IN_TOUCH_SLIDE			2			// drag/slide/move
#define PAGE_IN_TOUCH_UP			3
#define PAGE_IN_WHEEL_FORWARD		4
#define PAGE_IN_WHEEL_BACK			5
#define PAGE_IN_WHEEL_LEFT			6
#define PAGE_IN_WHEEL_RIGHT			7
#define PAGE_IN_ROTARY_CHANGE		8


#define PAGE_OBJ_HOVER_HELD			1
#define PAGE_OBJ_HOVER_SLIDE		2
#define PAGE_OBJ_HOVER_RELEASE		3
#define PAGE_OBJ_HOVER_NONE			4


/*
pageSendMessage(pageStruct, PAGE_CTL_STARTUP, frame->width, frame->height, NULL);
pageSendMessage(pageStruct, PAGE_CTL_INITIALIZE, frame->width, frame->height, frame);
pageSendMessage(pageStruct, PAGE_CTL_OPEN, time, 0, NULL);
pageSendMessage(pageStruct, PAGE_CTL_RENDER, time, renderCt, frame);
pageSendMessage(pageStruct, PAGE_CTL_CLOSE, 0, 0, NULL);
pageSendMessage(pageStruct, PAGE_CTL_INPUT, msg, flags, pos);		// msg = touch/press down, drag/slide and up/release
pageSendMessage(pageStruct, PAGE_CTL_SHUTDOWN, 0, 0, NULL);

::pageStruct created at registration and zero'd
::opaquePtr will be NULL until it has actively been set
failure on either of INITALIZE, OPEN or RENDER will cause a SHUTDOWN disabling any further access to page

*/

typedef int (*TPageCallback_t) (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

typedef struct{
	int id;
}TPAG2EHISTORY;

typedef struct{
	int id;					// page id
	int updateRate;			// desired render rate per second
	char *title;			// a page text label
	int zDepthId;

	TPageCallback_t callback;
	void *opaquePtr;		// user data passed with every callback, usually containing page's data struct
	void *pageStruct;		// per page data structure
	
	int initState[PAGE_CTL_TOTAL];		// delivery status of the various PAGE_CTL_xxx events (0:not sent, 1:sent)
	
	unsigned int isEnabled:1;			// can accept events, set after PAGE_CTL_INITIALIZE
	unsigned int inputEnabled:1;		// page wants input events. this is outside of the Common Control input pipeline
	unsigned int inputWantDrag:1;		// page wants to know about input drag/slide events
	unsigned int isOpaque:1;			// is a base layer (used only to decide which copy method best suits)
	unsigned int isConcurrent:1;		// always rendered, but may not always be top most
	unsigned int readyToRender:1;		// states that page is inbetween an _MSG_OPEN and _MSG_CLOSE state and available for or actively being rendered
	unsigned int padding:26;

	TSTACK *stack;			// stack records how we got here and so how to return to where we were
}TPAGE2;

typedef struct{
	TPAGE2 **page;
	int tPages;
	
	int basePageId;			// fallback page and page displayed upon startup. usually the video rendering page
	int zDepthId;			// 
	
	TVLCPLAYER *vp;
}TPAGES2;


typedef struct{
	TPAGE2 *page;
	TPAGES2 *pages;
	TVLCPLAYER *vp;
}TPAGE2COM;


typedef struct{
	TPAGE2COM *com;
}TPAGE2COMOBJ;


TPAGES2 *pages2New (TVLCPLAYER *vp, const int basePageId);
void pages2Delete (TPAGES2 *pages);

int page2Register (TPAGES2 *pages, const char *title, const int id, const int updateRate, TPageCallback_t callback, const size_t dataStructSize);

int page2Input (TPAGES2 *pages, const int inType, void *ptr, const unsigned int flags);

int page2Render (TPAGES2 *pages, TFRAME *frame, const int id);
void *page2PageStructGet (TPAGES2 *pages, const int id);

int page2Enable (TPAGES2 *pages, const int id);
void page2Disable (TPAGES2 *pages, const int id);

//  has the page been initiated or not (opened at least once). set once and can not be unset
int page2IsInitialized (TPAGES2 *pages, const int id);


int page2SetPrevious (void *pageStruct);

void pageUpdate (void *pageStruct);

// enable page rendering
int page2Set (TPAGES2 *pages, const int id, const int setStack);
// disable page rendering
void page2RenderDisable (TPAGES2 *pages, const int id);
// get page renderable status (enabled/disabled)
unsigned int page2RenderGetState (TPAGES2 *pages, const int id);	// is page being rendered or not

// get top most page
int pageRenderGetTop (TPAGES2 *pages);

// get top most page which accepts input
int pageInputGetTop (TPAGES2 *pages);

unsigned int page2InputGetState (TPAGES2 *pages, const int id);
void page2InputEnable (TPAGES2 *pages, const int id);
void page2InputDisable (TPAGES2 *pages, const int id);

TPAGE2 *pageRenderGetTopMostPage (TPAGES2 *pages);

int page2InputDragEnable (TPAGES2 *pages, const int id);
int page2InputDragDisable (TPAGES2 *pages, const int id);
int page2InputDragGetState (TPAGES2 *pages, const int id);


// send message to every initialized/enabled page (anyone that can listen)
int pageDispatchMessage (TPAGES2 *pages, const int msg, const int64_t dataInt1, const int64_t dataInt2, void *dataPtr);

// send message to specified page _only_, and only if its initialized
int pageSendMessage (TPAGES2 *pages, const int pageId, const int msg, const int64_t dataInt1, const int64_t dataInt2, void *dataPtr);



#define pageGetPtr(a,b)				page2PageStructGet((a)->pages,(b))
#define pageDisable(a,b)			page2RenderDisable((a)->pages,(b))
#define pageGet(a)					pageInputGetTop((a)->pages)
#define pageSet(a,b)				page2Set((a)->pages,(b),1)
#define pageIsDisplayed(a,b)		page2RenderGetState((a)->pages,(b))
#define pageGetSurface(x)			(((TPAGE2COMOBJ*)(x))->com->vp->ml->front)
#define pageGetSurfaceWidth(x)		(((TPAGE2COMOBJ*)(x))->com->vp->ml->front->width)
#define pageGetSurfaceHeight(x)		(((TPAGE2COMOBJ*)(x))->com->vp->ml->front->height)



#endif


