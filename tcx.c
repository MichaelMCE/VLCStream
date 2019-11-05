
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
#include "mxml/mxml.h"
#include "tcx.h"




tcx_activity *tcx_getActivity (tcx_activities *activities, const int activityNo)
{
	if (activityNo > 0 && activityNo <= activities->activities.total)
		return activities->activities.list[activityNo-1];
	return NULL;
}

static inline tcx_renderContext *tcx_renderContextAlloc ()
{
	return my_calloc(1, sizeof(tcx_renderContext));
}

tcx_renderContext *tcx_createRenderContext (tcx_activities *activities, const int activityNo, const int width, const int height)
{
	tcx_renderContext *rc = tcx_renderContextAlloc();
	if (!rc) return NULL;


	if (activityNo > 0){
		tcx_activity *activity = tcx_getActivity(activities, 1);
		
		rc->minLat = activity->stats.latitude.min;
		rc->maxLat = activity->stats.latitude.max;
		rc->minLong = activity->stats.longitude.min;
		rc->maxLong = activity->stats.longitude.max;
		rc->points.total = activity->stats.trackPoints;
	}else{
		rc->minLat = activities->stats.latitude.min;
		rc->maxLat = activities->stats.latitude.max;
		rc->minLong = activities->stats.longitude.min;
		rc->maxLong = activities->stats.longitude.max;
		rc->points.total = activities->stats.trackPoints;
	}
	
	
	rc->width = width;
	rc->height = height;
	rc->daspect = (double)rc->width / (double)rc->height;
	rc->dLong = rc->maxLong - rc->minLong;
	rc->dLat = rc->maxLat - rc->minLat;
	rc->aspect = rc->dLong / rc->dLat;
		
		
//	printf("tcx_createRenderContext %f %f\n", rc->dLat, rc->dLong);
		
	// 3.04 is the Strava.com map aspect/scale
	// 2.35 is my display aspect for a best fit (4/3 * 480/272)
	//%scaleFactor = 3.04;
	//rc->scaleFactor = 4.0/3.0 * rc->daspect;
	//rc->scaleFactor = 1.76470 * rc->daspect;
	rc->scaleFactor = 1.71 * rc->daspect;
	//rc->scaleFactor = 2.8;
	rc->zoomFactor = 1.0;
	
	//printf("rc->daspect %f %f, %f\n", rc->daspect, rc->aspect, rc->scaleFactor);

	rc->points.source = my_malloc((1+rc->points.total) * sizeof(tcx_renderPt));
	rc->points.world = my_malloc((1+rc->points.total) * sizeof(tcx_screenPt));
	
	rc->points.normalized = my_malloc((1+rc->points.total) * sizeof(tcx_renderPt));
	rc->points.scaled = my_malloc((1+rc->points.total) * sizeof(tcx_renderPt));
	rc->points.plotted = my_malloc((1+rc->points.total) * sizeof(tcx_screenPt));
	rc->points.screen = my_malloc((1+rc->points.total) * sizeof(tcx_screenPt));
	
	return rc;
}

static inline void tcx_freeRenderContext (tcx_renderContext *rc)
{
	if (rc->points.source) my_free(rc->points.source);
	if (rc->points.world) my_free(rc->points.world);
	
	if (rc->points.normalized) my_free(rc->points.normalized);
	if (rc->points.plotted) my_free(rc->points.plotted);
	if (rc->points.scaled) my_free(rc->points.scaled);
	if (rc->points.screen) my_free(rc->points.screen);
	my_free(rc);
}

static inline tcx_trackpoint *tcx_allocTrackpoint ()
{
	return my_calloc(1, sizeof(tcx_trackpoint));
}

static inline tcx_track *tcx_allocTrack ()
{
	return my_calloc(1, sizeof(tcx_track));
}

static inline tcx_lap *tcx_allocLap ()
{
	return my_calloc(1, sizeof(tcx_lap));
}

static inline tcx_activity *tcx_allocActivity ()
{
	return my_calloc(1, sizeof(tcx_activity));
}

static inline tcx_activities *tcx_allocActivities ()
{
	return my_calloc(1, sizeof(tcx_activities));
}

static inline tcx_trackpoint *tcx_createTrackpoint (tcx_track *track)
{
	if (!track->trackPoints.total)
		track->trackPoints.list = my_malloc(sizeof(tcx_trackpoint*));
	else
		track->trackPoints.list = my_realloc(track->trackPoints.list, (track->trackPoints.total+1)*sizeof(tcx_trackpoint*));
		
	track->trackPoints.list[track->trackPoints.total] = tcx_allocTrackpoint();
	return track->trackPoints.list[track->trackPoints.total++];
}

static inline tcx_track *tcx_createTrack (tcx_lap *lap)
{
	if (!lap->tracks.total)
		lap->tracks.list = my_malloc(sizeof(tcx_track*));
	else
		lap->tracks.list = my_realloc(lap->tracks.list, (lap->tracks.total+1)*sizeof(tcx_track*));
		
	lap->tracks.list[lap->tracks.total] = tcx_allocTrack();
	return lap->tracks.list[lap->tracks.total++];
}

static inline tcx_lap *tcx_createLap (tcx_activity *activity)
{
	if (!activity->laps.total)
		activity->laps.list = my_malloc(sizeof(tcx_lap*));
	else
		activity->laps.list = my_realloc(activity->laps.list, (activity->laps.total+1)*sizeof(tcx_lap*));
		
	activity->laps.list[activity->laps.total] = tcx_allocLap();
	return activity->laps.list[activity->laps.total++];
}

static inline tcx_activity *tcx_createActivity (tcx_activities *activities)
{
	if (!activities->activities.total)
		activities->activities.list = my_malloc(sizeof(tcx_activity*));
	else
		activities->activities.list = my_realloc(activities->activities.list, (activities->activities.total+1)*sizeof(tcx_activity*));

	activities->activities.list[activities->activities.total] = tcx_allocActivity();
	return activities->activities.list[activities->activities.total++];
}


static inline void tcx_freeTrackpoint (tcx_trackpoint *point)
{
	my_free(point);
}

static inline void tcx_freeTrack (tcx_track *track)
{
	if (track->trackPoints.total){
		while(track->trackPoints.total--)
			tcx_freeTrackpoint(track->trackPoints.list[track->trackPoints.total]);
		my_free(track->trackPoints.list);
	}
	my_free(track);
}

static inline void tcx_freeLap (tcx_lap *lap)
{
	if (lap->tracks.total){
		while(lap->tracks.total--)
			tcx_freeTrack(lap->tracks.list[lap->tracks.total]);
		my_free(lap->tracks.list);
	}
	my_free(lap);
}

static inline void tcx_freeActivity (tcx_activity *activity)
{
	if (activity->laps.total){
		while (activity->laps.total--)
			tcx_freeLap(activity->laps.list[activity->laps.total]);
		my_free(activity->laps.list);
	}
	my_free(activity);
}

static inline void tcx_freeActivities (tcx_activities *activities)
{
	//printf("tcx_freeActivities %p\n", activities);
	
	if (activities->activities.total){
		while (activities->activities.total--)
			tcx_freeActivity(activities->activities.list[activities->activities.total]);
		my_free(activities->activities.list);
	}
	my_free(activities);
}

int tcx_calcStats (tcx_activities *activities)
{
	if (!activities) return 0;
	
	int totalTrackPoints = 0;
	//printf("Activities: %i\n", activities->activities.total);
		
	activities->stats.latitude.min = 9999.0;
	activities->stats.latitude.max = -9999.0;
	activities->stats.longitude.min = 9999.0;
	activities->stats.longitude.max = -9999.0;
	activities->stats.trackPoints = 0;
	activities->stats.speed.ave = 0.0;
	activities->stats.speed.max = 0.0;
	activities->stats.speed.pts = 0;
	
	for (int a = 0; a < activities->activities.total; a++){
		tcx_activity *activity = activities->activities.list[a];
		//printf("Activity %i, Laps %i\n", a+1, activity->laps.total);
		
		double preDistance = 0.0;
		activity->stats.latitude.min = 9999.0;
		activity->stats.latitude.max = -9999.0;
		activity->stats.longitude.min = 9999.0;
		activity->stats.longitude.max = -9999.0;
		activity->stats.trackPoints = 0;
		activity->stats.speed.ave = 0.0;
		activity->stats.speed.max = 0.0;
		activity->stats.speed.pts = 0;
		
		memset(activity->cadence.hist, 0, ACTIVITY_CADENCE_HISTLEN * sizeof(int));
		activity->cadence.mode = 0;
		activity->cadence.totalPoints = 0;

		
		for (int l = 0; l < activity->laps.total; l++){
			tcx_lap *lap = activity->laps.list[l];
			//printf("Activity %i, Lap %i, Tracks: %i\n", a+1, l+1, lap->tracks.total);

			lap->stats.latitude.min = 9999.0;
			lap->stats.latitude.max = -9999.0;
			lap->stats.longitude.min = 9999.0;
			lap->stats.longitude.max = -9999.0;
			lap->stats.trackPoints = 0;
			lap->stats.speed.ave = 0.0;
			lap->stats.speed.max = 0.0;
			lap->stats.speed.pts = 0;
			int cad = 0;
			int cadPts = 0;
			int hr = 0;
			int hrPts = 0;

		
			for (int t = 0; t < lap->tracks.total; t++){
				tcx_track *track = lap->tracks.list[t];
				//printf("Activity %i, Lap %i, Track %i, Trackpoints %i\n", a+1, l+1, t+1, track->trackPoints.total);

				lap->stats.trackPoints += track->trackPoints.total;
				
				if (track->trackPoints.total){
					tcx_trackpoint *tp = track->trackPoints.list[0];
					track->time.start = tp->time.time64;
				}
				
				for (int p = 0; p < track->trackPoints.total; p++){
					tcx_trackpoint *tp = track->trackPoints.list[p];
#if 0
					printf("%i/%i/%i: longitude: %f\n", l, t, p, tp->longitude);
					printf("%i/%i/%i: altitude: %f\n", l, t, p, tp->altitude);
#endif
					if (tp->latitude != 0.0 && tp->longitude != 0.0){
						if (tp->latitude < lap->stats.latitude.min) lap->stats.latitude.min = tp->latitude;
						if (tp->latitude > lap->stats.latitude.max) lap->stats.latitude.max = tp->latitude;
						if (tp->longitude < lap->stats.longitude.min) lap->stats.longitude.min = tp->longitude;
						if (tp->longitude > lap->stats.longitude.max) lap->stats.longitude.max = tp->longitude;
					}
					
#define SPEED_CALC_POINTS		2	/* because this works _for me_ */

					// calc speed per trackpoint
					double preDist = 0.0;
					if ((p < SPEED_CALC_POINTS) || !track->trackPoints.list[p-SPEED_CALC_POINTS]->time.time64)
						preDist = preDistance;
					else
						preDist = track->trackPoints.list[p-SPEED_CALC_POINTS]->distance;
						
					if (preDist < 0.1) preDist = preDistance;
					
					if ((p >= SPEED_CALC_POINTS) && track->trackPoints.list[p-SPEED_CALC_POINTS]->time.time64){
						double distance = tp->distance - preDist;
						if (distance <= 0.001) distance = 0.0;
						double timeDelta = (tp->time.time64 - track->trackPoints.list[p-SPEED_CALC_POINTS]->time.time64);
						tp->speed = ((distance / timeDelta) * 60.0*60.0)/1000.0;
						
						if (tp->speed > lap->stats.speed.max)	
							lap->stats.speed.max = tp->speed;
							
						track->time.distance += distance;
						lap->stats.speed.pts++;
					}
					if (tp->distance > 0.1) preDistance = tp->distance;

					if (tp->cadence){
						activity->cadence.hist[tp->cadence]++;
						
						if (tp->cadence > lap->cadence.maximumCalculated)
							lap->cadence.maximumCalculated = tp->cadence;
						cad += tp->cadence;
						cadPts++;
					}
					if (tp->heartRate){
						hr += tp->heartRate;
						hrPts++;
					}

#if 0
					printf("time64: %I64d \n", tp->time.time64);
					printf("latitude: %f\n", tp->latitude);
					printf("longitude: %f\n", tp->longitude);
					printf("altitude: %f\n", tp->altitude);
					printf("distance: %f\n", tp->distance);
					printf("speed: %f\n", tp->speed);
					printf("cadence: %i\n", tp->cadence);
					printf("heartRate: %i\n\n", tp->heartRate);
#endif
				}
				if (track->trackPoints.total){
					tcx_trackpoint *tp = track->trackPoints.list[track->trackPoints.total-1];
					track->time.end = tp->time.time64;
					track->time.delta = (track->time.end - track->time.start)+1;
					
					lap->time.delta += track->time.delta;
				}
			}
			
			if (cadPts){
				activity->cadence.totalPoints += cadPts;
				lap->cadence.averageCalculated = cad / (double)cadPts;
			}
			if (hrPts)
				lap->heartRate.averageCalculated = hr / (double)hrPts;
				
			if (lap->stats.speed.pts){
				activity->stats.speed.pts += lap->stats.speed.pts;
				
				if (lap->stats.speed.max > activity->stats.speed.max)
					activity->stats.speed.max = lap->stats.speed.max;
			}
			
			if (lap->time.totalSeconds)
				lap->stats.speed.ave = ((lap->distance / lap->time.totalSeconds) * 60.0*60.0)/1000.0;
				
			activity->stats.trackPoints += lap->stats.trackPoints;
			
#if 0
			printf("lap %i: lap->heartRate.average %i\n", l+1, lap->heartRate.average);
			printf("lap %i: lap->heartRate.averageCalculated %i\n", l+1, lap->heartRate.averageCalculated);
			printf("lap %i: lap->distance %f\n", l+1, lap->distance);
			printf("lap %i: lap->time.totalSeconds %f\n", l+1, lap->time.totalSeconds);
			printf("lap %i: lap->time.delta %f\n", l+1, lap->time.delta);
			printf("lap %i: lap->cadence.average %i\n", l+1, lap->cadence.average);
			printf("lap %i: lap->cadence.averageCalculated %i\n", l+1, lap->cadence.averageCalculated);
			printf("lap %i: stats.speed.ave %f\n", l+1, lap->stats.speed.ave);
			printf("lap %i: stats.speed.max %f\n", l+1, lap->stats.speed.max);
			printf("lap %i: stats.latitude.min %f\n", l+1, lap->stats.latitude.min);
			printf("lap %i: stats.latitude.max %f\n", l+1, lap->stats.latitude.max);
			printf("lap %i: stats.longitude.min %f\n", l+1, lap->stats.longitude.min);
			printf("lap %i: stats.longitude.max %f\n\n", l+1, lap->stats.longitude.max);
#endif
			if (lap->stats.latitude.min < activity->stats.latitude.min) activity->stats.latitude.min = lap->stats.latitude.min;
			if (lap->stats.latitude.max > activity->stats.latitude.max) activity->stats.latitude.max = lap->stats.latitude.max;
			if (lap->stats.longitude.min < activity->stats.longitude.min) activity->stats.longitude.min = lap->stats.longitude.min;
			if (lap->stats.longitude.max > activity->stats.longitude.max) activity->stats.longitude.max = lap->stats.longitude.max;
			
			activity->distance += lap->distance;
			activity->time += lap->time.totalSeconds;
		}
		
		totalTrackPoints += activity->stats.trackPoints;

		if (activity->stats.speed.pts){
			activities->stats.speed.pts += activity->stats.speed.pts;
			
			if (activity->stats.speed.max > activities->stats.speed.max)
				activities->stats.speed.max = activity->stats.speed.max;
		}

		if (activity->time)
			activity->stats.speed.ave = ((activity->distance / activity->time) * 60.0*60.0)/1000.0;

		activities->stats.trackPoints += activity->stats.trackPoints;
		activities->stats.latitude.min = MIN(activities->stats.latitude.min, activity->stats.latitude.min);
		activities->stats.latitude.max = MAX(activities->stats.latitude.max, activity->stats.latitude.max);
		activities->stats.longitude.min = MIN(activities->stats.longitude.min, activity->stats.longitude.min);
		activities->stats.longitude.max = MAX(activities->stats.longitude.max, activity->stats.longitude.max);


		if (activity->cadence.totalPoints){
			int count = 0;
			for (int i = 0; i < ACTIVITY_CADENCE_HISTLEN; i++){
				if (activity->cadence.hist[i] >= count){
					count = activity->cadence.hist[i];
					activity->cadence.mode = i;
				}
			}
		}

#if 0
		printf("activity %i: activity->time %.0f\n", a+1, activity->time);
		printf("activity %i: activity->distance %.1fkm\n", a+1, activity->distance/1000.0);
		printf("activity %i: stats.speed.ave %f\n", a+1, activity->stats.speed.ave);
		printf("activity %i: stats.speed.max %f\n", a+1, activity->stats.speed.max);
		printf("activity %i: stats.latitude.min %f\n", a+1, activity->stats.latitude.min);
		printf("activity %i: stats.latitude.max %f\n", a+1, activity->stats.latitude.max);
		printf("activity %i: stats.longitude.min %f\n", a+1, activity->stats.longitude.min);
		printf("activity %i: stats.longitude.max %f\n\n", a+1, activity->stats.longitude.max);
		printf("activity %i: activity->cadence.mode %i\n", a+1, activity->cadence.mode);
#endif
	}
    
#if 0
	//printf("activities: stats.speed.ave %f\n", activities->stats.speed.ave);
	printf("activities: stats.speed.max %f\n", activities->stats.speed.max);
	printf("activities: stats.latitude.min %f\n", activities->stats.latitude.min);
	printf("activities: stats.latitude.max %f\n", activities->stats.latitude.max);
	printf("activities: stats.longitude.min %f\n", activities->stats.longitude.min);
	printf("activities: stats.longitude.max %f\n\n", activities->stats.longitude.max);
#endif
	

	//printf("tcx_calcStats: totalTrackPoints %i\n", totalTrackPoints);
	return totalTrackPoints;
}

static inline int tcx_normalizeActivity (tcx_renderContext *rc, tcx_activity *activity, const int activityNo, const double minLong, const double minLat, int *normalizedIdx)
{
	//double minLong = activity->stats.longitude.min;
	//double minLat = activity->stats.latitude.min;
	//int normalizedIdx = 0;

	for (int l = 0; l < activity->laps.total; l++){
		tcx_lap *lap = activity->laps.list[l];
	
		for (int t = 0; t < lap->tracks.total; t++){
			tcx_track *track = lap->tracks.list[t];

			for (int p = 0; p < track->trackPoints.total; p++){
				tcx_trackpoint *tp = track->trackPoints.list[p];
				
				if (tp->longitude != 0.0 && tp->latitude != 0.0){
					//printf("tcx_normalizeActivity %f %f\n", tp->latitude, tp->longitude);
					
					//rc->points.source[*normalizedIdx].longitude = tp->longitude;
					//rc->points.source[*normalizedIdx].latitude = tp->latitude;
					//rc->points.source[*normalizedIdx].data = ((uint64_t)activityNo<<48) | ((uint64_t)l<<32) | ((t/*&0xFFFF*/)<<16) | (p&0xFFFF);
					
					rc->points.normalized[*normalizedIdx].longitude = (tp->longitude - minLong) * rc->scaleLongZoom;
					rc->points.normalized[*normalizedIdx].latitude = 1.0 - ((tp->latitude - minLat) * rc->scaleLatZoom);
					rc->points.normalized[*normalizedIdx].data = ((uint64_t)activityNo<<48) | ((uint64_t)l<<32) | ((t/*&0xFFFF*/)<<16) | (p&0xFFFF);
					(*normalizedIdx)++;
				}
			}
		}
	}
	
	rc->points.total = *normalizedIdx;

	//printf("tcx_normalizeActivity: rc->points.total %i, %f %f\n", rc->points.total, minLong, minLat);
	return rc->points.total;
}

int tcx_normalize (tcx_renderContext *rc, tcx_activities *activities, const int activityNo, const double minLong, const double minLat)
{
	rc->points.total = 0;
	int normalizedIdx = 0;
	
	if (activityNo > 0){
		tcx_activity *activity = tcx_getActivity(activities, activityNo);
		return tcx_normalizeActivity(rc, activity, activityNo, minLong, minLat, &normalizedIdx);
	}else{
		int sum = 0;
		
		for (int a = 0; a < activities->activities.total; a++){
			tcx_activity *activity = activities->activities.list[a];
			sum += tcx_normalizeActivity(rc, activity, a, minLong, minLat, &normalizedIdx);
		}
		return sum;
	}
}

void tcx_scale (tcx_renderContext *rc, const double widthMultiplier, const double heightMultiplier)
{
	const double widthScale = rc->width * widthMultiplier;
	const double heightScale = rc->height * heightMultiplier;
	
	for (int i = 0; i < rc->points.total; i++){
		rc->points.scaled[i].longitude = rc->points.normalized[i].longitude * widthScale;
		rc->points.scaled[i].latitude = rc->points.normalized[i].latitude * heightScale;
		rc->points.scaled[i].data = rc->points.normalized[i].data;
	}
}

static inline int checkbounds (const int width, const int height, const int x, const int y)
{
	if (x < 0 || x >= width || y >= height || y < 0)
		return 0;
	else
		return 1;
}
/*
void tcx_screenSwap (tcx_renderContext *rc)
{
	if (rc->points.screen == rc->points.screens[0])
		rc->points.screen = rc->points.screens[1];
	else
		rc->points.screen = rc->points.screens[0];
}
*/
static inline tcx_screenPt *tcx_screenGetBack (tcx_renderContext *rc)
{
	return rc->points.screen;
/*
	tcx_screenPt *screen = rc->points.screen;
	if (screen == rc->points.screens[0])
		screen = rc->points.screens[1];
	else
		screen = rc->points.screens[0];
	return screen;*/
}

void tcx_screen (tcx_renderContext *rc, const double renderOffsetX, const double renderOffsetY)
{
	double offsetX, offsetY;
	double scale = ((rc->daspect * rc->aspect) / rc->scaleFactor);

	// center route
	if (rc->aspect <= rc->scaleFactor){
		scale *= rc->zoomFactor;
		offsetX = (rc->width - (rc->height * scale)) / 2.0;
		offsetY = (rc->height - (rc->height * rc->zoomFactor))/2.0;
	}else{
		scale /= rc->zoomFactor;
		offsetY = (rc->height - (rc->width / scale)) / 2.0;
		offsetX = (rc->width - (rc->width * rc->zoomFactor))/2.0;
	}

	offsetX += renderOffsetX;
	offsetY -= renderOffsetY;
	
	//printf("offsetX %f\n", offsetX);

	tcx_renderPt *scaled = rc->points.scaled;
	tcx_screenPt *pts = tcx_screenGetBack(rc);
	tcx_screenPt *plotted = rc->points.plotted;
	int plottedTotal = 0;
	int preX = -1;
	int preY = -1;
	//int ct = 0;
	
	for (int i = 0; i < rc->points.total; i++, scaled++, pts++){
		pts->data = scaled->data;
		pts->x = scaled->longitude + offsetX;
		pts->y = scaled->latitude  - offsetY;
		pts->isInside = checkbounds(rc->width, rc->height, pts->x, pts->y);
		
		if (pts->isInside){
			if (!(preX == pts->x && preY == pts->y)){
				preX = pts->x;
				preY = pts->y;
				
				*plotted = *pts;
				plotted++;
				plottedTotal++;
			//}else{
			//	ct++;
			}
		}
	}

	rc->startX = offsetX;
	rc->startY = offsetY;

	//printf("ct %i\n", ct);
	rc->points.plottedTotal = plottedTotal;
	//rc->points.screenModified++;
}

double tcx_calcScaleMultipliers (tcx_renderContext *rc)
{
	if (rc->aspect <= rc->scaleFactor){
		rc->aspectFactor = rc->aspect/rc->scaleFactor;
		rc->scaleLong = rc->aspectFactor / rc->dLong;
		rc->scaleLat = 1.0/rc->dLat;
			
		//printf("long factor: %f\n", factor);
		
	}else{
		rc->aspectFactor = 1.0/(rc->aspect/rc->scaleFactor);
		rc->scaleLat = rc->aspectFactor / rc->dLat;
		rc->scaleLong = 1.0 / rc->dLong;
						
		//printf("lat factor: %f\n", factor);
	}	

	//printf("calcNormalization %f %f %f\n", rc->aspectFactor, rc->scaleLat, rc->scaleLong);	
	
	return rc->aspectFactor;
}
	
void tcx_zoom (tcx_renderContext *rc, const double factor)
{
	rc->zoomFactor = factor;
	rc->scaleLatZoom = rc->scaleLat * rc->zoomFactor;
	rc->scaleLongZoom = rc->scaleLong * rc->zoomFactor;
}

static inline mxml_type_t mxml_cb (mxml_node_t *node)
{
	const char *name = node->value.element.name;
	//static int count = 0;
	
	//printf("mxml_cb %i: '%s'\n", count++, name);


	if (!strcmp(name, "Trackpoint")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Position")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "HeartRateBpm")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Track")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Time")){
		return MXML_TEXT;
	}else if (!strcmp(name, "DistanceMeters")){
		return MXML_REAL;
	}else if (!strcmp(name, "AltitudeMeters")){
		return MXML_REAL;
	}else if (!strcmp(name, "LongitudeDegrees")){
		return MXML_REAL;
	}else if (!strcmp(name, "LatitudeDegrees")){
		return MXML_REAL;
	}else if (!strcmp(name, "Extensions")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "SensorState")){
		return MXML_OPAQUE;
	}else if (!strcmp(name, "Cadence")){
		return MXML_INTEGER;
	}else if (!strcmp(name, "Lap")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Value")){
		return MXML_INTEGER;
	}else if (!strcmp(name, "Extensions")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "LX")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "TPX")){
		return MXML_ELEMENT;

	}else if (!strcmp(name, "MaxBikeCadence")){
		return MXML_INTEGER;	
				
	}else if (!strcmp(name, "TotalTimeSeconds")){
		return MXML_REAL;
	}else if (!strcmp(name, "MaximumSpeed")){
		return MXML_REAL;
	}else if (!strcmp(name, "AvgSpeed")){
		return MXML_REAL;	

	}else if (!strcmp(name, "AverageHeartRateBpm")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Activity")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Activities")){
		return MXML_ELEMENT;

	
	}else if (!strcmp(name, "Course")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Courses")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Folder")){
		return MXML_ELEMENT;
			
	}else if (!strcmp(name, "Id")){
		return MXML_TEXT;

	}else if (!strcmp(name, "Name")){
		return MXML_TEXT;
		
#if 0

	}else if (!strcmp(name, "MaximumHeartRateBpm")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Intensity")){
		return MXML_OPAQUE;	
	}else if (!strcmp(name, "TriggerMethod")){
		return MXML_OPAQUE;
	}else if (!strcmp(name, "Calories")){
		return MXML_INTEGER;
	}else if (!strcmp(name, "ProductID")){
		return MXML_INTEGER;	
	}else if (!strcmp(name, "UnitId")){
		return MXML_INTEGER;				
	}else if (!strcmp(name, "VersionMajor")){
		return MXML_OPAQUE;
	}else if (!strcmp(name, "VersionMinor")){
		return MXML_OPAQUE;
	}else if (!strcmp(name, "BuildMajor")){
		return MXML_OPAQUE;
	}else if (!strcmp(name, "BuildMinor")){
		return MXML_OPAQUE;
	}else if (!strcmp(name, "Type")){
		return MXML_OPAQUE;
	}else if (!strcmp(name, "Builder")){
		return MXML_OPAQUE;
	}else if (!strcmp(name, "LangID")){
		return MXML_OPAQUE;
	}else if (!strcmp(name, "PartNumber")){
		return MXML_OPAQUE;
		
	}else if (!strcmp(name, "TrainingCenterDatabase")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Folder")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "ActivityRef")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Build")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Version")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Author")){
		return MXML_ELEMENT;
	}else if (!strcmp(name, "Creator")){
		return MXML_ELEMENT;
#endif
	}

	//printf("IGNORING:'%s'\n", node->value.element.name);
    return MXML_IGNORE;
}

#if 0
static inline void dump (mxml_node_t *node)
{
	if (!node) return;


	if (node->type == MXML_TEXT)
		printf("TEXT: '%s'\n", node->value.text.string);
		
	else if (node->type == MXML_OPAQUE)
		printf("OPAQUE: '%s'\n", node->value.opaque);
		
	else if (node->type == MXML_REAL)
		printf("REAL: %.6f\n", node->value.real);
		
	else if (node->type == MXML_INTEGER)
		printf("INTEGER: %I64d\n", node->value.integer);
		
	else if (node->type == MXML_ELEMENT && node->value.element.num_attrs == 1)
		printf("ELEMENT (%i): '%s' '%s' '%s'\n", node->value.element.num_attrs, node->value.element.name, node->value.element.attrs[0].name, node->value.element.attrs[0].value);
	
	else if (node->type == MXML_ELEMENT && node->value.element.num_attrs >= 2)
		printf("ELEMENT (%i): '%s' '%s' '%s'\n", node->value.element.num_attrs, node->value.element.name, node->value.element.attrs[1].name, node->value.element.attrs[1].value);

	else if (node->type == MXML_ELEMENT)
		printf("ELEMENT: '%s'\n", node->value.element.name);
}
#endif




static inline uint64_t tmTo64 (tcx_time *t)
{
	int days = clkDaysInMonth(t->year, t->mon);
	return (t->mon * 60 * 60 * 24 * days) + (t->mday * 60 * 60 * 24) + (t->hour * 60 * 60) + (t->min * 60) + t->sec;
}


// not adjusted for timezone
static inline int timeToTime (const char *str, tcx_time *t)
{
	if (strlen(str) < 20) return 0;
	
	//"2015-06-13T13:05:59Z"
	
	t->year = atoi(str);
	t->mon = atoi(&str[5]);
	t->mday = atoi(&str[8]);
	t->hour = atoi(&str[11]);
	t->min = atoi(&str[14]);
	t->sec = atoi(&str[17]);
	return 1;
}

int tcx_timeStrToTime (const char *str, tcx_time *t)
{
	return timeToTime(str, t);
}

uint64_t tcx_timeTo64 (tcx_time *t)
{
	return tmTo64(t);
}

// 10692
//'2015-06-28T11:37:06Z' 2461026

static inline void time64ToTm (uint64_t t64, tcx_time *t)
{
	struct tm *tm = _localtime64((__time64_t*)&t64);
	if (tm){
		t->year = tm->tm_year;
		t->mon = tm->tm_mon;
		t->mday = tm->tm_mday;
		t->hour = tm->tm_hour;
		t->min = tm->tm_min;
		t->sec = tm->tm_sec;
	}else{
		t->mday = t64 / 60 / 60 / 24;
		t->hour = (t64 / 60 / 60) % 24;
		double val = 24.0;
		t->min = modf(t64 / 60.0 / 60.0, &val) * 60.0;
		t->sec = t64 % 60;
	}
	
	//printf("time64ToTm %I64d %i %i %i\n", t64, t->hour, t->min, t->sec);
	//printf("Time: %i %i %i:%.2i:%.2i, %i\n", tm->tm_year, tm->tm_mon, tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_isdst);
	
}

void tcx_time64ToTime (uint64_t t64, tcx_time *t)
{
	time64ToTm(t64, t);
}


static inline mxml_node_t *tcx_parseTrackpointPosition (mxml_node_t *parent, mxml_node_t *topNode, tcx_track *track, tcx_trackpoint *trackpoint)
{
	//printf("tcx_parseTrackpointPosition %p\n", parent);
	
	mxml_node_t *node = mxmlWalkNext(parent, topNode, MXML_DESCEND);

	while (node){
		if (node->type == MXML_ELEMENT){
			const char *name = node->value.element.name;

			if (!strcmp(name, "LatitudeDegrees")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_REAL) continue;

				trackpoint->latitude =  node->value.real;

			}else if (!strcmp(name, "LongitudeDegrees")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_REAL) continue;
				
				trackpoint->longitude =  node->value.real;
			}
		}
		
		node = mxmlWalkNext(node, topNode, MXML_NO_DESCEND);
		if (node){
			if (node->parent != parent){
				return mxmlWalkPrev(node, topNode, MXML_DESCEND);
			}
		}
	};

	return node;
}			

static inline mxml_node_t *tcx_parseTrackpoint (mxml_node_t *parent, mxml_node_t *topNode, tcx_track *track, tcx_trackpoint *trackpoint)
{
	//printf("tcx_parseTrackpoint %p\n", parent);
	mxml_node_t *node = mxmlWalkNext(parent, topNode, MXML_DESCEND);

	while (node){
		if (node->type == MXML_ELEMENT){
			const char *name = node->value.element.name;
			
			
			if (!strcmp(name, "Position")){
				node = tcx_parseTrackpointPosition(node, topNode, track, trackpoint);
	
			}else if (!strcmp(name, "AltitudeMeters")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_REAL) continue;
				
				trackpoint->altitude =  node->value.real;
				
			}else if (!strcmp(name, "DistanceMeters")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_REAL) continue;
				
				trackpoint->distance =  node->value.real;
				
			}else if (!strcmp(name, "Cadence")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_INTEGER) continue;

				trackpoint->cadence =  node->value.integer;
				
			}else if (!strcmp(name, "HeartRateBpm")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_ELEMENT) continue;
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);	// <Value>
				if (node->type != MXML_INTEGER) continue;
				
				trackpoint->heartRate =  node->value.integer;

			}else if (!strcmp(name, "Time")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_TEXT) continue;

				timeToTime(node->value.text.string, &trackpoint->time.time);
				trackpoint->time.time64 = tmTo64(&trackpoint->time.time);
				
				//printf("'%s' %I64d\n", node->value.text.string, trackpoint->time.time64);
			}
		}

		node = mxmlWalkNext(node, topNode, MXML_NO_DESCEND);
		if (node && node->parent != parent)
			return mxmlWalkPrev(node, topNode, MXML_DESCEND);

	};

	return node;
}

static inline mxml_node_t *tcx_parseTrack (mxml_node_t *parent, mxml_node_t *topNode, tcx_lap *lap, tcx_track *track)
{
	
	//printf("tcx_parseTrack %p\n", parent);
	mxml_node_t *node = mxmlWalkNext(parent, topNode, MXML_DESCEND);
	
	while (node){
		if (node->type == MXML_ELEMENT){
			const char *name = node->value.element.name;

			if (!strcmp(name, "Trackpoint"))
				node = tcx_parseTrackpoint(node, topNode, track, tcx_createTrackpoint(track));
		}

		node = mxmlWalkNext(node, topNode, MXML_NO_DESCEND);
		if (node && node->parent != parent)
			return mxmlWalkPrev(node, topNode, MXML_DESCEND);
	};

	return node;
}

static inline mxml_node_t *tcx_parseLapExtensions (mxml_node_t *parent, mxml_node_t *topNode, tcx_activity *activity, tcx_lap *lap)
{
	//printf("tcx_parseLapExtensions %p\n", parent);
	mxml_node_t *node = mxmlWalkNext(parent, topNode, MXML_DESCEND);


	while (node){
		if (node->type == MXML_ELEMENT){
			char *name = node->value.element.name;

			if (!strcmp(name, "AvgSpeed")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_REAL) continue;

				lap->speed.averageReported = node->value.real;

			}else if (!strcmp(name, "MaxBikeCadence")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_INTEGER) continue;

				lap->cadence.maximumReported = node->value.integer;
			}
		}

		node = mxmlWalkNext(node, topNode, MXML_NO_DESCEND);
		if (node && node->parent != parent)
			return mxmlWalkPrev(node, topNode, MXML_DESCEND);
	};
	
	return node;
}

static inline mxml_node_t *tcx_parseLap (mxml_node_t *parent, mxml_node_t *topNode, tcx_activity *activity, tcx_lap *lap)
{
	
	//printf("tcx_parseLap %p\n", parent);

	if (parent->value.element.num_attrs){
		timeToTime(parent->value.element.attrs[0].value, &lap->time.start);
		lap->time.start64 = tmTo64(&lap->time.start);
		//printf("lap starttime: '%s'\n", parent->value.element.attrs[0].value);
	}
	
	mxml_node_t *node = mxmlWalkNext(parent, topNode, MXML_DESCEND);
	
	while (node){
		if (node->type == MXML_ELEMENT){
			const char *name = node->value.element.name;
			
			if (!strcmp(name, "TotalTimeSeconds")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_REAL) continue;
				
				lap->time.totalSeconds = node->value.real;
				
			}else if (!strcmp(name, "DistanceMeters")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_REAL) continue;
				
				lap->distance = node->value.real;
				
			}else if (!strcmp(name, "MaximumSpeed")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_REAL) continue;
				
				lap->speed.maximum = node->value.real;
				
			}else if (!strcmp(name, "Calories")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_INTEGER) continue;
				
				lap->calories = node->value.integer;
				
			}else if (!strcmp(name, "AverageHeartRateBpm")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_ELEMENT) continue;
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_INTEGER) continue;
				
				lap->heartRate.average = node->value.integer;
				
			}else if (!strcmp(name, "Cadence")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_INTEGER) continue;
			
				lap->cadence.average = node->value.integer;

			}else if (!strcmp(name, "Track")){
				node = tcx_parseTrack(node, topNode, lap, tcx_createTrack(lap));
				
			}else if (!strcmp(name, "Extensions")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				node = tcx_parseLapExtensions(node, topNode, activity, lap);
			}
		}

		node = mxmlWalkNext(node, topNode, MXML_NO_DESCEND);
		if (node && node->parent != parent)
			return mxmlWalkPrev(node, topNode, MXML_DESCEND);
	};

	return node;
}

static inline mxml_node_t *tcx_parseActivity (mxml_node_t *parent, mxml_node_t *topNode, tcx_activities *activities, tcx_activity *activity)
{
	//printf("tcx_parseActivity %p\n", parent);
	
	if (parent->value.element.num_attrs){
		strncpy(activity->name, parent->value.element.attrs[0].value, sizeof(activity->name)-1);
		//printf("activity name: '%s'\n", activity->name);
	}
	
	mxml_node_t *node = mxmlWalkNext(parent, topNode, MXML_DESCEND);
	
	while (node){
		if (node->type == MXML_ELEMENT){
			const char *name = node->value.element.name;
			
			if (!strcmp(name, "Id")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_TEXT) continue;

				strncpy(activity->id, node->value.text.string, sizeof(activity->id)-1);
				//printf("id: '%s'\n", activity->id);
				tcx_time actTime = {0};
				timeToTime(activity->id, &actTime);
				activity->startTime64 = tmTo64(&actTime);

			}else if (!strcmp(name, "Lap")){
				node = tcx_parseLap(node, topNode, activity, tcx_createLap(activity));
			}
		}
		
		node = mxmlWalkNext(node, topNode, MXML_NO_DESCEND);
		if (node && node->parent != parent)
			return mxmlWalkPrev(node, topNode, MXML_DESCEND);
	};

	return node;
}

static inline mxml_node_t *tcx_parseCourse (mxml_node_t *parent, mxml_node_t *topNode, tcx_activities *activities, tcx_activity *activity)
{
	//printf("tcx_parseCourse %p\n", parent);
	
	if (parent->value.element.num_attrs){
		strncpy(activity->name, parent->value.element.attrs[0].value, sizeof(activity->name)-1);
		//printf("activity name: '%s'\n", activity->name);
	}
	
	mxml_node_t *node = mxmlWalkNext(parent, topNode, MXML_DESCEND);
	
	while (node){
		if (node->type == MXML_ELEMENT){
			const char *name = node->value.element.name;
			//printf("node %s\n", name);
			
			if (!strcmp(name, "Name")){
				node = mxmlWalkNext(node, topNode, MXML_DESCEND);
				if (node->type != MXML_TEXT) continue;

				strncpy(activity->id, node->value.text.string, sizeof(activity->id)-1);
				//printf("Course name: '%s'\n", activity->id);
				activity->startTime64 = 0;
				
			}else if (!strcmp(name, "Track")){
				tcx_lap *lap;
				if (activity->laps.total)
					lap = activity->laps.list[0];
				else
					lap = tcx_createLap(activity);
					
				node = tcx_parseTrack(node, topNode, lap, tcx_createTrack(lap));
				
			}else if (!strcmp(name, "Lap")){
				node = tcx_parseLap(node, topNode, activity, tcx_createLap(activity));
			}
		}
		
		node = mxmlWalkNext(node, topNode, MXML_DESCEND);
	};

	return node;
}

int tcx_load (const wchar_t *tcxFile, mxml_node_t **node, mxml_node_t **topNode)
{
	FILE *fp = _wfopen(tcxFile, L"rb");
	if (!fp) return 0;

	*topNode = mxmlNewXML(NULL);
	if (*topNode)
		*node = mxmlLoadFile(*topNode, fp, mxml_cb);
	
	fclose(fp);

	return *node != NULL;
}

tcx_activities *tcx_parse (mxml_node_t *node, mxml_node_t *topNode)
{
	tcx_activities *activities = NULL;

	while (node){
		//dump(node);
		
		if (node->type == MXML_ELEMENT){
			const char *name = node->value.element.name;

			if (!strcmp(name, "Activities")){
				if (!activities)
				 	activities = tcx_allocActivities();
				 
			}else if (!strcmp(name, "Activity")){
				tcx_activity *act = tcx_createActivity(activities);
				act->type = ACTIVITY_TYPE_ACTIVITY;
				node = tcx_parseActivity(node, topNode, activities, act);
			
			
			}else if (!strcmp(name, "Courses")){
				if (!activities)
				 	activities = tcx_allocActivities();

			}else if (!strcmp(name, "Course")){
				tcx_activity *act = tcx_createActivity(activities);
				act->type = ACTIVITY_TYPE_COURSE;
				node = tcx_parseCourse(node, topNode, activities, act);
			}
		}
		
		node = mxmlWalkNext(node, topNode, MXML_DESCEND);
	};
	
	return activities;
}

tcx_activities *tcx_openW (const wchar_t *tcxFile)
{
	mxml_node_t *node = NULL;
	mxml_node_t *topNode = NULL;
	
	if (tcx_load(tcxFile, &node, &topNode)){
		tcx_activities *activities = tcx_parse(node, topNode);
		mxmlDelete(topNode);
		return activities;
	}
	
	return NULL;
}

tcx_activities *tcx_open8 (const char *tcxFile)
{
	tcx_activities *activities = NULL;
	wchar_t *path = converttow(tcxFile);
	if (path){
		activities = tcx_openW(path);
		my_free(path);
	}
	
	return activities;
}

void tcx_close (tcx_activities *activities)
{
	tcx_freeActivities(activities);
}

void tcx_closeRenderContext (tcx_renderContext *rc)
{
	tcx_freeRenderContext(rc);
}
