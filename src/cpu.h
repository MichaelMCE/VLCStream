
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


#ifndef _CPU_H_
#define _CPU_H_

typedef struct {
	unsigned int weeks;
	unsigned int days;
	unsigned int hours;
	unsigned int minutes;
	unsigned int seconds;
}date64_t;


int cpuHasMMX ();
int cpuHas3DNOW ();
int cpuHasMMXEXT ();
int cpuHasSSE ();
int cpuHasSSE2 ();
int cpuHasSSE3 ();
int cpuHasSSSE3 ();
int cpuHasSSE4_1 ();
int cpuHasSSE4_2 ();
int cpuHasSSE4A ();

int cpuGetProcessorCount ();
double cpuGetProcessorUsage (TVLCPLAYER *vp);
double cpuGetCoreUsage (TVLCPLAYER *vp, const int core);

void cpuGetUpTime (date64_t *ut);

char *cpu_getCapabilityString (char *strbuffer, const int blen);



#endif

