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
#include <psapi.h>



enum _panelbtn {
	GHK_BUTTON_1 = 0,
	GHK_BUTTON_2,
	//GHK_BUTTON_3,
	GHK_BUTTON_TOTAL
};

#define GHK_BUTTON_BACK				GHK_BUTTON_1
#define GHK_BUTTON_TOGGLEKEYNAMES	GHK_BUTTON_2
#define VLCFILENAME					L"\\vlc.exe"






void ghkSendHotkey (const int modA, const int modB, const char key)
{
	if (modA) keybd_event(modA, 0, 0, 0);
	if (modB) keybd_event(modB, 0, 0, 0);
	keybd_event(key, 0, 0, 0);
	
	Sleep(50);
	
	keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
	if (modB) keybd_event(modB, 0, KEYEVENTF_KEYUP, 0);
	if (modA) keybd_event(modA, 0, KEYEVENTF_KEYUP, 0);
}



// 'Default IME'
// 'vlc'
// 'vlc ghk'
// 'MSCTFIME UI'
// 'Playlist'
// 'Adjustments and Effects'
// 'Messages'
// 'Program Guide'
// ''
char *ghkGetRunningVLCWindowTitle (HWND hWnd, char *buffer, const int blen)
{

	//HWND hWnd = FindWindowExW(0, NULL, L"QWidget", NULL);
	const int vlen = wcslen(VLCFILENAME);
	//printf("ghkGetRunningVLCWindowTitle %p\n", hWnd);

	while(hWnd){
		int pid = processGetWindowThreadPid(hWnd);
		//printf("pid %i\n", pid);
		if (pid){
			wchar_t *path = processGetFilename(pid);
			if (path){
				int plen = wcslen(path);
				int dlen = plen - vlen;
				if (dlen < 4){
					my_free(path);
					return NULL;
				}
				wchar_t *file = &path[dlen];
				if (!_wcsicmp(file, VLCFILENAME)){
					wchar_t title[2048] = {0};
					GetWindowTextW(hWnd, title, 2047);
						
					//wprintf(L"window: '%s' #%s#\n", title, file);
						
					if (*title){
						wchar_t *found = wcsstr(title, L" - VLC media player");
						if (!found){
							found = wcsstr(title, L"VLC media player");
							if (found){
								strcpy(buffer, " - ");
								my_free(path);
								return buffer;
							}
						}
						if (found){
							*found = 0;
							//wprintf(L"%i %p #%s#\n", (int)pid, hWnd, path);
							//wprintf(L"'%s' #%s#\n", file, title);
							char *title8 = convertto8(title);
							if (title8){
								strncpy(buffer, title8, blen);
								my_free(title8);
								my_free(path);
								return buffer;
							}
						}
					}
				}
				my_free(path);
			}
		}
		hWnd = GetWindow(hWnd, GW_HWNDNEXT);
	}

	return NULL;
}

int ghkIsVlcRunning ()
{
	return 	FindWindowExW(0, 0, L"QToolTip", NULL) && 
			FindWindowExW(0, 0, L"QPopup", NULL) && 
			FindWindowExW(0, 0, L"QWidget", NULL);

	/*wchar_t buffer[64];

	for (int p = 0; p <= 2; p++){
		for (int i = 0; i <= 10; i++){
			snwprintf(buffer, sizeof(buffer), L"VLC ghk 2.%d.%d", p, i);
			HWND hWnd = FindWindowW(NULL, buffer);

			//wprintf(L"'%s' %p\n",buffer, hWnd);
			if (hWnd) return 1;
		}
	}*/

	return 0;
}

void ghkFreeKey (THOTKEY *hk)
{
	if (hk){
		my_free(hk->name);
		my_free(hk->imagePath);
		my_free(hk);
	}
}

void ghkFreeKeyList (THOTKEY **keys, int total)
{
	if (total){
		while(total--)
			ghkFreeKey(keys[total]);
		my_free(keys);
	}
}

THOTKEY *ghkAllocKey ()
{
	return my_calloc(1, sizeof(THOTKEY));
}

THOTKEY **ghkAllocKeyList (const int total)
{
	return my_calloc(total+1, sizeof(THOTKEY*));
}


static inline int ghkPanButtonPress (TGLOBALHOTKEYS *ghk, TCCBUTTON *btn, const int btn_id, const TTOUCHCOORD *pos)
{
	//TVLCPLAYER *vp = btn->cc->vp;
	//printf("ghkPanButtonPress: %i: %i,%i\n", btn_id, pos->x, pos->y);

	TPANEL *panel = ghk->panel; //(TPANEL*)buttonGetUserData(button);
	if (!panel) return 0;

	panel->btns->t0 = getTickCount();	

	
	switch (btn_id){
	  case GHK_BUTTON_BACK:
	  	page2SetPrevious(ghk);
		return 1;

	  case GHK_BUTTON_TOGGLEKEYNAMES:
	  	ghk->showNames ^= 1;
		ghkPanelBuild(ghk->panel, ghk->keys, ghk->totalKeys, ghk->showNames);
		return 1;
	}

	return 0;	
}

static inline int64_t ccbtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;
		
	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//printf("ccbtn_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);

	TCCBUTTON *btn = (TCCBUTTON*)object;
	//const int id = (int)data2;

	if (msg == BUTTON_MSG_SELECTED_PRESS)
		return ghkPanButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	return 1;
}


int64_t ghk_panel_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;
	
	TPANEL *panel = (TPANEL*)object;
	//TVLCPLAYER *vp = panel->cc->vp;
	//printf("ghk_panel_cb %p, %p, %i %i %i %p\n", panel, vp, msg, data1, data2, dataPtr);
	
	if (msg == PANEL_MSG_ITEMSELECTED){
		THOTKEY *hk = panelImgStorageGet(panel, data1);
		if (hk && hk->key){
			//printf("key %i %i %c\n", hk->modifierA, hk->modifierB, hk->key);
			ghkSendHotkey(hk->modifierA, hk->modifierB, hk->key);
		}
	}
	return 1;
}

int ghkPanelAddKeys (TPANEL *panel, THOTKEY **keys, const int total, const int showNames)
{

	for (intptr_t i = 0; i < total; i++){
		THOTKEY *key = keys[i];
		wchar_t *path = converttow(key->imagePath);
		if (path){
			if (!showNames)
				key->id = panelImgAdd(panel, genAMId(panel,path), " ", (void*)i);
			else
				key->id = panelImgAdd(panel, genAMId(panel,path), key->name, (void*)i);
			//printf("ghkPanelAddKeys: %i, %i '%s'\n", i, key->id, key->imagePath);
			my_free(path);
			
			
			//panelImgAddSubtext(panel, key->id, buffer, 240<<24|COL_WHITE);
			panelImgStorageSet(panel, key->id, key);
			panelImgAttributeSet(panel, key->id, PANEL_ITEM_GENERAL);
			TPANELIMG *item = panelImgGetItem(panel, key->id);
			if (item) button2AnimateSet(item->btn, 1);
		}
	}

	//TMETRICS metrics = {0, 0, panel->metrics.width, panel->metrics.height};	
	//panel->vHeight = panelImgPositionMetricsCalc(panel->list, panel->listSize, &metrics, panel->itemHoriSpace, panel->itemVertSpace);
	return 1;
}

int ghkPanelBuild (TPANEL *panel, THOTKEY **keys, const int total, const int showNames)
{
	panelListResize(panel, total, 0);
	ghkPanelAddKeys(panel, keys, total, showNames);
	panelInvalidate(panel);
	
	return 1;
}

void ghkRenderTitle (TVLCPLAYER *vp, TFRAME *frame, const char *text)
{
	int x = 0;
	int y = 8;
	const int font = LFTW_B28;
	const int colour = 241<<24|COL_WHITE;
	lSetForegroundColour(frame->hw, colour);

	unsigned int shash = colour^generateHash(text, strlen(text)*sizeof(char));
	TFRAME *str = strcFindString(vp->strc, shash);
	if (!str){
		TMETRICS metrics;
		metrics.x = 1;
		metrics.y = 0;
		metrics.width = 0;
		metrics.height = 0;
		
		str = newStringEx(frame->hw, &metrics, frame->bpp, 0, font, (char*)text, frame->width, NSEX_RIGHT);
		if (str) strcAddString(vp->strc, str, shash);
	}
	if (str){
		x = (frame->width - str->width)/2;
		drawShadowedImage(str, frame, x, y, 0, 2, 1, 0);
	}
}

static inline int page_ghkRender (TGLOBALHOTKEYS *ghk, TVLCPLAYER *vp, TFRAME *frame)
{
	lSetCharacterEncoding(frame->hw, CMT_UTF8);
	
	if (vp->gui.hotkeys.isVlcRunning || vp->gui.hotkeys.alwaysAccessible){
		if (ghk->vlcTitle[0])
			ghkRenderTitle(vp, frame, ghk->vlcTitle);
	}
	
	//printf("vlc running: %i '%s'\n", ghk->isVlcRunning, ghk->vlcTitle);
	ccRender(ghk->panel, frame);
	return 1;
}

static inline int page_ghkInput (TGLOBALHOTKEYS *ghk, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
	  case PAGE_IN_WHEEL_BACK:{
	  	TTOUCHSWIPE *swipe = &ghk->panel->swipe;
	  	swipeReset(swipe);
	  	
	  	swipe->t0 = getTime(vp)-100.0;
	  	swipe->i32value = ghk->panel->itemOffset->y;
	  	swipe->sx = 10;
	  	swipe->sy = 150;
	  	swipe->ex = 10;
		swipe->ey = swipe->sy-25;
		swipe->dx = swipe->ex - swipe->sx;
		if (msg == PAGE_IN_WHEEL_FORWARD)
			swipe->dy = -(swipe->ey - swipe->sy);
		else
			swipe->dy = swipe->ey - swipe->sy;
		
		swipe->state = 0;
	  	swipe->adjust = swipe->dy;
	  	swipe->dt = 100.0;
		swipe->decayAdjust = abs(swipe->dy)/(double)swipe->dt;
		swipe->decayAdjust *= swipe->velocityFactor;
	  	swipe->state = 0;

	  	break;
	  }
	}
		
	return 1;
}

static inline int page_ghkStartup (TGLOBALHOTKEYS *ghk, TVLCPLAYER *vp, const int width, const int height)
{
	
	settingsGet(vp, "hotkeys.global.enabled", &vp->gui.hotkeys.globalEnabled);
	if (!vp->gui.hotkeys.globalEnabled) return 0;
	
	ghk->keys = NULL;
	ghk->totalKeys = 0;
	ghk->validKeys = 0;
	ghk->showNames = 0;
	ghk->vlcTitle[0] = 0;
	
	
	TPANEL *panel = ccCreate(vp->cc, PAGE_HOTKEYS, CC_PANEL, ghk_panel_cb, &vp->gui.ccIds[CCID_PANEL_HOTKEYS], GHK_BUTTON_TOTAL, 0);
	ghk->panel = panel;
	panel->itemOffset = &ghk->itemOffset;
	panel->itemOffset->x = 0;
	panel->itemOffset->y = 0;
	panel->itemMaxWidth = width * 0.30;
	panel->font = GBK_PANEL_FONT;
	
	ccSetUserData(panel, ghk);
	int y = (height * 0.122)+1;
	
	panelSetBoundarySpace(panel, 10, 40);
	ccSetMetrics(panel, 0, y, width, height - y);
	ccEnable(panel);
	
	
	//const int hoverCol = swatchGetColour(vp, PAGE_NONE, SWH_PLY_ISELECTED);
	TCCBUTTON *btn = panelSetButton(panel, GHK_BUTTON_BACK, L"common/back_right96.png", NULL, ccbtn_cb);
	ccSetPosition(btn, width - ccGetWidth(btn)-1, 0);
	
	btn = panelSetButton(panel, GHK_BUTTON_TOGGLEKEYNAMES, L"common/nametoggle.png", NULL, ccbtn_cb);
	ccSetPosition(btn, 0, 0);
	btn->flags.canAnimate = 1;
	
	return 1;
}

static inline int page_ghkInitalize (TGLOBALHOTKEYS *ghk, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_HOTKEYS);
	
	settingsGet(vp, "home.hotkeys.alwaysAccessible", &vp->gui.hotkeys.alwaysAccessible);
	settingsGet(vp, "hotkeys.global.showLabels", &ghk->showNames);
	settingsGet(vp, "hotkeys.local.cursor", &vp->gui.hotkeys.cursor);
	settingsGet(vp, "hotkeys.local.console", &vp->gui.hotkeys.console);

	
	str_list *strList = NULL;
	settingsGet(vp, "hotkeys.global.", &strList);
	if (strList){
		TGLOBALHOTKEYS *ghk = pageGetPtr(vp, PAGE_HOTKEYS);
		ghkFreeKeyList(ghk->keys, ghk->totalKeys);
		ghk->keys = ghkAllocKeyList(strList->total);
		ghk->totalKeys = strList->total;
		int validKeys = 0;

		THOTKEY hk;
		char modifierStrA[64];
		char modifierStrB[64];
		char hkname[256];
		char imagePath[MAX_PATH_UTF8+1];		

		for (int i = 0; i < strList->total; i++){
			char *name = cfg_configStrListItem(strList, i);
			if (name){
				memset(&hk, 0, sizeof(hk));

				//     modifierA,modifierB,key,image_path
				// eg; CTRL,ALT,A,hotkeys/image1.png

				int slen = strlen(name);
				int c  = strcspn(name, ",");
				*modifierStrA = 0;
				strncpy(modifierStrA, name, c);
				if (*modifierStrA){
					modifierStrA[c] = 0;
					hk.modifierA = hkModiferToKey(modifierStrA);
				}
				int i = c + 1;
				
				if (i >= slen-1) continue;
				c = strcspn(name+i, ",");
				*modifierStrB = 0;
				strncpy(modifierStrB, name+i, c);
				if (*modifierStrB){
					modifierStrB[c] = 0;
					hk.modifierB = hkModiferToKey(modifierStrB);
				}
				i += c + 1;
				
				if (i >= slen-1) continue;
				c = strcspn(name+i, ",");
				hk.key = *(name+i);
				i += c + 1;
				
				
				if (i >= slen-1) continue;
				c = strcspn(name+i, ",");
				strncpy(hkname, name+i, c);
				if (*hkname){
					hkname[c] = 0;
					hk.name = my_strdup(hkname);
				}
				i += c + 1;
				
				if (i >= slen-1) continue;
				*imagePath = 0;
				strncpy(imagePath, name+i, slen-i);
				if (*imagePath){
					imagePath[slen-i] = 0;
					hk.imagePath = my_strdup(imagePath);
				}else{
					continue;
				}
				
				ghk->keys[validKeys] = ghkAllocKey();
				my_memcpy(ghk->keys[validKeys], &hk, sizeof(THOTKEY));

				//printf("name:'%s'\nmodA:%s/%i\nmodB:%s/%i\nkey:%c\nimage:%s\n\n", hk.name,
				//		  modifierStrA, hk.modifierA, modifierStrB, hk.modifierB, hk.key, hk.imagePath);

				validKeys++;
			}
			ghk->totalKeys = validKeys;
		}
		cfg_configStrListFree(strList);
		my_free(strList);
	}
	
	return 1;
}

static inline int page_ghkShutdown (TGLOBALHOTKEYS *ghk, TVLCPLAYER *vp)
{
	ccDelete(ghk->panel);
	ghkFreeKeyList(ghk->keys, ghk->totalKeys);
	return 1;
}

int page_ghkCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TGLOBALHOTKEYS *ghk = (TGLOBALHOTKEYS*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_ghkCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_ghkRender(ghk, ghk->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		timerSet(ghk->com->vp, TIMER_STATEHELPER, 500);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_ghkInput(ghk, ghk->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_ghkStartup(ghk, ghk->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_ghkInitalize(ghk, ghk->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_RENDER_INIT){
		ghkPanelBuild(ghk->panel, ghk->keys, ghk->totalKeys, ghk->showNames);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_ghkShutdown(ghk, ghk->com->vp);
		
	}
	
	return 1;
}

