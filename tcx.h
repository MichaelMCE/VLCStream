
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



#ifndef _TCX_H_
#define _TCX_H_

#include "mxml/mxml.h"



typedef struct {
	double latitude;
	double longitude;
	uint64_t data;
}tcx_renderPt;

typedef struct {
	int isInside;
	int x;
	int y;
	uint64_t data;
}tcx_screenPt;

typedef struct {
	double daspect;
	double aspect;
	double aspectFactor;
	
	double scaleFactor;
	double scaleLong;
	double scaleLat;
	double scaleLatZoom;
	double scaleLongZoom;

	double zoomFactor;
			
	double dLong;
	double dLat;
	double minLong;
	double minLat;
	double maxLong;
	double maxLat;
	
	double startX;
	double startY;

	int width;
	int height;
	
	struct {
		int total;
		tcx_renderPt *source;
		tcx_renderPt *normalized;
		tcx_renderPt *scaled;
		tcx_screenPt *screen;

		tcx_screenPt *plotted;
		tcx_screenPt *world;
		int plottedTotal;
	}points;
	
	struct {
		int offsetX;
		int offsetY;
		
		int stub1;
		int stub2;
		int stub3;
		int stub4;
		int stub5;
		int stub6;
		int stub7;
	}map;
}tcx_renderContext;


#if 0
typedef struct {
    int year;
    int mon;
    int wday;
    int mday;
    int hour;
    int min;
    int sec;
    int milliseconds;
}tcx_time;
#else
typedef struct {
    int sec;
    int min;
    int hour;
    int mday;
    int mon;
    int year;
    //int wday;
    //int yday;
    //int isdst;
}tcx_time;
#endif

struct tcx_stats {
	int trackPoints;
		
	struct{
		double min;
		double max;
	}latitude;
	struct{
		double min;
		double max;
	}longitude;
	
	struct{
		double ave;
		double max;
		int pts;
	}speed;
	
	struct{
		double min;
		double max;
	}power;
};
	
typedef struct {
	double latitude;				// degrees
	double longitude;				// degrees
	double altitude;				// meters
	double distance;				// meters
	double speed;					// speed as calculated from previous time to this time over above distance
	double power;
	int cadence;					// note: cadence of zero will not factor in to any calculation(s)
	int heartRate;
	
	struct{
		tcx_time time;				// snapshot date/time
		uint64_t time64;				// as above but as a 64bit int, without the year
	}time;
}tcx_trackpoint;

typedef struct {
	struct {
		tcx_trackpoint **list;
		int total;					// total trackPoints
	}trackPoints;
	
	struct {
		double start;
		double end;
		double delta;
		double distance;
	}time;
}tcx_track;

typedef struct {
	struct{
		tcx_time start;				// start date/time of this lap
		uint64_t start64;			// as above but as a 64bit int, without the year
		double totalSeconds;		// length of recorded lap in seconds
		double delta;
	}time;
	
	struct{
		double maximum;				// on this lap
		double averageReported;		// value contained within <Extensions> field (not always available)
	}speed;
	
	struct{
		int averageCalculated;		// on this lap
		int maximumCalculated;		// on this lap
		int maximumReported;
		int average;				// for this lap
	}cadence;

	struct{
		int maximum;				// bpm
		int average;
		int averageCalculated;
	}heartRate;

	double distance;				// covered over this lap
	int calories;					// used for this lap

	struct tcx_stats stats;

	struct{
		tcx_track **list;
		int total;
	}tracks;
}tcx_lap;

#define ACTIVITY_CADENCE_HISTLEN		(512)
#define ACTIVITY_TYPE_ACTIVITY			1
#define ACTIVITY_TYPE_COURSE			2


typedef struct {
	int type;			// describes an activity:1 or course:2
	char name[64];		// name: Biking, running, etc..
	char id[64];		// seems to be the date
	uint64_t startTime64;
	
	struct tcx_stats stats;

	struct {
		tcx_lap **list;
		int total;
	}laps;
	
	double distance;
	double time;
	double speed;
	
	struct {
		int hist[ACTIVITY_CADENCE_HISTLEN];
		int mode;
		int totalPoints;		// excludes cadence values of 0
	}cadence;
}tcx_activity;

typedef struct {
	struct tcx_stats stats;
	
	struct {
		tcx_activity **list;
		int total;
	}activities;
}tcx_activities;


tcx_activities *tcx_openW (const wchar_t *tcxFile);
tcx_activities *tcx_open8 (const char *tcxFile);
void tcx_close (tcx_activities *activities);

tcx_renderContext *tcx_createRenderContext (tcx_activities *activities, const int activityNo, const int width, const int height);
void tcx_closeRenderContext (tcx_renderContext *rc);
tcx_activity *tcx_getActivity (tcx_activities *activities, const int activityNo);


void tcx_zoom (tcx_renderContext *rc, const double factor);
void tcx_scale (tcx_renderContext *rc, const double widthMultiplier, const double heightMultiplier);
void tcx_screen (tcx_renderContext *rc, const double renderOffsetX, const double renderOffsetY);
int tcx_normalize (tcx_renderContext *rc, tcx_activities *activities, const int activityNo, const double minLong, const double minLat);
int tcx_calcStats (tcx_activities *activities);
double tcx_calcScaleMultipliers (tcx_renderContext *rc);

uint64_t tcx_timeTo64 (tcx_time *t);
void tcx_time64ToTime (uint64_t t64, tcx_time *t);
int tcx_timeStrToTime (const char *str, tcx_time *t);


#endif


