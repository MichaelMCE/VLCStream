
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



#ifndef _PICQUEUE_H_
#define _PICQUEUE_H_


#define PICQUEUE_MAX	32

typedef struct{
	int status;
	int imgBtmId;			// base image, if any
	int imgTopId;			// rendered on top of base
	uint64_t timeEnd;		// display until this time is reached
	uint64_t timeAdded;
	T4POINT pos;
}TPICTURE;

typedef struct{
	int total;
	int imageSpaceCol;
	int imageSpaceRow;
	TPICTURE queue[PICQUEUE_MAX];
	TMLOCK *hLock;
	TIMAGEMANAGER *im;
	TASYNCIMGLOAD *imgLoader;
}TPICQUEUE;



int picQueueAdd (TPICQUEUE *pq, const wchar_t *imgBtm, const wchar_t *imgTop, const uint64_t timeEnd);


int picQueueGetTotal (TPICQUEUE *pq);
int picQueueRender (TPICQUEUE *pq, TFRAME *frame, const uint64_t time0, const int rowX, const int colY);


TPICQUEUE *picQueueNew (TIMAGEMANAGER *im, const int spaceCol, const int spaceRow);
void picQueueDelete (TPICQUEUE *pq);





#endif

