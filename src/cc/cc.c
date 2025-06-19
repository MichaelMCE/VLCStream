
// https://github.com/MichaelMCE/VLCStream
// 

// okio@users.sourceforge.net

//  Copyright (c) 2005-2025  
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

#define CC_HOVER_PERIOD		(200.0)


typedef struct{
	char *name;
	int type;
	TCommonCrtlCbCreate_t create;
	int dataLength;
}TCCREG;

static const TCCREG ccRegList[] = {
	{"SliderHori",		CC_SLIDER_HORIZONTAL,		sliderNew,		sizeof(TSLIDER)},
	{"SliderVert",		CC_SLIDER_VERTICAL,			sliderNew,		sizeof(TSLIDER)},
	{"ScrollbarHori",	CC_SCROLLBAR_HORIZONTAL,	scrollbarNew,	sizeof(TSCROLLBAR)},
	{"ScrollbarVert",	CC_SCROLLBAR_VERTICAL,		scrollbarNew,	sizeof(TSCROLLBAR)},
	{"Listbox",			CC_LISTBOX,					lbNew,			sizeof(TLB)},
	{"Treeview",		CC_TV,						tvNew,			sizeof(TTV)},
	{"Panel",			CC_PANEL,					panelNew,		sizeof(TPANEL)},
	{"Label",			CC_LABEL,					labelNew,		sizeof(TLABEL)},
	{"Button",			CC_BUTTON,					ccbuttonNew,	sizeof(TCCBUTTON)},
	{"Keypad",			CC_KEYPAD,					keypadNew,		sizeof(TKEYPAD)},
	{"Button2",			CC_BUTTON2,					ccbutton2New,	sizeof(TCCBUTTON2)},
	{"Pane",			CC_PANE,					paneNew,		sizeof(TPANE)},
	{"Graph",			CC_GRAPH,					graphNew,		sizeof(TGRAPH)},
	{"Listbox2",		CC_LISTBOX2,				listboxNew,		sizeof(TLISTBOX)},
	{"",				0,							NULL,			0}
};


int ccLock (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	return lockWait(obj->lock, INFINITE);
}

void ccUnlock (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	lockRelease(obj->lock);
}

int ccGenerateId (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	return ++obj->cc->ccIdIdx;
}

int64_t ccSendInput (void *object, TTOUCHCOORD *pos, const int flags)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	return obj->cb.input(obj, pos, flags);
}

int64_t ccSendMessage (void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	return obj->cb.msg(obj, msg, data1, data2, dataPtr);
}

int ccSetOwner (void *object, const int newOwner)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	int oldOwner = -1;
	if (ccLock(obj)){
		oldOwner = obj->pageOwner;
		obj->pageOwner = newOwner;
		ccUnlock(obj);
	}
	return oldOwner;
}

int ccGetOwner (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	int owner = -1;
	if (ccLock(obj)){
		owner = obj->pageOwner;
		ccUnlock(obj);
	}
	return owner;
}

void *ccGetPageOwner(void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	void *ptr = NULL;
	if (ccLock(obj)){
		ptr = pageGetPtr(obj->cc->vp, obj->pageOwner);
		ccUnlock(obj);
	}
	return ptr;
	
}

void ccSetUserData (void *object, void *data)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	obj->userPtr = data;
}

void *ccGetUserData (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	return obj->userPtr;
}

void ccSetUserDataInt (void *object, const int64_t data)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	obj->userInt64 = data;
}

int64_t ccGetUserDataInt (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	return obj->userInt64;
}

int ccSetPosition (void *object, const int x, const int y)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	int ret = 0;
	if (ccLock(obj)){
		ccSendMessage(obj, CC_MSG_SETPOSITION, x, y, NULL);
		ret = obj->cb.setPosition(obj, x, y);
		obj->metrics.x = x;
		obj->metrics.y = y;
		ccUnlock(obj);
	}
	return ret;
}

int ccPositionIsOverlapped (void *object, const int x, const int y)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	const int ccx = obj->metrics.x;
	const int ccy = obj->metrics.y;
	const int ccw = obj->metrics.width;
	const int cch = obj->metrics.height;

#if 0	
	int ct = 32;
	while (obj->isChild && ct--){
		if (obj->isChild){
			obj = ccGetUserData(obj);
			if (obj){
				x -= obj->metrics.x;
				y -= obj->metrics.y;
			}
		}
	}
#endif

	if (y >= ccy && y < ccy+cch){
		if (x >= ccx && x < ccx+ccw)
			return 1;
	}
	return 0;
}

void ccGetMetrics (void *object, TMETRICS *metrics)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	my_memcpy(metrics, &obj->metrics, sizeof(TMETRICS));
}

int ccSetMetrics (void *object, int x, int y, int width, int height)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;

	int ret = 0;
	if (ccLock(obj)){
		if (x == -1) x = ccGetPositionX(obj);
		if (y == -1) y = ccGetPositionY(obj);
		if (width == -1) width = ccGetWidth(obj);
		if (height == -1) height = ccGetHeight(obj);
	
		ret = obj->cb.setMetrics(object, x, y, width, height);

		//obj->metrics.width = width;
		//obj->metrics.height = height;
		ccSetPosition(obj, x, y);
		ccUnlock(obj);
	}
	return ret;
}

int ccCanDrag (const TCC *cc, const TTOUCHCOORD *pos, int pageOwner)
{
	const TCCOBJ *list = cc->objs;
	if (!list) return 0;
	//int ct = 1;
	
	do{
		TCCOBJECT *obj = list->obj;
		if (obj && pageOwner == obj->pageOwner){
			if (!obj->isChild && obj->canDrag){
				return 1;
			}
		}
		//ct++;
	}while((list=list->next));

	return 0;
}

static inline int isOverlap (TTOUCHCOORD *pos, const TCCOBJECT *obj)
{
	if (pos->y >= obj->metrics.y && pos->y < obj->metrics.y+obj->metrics.height){
		if (pos->x >= obj->metrics.x && pos->x < obj->metrics.x+obj->metrics.width)
			return 1;
	}
	return 0;
}

void ccInputSlideHoverEnable (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	obj->cc->cursor->slideHoverEnabled = 1;
}

void ccInputSlideHoverDisable (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	obj->cc->cursor->slideHoverEnabled = 0;
}

void ccInputEnable (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	//if (ccLock(obj)){
		obj->processInput = 1;
	//	ccUnlock(obj);
	//}
}

void ccInputDisable (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	//if (ccLock(obj)){
		obj->processInput = 0;
	//	ccUnlock(obj);
	//}
}

static inline TCCOBJECT *ccListRootGetObject (const TCCOBJ *list, const int id)
{
	//TCCOBJ *list = obj->objRoot;
	if (!list) return NULL;

	do{
		if (list->obj && list->obj->id == id)
			return list->obj;
			
	}while((list=list->next));

	return NULL;
}

static inline TCCOBJECT *ccListGetObject (TCCOBJECT *obj, const int id)
{
	return ccListRootGetObject(obj->objRoot, id);
}

int ccHandleInput (void *object, TTOUCHCOORD *pos, const int flags)
{
	int ret = 0;
	TCCOBJECT *obj = (TCCOBJECT*)object;

	if (obj->enabled && obj->processInput && isOverlap(pos, obj)){
		if (!pos->pen && flags == 1){		// drag/slide
			if (pos->id != obj->touchInputId)
				return -1;
		}else if (!pos->pen && !flags){
			if (pos->id == obj->touchInputId)
				return -1;
			else
				obj->touchInputId = pos->id;
		}

		//if (ccSendMessage(object, CC_MSG_INPUT, (pos->x<<16)|(pos->y&0xFFFF), flags, pos))
			ret = ccSendInput(object, pos, flags);
	}
	return ret;
}

int ccSetModal (TCC *cc, const int id)
{
	if (!id){
		cc->modal.state = 0;
		cc->modal.id = 0;
		return 1;
	}
	
	TCCOBJECT *obj = ccListRootGetObject(cc->objs, id);
	if (obj){
		cc->modal.id = id;
		cc->modal.state = 1;
		return id;
	}
	return 0;
}

int ccGetModal (TCC *cc, int *id)
{
	if (id) *id = cc->modal.id;
	return cc->modal.state;
}

static inline void ccListAddObject (TCCOBJ **list, TCCOBJECT *obj)
{
	if (!*list){
		*list = my_malloc(sizeof(TCCOBJ));
		if (!*list) return;

		(*list)->obj = obj;
		(*list)->next = NULL;
		return;
	}

	TCCOBJ *plist = *list;
	do{
		if (!plist->obj){
			plist->obj = obj;
			return;
		}else if (!plist->next){
			break;
		}
	}while((plist=plist->next));

	ccListAddObject(&plist->next, obj);
}

static inline void ccListRemoveObject (TCCOBJ *list, const int id)
{
	//TCCOBJ *list = obj->objRoot;
	//if (!list) return;
	
	do{
		if (list->obj && list->obj->id == id){
			list->obj = NULL;
			return;
		}
	}while((list=list->next));
}

static inline void ccFreeCommon (TCCOBJECT *obj)
{
	//if (obj->ccbtns)
	//	buttonsDeleteAll(obj->ccbtns);
	
	lockClose(obj->lock);
}

static inline void deleteObj (TCCOBJECT *obj, const int genEvent)
{
	if (genEvent){
		ccSendMessage(obj, CC_MSG_DELETE, obj->id, 0, NULL);
	}
	
	ccLock(obj);
	ccListRemoveObject(obj->objRoot, obj->id);
	obj->cb.free(obj);
	ccFreeCommon(obj);
	my_free(obj);
}

#if 0
static inline int deleteStackObjs (TCC *cc)
{
	int total = 0;
	
	int32_t *ids = stackCopyInt32(cc->deleteStack, &total);
	if (ids){
		for (int i = 0; i < total; i++){
			//int id = ids[i];
			intptr_t id = 0;
			if (stackPop(cc->deleteStack, &id)){
				TCCOBJECT *obj = ccListRootGetObject(cc->objs, id);
				if (obj) deleteObj(obj, 0);
			}
		}
		my_free(ids);
	}
	return total;
}

static inline int ccDeleteStackObjs (TCC *cc)
{
	int sum = 0;
	int total = deleteStackObjs(cc);
	while (total > 0){
		sum += total;
		total = deleteStackObjs(cc);
	}
	return sum;
}
	
void ccCleanupMemory(TCC *cc)
{
	ccDeleteStackObjs(cc);
}
#endif

int ccHandleInputAll (TCC *cc, TTOUCHCOORD *pos, const int flags, int page)
{
	const TCCOBJ *list = cc->objs; 
	if (!list) return 0;
	
	int ret = 0;
	TTOUCHCOORD poscpy;
	
	if (cc->modal.state){
		TCCOBJECT *obj = ccListRootGetObject(list, cc->modal.id);
		if (obj && obj->pageOwner == page && obj->processInput && !obj->isChild && obj->enabled){
			if (isOverlap(pos, obj)){
				if (ccLock(obj)){
					my_memcpy(&poscpy, pos, sizeof(TTOUCHCOORD));
					ret = ccHandleInput(obj, &poscpy, flags);
					ccUnlock(obj);
				}
			}
			return ret;
		}else if (!obj){
			ccSetModal(cc, 0);
		}
	}

#if 1
	do{
		if ((volatile TCCOBJECT*)list->obj && list->obj->processInput && list->obj->pageOwner == page && !list->obj->isChild && list->obj->enabled){
			if (isOverlap(pos, list->obj)){
				if (ccLock(list->obj)){
					my_memcpy(&poscpy, pos, sizeof(TTOUCHCOORD));
					ret = ccHandleInput((TCCOBJECT*)list->obj, &poscpy, flags);
					if ((volatile TCCOBJECT*)list->obj)
						ccUnlock(list->obj);
				}
			}
			
			if (ret > 0){
				break;
			}
		}
	}while(!ret && (list=list->next));
#else
	do{
		TCCOBJECT *obj = list->obj;
		if (obj && obj->pageOwner == page && obj->processInput && !obj->isChild && obj->enabled){
			if (isOverlap(pos, obj)){
				if (ccLock(obj)){
					my_memcpy(&poscpy, pos, sizeof(TTOUCHCOORD));
					ret = ccHandleInput(obj, &poscpy, flags);
					ccUnlock(obj);
				}
			}
			if (ret > 0) break;
		}
	}while(!ret && (list=list->next));

	if (!flags){		// free objects on any touch down event
		/*int total = */deleteStackObjs(cc);
	}
#endif
	return ret;
}

void ccEnable (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	
	if (ccLock(obj)){
		obj->cb.enable(obj);
		obj->enabled = 1;
		ccSendMessage(obj, CC_MSG_ENABLED, 0, 0, NULL);
		ccUnlock(obj);
	}
}

void ccDisable (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;

	if (ccLock(obj)){
		obj->cb.disable(object);
		obj->enabled = 0;
		ccSendMessage(obj, CC_MSG_DISABLED, 0, 0, NULL);
		ccUnlock(obj);
	}
}

static inline void drawRectangle (TFRAME *frame, TMETRICS *met, const unsigned int colour)
{
	lDrawRectangle(frame, met->x, met->y, met->x+met->width-1, met->y+met->height-1, colour);
}

static inline void drawRectangleFilled (TFRAME *frame, TMETRICS *met, const unsigned int colour)
{
	lDrawRectangleFilled(frame, met->x, met->y, met->x+met->width-1, met->y+met->height-1, colour);
}

static inline int ccObjectRender (TCCOBJECT *obj, TFRAME *frame)
{
#if DRAWCCOBJBOUNDSFILLED
	drawRectangleFilled(frame, &obj->metrics, 20<<24|DRAWCCOBJCOL);
#endif
#if DRAWCCOBJBOUNDS
	drawRectangle(frame, &obj->metrics, 120<<24|DRAWCCOBJCOL);
#endif
	ccSendMessage(obj, CC_MSG_RENDER, (int)obj->cc->vp->fTime, (int)(obj->cc->vp->rTime*1000.0), frame);
	return obj->cb.render(obj, frame);
}

int ccHoverRenderSigEnable (TCC *cc, const double fps)
{
	cc->cursor->virt.renderMMoveTargetFPS = fps;
	return cc->cursor->virt.enableMMoveRenderSig = 1;
}

void ccHoverRenderSigDisable (TCC *cc)
{
	cc->cursor->virt.enableMMoveRenderSig = 0;
}

int ccHoverRenderSigGetState (TCC *cc)
{
	return cc->cursor->virt.enableMMoveRenderSig;
}

double ccHoverRenderSigGetFPS (TCC *cc)
{
	return cc->cursor->virt.renderMMoveTargetFPS;
}

//MouseMove
int ccIsHoveredMM (TCC *cc, const int owner, const int x, const int y, const int setIfTrue)
{
	int ret = 0;
	TCCOBJ *list = cc->objs;
	do{
		TCCOBJECT *obj = list->obj;
		if (obj && (obj->pageOwner == owner || owner == -1)){
			 if (obj->enabled /*&& !obj->isChild*/){
			 	if (ccPositionIsOverlapped(obj, x, y)){
			 		if (setIfTrue){
			 			//if (!obj->isHovered){
			 				obj->isHovered = 1;
			 				//obj->hoverT0 = getTime(obj->cc->vp);
			 			//}
			 			obj->input_dd_mm = 1;
			 		}
			 		//return obj->id;
			 		ret = obj->id;
			 	}
			 }
		}
	}while((list=list->next));
	
	return ret;
}

// TODO:
// create/add an event to auto reset hovered object
int ccRender (void *object, TFRAME *frame)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;

	if (!obj->enabled) return 0;
	int ret = 0;
	
	if (ccLock(obj)){
		if (obj->cc->cursor->slideHoverEnabled || obj->input_dd_mm || obj->cc->cursor->isHooked){
			const int cx = obj->cc->cursor->dx;
			const int cy = obj->cc->cursor->dy;

			if (/*obj->input_dd_mm || */ccPositionIsOverlapped(obj, cx, cy)){
				if (!obj->isHovered){
					obj->isHovered = 1;
					ccSendMessage(obj, CC_MSG_HOVER, (cx<<16)|cy, obj->isHovered, NULL);
				}
			}else{
				if (obj->isHovered/* && (t1-obj->hoverT0 > CC_HOVER_PERIOD)*/){
					obj->isHovered = 0;
					ccSendMessage(obj, CC_MSG_HOVER, (cx<<16)|cy, obj->isHovered, NULL);
				}
			}
		}else if (obj->isHovered){
			if (obj->isHovered/* && (t1-obj->hoverT0 > CC_HOVER_PERIOD)*/){
				obj->isHovered = 0;
				const int cx = obj->cc->cursor->dx;
				const int cy = obj->cc->cursor->dy;
				ccSendMessage(obj, CC_MSG_HOVER, (cx<<16)|cy, obj->isHovered, NULL);
			}
		}
		obj->input_dd_mm = 0;
		ret = ccObjectRender(obj, frame);
		ccUnlock(obj);
	}
	
	return ret;
}

int ccRenderEx (void *object, TFRAME *frame, const int cx, const int cy)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	if (!obj->enabled) return 0;

	int ret = 0;
	
	if (ccLock(obj)){
		if (obj->cc->cursor->slideHoverEnabled || obj->input_dd_mm || obj->cc->cursor->isHooked){
			//if (obj->input_dd_mm) obj->input_dd_mm = 0;
			if (ccPositionIsOverlapped(obj, cx, cy)){
				if (!obj->isHovered){
					obj->isHovered = 1;
					ccSendMessage(obj, CC_MSG_HOVER, (cx<<16)|cy, obj->isHovered, NULL);
				}
			}else{
				if (obj->isHovered/* && (t1-obj->hoverT0 > CC_HOVER_PERIOD)*/){
					obj->isHovered = 0;
					ccSendMessage(obj, CC_MSG_HOVER, (cx<<16)|cy, obj->isHovered, NULL);
				}
			}
		}
		obj->input_dd_mm = 0;
		ret = ccObjectRender(obj, frame);
		ccUnlock(obj);
	}
	
	return ret;
}

int ccRenderAll (TCC *cc, TFRAME *frame, const int owner, const int cx, const int cy)
{
	int ct = 0;

	TCCOBJ *list = cc->objs;
	do{
		TCCOBJECT *obj = list->obj;
		if (obj && (obj->pageOwner == owner || owner == -1)){
			 if (/*obj->enabled &&*/ !obj->isChild)
				ct += ccRenderEx(obj, frame, cx, cy);
		}
	}while((list=list->next));
	
	return ct;
}

int ccGetState (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	return obj->enabled;
}

int ccGetWidth (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	return obj->metrics.width;
}

int ccGetHeight (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	return obj->metrics.height;
}

void ccGetPosition (void *object, int *x, int *y)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	//ccSendMessage(object, CC_MSG_GETPOSITION, x, y, NULL);
	if (x) *x = obj->metrics.x;
	if (y) *y = obj->metrics.y;
}

int ccGetPositionX (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	return obj->metrics.x;
}

int ccGetPositionY (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	return obj->metrics.y;
}

TCCOBJECT *ccGetObject (TCCOBJECT *obj, const int id)
{
	TCCOBJECT *ret = NULL;
	if (ccLock(obj)){
		ret = ccListGetObject(obj, id);
		ccUnlock(obj);
	}
	return ret;
}

#if 0
int ccDumpRect (TCCOBJ *list, const int page, int x1, int y1, int x2, int y2)
{
	if (x1 > x2) swapInt(&x2, &x1);
	if (y1 > y2) swapInt(&y2, &y1);
	
	int ct = 0;
	do{
		TCCOBJECT *obj = list->obj;
		if (obj->pageOwner == page && !obj->isChild && obj->enabled){
			if (isInside(obj->metrics.x, obj->metrics.y, x1, y1, x2, y2)
			|| isInside(obj->metrics.x+obj->metrics.width-1, obj->metrics.y, x1, y1, x2, y2)
			|| isInside(obj->metrics.x, obj->metrics.y+obj->metrics.height-1, x1, y1, x2, y2)
			|| isInside(obj->metrics.x+obj->metrics.width-1, obj->metrics.y+obj->metrics.height-1, x1, y1, x2, y2)
			){
				printf(":: %i: %i %i %i\n", obj->id, ++ct, obj->type, obj->isChild);
			}
		}
	}while((list=list->next));
	
	
	return ct;
}
#endif


int ccSetImageManager (TCC *cc, const unsigned int managerId, TARTMANAGER *manager)
{
	if (managerId < CC_IMAGEMANAGER_TOTAL){
		cc->im[managerId] = manager;
		return 1;
	}
	return 0;
}

TARTMANAGER *ccGetImageManager (TCC *cc, const unsigned int managerId)
{
	if ( managerId < CC_IMAGEMANAGER_TOTAL)
		return cc->im[managerId];
	else
		return NULL;
}

void *ccCreate (TCC *cc, const int pageOwner, const int type, const TCommonCrtlCbMsg_t obj_cb, int *id, const int var1, const int var2)
{
	return ccCreateEx(cc, pageOwner, type, obj_cb, id, var1, var2, NULL);
}

static inline int ccInitCommon (TCC *cc, TCCOBJECT *obj, const TCCREG *reg, const int pageOwner)
{
	obj->cc = cc;
	obj->type = reg->type;
	obj->lock = lockCreate(reg->name);
	obj->pageOwner = pageOwner;
	obj->isChild = 0;
	obj->id = ccGenerateId(obj);
	ccInputEnable(obj);
	return 1;
}

#if 1
void ccDelete (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;

	ccSendMessage(obj, CC_MSG_DELETE, obj->id, 0, NULL);
	ccLock(obj);
	ccListRemoveObject(obj->objRoot, obj->id);
	obj->cb.free(obj);
	ccFreeCommon(obj);
	my_free(obj);
}

#else
void ccDelete (void *object)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;

	ccSendMessage(obj, CC_MSG_DELETE, obj->id, 0, NULL);
	ccLock(obj);
	obj->cb.free(obj);
	ccUnlock(obj);
	
	if (!stackPush(obj->cc->deleteStack, obj->id))
		printf("ccDelete stack PUSH failed id:%i\n", obj->id);
}
#endif

static inline void ccFreeObjects (const TCC *cc)
{
	TCCOBJ *_list = cc->objs;
	TCCOBJ *list = _list;
	
	while(list){
		if (list->obj){
			if (!list->obj->isChild)
				deleteObj(list->obj, 1);
		}
		list = list->next;
	};

	TCCOBJ *next = NULL;
	list = _list;
	do{
		next = list->next;
		my_free(list);
	}while((list=next));
}

void *ccCreateEx (TCC *cc, const int pageOwner, const int type, const TCommonCrtlCbMsg_t obj_cb, int *id, const int var1, const int var2, void *udataPtr)
{
	TCCOBJECT *obj = NULL;

	for (int i = 0; ccRegList[i].type; i++){
		if (ccRegList[i].type == type){
			obj = my_calloc(1, ccRegList[i].dataLength);
			if (obj){
				ccInitCommon(cc, obj, &ccRegList[i], pageOwner);
				ccSetUserData(obj, udataPtr);
				
				if (!ccRegList[i].create(obj, NULL, pageOwner, type, obj_cb, id, var1, var2)){
					ccFreeCommon(obj);
					my_free(obj);
					return NULL;
				}else{
					ccListAddObject(&cc->objs, obj);
					obj->objRoot = cc->objs;
					ccSendMessage(obj, CC_MSG_CREATE, var1, var2, udataPtr);
					return obj;
				}
			}
		}
	}
	return NULL;
}

TCC *ccInit (TVLCPLAYER *vp, TGUIINPUT *input)
{
	TCC *cc = my_calloc(1, sizeof(TCC));
	if (cc){
		cc->objs = NULL;
		cc->ccIdIdx = 0;
		cc->vp = vp;
		cc->cursor = input;
	}
	return cc;
}

void ccDestroy (TCC *cc)
{
	ccFreeObjects(cc);
}
