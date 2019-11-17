
// libmylcd - http://mylcd.sourceforge.net/
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
//
//	You should have received a copy of the GNU Library General Public
//	License along with this library; if not, write to the Free
//	Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.



#ifndef _SBUI_H_
#define _SBUI_H_




int isSBUIEnabled (TVLCPLAYER *vp);


int sbuiGestureCBEnable (TVLCPLAYER *vp);
void sbuiGestureCBDisable (TVLCPLAYER *vp);

int sbuiDKCBEnable (TVLCPLAYER *vp);
void sbuiDKCBDisable (TVLCPLAYER *vp);
//int sbuiDKCB (const int dk, const int state, void *ptr);


int sbuiDKSetImages (TVLCPLAYER *vp);

int sbuiSetDKImage (TVLCPLAYER *vp, const int key, TFRAME *image);
int sbuiSetDKImageEx (TVLCPLAYER *vp, TFRAME *image, const int desX, const int desY, const int x1, const int y1, const int x2, const int y2);
int sbuiSetDKImageFile (TVLCPLAYER *vp, const int key, wchar_t *filepath);
int sbuiSetDKImageArtId (TVLCPLAYER *vp, const int key, const int id);
int sbuiSetDKImageChar (TVLCPLAYER *vp, const int dk, const int font, const unsigned int colour, const char Char);

int sbuiResync (TVLCPLAYER *vp, lDISPLAY did);
void sbuiSetApplState (const int state);
int sbuiSimulateDk (const int dk, void *ptr);
unsigned int sbuiStartImageThread (TVLCPLAYER *vp, const int threadFlags);
void sbuiDKStateChange ();

lDISPLAY sbuiGetLibmylcdDID (THWD *hw);

void sbuiCfgSetPadControl (TVLCPLAYER *vp, const int mode);
 
void timer_sbuiConnected (TVLCPLAYER *vp);
void timer_sbuiDisconnected (TVLCPLAYER *vp);
 
void sbuiWoken (TVLCPLAYER *vp);


#endif


