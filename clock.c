
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


#include <math.h>
#include "common.h"
#include <time.h>


#ifndef _TIMESPEC_DEFINED
struct timespec {
  time_t  tv_sec;   /* Seconds */
  long    tv_nsec;  /* Nanoseconds */
};
#endif

#ifndef clockid_t
typedef int clockid_t;
int __cdecl clock_gettime(clockid_t clock_id, struct timespec *tp);
#endif

static const char *monthName_upper[12]  = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
static const char *monthName_lower[12]  = {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"};
static const char *monthName_CapLow[12]  = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char *weekdayName_upper[8] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
static const char *weekdayName_lower[8] = {"mon", "tue", "wed", "thu", "fri", "sat", "sun", "mon"};
static const char *weekdayName_capLow[8] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat", "Sun"};
static const int daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};



const char *clockGetMonthShortname (const int month)
{
	if (month && month <= 12)
		return monthName_CapLow[month-1];
	else
		return  "---";
}


#if 0
static inline void drawCross (TFRAME *frame, const int cx, const int cy)
{
	lDrawLine(frame, cx, 0, cx, frame->height-1, 0xFF<<24 | COL_RED);
	lDrawLine(frame, 0, cy, frame->width-1, cy,  0xFF<<24 | COL_RED);
}
#endif


static inline void drawBand (TFRAME *frame, const double x, const double y, const int colBand, const int colMarker)
{
	//drawCross(frame, x, y);
	lEdgeFill(frame, x, y, colBand, colMarker);
}

const char *clkDayToName (const int day)
{
	return weekdayName_capLow[day&7];
}

static inline void rotateZ (const double angle, const double x, const double y, double *xr, double *yr)
{
	*xr = x*cos(angle) - y*sin(angle);
	*yr = x*sin(angle) + y*cos(angle);
}

static inline void drawMarker (TFRAME *frame, const int x1, const int y1, int x2, const int y2, const int colour)
{
	if (abs(x1-x2) < 1) x2++;		// prevent edge spillover at 0 degrees (12 O'Clock)
	//if (abs(y1-y2) < 1) y2++;
	lDrawLine(frame, x1, y1, x2, y2, colour);
}

static inline void drawImageCentered (TFRAME *img, TFRAME *des, const int xc, const int yc)
{
	drawImage(img, des, xc - (img->width/2), yc - (img->height/2), img->width-1, img->height-1);
}

struct tm *getTimeReal (double *nanos)
{
	if (nanos){
		struct timespec tp;
		clock_gettime(0, &tp);	// for nanoseconds only
		*nanos = tp.tv_nsec/1000000000.0;
	}

	const __time64_t t = _time64(0);
	//_localtime64_s (struct tm *_Tm, const __time64_t *_Time);
    return _localtime64(&t);
}

static inline int clkDaysInYear (const int year)
{
	if ((year%400 == 0) || (year%100 != 0 && (year%4 == 0)))
		return 366;
	else
		return 365;
}

int clkDaysInMonth (const int year, const int month)
{
	if (month == 1){		// Feb.
		if (clkDaysInYear(year+1900) == 365)
			return 28;
		else
			return 29;
	}else{
		return daysInMonth[month];
	}
}

static inline void clockRenderAnalogue (TVLCPLAYER *vp, TCLK *clk, TFRAME *frame, const int x, const int y, const int drawSec)
{
    double nanos;
	const struct tm *tdate = getTimeReal(&nanos);
	const double hr = tdate->tm_hour;
	const double min = tdate->tm_min;
	const double sec = tdate->tm_sec + nanos;

	const double s = 6.0 * sec;					// 6 = (360.0 / 60.0)
	const double m = fmod(6.0 * min, 360.0);	// 30 = 360.0 / 12.0
	const double h = 30.0 * (hr+((1.0/60.0)*min));

	TMETRICS metrics = {x, y-4, clk->analogue.face->width, clk->analogue.face->height};
	drawImage(clk->analogue.face, frame, x, y, clk->analogue.face->width-1, clk->analogue.face->height-1);

	if (drawSec)
		rotate(clk->analogue.sec, frame, &metrics, s, clk->analogue.sec->width);
	rotate(clk->analogue.hr, frame, &metrics, h, clk->analogue.hr->width);
	rotate(clk->analogue.min, frame, &metrics, m, clk->analogue.min->width);
	drawImage(clk->analogue.cap, frame, x+abs(clk->analogue.cap->width - clk->analogue.face->width)/2, y+abs(clk->analogue.cap->height-clk->analogue.face->height)/2 , clk->analogue.cap->width-1, clk->analogue.cap->height-1);

#if DRAWMISCDETAIL
	lDrawRectangle(frame, x, y, x+clk->analogue.face->width-1, y+clk->analogue.face->height-1, DRAWTOUCHRECTCOL);

	// locate cap
	int xc = x + (clk->analogue.face->width/2)-1;
	int yc = y + (clk->analogue.face->height/2)-1;
	lDrawCircleFilled(frame, xc, yc, 10, 0xFFFF0000);
	lDrawLine(frame, xc, yc, xc+50, yc+50, DRAWTOUCHRECTCOL);
	lDrawLine(frame, xc, yc, xc-50, yc-50, DRAWTOUCHRECTCOL);
	lDrawLine(frame, xc, yc, xc-50, yc+50, DRAWTOUCHRECTCOL);
	lDrawLine(frame, xc, yc, xc+50, yc-50, DRAWTOUCHRECTCOL);
#endif
}

static inline void clockRenderButterfly (TVLCPLAYER *vp, TCLK *clk, TFRAME *frame, const int x, const int y, const int drawSec)
{

	double nanos;
	const struct tm *tdate = getTimeReal(&nanos);
	const double hr = tdate->tm_hour;
	const double min = tdate->tm_min;
	const double sec = tdate->tm_sec + nanos;

	const double s = 6.0 * sec;					// 6 = (360.0 / 60.0)
	const double m = fmod(6.0 * min, 360.0);	// 30 = 360.0 / 12.0
	const double h = 30.0 * (hr+((1.0/60.0)*min));

	TMETRICS metrics = {x, y, clk->butterfly.face->width, clk->butterfly.face->height};
	drawImage(clk->butterfly.face, frame, x, y, clk->butterfly.face->width-1, clk->butterfly.face->height-1);

	metrics.x = -((clk->butterfly.face->width/2) - clk->butterfly.cx) + x;
	metrics.y = abs((clk->butterfly.face->height/2) - clk->butterfly.cy) + y;

	if (drawSec)
		rotate(clk->butterfly.sec, frame, &metrics, s, clk->butterfly.sec->width);
	rotate(clk->butterfly.hr, frame, &metrics, h, clk->butterfly.hr->width);
	rotate(clk->butterfly.min, frame, &metrics, m, clk->butterfly.min->width);
	drawImage(clk->butterfly.cap, frame, x+clk->butterfly.cx - (clk->butterfly.cap->width/2), y+clk->butterfly.cy - (clk->butterfly.cap->height/2), clk->butterfly.cap->width-1, clk->butterfly.cap->height-1);

#if DRAWMISCDETAIL
	lDrawRectangle(frame, x, y, x+clk->butterfly.face->width-1, y+clk->butterfly.face->height-1, DRAWTOUCHRECTCOL);

	// locate cap
	lDrawCircleFilled(frame, x+clk->butterfly.cx, y+clk->butterfly.cy, 10, 0xFFFF0000);
	lDrawLine(frame, x+clk->butterfly.cx, y+clk->butterfly.cy, x+clk->butterfly.cx+50, y+clk->butterfly.cy+50, 0xFF444444);
	lDrawLine(frame, x+clk->butterfly.cx, y+clk->butterfly.cy, x+clk->butterfly.cx-50, y+clk->butterfly.cy-50, 0xFF444444);
	lDrawLine(frame, x+clk->butterfly.cx, y+clk->butterfly.cy, x+clk->butterfly.cx-50, y+clk->butterfly.cy+50, 0xFF444444);
	lDrawLine(frame, x+clk->butterfly.cx, y+clk->butterfly.cy, x+clk->butterfly.cx+50, y+clk->butterfly.cy-50, 0xFF444444);
#endif
}

static inline void clockRenderDigitStringBoxDigital (TCLK *clk, TFRAME **digit, TFRAME *frame, int x, int y, const char *str)
{

	TLPRINTR rt;
	memset(&rt, 0, sizeof(TLPRINTR));

	//lSetForegroundColour(frame->hw, 255<<24|0xFF0000);
	unsigned int colour[2];
	if (getIdle(clk->com->vp)){
		colour[0] = 0xCFB000;		// outline
		colour[1] = COL_BLACK;		// foreground
	}else{
		colour[0] = COL_WHITE;
		colour[1] = COL_BLUE_SEA_TINT;
	}
		
	int flags = PF_MIDDLEJUSTIFY | PF_CLIPWRAP | PF_IGNOREFORMATTING;
#if DRAWMISCDETAIL
	flags |= PF_GLYPHBOUNDINGBOX | PF_TEXTBOUNDINGBOX;
#endif
	 
	int blurOp = LTR_BLUR5;
	lSetRenderEffect(frame->hw, blurOp);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, colour[0]);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_RADIUS, 11);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_SETTOP, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_ALPHA, 1000);
	
	rt.sx = x; rt.sy = y;
	lPrintEx(frame, &rt, CLK_BOXDIGITAL_TIME_FONT, flags, LPRT_CPY, str);

	blurOp = LTR_BLUR4;
	lSetRenderEffect(frame->hw, blurOp);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, colour[1]);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_RADIUS, 4);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_SETTOP, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_ALPHA, 900);

	rt.sx = x; rt.sy = y;
	lPrintEx(frame, &rt, CLK_BOXDIGITAL_TIME_FONT, flags, LPRT_CPY, str);

	lSetRenderEffect(frame->hw, LTR_DEFAULT);
}

static inline int clockRenderDigitStringDGetWidth (TCLK *clk, TFRAME **digit, const char *str)
{
	int w = 0;
	
	while (*str){
		if (*str >= '0' && *str <= ':'){
			int chr = *str - '0';
			w += (digit[chr]->width - clk->digital.charOverlap);
		}
		str++;
	}
	return w;
}

static inline int clockRenderDigitStringDGetHeight (TCLK *clk, TFRAME **digit)
{
	int h = 100;
	if (digit[0]) h = digit[0]->height;
	return h;
}

static inline void clockRenderDigitStringD (TCLK *clk, TFRAME **digit, TFRAME *frame, int x, int y, const char *str)
{
	while (*str){
		if (*str >= '0' && *str <= ':'){
			int chr = *str - '0';
			TFRAME *d = digit[chr];
			//if (x+d->width < frame->width)
				drawImage(d, frame, x, y, d->width-1, d->height-1);
				
#if DRAWBUTTONRECTS
			lDrawRectangle(frame, x, y, x+d->width-1, y+d->height-1, DRAWBUTTONRECTCOL);
#endif
			x += (d->width - clk->digital.charOverlap);
		}
		str++;
	}
}

static inline void clockRenderDigital (TVLCPLAYER *vp, TCLK *clk, TFRAME *frame, int x, int y, const int drawSec)
{
#if 0
	const struct tm *tdate = getTimeReal(NULL);
	const int hr = tdate->tm_hour;
	const int min = tdate->tm_min;
	const int sec = tdate->tm_sec;

	char buffer[32];
	if (drawSec)
		__mingw_snprintf(buffer, sizeof(buffer), "%.2i:%.2i:%.2i", hr, min, sec);
	else
		__mingw_snprintf(buffer, sizeof(buffer), "%.2i:%.2i", hr, min);
#else
	//char *buffer = "23:45";
	char buffer[32];
	char *format = "hh\':\'mm";
	GetTimeFormatA(0, TIME_FORCE24HOURFORMAT, 0, format, buffer, sizeof(buffer));
	//printf("clockRenderDigital #%s#\n", buffer);
#endif

	int w = clockRenderDigitStringDGetWidth(clk, clk->digital.images, buffer) + clk->digital.charOverlap;
	x = (frame->width - w)/2;
	if (x < 0) x = 0;
	y = (frame->height - clockRenderDigitStringDGetHeight(clk, clk->digital.images))/2;
	//printf("clockRenderDigitStringDGetWidth %i %i\n", w, x);
		
	clockRenderDigitStringD(clk, clk->digital.images, frame, x, y, buffer);
}

static inline void clockRenderBoxDigital (TVLCPLAYER *vp, TCLK *clk, TFRAME *frame, const int x, const int y, const int drawSec)
{
	const struct tm *tdate = getTimeReal(NULL);
	const int hr = tdate->tm_hour;
	const int min = tdate->tm_min;
	//const int sec = tdate->tm_sec;

	char buffer[16];
	__mingw_snprintf(buffer, sizeof(buffer), "%.2i", hr);
	
	int h = 0;
	lGetTextMetrics(frame->hw, buffer, PF_MIDDLEJUSTIFY|PF_CLIPWRAP, CLK_BOXDIGITAL_TIME_FONT, NULL, &h);
	int y1 = (frame->height/4) - (h/2);
	
	clockRenderDigitStringBoxDigital(clk, clk->digital.images, frame, 0, y1, buffer);
	__mingw_snprintf(buffer, sizeof(buffer), "%.2i", min);
	
	lGetTextMetrics(frame->hw, buffer, PF_MIDDLEJUSTIFY|PF_CLIPWRAP, CLK_BOXDIGITAL_TIME_FONT, NULL, &h);
	y1 = (frame->height - ((frame->height/4) + (h/2)))-1;
	
	clockRenderDigitStringBoxDigital(clk, clk->digital.images, frame, 0, y1, buffer);
}

static inline void clockRenderSBDK (TCLK *clk, const int dk, const int font, const unsigned int colour, const char Char)
{
	if (sbuiSetDKImageChar(clk->com->vp, dk, font, colour, Char)){
		lSleep(10);
	}
}
	
static inline void clockRenderSBDate (TCLK *clk, const int font, const int colourFore, const int foreAlpha, const int colourBack, const int radius, const double alpha, const int clearUnused)
{
	
	void *hw = clk->com->vp->ml->hw;
	
	lSetForegroundColour(hw, foreAlpha<<24|colourFore);
	lSetRenderEffect(hw, LTR_BLUR4);
	lSetFilterAttribute(hw, LTR_BLUR4, LTRA_BLUR_COLOUR, colourBack);
	lSetFilterAttribute(hw, LTR_BLUR4, LTRA_BLUR_RADIUS, radius);
	lSetFilterAttribute(hw, LTR_BLUR4, LTRA_BLUR_SETTOP, 1);
	lSetFilterAttribute(hw, LTR_BLUR4, LTRA_BLUR_X, 0);
	lSetFilterAttribute(hw, LTR_BLUR4, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(hw, LTR_BLUR4, LTRA_BLUR_ALPHA, alpha * 1000.0);


	const struct tm *tdate = getTimeReal(NULL);
	const char *wday = weekdayName_upper[tdate->tm_wday];

	if (clearUnused) clockRenderSBDK(clk, SBUI_DK_6, font, colourBack, 32);
	clockRenderSBDK(clk, SBUI_DK_7, font, colourBack, wday[0]);
	clockRenderSBDK(clk, SBUI_DK_8, font, colourBack, wday[1]);
	clockRenderSBDK(clk, SBUI_DK_9, font, colourBack, wday[2]);
	if (clearUnused) clockRenderSBDK(clk, SBUI_DK_10, font, colourBack, 32);
		
	if (tdate->tm_mday < 10){
		clockRenderSBDK(clk, SBUI_DK_1, font, colourBack, '\0'+48);
		clockRenderSBDK(clk, SBUI_DK_2, font, colourBack, tdate->tm_mday+48);
	}else{
		clockRenderSBDK(clk, SBUI_DK_1, font, colourBack, (tdate->tm_mday/10)+48);
		clockRenderSBDK(clk, SBUI_DK_2, font, colourBack, (tdate->tm_mday%10)+48);
	}
	
	const char *mon = monthName_upper[tdate->tm_mon];
	clockRenderSBDK(clk, SBUI_DK_3, font, colourBack, mon[0]);
	clockRenderSBDK(clk, SBUI_DK_4, font, colourBack, mon[1]);
	clockRenderSBDK(clk, SBUI_DK_5, font, colourBack, mon[2]);
	
	lSetRenderEffect(hw, LTR_DEFAULT);
}

static inline void clockRenderPredator5pt (TCLK *clk, TFRAME *frame, TFRAME **images, int xc, int yc, const int drawFlags, const int num)
{
	if (!drawFlags) return;

	const int drawBase = drawFlags&1;
	const int drawTop = drawFlags&2;
	const int draw1 = num&1;
	const int draw2 = num&2;
	const int draw4 = num&4;
	const int draw8 = num&8;
	const int draw16 = num&16;


	TFRAME *base, *top;
	int offsetX, offsetY;
		
	if (draw1){
		base = images[CLK_IMG_PRED_BASE0];
		top = images[CLK_IMG_PRED_TOP0];
		offsetX = 69;
		offsetY = 57;
		if (drawBase) drawImageCentered(base, frame, xc-offsetX, yc - offsetY);
		if (drawTop) drawImageCentered(top, frame, xc-offsetX, yc - offsetY);	
	}

	if (draw2){
		offsetX = 8;
		offsetY = 12;
		base = images[CLK_IMG_PRED_BASE18];
		top = images[CLK_IMG_PRED_TOP18];
		if (drawBase) drawImageCentered(base, frame, xc-offsetX, yc - offsetY);
		if (drawTop) drawImageCentered(top, frame, xc-offsetX, yc - offsetY);	
	}

	if (draw4){
		base = images[CLK_IMG_PRED_BASE135];
		top = images[CLK_IMG_PRED_TOP135];
		offsetX = 32;
		offsetY = 61;
		if (drawBase) drawImageCentered(base, frame, xc-offsetX, yc + offsetY);
		if (drawTop) drawImageCentered(top, frame, xc-offsetX, yc + offsetY);	
	}
	
	if (draw16){
		offsetX = 130;
		offsetY = 12;
		base = images[CLK_IMG_PRED_BASE243];
		top = images[CLK_IMG_PRED_TOP243];
		if (drawBase) drawImageCentered(base, frame, xc - offsetX, yc - offsetY);
		if (drawTop) drawImageCentered(top, frame, xc - offsetX, yc - offsetY);
	}

	if (draw8){
		base = images[CLK_IMG_PRED_BASE45];
		top = images[CLK_IMG_PRED_TOP45];
		offsetX = 106;
		offsetY = 61;
		if (drawBase) drawImageCentered(base, frame, xc-offsetX, yc + offsetY);
		if (drawTop) drawImageCentered(top, frame, xc-offsetX, yc + offsetY);		
	}
}

static inline void clockRenderPredator6pt (TCLK *clk, TFRAME *frame, TFRAME **images, int xc, int yc, const int drawFlags, const int num)
{
	
	if (!drawFlags) return;
	const int drawBase = drawFlags&1;
	const int drawTop = drawFlags&2;

	const int draw1 = num&1;
	const int draw2 = num&2;
	const int draw4 = num&4;
	const int draw8 = num&8;
	const int draw16 = num&16;
	const int draw32 = num&32;


	TFRAME *base, *top;
	int offsetX, offsetY;
		
	if (draw1){
		base = images[CLK_IMG_PRED_BASE45];
		top = images[CLK_IMG_PRED_TOP45];
		offsetX = 32;
		offsetY = 61;
		if (drawBase) drawImageCentered(base, frame, xc-offsetX, yc - offsetY);
		if (drawTop) drawImageCentered(top, frame, xc-offsetX, yc - offsetY);	
	}

	if (draw2){
		offsetX = 0;
		offsetY = 0;
		base = images[CLK_IMG_PRED_BASE90];
		top = images[CLK_IMG_PRED_TOP90];
		if (drawBase) drawImageCentered(base, frame, xc-offsetX, yc - offsetY);
		if (drawTop) drawImageCentered(top, frame, xc-offsetX, yc - offsetY);	
	}

	if (draw4){
		base = images[CLK_IMG_PRED_BASE135];
		top = images[CLK_IMG_PRED_TOP135];
		offsetX = 32;
		offsetY = 61;
		if (drawBase) drawImageCentered(base, frame, xc-offsetX, yc + offsetY);
		if (drawTop) drawImageCentered(top, frame, xc-offsetX, yc + offsetY);	
	}
	
	if (draw32){
		offsetX = 106;
		offsetY = 61;
		base = images[CLK_IMG_PRED_BASE135];
		top = images[CLK_IMG_PRED_TOP135];
		if (drawBase) drawImageCentered(base, frame, xc - offsetX, yc - offsetY);
		if (drawTop) drawImageCentered(top, frame, xc - offsetX, yc - offsetY);	
	}
	
	if (draw16){
		offsetX = 138;
		offsetY = 0;
		base = images[CLK_IMG_PRED_BASE90];
		top = images[CLK_IMG_PRED_TOP90];
		if (drawBase) drawImageCentered(base, frame, xc - offsetX, yc - offsetY);
		if (drawTop) drawImageCentered(top, frame, xc - offsetX, yc - offsetY);
	}

	if (draw8){
		base = images[CLK_IMG_PRED_BASE45];
		top = images[CLK_IMG_PRED_TOP45];
		offsetX = 106;
		offsetY = 61;
		if (drawBase) drawImageCentered(base, frame, xc-offsetX, yc + offsetY);
		if (drawTop) drawImageCentered(top, frame, xc-offsetX, yc + offsetY);		
	}
}

static inline void clockRenderPredator (TVLCPLAYER *vp, TCLK *clk, TFRAME *frame, const int xc, const int yc, const int isIdle)
{
	
	if (isIdle && clk->predator.images[CLK_IMG_PRED_SKULL])
		drawImageCentered(clk->predator.images[CLK_IMG_PRED_SKULL], frame, xc, yc);
	
	char buffer[32];
	double nanos;
	const struct tm *tdate = getTimeReal(&nanos);
    const double secs = tdate->tm_sec + nanos;
    const int mins = tdate->tm_min;
    const int hrs  = tdate->tm_hour;
	__mingw_snprintf(buffer, sizeof(buffer), "%.2i:%.2i", hrs, mins);
	
	const int font = CLK_PREDATOR_TIME_FONT;
	int op = LTR_BLUR4;
	lSetRenderEffect(frame->hw, op);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_COLOUR, 0xF30204);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_RADIUS, 12);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_SETTOP, 0);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_ALPHA, 1000);
	drawStr(frame, xc, 16, font, 0xFF<<24 | 0xAFA204, buffer, DS_MIDDLEJUSTIFY);

	op = LTR_BLUR5;
	lSetRenderEffect(frame->hw, op);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_COLOUR, 0x3F0204);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_RADIUS, 4);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_SETTOP, 0);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_Y, 0);
	if (!isIdle)
		lSetFilterAttribute(frame->hw, op, LTRA_BLUR_ALPHA, 450);
	else
		lSetFilterAttribute(frame->hw, op, LTRA_BLUR_ALPHA, 600);
	drawStr(frame, xc, 16, font, 0, buffer, DS_MIDDLEJUSTIFY);
	
	
	lSetRenderEffect(frame->hw, LTR_DEFAULT);


	int left = xc / 3.0;
	int middle = xc;
	int right = frame->width  - left;
	int y = yc + 46;

	TFRAME **images = clk->predator.images;
	for (int r = 1; r < 3; r++){	// draw base layers first
    	int x = left + 76;
    	clockRenderPredator5pt(clk, frame, images, x, y, r&1, hrs);
    	clockRenderPredator5pt(clk, frame, images, x, y, r&2, hrs);

    	x = middle + 65;
    	clockRenderPredator6pt(clk, frame, images, x, y, r&1, mins);
    	clockRenderPredator6pt(clk, frame, images, x, y, r&2, mins);
    
		x = right + 54;
    	clockRenderPredator6pt(clk, frame, images, x, y, r&1, secs);
    	clockRenderPredator6pt(clk, frame, images, x, y, r&2, secs);
    }
}

static inline void clockRenderPolar (TVLCPLAYER *vp, TCLK *clk, TFRAME *frame, const double xc, const double yc, const int drawSec)
{
	const int isIdle = getIdle(vp);
	
	double nanos;
	struct tm *tdate = getTimeReal(&nanos);
	double secs = tdate->tm_sec + nanos;
    double mins = tdate->tm_min + (secs / 60.0);
    double hrs  = tdate->tm_hour+ (mins / 60.0);
    
	// correct week begins on which day. defaults to Sunday; make it Monday
    if (tdate->tm_wday == 0) tdate->tm_wday = 6;
    else tdate->tm_wday--;
    
	double sec = (360.0 / 60.0) * secs;
	double min = (360.0 / 60.0) * mins;
	double hr = (360.0 / 24.0) * hrs;
	double daywk = (360.0 / 7.0) * (double)((double)tdate->tm_wday + ((1.0/24.0) * hrs));
	double daymon = (360.0 / (double)clkDaysInMonth(tdate->tm_year, tdate->tm_mon)) * (double)(tdate->tm_mday+1);
	double mon = (360.0 / 12.0) * (double)(tdate->tm_mon + (double)((tdate->tm_mday+1)/(double)clkDaysInMonth(tdate->tm_year, tdate->tm_mon)));

#if 0
	min = sec;
	hr = sec;
	daywk = sec;
	daymon = sec;
	mon = sec;
#endif

	const double diam = 80.0;		// (inner)
	const double h = yc - diam;
	const double pitch = 25.0;
	double radius = 240.0 - 32.0 - 3.0 + pitch;
	const int baseColour = clk->polar.baseColour;
	
	if (!isIdle)
		lDrawCircleFilled(frame, xc, yc, radius, 80<<24|(baseColour&0xFFFFFF));
	
	if (drawSec)
		lDrawCircle(frame, xc, yc, radius, baseColour);
	radius -= pitch;
	lDrawCircle(frame, xc, yc, radius, baseColour);
	radius -= pitch;
	lDrawCircle(frame, xc, yc, radius, baseColour);
	radius -= pitch;
	lDrawCircle(frame, xc, yc, radius, baseColour);
	radius -= pitch;
	lDrawCircle(frame, xc, yc, radius, baseColour);
	radius -= pitch;
	lDrawCircle(frame, xc, yc, radius, baseColour);
	radius -= pitch;
	lDrawCircle(frame, xc, yc, radius, baseColour);


	double pos = 253.5;
	const double len = pitch + 5.5;
	const double halfPitch = pitch / 2.0;
	const double vect = -((1.0/yc) * (double)(h + pitch));
	double xr = 0.0, yr = 0.0;
	double cx = 0.0, cy = 0.0;

	
	lDrawLine(frame, xc, yc-diam, xc, yc - (h + (pitch-4)), baseColour);	// begin marker for the fill


	if (drawSec){
		cx = xc+2;
		cy = yc - (h + halfPitch);
		rotateZ(sec * DEGTORAD, 0.0, vect, &xr, &yr);
		drawMarker(frame, xc+(xr*pos), yc+(yr*pos), xc+(xr*(pos+len)), yc+(yr*(pos+len)), baseColour);
		drawBand(frame, cx, cy, clk->polar.bandColours[0], baseColour);
	}
	
	cx = xc+2;
	cy = yc - (h - halfPitch);
	rotateZ(min * DEGTORAD, 0.0, vect, &xr, &yr);
	pos -= len+0.25;
	drawMarker(frame, xc+(xr*pos), yc+(yr*pos), xc+(xr*(pos+len)), yc+(yr*(pos+len)), baseColour);
	drawBand(frame, cx, cy, clk->polar.bandColours[1], baseColour);


	cx = xc+2;
	cy = (yc - (h - (pitch*1.0) - halfPitch));
	rotateZ(hr * DEGTORAD, 0.0, vect, &xr, &yr);
	pos -= len+0.75;
	drawMarker(frame, xc+(xr*pos), yc+(yr*pos), xc+(xr*(pos+len)), yc+(yr*(pos+len)), baseColour);
	drawBand(frame, cx, cy, clk->polar.bandColours[0], baseColour);
	
	cx = xc+2;
	cy = (yc - (h - (pitch*2.0) - halfPitch));
	rotateZ(daywk * DEGTORAD, 0.0, vect, &xr, &yr);
	pos -= len+0.0;
	drawMarker(frame, xc+(xr*pos), yc+(yr*pos), xc+(xr*(pos+len)), yc+(yr*(pos+len)), baseColour);
	drawBand(frame, cx, cy, clk->polar.bandColours[1], baseColour);

	cx = xc+2;
	cy = (yc - (h - (pitch*3.0) - halfPitch));
	rotateZ(daymon * DEGTORAD, 0.0, vect, &xr, &yr);
	pos -= len+0.5;
	drawMarker(frame, xc+(xr*pos), yc+(yr*pos), xc+(xr*(pos+len)), yc+(yr*(pos+len)), baseColour);
	drawBand(frame, cx, cy, clk->polar.bandColours[0], baseColour);

	cx = xc+2;
	cy = (yc - (h - (pitch*4.0) - halfPitch));
	rotateZ(mon * DEGTORAD, 0.0, vect, &xr, &yr);
	pos -= len+0.75;
	drawMarker(frame, xc+(xr*pos), yc+(yr*pos), xc+(xr*(pos+len)), yc+(yr*(pos+len)), baseColour);
	drawBand(frame, cx, cy, clk->polar.bandColours[1], baseColour);	



	int x = xc + 4;
	int y = yc - 240 + 10;
	
	if (drawSec)
		drawStr(frame, x, y, CLK_POLAR_LABEL_FONT, clk->polar.bandColours[1], "sec", NSEX_LEFT);
	y += pitch;
	drawStr(frame, x, y, CLK_POLAR_LABEL_FONT, clk->polar.bandColours[0], "min", NSEX_LEFT);
	y += pitch;
	drawStr(frame, x, y, CLK_POLAR_LABEL_FONT, clk->polar.bandColours[1], "hrs", NSEX_LEFT);
	y += pitch;
	drawStr(frame, x, y, CLK_POLAR_LABEL_FONT, clk->polar.bandColours[0], weekdayName_lower[tdate->tm_wday], NSEX_LEFT);
	y += pitch;
	drawStr(frame, x, y, CLK_POLAR_LABEL_FONT, clk->polar.bandColours[1], "day", NSEX_LEFT);
	y += pitch;
	drawStr(frame, x, y, CLK_POLAR_LABEL_FONT, clk->polar.bandColours[0], "mth"/*monthName_lower[tdate->tm_mon]*/, NSEX_LEFT);


	const int op = LTR_BLUR4;
	lSetRenderEffect(frame->hw, op);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_COLOUR, clk->polar.bandColours[1]);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_RADIUS, 2);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_SETTOP, 1);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, op, LTRA_BLUR_ALPHA, 1000);
	
	char buffer[32];
	__mingw_snprintf(buffer, sizeof(buffer), "%.2i:%.2i", tdate->tm_hour, tdate->tm_min);

	drawStr(frame, xc, yc-64, CLK_POLAR_INNER_FONT, clk->polar.bandColours[1], buffer, DS_MIDDLEJUSTIFY);
	drawStr(frame, xc, yc-24, CLK_POLAR_INNER_FONT, clk->polar.bandColours[1], weekdayName_lower[tdate->tm_wday], DS_MIDDLEJUSTIFY);
	
	char *justPad = " ";
	if (monthName_lower[tdate->tm_mon][0] == 'j') justPad = "  ";
	__mingw_snprintf(buffer, sizeof(buffer), "%s%s %.2i", justPad, monthName_lower[tdate->tm_mon], tdate->tm_mday);
	drawStr(frame, xc, yc+16, CLK_POLAR_INNER_FONT, clk->polar.bandColours[1], buffer, DS_MIDDLEJUSTIFY);
	
	lSetRenderEffect(frame->hw, LTR_DEFAULT);
	
}

static void (CALLBACK sbSetDkTimerCB)(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	//printf("sbSetDkTimerCB in: %i\n", (int)GetCurrentThreadId());
	
	TCLK *clk = (TCLK*)dwUser;
	if (getApplState(clk->com->vp)){
		if (renderLock(clk->com->vp)){
			if (clk->sbdk.renderState){	// don't render if page was just closed
				TCLK_SBDATE_CFG *date = &clk->digital.date;
				
				if (clk->displayType == CLOCK_BOXDIGITAL){
					//clockRenderSBDate(clk, CLK_SBDK_DATE_FONT, 0x000000, 173, 0xCFB000, 12, 1.0, 1);
					date = &clk->boxdigital.date;
					
				}else if (clk->displayType == CLOCK_BUTTERFLY){
					//clockRenderSBDate(clk, CLK_SBDK_DATE_FONT, 0xEEEEEE, 173, 0xD99738, 12, 1.0, 1);
					date = &clk->butterfly.date;
					
				}else if (clk->displayType == CLOCK_ANALOGUE){
					//clockRenderSBDate(clk, CLK_SBDK_DATE_FONT, COL_BLACK, 173, COL_BLUE_SEA_TINT, 8, 1.0, 1);
					date = &clk->analogue.date;
					
				}else if (clk->displayType == CLOCK_PREDATOR){
					sbuiSetDKImageArtId(clk->com->vp, SBUI_DK_6, clk->predator.imageIds[CLK_IMG_PRED_PREDLEFT]);
					lSleep(10);
					sbuiSetDKImageArtId(clk->com->vp, SBUI_DK_10, clk->predator.imageIds[CLK_IMG_PRED_PREDRIGHT]);
					lSleep(10);
					//clockRenderSBDate(clk, CLK_SBDK_DATE_FONT, 0x530204, 173, 0xF30204, 12, 1.0, 0);
					date = &clk->predator.date;
					
				}else if (clk->displayType == CLOCK_POLAR){
					//clockRenderSBDate(clk, CLK_SBDK_DATE_FONT, COL_BLUE_SEA_TINT, 173, 0xFFFFFF, 12, 1.0, 1);
					date = &clk->polar.date;
					
				}else{
					//clockRenderSBDate(clk, CLK_SBDK_DATE_FONT, 0xe6ccea, 250, 0xfa51cd, 15, 1.0, 1);
					date = &clk->digital.date;
				}
				clockRenderSBDate(clk, date->font, date->foreColour, date->foreAlpha, date->backColour, date->backRadius, date->backAlpha, date->clearUnused);
			}
			renderUnlock(clk->com->vp);
		}
	}
	clk->sbdk.timerState = 0;
	
	//printf("sbSetDkTimerCB out: %i\n", (int)GetCurrentThreadId());
}

static inline int page_clkRender (TCLK *clk, TVLCPLAYER *vp, TFRAME *frame)
{	
	if (clk->displayType == CLOCK_DIGITAL){
		clockRenderDigital(vp, clk, frame, 0, 0, 0);
		
	}else if (clk->displayType == CLOCK_ANALOGUE){
		clockRenderAnalogue(vp, clk, frame, clk->pos.x, clk->pos.y, !getIdle(vp));
		
#if DRAWMISCDETAIL
		lDrawLine(clk->analogue.face, 0, 0, clk->analogue.face->width-1, clk->analogue.face->height-1, 255<<24|COL_PURPLE);
		lDrawLine(clk->analogue.face, 0, clk->analogue.face->height-1, clk->analogue.face->width-1, 0, 255<<24|COL_PURPLE);
#endif	
	}else if (clk->displayType == CLOCK_BUTTERFLY){
		clockRenderButterfly(vp, clk, frame, clk->pos.x, clk->pos.y, !getIdle(vp));

#if DRAWMISCDETAIL
		lDrawLine(clk->butterfly.face, 0, 0, clk->butterfly.face->width-1, clk->butterfly.face->height-1, 255<<24|COL_PURPLE);
		lDrawLine(clk->butterfly.face, 0, clk->butterfly.face->height-1, clk->butterfly.face->width-1, 0, 255<<24|COL_PURPLE);
#endif		

	}else if (clk->displayType == CLOCK_BOXDIGITAL){
		clockRenderBoxDigital(vp, clk, frame, 0, 0, 0);
	
	}else if (clk->displayType == CLOCK_POLAR){
		clockRenderPolar(vp, clk, frame, clk->pos.x, clk->pos.y, !getIdle(vp));
		
	}else if (clk->displayType == CLOCK_PREDATOR){
		clockRenderPredator(vp, clk, frame, clk->pos.x, clk->pos.y, getIdle(vp));
	}

	if (isSBUIEnabled(vp) && getIdle(vp) && !clk->sbdk.timerState){
		clk->sbdk.timerState = 1;
		timeSetEvent(100, 20, sbSetDkTimerCB, (DWORD_PTR)clk, TIME_ONESHOT);
	}

#if DRAWMISCDETAIL
	lDrawLine(frame, 0, 0, frame->width-1, frame->height-1, 255<<24|COL_CYAN);
	lDrawLine(frame, 0, frame->height-1, frame->width-1, 0, 255<<24|COL_CYAN);
#endif
	return 1;
}

static inline int page_clkInput (TCLK *clk, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
		setVolumeDisplay(vp, setVolume(vp, getVolume(vp, VOLUME_APP)+3, VOLUME_APP));
		break;
			
	  case PAGE_IN_WHEEL_BACK:
	  	setVolumeDisplay(vp, setVolume(vp, getVolume(vp, VOLUME_APP)-3, VOLUME_APP));
		break;
		
	  case PAGE_IN_TOUCH_DOWN:
	  	timerClear(vp, TIMER_SETIDLEA);
	  	timerClear(vp, TIMER_SETIDLEB);
	  	timerClear(vp, TIMER_SETIDLEC);
	  	setAwake(vp);
	  	page2SetPrevious(clk);
	  	break;

	  /*case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  	break;*/
	}
		
	return 1;
}

static inline int page_clkStartup (TCLK *clk, TVLCPLAYER *vp, const int width, const int height)
{
	settingsGet(vp, "clock.face.dial.x", &clk->butterfly.cx);
	settingsGet(vp, "clock.face.dial.y", &clk->butterfly.cy);
	settingsGet(vp, "clock.digital.overlap", &clk->digital.charOverlap);
		
	clk->pos.x = 0;
	clk->pos.y = 0;
	clk->t0 = 0;	

	return 1;
}
		
static inline int page_clkInitalize (TCLK *clk, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_CLOCK);
	
	clk->polar.bandColours[0] = 0xFF<<24 | COL_BLUE_SEA_TINT;
	clk->polar.bandColours[1] = 0xFF<<24 | COL_WHITE;
	clk->polar.baseColour = 0xFF<<24 | COL_BLACK;


	TCLK_SBDATE_CFG *date = &clk->analogue.date;
	settingsGet(vp, "clock.date.analogue.font",			&date->font);
	settingsGet(vp, "clock.date.analogue.foreColour", 	&date->foreColour);
	settingsGet(vp, "clock.date.analogue.foreAlpha", 	&date->foreAlpha);
	settingsGet(vp, "clock.date.analogue.backColour",	&date->backColour);
	settingsGet(vp, "clock.date.analogue.backRadius", 	&date->backRadius);
	settingsGet(vp, "clock.date.analogue.backAlpha", 	&date->backAlpha);
	settingsGet(vp, "clock.date.analogue.clearUnused", 	&date->clearUnused);

	date = &clk->digital.date;
	settingsGet(vp, "clock.date.digital.font", &date->font);
	settingsGet(vp, "clock.date.digital.foreColour", &date->foreColour);
	settingsGet(vp, "clock.date.digital.foreAlpha", &date->foreAlpha);
	settingsGet(vp, "clock.date.digital.backColour", &date->backColour);
	settingsGet(vp, "clock.date.digital.backRadius", &date->backRadius);
	settingsGet(vp, "clock.date.digital.backAlpha", &date->backAlpha);
	settingsGet(vp, "clock.date.digital.clearUnused", &date->clearUnused);

	date = &clk->boxdigital.date;
	settingsGet(vp, "clock.date.boxdigital.font", &date->font);
	settingsGet(vp, "clock.date.boxdigital.foreColour", &date->foreColour);
	settingsGet(vp, "clock.date.boxdigital.foreAlpha", &date->foreAlpha);
	settingsGet(vp, "clock.date.boxdigital.backColour", &date->backColour);
	settingsGet(vp, "clock.date.boxdigital.backRadius", &date->backRadius);
	settingsGet(vp, "clock.date.boxdigital.backAlpha", &date->backAlpha);
	settingsGet(vp, "clock.date.boxdigital.clearUnused", &date->clearUnused);
	
	date = &clk->polar.date;
	settingsGet(vp, "clock.date.polar.font", &date->font);
	settingsGet(vp, "clock.date.polar.foreColour", &date->foreColour);
	settingsGet(vp, "clock.date.polar.foreAlpha", &date->foreAlpha);
	settingsGet(vp, "clock.date.polar.backColour", &date->backColour);
	settingsGet(vp, "clock.date.polar.backRadius", &date->backRadius);
	settingsGet(vp, "clock.date.polar.backAlpha", &date->backAlpha);
	settingsGet(vp, "clock.date.polar.clearUnused", &date->clearUnused);
	
	date = &clk->predator.date;
	settingsGet(vp, "clock.date.predator.font", &date->font);
	settingsGet(vp, "clock.date.predator.foreColour", &date->foreColour);
	settingsGet(vp, "clock.date.predator.foreAlpha", &date->foreAlpha);
	settingsGet(vp, "clock.date.predator.backColour", &date->backColour);
	settingsGet(vp, "clock.date.predator.backRadius", &date->backRadius);
	settingsGet(vp, "clock.date.predator.backAlpha", &date->backAlpha);
	settingsGet(vp, "clock.date.predator.clearUnused", &date->clearUnused);
	

	date = &clk->butterfly.date;
	settingsGet(vp, "clock.date.butterfly.font", &date->font);
	settingsGet(vp, "clock.date.butterfly.foreColour", &date->foreColour);
	settingsGet(vp, "clock.date.butterfly.foreAlpha", &date->foreAlpha);
	settingsGet(vp, "clock.date.butterfly.backColour", &date->backColour);
	settingsGet(vp, "clock.date.butterfly.backRadius", &date->backRadius);
	settingsGet(vp, "clock.date.butterfly.backAlpha", &date->backAlpha);
	settingsGet(vp, "clock.date.butterfly.clearUnused", &date->clearUnused);
		
	return 1;
}

static inline int page_clkRenderInit (TCLK *clk, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	
	wchar_t *digitsFolder;
	settingsGetW(vp, "clock.digital.digits", &digitsFolder);
	if (!digitsFolder) return 0;
	
	wchar_t buffer[MAX_PATH+1];
	for (int i = 0; i < 10; i++){
		__mingw_snwprintf(buffer, MAX_PATH, L"common/digits/%ls/%i.png", digitsFolder, i);
		clk->digital.imageIds[CLK_IMG_DIG_0+i] = imageManagerImageAdd(vp->im, buffer);
	}

	__mingw_snwprintf(buffer, MAX_PATH, L"common/digits/%ls/colon.png", digitsFolder);
	clk->digital.imageIds[CLK_IMG_DIG_COLON] = imageManagerImageAdd(vp->im, buffer);
	my_free(digitsFolder);

	clk->analogue.imageIds[CLK_IMG_FACE] = imageManagerImageAdd(vp->im, L"clock/analogue/face.png");
	clk->analogue.imageIds[CLK_IMG_HOUR] = imageManagerImageAdd(vp->im, L"clock/analogue/hr.png");
	clk->analogue.imageIds[CLK_IMG_MINUTE] = imageManagerImageAdd(vp->im, L"clock/analogue/min.png");
	clk->analogue.imageIds[CLK_IMG_SECOND] = imageManagerImageAdd(vp->im, L"clock/analogue/sec.png");
	clk->analogue.imageIds[CLK_IMG_CAP] = imageManagerImageAdd(vp->im, L"clock/analogue/cap.png");
	
	clk->butterfly.imageIds[CLK_IMG_FACE_BF] = imageManagerImageAdd(vp->im, L"clock/butterfly/face.png");
	clk->butterfly.imageIds[CLK_IMG_HOUR_BF] = imageManagerImageAdd(vp->im, L"clock/butterfly/hr.png");
	clk->butterfly.imageIds[CLK_IMG_MINUTE_BF] = imageManagerImageAdd(vp->im, L"clock/butterfly/min.png");
	clk->butterfly.imageIds[CLK_IMG_SECOND_BF] = imageManagerImageAdd(vp->im, L"clock/butterfly/sec.png");
	clk->butterfly.imageIds[CLK_IMG_CAP_BF] = imageManagerImageAdd(vp->im, L"clock/butterfly/cap.png");	


	const wchar_t *predatorImages[predatorImageTotal] = {
		L"clock/predator/base_45.png",	
	    L"clock/predator/base_90.png",
	    L"clock/predator/base_135.png",
	    L"clock/predator/top_45.png",
	    L"clock/predator/top_90.png",
	    L"clock/predator/top_135.png",
	    L"clock/predator/base_0.png",		// 6
	    L"clock/predator/base_18.png",		//(72d)
	    L"clock/predator/base_243.png",		// 8
	    L"clock/predator/top_0.png",
	    L"clock/predator/top_18.png",		// 10
	    L"clock/predator/top_243.png",
	    L"clock/predator/predator5.png",	// 12
	    L"clock/predator/skull.png",
	    L"clock/predator/predator6.png"};
	
	for (int i = 0; i < predatorImageTotal; i++)
		clk->predator.imageIds[i] = imageManagerImageAdd(vp->im, predatorImages[i]);

	renderSignalUpdateNow(vp);
	return 1;
}

static inline int page_clkRenderBegin (TCLK *clk, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	clk->displayType = vp->gui.clockType;
	
	if (clk->displayType == CLOCK_ANALOGUE){
		clk->analogue.face= imageManagerImageAcquire(vp->im, clk->analogue.imageIds[CLK_IMG_FACE]);
		clk->analogue.hr  = imageManagerImageAcquire(vp->im, clk->analogue.imageIds[CLK_IMG_HOUR]);
		clk->analogue.min = imageManagerImageAcquire(vp->im, clk->analogue.imageIds[CLK_IMG_MINUTE]);
		clk->analogue.sec = imageManagerImageAcquire(vp->im, clk->analogue.imageIds[CLK_IMG_SECOND]);
		clk->analogue.cap = imageManagerImageAcquire(vp->im, clk->analogue.imageIds[CLK_IMG_CAP]);
		
		clk->pos.x = (frame->width - clk->analogue.face->width)/2;
		clk->pos.y = (frame->height - clk->analogue.face->height)/2;
	
	}else if (clk->displayType == CLOCK_BUTTERFLY){
		clk->butterfly.face= imageManagerImageAcquire(vp->im, clk->butterfly.imageIds[CLK_IMG_FACE_BF]);
		clk->butterfly.hr  = imageManagerImageAcquire(vp->im, clk->butterfly.imageIds[CLK_IMG_HOUR_BF]);
		clk->butterfly.min = imageManagerImageAcquire(vp->im, clk->butterfly.imageIds[CLK_IMG_MINUTE_BF]);
		clk->butterfly.sec = imageManagerImageAcquire(vp->im, clk->butterfly.imageIds[CLK_IMG_SECOND_BF]);
		clk->butterfly.cap = imageManagerImageAcquire(vp->im, clk->butterfly.imageIds[CLK_IMG_CAP_BF]);
		
		clk->pos.x = (frame->width - clk->butterfly.face->width)/2;
		clk->pos.y = (frame->height - clk->butterfly.face->height)/2;
		
	}else if (clk->displayType == CLOCK_DIGITAL){
		clk->digital.images[CLK_IMG_DIG_COLON] = imageManagerImageAcquire(vp->im, clk->digital.imageIds[CLK_IMG_DIG_COLON]);
		for (int i = 0; i < 10; i++)
			clk->digital.images[i] = imageManagerImageAcquire(vp->im, clk->digital.imageIds[CLK_IMG_DIG_0+i]);

		//clk->pos.x = frame->width/2;
		//clk->pos.y = frame->height/2;
		
	}else if (clk->displayType == CLOCK_BOXDIGITAL){
		//clk->pos.x = frame->width/2;
		//clk->pos.y = frame->height/2;

	}else if (clk->displayType == CLOCK_POLAR){
		clk->pos.x = frame->width/2;
		clk->pos.y = frame->height/2;
		lRenderEffectReset(frame->hw, CLK_POLAR_INNER_FONT, LTR_BLUR4);
			
	}else if (clk->displayType == CLOCK_PREDATOR){
		clk->pos.x = frame->width/2;
		clk->pos.y = frame->height/2;

    	for (int i = 0; i < predatorImageTotal; i++)
    		clk->predator.images[i] = imageManagerImageAcquire(vp->im, clk->predator.imageIds[i]);
	}
	
	clk->sbdk.timerState = 0;
	clk->sbdk.renderState = 1;

	return 1;
}

static inline int page_clkRenderEnd (TCLK *clk, TVLCPLAYER *vp, int64_t destId, int64_t data2, void *opaquePtr)
{
	clk->sbdk.renderState = 0;
	
	if (clk->displayType == CLOCK_ANALOGUE){
		for (int i = 0; i < analogueImageTotal; i++)
    		imageManagerImageRelease(vp->im, clk->analogue.imageIds[i]);
		
	}else if (clk->displayType == CLOCK_BUTTERFLY){	
		for (int i = 0; i < butterflyImageTotal; i++)
    		imageManagerImageRelease(vp->im, clk->butterfly.imageIds[i]);
		
	}else if (clk->displayType == CLOCK_DIGITAL){
		imageManagerImageRelease(vp->im, clk->digital.imageIds[CLK_IMG_DIG_COLON]);
	
		for (int i = 0; i < 10; i++)
			imageManagerImageRelease(vp->im, clk->digital.imageIds[CLK_IMG_DIG_0+i]);
			
	}else if (clk->displayType == CLOCK_PREDATOR){		
		for (int i = 0; i < predatorImageTotal; i++)
    		imageManagerImageRelease(vp->im, clk->predator.imageIds[i]);
	}

	if (isSBUIEnabled(vp) && getIdle(vp))
		sbuiDKSetImages(vp);

	return 1;
}

static inline int page_clkShutdown (TCLK *clk, TVLCPLAYER *vp)
{
	return 1;
}

int page_clkCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TCLK *clk = (TCLK*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_clkCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_clkRender(clk, clk->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		return page_clkRenderBegin(clk, clk->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		return page_clkRenderEnd(clk, clk->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_clkInput(clk, clk->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_clkRenderInit(clk, clk->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_clkStartup(clk, clk->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_clkInitalize(clk, clk->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_clkShutdown(clk, clk->com->vp);
		
	}
	
	return 1;
}


