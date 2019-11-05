
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


#ifndef _CMDPARSER_H_
#define _CMDPARSER_H_

#define CMDPARSER_CMDIDENT		L'#'
#define CMDPARSER_NUMIDENT		L'#'
#define CMDPARSER_WILDIDENT		L'*'
#define CMDPARSER_COMMENT		L';'

#define CMDPARSER_SEARCHRESET	L'~'
#define CMDPARSER_SEARCHROOT	L'@'
#define CMDPARSER_SEARCHNEXT	L':'

#define CMDPARSER_SEARCHPLAY	L'>'

#define TIMRSKIP_FORWARD		1
#define TIMRSKIP_BACK			2


#define SEARCH_CURRENT			-3
#define SEARCH_CURRENT_PLAY		-4

#define SEARCH_ROOT				-5
#define SEARCH_ROOT_PLAY		-6

#define SEARCH_NEXT				-7
#define SEARCH_NEXT_PLAY		-8

#define SEARCH_RESET			-9
#define SEARCH_RESET_PLAY		-10


/*
'string'		search for string
>'string'		search for then play if found

~'string'		search from beginng (reset to beginning of current playlist)
~>'string'		search from beginng then play if found

@'string'		search from root
@>'string'		search from root then play if found

:'string'		search next
:>'string'		search next then play if found

*/


enum _bots{
	BOT_BOFH = 1,
	BOT_DUBYA,
	BOT_FACTS,
	BOT_MORBID
};



int editboxProcessString (TEDITBOX *input, wchar_t *txt, int ilen, void *ptr);
//void editboxDoCmdRegistration (TEDITBOX *input, void *vp);

void cmd_snapshot (wchar_t *var, int vlen, void *uptr, int unused1, int unused2);

// shouldn't be here
int artworkFlush (TVLCPLAYER *vp, TARTMANAGER *am);
void printAbout (TVLCPLAYER *vp);
void playlistsForceRefresh (TVLCPLAYER *vp, const int when);

void cmd_import (wchar_t *var, int vlen, void *uptr, int play, int unused2);

#endif

