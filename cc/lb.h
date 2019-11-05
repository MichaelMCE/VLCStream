
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


#ifndef _LB_H_
#define _LB_H_


#define LISTBOX_ITEM_STRING				1
#define LISTBOX_ITEM_IMAGE				2

#define LISTBOX_SBPOSITION_LEFT			1
#define LISTBOX_SBPOSITION_RIGHT		2


struct TLB {
	COMMONCTRLOBJECT;
	TCCBUTTONS *ccbtns;
	TFRAME *surface;		// tmp render surface

	int font;				// default font
	int verticalPadding;	// number of pixels between button placement(s)
	
	int maxWorkingWidth;
	int maxWorkingHeight;

	struct{
		TSCROLLBAR *vert;
		int position;		// 1:left, 2:right
		int offset;			// inner positional offset from edge
	}scrollbar;

	struct{
		int maxStringWidth;
		int justify;
		int drawScrollbar:1;
		int drawBaseGfx:1;
		int drawBaseBlur:1;
	}flags;
};



int lbNew (TCCOBJECT *object, void *unused, const int pageOwner, const int type, const TCommonCrtlCbMsg_t lb_cb, int *id, const int maxWidth, const int maxHeight);


int lbAddItem (TLB *lb, const int faceType, void *pri, const int var1, const int var2);		// insert item at end of list
int lbAddItemEx (TLB *lb, const int faceType, void *pri, void *sec, const int var1, const int var2, const int64_t data);		// data returned in callback as 'data2'
void lbRemoveItems (TLB *lb);		// remove all items
int lbGetTotalItems (TLB *lb);

int64_t lbGetFocus (TLB *lb);
int lbSetFocus (TLB *lb, int64_t const value);
int lbItemSetString (TLB *lb, const int item, const char *pri, const char *sec);
void lbScrollUp (TLB *lb);
void lbScrollDown (TLB *lb);

int lbScrollbarSetWidth (TLB *lb, const int swidth);



#endif
