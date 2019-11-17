
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



unsigned int *swatchGetPage (TVLCPLAYER *vp, const int page)
{
	return vp->gui.skin.swatch.col[page-PAGE_BASEID];
}

unsigned int swatchGetColour (TVLCPLAYER *vp, const int page, const int id)
{
	return swatchGetPage(vp, page)[id];
}

/*static inline*/ void swatchSetColour (TVLCPLAYER *vp, const int page, const int id, const unsigned int colour)
{
	swatchGetPage(vp, page)[id] = colour;
}

void swatchLoad (TVLCPLAYER *vp, TSWATCH *swatch, wchar_t *path)
{

	vp->gui.image[IMGC_SWATCH] = imageManagerImageAdd(vp->im, path);
	TFRAME *src = imageManagerImageClone(vp->im, vp->gui.image[IMGC_SWATCH]);


	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_IPLAYING,		lGetPixel(src, 0, 0));
	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_ISELECTED,		lGetPixel(src, 1, 0));
	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_RECTPLAYING,	lGetPixel(src, 2, 0));
	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_RECTSELECTED,	lGetPixel(src, 3, 0));

	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_PLACEHOLDER,	lGetPixel(src, 0, 2));
	
	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_PLAYPOSC1,		lGetPixel(src, 0, 3));
	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_PLAYPOSC2,		lGetPixel(src, 1, 3));
	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_PLAYPOSC3,		lGetPixel(src, 2, 3));
	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_PLAYPOSC4,		lGetPixel(src, 3, 3));
	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_PLAYPOSC5,		lGetPixel(src, 4, 3));
	
	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_TRKTEXT,		lGetPixel(src, 0, 4));
	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_TRKBACK,		lGetPixel(src, 1, 4));
	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_TRKOUTLINE,		lGetPixel(src, 2, 4));
	
	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_PLYLNAME,		lGetPixel(src, 0, 5));
	swatchSetColour(vp, PAGE_NONE,		SWH_PLY_PLYLNAMEBK,		lGetPixel(src, 1, 5));

	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_CHAPMARK,		lGetPixel(src, 0, 6));
	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_CHAPMARKFILL,	lGetPixel(src, 1, 6));
	
	swatchSetColour(vp, PAGE_SUB,		SWH_SUB_HIGHLIGHTED,	lGetPixel(src, 0, 7));
	swatchSetColour(vp, PAGE_SUB,		SWH_SUB_NONHIGHLIGHTED,	lGetPixel(src, 1, 7));
	
	swatchSetColour(vp, PAGE_CHAPTERS,	SWH_CHP_HIGHLIGHTED,	lGetPixel(src, 0, 8));
	swatchSetColour(vp, PAGE_CHAPTERS,	SWH_CHP_NONHIGHLIGHTED,	lGetPixel(src, 1, 8));
	swatchSetColour(vp, PAGE_CHAPTERS,	SWH_CHP_BACKGROUND,		lGetPixel(src, 2, 8));
	swatchSetColour(vp, PAGE_CHAPTERS,	SWH_CHP_PAGEBORDER,		lGetPixel(src, 3, 8));
	swatchSetColour(vp, PAGE_CHAPTERS,	SWH_CHP_TITLE,			lGetPixel(src, 4, 8));

	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_MARQTEXT,		lGetPixel(src, 0, 9));
	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_MARQTEXTBK,		lGetPixel(src, 1, 9));

	swatchSetColour(vp, PAGE_IMGOVR,	SWH_IOVR_IMGBORDER,		lGetPixel(src, 0, 10));
	
	swatchSetColour(vp, PAGE_ES,		SWH_ES_BLURBORDER,		lGetPixel(src, 0, 11));
	swatchSetColour(vp, PAGE_ES,		SWH_ES_TEXT,			lGetPixel(src, 1, 11));
	swatchSetColour(vp, PAGE_ES,		SWH_ES_TEXTBK,			lGetPixel(src, 2, 11));

	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_EBOXTEXT,		lGetPixel(src, 0, 12));
	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_EBOXTEXTBK,		lGetPixel(src, 1, 12));
	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_EBOXIPLAYING,	lGetPixel(src, 2, 12));
	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_EBOXISELECTED,	lGetPixel(src, 3, 12));
	
	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_FPSTEXT,		lGetPixel(src, 0, 13));
	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_FPSOUTLINE,		lGetPixel(src, 1, 13));
	
	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_TIMESTAMPTEXT,	lGetPixel(src, 0, 14));
	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_TIMESTAMPTEXTBK,lGetPixel(src, 1, 14));
	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_PANELEDGE,		lGetPixel(src, 2, 14));

	/*swatchSetColour(vp, PAGE_BROWSER,	SWH_FB_LINK,			lGetPixel(src, 0, 15));
	swatchSetColour(vp, PAGE_BROWSER,	SWH_FB_DIRS,			lGetPixel(src, 1, 15));
	swatchSetColour(vp, PAGE_BROWSER,	SWH_FB_FILES,			lGetPixel(src, 2, 15));
	swatchSetColour(vp, PAGE_BROWSER,	SWH_FB_MODULE,			lGetPixel(src, 3, 15));
	swatchSetColour(vp, PAGE_BROWSER,	SWH_FB_IHIGHLIGHT,		lGetPixel(src, 4, 15));
	swatchSetColour(vp, PAGE_BROWSER,	SWH_FB_PATH,			lGetPixel(src, 5, 15));
	swatchSetColour(vp, PAGE_BROWSER,	SWH_FB_SELECTEDFILE,	lGetPixel(src, 6, 15));
	swatchSetColour(vp, PAGE_BROWSER,	SWH_FB_FILESIZE,		lGetPixel(src, 7, 15));
	swatchSetColour(vp, PAGE_BROWSER,	SWH_FB_EMPTYLOCTEXT,	lGetPixel(src, 8, 15));*/
	
	swatchSetColour(vp, PAGE_META,		SWH_META_PAGEBORDER,	lGetPixel(src, 0, 16));
	swatchSetColour(vp, PAGE_META,		SWH_META_METANAME,		lGetPixel(src, 1, 16));
	swatchSetColour(vp, PAGE_META,		SWH_META_METAVALUE,		lGetPixel(src, 2, 16));
	
	swatchSetColour(vp, PAGE_EPG,		SWH_EPG_TEXT,			lGetPixel(src, 0, 17));
	swatchSetColour(vp, PAGE_EPG,		SWH_EPG_HIGHLIGHTED,	lGetPixel(src, 1, 17));
	swatchSetColour(vp, PAGE_EPG,		SWH_EPG_NONHIGHLIGHTED,	lGetPixel(src, 2, 17));
	swatchSetColour(vp, PAGE_EPG,		SWH_EPG_BLURBORDER,		lGetPixel(src, 3, 17));
	
	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_TRKBUBBLE,		lGetPixel(src, 0, 18));
	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_TRKBUBBLEBK,	lGetPixel(src, 1, 18));
	swatchSetColour(vp, PAGE_OVERLAY,	SWH_OVR_TRKBUBBLEBOR,	lGetPixel(src, 2, 18));
	
	//imageManagerImageDelete(vp->im, vp->gui.image[IMGC_SWATCH]);
	imageManagerImageFlush(vp->im, vp->gui.image[IMGC_SWATCH]);
	lDeleteFrame(src);
}


