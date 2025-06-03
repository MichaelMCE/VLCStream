
// libmylcd
// An LCD framebuffer library
// Michael McElligott
// okio@users.sourceforge.net

//  Copyright (c) 2005-2012  Michael McElligott
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


#ifndef _HIDDTOUCH_H_
#define _HIDDTOUCH_H_



typedef struct{
	int type;
	int params;
	int x;
	int y;
	int z;

	unsigned int ct;
	double time;
	double timePrev;
	double dt;
	unsigned int id;
}TTOUCHINPUT;



typedef int (*ptouchcb) (const TTOUCHINPUT *hidt, void *ptr);




#endif

