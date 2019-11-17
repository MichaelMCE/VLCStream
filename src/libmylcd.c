
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


#include "common.h"

#if 0
#undef mylog
#define mylog printf
#endif

#define BENCH_RENDER 	0



#if BENCH_RENDER

static uint64_t freq;
static uint64_t tStart;
static double resolution;

static inline void setResD ()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&tStart);
	QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
	resolution = 1.0 / (double)freq;
}

static inline double getTimeD ()
{
	uint64_t t1 = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&t1);
	return ((double)((uint64_t)(t1 - tStart) * resolution) * 1000.0);
}
#endif

int libmylcd_Render (TFRAME *frame)
{

#if BENCH_RENDER
	const double t0 = getTimeD();
#endif
	
//	lUpdate(frame->hw, frame->pixels, frame->frameSize);
	lRefreshAsync(frame, 0);
//	lRefresh(frame);
//	lRefreshArea(frame, 0, 0, 799/*frame->width-1*/, /*frame->height-1*/479);

#if BENCH_RENDER
	const double t1 = getTimeD();
	printf("render time: %.4f\n", t1-t0);
#endif
	return 1;
}

TMYLCD *libmylcd_Init (const int width, const int height, const int bpp)
{

	TMYLCD *ml = calloc(1, sizeof(TMYLCD));
	if (!ml) return NULL;
	
	mylog("initLibrary: starting libmylcd...\n");
	/* open libmylcd using default font and cmap directory location */
    if (!(ml->hw=lOpen(FONTD(""), FONTD("")))){
    	mylog("initLibrary: lOpen() failed\n");
    	return NULL;
    }

	mylog("initLibrary: requesting global display surface\n");
	// this is our primary display surface.
    if (!(ml->front=lNewFrame(ml->hw, width, height, bpp))){
    	lClose(ml->hw);
    	mylog("initLibrary: lNewFrame() failed\n");
    	return NULL;
    }else{
		ml->width = ml->front->width;
		ml->height = ml->front->height;
		ml->bpp = ml->front->bpp;

		lSetPixelWriteMode(ml->front, LSP_SET);
		lSetBackgroundColour(ml->hw, lGetRGBMask(ml->front, LMASK_BLACK));
		lSetForegroundColour(ml->hw, lGetRGBMask(ml->front, LMASK_WHITE));
		lSetFontZeroWidthSpacing(ml->hw, MFONT, -1);
		lSetFontCharacterSpacing(ml->hw, LFTW_UNICODE72, lGetFontCharacterSpacing(ml->hw,LFTW_UNICODE72)+4);
		lSetFontCharacterSpacing(ml->hw, LFTW_UNICODE36, lGetFontCharacterSpacing(ml->hw,LFTW_UNICODE36)+4);
		lSetFontCharacterSpacing(ml->hw, LFTW_RACER96, lGetFontCharacterSpacing(ml->hw,LFTW_RACER96)+16);
		lSetFontCharacterSpacing(ml->hw, LFTW_B34, lGetFontCharacterSpacing(ml->hw,LFTW_B34)+1);
		icoSetDefaultHeight(ml->hw, 256);
			
		
			
		mylog("initLibrary: libmylcd started successfully\n");
		
#if BENCH_RENDER
		setResD();
#endif
    	return ml;
    }
}

void libmylcd_Close (TMYLCD *ml)
{
	mylog("cleanup: closing libmylcd\n");

	for (int i = 0; i < DISPLAYMAX; i++){
		if (ml->display[i]){
			if (ml->display[i]->did)
				lCloseDevice(ml->hw, ml->display[i]->did);
			libmylcd_DisplayFree(ml->display[i]);
		}
	}
	if (ml->front){
		lDeleteFrame(ml->front);
		ml->front = NULL;
	}
	if (ml->hw){
		lClose(ml->hw);
		ml->hw = NULL;
	}
	free(ml);

	//CoUninitialize();

	mylog("cleanup: libmylcd shutdown\n");
}

void libmylcd_DisplayFree (TDISPLAY *disp)
{
	my_free(disp->name);
	my_free(disp);
}

TDISPLAY *libmylcd_DisplayCfg (const char *name, const int width, const int height, const int bpp)
{
	TDISPLAY *disp = my_calloc(1, sizeof(TDISPLAY));
	if (!disp) return NULL;
	disp->name = my_strdup(name);
	disp->width = width;
	disp->height = height;
	disp->bpp = bpp;
	disp->area.left = 0;
	disp->area.top = 0;
	disp->area.right = width-1;
	disp->area.btm = height-1;
	
	if (!strcmp(name, "switchbladefio")){
		disp->area.right = (SBUI_PAD_WIDTH-1);
		disp->area.btm = (SBUI_PAD_HEIGHT-1);
	}
	return disp;
}

int libmylcd_DisplayStart (THWD *hw, TDISPLAY *disp)
{
	mylog("starting device..\n");

	//CoInitializeEx(NULL, COINIT_MULTITHREADED|COINIT_SPEED_OVER_MEMORY);

	disp->did = (int)lSelectDevice(hw, disp->name, "NULL", disp->width, disp->height, disp->bpp, 0, &disp->area);
	if (!disp->did)	// if device 1 failed, try for a second device of the same type
		disp->did = (int)lSelectDevice(hw, disp->name, "NULL", disp->width, disp->height, disp->bpp, 1, &disp->area);
	
	if (disp->did){
		mylog("device '%s' started\n", disp->name);
	}else{
		mylog("device '%s' not found or unavailable\n", disp->name);
	}
	return disp->did;
}

int libmylcd_FlushFonts (THWD *hw)
{
	int ret = 0;
	int flushed = 0;


#if 0

	flushed = lFlushFont(hw, -1);
	if (flushed) ret += flushed-1;
	
#else

	flushed = lFlushFont(hw, LFTW_UNICODE36);
	if (flushed) ret += flushed-1;
	
	flushed = lFlushFont(hw, LFTW_UNICODE72);
	if (flushed) ret += flushed-1;
	
	flushed = lFlushFont(hw, LFTW_B24);
	if (flushed) ret += flushed-1;

	flushed = lFlushFont(hw, LFTW_B26);
	if (flushed) ret += flushed-1;
		
	flushed = lFlushFont(hw, LFTW_B28);
	if (flushed) ret += flushed-1;
	
	flushed = lFlushFont(hw, LFTW_B34);
	if (flushed) ret += flushed-1;
	
	flushed = lFlushFont(hw, LFTW_RACER96);
	if (flushed) ret += flushed-1;
	
	flushed = lFlushFont(hw, LFTW_NAMCO91);
	if (flushed) ret += flushed-1;
	
	flushed = lFlushFont(hw, LFTW_198FIVE162);
	if (flushed) ret += flushed-1;
	
	flushed = lFlushFont(hw, LFTW_76LONDON150);
	if (flushed) ret += flushed-1;
	
	flushed = lFlushFont(hw, LFTW_76LONDON34);
	if (flushed) ret += flushed-1;
	
	flushed = lFlushFont(hw, LFTW_76LONDON38);
	if (flushed) ret += flushed-1;

#endif
	
	return ret; // total chars flushed
}

int libmylcd_StartDisplay (TMYLCD *ml, const char *name, const int width, const int height, const int idx)
{
	
	//printf("libmylcd_StartDisplay '%s' %i\n", name, idx);
	
	
	TDISPLAY *disp = libmylcd_DisplayCfg(name, width, height, SKINFILEBPP);
	if (disp){
		disp->did = libmylcd_DisplayStart(ml->hw, disp);
		if (disp->did){
			ml->display[idx] = disp;
			ml->width = width;
			ml->height = height;
			ml->enableTouchInput = 1;
			ml->enableVirtualDisplay = 0;
						
#if ENABLE_BRIGHTNESS
			ml->enableBrightness = 1;
#else
			ml->enableBrightness = 0;
#endif
			return disp->did;
		}else{
			ml->display[idx] = NULL;
		}
		libmylcd_DisplayFree(disp);
	}
	return 0;
}
/*
int libmylcd_StartDisplay2 (THWD *hw, const char *name, const int width, const int height)
{
	
	//printf("libmylcd_StartDisplay2 '%s'\n", name);
	
	
	TDISPLAY *disp = libmylcd_DisplayCfg(name, width, height, SKINFILEBPP);
	if (disp){
		disp->did = libmylcd_DisplayStart(hw, disp);
		if (disp->did)
			return disp->did;

		libmylcd_DisplayFree(disp);
	}
	return 0;
}
*/


