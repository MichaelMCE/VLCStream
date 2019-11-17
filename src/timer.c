
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

static int timerLock (TVLCPLAYER *vp)
{
	return getApplState(vp);
}

static void timerUnlock (TVLCPLAYER *vp)
{
}

int timerInit (TVLCPLAYER *vp, const int id, void (*func) (TVLCPLAYER *), void *ptr)
{
	if (id < TIMER_TOTAL){
		vp->timers.queue[id].func = func;
		vp->timers.queue[id].ptr = ptr;
		vp->timers.queue[id].state = 0;
		vp->timers.queue[id].time = 0;
		return 1;
	}
	return 0;
}

void timerReset (TVLCPLAYER *vp, const int id)
{
	vp->timers.queue[id].state = 0;
	vp->timers.queue[id].time = 0;
}

void timerSet (TVLCPLAYER *vp, const int id, const double ms)
{
	if (timerLock(vp)){
		vp->timers.queue[id].state = 1;
		vp->timers.queue[id].time = getTime(vp) + ms;
		timerUnlock(vp);
	}		
	
	if (ms < 0.1){
		//printf("timerSet %i %.2f\n", id, ms);
		SetEvent(vp->ctx.hEvent);
		renderSignalUpdate(vp);
	}
}

static void timerFire (TVLCPLAYER *vp, const int id)
{
	if (vp->timers.queue[id].state){
		vp->timers.queue[id].state = 0;
		if (vp->applState)
			vp->timers.queue[id].func(vp);
	}
}

//void timerCheckAndFire (TVLCPLAYER *vp, const unsigned int t0)
void timerCheckAndFire (TVLCPLAYER *vp, const double t0)
{
	for (int i = 0; i < TIMER_TOTAL; i++){
		if (vp->timers.queue[i].state > 0){
			if (t0 >= vp->timers.queue[i].time){
				//printf("firing timer %i [%i] %i\n", i, (int)(t0-vp->timers.queue[i].time), vp->timers.queue[i].state);
				timerFire(vp, i);
				//printf("firing timer complete\n");
			}
		}
	}
}
