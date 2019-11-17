
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


#ifndef _LIBMYLCD_H_
#define _LIBMYLCD_H_



typedef struct{
	char *name;
	TRECT area;
	int width;
	int height;		// actual w,h & bpp of destination display
	int bpp;
	int did;		// libmylcd activated display handle
}TDISPLAY;

#define DISPLAYMAX	(4)

typedef struct{
	THWD *hw;
	TDISPLAY *display[DISPLAYMAX];
	
	TFRAME *front;	// primary render surface. read/write from render thread, read only from any other thread
	int width;
	int height;		// width, height and bpp of rendering surface
	int bpp;		// (need not be same as display surface)
	int virtualDisplayId;
	
	unsigned int enableTouchInput:1;
	unsigned int enableBrightness:1;
	unsigned int enableVirtualDisplay:1;
	unsigned int killRzDKManagerOnConnectFailRetry:1;
	unsigned int fill:28;
}TMYLCD;

int libmylcd_DisplayStart (THWD *hw, TDISPLAY *disp);
TDISPLAY *libmylcd_DisplayCfg (const char *name, const int width, const int height, const int bpp);
void libmylcd_DisplayFree (TDISPLAY *disp);
TMYLCD *libmylcd_Init (const int width, const int height, const int bpp);
void libmylcd_Close (TMYLCD *ml);
int libmylcd_FlushFonts (THWD *hw);
int libmylcd_StartDisplay (TMYLCD *ml, const char *name, const int width, const int height, const int idx);
int libmylcd_Render (TFRAME *frame);


MYLCD_EXPORT int icoSetDefaultHeight (THWD *hw, const int height);


#endif
