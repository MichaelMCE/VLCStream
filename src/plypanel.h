
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



#ifndef _PLYPANEL_H_
#define _PLYPANEL_H_


enum _icons{
	PANEL_ICON_AUDIO,
	PANEL_ICON_FOLDER,
	PANEL_ICON_VIDEO,
	PANEL_ICON_IMAGE,
	PANEL_ICON_PLAYLIST,

	PANEL_ICON_TOTAL
};

typedef struct{
	TPAGE2COM *com;
	
	TPANEL *panel;
	int currentPlcUID;
	T2POINT itemOffset;
	int imgArtSize;
	
	int iconId[PANEL_ICON_TOTAL];		// image manager ids
}TPLYPANEL;




int page_plyPanCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);


int plyPanelBuild (TVLCPLAYER *vp, TPANEL *panel, PLAYLISTCACHE *plc);
void plyPanRebuild (TVLCPLAYER *vp);
void plyPanRebuildCleanMeta (TVLCPLAYER *vp);
void plyPanSetCurrentUID (TVLCPLAYER *vp, const int uid);

#endif

