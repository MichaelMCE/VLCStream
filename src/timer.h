
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



#ifndef _TIMER_H_
#define _TIMER_H_



// defines a single shot timer
typedef struct{
	int state;			// ready to fire
	//uint64_t time;		// delta time to fire in ms
	double time;
	void (*func) (TVLCPLAYER *vp);
	void *ptr;
}TTIMER;



int timerInit (TVLCPLAYER *vp, const int id, void (*func) (TVLCPLAYER *), void *ptr);
void timerReset (TVLCPLAYER *vp, const int id);
void timerSet (TVLCPLAYER *vp, const int id, const double ms);
void timerCheckAndFire (TVLCPLAYER *vp, const double t0);


#define timerClear timerReset

/*
void timerUnlock (TVLCPLAYER *vp);
int timerLock (TVLCPLAYER *vp);*/

#endif


