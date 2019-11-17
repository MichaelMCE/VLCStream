
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



#ifndef _VCLOCK_H_
#define _VCLOCK_H_





enum _clockTypes
{
	CLOCK_ANALOGUE,
	CLOCK_DIGITAL,
	CLOCK_BOXDIGITAL,
	CLOCK_BUTTERFLY,
	CLOCK_POLAR,
	CLOCK_PREDATOR,
	
	CLOCK_TOTAL
};

enum _clkdig {
	CLK_IMG_DIG_0,
	CLK_IMG_DIG_1,
	CLK_IMG_DIG_2,
	CLK_IMG_DIG_3,
	CLK_IMG_DIG_4,
	CLK_IMG_DIG_5,
	CLK_IMG_DIG_6,
	CLK_IMG_DIG_7,
	CLK_IMG_DIG_8,
	CLK_IMG_DIG_9,
	CLK_IMG_DIG_COLON,
	
	digitalImageTotal
};

enum _clkpred {
	CLK_IMG_PRED_BASE45,
	CLK_IMG_PRED_BASE90,
	CLK_IMG_PRED_BASE135,
	CLK_IMG_PRED_TOP45,
	CLK_IMG_PRED_TOP90,
	CLK_IMG_PRED_TOP135,
	CLK_IMG_PRED_BASE0,
	CLK_IMG_PRED_BASE18,		/* 72 */
	CLK_IMG_PRED_BASE243,
	CLK_IMG_PRED_TOP0,
	CLK_IMG_PRED_TOP18,
	CLK_IMG_PRED_TOP243,
	CLK_IMG_PRED_PREDLEFT,
	CLK_IMG_PRED_SKULL,
	CLK_IMG_PRED_PREDRIGHT,

	predatorImageTotal
};

enum _clka {
	CLK_IMG_FACE,
	CLK_IMG_HOUR,
	CLK_IMG_MINUTE,
	CLK_IMG_SECOND,
	CLK_IMG_CAP,
	
	analogueImageTotal
};

enum _clkbf {
	CLK_IMG_FACE_BF,
	CLK_IMG_HOUR_BF,
	CLK_IMG_MINUTE_BF,
	CLK_IMG_SECOND_BF,
	CLK_IMG_CAP_BF,
	
	butterflyImageTotal
};


typedef struct{
	TPAGE2COM *com;
	int displayType;		// the clock we're displaying
	T2POINT pos;
	int t0;
	
	struct {
		int imageIds[digitalImageTotal];
		TFRAME *images[digitalImageTotal];
		int charOverlap;
		
		TCLK_SBDATE_CFG date;
	}digital;
	
	struct {
		int imageIds[analogueImageTotal];
		
		TFRAME *face;
		TFRAME *hr;
		TFRAME *min;
		TFRAME *sec;
		TFRAME *cap;
		
		TCLK_SBDATE_CFG date;
	}analogue;
	
	struct {
		int imageIds[butterflyImageTotal];
		
		TFRAME *face;
		TFRAME *hr;
		TFRAME *min;
		TFRAME *sec;
		TFRAME *cap;
		int cx;
		int cy;
		
		TCLK_SBDATE_CFG date;
	}butterfly;

	struct {
		TCLK_SBDATE_CFG date;
	}boxdigital;
		
	struct {
		int imageIds[predatorImageTotal];
		TFRAME *images[predatorImageTotal];
		
		TCLK_SBDATE_CFG date;
	}predator;

	struct {
		int bandColours[2];
		int baseColour;
		
		TCLK_SBDATE_CFG date;
	}polar;
	
	struct {
		int renderState;
		int timerState;
	}sbdk;
}TCLK;



struct tm *getTimeReal (double *nanos);

int page_clkCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

const char *clkDayToName (const int day);
const char *clockGetMonthShortname (const int month);
int clkDaysInMonth (const int year, const int month);

#endif

