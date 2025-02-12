
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



#ifndef _INPUT_H_
#define _INPUT_H_


#define INPUT_FLAG_DOWN		0
#define INPUT_FLAG_SLIDE	1
#define INPUT_FLAG_UP		3


typedef struct{
	int isHooked;				// is mouse hooked
	int slideHoverEnabled;		// hover detection without being hooked (mouse/pad)
	int draw;
	int x;				// Windows mouse co-ord X
	int y;				// Windows mouse co-ord Y
	int dx;				// local cursor co-ord X
	int dy;				// local cursor co-ord Y
	POINT pt;			// Windows cursor position when hooked
	int LBState;
	int MBState;
	int RBState;
	
	TPOINT dragRect0;		// from
	TPOINT dragRect1;		// to
	TPOINT dragRectDelta;	// size
	int dragRectIsEnabled;
	
	struct{
		int enableMMoveRenderSig;
		double renderMMoveTargetFPS;
	}virt;
}TGUIINPUT;


typedef struct{
	int state;
	double t0;
	double dt;
	int sx;
	int sy;
	int ex;
	int ey;
	int dx;
	int dy;
	int dragMinH;
	int dragMinV;
	
	uint64_t u64value;
	int i32value;
	double dvalue;
	
	double velocityFactor;
	double decayRate;
	double decayFactor;
	double decayAdjust;
	double adjust;
	
	double velocity;
}TTOUCHSWIPE;





void touchIn (TTOUCHCOORD *pos, int flags, void *ptr);
int touchDispatchFilter (TTOUCHCOORD *pos, const int flags, TVLCPLAYER *vp);

void touchDispatcherStop (TVLCPLAYER *vp);
void touchDispatcherStart (TVLCPLAYER *vp, const void *fn, const void *ptr);

void inputGetCursorPosition (TVLCPLAYER *vp, int *x, int *y);

unsigned int __stdcall inputDispatchThread (void *ptr);

#endif
