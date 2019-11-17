
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


#include "../common.h"


int lbScrollbarSetWidth (TLB *lb, const int swidth)
{
	return ccSetMetrics(lb->scrollbar.vert, -1, -1, swidth, -1);
}

static inline int64_t lbScrollbarCalcRange (TLB *lb)
{
	int64_t rmax = (buttonsTotalGet(lb->ccbtns) * (lb->verticalPadding)) + 1;
	if (rmax < lb->verticalPadding) rmax = lb->verticalPadding;
	scrollbarSetRange(lb->scrollbar.vert, 0, rmax, lb->scrollbar.vert->firstItem, lb->metrics.height);
	return rmax;
}

int lbGetTotalItems (TLB *lb)
{
	int ret = 0;
	if (ccLock(lb)){
		ret = buttonsTotalGet(lb->ccbtns);
		ccUnlock(lb);
	}
	return ret;
}

void lbRemoveItems (TLB *lb)
{
	if (ccLock(lb)){
		buttonsDeleteAll(lb->ccbtns);
		lb->ccbtns = NULL;
		ccUnlock(lb);
	}
}

static inline int lbButtonSumFaceWidth (TLB *lb, TCCBUTTON *btn)
{
	int width = 0;
	int widthPri = 0;
	int widthSec = 0;
		
	int ret = buttonFaceTextMetricsGet(btn, BUTTON_PRI, NULL, NULL, &widthPri, NULL);
	if (!ret){
		width = btn->metrics.width;
		
	}else{
		if (buttonFaceTextMetricsGet(btn, BUTTON_SEC, NULL, NULL, &widthSec, NULL))
			width = MAX(widthPri, widthSec);
		else
			width = widthPri;
	}
	
	if (/*lb->flags.justify == PF_LEFTJUSTIFY ||*/ width > lb->metrics.width){
		buttonFaceTextFlagsSet(btn, BUTTON_PRI, PF_LEFTJUSTIFY);
		buttonFaceTextFlagsSet(btn, BUTTON_SEC, PF_LEFTJUSTIFY);
	}
	
	return width;
}


static inline int lbBuildMetrics (TLB *lb, int64_t value)
{
	//printf("lbBuildMetrics %I64d\n", value);
	
	const int total = buttonsTotalGet(lb->ccbtns);
	value = scrollbarSetFirstItem(lb->scrollbar.vert, value);
	if (value < 0 || (value > total * lb->verticalPadding)) value = 0;

	int btnTop = value / lb->verticalPadding;
	if (btnTop < 0) btnTop = 0;
	
	const int btnBtm = btnTop + ((lb->metrics.height / lb->verticalPadding) + 1);
	const int yAdjust = value % lb->verticalPadding;
	int y = (lb->metrics.y + 1) - yAdjust;

	
	//printf("@@ lbBuildMetrics %i %i %i %i %I64d, %i\n", btnTop, btnBtm, yAdjust, y, value, total);
	

	for (int i = 0; i < total; i++){
		TCCBUTTON *btn = buttonsButtonGet(lb->ccbtns, i);
		if (!btn) break;
		
		if ((i >= btnTop && i <= btnBtm) && y < lb->metrics.y + lb->metrics.height){
			//int x = lb->metrics.x + (abs(btn->metrics.width - lb->metrics.width)/2);
			int width = lbButtonSumFaceWidth(lb, btn);
			
			int x;
			if (lb->flags.justify == PF_MIDDLEJUSTIFY && width < lb->metrics.width)
				x = lb->metrics.x + (abs(width - lb->metrics.width)/2);
			else
				x = lb->metrics.x + 1;

			//printf("lb width %i %i %i\n", width, x, btn->metrics.width);

			ccSetPosition(btn, x, y);
			y += lb->verticalPadding;
			
			ccEnable(btn);
			//printf("enable %i\n", btn->id);
		}else{
			ccDisable(btn);
			//printf("disable %i\n", btn->id);
		}

		//buttonFaceConstraintsSet(btn, lb->metrics.x, lb->metrics.y, lb->metrics.x+lb->metrics.width-1, lb->metrics.y+lb->metrics.height-1);
		//buttonFaceConstraintsEnable(btn);
	}

	return y;
}

static inline void lbSetItemText (TLB *lb, const int item, const char *pri, const char *sec)
{
	TCCBUTTON *btn = buttonsButtonGet(lb->ccbtns, item);
	if (btn){
		if (pri)
			buttonFaceTextUpdate(btn, BUTTON_PRI, pri);
		if (sec)
			buttonFaceTextUpdate(btn, BUTTON_SEC, sec);
	}
}

int64_t lbGetFocus (TLB *lb)
{
	int64_t ret = 0;
	if (ccLock(lb)){
		ret = scrollbarGetFirstItem(lb->scrollbar.vert);
		//printf("lbGetFocus %i\n", (int)ret);
		ccUnlock(lb);
	}
	return ret;
}

int lbSetFocus (TLB *lb, const int64_t value)
{
	int ret = 0;
	if (ccLock(lb)){
		//printf("lbSetFocus %i\n", (int)value);
		ret = lbBuildMetrics(lb, value/* * lb->verticalPadding*/);
		ccUnlock(lb);
	}
	return ret;
}

void lbScrollUp (TLB *lb)
{
	if (ccLock(lb)){
		int64_t value = lbGetFocus(lb) - lb->verticalPadding;
		lbBuildMetrics(lb, value);
		ccUnlock(lb);
	}
}

void lbScrollDown (TLB *lb)
{
	if (ccLock(lb)){
		int64_t value = lbGetFocus(lb) + lb->verticalPadding;
		lbBuildMetrics(lb, value);
		ccUnlock(lb);
	}
}

static inline int64_t btn_item_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;
	
	//printf("btn_item_cb: %p id:%i, objType:%i, msg:%i, data1:%I64d, data2:%I64d\n", btn_item_cb,obj->id, obj->type, msg, data1, data2);
	
	TCCBUTTON *btn = (TCCBUTTON*)obj;
	
	if (msg == BUTTON_MSG_SELECTED_PRESS){
		TLB *lb = ccGetUserData(btn);
		int idx = buttonsButtonToIdx(lb->ccbtns, btn);
		//printf("button %i, %i\n", btn->id, idx);
		ccSendMessage(lb, LISTBOX_MSG_ITEMSELECTED, idx, buttonDataGet(btn), NULL);
	}
	
	return 1;
}

static inline TCCBUTTON *lbCreateItem (TLB *lb)
{
	int total = 0;
	
	if (!lb->ccbtns){
		total = 1;
		lb->ccbtns = buttonsCreate(lb->cc, lb->pageOwner, total, btn_item_cb);
	}else{
		total = buttonsTotalGet(lb->ccbtns);
		buttonsTotalSet(lb->ccbtns, ++total, btn_item_cb);
	}
		
	TCCBUTTON *btn = buttonsButtonGet(lb->ccbtns, total-1);
	btn->isChild = 1;
	ccSetUserData(btn, lb);
	return btn;
}

static inline TCCBUTTON *lbAddItemImage (TLB *lb, TFRAME *pri, TFRAME *sec, const int x, const int y)
{
	return NULL;
}

static inline TCCBUTTON *lbItemBuildString (TLB *lb, TCCBUTTON *btn, char *pri, char *sec, int font, int colour)
{
	if (!font) font = lb->font;
	//if (!colour) colour = (255<<24) | COL_WHITE;


	//if string is too short it may be difficult to press/activate, so incease it's width
	/*int priLen = strlen(pri);
	if (priLen < 6){
		char buffer[priLen+32];
		snprintf(buffer, priLen+8, "%s      ", pri);
		buttonFaceTextSet(btn, BUTTON_PRI, buffer, PF_MIDDLEJUSTIFY, font, 0, 2);
	}else{*/
		buttonFaceTextSet(btn, BUTTON_PRI, pri, PF_MIDDLEJUSTIFY, font, 0, 2);
	//}
		
	if (lb->flags.justify == PF_LEFTJUSTIFY)
		buttonFaceTextFlagsSet(btn, BUTTON_PRI, PF_LEFTJUSTIFY);
	
	buttonFaceTextColourSet(btn, BUTTON_PRI, colour, 255<<24 | COL_BLACK, (177<<24) | COL_BLUE_SEA_TINT);
	if (sec){
		/*int strIdb = */buttonFaceTextSet(btn, BUTTON_SEC, sec, PF_MIDDLEJUSTIFY, font, 0, 2);	

		if (lb->flags.justify == PF_LEFTJUSTIFY)
			buttonFaceTextFlagsSet(btn, BUTTON_SEC, PF_LEFTJUSTIFY);
		buttonFaceTextColourSet(btn, BUTTON_SEC, colour, 255<<24 | COL_BLACK, (177<<24) | COL_BLUE_SEA_TINT);
		buttonFaceAutoSwapEnable(btn, BUTTON_FACE_SWAP_HOVERED);
	}else{
		buttonFaceAutoSwapDisable(btn);
	}

	int width = lbButtonSumFaceWidth(lb, btn);
	int height = lb->verticalPadding - 1;

	int x;
	if (lb->flags.justify == PF_MIDDLEJUSTIFY && width < lb->metrics.width)
		x = lb->metrics.x + (abs(width - lb->metrics.width)/2);
	else
		x = 1;
	
	//printf("additem %i %i %i '%s'\n", btn->id, x, width, pri);
	ccSetMetrics(btn, x, lb->metrics.y, width, height);
	buttonFaceHoverSet(btn, 1, COL_HOVER, 0.8);

	if (lb->flags.maxStringWidth/* || ccGetState(lb->scrollbar.vert)*/){
		if (pri)
			buttonFaceTextWidthSet(btn, BUTTON_PRI, lb->flags.maxStringWidth);
		if (sec)
			buttonFaceTextWidthSet(btn, BUTTON_SEC, lb->flags.maxStringWidth);
	}
	return btn;
}

int lbItemSetString (TLB *lb, const int i, const char *pri, const char *sec)
{
	int ret = 0;
	if (ccLock(lb)){
		TCCBUTTON *btn = buttonsButtonGet(lb->ccbtns, i);
		if (btn){
			lbSetItemText(lb, i, pri, sec);
			
			int width = lbButtonSumFaceWidth(lb, btn);
			int height = lb->verticalPadding - 1;
			
			int x;
			if (lb->flags.justify == PF_MIDDLEJUSTIFY && width < lb->metrics.width)
				x = lb->metrics.x + (abs(width - lb->metrics.width)/2);
			else
				x = 1;
	
			ccSetMetrics(btn, x, lb->metrics.y, width, height);
			buttonFaceHoverSet(btn, 1, COL_HOVER, 0.8);
	
			if (lb->flags.maxStringWidth/* || ccGetState(lb->scrollbar.vert)*/){
				if (pri)
					ret = buttonFaceTextWidthSet(btn, BUTTON_PRI, lb->flags.maxStringWidth);
				if (sec)
					ret |= buttonFaceTextWidthSet(btn, BUTTON_SEC, lb->flags.maxStringWidth);
			}
		}
		ccUnlock(lb);
	}
	return ret;
}

static inline TCCBUTTON *lbAddItemString (TLB *lb, char *pri, char *sec, int font, int colour)
{
	TCCBUTTON *btn = lbCreateItem(lb);
	if (btn)
		lbItemBuildString(lb, btn, pri, sec, font, colour);
	return btn;
}

int lbAddItem (TLB *lb, const int faceType, void *pri, const int var1, const int var2)
{
	int ret = -1;

	if (ccLock(lb)){
		TCCBUTTON *btn = NULL;
		
		if (faceType == LISTBOX_ITEM_STRING)
			btn = lbAddItemString(lb, pri, NULL, var1, var2);
		else if (faceType == LISTBOX_ITEM_IMAGE)
			btn = lbAddItemImage(lb, pri, NULL, var1, var2);


		if (btn){
			ccDisable(btn);
			lbBuildMetrics(lb, lbGetFocus(lb));
			
			if (lbScrollbarCalcRange(lb) <= lb->metrics.height)
				ccDisable(lb->scrollbar.vert);
			else
				ccEnable(lb->scrollbar.vert);

			ret = buttonsButtonToIdx(lb->ccbtns, btn);
			
		}
		ccUnlock(lb);
	}	
	return ret;
}

int lbAddItemEx (TLB *lb, const int faceType, void *pri, void *sec, const int var1, const int var2, const int64_t data)
{
	int ret = -1;

	if (ccLock(lb)){
		TCCBUTTON *btn = NULL;
		
		if (faceType == LISTBOX_ITEM_STRING)
			btn = lbAddItemString(lb, pri, sec, var1, var2);
		else if (faceType == LISTBOX_ITEM_IMAGE)
			btn = lbAddItemImage(lb, pri, sec, var1, var2);


		if (btn){
			ccDisable(btn);
			buttonDataSet(btn, data);
			lbBuildMetrics(lb, lbGetFocus(lb));

			if (lbScrollbarCalcRange(lb) <= lb->metrics.height)
				ccDisable(lb->scrollbar.vert);
			else
				ccEnable(lb->scrollbar.vert);

			ret = buttonsButtonToIdx(lb->ccbtns, btn);
			
		}
		ccUnlock(lb);
	}	
	return ret;
}



// ############################################################################################################
// ############################################################################################################
// ############################################################################################################


static inline int64_t lb_scrollbar_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//TCCOBJECT *obj = (TCCOBJECT*)object;
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;
	
	//printf("lb_scrollbar_cb. id:%i, objType:%i, msg:%i, data1:%I64d, data2:%I64d\n", obj->id, obj->type, msg, data1, data2);
	
	//if (obj->type == CC_SCROLLBAR_VERTICAL){
		TSCROLLBAR *scrollbar = (TSCROLLBAR*)object;
		TLB *lb = ccGetUserData(scrollbar);

		switch (msg){
	  	  case SCROLLBAR_MSG_VALCHANGED:{
	  	  	int64_t *item = (int64_t*)dataPtr;
	  	  	//printf("lb_scrollbar_cb %I64d\n", *item);

	  	  	//int value = listboxGetFocus(lb);
			//lbSetFocus(lb, *item);
			lbBuildMetrics(lb, *item);
			//ccSendMessage(lb, LISTBOX_MSG_VALCHANGED, value, value / lb->buttonSpacing, NULL);
			break;
	  	  }
	  	}
	//}
	return -1;
}

// ########## DISABLED ##########
void lbEnable (void *object)
{
	return;

	TLB *lb = (TLB*)object;
	lb->enabled = 1;

	if (lbScrollbarCalcRange(lb) <= lb->metrics.height)
		ccDisable(lb->scrollbar.vert);
	else
		ccEnable(lb->scrollbar.vert);
}

void lbDisable (void *object)
{
	TLB *lb = (TLB*)object;
	lb->enabled = 0;
	ccDisable(lb->scrollbar.vert);
}

int lbRender (void *object, TFRAME *frame)
{
	TLB *lb = (TLB*)object;

	copyAreaNoBlend(frame, lb->surface, lb->metrics.x, lb->metrics.y, lb->metrics.x, lb->metrics.y, lb->metrics.x+lb->metrics.width-1, lb->metrics.y+lb->metrics.height-1);
	//copyAreaNoBlend(frame, lb->surface, 0, 0, lb->metrics.x, lb->metrics.y, lb->metrics.x+lb->metrics.width-1, lb->metrics.y+lb->metrics.height-1);

	if (lb->flags.drawBaseBlur/* && pageGetSec(lb->cc->vp) == -1*/)
		lBlurArea(lb->surface, lb->metrics.x, lb->metrics.y, lb->metrics.x+lb->metrics.width-1, lb->metrics.y+lb->metrics.height-1, 2);

	if (lb->flags.drawBaseGfx)
		fillFrameAreaColour(lb->surface, lb->metrics.x, lb->metrics.y, lb->metrics.x+lb->metrics.width-1, lb->metrics.y+lb->metrics.height-1, (60<<24)|COL_BLUE_SEA_TINT);

	buttonsRenderAll(lb->ccbtns, lb->surface, BUTTONS_RENDER_HOVER);
	//buttonsRenderAll(lb->ccbtns, frame, BUTTONS_RENDER_HOVER);
	//buttonsRenderAllEx(lb->ccbtns, lb->surface, BUTTONS_RENDER_HOVER, lb->cc->cursor->dx - lb->metrics.x, lb->cc->cursor->dy - lb->metrics.y);
	
	copyAreaNoBlend(lb->surface, frame, lb->metrics.x, lb->metrics.y, lb->metrics.x, lb->metrics.y, lb->metrics.x+lb->metrics.width-1, lb->metrics.y+lb->metrics.height-1);
	//fastFrameCopy(lb->surface, frame, lb->metrics.x, lb->metrics.y);


	if (lb->flags.drawBaseGfx){
		int swidth = ccGetWidth(lb->scrollbar.vert)+2;
		//if (!lb->flags.drawScrollbar){
			swidth = 0;
			lDrawLine(frame, lb->metrics.x+lb->metrics.width, lb->metrics.y, lb->metrics.x+lb->metrics.width, lb->metrics.y+lb->metrics.height-1, (255<<24)|COL_BLUE_SEA_TINT);
		//}
		lDrawLine(frame, lb->metrics.x, lb->metrics.y, lb->metrics.x+lb->metrics.width-1-swidth, lb->metrics.y, (255<<24)|COL_BLUE_SEA_TINT);
		lDrawLine(frame, lb->metrics.x, lb->metrics.y+1, lb->metrics.x, lb->metrics.y+lb->metrics.height-1, (255<<24)|COL_BLUE_SEA_TINT);
		lDrawLine(frame, lb->metrics.x, lb->metrics.y+lb->metrics.height-1, lb->metrics.x+lb->metrics.width-1-swidth, lb->metrics.y+lb->metrics.height-1, (255<<24)|COL_BLUE_SEA_TINT);
	}
	
	if (lb->flags.drawScrollbar)
		ccRender(lb->scrollbar.vert, frame);	
	
	//lDrawRectangle(frame, lb->metrics.x, lb->metrics.y, lb->metrics.x+lb->metrics.width-1, lb->metrics.y+lb->metrics.height-1, DRAWTOUCHRECTCOL);
	return 1;
}

static inline int isOverlap (TTOUCHCOORD *pos, const TCCOBJECT *obj)
{
	if (pos->y >= obj->metrics.y && pos->y < obj->metrics.y+obj->metrics.height){
		if (pos->x >= obj->metrics.x && pos->x < obj->metrics.x+obj->metrics.width)
			return 1;
	}
	return 0;
}

int lbHandleInput (void *object, TTOUCHCOORD *pos, const int flags)
{
	TLB *lb = (TLB*)object;
	
	int ret = ccHandleInput(lb->scrollbar.vert, pos, flags);
	//printf("lbhandleinput sb %i, %i %i %i %i\n", ret, lb->metrics.x, lb->metrics.y, lb->metrics.width, lb->metrics.height);
	if (ret) return ret;

	if (lb->enabled /*&& isOverlap(pos, object)*/){
		TTOUCHCOORD local;
		const int total = buttonsTotalGet(lb->ccbtns);
		
		for (int i = 0; i < total; i++){
			TCCBUTTON *btn = buttonsButtonGet(lb->ccbtns, i);
			//printf("lbHandleInput %i: %i %i %i\n", i, btn->enabled, btn->metrics.y, pos->y);
			if (btn->enabled){
				my_memcpy(&local, pos, sizeof(TTOUCHCOORD));
				int ret = ccHandleInput(btn, &local, flags);
				if (ret) return ret;
			}
		}
	}
	return ret;
}

int lbSetMetrics (void *object, int x, int y, int width, int height)
{
	TLB *lb = (TLB*)object;
	
	int64_t focus = lbGetFocus(lb);
	if (width > lb->maxWorkingWidth) width = lb->maxWorkingWidth;
	if (height > lb->maxWorkingHeight) height = lb->maxWorkingHeight;
	
	lb->metrics.x = x;
	lb->metrics.y = y;
	lb->metrics.width = width;
	lb->metrics.height = height;

	if (lb->scrollbar.position == LISTBOX_SBPOSITION_RIGHT)
		ccSetMetrics(lb->scrollbar.vert, (lb->metrics.x + lb->metrics.width - ccGetWidth(lb->scrollbar.vert) - 1) - lb->scrollbar.offset, y+1, -1, height-2);
	else if (lb->scrollbar.position == LISTBOX_SBPOSITION_LEFT)
		ccSetMetrics(lb->scrollbar.vert, lb->metrics.x + lb->scrollbar.offset, y+1, -1, height-2);
	
	lbBuildMetrics(lb, focus);
	
	if (lbScrollbarCalcRange(lb) <= lb->metrics.height)
		ccDisable(lb->scrollbar.vert);
	else
		ccEnable(lb->scrollbar.vert);

	//lbSetFocus(lb, focus);
	return 1;
	
}

int lbSetPosition (void *object, const int x, const int y)
{
	TLB *lb = (TLB*)object;
	lb->metrics.x = x;
	lb->metrics.y = y;

	if (lb->scrollbar.position == LISTBOX_SBPOSITION_RIGHT)
		ccSetMetrics(lb->scrollbar.vert, (x + lb->metrics.width - ccGetWidth(lb->scrollbar.vert) - 1) - lb->scrollbar.offset, y+1, -1, -1);
	else if (lb->scrollbar.position == LISTBOX_SBPOSITION_LEFT)
		ccSetMetrics(lb->scrollbar.vert, x + lb->scrollbar.offset, y+1, -1, -1);

	lbBuildMetrics(lb, lbGetFocus(lb));
	
	if (lbScrollbarCalcRange(lb) <= lb->metrics.height)
		ccDisable(lb->scrollbar.vert);
	else
		ccEnable(lb->scrollbar.vert);
		
	return 1;
}

void lbDelete (void *object)
{
	TLB *lb = (TLB*)object;

	ccDelete(lb->scrollbar.vert);
	lDeleteFrame(lb->surface);
	if (lb->ccbtns) 
		buttonsDeleteAll(lb->ccbtns);
	lb->ccbtns = NULL;
}

int lbNew (TCCOBJECT *object, void *unused, const int pageOwner, const int type, const TCommonCrtlCbMsg_t lb_cb, int *id, const int width, const int height)
{
	TLB *lb = (TLB*)object;
	

	if (id) *id = lb->id;
	lb->type = type;

	lb->cb.msg = lb_cb;
	lb->cb.render = lbRender;
	lb->cb.create = lbNew;
	lb->cb.free = lbDelete;
	lb->cb.enable = lbEnable;
	lb->cb.disable = lbDisable;
	lb->cb.input = lbHandleInput;
	lb->cb.setPosition = lbSetPosition;
	lb->cb.setMetrics = lbSetMetrics;
	
	//lb->cursor.x = -1;
	//lb->cursor.y = -1;
	lb->metrics.x = 0;
	lb->metrics.y = 0;
	lb->metrics.width = width;
	lb->metrics.height = height;
	lb->maxWorkingWidth = lb->metrics.width;
	lb->maxWorkingHeight = lb->metrics.height;
	
	lb->surface = lNewFrame(lb->cc->vp->ml->hw, lb->cc->vp->ml->width, lb->cc->vp->ml->height, LFRM_BPP_32);
	
	lb->canDrag = 1;
	lb->flags.maxStringWidth = 0;
	lb->flags.justify = PF_MIDDLEJUSTIFY;
	lb->flags.drawScrollbar = 1;
	lb->flags.drawBaseGfx = 1;
	lb->flags.drawBaseBlur = 1;

	lb->font = LISTBOX_FONT_DEFAULT;

	const char *str = "09AIEOUaieouBz{}[]&#12298;&#12299;";
	lGetTextMetrics(lb->cc->vp->ml->hw, str, 0, LFONT, NULL, &lb->verticalPadding);
	lb->verticalPadding += 1;

	lb->scrollbar.vert = ccCreate(lb->cc, pageOwner, CC_SCROLLBAR_VERTICAL, lb_scrollbar_cb, NULL, 0, 0);
	
	ccSetUserData(lb->scrollbar.vert, lb);
	ccSetMetrics(lb->scrollbar.vert, 0, 1, SCROLLBAR_VERTWIDTH, lb->metrics.height-2);
	lbScrollbarCalcRange(lb);
	ccDisable(lb->scrollbar.vert);
	
	lb->scrollbar.position = LISTBOX_SBPOSITION_RIGHT;
	lb->scrollbar.offset = 0;
	lb->scrollbar.vert->isChild = 1;
	lb->scrollbar.vert->flags.drawBlur = 0;
	lb->scrollbar.vert->flags.drawBase = 0;
	lb->scrollbar.vert->flags.drawFrame = 0;
		
	return 1;
}

