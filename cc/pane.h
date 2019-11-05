
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


#ifndef _PANE_H_
#define _PANE_H_


#define PANE_LAYOUT_HORI			1		// left justified, multiple columns with horizontal scrolling
#define PANE_LAYOUT_VERT			2		// left justified, single column, muliple rows, vertical scrolling
#define PANE_LAYOUT_VERTCENTER		3		// middle justified, single column, muliple rows, vertical scrolling 
#define PANE_LAYOUT_HORICENTER		4		// middle justified, single row, multiple columns, horizontal scrolling

#define PANE_INVALIDATE				1

#define PANE_OBJ_STRING				1
#define PANE_OBJ_IMAGE				2

#define PANE_DIRECTION_STOP			0				// no movement (left or right / up or down)
#define PANE_DIRECTION_LEFT			1				// right to left
#define PANE_DIRECTION_RIGHT		2				// left to right 
#define PANE_DIRECTION_UP			3
#define PANE_DIRECTION_DOWN			4

#define PANE_SLIDEMODE_PANE			1
#define PANE_SLIDEMODE_ITEM			2



// image allignment/placement (PANE_OBJ_IMAGE)
enum _imgPos {
	PANE_IMAGE_CENTRE = 1,
	PANE_IMAGE_NORTH,
	PANE_IMAGE_SOUTH,
	PANE_IMAGE_WEST,
	PANE_IMAGE_EAST,
	PANE_IMAGE_NW,
	PANE_IMAGE_NE,
	PANE_IMAGE_SW,
	PANE_IMAGE_SE,
	PANE_IMAGE_POS			// for this 'pos' is the local position, otherwise it's the offset around the above Geo. directions
};



#define PANE_SLIDE_DISABLED		(-1)
#define PANE_SLIDE_NONE			(0)
#define PANE_SLIDE_PRESS		(1)
#define PANE_SLIDE_SLIDE		(2)
#define PANE_SLIDE_RELEASE		(3)


typedef struct{
	int imgIds[128];
	int total;
	TARTMANAGER *am;
	void *vp;
}img_readhead;


typedef struct {
	int id;
	int type;

	struct {
		int itemId;			// label item id
		//char *string;
		int hasIcon;		// render ::image next to this
		int overlapIcon;	// text overlaps icon (same column space)
		TMETRICS metrics;	// position within label, width/height of string
	}text;
	
	struct {
		int itemId;			// label item id
		int imgcId;			// image manager id
		int dir;			// image position type
		TMETRICS metrics;	// position within label
		TPOINT offset;		// image placement offset from when using direction PANE_IMAGE_ (except PANE_IMAGE_POS)
	}image;
	
	struct {
		void *object;
	}cc;
}TPANEOBJ;

typedef struct {
	int state;
	int slideEnabled;
	int slideMode;			// pane slide or item grab/slide
	
	struct {
		int x;
		int y;
		double time;		// ms, time event received (getTime(vp))
		int id;				// pos->id
	}start;
	
	struct {
		int x;
		int y;
	}last;					// previous location
		
	struct {
		int x;
		int y;
		double time;
	}delta;					// distance travelled from previous event to this (relative)

	struct {
		int x;
		int y;
	}travelled;				// total cumulative distance travelled
	
	struct {
		int x;
		int y;
		double dt;			// deltaTime, time between inital press and final release
		int id;				// pos->id
		
		struct {
			int x;
			int y;
			unsigned int area;	// area from start to release (up)
		}travelled;				// total cumulative distance travelled
	}released;

	struct {
		double x;
		double y;
	}acceleration;			// movement multiplier
	
	struct {
	  TLABEL *label;
	  int itemId;
	  int artId;
	  int dx;
	  int dy;
	  int id;
	  int state;
	  int heldId;
	  int heldType;
	}drag;
}TPANEINPUT;

struct TPANE {
	COMMONCTRLOBJECT;
	
	TLABEL *base;
	TLISTITEM *items;
	TLISTITEM *itemsLast;

	TLISTITEM *firstEnabledItem;
	TLISTITEM *lastEnabledItem;
	int firstEnabledImgId;
	int firstEnabledImgIdx;
	int firstEnabledStrId;
	int firstEnabledStrIdx;

	int tItemWidth;			// temp
	int tItemHeight;
	int horiColumnSpace;	// size of gap between horizontal columns
	int vertLineHeight;		// configure line height (line pitch) of PANE_LAYOUT_VERT
	
	int aveLineHeight;
	int totalHeight;

		
	TPOINT pos;
	TPOINT offset;
	TPOINT previous;
	int direction;			// PANE_DIRECTION_
	TPANEINPUT input;

	int layoutMode;			// 1:left to right scrollable. 2:top down scrollable
	int isInvalidated;		// set when item list is modified, indicates item list objects require repositioning
	int idSrc;
	
	
	struct {
		struct {
			int text;		// number of text objs added
			int img;		// number of img objs added
		}total;
		struct {
			int singleLine;		// 1: single line text only, no wrap around. 0: allow text to wrap around.
			int wordWrap;		// enable word wrap when :textSingleLine is disabled
			int renderFlags;
			TPOINT globalOffset;
		}text;
		struct {
			int enabled;
			int number;			// read ahead this number of images
		}readAhead;
	}flags;
};



int paneNew (TCCOBJECT *object, void *unused, const int pageOwner, const int type, const TCommonCrtlCbMsg_t panel_cb, int *id, const int width, const int height);

void paneRemoveAll (TPANE *pane);	// remove all items from pane
void paneRemoveItem (TPANE *pane, const int itemId);

//	 imgcId: string placed right side of image
//	-imgcId: string placed on top of image (overlaps)
int paneTextAdd (TPANE *pane, const int imgcId, const double imgScale, const char *string, const int font, const int64_t udataInt64);
int paneImageAdd (TPANE *pane, const int imgcId, const double scale, const int dir, const int x, const int y, const int64_t udataInt64);

int paneTextAddEx (TPANE *pane, const int imgcId, const double imgScale, const char *string, const int font, const unsigned int colour, const int unused, const int64_t udataInt64);

int paneTextReplace (TPANE *pane, const int textId, const char *string);
int paneImageReplace (TPANE *pane, const int imageId, const int newImageId);

int paneItemGetDetail (TPANE *pane, const int itemId, char **str, int *imgId);
int paneIndexToItemId (TPANE *pane, const int idx);		// base 0 idx
int paneItemGetData (TPANE *pane, const int itemId, int64_t *data);

void paneSetLayout (TPANE *pane, const int layoutMode);
void paneSetAcceleration (TPANE *pane, const double x, const double y);
void paneSwipeDisable (TPANE *pane);
void paneSwipeEnable (TPANE *pane);

int paneScroll (TPANE *pane, const int nPixels);
int paneScrollSet (TPANE *pane, const int x, const int y, const int invalidate);
int paneScrollGet (TPANE *pane, int *xOffset, int *yOffset);
int paneScrollReset (TPANE *pane);

void paneDragEnable (TPANE *pane);
void paneDragDisable (TPANE *pane);

int paneFocusSet (TPANE *pane, const int itemId);

void paneTextMulityLineEnable (TPANE *pane);
void paneTextMulityLineDisable (TPANE *pane);
void paneTextWordwrapEnable (TPANE *pane);
void paneTextWordwrapDisable (TPANE *pane);


void panePreloadItems (TPANE *pane, const int count);

#endif

