
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



#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

typedef struct{
	TPAGE2COM *com;
	
	int stub;
}TTEMPLATE;




// new page template
#if 0


static inline int page_Render (void *pageStruct, TVLCPLAYER *vp, TFRAME *frame)
{
	return 1;
}

static inline int page_RenderInit (void *pageStruct, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	return 1;
}

static inline int page_RenderBegin (void *pageStruct, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	return 1;
}

static inline int page_RenderEnd (void *pageStruct, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, void *opaquePtr)
{
	return 1;
}

static inline int page_Startup (void *pageStruct, TVLCPLAYER *vp, const int width, const int height)
{
	return 1;
}

static inline int page_Initalize (void *pageStruct, TVLCPLAYER *vp, const int width, const int height)
{
	return 1;
}

static inline int page_Shutdown (void *pageStruct, TVLCPLAYER *vp)
{
	return 1;
}

static inline int page_Input (void *pageStruct, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_TOUCH_DOWN:
	  case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  case PAGE_IN_WHEEL_FORWARD:
	  case PAGE_IN_WHEEL_BACK:
	  case PAGE_IN_WHEEL_LEFT:
	  case PAGE_IN_WHEEL_RIGHT:
	}
	return 1;
}

int page_Callback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TPAGE2COMOBJ *page = (TPAGE2COMOBJ*)pageStruct;
	
	// printf("# page_Callback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_Render(pageStruct, page->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_Input(pageStruct, page->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_START){
		return page_RenderBegin(pageStruct, page->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		return page_RenderEnd(pageStruct, page->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_RenderInit(pageStruct, page->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_Startup(pageStruct, page->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_Initalize(pageStruct, page->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_Shutdown(pageStruct, page->com->vp);
		
	}
	
	return 1;
}

#endif



#endif


