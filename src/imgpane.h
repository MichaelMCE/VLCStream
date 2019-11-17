// http://mylcd.sourceforge.net/
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


#ifndef _IMGPANE_H_
#define _IMGPANE_H_




typedef struct {
	TPAGE2COM *com;
	
	TPANE *pane;
	TASYNCIMGLOAD *imgLoader;
	TARTMANAGER *am;
	wchar_t *path;	
	int *itemIdList;
	int itemIdListTotal;
	int nameToggleState;

	TCCBUTTONS *btns;
}TIMGPANE;


int imgPane_Callback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);




void imgPaneEmpty (TIMGPANE *imgpane);
void imgPaneSetImageFocus (TIMGPANE *imgpane, const int idx);
int imgPaneAddPath (TIMGPANE *imgpane, TPANE *pane, const wchar_t *path, const char *file, const int addTitle, const double scale);

#endif
