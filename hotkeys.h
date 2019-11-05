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


#ifndef _HOTKEYS_H_
#define _HOTKEYS_H_



//     modifierA,modifierB,key,x,y,image_path
// eg; CTRL,ALT,A,name,200,100,hotkeys/image1.png

typedef struct{
	char *name;
	int modifierA;
	int modifierB;
	int key;
	char *imagePath;
	int id;
}THOTKEY;

typedef struct{
	TPAGE2COM *com;
	
	THOTKEY **keys;
	int totalKeys;
	int validKeys;
	
	TPANEL *panel;
	T2POINT itemOffset;
	
	int showNames;
	char vlcTitle[MAX_PATH_UTF8+1];		// vlc window title
}TGLOBALHOTKEYS;



int page_ghkCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);


int ghkPanelBuild (TPANEL *panel, THOTKEY **keys, const int total, const int showNames);

THOTKEY *ghkAllocKey ();
void ghkFreeKeyList (THOTKEY **keys, int total);
THOTKEY **ghkAllocKeyList (const int total);

void ghkSendHotkey (const int modA, const int modB, const char key);
int ghkIsVlcRunning ();
char *ghkGetRunningVLCWindowTitle (HWND hWnd, char *buffer, const int blen);

#endif

