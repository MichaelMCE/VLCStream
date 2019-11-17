
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

#ifndef _EXPPANEL_H_
#define _EXPPANEL_H_



enum _panelIds {
	PANEL_SID_DRIVES,				//  logicial drive letters
	PANEL_SID_REMOTE,				//  network folders mounted as logical drive letter
	PANEL_SID_DIRS,					//  folders and links
	PANEL_SID_FILES,				//  files/modules
	PANEL_SID_TOTAL
};

#define PANEL_MODULE_TEXT_POSX			(1)
#define PANEL_MODULE_TEXT_POSY			(40)
#define PANEL_MODULE_TEXT_MAXWIDTH		(780)



typedef struct {
	int *idList;
	int idTotal;
	int state;
}TPANELSEPARATOR;


typedef struct{
	TPAGE2COM *com;
	
	T2POINT itemOffset;		// panel vertical offset
	TPANEL *panel;
	int panelIds[PANEL_SID_TOTAL];

	TLOGICALDRIVE *drives;
	int logicalDriveTotal;
	
	TSHELLFOLDEROBJS *shellFolders;
	int shellFoldersTotal;
	
	TLINKSHORTCUTS userLinks;

	wchar_t mycomputer[MAX_PATH+1];
	wchar_t selectedFile[MAX_PATH+1];	// used to store the path of the active/highlighted module 
}TEXPPANEL;


int page_expPanCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);



void expanTimerPanelRebuild (TVLCPLAYER *vp);
void expanTimerPanelRebuildSetPage (TVLCPLAYER *vp);


#endif
