
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



#ifndef _GARMIN_H_
#define _GARMIN_H_


#include "tcx.h"

#define TCX_CURSOR_TIMEOUT				15

#define TCX_RENDER_SCALE_DEFAULT		0.980
#define TCX_RENDER_SCALE_MULTIPLIER		0.85

#define TCX_RENDER_ROUTE_LINE			0x01		// connecting line
#define TCX_RENDER_ROUTE_SHADOW			0x02		// dark underlay
#define TCX_RENDER_ROUTE_POINT			0x04		// single pixel plot
#define TCX_RENDER_ROUTE_POINT_GLOW		0x08		// 4 point highlight/glow
#define TCX_RENDER_ROUTE_POINT_LARGE	0x10		// 2 band curcle
#define TCX_RENDER_ROUTE_LAP_HIGHLIGHT	0x20		// lap highlight


typedef struct {
	double latitude;
	double longitude;
}tcx_point;

typedef struct {
	tcx_point nw;		// top left
	tcx_point se;		// bottom right
	
	int width;			// image width
	int height;			// and height
}tcx_map;


struct sheet {
	int id;		// graph sheet id
	double multiplier;
	int colour;

	struct {
		struct {
			TLABEL *label;
			int itemId;
		}view;
	}ui;
};

struct tcxsheets {
	struct sheet altitude;
	struct sheet speed;
	struct sheet heartRate;
	struct sheet cadence;
	struct sheet power;		// power meter. unimplemented
};


typedef struct {
	TPAGE2COM *com;
	TGRAPH *graph;

	tcx_activities *activities;
	tcx_renderContext *renderContext;
	
	struct tcxsheets sheets;
	
	struct {
		int cursorTrkPtIdx;
		uint64_t cursorTrkPtData;
		
		char *file;
		
		struct {
			double factor;
			double multiplier;		// zoom factor
			double initial;
		}scale;

		struct {
			double x;
			double y;
			double startX;
			double startY;
			double deltaX;
			double deltaY;
		
			struct {	
				double x;
				double y;
				int ready;
			}modified;
		}offset;
					
		struct {
			int enabled;			// to render or not to render
			int flags;				// what to render
			int sheets;				// number of sheets enabled for render
			int cursorMode;
			int statsPosY;
			
			struct {
				int list[32];
				int total;
			}colours;
		}render;
		
		struct {
			double metersPerPixelH;
			double metersPerPixelV;
			
			double distanceLong;		// length of route hori, in meters
			double distanceLat;			// length of route vert, in meters
		}ruler;

		struct {
			struct {
				int x;
				int y;
			}position;
			
			struct {
				double longitude;
				double latitude;
			}location;
		}cursor;
		
		struct {
			double longitude;
			double latitude;
		}location;
	}route;
	
	int drawCursor;
	int drawCursorTimeout;
	int slideState;
	uint64_t t0;
			
	struct {
		TFRAME *original;
		TFRAME *surface;
		uint64_t colour;
		TLABEL *label;			// used only to capture input
		int id;
	}background;
	
	struct {
		TCCBUTTON *import;
		TCCBUTTON *info;
		TCCBUTTON *route;
		TCCBUTTON *zoomIn;
		TCCBUTTON *zoomOut;
		TCCBUTTON *zoomReset;
		
		TCCBUTTON *cursorModeA;		// trkPt inspect
		TCCBUTTON *cursorModeB;		// route inspect
		TCCBUTTON *cursorModeC;		// map location

		TCCBUTTON *onlineMap;		// find GPS location on map (defualts to OSM)
		TCCBUTTON *onlineAddress;	// find/guess address from GPS using Google api
	}ui;
	
	
	struct {
		TFRAMESTRINGCACHE *bin;
	}strc;

	struct {
		TPANE *pane;
		int font;
	}info;
	
	int graphLapHighlight;
	
	int crosshair;
	int mapMarker;
}TTCX;

int page_tcxCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

int garminConfigIsTcxEnabled (void *pageStruct);

#endif

