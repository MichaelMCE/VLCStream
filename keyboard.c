
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


extern volatile double UPDATERATE_BASE;




TKEYPAD *keyboardGetKeypad (void *pageStruct)
{
	TVLCPLAYER *vp = ((TPAGE2COMOBJ*)pageStruct)->com->vp;
	TKEYBOARD *vkey = pageGetPtr(vp, PAGE_VKEYBOARD);
 	return vkey->kp;
}

static inline int keyboardKeyPress (TKEYBOARD *vkey, TVLCPLAYER *vp, TKEYPAD *kp, TKP_EDITBOX *eb, const int btnId)
{
	if (btnId == VKEY_CB_PASTE){
		//printf("VKEY_CB_PASTE\n");
		keypadPlayKeyAlert(kp);
		HWND hWnd = (HWND)vp->gui.hMsgWin;
		keypadClipBoardGet(kp, hWnd, eb);
		
	}else if (btnId == VKEY_CB_COPY){
		//printf("VKEY_CB_COPY\n");
		keypadPlayKeyAlert(kp);
		HWND hWnd = (HWND)vp->gui.hMsgWin;
		wchar_t *buffer = keypadEditboxGetBufferW(eb);
		if (buffer){
			keypadClipBoardSet(kp, hWnd, buffer);
			my_free(buffer);
		}
	}else if (btnId == VKEY_UNDO){
		keypadPlayKeyAlert(kp);
		//printf("VKEY_UNDO\n");
		keypadEditboxUndoBuffer(eb);

	}else if (btnId == VKEY_CLOSE){
		//printf("VKEY_CLOSE %p\n", ccGetUserData(btn));
		ccDisable(kp);
		/*if (pageGetSec(btn->cc->vp) == PAGE_VKEYBOARD)
			pageSetSec(btn->cc->vp, -1);
		else*/
		if (page2RenderGetState(vp->pages, PAGE_VKEYBOARD))
			page2SetPrevious(vkey);
		//page2RenderDisable(btn->cc->vp->pages, PAGE_VKEYBOARD);
		
	}else if (btnId == VKEY_CARET_LEFT){
		keypadPlayKeyAlert(kp);
		keypadEditboxCaretMoveLeft(eb);

	}else if (btnId == VKEY_CARET_RIGHT){
		keypadPlayKeyAlert(kp);
		keypadEditboxCaretMoveRight(eb);
		
	}else if (btnId == VKEY_CARET_START){
		keypadPlayKeyAlert(kp);
		keypadEditboxCaretMoveStart(eb);
		
	}else if (btnId == VKEY_CARET_END){
		keypadPlayKeyAlert(kp);
		keypadEditboxCaretMoveEnd(eb);
		
	}else{
		return 0;
	}
	return 1;
}

static inline int64_t ccbtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER/* || msg == CC_MSG_INPUT*/) return 1;
	
	TCCBUTTON *btn = (TCCBUTTON*)object;
	
	if (msg == BUTTON_MSG_SELECTED_PRESS){
		TKEYPAD *kp = ccGetUserData(btn);
		TVLCPLAYER *vp = kp->cc->vp;
		TKEYBOARD *vkey = pageGetPtr(vp, ccGetOwner(btn));
		TKP_EDITBOX *eb = &kp->editbox;
		vkey->btns->t0 = getTickCount();

		int btnId = (int)ccGetUserDataInt(btn);
		//printf("keyboard key %i, %i\n", btn->id, btnId);
		keyboardKeyPress(vkey, vp, kp, eb, btnId);
	}

	return 1;
}  

int64_t keypadMsg_cb (const void *object, const int msg, const int64_t dataInt1, const int64_t dataInt2, void *dataPtr)
{
	//printf("keypadMsg_cb msg:%i\n", msg);

#if 0
	if (msg == KP_MSG_PAD_RENDER){
		TKEYPAD *kp = (TKEYPAD*)object;
		keypadDispatchEvent(kp, KP_RENDER_PRE, KP_MSG_PAD_RENDER, dataInt1, dataPtr);
	}
#endif
	return 1;
}

static inline int page_vkbRender (TKEYBOARD *vkey, TVLCPLAYER *vp, TFRAME *frame)
{
	lSetForegroundColour(frame->hw, 0xFFFFFFFF);
	
#if 0
	// display Id of most recently registered listener
	int id = keypadEditboxGetUserData(&vkey->kp->editbox);
	drawHex(frame, (frame->width/2)-32, 24, UPDATERATE_FONT, id, 0);
#endif

	int dt = buttonsRenderAll(vkey->btns, frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);
	ccRender(vkey->kp, frame);
	
	const int64_t dt1 = getTickCount() - vkey->kp->lastKeyPressTime0;
	if (dt1 < UPDATERATE_LENGTH || dt < UPDATERATE_LENGTH)
		setTargetRate(vp, UPDATERATE_BASE_VISUALS);
	else
		setTargetRate(vp, UPDATERATE_BASE);
	return 1;
}

static inline int page_vkbInput (TKEYBOARD *vkey, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
		//keypadPlayKeyAlert(vkey->kp);
		keypadEditboxCaretMoveLeft(&vkey->kp->editbox);
		return keypadEditboxGetCharTotal(&vkey->kp->editbox) > 0;

	  case PAGE_IN_WHEEL_BACK:
		//keypadPlayKeyAlert(vkey->kp);
		keypadEditboxCaretMoveRight(&vkey->kp->editbox);
		return keypadEditboxGetCharTotal(&vkey->kp->editbox) > 0;
	}
		
	return 0;
}

static inline int page_vkbStartup (TKEYBOARD *vkey, TVLCPLAYER *vp, const int fw, const int fh)
{
	
	int id = 0;
	vkey->kp = ccCreate(vp->cc, PAGE_VKEYBOARD, CC_KEYPAD, keypadMsg_cb, &id, 4, 80);
	ccSetMetrics(vkey->kp, -1, -1, fw, fh);
	
	vkey->btns = buttonsCreate(vp->cc, PAGE_VKEYBOARD, VKEY_TOTAL, ccbtn_cb);
	int btnSpace = 92;
	
	TCCBUTTON *btn = buttonsCreateButton(vkey->btns, L"vkeyboard/kp_close.png", NULL, VKEY_CLOSE, 1, 0, 2, 2);
	ccSetUserData(btn, vkey->kp);
	btn = buttonsCreateButton(vkey->btns, L"vkeyboard/kp_paste.png", NULL, VKEY_CB_PASTE, 1, 1, ccGetPositionX(btn) + btnSpace, ccGetPositionY(btn));
	ccSetUserData(btn, vkey->kp);
	btn = buttonsCreateButton(vkey->btns, L"vkeyboard/kp_copy.png", NULL, VKEY_CB_COPY, 1, 1, ccGetPositionX(btn) + btnSpace, ccGetPositionY(btn));
	ccSetUserData(btn, vkey->kp);
	btn = buttonsCreateButton(vkey->btns, L"vkeyboard/kp_undo.png", NULL, VKEY_UNDO, 1, 1, ccGetPositionX(btn) + btnSpace, ccGetPositionY(btn));
	ccSetUserData(btn, vkey->kp);
	btn = buttonsCreateButton(vkey->btns, L"vkeyboard/kp_left.png", NULL, VKEY_CARET_LEFT, 1, 1, ccGetPositionX(btn) + (btnSpace*1.75), ccGetPositionY(btn));
	ccSetUserData(btn, vkey->kp);
	btn = buttonsCreateButton(vkey->btns, L"vkeyboard/kp_right.png", NULL, VKEY_CARET_RIGHT, 1, 1, ccGetPositionX(btn) + btnSpace, ccGetPositionY(btn));
	ccSetUserData(btn, vkey->kp);
	btn = buttonsCreateButton(vkey->btns, L"vkeyboard/kp_start.png", NULL, VKEY_CARET_START, 1, 1, ccGetPositionX(btn) + btnSpace, ccGetPositionY(btn));
	ccSetUserData(btn, vkey->kp);
	btn = buttonsCreateButton(vkey->btns, L"vkeyboard/kp_end.png", NULL, VKEY_CARET_END, 1, 1, ccGetPositionX(btn) + btnSpace, ccGetPositionY(btn));
	ccSetUserData(btn, vkey->kp);
	return 1;
}

static inline int page_vkbInitalize (TKEYBOARD *vkey, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_VKEYBOARD);
	
	
	TKEYPAD *kp = vkey->kp;
	buildKeypad(kp);
	vkey->btns->t0 = getTickCount()-5000;
	return 1;
}

static inline int page_vkbShutdown (TKEYBOARD *vkey, TVLCPLAYER *vp)
{
	ccDelete(vkey->kp);
	buttonsDeleteAll(vkey->btns);
	return 1;
}

void page_vkbRenderStart (TKEYBOARD *vkey, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	//printf("page_vkbRenderStart\n");
	
	TKEYPAD *kp = vkey->kp;
	if (!ccGetState(kp)) ccEnable(kp);
	ccHoverRenderSigEnable(vp->cc, 16.0);
}

void page_vkbRenderEnd (TKEYBOARD *vkey, TVLCPLAYER *vp, int64_t destId, int64_t data2, void *opaquePtr)
{
	//printf("page_vkbRenderEnd\n");
	ccHoverRenderSigDisable(vp->cc);
	TKEYPAD *kp = vkey->kp;
	if (ccGetState(kp)) ccDisable(kp);
}

static inline int page_vkbInputKeyDown (TKEYBOARD *vkey, TVLCPLAYER *vp, const int key, const int64_t modifierFlags)
{
#if 0
	if (key != 13)
		printf("page_vkbInputKeyDown '%c' %i %I64X\n", key, key, modifierFlags);
	else
		printf("page_vkbInputKeyDown %i %I64X\n", key, modifierFlags);
#endif

	TKEYPAD *kp = vkey->kp;
	
	if (key == VK_HOME)
		return keyboardKeyPress(vkey, vp, kp, &kp->editbox, VKEY_CARET_START);
	else if (key == VK_END)
		return keyboardKeyPress(vkey, vp, kp, &kp->editbox, VKEY_CARET_END);
	else if (key == VK_LEFT)
		return keyboardKeyPress(vkey, vp, kp, &kp->editbox, VKEY_CARET_LEFT);
	else if (key == VK_RIGHT)
		return keyboardKeyPress(vkey, vp, kp, &kp->editbox, VKEY_CARET_RIGHT);
	else if (key == VK_ESCAPE)
		return keyboardKeyPress(vkey, vp, kp, &kp->editbox, VKEY_CLOSE);
	else if (key == VK_CANCEL)
		return keyboardKeyPress(vkey, vp, kp, &kp->editbox, VKEY_CB_COPY);
	else if (key == 0x16 && (modifierFlags&KP_VK_CONTROL))
		return keyboardKeyPress(vkey, vp, kp, &kp->editbox, VKEY_CB_PASTE);
	else if (key == 'Z' && (modifierFlags&KP_VK_CONTROL))
		return keyboardKeyPress(vkey, vp, kp, &kp->editbox, VKEY_UNDO);
	else if (key == '4' && (modifierFlags&(KP_VK_CONTROL|KP_VK_ALT)))
		return keypadInputKeyPressCb(vkey->kp, KP_EURO, modifierFlags);
	else if (key == '.'/*VK_DELETE*/ && (!modifierFlags || modifierFlags&KP_VK_REPEAT))	// forward/right side delete
		return keypadInputKeyPressCb(vkey->kp, KP_KEYS_DELETE, modifierFlags);
		
	return 0;
}

static inline int page_vkbInputCharDown (TKEYBOARD *vkey, TVLCPLAYER *vp, const int key, const int64_t modifierFlags)
{
	if (key == VK_CANCEL || key == 0x16)	// copy and paste
		return page_vkbInputKeyDown(vkey, vp, key, modifierFlags);
	else if (key < VK_SPACE && key != VK_RETURN && key != VK_BACK)
		return 0;
#if 0
	if (key != 13)
		printf("page_vkbInputCharDown '%c' %X/%i %I64X\n", key, key, key, modifierFlags);
	else
		printf("page_vkbInputCharDown %X/%i %I64X\n", key, key, modifierFlags);
#endif

	return keypadInputKeyPressCb(vkey->kp, key, modifierFlags);
}

int page_vkbCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TKEYBOARD *vkey = (TKEYBOARD*)pageStruct;
	
	//if (msg != PAGE_CTL_RENDER)
	//	 printf("# page_vkbCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		TKEYPAD *kp = vkey->kp;
		keypadDispatchEvent(kp, KP_RENDER_PRE, KP_MSG_PAD_RENDER, dataInt1, dataPtr);
		return page_vkbRender(vkey, vkey->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		page_vkbRenderStart(vkey, vkey->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		page_vkbRenderEnd(vkey, vkey->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_vkbInput(vkey, vkey->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_MSG_CHAR_DOWN){
		if (page_vkbInputCharDown(vkey, vkey->com->vp, dataInt1, dataInt2))
			pageUpdate(vkey);
		else
			return 0;
	}else if (msg == PAGE_MSG_KEY_DOWN){
		if (page_vkbInputKeyDown(vkey, vkey->com->vp, dataInt1, dataInt2))
			pageUpdate(vkey);
		else
			return 0;
	}else if (msg == PAGE_CTL_STARTUP){
		return page_vkbStartup(vkey, vkey->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_vkbInitalize(vkey, vkey->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_vkbShutdown(vkey, vkey->com->vp);
		
	}
	
	return 1;
}


