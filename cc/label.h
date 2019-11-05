
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


#ifndef _LABEL_H_
#define _LABEL_H_




#define LABEL_RENDER_INV			0x000
#define LABEL_RENDER_BASE			0x001
#define LABEL_RENDER_BLUR			0x002
#define LABEL_RENDER_BORDER_PRE		0x004
#define LABEL_RENDER_BORDER_POST	0x008
#define LABEL_RENDER_TEXT			0x010
#define LABEL_RENDER_IMAGE			0x020
#define LABEL_RENDER_CCOBJ			0x040
#define LABEL_RENDER_HOVER			0x080
#define LABEL_RENDER_HOVER_OBJ		0x100	/* hover when over label item only */

#define LABEL_RENDER_ALL			(0xFF &~LABEL_RENDER_HOVER)		/* all except hover*/
#define LABEL_RENDER_ALL_NOBASE		(LABEL_RENDER_ALL &~LABEL_RENDER_BLUR &~LABEL_RENDER_BORDER_PRE &~LABEL_RENDER_BASE)


#define LABEL_BORDER_SET_PRE		0
#define LABEL_BORDER_SET_POST		1

#define LABEL_OBJTYPE_TEXT			1
#define LABEL_OBJTYPE_IMAGE			2
#define LABEL_OBJTYPE_CCOBJECT		3
#define LABEL_OBJTYPE_IMGCACHEID	4	/* imageManager item object reference  */
#define LABEL_OBJTYPE_ARTCACHEID	5	/* artManager item object reference  */



typedef struct{
	T2POINT pos;				// relative (to label) position
	T2POINT offset;				// relative position including justified offset, if any
	
	char string[MAX_PATH_UTF8+1];
	TFRAME *frame;			// the rendered string
	TFRAME *blur;
	int charRenderOffset;	// render string from this index (string[charRenderOffset])
		
	int flags;					// PF_ print flags
	int font;
	unsigned int wrapText;
	unsigned int colInk;
	unsigned int colBack;
	unsigned int colOutline;
	unsigned int isScaled;
	double scale;
	int maxW;
	int maxH;
	int blurRadius;
	int filterType;		// filter rendering mode (0:outline, 1:heavy blur, 2:3 pass direct render with shadow, outlene and foreground)
	THWD *hw;
}TLABELTEXT;

typedef struct{
	T2POINT pos;		// location local to label
	T4POINT actual;

	TFRAME *img;
	TFRAME *working;
	
	int hasConstraints;	// render image but constrained within below rect
	T4POINT constraint;
	
	int canHover;
	unsigned int hoverColour;
	double hoverAlpha;
	
	int scaleImage;		// 1: scale image by scaleBy;
	double scaleBy;		//
	int opacity;		// 0 - 100
}TLABELIMAGE;

typedef struct{
	T2POINT pos;		// location local to label
	T4POINT actual;

	int imgId;			// reference id as returned by artManagerAdd()
	TARTMANAGER *im;	// handle to image manager
	TFRAME *drawable;	
	int drawableDrawn;
	
	int hasConstraints;	// render image but constrained within below rect
	T4POINT constraint;
	
	int canHover;
	unsigned int hoverColour;
	double hoverAlpha;
	
	int scaleImage;		// 1: scale image by scaleBy;
	double scaleBy;		//
	double scaleAcquired;
	int opacity;		// 0 - 100
	
	int groupId;
}TLABELARTCA;

#define TLABELIMGCA TLABELARTCA


typedef struct{
	T2POINT pos;		// location local to label
	void *cc;			// CC object (CC_BUTTON, CC_LISTBOX, etc..)
}TLABELCCOBJ;

typedef struct{
	int objType;		// type LABEL_OBJTYPE_
	int id;
	int enabled;
	int64_t dataInt64;		// user storage
	
	union {
		TLABELTEXT  *text;
		TLABELIMAGE *image;
		TLABELCCOBJ *ccObj;
		TLABELARTCA *artc;
		void *obj;
	}u;
}TLABELOBJ;

typedef struct{
	TLISTITEM *head;	// list of TLABELOBJ items
	TLISTITEM *tail;
}TLABELOBJLIST;

typedef struct{
	int thickness;				// number of lines which make up the border
	unsigned int colour[2][16];	// 32bit colour before and after blur is applied. number of entries should match the thickness above
								// colour[0][..] = pre blur, colour[1][..] = post blur
}TLABELBORDER;

typedef struct{
	unsigned int colour;
}TLABELBASE;

struct TLABEL{
	COMMONCTRLOBJECT;

	TLABELOBJLIST *objs;
	TLABELBORDER border;
	TLABELBASE base;
	unsigned int renderflags;
	int idSrc;
};




int labelNew (TCCOBJECT *object, void *unused, const int pageOwner, const int type, const TCommonCrtlCbMsg_t label_cb, int *id, const int width, const int height);

int labelTextCreate (TLABEL *label, const char *str, const int pf_flags, const int font, const int xLocal, const int yLocal);
int labelTextCreateEx (TLABEL *label, const char *str, const int pf_renderFlags, const int font, const int xLocal, const int yLocal, const int renderType, const int64_t udata);
int labelImageCreate (TLABEL *label, TFRAME *img, const int resize, const int xLocal, const int yLocal);
int labelCCCreate (TLABEL *label, void *ccObject, const int xLocal, const int yLocal);

// to avoid contention two seperate image managers are in use
// using the vp->im 'image' manager, intended for skin(s) only
int labelImgcCreate (TLABEL *label, const int imgId, const int resize, const int xLocal, const int yLocal);
// using the vp->am 'art' manager, intended for artwork
int labelArtcCreate (TLABEL *label, const int imgId, const double scale, const int resize, const int xLocal, const int yLocal);
int labelArtcCreateEx (TLABEL *label, const int imgId, const double scale, const int resize, const int xLocal, const int yLocal, const int hoverState, const unsigned int hoverColour, const double hoverAlpha, const int64_t udata);



void labelItemsDisable (TLABEL *label);
void labelItemsEnable (TLABEL *label);
int labelItemEnable (TLABEL *label, const int id);
int labelItemDisable (TLABEL *label, const int id);
int labelItemGetEnabledStatus (TLABEL *label, const int id);
int labelItemDelete (TLABEL *label, const int id);
void labelItemsDelete (TLABEL *label);	// remove all objects
int labelItemPositionSet (TLABEL *label, const int id, const int xLocal, const int yLocal);
int labelItemPositionGet (TLABEL *label, const int id, int *xLocal, int *yLocal);

int64_t labelItemDataGet (TLABEL *label, const int id);			// user space
int64_t labelItemDataSet (TLABEL *label, const int id, const int64_t data);

int ccLabelFlushAll (TCC *cc);

int labelCCSet (TLABEL *label, const int id, void *object);
void *labelCCGet (TLABEL *label, const int id);

int labelImageSet (TLABEL *label, const int id, TFRAME *img, const int resize);
TFRAME *labelImageGet (TLABEL *label, const int id);		// clone. use lDeleteFrame() to free
TFRAME *labelImageGetSrc (TLABEL *label, const int id);		// actual. do not free
int labelImageScaleSet (TLABEL *label, const int id, const double scale);
double labelImageScaleGet (TLABEL *label, const int id);
int labelImageGetHeight (TLABEL *label, const int id);
int labelImageGetWidth (TLABEL *label, const int id);
void labelImageHoverSet (TLABEL *label, const int id, const unsigned int colour, const double alpha);



int labelArtcSet (TLABEL *label, const int id, const int imgId, const int resize);
int labelArtcGet (TLABEL *label, const int id);		// get the artManager reference id
int labelArtcGetSrc (TLABEL *label, const int id);	// is identical to labelArtcGet
int labelArtcScaleSet (TLABEL *label, const int id, const double scale);
double labelArtcScaleGet (TLABEL *label, const int id);
int labelArtcOpacitySet (TLABEL *label, const int id, const int opacity);
int labelArtcOpacityGet (TLABEL *label, const int id);
int labelArtcGetHeight (TLABEL *label, const int id);
int labelArtcGetWidth (TLABEL *label, const int id);
int labelArtcGetMetrics (TLABEL *label, const int id, int *width, int *height);
void labelArtcHoverSet (TLABEL *label, const int id, const unsigned int colour, const double alpha);

void labelBorderProfileSet (TLABEL *label, const int set, const unsigned int *colour, const int total);
void labelBaseColourSet (TLABEL *label, const unsigned int colour);
unsigned int labelRenderFlagsGet (TLABEL *label);
void labelRenderFlagsSet (TLABEL *label, const unsigned int flags);



/*
	string configuration methods
*/
int labelStringSet (TLABEL *label, const int id, const char *str);
char *labelStringGet (TLABEL *label, const int id);		// return a copy of the string. free with my_free()

int labelStringPositionGet (TLABEL *label, const int id, int *xLocal, int *yLocal);

int labelStringRenderFlagsGet (TLABEL *label, const int id);
void labelStringRenderFlagsSet (TLABEL *label, const int id, const int flags);

double labelRenderScaleGet (TLABEL *label, const int id);
void labelRenderScaleSet (TLABEL *label, const int id, const double scale);

int labelRenderColourGet (TLABEL *label, const int id, unsigned int *fore, unsigned int *back, unsigned int *outline);
void labelRenderColourSet (TLABEL *label, const int id, const unsigned int fore, const unsigned int back, const unsigned int outline);

int labelRenderBlurRadiusGet (TLABEL *label, const int id);
void labelRenderBlurRadiusSet (TLABEL *label, const int id, const int radius);

int labelRenderFilterGet (TLABEL *label, const int id);
void labelRenderFilterSet (TLABEL *label, const int id, const int type);	// set between 0:shadowed text, 1:smooth+shadowed, 2:greater smoothing with greater shadow coverage with word wrap

int labelRenderFontGet (TLABEL *label, const int id);
void labelRenderFontSet (TLABEL *label, const int id, const int font);

int labelStringSetMaxWidth (TLABEL *label, const int id, const int width);
int labelStringGetMetrics (TLABEL *label, const int id, int *x, int *y, int *width, int *height);


// testing..
int labelStringSetLeftOffsetIndex (TLABEL *label, const int id, const int offset);
int labelStringGetLeftOffsetIndex (TLABEL *label, const int id);


void *labelItemGet (TLABEL *label, const int id);
int labelItemGetType (TLABEL *label, const int id);



#define labelImgcSet			labelArtcSet       
#define labelImgcGet            labelArtcGet       
#define labelImgcGetSrc         labelArtcGetSrc    
#define labelImgcScaleSet       labelArtcScaleSet
#define labelImgcScaleGet       labelArtcScaleGet
#define labelImgcOpacitySet     labelArtcOpacitySet  
#define labelImgcOpacityGet     labelArtcOpacityGet
#define labelImgcGetHeight      labelArtcGetHeight 
#define labelImgcGetWidth       labelArtcGetWidth  
#define labelImgcGetMetrics     labelArtcGetMetrics
#define labelImgcHoverSet       labelArtcHoverSet  



typedef struct{
	int bwidth;			// width/height of first item. assume rest are the same
	int bheight;

	int width;
	int height;

	int *itemIds;		// label item id's
	TLABEL *base;
}TBTNPANEL;

 
typedef struct{
	TLABEL *label;
	int ccId;
	int strId;			// reference Id supplied by label control
}TLABELSTR;

TLABELSTR *labelStrCreate (void *pageStruct, const char *text, const int font, const int x, const int y, int width, void *data, const int justify);
void labelStrDelete (TLABELSTR *lblstr);
void labelStrUpdate (TLABELSTR *lblstr, const char *text);
int labelStrRender (TLABELSTR *lblstr, TFRAME *frame);
void labelStrEnable (TLABELSTR *lblstr);
void labelStrDisable (TLABELSTR *lblstr);

#endif

