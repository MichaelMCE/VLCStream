
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



#ifndef _BASS_H_
#define _BASS_H_

#if (ENABLE_BASS || !RELEASEBUILD)

#include "bass/bass.h"
#include "bass/basswasapi.h"


#define BASS_VISUAL_FFT			0
#define BASS_VISUAL_FFTBAR		1
#define BASS_VISUAL_TOTAL		2


typedef struct{
	int *buffer;
	size_t bufferLen;
	int width;
	int height;
}TBASSVISUAL;

typedef struct{
	int inEnabled;
	int outEnabled;
	float volume;
	
	uintptr_t hThreadInput;
	unsigned int threadId;
	
	uintptr_t hThreadAudioLevel;
	unsigned int threadIdAudioLevel;
	HANDLE changeEventAudioLevel;
		
	uintptr_t hThreadVolChange;
	unsigned int threadVolChangeId;
	HANDLE volumeChangeEvent;

	
	int visuals;
	int vwidth;
	int vheight;
	
	HRECORD chan;
	TBASSVISUAL vis[BASS_VISUAL_TOTAL];
	
	int *visData[8];
	
	TVLCPLAYER *vp;
}TBASSLIB;

int bass_start (TBASSLIB *bass, const int visuals, const int vwidth, const int vheight, TVLCPLAYER *vp);
void bass_close (TBASSLIB *bass);


float bass_volumeGet (TBASSLIB *bass);
void bass_volumeSet (TBASSLIB *bass, const float vol);

int bass_muteGet (TBASSLIB *bass);
void bass_muteSet (TBASSLIB *bass, const int mute);


void bass_render (TBASSLIB *bass, TFRAME *frame, const int x, const int y);

#endif
#endif