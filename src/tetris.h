
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



#ifndef _TETRIS_H_
#define _TETRIS_H_

#include "tetris/stc.h"
#include "tetris/platform.h"

typedef struct {
	TPAGE2COM *com;
	
	StcGame *game;
	int x;
	int y;
	int blockWidth;
	int blockHeight;
}TTETRIS;


int page_tetrisCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);



int tetrisInputProc (TVLCPLAYER *vp, TTETRIS *tet, const int key);




int tetrisGameInput (TVLCPLAYER *vp, TTETRIS *tetris, const int key); 
void tetrisGameClose (TTETRIS *tetris);
void tetrisGameDraw (TTETRIS *tetris, TFRAME *frame, const int x, const int y);
int tetrisGameInit (TTETRIS *tetris, TFRAME *frame);

void tetrisPrerenderSetup (TVLCPLAYER *vp);

#endif


