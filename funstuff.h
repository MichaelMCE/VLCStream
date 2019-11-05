
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



#ifndef _FUNSTUFF_H_
#define _FUNSTUFF_H_



typedef struct{
	char *name;			// sheet/page name
	TASCIILINE *file;
	
	int total;			// line count
	int last;			// previously accessed line 
}TREPLYSHEET;


typedef struct{
	TREPLYSHEET **sheet;	// page with lines of text
	int total;
}TCMDREPLY;




TCMDREPLY *sheetsNew (const int total);
void sheetsFree (TCMDREPLY *sheets);

int sheetAdd (TCMDREPLY *sheets, const char *name, const wchar_t *path);
char *sheetGetLine (TCMDREPLY *sheets, const char *name, const int line);
char *sheetGetLineRand (TCMDREPLY *sheets, const char *name);


#endif


