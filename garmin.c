
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


#if ENABLE_GARMINTCX



extern volatile double UPDATERATE_BASE;


#define WORLD_ZOOM 		12



#ifdef drawImage
#undef drawImage
#undef drawImageNoBlend
#endif

#define drawImage(s,d,x,y) copyArea(s,d,x,y,0,0,s->width-1,d->height-1)
#define drawImageNoBlend(s,d,x,y) copyAreaNoBlend(s,d,x,y,0,0,s->width-1,d->height-1)


#define SLIDE_RELEASED					0
#define SLIDE_HELD						1
#define SLIDE_SLIDING					2
#define	CUBESIZE						48
#define CURSORMARK_COLOUR				(255<<24|0xA1FFA1)

/*
static TLPOINTEX window;
static tcx_map map;
static int windowSet;*/

typedef struct {
	TTCX *tcx;
	char *file;
}TCXTHREADOPAQUE;


// http://www.google.co.uk/maps/@54.481282,-6.225284,16z
// http://maps.googleapis.com/maps/api/staticmap?center=54.615684,-5.937209&zoom=13&size=256x256&sensor=false
// http://maps.googleapis.com/maps/api/staticmap?center=54.611053,-5.926842&zoom=15&size=512x288&scale=2
// http://maps.googleapis.com/maps/api/staticmap?visible=54.611053,-5.926842|54.643003,-5.935705&size=512x512
// http://maps.googleapis.com/maps/api/staticmap?center=54.618777,-5.938356&zoom=18&size=256x256

static const char *mapApplication = "C:\\Program Files (x86)\\Firefox\\firefox.exe";
//static const char *mapURL = "%s http://www.openstreetmap.org/#map=%.1f/%f/%f&layers=C";
// http://tile.openstreetmap.org/zoom/x/y.png"
 static const char *mapURL = "%s http://www.openstreetmap.org/?mlat=%f&mlon=%f#map=%.1f/%f/%f";//&layers=C";
//static const char *mapURL = "%s http://qa.poole.ch/?zoom=%.1f&lat=%f&lon=%f&layers=FFFF0B";
//static const char *mapURL = "%s http://www.google.co.uk/maps/place/%f+%f/@%f,%f,%.1fz?force=lite";


static const char *infoURL = "http://maps.googleapis.com/maps/api/geocode/json?latlng=%f,%f&sensor=true";



// find closest address to this location
static inline char *geoCodeReserveLookup (tcx_point *pt)
{
	
	char buffer[MAX_PATH_UTF8];
	__mingw_snprintf(buffer, MAX_PATH_UTF8, infoURL, pt->latitude, pt->longitude);


	size_t totalRead = 0;
	char *json = getUrl(buffer, &totalRead);
	if (!json) return NULL;

	//printf("##%s##\n", json);

	char *ret = NULL;
	if (totalRead > 64)
		ret = jsonGetTag(json, "formatted_address");

	my_free(json);
	return ret;
}

#if 0
static inline float calcCourse (vector2_t *_1,  vector2_t *_2)
{
	if (fabs(_1->x - _2->x) < 1e-6f && fabs(_1->y - _2->y) < 1e-6f)
		return 0.0f;
	else if (fabs(_1->y - 90.0f) < 1e-5f) // Starting from N pole
		return 180.0f;
	else if (fabs(_1->y + 90.0f) < 1e-5f) // Starting from S pole
		return 0.0f;

	const float fX1 = _1->x * M_PI/180.0f;
	const float fY1 = _1->y * M_PI/180.0f;
	const float fX2 = _2->x * M_PI/180.0f;
	const float fY2 = _2->y * M_PI/180.0f;

	const double fCosY2 = cosf(fY2);

	float fCourse = 180.0f/M_PI * atan2f(
									sinf(fX2 - fX1)*fCosY2,
									cosf(fY1)*sinf(fY2) - sinf(fY1)*fCosY2*cosf(fX2 - fX1)
								  );

	if (fCourse < 0.0f) fCourse += 360.0f;

	return fCourse;
}
#endif

static inline double calcDistM (double lat1, double lon1, double lat2, double lon2)
{
	
	//printf("%f %f %f %f\n", lat1, lon1, lat2, lon2);
	
	const double R = 6378137.0;		// Earths radius
	const double pi80 = M_PI / 180.0;

	//double d = acos( sin(lat1*M_PI/180.0)*sin(lat2*M_PI/180.0) + cos(lat1*M_PI/180.0)*cos(lat2*M_PI/180.0)*cos(lon2*M_PI /180.0-lon1*M_PI/180.0) ) * R;
	//printf("dist b; %.0f\n", d);
		
	lat1 *= pi80;
	lon1 *= pi80;
	lat2 *= pi80;
	lon2 *= pi80;
	double dlat = fabs(lat2 - lat1);
	double dlon = fabs(lon2 - lon1);
	double a = sin(dlat / 2.0) * sin(dlat / 2.0) + cos(lat1) * cos(lat2) * sin(dlon /2.0) * sin(dlon / 2.0);
	double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
	double d = R * c;
	//printf("dist %.0f\n", d);

	return d;
}

static inline double calcDistKmTrkPt (const tcx_trackpoint *pt1, const tcx_trackpoint *pt2)
{
	const double lat1 = pt1->latitude;
	const double lon1 = pt1->longitude;
	const double lat2 = pt2->latitude;
	const double lon2 = pt2->longitude;

	//double d = acos( sin(lat1*PI/180.0)*sin(lat2*PI/180.0) + cos(lat1*PI/180.0)*cos(lat2*PI/180.0)*cos(lon2*PI /180.0-lon1*PI/180.0) ) * R;
	//printf("dist %.0f\n", d);
	
	return calcDistM(lat1, lon1, lat2, lon2) / 1000.0;
}

static inline uint64_t calcDistPx (tcx_screenPt *pts, const int x, const int y)
{
	uint64_t dx = abs(pts->x - x)+1;
	uint64_t dy = abs(pts->y - y)+1;
	uint64_t area = (dx * dx) + (dy * dy);
	
	return area;
}

// lat/long to position on screen
static inline void routeLocationToScreen (TTCX *tcx, const double longitude, const double latitude, int *x, int *y)
{
	tcx_renderContext *rc = tcx->renderContext;
	
	// normalize
	double h = (longitude - rc->minLong) * rc->scaleLongZoom;
	double v = 1.0 - ((latitude - rc->minLat) * rc->scaleLatZoom);

	// scale to screen
	*x = (rc->width * h) + rc->startX;
	*y = (rc->height * v) - rc->startY;
}

// on screen cursor position to lat/long
static inline void routeScreenToLocation (TTCX *tcx, int x, int y, double *longitude, double *latitude)
{
	tcx_renderContext *rc = tcx->renderContext;
	
	x -= rc->startX;
	double pos = (1.0 / (double)rc->width) * x;
	pos /= rc->scaleLongZoom;
	*longitude = rc->dLong * pos;
	*longitude *= rc->scaleLongZoom;
	*longitude /= tcx->route.scale.factor;

	y = (rc->height - y) - rc->startY;
	pos = (1.0 / (double)rc->height) * y;
	pos /= rc->scaleLatZoom;
	*latitude = rc->dLat * pos;
	*latitude *= rc->scaleLatZoom;
	*latitude /= tcx->route.scale.factor;

	if (rc->aspect <= rc->scaleFactor)
		*longitude /= rc->aspectFactor;
	else
		*latitude /= rc->aspectFactor;

	*longitude += rc->minLong;
	*latitude += rc->minLat;
}

static inline void locationToWorldGrid (const int zoom, const double latitude, const double longitude, int *x, int *y)
{
	*x = (int)(256.0 * ((longitude + 180.0) / 360.0 * (1<<zoom)));
	*y = (int)(256.0 * ((1.0 - log(tan(latitude * M_PI / 180.0) +  1.0 / cos(latitude * M_PI / 180.0)) / M_PI) / 2.0 * (1<<zoom)));
}

static inline void worldGridToLocation (const int zoom, int x, int y, double *latitude, double *longitude)
{
	const double n = M_PI - ((2.0 * M_PI * y / 256.0) / pow(2.0, zoom));
	*latitude = (double)(180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n))));
	*longitude = (double)(((x / 256.0) / pow(2.0, zoom) * 360.0) - 180.0);
}

static inline void locationToImageGrid (const int zoom, const double latitude, const double longitude, int *x, int *y)
{
	*x = (int)(/*256.0 * */((longitude + 180.0) / 360.0 * (1<<zoom)));
	*y = (int)(/*256.0 * */((1.0 - log(tan(latitude * M_PI / 180.0) +  1.0 / cos(latitude * M_PI / 180.0)) / M_PI) / 2.0 * (1<<zoom)));
}

static inline void imageGridToLocation (const int zoom, int x, int y, double *latitude, double *longitude)
{
	const double n = M_PI - ((2.0 * M_PI * y /*/ 256.0*/) / pow(2.0, zoom));
	*latitude = (double)(180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n))));
	*longitude = (double)(((x /*/ 256.0*/) / pow(2.0, zoom) * 360.0) - 180.0);
}

static inline void calcMapWindow (TTCX *tcx, const tcx_map *mapDesc, TLPOINTEX *window)
{
	tcx_renderContext *rc = tcx->renderContext;

	double leftLong = mapDesc->nw.longitude;
	double topLat = mapDesc->nw.latitude;
	double rightLong = mapDesc->se.longitude;
	double btmLat = mapDesc->se.latitude;
	
	double dLong = fabs(leftLong - rightLong);
	double metersPerPixel = calcDistM(0.0, 0.0, 0.0, dLong) / (double)mapDesc->width;
	double leftInner = calcDistM(0.0, leftLong, 0.0, rc->minLong);
	window->x1 = leftInner / metersPerPixel;
	double rightInner = calcDistM(0.0, leftLong, 0.0, rc->maxLong);
	window->x2 = rightInner / metersPerPixel;

	double dLat = fabs(topLat - btmLat);
	metersPerPixel = calcDistM(0.0, 0.0, dLat, 0.0) / (double)mapDesc->height;
	double topInner = calcDistM(topLat, 0.0, rc->maxLat, 0.0);
	window->y1 = topInner / metersPerPixel;
	double btmInner = calcDistM(topLat, 0.0, rc->minLat, 0.0);
	window->y2 = btmInner / metersPerPixel;

	//printf("box %i,%i %i,%i\n", window->x1, window->y1, window->x2, window->y2);
}


static inline void calcWorldPositions (TTCX *tcx, tcx_renderContext *rc, const int zoom, int offsetX, int offsetY)
{
	//printf("calcWorldPositions %i %i %i\n", zoom, offsetX, offsetY);
	
	tcx_renderPt *src = rc->points.source;
	tcx_screenPt *pts = rc->points.world;
	const int total = rc->points.total;
	
	
	int leftGrid, topGrid;
	int rightGrid, btnGrid;
	locationToWorldGrid(zoom, rc->maxLat, rc->minLong, &leftGrid, &topGrid);
	locationToWorldGrid(zoom, rc->minLat, rc->maxLong, &rightGrid, &btnGrid);

	int leftOffset = leftGrid % 256;
	int topOffset = topGrid % 256;
	
	
	/*const int x1 = leftGrid / 256;
	const int y1 = topGrid / 256;
	const int x2 = rightGrid / 256;
	const int y2 = btnGrid / 256;*/
	
	//printf("map grid box: %i %i %i %i\n", x1, y1, x2, y2);

	int minX = ((((rightGrid / 256) - (leftGrid/256)) * 256) + (rightGrid % 256)) - leftOffset;
	int minY = ((((btnGrid / 256) - (topGrid/256)) * 256) + (btnGrid % 256)) - topOffset;
	
	int ox = (rc->width - minX) / 2;
	int oy = (rc->height - minY) / 2;

	rc->map.offsetX = leftOffset - (ox + offsetX);
	rc->map.offsetY = topOffset - (oy + offsetY);

	leftGrid /= 256;
	topGrid /= 256;

	int x, y;
	
	for (int i = 0; i < total; i++, src++, pts++){
		locationToWorldGrid(zoom, src->latitude, src->longitude, &x, &y);
		
		pts->x = (((x / 256) - leftGrid) * 256) + (x % 256);
		pts->x -= rc->map.offsetX;
		
		pts->y = (((y / 256) - topGrid) * 256) + (y % 256);
		pts->y -= rc->map.offsetY;
	}
}

static inline void infoAddItem (TTCX *tcx, const char *line)
{
	paneTextAdd(tcx->info.pane, 0, 1.0, line, tcx->info.font, 0);
}

static inline void infoAddItemEx (TTCX *tcx, const char *fmt, ...)
{
	char buffer[MAX_PATH_UTF8+1];
	
	VA_OPEN(ap, fmt);
	__mingw_vsnprintf(buffer, MAX_PATH_UTF8, fmt, ap);
	VA_CLOSE(ap);
	
	infoAddItem(tcx, buffer);
}

static inline int lap2Colour (TTCX *tcx, const int lap)
{
	const int *colours = tcx->route.render.colours.list;
	const int coloursTotal = tcx->route.render.colours.total;
	return colours[lap%coloursTotal];
}

static inline int getCursorX (TGRAPH *graph)
{
	return graph->cc->cursor->dx;
}

static inline int getCursorY (TGRAPH *graph)
{
	return graph->cc->cursor->dy;
}

static inline int getCursorXAdjusted (TGRAPH *graph)
{
	return getCursorX(graph) /*+ MOFFSETX - 23*/;
}

static inline int getCursorYAdjusted (TGRAPH *graph)
{
	return getCursorY(graph) /*- MOFFSETY + 9*/;
}

static inline tcx_trackpoint *routeGetTrkPt (tcx_activities *activities, const int activity, const int lap, const int track, const int pt)
{
	tcx_trackpoint *tpt = NULL;
	
	tcx_activity *a = tcx_getActivity(activities, activity+1);
	if (a){
		tcx_lap *l = a->laps.list[lap];
		if (l)
			tpt = l->tracks.list[track]->trackPoints.list[pt];
	}
	return tpt;
}

static inline uint64_t routeGetData (tcx_renderContext *rc, const int idx)
{
	return rc->points.screen[idx].data;
}

static inline uint64_t routeGetDataP (tcx_renderContext *rc, const int idx)
{
	return rc->points.plotted[idx].data;
}

static inline void routeGetTrkPtDataP (tcx_renderContext *rc, int idx, int *activity, int *lap, int *track, int *pt)
{
	if (idx > rc->points.plottedTotal-1) idx = rc->points.plottedTotal-1;
	
	const uint64_t data = routeGetDataP(rc, idx);
	*activity = (data>>48)&0xFFFF; 
	*lap = (data>>32)&0xFFFF;
	*track = (data>>16)&0xFFFF;
	*pt = data&0xFFFF;
}

// decompose trackpoint data
static inline void routeGetTrkPtData (tcx_renderContext *rc, int idx, int *activity, int *lap, int *track, int *pt)
{
	if (idx > rc->points.total-1) idx = rc->points.total-1;
	
	const uint64_t data = routeGetData(rc, idx);
	*activity = (data>>48)&0xFFFF; 
	*lap = (data>>32)&0xFFFF;
	*track = (data>>16)&0xFFFF;
	*pt = data&0xFFFF;
}

static inline void garminTcxResetScale (TTCX *tcx)
{
	tcx->route.scale.factor = tcx->route.scale.initial;
	tcx->route.offset.x = 0;
	tcx->route.offset.y = 0;
}

static inline int garminIsTcxEnabled (TVLCPLAYER *vp)
{
	int state = 0;
	settingsGet(vp, "home.enableTCX", &state);
	return state;
}

int garminConfigIsTcxEnabled (void *pageStruct)
{
	TPAGE2COMOBJ *page = (TPAGE2COMOBJ*)pageStruct;
	return garminIsTcxEnabled(page->com->vp);
}

static inline TFRAME *strCacheNewString_1 (TFRAMESTRINGCACHE *strc, const int font, const int col, const char *format, const double value)
{
	unsigned int hash = (font << 20);
	hash ^= col << 8;
	hash ^= (format[2] << 4);
	hash ^= (unsigned int)(value * 10000.0);
	
	
	TFRAME *str = strcFindString(strc, hash);
	if (str) return str;
	
	str = lNewString(strc->hw, LFRM_BPP_32, 0, font, format, value);
	if (str){
		strcAddString(strc, str, hash);
		return str;
	}

	return NULL;
}

static inline TFRAME *strCacheNewString_2 (TFRAMESTRINGCACHE *strc, const int font, const int col, const char *format, const double value1, const double value2)
{
	unsigned int hash = (font << 20);
	hash ^= col << 8;
	hash ^= (format[2] << 4);
	hash ^= (unsigned int)(value1 * 100000.0);
	hash ^= (unsigned int)(value2 * 100000.0);
	
	
	TFRAME *str = strcFindString(strc, hash);
	if (str) return str;
	
	str = lNewString(strc->hw, LFRM_BPP_32, 0, font, format, value1, value2);
	if (str){
		strcAddString(strc, str, hash);
		return str;
	}

	return NULL;
}

static inline TFRAME *strCacheNewString_3 (TFRAMESTRINGCACHE *strc, const int font, const int col, const char *format, const int value1, const int value2, const int value3)
{
	unsigned int hash = (font << 20);
	hash ^= col << 8;
	hash ^= (format[2] << 4);
	hash ^= (unsigned int)(value1 * 100);
	hash ^= (unsigned int)(value2 * 1000);
	hash ^= (unsigned int)(value3 * 10000);
	
	
	TFRAME *str = strcFindString(strc, hash);
	if (str) return str;
	
	str = lNewString(strc->hw, LFRM_BPP_32A, 0, font, format, value1, value2, value3);
	if (str){
		strcAddString(strc, str, hash);
		return str;
	}

	return NULL;
}

// %f
static inline void drawVal_1 (TTCX *tcx, TFRAME *frame, const int x, const int y, const int col, const char *format, const double value)
{
	TFRAME *str = strCacheNewString_1(tcx->strc.bin, MFONT, col, format, value);
	if (str) drawImage(str, frame, x, y);
}

// %f %f
static inline void drawVal_2 (TTCX *tcx, TFRAME *frame, const int x, const int y, const int col, const char *format, const double value1, const double value2)
{
	TFRAME *str = strCacheNewString_2(tcx->strc.bin, MFONT, col, format, value1, value2);
	if (str) drawImage(str, frame, x, y);
}

// %i %i %i
static inline void drawVal_3 (TTCX *tcx, TFRAME *frame, const int x, const int y, const int col, const char *format, const int value1, const int value2, const int value3)
{
	TFRAME *str = strCacheNewString_3(tcx->strc.bin, MFONT, col, format, value1, value2, value3);
	if (str) drawImage(str, frame, x, y);
}

static inline void routeDrawLapStats (TTCX *tcx, TFRAME *frame, const int _x, const int _y, tcx_lap *lap, const int lapNo)
{
	const int col = 255<<24|0xFFFFFF;
	const int vpitch = 24;
	int x = _x;
	int y = _y;
		
	drawVal_1(tcx, frame, x-100, y, 	col, "Lap %.0f:", lapNo);
	drawVal_1(tcx, frame, x, y, 		col, "Speed: %.1f km/h", lap->stats.speed.ave);
	drawVal_1(tcx, frame, x, y+=vpitch, col, "Length: %.1f km", lap->distance/1000.0);


	tcx_time t = {0};
	tcx_time64ToTime(lap->time.totalSeconds, &t);
	drawVal_3(tcx, frame, x, y+=vpitch, col, "Time: %i:%.2i:%.2i", t.hour, t.min, t.sec);

	if (lap->cadence.average){
		drawVal_1(tcx, frame, x, y+=vpitch, col, "Ave. Cadence: %.0f", lap->cadence.average);

		if (lap->cadence.maximumCalculated)
			drawVal_1(tcx, frame, x, y+=vpitch, col, "Max. Cadence: %.0f", lap->cadence.maximumCalculated);
	}

	if (lap->stats.speed.max > 1.0)
		drawVal_1(tcx, frame, x, y+=vpitch, col, "Max. Speed: %.1f km/h", lap->stats.speed.max);

	if (lap->heartRate.average)
		drawVal_1(tcx, frame, x, y+=vpitch, col, "Heartrate: %.0f", lap->heartRate.average);

	
	/*tcx_trackpoint *tp1 = lap->tracks.list[0]->trackPoints.list[0];
	tcx_trackpoint *tp2 = lap->tracks.list[lap->tracks.total-1]->trackPoints.list[lap->tracks.list[lap->tracks.total-1]->trackPoints.total-1];
	double d = calcDistKm(tp1->latitude, tp1->longitude, tp2->latitude, tp2->longitude);
	printf("dist %.0f\n", d);*/
}

static inline void routeDrawMapCursorLocation (TTCX *tcx, TFRAME *frame, int x, int y, const int cx, const int cy)
{

	const int col = 255<<24|0xFFFFFF;
	lSetForegroundColour(frame->hw, col);
	lSetBackgroundColour(frame->hw, 255<<24|0x000000);
	lSetRenderEffect(frame->hw, LTR_OUTLINE2);
	lSetFilterAttribute(frame->hw, LTR_OUTLINE2, 0, 240<<24 | COL_BLACK);
	const int vpitch = 24;
	
	double locLong, locLat;
	routeScreenToLocation(tcx, cx, cy, &locLong, &locLat);
	//printf(": %f %f\n", locLong, locLat);
		
	drawVal_1(tcx, frame, x, y,          col, "Latitude: %f", locLat);
	drawVal_1(tcx, frame, x, y += vpitch, col, "Longitude: %f", locLong);

	int gridx, gridy;
	locationToImageGrid(WORLD_ZOOM, locLat, locLong, &gridx, &gridy);
	drawVal_3(tcx, frame, x, y += vpitch, col, "%i, %i (%iz)", gridx, gridy, WORLD_ZOOM);


	double distH = frame->width * tcx->route.ruler.metersPerPixelH / 1000.0;
	double distV = frame->height * tcx->route.ruler.metersPerPixelV / 1000.0;
		
	drawVal_2(tcx, frame, frame->width - 250, y, COL_WHITE,		\
		"Area covered:\n\tHorizontal: %.1f Km\n\tVertical: %.1f Km", distH, distV);
	
	drawVal_2(tcx, frame, frame->width - 250, y+80, COL_WHITE,	\
		"MetersPerPixel:\n\tHorizontal: %f\n\tVertical: %f", tcx->route.ruler.metersPerPixelH, tcx->route.ruler.metersPerPixelV);	
	
	tcx->route.cursor.position.x = cx;
	tcx->route.cursor.position.y = cy;
	tcx->route.cursor.location.longitude = locLong;
	tcx->route.cursor.location.latitude = locLat;
}

static inline void routeDrawTrkPtStats (TTCX *tcx, TFRAME *frame, const int _x, const int _y, tcx_trackpoint *tpt, uint64_t time64)
{
	const int col = 255<<24|0xFFFFFF;
	lSetForegroundColour(frame->hw, col);
	lSetBackgroundColour(frame->hw, 255<<24|0x000000);
	lSetRenderEffect(frame->hw, LTR_OUTLINE2);
	lSetFilterAttribute(frame->hw, LTR_OUTLINE2, 0, 240<<24 | COL_BLACK);
	//lSetFilterAttribute(frame->hw, LTR_SHADOW, 0, LTRA_SHADOW_N|LTRA_SHADOW_S|LTRA_SHADOW_E|LTRA_SHADOW_W | LTRA_SHADOW_S5 | LTRA_SHADOW_OS(0) | LTRA_SHADOW_TR(240));
	//lSetFilterAttribute(frame->hw, LTR_SHADOW, 1, 0x0);	// set shadow colour (default is 0x00000)
	
	int x = _x;
	int y = _y;
	const int vpitch = 24;
	const int hpitch = 210;

	drawVal_1(tcx, frame, x, y, 		col, "Altitude: %.1f", tpt->altitude);
	drawVal_1(tcx, frame, x, y+=vpitch, col, "Distance: %.1f", tpt->distance/1000.0);
	drawVal_1(tcx, frame, x, y+=vpitch, col, "Speed: %.1f", tpt->speed);

	if (tpt->cadence)
		drawVal_1(tcx, frame, x, y+=vpitch, col, "Cadence: %.0f", tpt->cadence);
	if (tpt->heartRate)
		drawVal_1(tcx, frame, x, y+=vpitch, col, "Heartrate: %.0f", tpt->heartRate);

	drawVal_2(tcx, frame, x, y+=vpitch, col, 	 "Pos: %f, %f", tpt->latitude, tpt->longitude);
	struct tm *tm = _localtime64((__time64_t*)&tpt->time.time64);
	if (tm)
		drawVal_3(tcx, frame, x+=hpitch, y=_y, col,  "Clock: %.2i:%.2i:%.2i", tm->tm_hour, tm->tm_min, tm->tm_sec);
	//drawVal_3(tcx, frame, x+=hpitch, y=_y, col,  "Clock: %.2i:%.2i:%.2i", tpt->time.time.hour, tpt->time.time.min, tpt->time.time.sec);

	tcx_time t = {0};
	tcx_time64ToTime(time64, &t);
	drawVal_3(tcx, frame, x, y+=vpitch, col, "Time: %i:%.2i:%.2i", t.hour, t.min, t.sec);
	if (tpt->power > 0.0)
		drawVal_1(tcx, frame, x, y+=vpitch, col, "Power: ", tpt->power);

	//printf("Pos: %f  %f\n", tpt->latitude, tpt->longitude);
	
	/* double d = calcDistM(
	 54.481420-0.00925,  -6.224985,
	 54.462925+0.00925,  -6.083169);
	 
	 printf("dist %f\n", d);*/
}

static inline void routeDrawTrkPtMark (TFRAME *frame, tcx_screenPt *pt, const int r, const int innerCol, const int outerCol)
{
	lDrawCircleFilled(frame, pt->x, pt->y, r-1, innerCol);
	lDrawCircle(frame, pt->x, pt->y, r, outerCol);
}

static inline void routeDrawRouteMark (TFRAME *frame, tcx_renderContext *rc, const double x, const int innerCol, const int outerCol)
{
	int idx = rc->points.total * (x/100.0);
	if (idx < 0)
		idx = 0;
	else if (idx > rc->points.total-1)
		idx = rc->points.total-1;
	
	routeDrawTrkPtMark(frame, &rc->points.screen[idx], 5, innerCol, outerCol);
}

static inline void routeDrawStartMarker (TFRAME *frame, tcx_renderContext *rc)
{
	routeDrawRouteMark(frame, rc, 0.0, 240<<24|COL_GREEN, 200<<24 | 0xEEEEEE);
}

static inline void routeDrawEndMarker (TFRAME *frame, tcx_renderContext *rc)
{
	routeDrawRouteMark(frame, rc, 100.0, 240<<24|COL_RED, 200<<24 | 0xEEEEEE);
}

static inline void routeDrawTrkPtCursor (TFRAME *frame, const int x, const int y, tcx_screenPt *pt)
{
	const int backCol = 100<<24|COL_BLACK;
	const int lineCol = CURSORMARK_COLOUR;

	
	lDrawLine(frame, x-1, y-1, pt->x-1, pt->y-1, backCol);
	lDrawLine(frame, x+1, y+1, pt->x+1, pt->y+1, backCol);
	lDrawLine(frame, x-1, y, pt->x-1, pt->y, backCol);
	lDrawLine(frame, x, y-1, pt->x, pt->y-1, backCol);
	lDrawLine(frame, x+1, y, pt->x+1, pt->y, backCol);
	lDrawLine(frame, x, y+1, pt->x, pt->y+1, backCol);

	lDrawLine(frame, x, y, pt->x, pt->y, lineCol);
}

static inline void graphDrawPositionMarker (TGRAPH *graph, TFRAME *frame, const double pos, const int colour)
{
	const int x1 = pos * (double)frame->width;
	const int y1 = ccGetPositionY(graph);
	const int y2 = y1 + ccGetHeight(graph)-2;
			
	lDrawLine(frame, x1-1, y1, x1-1, y2+1, 100<<24|COL_BLACK);
	lDrawLine(frame, x1+1, y1, x1+1, y2+1, 100<<24|COL_BLACK);
	lDrawLine(frame, x1, y1, x1, y2, colour);
	lDrawTriangleFilled(frame, x1-6, y2+7, x1, y2+1, x1+6, y2+7, CURSORMARK_COLOUR);
	lDrawTriangle(frame, x1-7, y2+8, x1, y2+1, x1+7, y2+8, 127<<24|COL_BLACK);
}

static inline void graphDrawLapBand1 (TGRAPH *graph, TFRAME *frame, tcx_renderContext *rc, const int lap, const int colour)
{
	int lapStart = -1;
	int lapEnd = -1;
	tcx_renderPt *pt = rc->points.scaled;
	
	for (int i = 0; i < rc->points.total; i++, pt++){
		if (pt->data>>32 == lap){
			if (lapStart == -1)
				lapStart = i;
			else
				lapEnd = i;
		}
	}
	
	if (lapStart >=0 && lapEnd > lapStart){
		//printf("graphDrawLapBand: %i %i\n", lapStart, lapEnd);

		double posStart = lapStart / (double)rc->points.total;
		int x1 = posStart * (double)frame->width;
		
		double posEnd = lapEnd / (double)rc->points.total;
		int x2 = posEnd * (double)frame->width;
		
		int y1 = ccGetPositionY(graph);
		int y2 = y1 + ccGetHeight(graph)-2;

		lDrawRectangleDottedFilled(frame, x1, y1, x2, y2, colour);
	}
}

static inline void graphDrawLapBand2 (TGRAPH *graph, TFRAME *frame, tcx_renderContext *rc, const int lap, const int colour)
{
	int lapStart = -1;
	int lapEnd = -1;
	tcx_renderPt *pt = rc->points.scaled;
	
	for (int i = 0; i < rc->points.total; i++, pt++){
		if (pt->data>>32 == lap){
			if (lapStart == -1)
				lapStart = i;
			else
				lapEnd = i;
		}
	}
	
	if (lapStart >=0 && lapEnd > lapStart){
		//printf("graphDrawLapBand: %i %i\n", lapStart, lapEnd);

		double posStart = lapStart / (double)rc->points.total;
		int x1 = posStart * (double)frame->width;
		double posEnd = lapEnd / (double)rc->points.total;
		
		int x2 = posEnd * (double)frame->width;
		int y2 = ccGetPositionY(graph) + ccGetHeight(graph);

		lDrawRectangleFilled(frame, x1, y2, x2, y2+3, colour);
	}
}

static inline void routeDrawMapMarker (TTCX *tcx, TFRAME *frame, const int x, const int y)
{
	drawImg(tcx->com->vp, frame, tcx->mapMarker, x-16, y-38);
}

static inline void drawCrosshair (TTCX *tcx, TFRAME *frame, const int x, const int y)
{
	drawImg(tcx->com->vp, frame, tcx->crosshair, x-16, y-16);
}

static inline void graphDrawCrosshairB (TGRAPH *graph, TFRAME *frame, const int x, const int y)
{

	const int col = 220<<24 | COL_WHITE;
	const int holeSize = 2;

	TMETRICS met;
	ccGetMetrics(graph, &met);
			
	lDrawLine(frame, x, met.y - holeSize, x, y - holeSize, col);				// up
	lDrawLine(frame, met.x, y, x - holeSize, y, col);							// left
	lDrawLine(frame, x + holeSize, y, x + holeSize + met.width-1, y, col);		// right
	lDrawLine(frame, x, y + holeSize, x, holeSize + met.y + met.height-1, col);	// down
}

static inline void graphDrawCursor (TGRAPH *graph, TFRAME *frame)
{
	graphDrawCrosshairB(graph, frame, getCursorX(graph), getCursorY(graph));
}

static inline int graphDrawCursorSheetValue (TTCX *tcx, TGRAPH *graph, TFRAME *frame, struct sheet *sht, const int x, const int y, const int type)
{
	int enabled = 0;
	
	TGRAPHSHEET *sheet = graphSheetAcquire(graph, NULL, sht->id);
	if (sheet){
		if ((enabled=sheet->render.enabled)){
			TMETRICS metrics;
			ccGetMetrics(graph, &metrics);
		
			double range = (metrics.height/sheet->render.scale)/100.0;
			double value = (graph->hoveredPt.y * range) + (sheet->stats.min * (double)(sheet->render.mode&GRAPH_AUTOSCALE));
			value /= sht->multiplier;
			//printf("render %.2f %.2f %.2f %.2f\n", graph->hoveredPt.y, value, sheet->stats.min/sht->multiplier, sheet->stats.max/sht->multiplier);
				
			const int col = 240<<24|(sht->colour&0xFFFFFF);
			lSetForegroundColour(frame->hw, col);
			lSetFilterAttribute(frame->hw, LTR_OUTLINE2, 0, 123<<24 | (sht->colour&0xFFFFFF));
			
			char *format = "%.1f";
			if (type == 1) format = "%.0f";
			TFRAME *val = strCacheNewString_1(tcx->strc.bin, GARMIN_HEADING_FONT, col, format, value);
			if (val) drawImage(val, frame, x, y);
		}
		graphSheetRelease(sheet);
	}
	return enabled;
}

static inline void setPixel_NB (const TFRAME *restrict frm, const int x, const int y, const int value)
{
	*(uint32_t*)(frm->pixels+((y*frm->pitch)+(x<<2))) = value;
}

static inline void connectLine (TFRAME *restrict frame, const int x1, const int y1, const int x2, const int y2, const int colour)
{
	if (x1 == x2 && y1 == y2)
		lSetPixel(frame, x1, y1, colour);
	else
		lDrawLine(frame, x1, y1, x2, y2, colour);
}

static inline void routeRenderLine (TTCX *tcx, TFRAME *frame, const tcx_renderContext *rc)
{
	tcx_screenPt *pts = rc->points.screen;
	const int len = rc->points.total;

	
	for (int i = 0; i < len-1; i++, pts++){
		if (!pts->isInside && !pts[1].isInside) continue;
		
		int lap = pts->data>>32;
		if (lap == (int)(pts[1].data>>32)){
			lap = lap2Colour(tcx,lap);

			int x1 = pts->x;
			int y1 = pts->y;
			int x2 = pts[1].x;
			int y2 = pts[1].y;
			connectLine(frame, x1, y1, x2, y2, lap);
		}
	}
}

static inline void routeRenderShadow (TTCX *tcx, TFRAME *frame, const tcx_renderContext *rc)
{
	//const int col1 = 83<<24 | COL_BLACK;
	//const int col2 = 53<<24 | COL_BLACK;
	//const int col3 = 33<<24 | COL_BLACK;
	
	const int col1 = 33<<24 | COL_BLACK;
	const int col2 = 23<<24 | COL_BLACK;
	const int col3 = 13<<24 | COL_BLACK;
	
	tcx_screenPt *pts = rc->points.screen;	
	const int len = rc->points.total;

	for (int i = 0; i < len-1; i++, pts++){
		if (!pts->isInside && !pts[1].isInside) continue;
		
		uint64_t lap = pts->data>>32;
		if (lap == (uint64_t)(pts[1].data>>32)){
			int col = 93<<24|(lap2Colour(tcx,lap)&0xFFFFFF);
			int x1 = pts->x;
			int y1 = pts->y;
			int x2 = pts[1].x;
			int y2 = pts[1].y;


			connectLine(frame, x1, y1-3, x2, y2-3, col3);
			connectLine(frame, x1, y1+3, x2, y2+3, col3);
			connectLine(frame, x1-3, y1, x2-3, y2, col3);
			connectLine(frame, x1+3, y1, x2+3, y2, col3);
			
			connectLine(frame, x1-2, y1-2, x2-2, y2-2, col2);
			connectLine(frame, x1+2, y1+2, x2+2, y2+2, col2);
			connectLine(frame, x1-2, y1+2, x2+2, y2-2, col2);
			connectLine(frame, x1+2, y1-2, x2-2, y2+2, col2);
			
			connectLine(frame, x1, y1-2, x2, y2-2, col1);
			connectLine(frame, x1, y1+2, x2, y2+2, col1);
			connectLine(frame, x1-2, y1, x2-2, y2, col1);
			connectLine(frame, x1+2, y1, x2+2, y2, col1);

			connectLine(frame, x1, y1-1, x2, y2-1, col);
			connectLine(frame, x1, y1+1, x2, y2+1, col);
			connectLine(frame, x1-1, y1, x2-1, y2, col);
			connectLine(frame, x1+1, y1, x2+1, y2, col);
		}
	}
}

static inline void routeRenderPointGlow (TTCX *tcx, TFRAME *frame, const tcx_renderContext *rc)
{
	const int col = 255<<24|0xFF00FF;
	tcx_screenPt *pts = rc->points.plotted;
	const int len = rc->points.plottedTotal;
		

	for (int i = 0; i < len; i++, pts++){
		if (pts->x-1 >= 0 && pts->x+1 < frame->width){
			if (pts->y-1 >= 0 && pts->y+1 < frame->height){
				setPixel_NB(frame, pts->x-1, pts->y-1, col);
				setPixel_NB(frame, pts->x+1, pts->y-1, col);
				setPixel_NB(frame, pts->x+1, pts->y+1, col);
				setPixel_NB(frame, pts->x-1, pts->y+1, col);
			}
		}
	}
}

static inline void routeRenderPointGlowEx (TTCX *tcx, TFRAME *frame, const tcx_renderContext *rc, const int when)
{
	const int col = 200<<24|COL_PURPLE;
	tcx_screenPt *pts = rc->points.plotted;
	const int len = rc->points.plottedTotal;
		
	const int width = frame->width;
	const int height = frame->height;

	for (int i = 0; i < len; i++, pts++){
		if (when == (int)(pts->data>>32)){
			if (pts->x-1 >= 0 && pts->x+1 < width){
				if (pts->y-1 >= 0 && pts->y+1 < height){
					lDrawCircle(frame, pts->x, pts->y, 3, col);
					lDrawCircle(frame, pts->x, pts->y, 2, col);
				}
			}
		}
	}

}
		
static inline void routeRenderPointLarge (TTCX *tcx, TFRAME *frame, const tcx_renderContext *rc)
{
	const int col1 = 123<<24|0xFF00FF;
	const int col2 = 255<<24|0xFF00FF;
	tcx_screenPt *pts = rc->points.plotted;
	const int len = rc->points.plottedTotal;
		
	for (int i = 0; i < len; i++, pts++){
		lDrawCircle(frame, pts->x, pts->y, 4, col1);
		lDrawCircle(frame, pts->x, pts->y, 3, col2);
	}
}

static inline void routeRenderPoint (TTCX *tcx, TFRAME *frame, const tcx_renderContext *rc)
{
	const int col = 255<<24|COL_WHITE;
	tcx_screenPt *pts = rc->points.plotted;
	const int len = rc->points.plottedTotal;
		
	for (int i = 0; i < len; i++, pts++)
		setPixel_NB(frame, pts->x, pts->y, col);
}

static inline void routeRenderLapHighlight (TTCX *tcx, TFRAME *frame, const tcx_renderContext *rc)
{
	const int col = 203<<24|COL_WHITE;
	tcx_screenPt *pts = rc->points.screen;
	const int len = rc->points.total-1;
	const int data = tcx->route.cursorTrkPtData>>32;
		
	for (int i = 0; i < len; i++, pts++){
		if ((pts->data>>32) != data) continue;
		if (!pts->isInside && !pts[1].isInside) continue;

		if (pts->data>>32 == (int)(pts[1].data>>32)){
			int x1 = pts->x;
			int y1 = pts->y;
			int x2 = pts[1].x;
			int y2 = pts[1].y;

			connectLine(frame, x1, y1-1, x2, y2-1, col);
			connectLine(frame, x1, y1+1, x2, y2+1, col);
			connectLine(frame, x1-1, y1, x2-1, y2, col);
			connectLine(frame, x1+1, y1, x2+1, y2, col);
			connectLine(frame, x1, y1, x2, y2, col);
		}
	}
}

static inline void garminTcxRenderRouteWorld (TTCX *tcx, TFRAME *frame, const tcx_renderContext *rc, const int flags)
{
	tcx_screenPt *pts = rc->points.world;
	const int total = rc->points.total;
	const int col = 255<<24|COL_BLACK;
	
	for (int i = 0; i < total; i++, pts++){
		lSetPixel(frame, pts->x, pts->y, col);
		//lDrawCircle(frame, pts->x, pts->y, 4, col);
		lDrawCircle(frame, pts->x, pts->y, 1, col);
		
		
	}
}

static inline void garminTcxRenderRoute (TTCX *tcx, TFRAME *frame, const tcx_renderContext *rc, const int flags)
{
	if (flags&TCX_RENDER_ROUTE_SHADOW){
		if (tcx->activities->activities.total <= 10 || (tcx->slideState != SLIDE_SLIDING && tcx->activities->activities.total > 10))
			routeRenderShadow(tcx, frame, rc);
	}

	if (flags&TCX_RENDER_ROUTE_LINE)
		routeRenderLine(tcx, frame, rc);

	if (tcx->slideState != SLIDE_SLIDING){
		const double scale = tcx->route.scale.factor;
		
		if (flags&TCX_RENDER_ROUTE_POINT_GLOW){
			if (scale >= 15.0)
				routeRenderPointGlow(tcx, frame, rc);
		}

		if (flags&TCX_RENDER_ROUTE_POINT_LARGE){
			const double scale = tcx->route.scale.factor;
			if (scale >= 30.0 && tcx->activities->activities.total <= 10)
				routeRenderPointLarge(tcx, frame, rc);
		}
	}

	if (flags&TCX_RENDER_ROUTE_POINT){
		const double scale = tcx->route.scale.factor;
		if (scale >= 5.0)
			routeRenderPoint(tcx, frame, rc);
	}

	if (flags&TCX_RENDER_ROUTE_LAP_HIGHLIGHT){
		if (tcx->route.cursorTrkPtIdx >= 0 && tcx->slideState != SLIDE_SLIDING){
			routeRenderLapHighlight(tcx, frame, rc);
			if (tcx->route.scale.factor >= 15)
			routeRenderPointGlowEx(tcx, frame, rc, tcx->route.cursorTrkPtData>>32);
		}
	}

}

static inline void drawSheetIcons (TTCX *tcx, TFRAME *frame)
{
	ccRender(tcx->sheets.altitude.ui.view.label, frame);
	ccRender(tcx->sheets.speed.ui.view.label, frame);
	ccRender(tcx->sheets.heartRate.ui.view.label, frame);
	ccRender(tcx->sheets.cadence.ui.view.label, frame);
	//ccRender(tcx->sheets.power.ui.view.label, frame);
	ccRender(tcx->ui.route, frame);
}

static inline double calcRulerRange (const double meters)
{
	//if (meters <= 1.0) return 0.1;
	if (meters < 10.0) return 1.0;
	if (meters < 100.0) return 10.0;
	if (meters < 1000.0) return 100.0;
	if (meters < 10000.0) return 1000.0;
	if (meters < 100000.0) return 10000.0;

	return 1000.0;
}

static inline void drawRuler (TTCX *tcx, TFRAME *frame, const int y, const int colour)
{
	double width = 200.0;			// target width of ruler
	const int thickness = 2;		// ruler thickness
	const int barDepth = 10;		// marker height

	double m = width * tcx->route.ruler.metersPerPixelH;			// width of target in meters
	if (m < 1.0) m = 1.0;
	double scale = calcRulerRange(m);
	
	width -= (fmod(m, scale) / tcx->route.ruler.metersPerPixelH);	// adjust for a round number
	double value = width * tcx->route.ruler.metersPerPixelH;		// scaled value
	if (value < 0.001) value = 0.001;	// 1cm

	int x1 = abs((int)(frame->width-width)) / 2;
	int x2 = x1 + width-1;
		
	lDrawLine(frame, x1, y-barDepth, x1, y-thickness-1, colour);
	lDrawLine(frame, x2, y-barDepth, x2, y-thickness-1, colour);
	lDrawRectangleFilled(frame, x1, y, x2, y-thickness, colour);
	
	TFRAME *str;
	if (value <= 0.99999)
		str = lNewString(frame->hw, LFRM_BPP_32A, 0, MFONT, "%.0fcm", value*1000.0);
	else if (value >= 10000.0)
		str = lNewString(frame->hw, LFRM_BPP_32A, 0, MFONT, "%.0fkm", value/1000.0);
	else
		str = lNewString(frame->hw, LFRM_BPP_32A, 0, MFONT, "%.0fm", value);
	if (str){
		x1 = abs(frame->width - str->width) / 2;
		int y1 = y - (thickness + str->height + 2);
		drawImage(str, frame, x1, y1);
		lDeleteFrame(str);
	}
	
	drawVal_1(tcx, frame, x1 + 130, y-18 , colour, "%.2fx", tcx->route.scale.factor);
}

static inline int page_tcxRender (TTCX *tcx, TFRAME *frame)
{
	if (!tcx->renderContext){
		ccRender(tcx->ui.import, frame);
		return 1;
	}
	
/*
	static TFRAME *tmpImg;
	if (!tmpImg){
		wchar_t *path = L"P:\\temp\\incoming\\map.png";
		tmpImg = lNewImage(frame->hw, path, LFRM_BPP_32A);
	}else
		drawImageScaledCenter(tmpImg, frame, tcx->route.scale.factor, tcx->route.offset.x-tcx->route.offset.deltaX, tcx->route.offset.y-tcx->route.offset.deltaY);
*/
	
#if 0
	static TFRAME *srcMap;
	if (!srcMap){
		wchar_t *path = L"M:\\RamDiskTemp\\belfast2.png";
		srcMap = lNewImage(frame->hw, path, LFRM_BPP_32A);
	}else if (0 && windowSet){
		tcx_renderContext *rc = tcx->renderContext;
		
		int src_x = window.x1;
		int src_y = window.y1;
		int src_width = (window.x2 - window.x1)+1;
		int src_height = ((window.y2 - window.y1)+1) * 1.0;
		copyAreaScaled(srcMap, frame, src_x, src_y, src_width, src_height, 0, 0, frame->width, frame->height);
		

		printf("%f %f %f %f %f\n", rc->aspect, rc->aspectFactor, rc->scaleFactor, rc->scaleLatZoom, rc->scaleLongZoom);
	}
#endif

	if (tcx->route.offset.modified.ready){
		tcx->route.offset.modified.ready = 0;
		tcx_screen(tcx->renderContext, tcx->route.offset.modified.x, tcx->route.offset.modified.y);
		//calcWorldPositions(tcx, tcx->renderContext, WORLD_ZOOM, tcx->route.offset.modified.x, tcx->route.offset.modified.y);
	}

	tcx_renderContext *rc = tcx->renderContext;
	
#if 0
	static TFRAME *srcMap[4];
	if (!srcMap[0]){
		wchar_t *path[4] = {MAPS(L"12/1980/1303.png"),
							MAPS(L"12/1979/1303.png"),
							MAPS(L"12/1980/1304.png"),
							MAPS(L"12/1979/1304.png")};
		srcMap[0] = lNewImage(frame->hw, path[0], LFRM_BPP_32);
		srcMap[1] = lNewImage(frame->hw, path[1], LFRM_BPP_32);
		srcMap[2] = lNewImage(frame->hw, path[2], LFRM_BPP_32);
		srcMap[3] = lNewImage(frame->hw, path[3], LFRM_BPP_32);
	}else if (1){
		
		double zoom = WORLD_ZOOM;
		double lat = rc->maxLat;
		double lon = rc->maxLong;
		int wx, wy;
		locationToWorldGrid(zoom, lat, lon, &wx, &wy);

		int leftGrid, topGrid;
		locationToWorldGrid(zoom, rc->maxLat, rc->minLong, &leftGrid, &topGrid);
		int x = (((wx / 256) - (leftGrid/256)) * 256);// - rc->map.offsetX;
		int y = (((wy / 256) - (topGrid/256)) * 256);// - rc->map.offsetY;

		printf("%i %i, %i %i\n", x, y, rc->map.offsetX, rc->map.offsetY);
		
		x -= rc->map.offsetX;		
		y -= rc->map.offsetY;

		drawImageNoBlend(srcMap[0], frame, x, y);
		drawImageNoBlend(srcMap[1], frame, x-256, y);
		drawImageNoBlend(srcMap[2], frame, x, y+256);
		drawImageNoBlend(srcMap[3], frame, x-256, y+256);
	}
#endif


	TGRAPH *graph = tcx->graph;
	if (tcx->route.render.cursorMode == 2 && tcx->route.cursorTrkPtIdx/*tcx->graphLapHighlight*/ >= 0)
		graphDrawLapBand2(graph, frame, tcx->renderContext, tcx->graphLapHighlight, 143<<24 | 0x81FF81);

	ccRender(graph, frame);
	if (tcx->slideState != SLIDE_SLIDING)
		drawSheetIcons(tcx, frame);
	if (tcx->route.render.enabled > 0){
		garminTcxRenderRoute(tcx, frame, tcx->renderContext, tcx->route.render.flags);
		drawRuler(tcx, frame, frame->height - 42, 180<<24 | COL_BLACK);
	}

	garminTcxRenderRouteWorld(tcx, frame, tcx->renderContext, tcx->route.render.flags);
	ccRender(tcx->ui.zoomIn, frame);
	ccRender(tcx->ui.zoomOut, frame);
	ccRender(tcx->ui.zoomReset, frame);

	const int64_t dt = getTickCount() - tcx->t0;
	if (dt < UPDATERATE_LENGTH)
		setTargetRate(tcx->com->vp, 35);
	else{
		setTargetRate(tcx->com->vp, UPDATERATE_BASE);
		ccHoverRenderSigEnable(graph->cc, 35.0);
	}

	int enableMapInspect = tcx->route.render.cursorMode == 3;
	if (enableMapInspect){
		int x, y;
		routeLocationToScreen(tcx, tcx->route.location.longitude, tcx->route.location.latitude, &x, &y);
		routeDrawMapMarker(tcx, frame, x, y);
	}

	if (tcx->slideState == SLIDE_SLIDING){
		drawCrosshair(tcx, frame, frame->width/2, frame->height/2);
		return 1;
	}

	ccRender(tcx->ui.import, frame);
	ccRender(tcx->ui.cursorModeA, frame);
	ccRender(tcx->ui.cursorModeB, frame);
	ccRender(tcx->ui.cursorModeC, frame);
	ccRender(tcx->ui.info, frame);
	if (enableMapInspect){
		ccRender(tcx->ui.onlineMap, frame);
		ccRender(tcx->ui.onlineAddress, frame);
	}

	if (ccRender(tcx->info.pane, frame))
		return 1;
	

	int enablePointInspect = tcx->route.render.cursorMode == 2;
	int enableRouteInspect = tcx->route.render.cursorMode == 1;
	
	enablePointInspect = enablePointInspect && (tcx->route.render.sheets > 0);
	if (enablePointInspect){
		const int isOverlapped = ccPositionIsOverlapped(graph, getCursorX(graph), getCursorY(graph));
		if (/*tcx->drawCursorTimeout-- > 0 ||*/ isOverlapped /*ccIsHovered(graph)*/){
			cursorSetState(tcx->graph->cc->cursor, 0);
			graphDrawCursor(graph, frame);

			lSetRenderEffect(frame->hw, LTR_OUTLINE2);
			const int y = frame->height - 40;
			const double pitch = 0.135;
			double x = 0.30;
	
			int drawn = graphDrawCursorSheetValue(tcx, graph, frame, &tcx->sheets.altitude, frame->width * x, y, 0);
			if (drawn) x += pitch;
			drawn = graphDrawCursorSheetValue(tcx, graph, frame, &tcx->sheets.speed, frame->width * x, y, 0);
			if (drawn) x += pitch;
			drawn = graphDrawCursorSheetValue(tcx, graph, frame, &tcx->sheets.heartRate, frame->width * x, y, 1);
			if (drawn) x += pitch;
			graphDrawCursorSheetValue(tcx, graph, frame, &tcx->sheets.cadence, frame->width * x, y, 1);

		}else if (tcx->drawCursor){
			cursorSetState(tcx->graph->cc->cursor, tcx->drawCursor);

		}
	}

	if (tcx->route.render.enabled > 0){
		if (tcx->route.render.sheets && enablePointInspect)
			routeDrawRouteMark(frame, rc, graph->hoveredPt.x, 220<<24|COL_WHITE, 200<<24 | 0x111111);
		routeDrawStartMarker(frame, rc);
		routeDrawEndMarker(frame, rc);

	}

	if (enableMapInspect){

		routeDrawMapCursorLocation(tcx, frame, 10, tcx->route.render.statsPosY, getCursorX(graph), getCursorY(graph));
		/*int x, y;
		routeLocationToScreen(tcx, tcx->route.location.longitude, tcx->route.location.latitude, &x, &y);
		routeDrawMapMarker(tcx, frame, x, y);*/

	}else if (enablePointInspect){
		int activity, lap, track, pt;
		routeGetTrkPtData(rc, rc->points.total*(graph->hoveredPt.x/100.0), &activity, &lap, &track, &pt);

		tcx_activity *a = tcx_getActivity(tcx->activities, activity+1);
		if (a){
			tcx_lap *l = a->laps.list[lap];
			tcx_trackpoint *tpt = l->tracks.list[track]->trackPoints.list[pt];
			uint64_t t64 = (tpt->time.time64 - a->startTime64)+1;

			//printf("%i %i %i\n", activity+1, lap+1, pt+1);
			//lPrintf(frame, 200, 2, MFONT, LPRT_CPY, "%i/%i/%i\n", activity+1, lap+1, pt+1);
			routeDrawTrkPtStats(tcx, frame, 10, tcx->route.render.statsPosY, tpt, t64);
			routeDrawLapStats(tcx, frame, frame->width - 210, tcx->route.render.statsPosY, l, lap+1);
		}
	}

	if (tcx->route.render.enabled > 0 && enableRouteInspect){
		const int x = getCursorXAdjusted(graph);
		const int y = getCursorYAdjusted(graph);
		uint64_t minDist = 999999999;
		int minIdx = -1;
	
		//tcx_screenPt *pts = rc->points.screen;
		//const int listLen = rc->points.total;
		tcx_screenPt *pts = rc->points.plotted;
		const int listLen = rc->points.plottedTotal;
		
		for (int i = 0; i < listLen; i++, pts++){
			if (!pts->isInside) continue;

			uint64_t dist = calcDistPx(pts, x, y);
			if (dist > 0 && dist < minDist){
				minDist = dist;
				minIdx = i;
			}
		}

		if (minIdx >= 0 && minDist < 6072){
			int activity, lap, track, pt;
			tcx->route.cursorTrkPtIdx = minIdx;
			tcx->route.cursorTrkPtData = routeGetDataP(rc, tcx->route.cursorTrkPtIdx);
			
			routeGetTrkPtDataP(rc, tcx->route.cursorTrkPtIdx, &activity, &lap, &track, &pt);
			if (tcx->graphLapHighlight != lap) pageUpdate(tcx);
			tcx->graphLapHighlight = lap;
			
    		// draw position marker (vertical line along horizontal graph)
			if (tcx->route.render.enabled > 0 && tcx->route.render.sheets){
				//graphDrawLapBand(graph, frame, tcx->renderContext, lap, 60<<24 | COL_BLACK);
				uint64_t data = rc->points.plotted[tcx->route.cursorTrkPtIdx].data;
				pts = rc->points.screen;
				const int len = rc->points.total;
				
				for (int i = 0; i < len; i++, pts++){
					if (pts->data == data){
						graphDrawPositionMarker(graph, frame, i/(double)len, lap2Colour(tcx,lap));
						break;
					}
				}
			}

			// highlight hovered point
			//printf("tcx->route.scale.factor %f\n", tcx->route.scale.factor);
			if (tcx->route.scale.factor >= 2.0 && tcx->route.cursorTrkPtIdx >= 0){

				pts = &rc->points.plotted[tcx->route.cursorTrkPtIdx];
				routeDrawTrkPtMark(frame, pts, 4, 240<<24|COL_WHITE, 240<<24|COL_BLACK);
			}
			routeDrawTrkPtCursor(frame, x, y, &rc->points.plotted[tcx->route.cursorTrkPtIdx]);
			tcx_activity *a = tcx_getActivity(tcx->activities, activity+1);
			if (a){
				tcx_lap *l = a->laps.list[lap];
				tcx_trackpoint *tpt = l->tracks.list[track]->trackPoints.list[pt];
				uint64_t t64 = (tpt->time.time64 - a->startTime64)+1;	// fixme. does not account for startTime64 being later than endtime (day before)
				//lPrintf(frame, 200, 2, MFONT, LPRT_CPY, "%i/%i/%i/%i\n", activity+1, lap+1, track+1, pt+1);
				routeDrawTrkPtStats(tcx, frame, 10, tcx->route.render.statsPosY, tpt, t64);
				routeDrawLapStats(tcx, frame, frame->width - 210, tcx->route.render.statsPosY, l, lap+1);
			}
		}else{
			tcx->route.cursorTrkPtData = 0;
			tcx->route.cursorTrkPtIdx = -1;
		}
	}else{
		tcx->route.cursorTrkPtData = 0;
		tcx->route.cursorTrkPtIdx = -1;
	}

	return 1;
}

static inline int page_tcxRenderBegin (TTCX *tcx, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	lSetBackgroundColour(frame->hw, 0xFFFFFF);
	lSetFilterAttribute(frame->hw, LTR_OUTLINE2, 0, 180<<24 | COL_TASKBARBK);
	lSetRenderEffect(frame->hw, LTR_OUTLINE2);

	tcx->drawCursor = cursorGetState(tcx->graph->cc->cursor);
	
	ccHoverRenderSigEnable(vp->cc, 35.0);
	
	tcx->background.original = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_BGIMAGE]);	// don't delete original back
	if (tcx->background.original){
		tcx->background.surface = lNewFrame(frame->hw, frame->width, frame->height, frame->bpp);
		if (tcx->background.surface){
			fillFrameColour(tcx->background.surface, tcx->background.colour);
			vp->gui.skin.bg = tcx->background.surface;
		}
	}
	
	tcx->route.render.sheets = graphRenderGetSheetCount(tcx->graph);
	return 1;
}

static inline int page_tcxRenderEnd (TTCX *tcx, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, void *opaquePtr)
{
	if (tcx->background.original){
		vp->gui.skin.bg = tcx->background.original;
		imageManagerImageRelease(vp->im, vp->gui.image[IMGC_BGIMAGE]);
		if (tcx->background.surface)
			lDeleteFrame(tcx->background.surface);
	}
	
	ccHoverRenderSigDisable(vp->cc);
	cursorSetState(tcx->graph->cc->cursor, tcx->drawCursor);

	lSetRenderEffect(vp->ml->hw, LTR_DEFAULT);
	return 1;
}

static inline tcx_activities *garminTcxLoad (const char *file)
{
	//double t0 = getTime(vp);
	tcx_activities *activities = tcx_open8(file);
	if (activities) tcx_calcStats(activities);
	//double t1 = getTime(vp);
	return activities;
}

static inline void garminTcxRescale (TTCX *tcx, tcx_renderContext *rc, tcx_activities *activities, const double scale, const double offsetX, const double offsetY)
{
	//printf("garminTcxRescale %f %f %f\n", scale, offsetX, offsetY);
	
	// normalize() and scale() must be called after each call to zoom();
	tcx_zoom(rc, scale);

	// normalize data points
	tcx_normalize(rc, activities, 0, rc->minLong, rc->minLat);

	//calcWorldPositions(tcx, rc, WORLD_ZOOM, offsetX, offsetY);

	// scale for display 
	tcx_scale(rc, 1.0, 1.0);

	// [p]re-calculate display co-ords
	tcx->route.offset.modified.x = offsetX;
	tcx->route.offset.modified.y = offsetY;
	tcx->route.offset.modified.ready = 1;
	//tcx_screen(rc, offsetX, offsetY);


	double dPixelsW;
	double dPixelsH;
	if (rc->aspect <= rc->scaleFactor){
		dPixelsW = (rc->width * rc->aspectFactor) * scale;
		dPixelsH = (rc->height * rc->aspectFactor) * (1.0/rc->aspectFactor) * scale;
	}else{
		dPixelsW = (rc->width * rc->aspectFactor) * (1.0/rc->aspectFactor) * scale;
		dPixelsH = (rc->height * rc->aspectFactor) * scale;
	}
	
	
	double mlat = rc->minLat + (rc->dLat / 2.0);
	double mlong = rc->minLong + (rc->dLong / 2.0);
	tcx->route.ruler.distanceLong = calcDistM(mlat, rc->minLong, mlat, rc->maxLong);
	tcx->route.ruler.distanceLat = calcDistM(rc->minLat, mlong, rc->maxLat, mlong);
	
	//printf("garminTcxRescale: %.2f %.2f\n", tcx->route.ruler.distanceLong, tcx->route.ruler.distanceLat);
	
	tcx->route.ruler.metersPerPixelH = tcx->route.ruler.distanceLong / dPixelsW;
	tcx->route.ruler.metersPerPixelV = tcx->route.ruler.distanceLat / dPixelsH;

}

static inline tcx_renderContext *garminTcxGenerateRenderContext (TTCX *tcx, tcx_activities *activities, const int width, const int height)
{
	//printf("tcx_open time %.1f\n", t1-t0);
	
	tcx_renderContext *rc = tcx_createRenderContext(activities, 0, width, height);
	if (rc){
		// calc normalization factors
		tcx_calcScaleMultipliers(rc);
		garminTcxResetScale(tcx);
		garminTcxRescale(tcx, rc, activities, tcx->route.scale.factor, tcx->route.offset.x, tcx->route.offset.y);
	}
	return rc;
}

static inline int garminTcxBuildGraphs (tcx_activities *activities, const int activityNo, TGRAPH *graph, struct tcxsheets *sheets)
{

	tcx_activity *activity = tcx_getActivity(activities, activityNo);	
	if (!activity) return 0;
		
	graphClear(graph, 0);
	
	TGRAPHSHEET *shtCad = graphSheetAcquire(graph, NULL, sheets->cadence.id);
	TGRAPHSHEET *shtAlt = graphSheetAcquire(graph, NULL, sheets->altitude.id);
	TGRAPHSHEET *shtHr  = graphSheetAcquire(graph, NULL, sheets->heartRate.id);
	TGRAPHSHEET *shtSpd = graphSheetAcquire(graph, NULL, sheets->speed.id);
	TGRAPHSHEET *shtPwr = graphSheetAcquire(graph, NULL, sheets->power.id);
	
	
	int hasAltitude = 0;
	int hasSpeed = 0;		// (distanceMeters)
	int hasPower = 0;
	int hasHeartRate = 0;
	int hasCadence = 0;
	
	for (int l = 0; l < activity->laps.total; l++){
		tcx_lap *lap = activity->laps.list[l];
		
		if (lap->heartRate.average > 0) hasHeartRate = 1;
		if (lap->cadence.average > 0) hasCadence = 1;
	
		for (int t = 0; t < lap->tracks.total; t++){
			tcx_track *track = lap->tracks.list[t];

			for (int p = 0; p < track->trackPoints.total; p++){
				tcx_trackpoint *tp = track->trackPoints.list[p];

				if (tp->altitude > 0.001 || tp->altitude < -0.001){
					//printf("tp->altitude %f %f\n", tp->speed, tp->altitude);
					
					graphSheetAddData(shtAlt, tp->altitude * sheets->altitude.multiplier, 0);

					if (tp->speed >= 0.0){
						double speed = tp->speed;
						if (speed <= 0.1) speed = 0.1;
						else if (speed >= 100.0) speed = 0.0;
						graphSheetAddData(shtSpd, speed * sheets->speed.multiplier, 0);
					}
                	
					if (hasCadence && tp->cadence > 10)
						graphSheetAddData(shtCad, tp->cadence * sheets->cadence.multiplier, 0);
                	
					if (hasHeartRate && tp->heartRate > 20){
						int hr = tp->heartRate;
						if (hr < 30 || hr > 240) hr = 30;
						graphSheetAddData(shtHr, hr * sheets->heartRate.multiplier, 0);
					}
				//}else{
				//	printf("tp->speed %f %f\n", tp->speed, tp->altitude);
				}
			}
		}
	}

	hasAltitude = graphSheetDataGetLength(shtAlt) > 1;
	hasSpeed = graphSheetDataGetLength(shtSpd) > 1;
	
	shtAlt->render.enabled = hasAltitude;
	shtSpd->render.enabled = hasSpeed;
	shtHr->render.enabled = hasHeartRate;
	shtCad->render.enabled = hasCadence;
	shtPwr->render.enabled = hasPower;

#if 0
	printf("shtAlt length %i (%i)\n", graphSheetDataGetLength(shtAlt), shtAlt->render.enabled);
	printf("shtSpd length %i (%i)\n", graphSheetDataGetLength(shtSpd), shtSpd->render.enabled);
	printf("shtCad length %i (%i)\n", graphSheetDataGetLength(shtCad), shtCad->render.enabled);
	printf("shtHr  length %i (%i)\n", graphSheetDataGetLength(shtHr), shtHr->render.enabled);
	//printf("shtPwr length %i (%i)\n", graphSheetDataGetLength(shtPwr), shtPwr->render.enabled);
#endif

	graphSheetRelease(shtSpd);
	graphSheetRelease(shtAlt);
	graphSheetRelease(shtCad);
	graphSheetRelease(shtHr);
	graphSheetRelease(shtPwr);
	
	return 1;
}

static inline int gaminTcxSheetsSetUI (TGRAPH *graph, struct tcxsheets *sheets, int x, int y)
{

	
	const int hpitch = CUBESIZE + 20;
	TGRAPHSHEET *shtCad = graphSheetAcquire(graph, NULL, sheets->cadence.id);
	TGRAPHSHEET *shtAlt = graphSheetAcquire(graph, NULL, sheets->altitude.id);
	TGRAPHSHEET *shtHr  = graphSheetAcquire(graph, NULL, sheets->heartRate.id);
	TGRAPHSHEET *shtSpd = graphSheetAcquire(graph, NULL, sheets->speed.id);
	TGRAPHSHEET *shtPwr = graphSheetAcquire(graph, NULL, sheets->power.id);
	

	if (graphSheetDataGetLength(shtPwr) > 1){
		//if (shtPwr->render.enabled)
			ccEnable(sheets->power.ui.view.label);
		ccSetPosition(sheets->power.ui.view.label, x -= hpitch, y);
	}else{
		ccDisable(sheets->power.ui.view.label);
	}
	
	if (graphSheetDataGetLength(shtCad) > 1){
		//if (shtCad->render.enabled)
			ccEnable(sheets->cadence.ui.view.label);
		ccSetPosition(sheets->cadence.ui.view.label, x -= hpitch, y);
	}else{
		ccDisable(sheets->cadence.ui.view.label);
	}
	
	if (graphSheetDataGetLength(shtHr) > 1){
		//if (shtHr->render.enabled)
			ccEnable(sheets->heartRate.ui.view.label);
		ccSetPosition(sheets->heartRate.ui.view.label, x -= hpitch, y);
	}else{
		ccDisable(sheets->heartRate.ui.view.label);
	}
	
	if (graphSheetDataGetLength(shtSpd) > 1){
		//if (shtSpd->render.enabled)
			ccEnable(sheets->speed.ui.view.label);
		ccSetPosition(sheets->speed.ui.view.label, x -= hpitch, y);
	}else{
		ccDisable(sheets->speed.ui.view.label);
	}
	
	if (graphSheetDataGetLength(shtAlt) > 1){
		//if (shtAlt->render.enabled)
			ccEnable(sheets->altitude.ui.view.label);
		ccSetPosition(sheets->altitude.ui.view.label, x -= hpitch, y);
	}else{
		ccDisable(sheets->altitude.ui.view.label);
	}
	
	graphSheetRelease(shtSpd);
	graphSheetRelease(shtAlt);
	graphSheetRelease(shtCad);
	graphSheetRelease(shtHr);
	graphSheetRelease(shtPwr);
					
	return 1;
}

static inline int garminImportActivities (TTCX *tcx, const char *file)
{
	tcx_activities *activities = garminTcxLoad(file);
	if (activities && activities->activities.total){
		int width = getFrontBuffer(tcx->com->vp)->width;
		int height = getFrontBuffer(tcx->com->vp)->height;
	
		tcx_renderContext *renderContext = garminTcxGenerateRenderContext(tcx, activities, width, height);
		if (renderContext){
			int totalActivities = 0;
			
			if (renderLock(tcx->com->vp)){
				if (ccLock(tcx->graph)){
					if (tcx->renderContext) tcx_closeRenderContext(tcx->renderContext);
					tcx->renderContext = renderContext;
					if (tcx->activities) tcx_close(tcx->activities);
					tcx->activities = activities;

					garminTcxBuildGraphs(activities, 1, tcx->graph, &tcx->sheets);
					gaminTcxSheetsSetUI(tcx->graph, &tcx->sheets, width-64, 2);
					totalActivities = activities->activities.total;
					ccUnlock(tcx->graph);
				}
				renderUnlock(tcx->com->vp);
			}
			return totalActivities;
		}
	}
	return 0;
}


static inline int64_t info_pane_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TPANE *pane = (TPANE*)object;
	TTCX *tcx = ccGetUserData(pane);
	
	
	/*if (msg == CC_MSG_RENDER){
		TFRAME *frame = dataPtr;
		
		int y = ccGetPositionY(tcx->graph) + ccGetHeight(tcx->graph) + 16;
		double distH = frame->width * tcx->route.ruler.metersPerPixelH / 1000.0;
		double distV = frame->height * tcx->route.ruler.metersPerPixelV / 1000.0;
		
		drawVal_2(tcx, frame, frame->width - 250, y, COL_WHITE,		\
			"Area:\n\tHorizontal: %.1f Km\n\tVertical: %.1f Km", distH, distV);

	}else */if (msg == CC_MSG_ENABLED){
		tcx_activity *act = tcx_getActivity(tcx->activities, 1);
		if (act){
			if (tcx->route.file){
				paneRemoveAll(tcx->info.pane);
				
				infoAddItem(tcx, tcx->route.file);
				infoAddItemEx(tcx, "Date: %s", act->id);
				tcx_time t = {0};
				tcx_time64ToTime(act->startTime64, &t);
				infoAddItemEx(tcx, "Start time: %i:%.2i:%.2i", t.hour, t.min, t.sec);
				
				tcx_time64ToTime(act->time, &t);
				infoAddItemEx(tcx, "Total time: %i:%.2i:%.2i", t.hour, t.min, t.sec);
				
				infoAddItemEx(tcx, "Distance: %.2f km", act->distance/1000.0);
				infoAddItemEx(tcx, "Speed: %.2f km/h", act->stats.speed.ave);
				if (act->cadence.mode)
					infoAddItemEx(tcx, "Cadence: %i", act->cadence.mode);
				infoAddItemEx(tcx, "Laps: %i", act->laps.total);
				infoAddItemEx(tcx, "Sport: %s", act->name);
			}
		}
	}else if (msg == CC_MSG_DISABLED){
		paneRemoveAll(tcx->info.pane);
	}
	
	
	return 1;
}

static inline int64_t cc_graph_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//printf("ccGraph_cb in %p, %i %I64d %I64d %p\n", object, msg, data1, data2, dataPtr);
	
	if (msg == CC_MSG_HOVER){
		TTCX *tcx = ccGetUserData((void*)object);
		tcx->drawCursorTimeout = TCX_CURSOR_TIMEOUT;
	}
	
	return 1;
}

static inline int64_t cc_label_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER) return 1;
	
	TLABEL *label = (TLABEL*)object;
	TTCX *tcx = ccGetUserData(label);
		
	//printf("cc_label_cb in %p, %i %I64d %I64d %p, %p\n", object, msg, data1, data2, dataPtr, tcx->background.label);

	if (!tcx->renderContext) return 1;
	
	if (label->id == tcx->background.id){
		if (msg == LABEL_MSG_BASE_SELECTED_PRESS){
			//printf("LABEL_MSG_BASE_SELECTED_PRESS\n");
			
			if (tcx->route.render.enabled > 0){
				tcx->slideState = SLIDE_HELD;
				tcx->route.offset.startX = (data1>>16)&0xFFFF;
				tcx->route.offset.startY = data1&0xFFFF;
				tcx_screen(tcx->renderContext, tcx->route.offset.x, tcx->route.offset.y);
			}
		}else if (msg == LABEL_MSG_BASE_SELECTED_SLIDE){
			//printf("LABEL_MSG_BASE_SELECTED_SLIDE\n");
			
			// generate a small deadzone, route movement shouldn't be active within this space
			int x  = ((data1>>16)&0xFFFF);
			int y = (data1&0xFFFF);
			if (y <= 72) return 0;
			
			if (tcx->slideState){
				tcx->slideState = SLIDE_SLIDING;
				tcx->route.offset.deltaX = tcx->route.offset.startX - x;
				tcx->route.offset.deltaY = tcx->route.offset.startY - y;

				double x = tcx->route.offset.x - tcx->route.offset.deltaX;
				double y = tcx->route.offset.y - tcx->route.offset.deltaY;
				if (fabs(x) >= 2 && fabs(y) >= 2){
#if 1
					tcx->route.offset.modified.x = x;
					tcx->route.offset.modified.y = y;
					tcx->route.offset.modified.ready = 1;
#else
					tcx_screen(tcx->renderContext, x, y);
#endif
				}
			}
		}else if (msg == LABEL_MSG_BASE_SELECTED_RELEASE){
			//printf("LABEL_MSG_BASE_SELECTED_RELEASE\n");
			if (tcx->slideState == SLIDE_SLIDING){
				tcx->slideState = SLIDE_RELEASED;
				tcx->route.offset.x -= tcx->route.offset.deltaX;
				tcx->route.offset.y -= tcx->route.offset.deltaY;
				tcx_screen(tcx->renderContext, tcx->route.offset.x, tcx->route.offset.y);
				
				tcx->route.offset.startX = 0;
				tcx->route.offset.startY = 0;
				tcx->route.offset.deltaX = 0;
				tcx->route.offset.deltaY = 0;
				pageUpdate(tcx);
			}else{
				if (tcx->slideState == SLIDE_HELD && tcx->route.render.cursorMode == 3){
					// viewport of image map
					/*map.nw.longitude = -6.317782402038574;
					map.nw.latitude = 54.85379508572291;
					map.se.longitude = -5.629377365112305;
					map.se.latitude = 54.413486043109486;
					map.width = 4008;
					map.height = 4434;
					
					calcMapWindow(tcx, &map, &window);
					printf("box: %i,%i %i,%i\n", window.x1, window.y1, window.x2, window.y2);
					windowSet = 1;*/
										
					if (tcx->route.cursor.location.longitude == tcx->route.location.longitude &&
						tcx->route.cursor.location.latitude == tcx->route.location.latitude)
					{
						tcx->route.location.longitude = 0.0;
						tcx->route.location.latitude = 0.0;
						buttonFaceTextUpdate(tcx->ui.onlineMap, BUTTON_PRI, " ");
						//buttonFaceTextUpdate(tcx->ui.onlineAddress, BUTTON_PRI, " ");
						
					}else{
						tcx->route.location.longitude = tcx->route.cursor.location.longitude;
						tcx->route.location.latitude = tcx->route.cursor.location.latitude;
	
						char buffer[64];
						__mingw_snprintf(buffer, sizeof(buffer), "%f, %f", tcx->route.location.latitude, tcx->route.location.longitude);
						clipboardSend(tcx->com->vp, buffer);
						buttonFaceTextUpdate(tcx->ui.onlineMap, BUTTON_PRI, buffer);
						//buttonFaceTextUpdate(tcx->ui.onlineAddress, BUTTON_PRI, " Find address");
					}
					pageUpdate(tcx);
				}
			
				tcx->slideState = SLIDE_RELEASED;
				tcx->route.offset.deltaX = 0;
				tcx->route.offset.deltaY = 0;
#if 0				
				if (tcx->route.cursorTrkPtIdx >= 0){
					int activity = ((tcx->route.cursorTrkPtData>>48) & 0xFFFF)+1;
					
					TGRAPH *graph = tcx->graph;
					struct tcxsheets *sheets = &tcx->sheets;
					TGRAPHSHEET *shtCad = graphSheetAcquire(graph, NULL, sheets->cadence.id);
					TGRAPHSHEET *shtAlt = graphSheetAcquire(graph, NULL, sheets->altitude.id);
					TGRAPHSHEET *shtHr  = graphSheetAcquire(graph, NULL, sheets->heartRate.id);
					TGRAPHSHEET *shtSpd = graphSheetAcquire(graph, NULL, sheets->speed.id);
					TGRAPHSHEET *shtPwr = graphSheetAcquire(graph, NULL, sheets->power.id);
	
					int enabled[6];	
					enabled[0] = shtAlt->render.enabled;
					enabled[1] = shtSpd->render.enabled;
					enabled[2] = shtHr->render.enabled;
					enabled[3] = shtCad->render.enabled;
					enabled[4] = shtPwr->render.enabled;
					
					if (garminTcxBuildGraphs(tcx->activities, activity, tcx->graph, &tcx->sheets))
						gaminTcxSheetsSetUI(tcx->graph, &tcx->sheets, ccGetWidth(tcx->graph)-64, 2);
						
					shtAlt->render.enabled = enabled[0];
					shtSpd->render.enabled = enabled[1];
					shtHr->render.enabled = enabled[2];
					shtCad->render.enabled = enabled[3];
					shtPwr->render.enabled = enabled[4];
					
					graphSheetRelease(shtSpd);
					graphSheetRelease(shtAlt);
					graphSheetRelease(shtCad);
					graphSheetRelease(shtHr);
					graphSheetRelease(shtPwr);
				}
#endif
			}
		}
	}else if (msg == LABEL_MSG_BASE_SELECTED_PRESS){
		int sheetId = ccGetUserDataInt(label);
		if (sheetId > 0){
			TGRAPHSHEET *sheet = graphSheetAcquire(tcx->graph, NULL, sheetId);
			if (sheet){
				sheet->render.enabled ^= 0x1;
				graphSheetRelease(sheet);
			}
			tcx->route.render.sheets = graphRenderGetSheetCount(tcx->graph);
		}
	}

	return 1;
}

static inline int64_t cc_btn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (msg == CC_MSG_RENDER) return 1;
	
	//printf("cc_btn_cb  in %p, %i %I64d %I64d %p\n", object, msg, data1, data2, dataPtr);
	
	if (msg == CC_MSG_ENABLED){
		TCCBUTTON *btn = (TCCBUTTON*)object;
		TTCX *tcx = ccGetUserData(btn);

		// don't handle one before the other is init'd
		if (!tcx->ui.info || !tcx->info.pane)
			return 1;
		
		if (tcx->ui.info->id == btn->id){
			if (ccGetState(tcx->info.pane)){	// update contents of info box
				ccDisable(tcx->info.pane);
				ccEnable(tcx->info.pane);
			}
		}

	}else if (msg == BUTTON_MSG_SELECTED_PRESS){
		TCCBUTTON *btn = (TCCBUTTON*)object;
		TTCX *tcx = ccGetUserData(btn);
		tcx->t0 = getTickCount();
		
		if (tcx->ui.info->id == btn->id){
			if (ccGetState(tcx->info.pane))
				ccDisable(tcx->info.pane);
			else
				ccEnable(tcx->info.pane);
				
		}else if (tcx->ui.cursorModeA->id == btn->id){
			ccDisable(tcx->ui.cursorModeA);
			ccEnable(tcx->ui.cursorModeB);
			ccDisable(tcx->ui.onlineMap);
			ccDisable(tcx->ui.onlineAddress);
			
			tcx->route.render.cursorMode = 2;
			
		}else if (tcx->ui.cursorModeB->id == btn->id){
			ccDisable(tcx->ui.cursorModeB);
			ccEnable(tcx->ui.cursorModeC);
			ccEnable(tcx->ui.onlineMap);
			ccEnable(tcx->ui.onlineAddress);
						
			tcx->route.render.cursorMode = 3;
			
		}else if (tcx->ui.cursorModeC->id == btn->id){
			ccDisable(tcx->ui.cursorModeC);
			ccEnable(tcx->ui.cursorModeA);
			ccDisable(tcx->ui.onlineMap);
			ccDisable(tcx->ui.onlineAddress);
						
			tcx->route.render.cursorMode = 1;
			//printf("tcx->route.render.cursorMode %i %i\n", activeFace , tcx->route.render.cursorMode);

		}else if (tcx->ui.onlineAddress->id == btn->id){
			tcx_point pt = {tcx->route.location.latitude, tcx->route.location.longitude};
			char *info = geoCodeReserveLookup(&pt);
			if (info){
				//printf("#%s#\n", info);
				buttonFaceTextUpdate(tcx->ui.onlineAddress, BUTTON_PRI, info);
				clipboardSend(tcx->com->vp, info);
				my_free(info);
			}			
		}else if (tcx->ui.onlineMap->id == btn->id){
			if (tcx->route.location.latitude != 0.0 && tcx->route.location.longitude != 0.0){
				char buffer[MAX_PATH_UTF8];
				//__mingw_snprintf(buffer, MAX_PATH_UTF8, mapURL, mapApplication, tcx->route.location.latitude, tcx->route.location.longitude, tcx->route.location.latitude, tcx->route.location.longitude, 15.5);
				__mingw_snprintf(buffer, MAX_PATH_UTF8, mapURL, mapApplication, tcx->route.location.latitude, tcx->route.location.longitude, 15.5, tcx->route.location.latitude, tcx->route.location.longitude);
				//printf("process #%s#\n", buffer);
				processCreate(buffer);
			}
		}else if (tcx->ui.route->id == btn->id){
			if (tcx->route.render.enabled == -1) tcx->route.render.enabled = 0;
			tcx->route.render.enabled = !tcx->route.render.enabled;
			
			if (tcx->route.render.enabled){
				ccEnable(tcx->ui.zoomIn);
				ccEnable(tcx->ui.zoomOut);
				ccEnable(tcx->ui.zoomReset);
			}else{
				ccDisable(tcx->ui.zoomIn);
				ccDisable(tcx->ui.zoomOut);
				ccDisable(tcx->ui.zoomReset);
			}
		}else if (tcx->ui.zoomIn->id == btn->id){
			tcx->route.scale.factor /= tcx->route.scale.multiplier;
			tcx->route.offset.x /= tcx->route.scale.multiplier;
			tcx->route.offset.y /= tcx->route.scale.multiplier;
			garminTcxRescale(tcx, tcx->renderContext, tcx->activities, tcx->route.scale.factor, tcx->route.offset.x, tcx->route.offset.y);
				
		}else if (tcx->ui.zoomOut->id == btn->id){
			tcx->route.scale.factor *= tcx->route.scale.multiplier;
			tcx->route.offset.x *= tcx->route.scale.multiplier;
			tcx->route.offset.y *= tcx->route.scale.multiplier;
			garminTcxRescale(tcx, tcx->renderContext, tcx->activities, tcx->route.scale.factor, tcx->route.offset.x, tcx->route.offset.y);
			
		}else if (tcx->ui.zoomReset->id == btn->id){
			garminTcxResetScale(tcx);
			garminTcxRescale(tcx, tcx->renderContext, tcx->activities, tcx->route.scale.factor, tcx->route.offset.x, tcx->route.offset.y);
			
		}else if (tcx->ui.import->id == btn->id){
			if (tcx->route.file){
				char *dir = getDirectory(tcx->route.file);
				if (dir){
					TFILEPANE *filepane = pageGetPtr(tcx->com->vp, PAGE_FILE_PANE);
					filepaneSetFilterMask(filepane, FILEMASKS_ALL);
					filepaneSetPath(filepane, dir);
					pageSet(tcx->com->vp, PAGE_FILE_PANE);
					my_free(dir);
					return 1;
				}
			}
			pageSet(tcx->com->vp, PAGE_EXP_PANEL);
		}
	}

	return 1;
}



static inline int page_tcxStartup (TTCX *tcx, TVLCPLAYER *vp, const int width, const int height)
{
	if (!garminConfigIsTcxEnabled(tcx))
		return 0;

	tcx->strc.bin = strcNew(vp->ml->hw);
	if (!tcx->strc.bin) return 0;

	str_list *clrList = NULL;
	settingsGet(vp, "tcx.route.colour.", &clrList);
	if (clrList){
		tcx->route.render.colours.total = clrList->total;
		for (int i = 0; i < clrList->total && i < 32; i++)
			tcx->route.render.colours.list[i] = hexToInt(cfg_configStrListItem(clrList,i));
				
		cfg_configStrListFree(clrList);
		my_free(clrList);
	}

	tcx->graph = ccCreateEx(vp->cc, PAGE_TCX, CC_GRAPH, cc_graph_cb, NULL, width, 300, tcx);
	ccSetPosition(tcx->graph, 0, (height - ccGetHeight(tcx->graph))/2);
	ccSetPosition(tcx->graph, 0, 72);
	
	tcx->sheets.altitude.id = graphNewSheet(tcx->graph, "AltitudeMeters", width);
	tcx->sheets.altitude.multiplier = 100.0;
	tcx->sheets.altitude.colour = 225<<24|COL_GREEN;
	
	tcx->sheets.cadence.id = graphNewSheet(tcx->graph, "Cadence", width);
	tcx->sheets.cadence.multiplier = 1.0;
	tcx->sheets.cadence.colour = 240<<24|COL_YELLOW;
	
	tcx->sheets.heartRate.id = graphNewSheet(tcx->graph, "HeartRateBpm", width);
	tcx->sheets.heartRate.multiplier = 1.0;
	tcx->sheets.heartRate.colour = 220<<24|COL_RED;
	
	tcx->sheets.speed.id = graphNewSheet(tcx->graph, "DistanceMeters", width);
	tcx->sheets.speed.multiplier = 10.0;
	tcx->sheets.speed.colour = 250<<24|COL_AQUA;

	tcx->sheets.power.id = graphNewSheet(tcx->graph, "PowerMeter", width);
	tcx->sheets.power.multiplier = 1.0;
	tcx->sheets.power.colour = 220<<24|COL_BLUE;


	//const int CUBESIZE = 32;
	
	TLABEL *label = ccCreateEx(vp->cc, PAGE_TCX, CC_LABEL, cc_label_cb, NULL, 0, 0, tcx);
	labelBaseColourSet(label, tcx->sheets.altitude.colour);
	ccSetMetrics(label, 0, 0, CUBESIZE, CUBESIZE);
	label->renderflags = LABEL_RENDER_BASE;
	tcx->sheets.altitude.ui.view.label = label;
	ccSetUserDataInt(label, tcx->sheets.altitude.id);
	
	label = ccCreateEx(vp->cc, PAGE_TCX, CC_LABEL, cc_label_cb, NULL, 0, 0, tcx);
	labelBaseColourSet(label, tcx->sheets.speed.colour);
	ccSetMetrics(label, 0, 0, CUBESIZE, CUBESIZE);
	label->renderflags = LABEL_RENDER_BASE;
	tcx->sheets.speed.ui.view.label = label;
	ccSetUserDataInt(label, tcx->sheets.speed.id);
	
	label = ccCreateEx(vp->cc, PAGE_TCX, CC_LABEL, cc_label_cb, NULL, 0, 0, tcx);
	labelBaseColourSet(label, tcx->sheets.heartRate.colour);
	ccSetMetrics(label, 0, 0, CUBESIZE, CUBESIZE);
	label->renderflags = LABEL_RENDER_BASE;
	tcx->sheets.heartRate.ui.view.label = label;
	ccSetUserDataInt(label, tcx->sheets.heartRate.id);
	
	label = ccCreateEx(vp->cc, PAGE_TCX, CC_LABEL, cc_label_cb, NULL, 0, 0, tcx);
	labelBaseColourSet(label, tcx->sheets.cadence.colour);
	ccSetMetrics(label, 0, 0, CUBESIZE, CUBESIZE);
	label->renderflags = LABEL_RENDER_BASE;
	tcx->sheets.cadence.ui.view.label = label;
	ccSetUserDataInt(label, tcx->sheets.cadence.id);
	
	label = ccCreateEx(vp->cc, PAGE_TCX, CC_LABEL, cc_label_cb, NULL, 0, 0, tcx);
	labelBaseColourSet(label, tcx->sheets.power.colour);
	ccSetMetrics(label, 0, 0, CUBESIZE, CUBESIZE);
	label->renderflags = LABEL_RENDER_BASE;
	tcx->sheets.power.ui.view.label = label;
	ccSetUserDataInt(label, tcx->sheets.power.id);

	
	TCCBUTTON *btn = ccCreateEx(vp->cc, PAGE_TCX, CC_BUTTON, cc_btn_cb, NULL, 0, 0, tcx);
	tcx->ui.route = btn;
	buttonFacePathSet(btn, L"tcx/route.png", NULL, 0, 0);
	ccSetPosition(btn, width - ccGetWidth(btn) - 16, 2);
	ccEnable(btn);
	
	btn = ccCreateEx(vp->cc, PAGE_TCX, CC_BUTTON, cc_btn_cb, NULL, 0, 0, tcx);
	tcx->ui.import = btn;
	buttonFacePathSet(btn, L"tcx/import.png", NULL, 0, 0);
	buttonFaceHoverSet(btn, 1, COL_HOVER, 0.8);
	ccEnable(btn);

	btn = ccCreateEx(vp->cc, PAGE_TCX, CC_BUTTON, cc_btn_cb, NULL, 0, 0, tcx);
	tcx->ui.info = btn;
	buttonFacePathSet(btn, L"tcx/info.png", NULL, 0, 0);
	buttonFaceHoverSet(btn, 1, COL_HOVER, 0.8);
	ccSetPosition(btn, ccGetPositionX(tcx->ui.import) + ccGetWidth(tcx->ui.import) + 16, 0);

	btn = ccCreateEx(vp->cc, PAGE_TCX, CC_BUTTON, cc_btn_cb, NULL, 0, 0, tcx);
	tcx->ui.cursorModeA = btn;
	buttonFacePathSet(btn, L"tcx/cursormodea.png", NULL, 0, 0);
	buttonFaceHoverSet(btn, 1, COL_HOVER, 0.8);
	ccSetPosition(btn, ccGetPositionX(tcx->ui.info) + ccGetWidth(tcx->ui.info) + 16, 0);
	ccEnable(btn);
		
	btn = ccCreateEx(vp->cc, PAGE_TCX, CC_BUTTON, cc_btn_cb, NULL, 0, 0, tcx);
	tcx->ui.cursorModeB = btn;
	buttonFacePathSet(btn, L"tcx/cursormodeb.png", NULL, 0, 0);
	buttonFaceHoverSet(btn, 1, COL_HOVER, 0.8);
	ccSetPosition(btn, ccGetPositionX(tcx->ui.info) + ccGetWidth(tcx->ui.info) + 16, 0);
	//ccEnable(btn);

	btn = ccCreateEx(vp->cc, PAGE_TCX, CC_BUTTON, cc_btn_cb, NULL, 0, 0, tcx);
	tcx->ui.cursorModeC = btn;
	buttonFacePathSet(btn, L"tcx/cursormodec.png", NULL, 0, 0);
	buttonFaceHoverSet(btn, 1, COL_HOVER, 0.8);
	ccSetPosition(btn, ccGetPositionX(tcx->ui.info) + ccGetWidth(tcx->ui.info) + 16, 0);
	//ccDisable(btn);

	btn = ccCreateEx(vp->cc, PAGE_TCX, CC_BUTTON, cc_btn_cb, NULL, 0, 0, tcx);
	tcx->ui.onlineMap = btn;
	buttonFacePathSet(btn, L"tcx/gotomap.png", NULL, 0, 0);
	buttonFaceHoverSet(btn, 0, COL_HOVER, 0.8);
	buttonAnimateSet(btn, 0);
	buttonFaceTextSet(btn, BUTTON_PRI, "  ", PF_IGNOREFORMATTING, MFONT, 4, 18);
	ccSetPosition(btn, 4, height - 112);
	
	btn = ccCreateEx(vp->cc, PAGE_TCX, CC_BUTTON, cc_btn_cb, NULL, 0, 0, tcx);
	tcx->ui.onlineAddress = btn;
	buttonFacePathSet(btn, L"tcx/address.png", NULL, 0, 0);
	buttonFaceHoverSet(btn, 0, COL_HOVER, 0.8);
	buttonAnimateSet(btn, 0);
	buttonFaceTextSet(btn, BUTTON_PRI, " Find address", PF_IGNOREFORMATTING, MFONT, 4, 12);
	ccSetPosition(btn, 4, height - 55);
	//btn->btnlbl[0]->label->renderflags = LABEL_RENDER_TEXT;
	
	
	btn = ccCreateEx(vp->cc, PAGE_TCX, CC_BUTTON, cc_btn_cb, NULL, 0, 0, tcx);
	tcx->ui.zoomOut = btn;
	buttonFacePathSet(btn, L"tcx/zoomout.png", NULL, 0, 0);
	buttonAnimateSet(btn, 1);
	buttonFaceHoverSet(btn, 1, COL_HOVER, 0.8);
	ccSetPosition(btn, (width - ccGetWidth(btn))/2, 2);
	ccEnable(btn);

	btn = ccCreateEx(vp->cc, PAGE_TCX, CC_BUTTON, cc_btn_cb, NULL, 0, 0, tcx);
	tcx->ui.zoomIn = btn;
	buttonFacePathSet(btn, L"tcx/zoomin.png", NULL, 0, 0);
	buttonAnimateSet(btn, 1);
	buttonFaceHoverSet(btn, 1, COL_HOVER, 0.8);
	ccSetPosition(btn, ccGetPositionX(tcx->ui.zoomOut) - ccGetWidth(btn) - 16, 2);
	ccEnable(btn);
		
	btn = ccCreateEx(vp->cc, PAGE_TCX, CC_BUTTON, cc_btn_cb, NULL, 0, 0, tcx);
	tcx->ui.zoomReset = btn;
	buttonFacePathSet(btn, L"tcx/zoomreset.png", NULL, 0, 0);
	buttonAnimateSet(btn, 1);
	buttonFaceHoverSet(btn, 1, COL_HOVER, 0.8);
	ccSetPosition(btn, ccGetPositionX(tcx->ui.zoomOut) + ccGetWidth(tcx->ui.zoomOut) + 16, 2);
	ccEnable(btn);

	tcx->background.label = ccCreateEx(vp->cc, PAGE_TCX, CC_LABEL, cc_label_cb, &tcx->background.id, 0, 0, tcx);
	ccSetUserDataInt(tcx->background.label, -1);
	ccSetMetrics(tcx->background.label, 0, 0, width, height);
	//ccInputDisable(tcx->background.label);
	labelRenderFlagsSet(tcx->background.label, 0);
	ccEnable(tcx->background.label);
	tcx->background.label->canDrag = 1;


	TPANE *pane = ccCreateEx(vp->cc, PAGE_TCX, CC_PANE, info_pane_cb, NULL, width-(8*1), 260, tcx);
	tcx->info.pane = pane;
	tcx->info.font = MFONT;
	ccSetPosition(pane, 8, 80);
	paneSetAcceleration(pane, 0.0, 0.0);
	paneSetLayout(pane, PANE_LAYOUT_VERT);
	pane->horiColumnSpace = 0;
	pane->flags.readAhead.enabled = 0;
	pane->vertLineHeight = 24;
	ccInputDisable(pane);
	ccInputSlideHoverDisable(pane);
	paneSwipeDisable(pane);
	labelBaseColourSet(pane->base, 80<<24 | COL_ORANGE);
	labelRenderFlagsSet(pane->base, LABEL_RENDER_TEXT/*|LABEL_RENDER_BASE*/);
	return 1;
}

static inline int page_tcxInitalize (TTCX *tcx, TVLCPLAYER *vp, const int width, const int height)
{
	
	wchar_t bufferw[MAX_PATH+1];
	tcx->crosshair = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"tcx/crosshair.png"));
	tcx->mapMarker = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"tcx/mapmarker.png"));
	
	TGRAPHSHEET *sheet = graphSheetAcquire(tcx->graph, NULL, tcx->sheets.altitude.id);
	if (sheet){
		sheet->stats.min = 99999;
		sheet->stats.max = -99999;
		sheet->stats.floor = 0;
		sheet->stats.ceiling = 264.0 * tcx->sheets.altitude.multiplier;
		sheet->render.mode = GRAPH_SCOPE | GRAPH_AUTOSCALE;
		sheet->render.scale = 0.0;
		//sheet->render.scale = (sheet->render.metrics.height / (double)sheet->stats.max) / tcx->sheets.altitude.multiplier;
		sheet->render.hints = GRAPH_HINT_SHADOW1|GRAPH_HINT_SCOPE_FILL;
		sheet->render.palette[GRAPH_PAL_SCOPE] = tcx->sheets.altitude.colour;
		sheet->render.palette[GRAPH_PAL_SHADOW1] = 80<<24|(tcx->sheets.altitude.colour&0xFFFFFF);
		sheet->render.palette[GRAPH_PAL_SHADOW2] = 80<<24|(tcx->sheets.altitude.colour&0xFFFFFF);
		sheet->graph.canGrow = 1;
		graphSheetRelease(sheet);
	}

	sheet = graphSheetAcquire(tcx->graph, NULL, tcx->sheets.speed.id);
	if (sheet){
		sheet->render.mode = GRAPH_SCOPE;
		sheet->render.scale = 0.25;
		sheet->render.hints = GRAPH_HINT_SHADOW1|GRAPH_HINT_SHADOW2;
		sheet->render.palette[GRAPH_PAL_SCOPE] = tcx->sheets.speed.colour;
		sheet->render.palette[GRAPH_PAL_POLYLINE] = 240<<24|COL_AQUA;
		sheet->stats.min = 0;
		sheet->stats.max = 00;
		sheet->graph.canGrow = 1;
		graphSheetRelease(sheet);
	}

	sheet = graphSheetAcquire(tcx->graph, NULL, tcx->sheets.heartRate.id);
	if (sheet){
		sheet->render.mode = GRAPH_SCOPE | GRAPH_AUTOSCALE;
		sheet->render.scale = 1.0;
		sheet->render.hints = GRAPH_HINT_SHADOW1;
		sheet->render.palette[GRAPH_PAL_SCOPE] = tcx->sheets.heartRate.colour;
		sheet->render.palette[GRAPH_PAL_POLYLINE] = 240<<24|COL_RED;
		sheet->stats.min = 50;
		sheet->stats.max = 190;
		sheet->graph.canGrow = 1;
		graphSheetRelease(sheet);
	}
		
	sheet = graphSheetAcquire(tcx->graph, NULL, tcx->sheets.cadence.id);
	if (sheet){
		sheet->render.mode = GRAPH_SCOPE/* | GRAPH_AUTOSCALE*/;
		sheet->render.scale = 1.20;
		sheet->render.hints = GRAPH_HINT_SHADOW1;
		sheet->render.palette[GRAPH_PAL_SCOPE] = tcx->sheets.cadence.colour;
		sheet->render.palette[GRAPH_PAL_POLYLINE] = 240<<24|COL_YELLOW;
		sheet->stats.min = 0;
		sheet->stats.max = 0;
		sheet->graph.canGrow = 1;
		graphSheetRelease(sheet);
	}

	sheet = graphSheetAcquire(tcx->graph, NULL, tcx->sheets.power.id);
	if (sheet){
		sheet->render.mode = GRAPH_SCOPE | GRAPH_AUTOSCALE;
		sheet->render.scale = 1.0;
		sheet->render.hints = GRAPH_HINT_SHADOW1;
		sheet->render.palette[GRAPH_PAL_SCOPE] = tcx->sheets.power.colour;
		sheet->stats.min = 99999;
		sheet->stats.max = -99999;
		sheet->graph.canGrow = 1;
		graphSheetRelease(sheet);
	}

	settingsGet(vp, "tcx.background.colour", &tcx->background.colour);


	tcx->route.render.cursorMode = 1;
	tcx->route.cursorTrkPtIdx = -1;
	tcx->route.cursorTrkPtData = 0;
	
	return 1;
}

static inline int page_tcxShutdown (TTCX *tcx, TVLCPLAYER *vp)
{
	ccDelete(tcx->sheets.altitude.ui.view.label);
	ccDelete(tcx->sheets.speed.ui.view.label);
	ccDelete(tcx->sheets.heartRate.ui.view.label);
	ccDelete(tcx->sheets.cadence.ui.view.label);
	ccDelete(tcx->sheets.power.ui.view.label);
		
	ccDelete(tcx->background.label);
	ccDelete(tcx->ui.import);
	ccDelete(tcx->ui.route);
	ccDelete(tcx->ui.cursorModeA);
	ccDelete(tcx->ui.cursorModeB);
	ccDelete(tcx->ui.cursorModeC);
	ccDelete(tcx->ui.onlineMap);
	ccDelete(tcx->ui.onlineAddress);
	ccDelete(tcx->ui.info);
	ccDelete(tcx->ui.zoomIn);
	ccDelete(tcx->ui.zoomOut);
	ccDelete(tcx->ui.zoomReset);


	if (tcx->activities) tcx_close(tcx->activities);
	if (tcx->renderContext) tcx_closeRenderContext(tcx->renderContext);
	if (tcx->route.file) my_free(tcx->route.file);

	ccDelete(tcx->graph);
	ccDelete(tcx->info.pane);
	strcFree(tcx->strc.bin);
	
	return 1;
}

static inline int page_tcxInput (TTCX *tcx, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	//printf("page_tcxInput %i\n", msg);

	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
	  	tcx->route.scale.factor /= tcx->route.scale.multiplier;
		tcx->route.offset.x /= tcx->route.scale.multiplier;
		tcx->route.offset.y /= tcx->route.scale.multiplier;
		garminTcxRescale(tcx, tcx->renderContext, tcx->activities, tcx->route.scale.factor, tcx->route.offset.x, tcx->route.offset.y);
	  	 break;
	  case PAGE_IN_WHEEL_BACK:
		tcx->route.scale.factor *= tcx->route.scale.multiplier;
		tcx->route.offset.x *= tcx->route.scale.multiplier;
		tcx->route.offset.y *= tcx->route.scale.multiplier;
		garminTcxRescale(tcx, tcx->renderContext, tcx->activities, tcx->route.scale.factor, tcx->route.offset.x, tcx->route.offset.y);
		break;
	};
	return 1;
}

static inline int garminImportTcx (TTCX *tcx, const char *file)
{

	int activitiesFound = 0;
	if (isGaminFile8(file) && ((activitiesFound=garminImportActivities(tcx, file)))){
		if (tcx->route.file) my_free(tcx->route.file);
		tcx->route.file = my_strdup(file);
	}
	return activitiesFound;
}

static inline unsigned int __stdcall tcxLoaderThread (void *uptr)
{
	TCXTHREADOPAQUE *opaque = (TCXTHREADOPAQUE*)uptr;
	TTCX *tcx = opaque->tcx;

	int activitiesFound = garminImportTcx(tcx, opaque->file);
	if (activitiesFound){
		ccEnable(tcx->ui.info);
		if (tcx->route.render.enabled == -1)
			tcx->route.render.enabled = 1;
	}else if (!activitiesFound){
		dbprintf(tcx->com->vp, "No activities found in file");
	}

	my_free(opaque->file);
	my_free(opaque);

	pageUpdate(tcx);
	_endthreadex(1);
	return 1;
}

static inline void garminImportTcxAsync (TTCX *tcx, const char *file)
{
	unsigned int tid;
	TCXTHREADOPAQUE *opaque = my_malloc(sizeof(TCXTHREADOPAQUE));
	opaque->tcx = tcx;
	opaque->file = my_strdup(file);
	_beginthreadex(NULL, 0, tcxLoaderThread, opaque, 0, &tid);	
}

static inline int page_tcxRenderInit (TTCX *tcx, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	settingsGet(vp, "tcx.scale.multiplier", &tcx->route.scale.multiplier);
	if (tcx->route.scale.multiplier >= 1.0 || tcx->route.scale.multiplier < 0.1)	
		tcx->route.scale.multiplier = TCX_RENDER_SCALE_MULTIPLIER;

	settingsGet(vp, "tcx.scale.initial", &tcx->route.scale.initial);
	if (tcx->route.scale.initial >= 400.0 || tcx->route.scale.initial < 0.01)
		tcx->route.scale.initial = TCX_RENDER_SCALE_DEFAULT;
	tcx->route.scale.factor = tcx->route.scale.initial;	
		
	char *file = NULL;
	settingsGet(vp, "tcx.file", &file);
	if (!file) return 1;

	if (isGaminFile8(file))
		garminImportTcxAsync(tcx, file);
	my_free(file);

	tcx->route.render.enabled = -1;
	tcx->route.render.flags = TCX_RENDER_ROUTE_LINE | TCX_RENDER_ROUTE_SHADOW;
	//tcx->route.render.flags |= TCX_RENDER_ROUTE_POINT | TCX_RENDER_ROUTE_POINT_GLOW;// | TCX_RENDER_ROUTE_POINT_LARGE;
	tcx->route.render.flags |= TCX_RENDER_ROUTE_LAP_HIGHLIGHT;
	tcx->route.render.statsPosY = ccGetPositionY(tcx->graph) + ccGetHeight(tcx->graph) + 8;
	tcx->graphLapHighlight = -1;
	
	ccEnable(tcx->graph);
	//ccEnable(tcx->info.pane);
/*
	double d = calcDistKm(54.641637, -6.056273, 54.703819, -6.190091);
	double h = calcDistKm(54.701880, -6.093192, 54.701880, -5.937209);
	double v = calcDistKm(54.701880, -6.093192, 54.615684, -6.093192);
	
	
	printf("%f: %f %f\n", d, h, v);
	*/	
	return 1;
}

static inline int page_tcxOpenLocationFile (TTCX *tcx, const char *location, const char *file)
{
	if (!isGaminFile8(file)) return 1;
	
	pageSet(tcx->com->vp, PAGE_TCX);
	//dbprintf(tcx->com->vp, "Importing  ... '%s'", file);
	
	char buffer[MAX_PATH_UTF8+1];
	__mingw_snprintf(buffer, MAX_PATH_UTF8, "%s%s", location, file);

	garminImportTcxAsync(tcx, buffer);

	return 0;
}

static inline int page_tcxCfgWrite (TTCX *tcx, TVLCPLAYER *vp)
{
	if (tcx->route.file)
		settingsSet(vp, "tcx.file", tcx->route.file);

	return 1;
}

int page_tcxCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TPAGE2COMOBJ *page = (TPAGE2COMOBJ*)pageStruct;
	
	// printf("# page_tcxCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_tcxRender(pageStruct, dataPtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_tcxInput(pageStruct, page->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_START){
		return page_tcxRenderBegin(pageStruct, page->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		return page_tcxRenderEnd(pageStruct, page->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_tcxRenderInit(pageStruct, page->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_tcxStartup(pageStruct, page->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_tcxInitalize(pageStruct, page->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_tcxShutdown(pageStruct, page->com->vp);
		
	}else if (msg == PAGE_MSG_FILE_SELECTED){
		return page_tcxOpenLocationFile(pageStruct, (const char*)(intptr_t)dataInt1, (const char*)(intptr_t)dataInt2);
	
	}else if (msg == PAGE_MSG_CFG_WRITE){
		return page_tcxCfgWrite(pageStruct, page->com->vp);
	}
	
	return 1;
}


#endif

