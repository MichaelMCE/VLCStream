
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



#ifndef _PLM_H_
#define _PLM_H_



#define SWIPE_UP		0
#define SWIPE_DOWN		1
#define SWIPE_SLIDE		2




typedef struct {
	TPAGE2COM *com;
	
	TSHELF *shelf;
	TSLIDER *slider;
	TTOUCHSWIPE drag;
	int reset;
	int resetTrack;
	int from;
	int to;
	
	TBTNPANEL albpan;
	TLABEL *labPanClosed;	// could have made this a button instead..
	TCCBUTTONS *btns;
	
	TLABEL *title;
	int noartId;

	struct{
		double sigma;
		double rho;
		double expMult;
		double rate;
		double spacing;
		
		double artOpacityMult;
		double artScaleMult;
		
		double sliderX;
		double sliderY;
		double sliderW;
		double titleY;
		double textVSpace;
	}cfg;
}TSPL;


int page_plyPlmCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);


void invalidateShelf (TVLCPLAYER *vp, TSPL *spl, int start);
void resetShelfPosition (TVLCPLAYER *vp, TSPL *spl, int track);

void swipeReset (TTOUCHSWIPE *swipe);
void bringAlbumToFocus (TSPL *spl, const int idx);
void plyPlmRefresh (TVLCPLAYER *vp);


void drawAlbumPositionText (TVLCPLAYER *vp, TFRAME *frame, double y, const double alb, const int mx, const int my);

int renderShelfItemCB (void *ptr, const int idx, TFRAME *src, TFRAME *des, const int x, const int y, const int w, const int h, double modifier);


void freeImageCB (void *ptr, const int idx, const int artId);


#endif


