
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



#ifndef _EQ_H_
#define _EQ_H_


//#if (LIBVLC_VERSION_MAJOR >= 2 && LIBVLC_VERSION_MINOR >= 1)

#define EQBAND_NAME_LEN			(12)
#define EQPRESET_NAME_LEN		(64)
#define EQBANDS_MAX				(16)			// profileMax?


typedef struct {
	int index;
	double frequency;
	char name[EQBAND_NAME_LEN+1];
	TSLIDER *slider;
	int ccId;
	unsigned int stringHash;
}TEQBAND;	// profile


typedef struct {
	TPAGE2COM *com;
	
	TVLCPLAYER *vp;
	TEQBAND	bands[EQBANDS_MAX];
	int tBands;
	int tPresets;
	char name[EQPRESET_NAME_LEN+1];
	
	libvlc_equalizer_t *eqObj;
	int touched;
	int preset;	// preset index. used when cycling through the various presets
	
	double t0;
	TCCBUTTONS *btns;
}TEQ;


int page_eqCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);



int eqPresetGetTotal (TEQ *eq);
int eqBuild (TEQ *eq, const int presetIdx);
int eqApply (TEQ *eq, TVLCCONFIG *vlc, const int forceSet);

int eqBandSet (TEQ *eq, const int band, const double amp);
double eqBandGet (TEQ *eq, const int band);

int eqApplyPreset (TEQ *eq, const int preset);
const char *eqGetProfileName (const int index);
int eqProfileGetTotal();
int eqBandGetTotal ();
double eqSliderToAmp (const double sliderValue);
int eqAmpToSlider (const double amp);



//#endif


#endif


