
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





void tetrisPrerenderSetup (TVLCPLAYER *vp)
{
	const int colour = COL_PURPLE_GLOW;	// 24bit only
	const int alpha = 0.90 * 1000.0;	// alpha range of 0 to 1000
	const int radius = 4;				// 0 - 255
	const int blur = LTR_BLUR5;
	
	lRenderEffectReset(vp->ml->hw, TETRIS_STATS_FONT, blur);	// reset previous blur state
	lSetRenderEffect(vp->ml->hw, blur);
	lSetFilterAttribute(vp->ml->hw, blur, LTRA_BLUR_COLOUR, colour);
	lSetFilterAttribute(vp->ml->hw, blur, LTRA_BLUR_RADIUS, radius);
	lSetFilterAttribute(vp->ml->hw, blur, LTRA_BLUR_SETTOP, 1);
	lSetFilterAttribute(vp->ml->hw, blur, LTRA_BLUR_X, 0);
	lSetFilterAttribute(vp->ml->hw, blur, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(vp->ml->hw, blur, LTRA_BLUR_ALPHA, alpha);
	
	// enable the keyboard hook
	PostMessage(vp->gui.hMsgWin, WM_HOTKEY, 0, vp->gui.hotkeys.console<<16);
}

int tetrisInputProc (TVLCPLAYER *vp, TTETRIS *tetris, int key)
{
	tetrisGameInput(vp, tetris, key);
	return 1;
}

static inline int page_tetrisRender (TTETRIS *tet, TVLCPLAYER *vp, TFRAME *frame)
{
	tetrisGameDraw(tet, frame, tet->x, tet->y);
	return 1;
}

static inline int page_tetrisInput (TTETRIS *tet, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_TOUCH_DOWN:
		if (pageGet(vp) == PAGE_TETRIS)
			page2SetPrevious(tet);
		//else if (pageGetSec(vp) == PAGE_TETRIS)
		//	pageSetSec(vp, -1);
		break;
	  case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  	break;
	}
		
	return 1;
}

static inline int page_tetrisStartup (TTETRIS *tet, TVLCPLAYER *vp, const int width, const int height)
{
	tet->blockWidth = height*0.05;
	tet->blockHeight = tet->blockWidth/1.2727;
	tet->x = (width - (tet->blockWidth*BOARD_TILEMAP_WIDTH))/2;
	tet->y = (height - (tet->blockHeight*BOARD_TILEMAP_HEIGHT))/2;
	return tetrisGameInit(tet, getFrontBuffer(vp));
}

static inline int page_tetrisInitalize (TTETRIS *tet, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_TETRIS);
	return 1;
}

static inline int page_tetrisShutdown (TTETRIS *tet, TVLCPLAYER *vp)
{
	tetrisGameClose(tet);
	return 1;
}

int page_tetrisCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TTETRIS *tet = (TTETRIS*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_tetrisCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_tetrisRender(tet, tet->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		tetrisPrerenderSetup(tet->com->vp);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_tetrisInput(tet, tet->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_tetrisStartup(tet, tet->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_tetrisInitalize(tet, tet->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_tetrisShutdown(tet, tet->com->vp);
		
	}
	
	return 1;
}

