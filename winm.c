
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


// http://www.jstookey.com/arcade/rawmouse/

#define OEMRESOURCE 1




#include "common.h"
/*#define CINTERFACE
#define COBJMACROS
#include <dbt.h>
#include <shlobj.h>
#include <initguid.h>
#include <shobjidl.h>
#undef CINTERFACE
#undef COBJMACROS*/

#include <dbt.h>
#include <initguid.h>
#include <gdiplus/gdiplus.h>




#define VK_RED					VK_F1
#define VK_ORANGE				VK_F2
#define VK_BLUE					VK_F3
#define VK_YELLOW				VK_F4
#define VK_SEEK_BACK			'B'
#define VK_SEEK_FORWARD			'F'
//#define VK_FULLSCREEN			'3'

#define VK_MEDIA_SEEK_BACK		VK_SEEK_BACK
#define VK_MEDIA_SEEK_FOWARD	VK_SEEK_FORWARD





#ifndef SHCNF_ACCEPT_INTERRUPTS
#define SHCNF_ACCEPT_INTERRUPTS 0x0001 
#define SHCNF_ACCEPT_NON_INTERRUPTS 0x0002
#define SHCNF_NO_PROXY 0x8000
#endif

#ifndef RIDEV_EXINPUTSINK
#define RIDEV_EXINPUTSINK 0x1000
#endif

#define MOUSE_CAP_OFF		0
#define MOUSE_CAP_ON		1
static int mouseCapState = MOUSE_CAP_OFF;

static unsigned int taskbarRestartMsg[2];


enum _menustringids {
	MENU_STRING_Position,
	MENU_STRING_Volume,
	MENU_STRING_Mute,
	MENU_STRING_PlayInVLC,
	MENU_STRING_Chapters,
	MENU_STRING_Programme,
	MENU_STRING_Subtitles,
	MENU_STRING_Play,
	MENU_STRING_Stop,
	MENU_STRING_Previous,
	MENU_STRING_Next,
	MENU_STRING_Screenshot,
	MENU_STRING_Playlist,
	MENU_STRING_Flush,
	MENU_STRING_RTD,
	MENU_STRING_Quit,
	
	MENU_STRING_TOTAL
};

static const char *menuStrings[MENU_STRING_TOTAL] = {
	"  Position",
	"  Volume",
	"  Toggle Mute",
	"  Divert to VLC",
	"  Chapters",
	"  Programme",
	"  Subtitles",
	"  Play",
	"  Stop",
	"  Previous",
	"  Next",
	"  Save Screenshot",
	"  Save Playlist",
	"  Flush Caches",
	"  Render to Desktop",
	"  Quit"
};


static inline int contextMenuLoadImage (TVLCSTRAY *tray, const int how, const int bm_idx, const wchar_t *resourceName)
{
	if (how == 1){
		const int flags = LR_LOADTRANSPARENT | LR_DEFAULTSIZE| LR_LOADMAP3DCOLORS | LR_COPYFROMRESOURCE/*|LR_VGACOLOR*/;
		tray->hbm[bm_idx] = (HBITMAP)LoadImageW(GetModuleHandle(0), resourceName, IMAGE_BITMAP, 0, 0, flags);

	}else if (how == 2){
		HBITMAP bm = NULL;
		GpBitmap *gbm = NULL;
	
		if (!GdipCreateBitmapFromResource(GetModuleHandle(0), resourceName, &gbm)){
			if (!GdipCreateHBITMAPFromBitmap(gbm, &bm, 0xFFFFFFFF))
				tray->hbm[bm_idx] = bm;
			GdipDisposeImage(gbm);
		}
	}else if (how == 3){
		HBITMAP bm = NULL;
		GpBitmap *gbm = NULL;
	
		if (!GdipCreateBitmapFromFile(resourceName, &gbm)){
			if (!GdipCreateHBITMAPFromBitmap(gbm, &bm, 0xFFFFFFFF))
				tray->hbm[bm_idx] = bm;
			GdipDisposeImage(gbm);
		}
	}

	return tray->hbm[bm_idx] != NULL;
}

static inline const char *contextMenuGetString (const int idx)
{
	return menuStrings[idx];
}

static inline const char *contextMenuItemToString (const int itemId)
{
	switch (itemId){
	  case TRAY_MENU_POSITION:
	  	return contextMenuGetString(MENU_STRING_Position);
	  case TRAY_MENU_VOLUME:
		return contextMenuGetString(MENU_STRING_Volume);
	  case TRAY_MENU_MUTE:
		return contextMenuGetString(MENU_STRING_Mute);
	  case TRAY_MENU_PLAYINVLC:
		return contextMenuGetString(MENU_STRING_PlayInVLC);
	  case TRAY_MENU_EXIT:
		return contextMenuGetString(MENU_STRING_Quit);
	  case TRAY_MENU_PLAY:
		return contextMenuGetString(MENU_STRING_Play);
	  case TRAY_MENU_STOP:
		return contextMenuGetString(MENU_STRING_Stop);
	  case TRAY_MENU_PREV:
		return contextMenuGetString(MENU_STRING_Previous);
	  case TRAY_MENU_NEXT:
		return contextMenuGetString(MENU_STRING_Next);
	  case TRAY_MENU_CHAPTERS:
		return contextMenuGetString(MENU_STRING_Chapters);
	  case TRAY_MENU_PROGRAMME:
		return contextMenuGetString(MENU_STRING_Programme);
	  case TRAY_MENU_SUBTITLES:
		return contextMenuGetString(MENU_STRING_Subtitles);
	  case TRAY_MENU_SCREENSHOT:
	  	return contextMenuGetString(MENU_STRING_Screenshot);
	  case TRAY_MENU_SAVEPLAYLIST:
	  	return contextMenuGetString(MENU_STRING_Playlist);
	  case TRAY_MENU_FLUSH:
	  	return contextMenuGetString(MENU_STRING_Flush);
	  case TRAY_MENU_RTD:
	  	return contextMenuGetString(MENU_STRING_RTD);
	}
	return "<unknown item>";
}


#if (MOUSEHOOKCAP)


typedef struct {
	uint64_t a4;
	
	uint16_t a16;
	uint16_t b16;
	
	uint8_t a8;
	uint8_t b8;
	uint8_t c8;
	uint8_t d8;
	uint8_t e8;
	uint8_t f8;
	uint8_t g8;
	uint8_t h8;
}guid;


#if 1
//DEFINE_GUID(GUID_INTERFACE_HID_LCD, 0xf1416dc1, 0x9db4, 0x4b93, 0xb2, 0xdf, 0x7c, 0xa1, 0xf3, 0x56, 0x91, 0xe0);
DEFINE_GUID(GUID_DEVINTERFACE_USB_HID,    0x4D1E55B2, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30);
DEFINE_GUID(GUID_DEVINTERFACE_USB_HUB,    0xf18a0e88, 0xc30c, 0x11d0, 0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8);
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L,0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);

//DEFINE_GUID(GUID_INTERFACE_HID_NOTIFY, 0x2c4e2e88L, 0x25e6, 0x4c33, 0x88, 0x2f, 0x3d, 0x82, 0xe6, 0x07, 0x36, 0x81);
//DEFINE_GUID(GUID_INTERFACE_HID_PARSE, 0xf5c315a5, 0x69ac, 0x4bc2, 0x92, 0x79, 0xd0, 0xb6, 0x45, 0x76, 0xf4, 0x4b);

#if 1
UINT (WINAPI *pSHChangeNotifyRegister) (HWND hWnd, DWORD dwFlags, LONG wEventMask, UINT uMsg, DWORD cItems,SHChangeNotifyEntry *lpItems);
WINBOOL (WINAPI *pSHChangeNotifyDeregister) (unsigned long ulID);
HANDLE (WINAPI *pSHChangeNotification_Lock) (HANDLE hChangeNotification,DWORD dwProcessId,LPITEMIDLIST **pppidl,LONG *plEvent);
WINBOOL (WINAPI *pSHChangeNotification_Unlock) (HANDLE hLock);

#define SHChangeNotifyRegister pSHChangeNotifyRegister
#define SHChangeNotifyDeregister pSHChangeNotifyDeregister
#define SHChangeNotification_Lock pSHChangeNotification_Lock
#define SHChangeNotification_Unlock pSHChangeNotification_Unlock

#endif
#endif

static inline unsigned int swapRB (const unsigned int b)
{
	return (b&0x00FF00) | ((b&0xFF0000)>>16) | ((b&0x0000FF)<<16);
}


//if (hwnd) hwnd = FindWindowExA(hwnd, NULL, "SysListView32", NULL);

static inline HWND getDesktopHWND ()
{
	char classname[260];

	HWND hwndChild = GetWindow(GetShellWindow(), GW_CHILD);
	while(hwndChild){
		GetClassNameA(hwndChild, classname, sizeof(classname));
		//printf("#%s#\n", classname);
		if (!stricmp(classname, "SHELLDLL_DefView")){
			HWND hwndChild2 = GetWindow(hwndChild, GW_CHILD);
			//printf("%p\n", hwndChild2);
			if (hwndChild2)
				return hwndChild2;
		}
		hwndChild = GetWindow(hwndChild, GW_HWNDNEXT);
	}
	return NULL;
}

static inline HWND taskbarGetHWND (const char *toolbarName)
{
	char classname[WINTOOLBAR_NANELENGTH+1];
	HWND hwndBar = FindWindowA("Shell_TrayWnd", NULL);
	if (!hwndBar) return NULL;

	HWND hwndChild = GetWindow(hwndBar, GW_CHILD);
	while(hwndChild) {
		GetClassNameA(hwndChild, classname, WINTOOLBAR_NANELENGTH);

		if (!stricmp(classname, "ReBarWindow32")){
			HWND hwndChild2 = GetWindow(hwndChild, GW_CHILD);

			while(hwndChild2){
				GetWindowTextA(hwndChild2, classname, WINTOOLBAR_NANELENGTH);
				if (!strcmp(classname, toolbarName))
					return hwndChild2;
				hwndChild2 = GetWindow(hwndChild2, GW_HWNDNEXT);
			}
			return NULL;
		}
		hwndChild = GetWindow(hwndChild, GW_HWNDNEXT);
	}
	return NULL;
}

static inline void taskbarClear (HWND hwnd)
{
	RedrawWindow(hwnd, NULL, 0, RDW_INVALIDATE|RDW_UPDATENOW);
}

static inline HANDLE taskbarCreateFont (TVLCSTASKBAR *tb)
{
	LOGFONTW lf;
	memset(&lf, 0, sizeof(lf));

	wchar_t *face = converttow(tb->font.name);
	if (!face) return GetStockObject(SYSTEM_FONT);
	wcscpy(lf.lfFaceName, face);
	my_free(face);

	if (tb->font.point > 1){
		lf.lfWidth = 0;
		HDC dc = GetDC(tb->hwnd);
		if (dc == NULL) tb->hwnd = taskbarGetHWND(tb->toolbarName);
		dc = GetDC(tb->hwnd);
		lf.lfHeight = -1 * tb->font.point * GetDeviceCaps(dc, LOGPIXELSY) / 72;
		
		ReleaseDC(tb->hwnd, dc);
	}else{
		lf.lfWidth = tb->font.width;
		lf.lfHeight = tb->font.height;
	}

	lf.lfWeight = tb->font.weight;
	lf.lfQuality = tb->font.quality;
	lf.lfPitchAndFamily = /*FF_SWISS*//*|FF_ROMAN|*/FF_DECORATIVE|VARIABLE_PITCH|FF_SWISS;
	//lf.lfOutPrecision = 
	lf.lfClipPrecision = CLIP_STROKE_PRECIS|CLIP_TT_ALWAYS|CLIP_DFA_DISABLE;
	
	// Rouge Light 10,26
	// Segoe UI Light
	// Palindrome Condensed SSi
	
	HANDLE hFont = CreateFontIndirectW(&lf);
	if (!hFont) hFont = GetStockObject(SYSTEM_FONT);
	return hFont;
}

static inline int taskbarDrawText (TVLCSTASKBAR *tb, const wchar_t *text)
{
	HDC dc = GetDC(tb->hwnd);

	SetTextColor(dc, tb->colour.fore);
	if (tb->colour.bkMode == TB_OPAQUE){
		SetBkColor(dc, tb->colour.back);
		SetBkMode(dc, OPAQUE);
	}else{
		SetBkMode(dc, TRANSPARENT);
	}

	RECT rc;
	GetWindowRect(tb->hwnd, &rc);

	// allign to right side of icons
	rc.right = (rc.right - rc.left)+1;
	rc.bottom = (rc.bottom - rc.top)+1;
	//rc.left = WINTOOLBAR_ICONS * WINTOOLBAR_ICONWIDTH;
	rc.left = tb->pos.x;
	rc.top = tb->pos.y;

	HANDLE hOldFont = SelectObject(dc, tb->font.hfontObj);
	taskbarClear(tb->hwnd);
	int ret = DrawTextW(dc, text, wcslen(text), &rc, DT_SINGLELINE|/*DT_LEFT|*/DT_VCENTER/*|DT_CALCRECT*/);

	SelectObject(dc, hOldFont);
	ReleaseDC(tb->hwnd, dc);
	
	return (ret > 0);
}

static inline void taskbarSetText (TVLCSTASKBAR *tb, const wchar_t *text)
{
	if (!tb->hwnd){
		tb->hwnd = taskbarGetHWND(tb->toolbarName);
		if (tb->hwnd) tb->font.hfontObj = taskbarCreateFont(tb);
	}
		
	int ret = 0;
	if (tb->hwnd)
		ret = taskbarDrawText(tb, text);

	if (!ret) tb->enabled = 0;
}

void taskbarRemoveString (TVLCSTASKBAR *tb)
{
	if (tb->hwnd)
		taskbarClear(tb->hwnd);
}

static inline wchar_t *taskbarBuildStringW (TVLCSTASKBAR *tb, TVLCPLAYER *vp)
{
	char str[MAX_PATH_UTF8+1];
	*str = 0;
	char *arg[8];
	int argct = 0;
	int track = -1;
	
	if (tb->string.track){
		track = getPlayingItem(vp);
		if (track++ >= 0){
			arg[argct] = my_calloc(1, 64);
			__mingw_snprintf(arg[argct], 63, "%i: ", track);
			argct++;
		}
	}
		
	if (tb->string.title){
		arg[argct] = getPlayingProgramme(vp);
		if (!arg[argct]) arg[argct] = getPlayingTitle(vp);
		argct += (arg[argct] != NULL);
	}

	if (tb->string.artist){
		arg[argct] = getPlayingArtist(vp);
		argct += (arg[argct] != NULL);
	}	

	if (tb->string.album){
		arg[argct] = getPlayingAlbum(vp);
		argct += (arg[argct] != NULL);
	}
	
	if (tb->string.description){
		arg[argct] = getPlayingDescription(vp);
		argct += (arg[argct] != NULL);
	}

	if (tb->string.path){
		arg[argct] = getPlayingPath(vp);
		argct += (arg[argct] != NULL);
	}


	for (int i = 0; i < argct; ){
		strncat(str, arg[i], MAX_PATH_UTF8);
		if (++i != argct){
			if (!(i == 1 && track >= 0))
				strncat(str, " - ", MAX_PATH_UTF8);
		}
	}

	if (argct){
		while (argct--) my_free(arg[argct]);
		return converttow(str);
	}else{
		return NULL;
	}
}

static inline void taskbarSetPlayingString (TVLCSTASKBAR *tb, TVLCPLAYER *vp)
{
	wchar_t *str = taskbarBuildStringW(tb, vp);
	if (str){
		taskbarSetText(tb, str);
		my_free(str);
	}
}

static inline void taskbarToolbarRelease (TVLCSTASKBAR *tb)
{
	if (tb->enabled)
		taskbarRemoveString(tb);
	if (tb->font.hfontObj)
		DeleteObject(tb->font.hfontObj);
	if (tb->font.name)
		my_free(tb->font.name);
}

static inline void taskbarToolbarInit (TVLCPLAYER *vp, TVLCSTASKBAR *tb)
{
	char *toolbarName = NULL;
	settingsGet(vp, "taskbar.toolbarName", &toolbarName);
	if (toolbarName){
		strncpy(tb->toolbarName, toolbarName, WINTOOLBAR_NANELENGTH);
		my_free(toolbarName);
	}else{
		tb->enabled = 0;
	}

	settingsGet(vp, "taskbar.pos.x", &tb->pos.x);
	settingsGet(vp, "taskbar.pos.y", &tb->pos.y);

	settingsGet(vp, "taskbar.string.trackNo", &tb->string.track);
	settingsGet(vp, "taskbar.string.title", &tb->string.title);
	settingsGet(vp, "taskbar.string.artist", &tb->string.artist);
	settingsGet(vp, "taskbar.string.album", &tb->string.album);
	settingsGet(vp, "taskbar.string.description", &tb->string.description);
	settingsGet(vp, "taskbar.string.path", &tb->string.path);

	settingsGet(vp, "taskbar.colour.fore", &tb->colour.fore);
	tb->colour.fore = swapRB(tb->colour.fore);
	
	settingsGet(vp, "taskbar.colour.back", &tb->colour.back);
	tb->colour.back = swapRB(tb->colour.back);
	
	settingsGet(vp, "taskbar.colour.mode", &tb->colour.bkMode);
	
	settingsGet(vp, "taskbar.font.name", &tb->font.name);
	settingsGet(vp, "taskbar.font.width", &tb->font.width);
	settingsGet(vp, "taskbar.font.height", &tb->font.height);
	settingsGet(vp, "taskbar.font.point", &tb->font.point);
	settingsGet(vp, "taskbar.font.weight", &tb->font.weight);
	settingsGet(vp, "taskbar.font.quality", &tb->font.quality);

	tb->hwnd = taskbarGetHWND(tb->toolbarName);
	if (tb->hwnd)
		tb->font.hfontObj = taskbarCreateFont(tb);
}

static inline void taskbarTrackRedraw (TVLCPLAYER *vp)
{
	if (!vp->gui.taskbar.enabled) return;
	taskbarSetPlayingString(&vp->gui.taskbar, vp);
}


//TIMER_TASKBARTITLE_UPDATE
void timer_drawTaskbarTrackTitle (TVLCPLAYER *vp)
{
	//printf("timer_drawTaskbarTrackTitle\n");

	if (!vp->gui.taskbar.enabled) return;
	taskbarSetPlayingString(&vp->gui.taskbar, vp);
	
	//if (getPlayState(vp) == 1 || getPlayState(vp) == 2)
		timerSet(vp, TIMER_TASKBARTITLE_UPDATE, 15*1000);
}

static inline int guidToId (const char *guid, int *vid, int *pid)
{
	*vid = 0;
	*pid = 0;
	
	char *v = strchr(guid, '#');
	if (v++){
		if (!strncmp(v, "VID_", 4)){
			sscanf(v+4, "%x", vid);
			if (vid){
				v = strchr(v, '&');
				if (v++){
  					if (!strncmp(v, "PID_", 4)){
						sscanf(v+4, "%x", pid);
						//if (pid)
						//	printf("vid %.4X, pid %.4X\n", *vid, *pid);
					}
				}
			}
		}
	}
	return (*vid) && (*pid);
}

static inline void centreCursor (TVLCPLAYER *vp, int x, int y)
{
	int resetCur = 0;
	  	
	if (x < 15){
	 	x = 15;
	 	resetCur = 1;
	 }else{
		int w = GetSystemMetrics(SM_CXSCREEN);
		if (x > w-15) x = w-15;
		resetCur = 1;
	}
	if (y < 15){
		y = 15;
		resetCur = 1;
	}else{
	    int h = GetSystemMetrics(SM_CYSCREEN);
	    if (y > h-15) y = h-15;
	    resetCur = 1;
	}
	if (resetCur)
		SetCursorPos(x, y);
		
  	vp->gui.cursor.x = x;
  	vp->gui.cursor.y = y;
	vp->gui.cursor.dx = vp->ctx.working->width/2;
	vp->gui.cursor.dy = vp->ctx.working->height/2;
  	vp->gui.cursor.isHooked = 1;
}

static inline void mouseMove (TVLCPLAYER *vp, const int buttonState, const int x, const int y, const int opMode)
{
	if (opMode == 0){
		// sanity check
		if (vp->gui.cursor.isHooked != 1)
	  		centreCursor(vp, x, y);

  		vp->gui.cursor.dx -= (vp->gui.cursor.x - x);
		vp->gui.cursor.dy -= (vp->gui.cursor.y - y);
		
	}else if (opMode == 1){
		vp->gui.cursor.dx += x;
		vp->gui.cursor.dy += y;
		
	}else if (opMode == 2){
		vp->gui.cursor.dx = x;
		vp->gui.cursor.dy = y;
	}

	if (vp->gui.cursor.dx > vp->ctx.working->width-1)
		vp->gui.cursor.dx = vp->ctx.working->width-1;		
	else if (vp->gui.cursor.dx < 0)
		vp->gui.cursor.dx = 0;
		
	if (vp->gui.cursor.dy > vp->ctx.working->height-1)
		vp->gui.cursor.dy = vp->ctx.working->height-1;
	else if (vp->gui.cursor.dy < 0)
		vp->gui.cursor.dy = 0;

  	if (vp->gui.cursor.LBState){	// enable mouse drag
  		TTOUCHCOORD pos;
  		pos.time = getTime(vp);
  		pos.x = vp->gui.cursor.dx;
  		pos.y = vp->gui.cursor.dy;
  		pos.pen = 0;
  		pos.dt = 10;
  		pos.z1 = 100;
  		pos.z2 = 100;
  		pos.pressure = 100;
  		vp->gui.cursor.LBState = 2;
	 	touchSimulate(&pos, TOUCH_VINPUT|1, vp);

  	}else if (opMode != 2){
  		const double t0 = getTime(vp);
 		if (t0 - vp->lastRenderTime > ((1.0/(double)(UPDATERATE_MAXUI+10.0))*1000.0))
  			renderSignalUpdate(vp);
  	}
}

static inline void mouseLBDown (TVLCPLAYER *vp, const int x, const int y)
{
	//printf("mouseLBDown %i,%i\n", x, y);

	TTOUCHCOORD pos;
	pos.x = vp->gui.cursor.dx;
	pos.y = vp->gui.cursor.dy;
	pos.pen = 0;
	pos.time = getTime(vp);
	pos.dt = 1000;
	pos.z1 = 100;
	pos.z2 = 100;
	pos.pressure = 100;
	vp->gui.cursor.LBState = 1;
	touchSimulate(&pos, TOUCH_VINPUT|0, vp);

	/*pos.pen = 1;	// generate a finger up response
	pos.dt = 10;
	touchSimulate(&pos, TOUCH_VINPUT|3, vp);*/

	//renderSignalUpdate(vp);
}

static inline void mouseLBUp (TVLCPLAYER *vp, const int x, const int y)
{
	//printf("mouseLBUp %i,%i\n", x, y);
	
	if (vp->gui.cursor.LBState/* == 2*/){
		TTOUCHCOORD pos;
	  	pos.x = vp->gui.cursor.dx;
	  	pos.y = vp->gui.cursor.dy;
	  	pos.pen = 1;
	  	pos.time = getTime(vp);
	  	pos.dt = 5;
	  	pos.z1 = 100;
	  	pos.z2 = 100;
	  	pos.pressure = 100;
		touchSimulate(&pos, TOUCH_VINPUT|3, vp);
	}
	vp->gui.cursor.LBState = 0;
}

static inline void mouseMBDown (TVLCPLAYER *vp, const int x, const int y)
{
	if (vp->gui.cursor.isHooked){
		captureMouse(vp, 0);
		mHookUninstall();
	}
	vp->gui.cursor.MBState = 1;
	renderSignalUpdate(vp);
}
 
static inline void mouseMBUp (TVLCPLAYER *vp, const int x, const int y)
{
	vp->gui.cursor.MBState = 0;
}

static inline void mouseRBDown (TVLCPLAYER *vp, const int x, const int y)
{
	//printf("mouseRBDown %i %i\n", x, y);
	
	vp->gui.cursor.RBState = 1;
	
	const int top = pageInputGetTop(vp->pages);
	void *ptr = page2PageStructGet(vp->pages, top);
	
	//printf("mouseRBDown %i %p\n", top, ptr);
	
	if (ptr){
		if (!page2SetPrevious(ptr)){
			//printf("previous set == 0\n");
			
			if (top == PAGE_VKEYBOARD)
				page2RenderDisable(vp->pages, PAGE_VKEYBOARD);
		}else{
			//printf("previous set == 1\n");
		}
	}
	
	renderSignalUpdate(vp);
}

static inline void mouseRBUp (TVLCPLAYER *vp, const int x, const int y)
{
	vp->gui.cursor.RBState = 0;
}	

static inline void mouseWheelForward (TVLCPLAYER *vp, const int x, const int y)
{
	//printf("mouseWheelForward %ix%i\n", x, y);
	unsigned int mpos = ((x&0xFFFF)<<16) | (y&0xFFFF);
	page2Input(vp->pages, PAGE_IN_MOUSE, &mpos, PAGE_IN_WHEEL_FORWARD);
}

static inline void mouseWheelBack (TVLCPLAYER *vp, const int x, const int y)
{
	//printf("mouseWheelBack %ix%i\n", x,y);

	unsigned int mpos = ((x&0xFFFF)<<16) | (y&0xFFFF);
	page2Input(vp->pages, PAGE_IN_MOUSE, &mpos, PAGE_IN_WHEEL_BACK);
}

static inline void mouseWheelLeft (TVLCPLAYER *vp, const int x, const int y)
{
	//printf("mouseWheelLeft %ix%i\n", x,y);

	unsigned int mpos = ((x&0xFFFF)<<16) | (y&0xFFFF);
	page2Input(vp->pages, PAGE_IN_MOUSE, &mpos, PAGE_IN_WHEEL_LEFT);
}

static inline void mouseWheelRight (TVLCPLAYER *vp, const int x, const int y)
{
	//printf("mouseWheelRight %ix%i\n", x,y);

	unsigned int mpos = ((x&0xFFFF)<<16) | (y&0xFFFF);
	page2Input(vp->pages, PAGE_IN_MOUSE, &mpos, PAGE_IN_WHEEL_RIGHT);
}

static inline void mouseWheel (TVLCPLAYER *vp, const int msg, const int x, const int y)
{
	if (msg == WM_MWHEEL_FORWARD){
		if (renderLock(vp)){
			mouseWheelForward(vp, x, y);
			renderUnlock(vp);
		}
	}else if (msg == WM_MWHEEL_BACK){
		if (renderLock(vp)){
			mouseWheelBack(vp, x, y);
			renderUnlock(vp);
		}
	}else if (msg == WM_MWHEEL_LEFT){
		if (renderLock(vp)){
			mouseWheelLeft(vp, x, y);
			renderUnlock(vp);
		}		
	}else if (msg == WM_MWHEEL_RIGHT){
		if (renderLock(vp)){
			mouseWheelRight(vp, x, y);
			renderUnlock(vp);
		}
	}		
}

static inline void mouseButton (TVLCPLAYER *vp, const int msg, const int x, const int y)
{
	//printf("mouseButton %i, %i %i\n", msg, x, y);

	if (msg == WM_MOUSEMOVE)
		mouseMove(vp, -1, x, y, 0);
		
	else if (msg == WM_LBUTTONDOWN)
		mouseLBDown(vp, x, y);
		
	else if (msg == WM_LBUTTONUP)
		mouseLBUp(vp, x, y);
		
	else if (msg == WM_MBUTTONDOWN)
		mouseMBDown(vp, x, y);
		
	else if (msg == WM_MBUTTONUP)
		mouseMBUp(vp, x, y);
		
	else if (msg == WM_RBUTTONDOWN)
		mouseRBDown(vp, x, y);
		
	else if (msg == WM_RBUTTONUP)
		mouseRBUp(vp, x, y);
}

static inline int hookCB (TVLCPLAYER *vp, const int msg, const int var1, const int var2)
{
	//dbprintf(vp, "hook: %i %i %i\n", msg1, msg2, msg3);
	//printf("hook: %i %i %i\n", msg1, msg2, msg3);
	
	if (!vp || !vp->applState) return 0;

	switch (msg){
	  case WM_MOUSEMOVE:
	  	mouseMove(vp, -1, var1, var2, 0);
	  	break;
	  case WM_LBUTTONDOWN:
	  case WM_RBUTTONDOWN:
	  case WM_MBUTTONDOWN:
	  	//printf("\n\n");	  
	  case WM_LBUTTONUP:
	  case WM_MBUTTONUP:
	  case WM_RBUTTONUP:
	  	mouseButton(vp, msg, var1, var2);
	  	break;
	  case WM_MWHEEL_FORWARD:
	  case WM_MWHEEL_BACK:
	  case WM_MWHEEL_LEFT:
	  case WM_MWHEEL_RIGHT:
	  	mouseWheel(vp, msg, vp->gui.cursor.dx, vp->gui.cursor.dy);
	  	break;
	};

	return 1;
}
	
static inline int processWindowMessages (TVLCPLAYER *vp)
{
	MSG msg;
	memset(&msg, 0, sizeof(MSG));
	int ret = 0;
	
	if ((ret=GetMessage(&msg, NULL/*vp->gui.hMsgWin*/, 0, 0)) > 0){
		//HWND hwnd = GetActiveWindow();
		//if (!IsWindow(hwnd) || !IsDialogMessageA(hwnd, &msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		//}
	}
	
	return ret;
}

// used for capturing mouse enter/exit virtual window, otherwise not required
#if 1
void mouseRawInputInit (HANDLE hwnd)
{

#if 0
	UINT nDevices;
	if (GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0){ 
		return;
	}
	
	PRAWINPUTDEVICELIST pRawInputDeviceList = (RAWINPUTDEVICELIST*)my_malloc(sizeof(RAWINPUTDEVICELIST) * nDevices);
	GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST));
	// do the job...
	//printf("Number of raw input devices: %i\n\n", nDevices);


	char *types[] = {"Mouse", "Keyboard", "HID", "unknown"};

	for (int i = 0; i < nDevices; i++){
		int type = pRawInputDeviceList[i].dwType;
		HANDLE hdevice = pRawInputDeviceList[i].hDevice;
		
		printf("device %i, type:%i, %s \n", i, type, types[type]);
		if (type == RIM_TYPEMOUSE){
			RID_DEVICE_INFO devinfo;
			unsigned int dsize = sizeof(devinfo);
			GetRawInputDeviceInfoA(hdevice, RIDI_DEVICEINFO, &devinfo, &dsize);
			//printf("# ret %i %i\n", ret, dsize);
			printf("\t id:%i buttons:%i samRate:%i\n", (int)devinfo.mouse.dwId, (int)devinfo.mouse.dwNumberOfButtons, (int)devinfo.mouse.dwSampleRate);
			
		}else if (type == RIM_TYPEKEYBOARD){
			RID_DEVICE_INFO devinfo;
			unsigned int dsize = sizeof(devinfo);
			GetRawInputDeviceInfoA(hdevice, RIDI_DEVICEINFO, &devinfo, &dsize);

			printf("\t type:%i subType:%i mode:%i tFuncKeys:%i tIndicators:%i tKeys:%i\n",
			    (int)devinfo.keyboard.dwType,
    			(int)devinfo.keyboard.dwSubType,
    			(int)devinfo.keyboard.dwKeyboardMode,
    			(int)devinfo.keyboard.dwNumberOfFunctionKeys,
    			(int)devinfo.keyboard.dwNumberOfIndicators,
    			(int)devinfo.keyboard.dwNumberOfKeysTotal);

		}else if (type == RIM_TYPEHID){
			RID_DEVICE_INFO devinfo;
			unsigned int dsize = sizeof(devinfo);
			GetRawInputDeviceInfoA(hdevice, RIDI_DEVICEINFO, &devinfo, &dsize);

    		printf("\t vid:%X pid:%X ver:%i usagePage:%i usage:%i\n",
    			(int)devinfo.hid.dwVendorId,
    			(int)devinfo.hid.dwProductId,
    			(int)devinfo.hid.dwVersionNumber,
    			(int)devinfo.hid.usUsagePage,
    			(int)devinfo.hid.usUsage);
		}
		printf("\n");
	}

	// after the job, free the RAWINPUTDEVICELIST
	my_free(pRawInputDeviceList);

#endif

	RAWINPUTDEVICE Rid;
	Rid.usUsagePage = 1;
	Rid.usUsage = 2; 
	Rid.dwFlags = RIDEV_INPUTSINK|RIDEV_EXINPUTSINK/*| RIDEV_NOLEGACY*//* | RIDEV_CAPTUREMOUSE*/; ///*RIDEV_REMOVE |*/ RIDEV_NOHOTKEYS /*| RIDEV_EXINPUTSINK */| RIDEV_CAPTUREMOUSE;
	Rid.hwndTarget = hwnd;
	
	if (RegisterRawInputDevices(&Rid, 1/*nDevices*/, sizeof(RAWINPUTDEVICE)) == FALSE){
		//printf("RawInput init failed:\n");
	}
	return;
}

static inline void mouseRawInputProcess (TVLCPLAYER *vp, HRAWINPUT ri)
{
	//printf("@@ mouseRawInputProcess %X\n", (int)ri);
	
	UINT dwSize;
	GetRawInputData(ri, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
	RAWINPUT *raw = my_malloc(sizeof(RAWINPUT*) * dwSize);
	if (raw == NULL) return;
	
	//const int totalRecords = dwSize/sizeof(RAWINPUTHEADER);
	GetRawInputData(ri, RID_INPUT, raw, &dwSize, sizeof(RAWINPUTHEADER));

	//printf("rawInput type:%i\n", (int)raw->header.dwType);

	if (raw->header.dwType == RIM_TYPEMOUSE){
		if (raw->data.mouse.usButtonFlags&RI_MOUSE_LEFT_BUTTON_UP){
			if (mouseCapState == MOUSE_CAP_ON){
				mouseCapState = MOUSE_CAP_OFF;
				//printf("rawInput UP %i,%i\n", vp->gui.cursor.dx, vp->gui.cursor.dy);
				mouseButton(vp, WM_LBUTTONUP, vp->gui.cursor.dx, vp->gui.cursor.dy);
			}
		}
#if 0
		wprintf(L"\nrawMouse:usFlags=%04x\nulExtraInformation:%i\nulButtons=%04x \nusButtonFlags=%04x \nusButtonData=%04x \nulRawButtons=%04x \nlLastX=%ld \nlLastY=%ld\n",
			raw->data.mouse.usFlags, (int)raw->data.mouse.ulExtraInformation, 
			raw->data.mouse.ulButtons, raw->data.mouse.usButtonFlags, 
			raw->data.mouse.usButtonData, raw->data.mouse.ulRawButtons,
			raw->data.mouse.lLastX, raw->data.mouse.lLastY);
#endif
		
	}else if (raw->header.dwType == RIM_TYPEKEYBOARD){
		//printf("rawinput key %i %c %X %i\n", raw->data.keyboard.VKey, raw->data.keyboard.VKey, raw->data.keyboard.Message, raw->data.keyboard.Flags);
#if 0
		if (raw->data.keyboard.Flags == RI_KEY_MAKE){
			if (page2RenderGetState(vp->pages, PAGE_TETRIS)){	// is page visable
	  			tetrisInputProc(vp, pageGetPtr(vp, PAGE_TETRIS), raw->data.keyboard.VKey);
	  			renderSignalUpdate(vp);
	  			return;
	  		}
	  	}
#endif
	}

	//DefRawInputProc(&raw, totalRecords, sizeof(RAWINPUT));
	my_free(raw); 
}
#endif

static inline void onDeviceArrive (TVLCPLAYER *vp, const uintptr_t type, const int vid, const int pid)
{
	//printf("Device Arrive: vid %.4X, pid %.4X\n", vid, pid);
	
	pageDispatchMessage(vp->pages, PAGE_MSG_DEVICE_ARRIVE, vid, pid, (void*)type);
}

static inline void onDeviceDepart (TVLCPLAYER *vp, const uintptr_t type, const int vid, const int pid)
{
	//printf("Device Depart: vid %.4X, pid %.4X\n", vid, pid);
	
	pageDispatchMessage(vp->pages, PAGE_MSG_DEVICE_DEPART, vid, pid, (void*)type);
}

static inline int playlistMenuRemoveImage (TVLCSTRAY *tray, const int itemId)
{
	return (int)SetMenuItemBitmaps(tray->playlist.hmenu, itemId, MF_BITMAP, NULL, NULL);
}

static inline int playlistMenuSetImage (TVLCSTRAY *tray, const int itemId, const int bm_image)
{
	if (bm_image >= WINSYSTRAY_IMAGETOTAL) return 0;
	return (int)SetMenuItemBitmaps(tray->playlist.hmenu, itemId, MF_BITMAP, tray->hbm[bm_image], NULL);
}

static inline int playlistMenuAddString (TVLCSTRAY *tray, const int id, const int maxLen, char *str)
{
#if 0
   	MENUITEMINFO menuItem;
    menuItem.cbSize = sizeof(MENUITEMINFO);
    menuItem.fType = MFT_STRING | MFT_BITMAP;
    menuItem.fMask = MIIM_ID | MIIM_STRING | MIIM_BITMAP;
    menuItem.wID = id;
    menuItem.dwTypeData = str;
    menuItem.cch = strlen(str);
   	InsertMenuItemA(tray->playlist.hmenu, 0, FALSE, &menuItem);
#endif

   	wchar_t *strw = converttow(str);
   	if (strw){
		if (maxLen > 12){
			int slen = wcslen(strw);
			if (slen >= maxLen)
				strw[maxLen] = 0;
		}
		
   		AppendMenuW(tray->playlist.hmenu, MF_STRING, id, strw);
   		my_free(strw);
   	}
 	return 1;
}

static inline int playlistMenuAddSeparator (TVLCSTRAY *tray)
{
#if 0
   	MENUITEMINFO mii;
   	memset(&mii, 0, sizeof(mii));
   	
  	HMENU hMenuRecentFiles = CreatePopupMenu ();
  	AppendMenuW(hMenuRecentFiles, MF_STRING, 0, L"testing");
  	
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING | MIIM_DATA | MIIM_ID | MIIM_SUBMENU;
	mii.fType = MFT_STRING;
	mii.wID = 9999;
	mii.hSubMenu = hMenuRecentFiles;
	mii.dwTypeData = "menu";
	InsertMenuItemA(tray->playlist.hmenu, GetMenuItemCount(tray->playlist.hmenu), 1, &mii);
   	
#else
   	AppendMenuA(tray->playlist.hmenu, MFT_SEPARATOR, 0, NULL);
#endif
 	return 1;
}

static inline void playlistMenuDestroy (TVLCSTRAY *tray)
{
	if (tray->playlist.hmenu){
		DestroyMenu(tray->playlist.hmenu);
		tray->playlist.hmenu = NULL;
	}
}

// playlist menu
static inline int playlistMenuCreate (TVLCSTRAY *tray)
{
	playlistMenuDestroy(tray);
	tray->playlist.hmenu = CreatePopupMenu();
	
	return (tray->playlist.hmenu != NULL);
}
/*
static inline int contextMenuSetImageGp (TVLCSTRAY *tray, const int itemId, const wchar_t *bm_image)
{
	int ret = 0;
	HBITMAP bm = NULL;
	GpBitmap *gbm = NULL;

	if (!GdipCreateBitmapFromResource(GetModuleHandle(0), bm_image, &gbm)){
		if (!GdipCreateHBITMAPFromBitmap(gbm, &bm, 0xFFFFFFFF)){
			ret = SetMenuItemBitmaps(tray->context.hmenu, TRAY_MENU_PLAYINVLC, MF_BITMAP, bm, NULL);
			CloseHandle(bm);
		}
	}
	return ret && bm;
}*/

static inline int contextMenuSetImage (TVLCSTRAY *tray, const int itemId, const int bm_image)
{
	if (bm_image >= WINSYSTRAY_IMAGETOTAL) return 0;
	return (int)SetMenuItemBitmaps(tray->context.hmenu, itemId, MF_BITMAP, tray->hbm[bm_image], NULL);
}

void taskbarPostMessage (TVLCPLAYER *vp, const int msg, const int var1, const intptr_t var2)
{
	TVLCSTRAY *tray = &vp->gui.tray;
	PostMessage(tray->hwnd, msg, var1, var2);
}

int contextMenuIsTrackbarVisable (TVLCPLAYER *vp)
{
	TVLCSTRAY *tray = &vp->gui.tray;
	return tray->position.isVisable && tray->enabled;
}

int contextMenuIsVolumeVisable (TVLCPLAYER *vp)
{
	TVLCSTRAY *tray = &vp->gui.tray;
	return tray->volume.isVisable && tray->enabled;
}

int contextMenuIsChaptersVisable (TVLCPLAYER *vp)
{
	TVLCSTRAY *tray = &vp->gui.tray;
	return tray->chapters.isVisable && tray->enabled;
}

int contextMenuIsProgrammeVisable (TVLCPLAYER *vp)
{
	TVLCSTRAY *tray = &vp->gui.tray;
	return tray->programme.isVisable && tray->enabled;
}

static inline void contextMenuDestroy (TVLCSTRAY *tray)
{
	if (tray->context.hmenu){
		tray->position.isVisable = 0;
		tray->volume.isVisable = 0;
		tray->chapters.isVisable = 0;
		tray->programme.isVisable = 0;
		
		if (tray->position.hwnd)
			DestroyWindow(tray->position.hwnd);
		if (tray->volume.hwnd)
			DestroyWindow(tray->volume.hwnd);
		if (tray->context.hmenu)
			DestroyMenu(tray->context.hmenu);
		if (tray->chapters.hmenu)
			DestroyMenu(tray->chapters.hmenu);
		if (tray->programme.hmenu)
			DestroyMenu(tray->programme.hmenu);
	}
}

static inline void contextMenuHide (TVLCSTRAY *tray)
{
	tray->chapters.isVisable = 0;
	tray->position.isVisable = 0;
	tray->volume.isVisable = 0;
	tray->programme.isVisable = 0;
	
	ShowWindow(tray->position.hwnd, SW_HIDE);
	ShowWindow(tray->volume.hwnd, SW_HIDE);
}

static inline void contextMenuHandletTrackbarPosition (TVLCPLAYER *vp, const int pos)
{
	const int state = getPlayState(vp);
	if (state && state != 8){		// is playing but not at the end
		double position = (double)pos / (double)TRACK_POSITION_RANGE;
		clipFloat(position);
		TVLCCONFIG *vlc = getConfig(vp);
		vlc->position = position;
		vlc_setPosition(vlc, vlc->position);
	}
}

static inline void contextMenuHandletTrackbarVolume (TVLCPLAYER *vp, const int vol)
{
	int volume = TRACK_VOLUME_RANGE - vol;
	if (volume > 100) volume = 100;
	else if (volume < 0) volume = 0;
	setVolume(vp, volume, VOLUME_APP);
}

static inline void contextMenuSetTrackbarPosition (TVLCSTRAY *tray, const double position)
{
	PostMessage(tray->position.hwnd, TBM_SETPOS, 1, TRACK_POSITION_RANGE*position);
}

static inline void contextMenuEnableTrackbar (TVLCSTRAY *tray, const int calcPosition)
{
	if (calcPosition){
		if (!GetCursorPos(&tray->position.menuPos))
			return;
		//tray->position.menuPos.y -= 0;
	}
	if (SetWindowPos(tray->position.hwnd, NULL, tray->position.menuPos.x-100, tray->position.menuPos.y-32, TRACK_POSITION_LENGTH, 64, SWP_ASYNCWINDOWPOS|SWP_FRAMECHANGED|SWP_SHOWWINDOW))
		tray->position.isVisable = 1;
}

static inline void contextMenuSetVolumePosition (TVLCSTRAY *tray, const int position)
{
	float vol = (TRACK_VOLUME_RANGE - position) / 100.0;
	clipFloat(vol);
	PostMessage(tray->volume.hwnd, TBM_SETPOS, 1, (int)(TRACK_VOLUME_RANGE*vol));
}

static inline void contextMenuEnableVolume (TVLCSTRAY *tray, const int calcPosition)
{
	if (calcPosition){
		if (!GetCursorPos(&tray->volume.menuPos))
			return;
	}
	
	if (SetWindowPos(tray->volume.hwnd, NULL, tray->volume.menuPos.x-40, tray->volume.menuPos.y-32, 44, TRACK_VOLUME_HEIGHT, SWP_ASYNCWINDOWPOS|SWP_FRAMECHANGED|SWP_SHOWWINDOW))
		tray->volume.isVisable = 1;
}

static inline void contextMenuShow (TVLCSTRAY *tray, const int calcPosition)
{
	if (calcPosition){
		if (!GetCursorPos(&tray->context.menuPos))
			return;
		RECT rc;
		GetWindowRect(GetDesktopWindow(), &rc);
		if (tray->context.menuPos.y > rc.bottom-52)
			tray->context.menuPos.y = rc.bottom-52;
	}

	SetForegroundWindow(GetLastActivePopup(tray->hwnd));
	const int flags = TPM_CENTERALIGN|TPM_LEFTBUTTON|TPM_VERTICAL|TPM_NOANIMATION;
	TrackPopupMenu(tray->context.hmenu, flags, tray->context.menuPos.x, tray->context.menuPos.y, 0, tray->hwnd, NULL);
	PostMessage(tray->hwnd, WM_NULL, 0, 0);
}

static inline void contextMenuCheckItem (TVLCSTRAY *tray, const int id)
{
	ModifyMenuA(tray->context.hmenu, id, MF_CHECKED, id, contextMenuItemToString(id));
}

static inline void contextMenuUnCheckItem (TVLCSTRAY *tray, const int id)
{
	ModifyMenuA(tray->context.hmenu, id, MF_UNCHECKED, id, contextMenuItemToString(id));
}

static inline void contextMenuEnableItem (TVLCSTRAY *tray, const int id)
{
	ModifyMenuA(tray->context.hmenu, id, MF_ENABLED, id, contextMenuItemToString(id));
}

static inline void contextMenuDisableItem (TVLCSTRAY *tray, const int id)
{
	ModifyMenuA(tray->context.hmenu, id, MF_DISABLED|MF_GRAYED, id, contextMenuItemToString(id));
}

static inline int contextMenuAddString (TVLCSTRAY *tray, const int id)
{
	return AppendMenuA(tray->context.hmenu, MF_STRING, id, contextMenuItemToString(id));
}

static inline int contextMenuAddSeparator (TVLCSTRAY *tray)
{
	return AppendMenuA(tray->context.hmenu, MF_SEPARATOR, 0, NULL);
}

static inline int contextMenuCreate (TVLCSTRAY *tray, TVLCPLAYER *vp, const int addChapters, const int addProgramme, const int addSubtitles)
{
	tray->context.hmenu = CreatePopupMenu();

	contextMenuAddString(tray, TRAY_MENU_POSITION);
	contextMenuAddString(tray, TRAY_MENU_VOLUME);
	contextMenuAddString(tray, TRAY_MENU_MUTE);
	contextMenuAddString(tray, TRAY_MENU_PLAYINVLC);

	if (addChapters){
		contextMenuAddString(tray, TRAY_MENU_CHAPTERS);
		contextMenuSetImage(tray, TRAY_MENU_CHAPTERS, BM_CHAPTERS);
		tray->context.menuChaptersAdded = 1;
	}else{
		tray->context.menuChaptersAdded = 0;
	}

	if (addProgramme){
		contextMenuAddString(tray, TRAY_MENU_PROGRAMME);
		contextMenuSetImage(tray, TRAY_MENU_PROGRAMME, BM_PROGRAMME);
		tray->context.menuProgrammeAdded = 1;
	}else{
		tray->context.menuProgrammeAdded = 0;
	}

	if (addSubtitles){
		tray->subtitles.hmenu = CreatePopupMenu();

		TVLCCONFIG *vlc = getConfig(vp);
		TVLCSPU *spu = &vlc->spu;
		
		int total = spu->total;
		const int selected = spu->selected;
		
		libvlc_track_description_t *st = spu->desc;
		char buffer[512];
		
		for (int i = 0; i < total && st; i++, st=st->p_next){
			char *subName = st->psz_name;
			if (!subName){
				__mingw_snprintf(buffer, sizeof(buffer), "Subtitle %i", i+1);
				subName = buffer;
			}

			wchar_t *strw = converttow(subName);
   			if (strw){
   				int id = TRAY_MENU_SUBTITLES_BASE + st->i_id + 1;
				AppendMenuW(tray->subtitles.hmenu, MF_ENABLED|MF_STRING, id, strw);
				if (st->i_id == selected){
					CheckMenuItem(tray->subtitles.hmenu, id, MF_CHECKED);
					tray->subtitles.lastSelection = id;
				}
				my_free(strw);
			}
		}
		AppendMenuA(tray->context.hmenu, MF_POPUP|MF_ENABLED, (UINT_PTR)tray->subtitles.hmenu, contextMenuItemToString(TRAY_MENU_SUBTITLES));
		tray->context.menuSubtitlesAdded = 1;
	}else{
		tray->context.menuSubtitlesAdded = 0;
		tray->subtitles.lastSelection = 0;
	}
	
	contextMenuAddSeparator(tray);
	contextMenuAddString(tray, TRAY_MENU_PLAY);
	contextMenuAddString(tray, TRAY_MENU_STOP);
	contextMenuAddString(tray, TRAY_MENU_PREV);
	contextMenuAddString(tray, TRAY_MENU_NEXT);
	contextMenuAddSeparator(tray);
	contextMenuAddString(tray, TRAY_MENU_FLUSH);
	contextMenuAddString(tray, TRAY_MENU_SCREENSHOT);
	contextMenuAddString(tray, TRAY_MENU_SAVEPLAYLIST);
	contextMenuAddString(tray, TRAY_MENU_RTD);
	contextMenuAddSeparator(tray);
	contextMenuAddString(tray, TRAY_MENU_EXIT);

	contextMenuSetImage(tray, TRAY_MENU_POSITION, BM_POSITION);
	contextMenuSetImage(tray, TRAY_MENU_MUTE, BM_MUTE);
	contextMenuSetImage(tray, TRAY_MENU_VOLUME, BM_VOLUME);
	contextMenuSetImage(tray, TRAY_MENU_PLAYINVLC, BM_VLC);
	contextMenuSetImage(tray, TRAY_MENU_PLAY, BM_PLAY);
	contextMenuSetImage(tray, TRAY_MENU_STOP, BM_STOP);
	contextMenuSetImage(tray, TRAY_MENU_PREV, BM_PREV);
	contextMenuSetImage(tray, TRAY_MENU_NEXT, BM_NEXT);

	contextMenuDisableItem(tray, TRAY_MENU_POSITION);
	contextMenuDisableItem(tray, TRAY_MENU_VOLUME);
	contextMenuDisableItem(tray, TRAY_MENU_MUTE);
	contextMenuDisableItem(tray, TRAY_MENU_PLAYINVLC);
	contextMenuDisableItem(tray, TRAY_MENU_STOP);

	// track position control
	tray->position.hwnd = CreateWindow(TRACKBAR_CLASS, "Track position", TBS_HORZ|TBS_TOP|WS_DLGFRAME|WS_POPUP|WS_BORDER,
			0, 0, TRACK_POSITION_LENGTH, 60, tray->hwnd, NULL, vp->instanceModule, 0);
	ShowWindow(tray->position.hwnd, SW_HIDE);
			
	SendMessage(tray->position.hwnd, TBM_SETRANGE, FALSE, (LPARAM)MAKELONG(0,TRACK_POSITION_RANGE));
	SendMessage(tray->position.hwnd, TBM_SETLINESIZE, 0, TRACK_POSITION_STEPSIZE);
	SendMessage(tray->position.hwnd, TBM_SETPAGESIZE, 0, TRACK_POSITION_STEPSIZE);
	SendMessage(tray->position.hwnd, TBM_CLEARTICS, 1, 0);
	for (int i = 0; i < TRACK_POSITION_RANGE; i += TRACK_POSITION_STEPSIZE)
		SendMessage(tray->position.hwnd, TBM_SETTIC, 0, i);


	// volume control
	tray->volume.hwnd = CreateWindow(TRACKBAR_CLASS, "Volume", TBS_VERT|TBS_LEFT|WS_DLGFRAME|WS_POPUP|WS_BORDER,
			0, 0, 44, TRACK_VOLUME_HEIGHT, tray->hwnd, NULL, vp->instanceModule, 0);
	ShowWindow(tray->volume.hwnd, SW_HIDE);
	
	//SendMessage(tray->volume.hwnd, TBM_SETRANGE, 0, (LPARAM)MAKELONG(0,TRACK_VOLUME_RANGE));
	SendMessage(tray->volume.hwnd, TBM_SETRANGEMIN, 0, 0);
	SendMessage(tray->volume.hwnd, TBM_SETRANGEMAX, 0, TRACK_VOLUME_RANGE);
	SendMessage(tray->volume.hwnd, TBM_SETLINESIZE, 0, TRACK_VOLUME_STEPSIZE);
	SendMessage(tray->volume.hwnd, TBM_SETPAGESIZE, 0, TRACK_VOLUME_STEPSIZE);
	SendMessage(tray->volume.hwnd, TBM_CLEARTICS, 1, 0);
	for (int i = 0; i < TRACK_VOLUME_RANGE; i += TRACK_VOLUME_STEPSIZE)
		SendMessage(tray->volume.hwnd, TBM_SETTIC, 0, i);

	SetWindowLongPtr(tray->position.hwnd, GWLP_USERDATA, (LONG_PTR)vp);
	SetWindowLongPtr(tray->volume.hwnd, GWLP_USERDATA, (LONG_PTR)vp);

	return (tray->position.hwnd != NULL);
}


static inline int chaptersMenuRemoveImage (TVLCSTRAY *tray, const int itemId)
{
	return (int)SetMenuItemBitmaps(tray->chapters.hmenu, itemId, MF_BITMAP, NULL, NULL);
}

static inline int chaptersMenuSetImage (TVLCSTRAY *tray, const int itemId, const int bm_image)
{
	if (bm_image >= WINSYSTRAY_IMAGETOTAL) return 0;
	return (int)SetMenuItemBitmaps(tray->chapters.hmenu, itemId, MF_BITMAP, tray->hbm[bm_image], NULL);
}

static inline int chaptersMenuAddString (TVLCSTRAY *tray, const int id, const char *str)
{
	int ret = 0;
	wchar_t *strw = converttow(str);
   	if (strw){
		ret = AppendMenuW(tray->chapters.hmenu, MF_STRING, id, strw);
		my_free(strw);
	}
	return ret;
}

static inline void chaptersMenuDestroy (TVLCSTRAY *tray)
{
	if (tray->chapters.hmenu){
		DestroyMenu(tray->chapters.hmenu);
		tray->chapters.hmenu = NULL;
		tray->chapters.isVisable = 0;
	}
}

static inline int chaptersMenuCreate (TVLCSTRAY *tray, TVLCPLAYER *vp)
{
	if (tray->chapters.hmenu)
		chaptersMenuDestroy(tray);
	tray->chapters.hmenu = CreatePopupMenu();

	const int currentChapter = getPlayingChapter(vp);
	//printf("currentChapter %i\n", currentChapter);
	
	const int total = getTotalChapters(vp);
	
	for (int i = 0; i < total; i++){
		char *str = getChapterNameByIndex(vp, i);
		if (str){
			int id = TRAY_MENU_CHAPTERS_BASE+i;
			chaptersMenuAddString(tray, id, str);
			if (i == currentChapter){
				chaptersMenuSetImage(tray, id, BM_PLAYING);
				tray->chapters.lastSelection = id;
			}
			my_free(str);
		}
	}
	return tray->chapters.hmenu != NULL;	
}

static inline void chaptersMenuShow (TVLCSTRAY *tray, const int calcPosition)
{
	if (calcPosition){
		if (!GetCursorPos(&tray->chapters.menuPos))
			return;
		RECT rc;
		GetWindowRect(GetDesktopWindow(), &rc);
		if (tray->chapters.menuPos.y > rc.bottom-52)
			tray->chapters.menuPos.y = rc.bottom-52;
	}

	SetForegroundWindow(GetLastActivePopup(tray->hwnd));
	const int flags = TPM_CENTERALIGN|TPM_LEFTBUTTON|TPM_VERTICAL|TPM_NOANIMATION;
	tray->chapters.isVisable = 1;
	TrackPopupMenu(tray->chapters.hmenu, flags, tray->chapters.menuPos.x, tray->chapters.menuPos.y, 0, tray->hwnd, NULL);
	PostMessage(tray->hwnd, WM_NULL, 0, 0);
}

static inline void chaptersMenuHandleSelection (TVLCPLAYER *vp, TVLCSTRAY *tray, const int itemId, const int menuIsPersistant)
{
	int chapter = itemId-TRAY_MENU_CHAPTERS_BASE;
	
	//printf("chaptersMenuHandleSelection %i\n", chapter);
	setChapter(vp, chapter);
	
	chaptersMenuRemoveImage(tray, tray->chapters.lastSelection);
	chaptersMenuSetImage(tray, itemId, BM_PLAYING);
	tray->chapters.lastSelection = itemId;
	if (menuIsPersistant)
		chaptersMenuShow(tray, 0);
	
}

static inline int programmeMenuAddString (TVLCSTRAY *tray, const int id, const char *str)
{
	int ret = 0;
	wchar_t *strw = converttow(str);
   	if (strw){
		ret = AppendMenuW(tray->programme.hmenu, MF_STRING, id, strw);
		my_free(strw);
	}
	return ret;
}

static inline void programmeMenuDestroy (TVLCSTRAY *tray)
{
	if (tray->programme.hmenu){
		DestroyMenu(tray->programme.hmenu);
		tray->programme.hmenu = NULL;
		tray->programme.isVisable = 0;
		
		if (tray->programme.pidLookup){
			my_free(tray->programme.pidLookup);
			tray->programme.pidLookup = NULL;
		}
	}
}

static inline int programmeMenuCreate (TVLCSTRAY *tray, TVLCPLAYER *vp)
{

	int showPID = 0;
	settingsGet(vp, "systray.dvb.showPID", &showPID);

	if (tray->programme.hmenu)
		programmeMenuDestroy(tray);
	tray->programme.hmenu = CreatePopupMenu();

	int total = 0;
	TVLCEPG **epg = epg_dupEpg(getConfig(vp), &total);
	if (!total || !epg) return 0;
	char buffer[512];
	
	if (tray->programme.pidLookup)
		my_free(tray->programme.pidLookup);
	tray->programme.pidLookup = my_calloc(total, sizeof(int));
	tray->programme.pidTotal = total;
	
	for (int i = 0; i < total; i++){
		char *name = epg[i]->psz_name;
		
		int programme = 0;
		if (name){
			char *bracket = strrchr(name, ' ');
			if (bracket){
				programme = (int)strtol(bracket, NULL, 10);
				if (programme < 0) programme = 0;

				char *bracket = strrchr(name, '[');
				if (bracket) *(bracket-1) = 0;
			}
		}

		if (programme){
			tray->programme.pidLookup[i] = programme;
			if (showPID){
				__mingw_snprintf(buffer, sizeof(buffer), "%s  -  %i", name, programme);
				programmeMenuAddString(tray, TRAY_MENU_PROGRAMME_BASE+i, buffer);
			}else{
				programmeMenuAddString(tray, TRAY_MENU_PROGRAMME_BASE+i, name);
			}
		}
		
	}
	
	if (total)
		epg_freeEpg(epg, total);

	return tray->programme.hmenu != NULL;	
}

static inline void programmeMenuShow (TVLCSTRAY *tray, const int calcPosition)
{
	if (calcPosition){
		if (!GetCursorPos(&tray->programme.menuPos))
			return;
		RECT rc;
		GetWindowRect(GetDesktopWindow(), &rc);
		if (tray->programme.menuPos.y > rc.bottom-52)
			tray->programme.menuPos.y = rc.bottom-52;
	}

	SetForegroundWindow(GetLastActivePopup(tray->hwnd));

	const int flags = TPM_CENTERALIGN|TPM_LEFTBUTTON|TPM_VERTICAL|TPM_NOANIMATION;
	tray->programme.isVisable = 1;
	TrackPopupMenu(tray->programme.hmenu, flags, tray->programme.menuPos.x, tray->programme.menuPos.y, 0, tray->hwnd, NULL);
	PostMessage(tray->hwnd, WM_NULL, 0, 0);
}

static inline int programmeMenuHandleSelection (TVLCPLAYER *vp, TVLCSTRAY *tray, const int itemId, const int menuIsPersistant)
{
	//printf("prgorammeMenuHandleSelection %i\n", itemId);

	int id = itemId - TRAY_MENU_PROGRAMME_BASE;
	if (id >= 0 && id < tray->programme.pidTotal){
		int pid = tray->programme.pidLookup[id];
		//printf("channel %i\n", pid);

		vlc_setProgram(getConfig(vp), pid);
		if (menuIsPersistant)
			programmeMenuShow(tray, 0);
		return pid;
	}
	return 0;
}

static inline void subtitlesMenuHandleSelection (TVLCPLAYER *vp, TVLCSTRAY *tray, const int itemId, const int menuIsPersistant)
{
	const int subtitle = (itemId - TRAY_MENU_SUBTITLES_BASE) - 1;
	
	//printf("subtitleMenuHandleSelection %i\n", subtitle);

	TVLCCONFIG *vlc = getConfig(vp);
	vlc_setSubtitle(vlc, subtitle);
	vlc->spu.selected = vlc_getSubtitle(vlc);
	
	CheckMenuItem(tray->subtitles.hmenu, tray->subtitles.lastSelection, MF_UNCHECKED);
	CheckMenuItem(tray->subtitles.hmenu, itemId, MF_CHECKED);
	tray->subtitles.lastSelection = itemId;
	
	if (menuIsPersistant)
		contextMenuShow(tray, 0);
}

int toggleRTD (TVLCPLAYER *vp, TVLCSTRAY *tray)
{
	if (vp->ctx.winRender.enable){
		vp->ctx.winRender.enable = 0;
		RECT rc = {0, 0, .right=GetSystemMetrics(SM_CXSCREEN), .bottom=GetSystemMetrics(SM_CYSCREEN)};
		RedrawWindow(vp->ctx.winRender.wnd, &rc, 0, RDW_INVALIDATE|RDW_UPDATENOW|RDW_INTERNALPAINT|RDW_ALLCHILDREN);

		if (vp->ml->enableVirtualDisplay)
			lSetDisplayOption(vp->ml->hw, vp->ml->virtualDisplayId, lOPT_DDRAW_SHOW, NULL);
	}else{
		TFRAME *front = getFrontBuffer(vp);
		BITMAPINFO *bitHdr = (BITMAPINFO*)vp->ctx.winRender.bitBuff;
		bitHdr->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitHdr->bmiHeader.biPlanes = 1;
		bitHdr->bmiHeader.biBitCount = 32;
		bitHdr->bmiHeader.biCompression = BI_RGB;
		bitHdr->bmiHeader.biWidth = front->width;
		bitHdr->bmiHeader.biHeight = -front->height;
		vp->ctx.winRender.bitHdr = bitHdr;
		//vp->ctx.winRender.copyMode = 1;
		vp->ctx.winRender.wnd = getDesktopHWND();
		vp->ctx.winRender.enable = 1;
		
		/*RECT rc = {0};
		GetWindowRect(vp->ctx.winRender.wnd, &rc);
		vp->ctx.winRender.cxScreen = (rc.right - rc.left);
		vp->ctx.winRender.cyScreen = (rc.bottom - rc.top);
		imageBestFit(vp->ctx.winRender.cxScreen, vp->ctx.winRender.cyScreen,
			front->width, front->height, &vp->ctx.winRender.wStretched, &vp->ctx.winRender.hStretched);
		*/
		if (vp->ml->enableVirtualDisplay && vp->ctx.winRender.wnd)
			lSetDisplayOption(vp->ml->hw, vp->ml->virtualDisplayId, lOPT_DDRAW_HIDE, NULL);
	}
	
	if (vp->ctx.winRender.enable)
		contextMenuCheckItem(tray, TRAY_MENU_RTD);
	else
		contextMenuUnCheckItem(tray, TRAY_MENU_RTD);
	
	return vp->ctx.winRender.enable;
}

void contextMenuHandleSelection (TVLCPLAYER *vp, TVLCSTRAY *tray, const int itemId, const int menuIsPersistant)
{
		  		
	if (itemId == TRAY_MENU_POSITION){
		wakeup(vp);
		TVLCCONFIG *vlc = getConfig(vp);
		if (getPlayState(vp) && getPlayState(vp) != 8){
			contextMenuSetTrackbarPosition(tray, vlc->position);
			contextMenuEnableTrackbar(tray, 1);
		}
	}else if (itemId == TRAY_MENU_VOLUME){
		wakeup(vp);
		if (getPlayState(vp) && getPlayState(vp) != 8){
			contextMenuSetVolumePosition(tray, getVolume(vp,VOLUME_APP));
			contextMenuEnableVolume(tray, 1);
		}		
	}else if (itemId == TRAY_MENU_MUTE){
		if (getPlayState(vp) && getPlayState(vp) != 8)
			toggleMute(vp);
	
	}else if (itemId == TRAY_MENU_PLAYINVLC){
		if (getPlayState(vp) == 1 || getPlayState(vp) == 2){
			if (renderLock(vp)){
				startVlcTrackPlayback(vp);
				renderUnlock(vp);
			}
		}
	}else if (itemId == TRAY_MENU_CHAPTERS){
		chaptersMenuCreate(tray, vp);
		chaptersMenuShow(tray, 1);
		
	}else if (itemId == TRAY_MENU_PROGRAMME){
		//printf("TRAY_MENU_PROGRAMME\n");
		programmeMenuCreate(tray, vp);
		programmeMenuShow(tray, 1);

	//}else if (itemId == TRAY_MENU_SUBTITLES){
	//	printf("TRAY_MENU_SUBTITLES\n");
		
	}else if (itemId == TRAY_MENU_EXIT){
		if (hasPageBeenAccessed(vp, PAGE_SEARCH))
			searchForceStop(vp);
		wakeup(vp);
		timerSet(vp, TIMER_SHUTDOWN, 0);
		PostMessage(tray->hwnd, WM_QUIT, 0, 0);
		
	}else if (itemId == TRAY_MENU_PLAY){
		wakeup(vp);
		if (getPlayState(vp) == 1 || getPlayState(vp) == 2)
  			timerSet(vp, TIMER_PLAYPAUSE, 0);
  		else
  			timerSet(vp, TIMER_PLAY, 0);
	}else if (itemId == TRAY_MENU_STOP){	
		wakeup(vp);
		timerSet(vp, TIMER_STOP, 0);
		
	}else if (itemId == TRAY_MENU_PREV){	
		wakeup(vp);
		timerSet(vp, TIMER_PREVTRACK, 0);
		
	}else if (itemId == TRAY_MENU_NEXT){	
		wakeup(vp);
		timerSet(vp, TIMER_NEXTTRACK, 0);
	
	}else if (itemId == TRAY_MENU_SCREENSHOT){
		vp->gui.snapshot.annouce = 1;
		vp->gui.snapshot.save = 2;
		renderSignalUpdateNow(vp);
		
	}else if (itemId == TRAY_MENU_SAVEPLAYLIST){
#if 1
		savePlaylistAsync(vp);
#else
		int recWritten = playerWriteDefaultPlaylist(vp, VLCSPLAYLIST);
		if (recWritten)
			dbwprintf(vp, L"%i records written to %s", recWritten, VLCSPLAYLIST);
		else
			dbwprintf(vp, L"Playlist write failed [%s]", VLCSPLAYLIST);
#endif
	}else if (itemId == TRAY_MENU_FLUSH){
		timerSet(vp, TIMER_FLUSH, 0);
		
	}else if (itemId == TRAY_MENU_RTD){
		toggleRTD(vp, tray);
		renderSignalUpdateNow(vp);
	}
}

static inline void taskbarTrayClose (TVLCSTRAY *tray)
{
	if (!tray->enabled) return;

	playlistMenuDestroy(tray);
	contextMenuDestroy(tray);
	chaptersMenuDestroy(tray);
	
	for (int i = 0; i < WINSYSTRAY_IMAGETOTAL; i++){
		if (tray->hbm[i])
			DeleteObject(tray->hbm[i]);
	}
	
	NOTIFYICONDATA nData;
	memset(&nData, 0, sizeof(NOTIFYICONDATA));
	nData.cbSize = sizeof(nData);
	nData.hWnd = tray->hwnd;
	nData.uID = APP_ICON;

	Shell_NotifyIconA(NIM_DELETE, &nData);
}

static inline int taskbarTraySetTip (TVLCSTRAY *tray, char *str)
{
	//printf("taskbarTraySetTip '%s'\n", str);

	if (!tray->tipsEnabled) return 0;

	NOTIFYICONDATAW nData;
	memset(&nData, 0, sizeof(NOTIFYICONDATAW));
	
	wchar_t *strw = converttow(str);
	if (!strw) return 0;

	wcsncpy(nData.szTip, strw, (sizeof(nData.szTip)/sizeof(wchar_t))-1);
	my_free(strw);
		
	nData.cbSize = sizeof(NOTIFYICONDATAW);
	nData.hWnd = tray->hwnd;
	nData.uID = APP_ICON;
	nData.uCallbackMessage = WM_SYSTRAY;
	nData.uFlags = NIF_MESSAGE | NIF_TIP;
			
	return Shell_NotifyIconW(NIM_MODIFY, &nData);
}

static inline int taskbarTraySetInfo (TVLCSTRAY *tray, char *str)
{
	//printf("taskbarTraySetInfo '%s'\n", str);

	if (!tray->infoEnabled) return 0;

	NOTIFYICONDATAW nData;
	memset(&nData, 0, sizeof(NOTIFYICONDATAW));
	
	wchar_t *strw = converttow(str);
	if (!strw) return 0;

	wcsncpy(nData.szInfo, strw, (sizeof(nData.szInfo)/sizeof(wchar_t))-1);
	my_free(strw);
	
	nData.cbSize = sizeof(NOTIFYICONDATAW);
	nData.hWnd = tray->hwnd;
	nData.uID = APP_ICON;
	nData.uCallbackMessage = WM_SYSTRAY;
	nData.uFlags = NIF_MESSAGE | NIF_INFO;
			
	return Shell_NotifyIconW(NIM_MODIFY, &nData);
}

#if 0
static inline int taskbarTraySetInfoTitle (TVLCSTRAY *tray, char *title, char *info)
{
	//printf("taskbarTraySetInfoTitle '%s' '%s'\n", info, title);
	if (!tray->infoEnabled) return 0;
	
	NOTIFYICONDATAW nData;
	memset(&nData, 0, sizeof(NOTIFYICONDATAW));
	
	wchar_t *infow = converttow(info);
	wchar_t *titlew = converttow(title);
	wcsncpy(nData.szInfo, infow, (sizeof(nData.szInfo)/sizeof(wchar_t))-1);
	wcsncpy(nData.szInfoTitle, titlew, (sizeof(nData.szInfoTitle)/sizeof(wchar_t))-1);
	my_free(infow);
	my_free(titlew);
	
	nData.dwInfoFlags = NIIF_USER | NIIF_NOSOUND;
	
	nData.cbSize = sizeof(NOTIFYICONDATAW);
	nData.hWnd = tray->hwnd;
	nData.uID = APP_ICON;
	nData.uCallbackMessage = WM_SYSTRAY;
	nData.uFlags = NIF_MESSAGE | NIF_INFO;
			
	return Shell_NotifyIconW(NIM_MODIFY, &nData);
}
#endif

static inline int taskbarTrayInit (TVLCPLAYER *vp, TVLCSTRAY *tray, HWND hwnd)
{

	NOTIFYICONDATA nData;
	memset(&nData, 0, sizeof(NOTIFYICONDATA));

	nData.hWnd = tray->hwnd = hwnd;
	nData.cbSize = sizeof(NOTIFYICONDATA);
	nData.uID = APP_ICON;

	if (!vp->instanceCheck)
		nData.hIcon = LoadIconA(vp->instanceModule, "APP_PRIMARY");
	else
		nData.hIcon = LoadIconA(vp->instanceModule, "APP_SECONDARY");
	nData.uCallbackMessage = WM_SYSTRAY;
	strcpy(nData.szTip, "VLCStream");
	nData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

	if (!Shell_NotifyIconA(NIM_ADD, &nData))
		return 0;

	wchar_t *idir = L"common/icons";
	wchar_t buffer[MAX_PATH+1];

	// load resource images (currently 13x13 bitmaps)
	contextMenuLoadImage(tray, 1, BM_FOLDER,	L"BM_FOLDER");
	contextMenuLoadImage(tray, 1, BM_PLAYING,	L"BM_PLAYING");
	contextMenuLoadImage(tray, 1, BM_FOLDERPLY,	L"BM_FOLDERPLY");
	contextMenuLoadImage(tray, 1, BM_POSITION,	L"BM_POSITION");
	contextMenuLoadImage(tray, 1, BM_CHAPTERS,	L"BM_CHAPTERS");
	contextMenuLoadImage(tray, 1, BM_PLAY,		L"BM_PLAY");
	contextMenuLoadImage(tray, 1, BM_PAUSE,		L"BM_PAUSE");
	contextMenuLoadImage(tray, 1, BM_STOP,		L"BM_STOP");
	contextMenuLoadImage(tray, 1, BM_PREV,		L"BM_PREV");
	contextMenuLoadImage(tray, 1, BM_NEXT,		L"BM_NEXT");
	contextMenuLoadImage(tray, 3, BM_VLC,		buildSkinDEx(vp,buffer,idir,L"vlc13.png"));
	contextMenuLoadImage(tray, 1, BM_MUTE,		L"BM_MUTE");
	contextMenuLoadImage(tray, 1, BM_MUTED,		L"BM_MUTED");
	contextMenuLoadImage(tray, 1, BM_VOLUME,	L"BM_VOLUME");
	contextMenuLoadImage(tray, 1, BM_PROGRAMME,	L"BM_PROGRAMME");
	
	playlistMenuCreate(tray);
	contextMenuCreate(tray, vp, 0, 0, 0);

	return 1;
}

static inline void initTray (TVLCPLAYER *vp, HWND hwnd)
{
    TVLCSTASKBAR *tb = &vp->gui.taskbar;
    settingsGet(vp, "taskbar.enabled", &tb->enabled);
	if (tb->enabled)
		taskbarToolbarInit(vp, tb);

    TVLCSTRAY *tray = &vp->gui.tray;
    settingsGet(vp, "systray.enabled", &tray->enabled);
    if (tray->enabled){
    	settingsGet(vp, "systray.tips.enabled", &tray->tipsEnabled);
    	settingsGet(vp, "systray.info.enabled", &tray->infoEnabled);
    	taskbarTrayInit(vp, tray, hwnd);
    	SetWindowLongPtr(tray->position.hwnd, GWLP_USERDATA, (LONG_PTR)vp);
    	SetWindowLongPtr(tray->volume.hwnd, GWLP_USERDATA, (LONG_PTR)vp);
    }
}

static inline void playlistMenuShow (TVLCSTRAY *tray, int calcPosition)
{
	if (calcPosition){
		if (!GetCursorPos(&tray->playlist.menuPos))
			return;
		//tray->playlist.menuPos.y -= 32;
		RECT rc;
		GetWindowRect(GetDesktopWindow(), &rc);
		if (tray->playlist.menuPos.y > rc.bottom-52)
			tray->playlist.menuPos.y = rc.bottom-52;
	}

	SetForegroundWindow(GetLastActivePopup(tray->hwnd));

	const int flags = TPM_CENTERALIGN|TPM_LEFTBUTTON|TPM_VERTICAL|TPM_NOANIMATION;
	TrackPopupMenu(tray->playlist.hmenu, flags, tray->playlist.menuPos.x, tray->playlist.menuPos.y, 0, tray->hwnd, NULL);
	PostMessage(tray->hwnd, WM_NULL, 0, 0);
}

static inline void playlistMenuClean (TVLCSTRAY *tray)
{
	const int total = GetMenuItemCount(tray->playlist.hmenu)-1;
	for (int i = total; i >= 0; i--)
		RemoveMenu(tray->playlist.hmenu, i, MF_BYPOSITION);
}

static inline void playlistMenuSetInfoTrack (TVLCPLAYER *vp, TVLCSTRAY *tray, const int uid, const int track)
{
	if (!tray->infoEnabled) return;
	
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	if (!plc) return;
	char *str = playlistGetTitleDup(plc, track);
	if (str){
		taskbarTraySetInfo(tray, str);
		my_free(str);
	}
}

void playlistMenuSetTipTrack (TVLCPLAYER *vp, TVLCSTRAY *tray, char *title, const int uid, const int track)
{
	//printf("playlistMenuSetTipTrack %i '%s' %X %i\n", tray->tipsEnabled, title, uid, track);
	if (!tray->tipsEnabled) return;
	
	if (title){
		taskbarTraySetTip(tray, title);
	}else if (track == -1 && uid > 0){
		char *playlistName = playlistManagerGetName(vp->plm, uid);
		if (playlistName){
			taskbarTraySetTip(tray, playlistName);
			my_free(playlistName);
		}
	}else if (uid > 0){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
		if (plc){
			unsigned int hash = playlistGetHash(plc, track);
			if (!hash) return;
			char buffer[MAX_PATH_UTF8+1];
						
			char *name = playlistGetNameDup(plc);
			char *title = tagRetrieveDup(vp->tagc, hash, MTAG_NowPlaying);
			if (!title) title = playlistGetTitleDup(plc, track);
			char *artist = tagRetrieveDup(vp->tagc, hash, MTAG_Artist);
			char *length = tagRetrieveDup(vp->tagc, hash, MTAG_LENGTH);
			if (!length) length = my_strdup("0:00");

			if (name && title && length && artist)
				__mingw_snprintf(buffer, MAX_PATH_UTF8, "  %s  \n  %s  \n  %s  \n  %s  ", name, title, length, artist);
			else if (name && title && length)
				__mingw_snprintf(buffer, MAX_PATH_UTF8, "  %s  \n  %s  \n  %s  ", name, title, length);
			else if (name && title)
				__mingw_snprintf(buffer, MAX_PATH_UTF8, "  %s  \n  %s  ", name, title);
			else if (name)
				__mingw_snprintf(buffer, MAX_PATH_UTF8, "  %s  ", name);
			else if (title)
				__mingw_snprintf(buffer, MAX_PATH_UTF8, "  %s  ", title);
			else
				return;
				
			taskbarTraySetTip(tray, buffer);
			if (name) my_free(name);
			if (title) my_free(title);
			if (length) my_free(length);
			if (artist) my_free(artist);
		}
	}
}

static inline void playlistMenuSetPlaylist (TVLCSTRAY *tray, TVLCPLAYER *vp, const int uid)
{
	//printf("playlistMenuSetPlaylist %i\n", uid);

	int showUID = 0;
	int showTrackNo = 0;
	int showLength = 0;
	int maxStringLength = 0;
	settingsGet(vp, "systray.playlist.track.showNo", &showTrackNo);
	settingsGet(vp, "systray.playlist.track.showLength", &showLength);
	settingsGet(vp, "systray.playlist.showUID", &showUID);
	settingsGet(vp, "systray.playlist.maxStringLength", &maxStringLength);
	
	
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	if (!plc) return;
	
	char titleTmp[MAX_PATH_UTF8+1];
	if (uid != getRootPlaylistUID(vp)){
		char *playlistName = playlistGetNameDup(plc);
		if (!showUID){
			playlistMenuAddString(tray, (uid<<16)|TRAY_PLY_TITLE, maxStringLength, playlistName);
		}else{
			__mingw_snprintf(titleTmp, MAX_PATH_UTF8, "%s - %X", playlistName, uid);
			playlistMenuAddString(tray, (uid<<16)|TRAY_PLY_TITLE, maxStringLength, titleTmp);
		}
		playlistMenuAddSeparator(tray);
		playlistMenuAddString(tray, (uid<<16)|TRAY_PLY_BACK, 0, " ..");
		//playlistMenuSetTipTrack(vp, tray, playlistName, uid, -1);
		my_free(playlistName);
	}

	char buffer[MAX_PATH_UTF8+1];
	char length[MAX_PATH_UTF8+1];
	buffer[0] = ' ';
	buffer[1] = 0;
	int total = playlistGetTotal(plc);
	
	int playingTrk;
	if (getQueuedPlaylistUID(vp) != uid)
		playingTrk = -666;
	else
		playingTrk = getPlayingItem(vp);
	
	PLAYLISTCACHE *const plcQ = getQueuedPlaylist(vp);
	for (int i = 0; i < total; i++){
		if (!playlistGetTitle(plc, i, &buffer[1], MAX_PATH_UTF8-1))
			break;
	
		const int id = ((uid&0xFFFF)<<16)|(TRAY_PLY_BASEID+1+i);

		if (playlistGetItemType(plc, i) == PLAYLIST_OBJTYPE_PLC){
			playlistMenuAddString(tray, id, maxStringLength, buffer);
			
			if (playlistIsChild(plcQ, playlistGetPlaylist(plc,i)))
				playlistMenuSetImage(tray, id, BM_FOLDERPLY);
			else
				playlistMenuSetImage(tray, id, BM_FOLDER);
		}else{
			char *title = buffer;
			if (showTrackNo){
				title = titleTmp;
				__mingw_snprintf(title, MAX_PATH_UTF8, " %i:%s", i+1, buffer);
			}
			
			if (showLength){
				char *trkLen = tagRetrieveDup(vp->tagc, playlistGetHash(plc,i), MTAG_LENGTH);
				if (!trkLen) trkLen = my_strdup("0:00");
				__mingw_snprintf(length, MAX_PATH_UTF8, " %s  (%s)", title, trkLen);
				my_free(trkLen);
				playlistMenuAddString(tray, id, maxStringLength, length);
			}else{
				playlistMenuAddString(tray, id, maxStringLength, title);
			}

			if (i == playingTrk){
				playlistMenuSetImage(tray, id, BM_PLAYING);
				tray->playlist.lastSelection = id;
			}
		}
	}
}

static inline void playlistMenuSelect_back (TVLCPLAYER *vp, TVLCSTRAY *tray, int uid)
{
	//printf("playlistMenuSelect_back %X\n", uid);
	
	uid = playlistManagerGetPlaylistParentByUID(vp->plm, uid);
	if (!uid) uid = PLAYLIST_UID_BASE+1;
	
	setDisplayPlaylistByUID(vp, uid);
	playlistMenuClean(tray);
	playlistMenuSetPlaylist(tray, vp, uid);
	playlistMenuShow(tray, 0);
}

static inline void playlistMenuSelect_playlist (TVLCPLAYER *vp, TVLCSTRAY *tray, const int uid)
{
	//printf("playlistMenuSelect_playlist %X\n", uid);
	
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	if (!plc) return;

	playlistMetaGetMeta(vp, plc, 0, playlistGetTotal(plc)-1, NULL);
	
	setDisplayPlaylistByUID(vp, uid);
	playlistMenuClean(tray);
	playlistMenuSetPlaylist(tray, vp, uid);
	playlistMenuShow(tray, 0);
}

static inline void playlistMenuSelect_track (TVLCPLAYER *vp, TVLCSTRAY *tray, const int uid, const int track)
{
	//printf("playlistMenuSelect_track %X %i\n", uid, track);
	
	TTIMERPLAYTRACK *tpt = &vp->gui.playtrack;
	tpt->uid = uid;
	tpt->track = track;

	wakeup(vp);
	timerSet(vp, TIMER_PLAYTRACK, 0);
	playlistMenuSetInfoTrack(vp, tray, uid, track);
}

static inline void playlistMenuItemSelected (TVLCPLAYER *vp, TVLCSTRAY *tray, const int item, const int menuIsPersistant)
{

	int uid = (item>>16)&0xFFFF;	
	int pos = (item&0xFFFF);

	//printf("menu item selected %i, %i %i\n", item, uid, pos);
		
	if (pos == TRAY_PLY_TITLE){
		playlistMenuClean(tray);
		playlistMenuSetPlaylist(tray, vp, uid);
		playlistMenuShow(tray, 0);
		
	}else if (pos == TRAY_PLY_BACK){
		playlistMenuSelect_back(vp, tray, uid);

	}else{
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
		if (!plc) return;

		pos -= TRAY_PLY_BASEID + 1;
		int itemType = playlistGetItemType(plc, pos);
		
		if (itemType == PLAYLIST_OBJTYPE_PLC){
			uid = playlistGetPlaylistUID(plc, pos);
			if (uid > PLAYLIST_UID_BASE)
				playlistMenuSelect_playlist(vp, tray, uid);

		}else if (itemType == PLAYLIST_OBJTYPE_TRACK){
			playlistMenuSelect_track(vp, tray, uid, pos);
			
			if (menuIsPersistant&0x01){
				playlistMenuRemoveImage(tray, tray->playlist.lastSelection);
				playlistMenuSetImage(tray, item, BM_PLAYING);
				tray->playlist.lastSelection = item;
				playlistMenuShow(tray, 0);
			}
		}
	}
}

int getModifierKeyState (TVLCPLAYER *vp, const int kp_key)
{
	return vp->gui.modifierKeyStates&kp_key;
}

static inline int getModifierKeyStates (TVLCPLAYER *vp)
{
	return vp->gui.modifierKeyStates;
}

static inline int setModifierKeyStates (TVLCPLAYER *vp, const int winModifier)
{
	int keyState = 0;
	
	if (winModifier&0x40000000) keyState = KP_VK_REPEAT;
	if (GetAsyncKeyState(VK_CONTROL)&0x8000) keyState |= KP_VK_CONTROL;
	if (GetAsyncKeyState(VK_MENU)&0x8000) keyState |= KP_VK_ALT;
	if (GetAsyncKeyState(VK_SHIFT)&0x8000) keyState |= KP_VK_SHIFT;
	if (GetAsyncKeyState(VK_LWIN)&0x8000 || GetAsyncKeyState(VK_RWIN)&0x8000) keyState |= KP_VK_WIN;
	if (GetAsyncKeyState(VK_APPS)&0x8000) keyState |= KP_VK_APPS;

	vp->gui.modifierKeyStates = keyState;
	return keyState;
}

static inline void keyboardHandleModifierPress (TVLCPLAYER *vp, const int key, const int modifier)
{
	//printf("keyboardPress key:%i alt:%i ctrl:%i shf:%i win:%i apps:%i\n", key, modifier&KP_VK_ALT, modifier&KP_VK_CONTROL, modifier&KP_VK_SHIFT, modifier&KP_VK_WIN, modifier&KP_VK_APPS);
	if (modifier&KP_VK_REPEAT || key == 0x11)	// 0x11 is the CtrlKey key code
		return;

	//printf("keyboardHandleModifierPress key:%i(%X) modifer:0x%.2X\n", key, key, modifier);

	if (modifier&KP_VK_ALT){
		
	}else if (modifier&KP_VK_SHIFT){
		if (key == 'S'){				// enable/disable fps overlay
			setShowStats(vp, getShowStatsState(vp)^1);
		
		//}else if (key == 'F'){			// free memory by flushing image caches
		//	timerSet(vp, TIMER_FLUSH, 100);
			
		}else if (key == 'T'){			// display time
				pageSet(vp, PAGE_CLOCK);
		}else{
			return;
		}
		renderSignalUpdate(vp);

	}else if (modifier&KP_VK_CONTROL){
		if (pageSendMessage(vp->pages, pageInputGetTop(vp->pages), PAGE_MSG_KEY_DOWN, key, modifier, NULL)){
			//printf("mod key unhandled %i %X\n", key, modifier);

			if (key == 'F'){			// go to search page
				if (!pageIsDisplayed(vp, PAGE_SEARCH) && !pageIsDisplayed(vp, PAGE_VKEYBOARD))
					pageSet(vp, PAGE_SEARCH);
					
			}else if (key == 'B'){		// change background
				reloadSkin(vp, 1);
			
			}else if (key == 'P'){		// go to playlist page
				pageSet(vp, PAGE_PLY_PANE);
				
			}else if (key == 'S'){		// save playlist
				savePlaylistAsync(vp);
			
			}else if (key == 'E'){		// go to EQ page
				pageSet(vp, PAGE_EQ);
				
			}else if (key == 'O'){		// open browser
				pageSet(vp, PAGE_EXP_PANEL);

			}else{
				return;
			}
			/* TODO:
				add a 'register key and callback' interface
				change aspect
				change subs
				change audio
				change chapter
				change title
			*/
			renderSignalUpdate(vp);
		}
	}else if (!modifier){
		pageSendMessage(vp->pages, pageInputGetTop(vp->pages), PAGE_MSG_KEY_DOWN, key, 0, NULL);
	}
}


/*
#ifdef __GNUC__
#define SHARED(T,X) T X __attribute__((section(".shared"), shared)) = (T)0
#endif
SHARED(unsigned, WM_ShellHook);

extern unsigned int WM_ShellHook;
*/


static inline LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TVLCPLAYER *vp = (TVLCPLAYER*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (!vp) vp = (TVLCPLAYER*)lParam;

	//if (message != 3224 && message != 289 && message != 287 && message != 255 && message != 5536)
	//	printf("%p %i/%X %i %X, %X %X, %X %X, %p\n", hwnd, (int)message, (int)message, (int)wParam, (int)lParam, (int)wParam&0xFFFF, (int)lParam&0xFFFF, (int)(wParam>>16)&0xFFFF, (int)(lParam>>16)&0xFFFF, vp);


	/*if (message == WM_ShellHook){
		printf("WM_ShellHook\n");
	}*/

	switch (message){
	 /* case WM_SHELL_HOOK:
	  	printf("WM_SHELL_HOOK\n");
	  	break;*/
	  case WM_ENTERMENULOOP:
	  case WM_EXITMENULOOP:
	  	taskbarTrackRedraw(vp);
	  	break;
	  case WM_ACTIVATEAPP:
		if (!wParam){
			TVLCSTRAY *tray = &vp->gui.tray;
	  		contextMenuHide(tray);
	  		taskbarTrackRedraw(vp);
	  	}
	  	break;
	  case WM_SYSTRAY:
		if (wParam == APP_ICON){
			TVLCSTRAY *tray = &vp->gui.tray;
			if (!tray->enabled) return 0;
			
			if (lParam == WM_LBUTTONUP){
				playlistMenuClean(tray);
				playlistMenuSetPlaylist(tray, vp, getDisplayPlaylistUID(vp));
				playlistMenuShow(tray, 1);
				return 0;
				
			}else if (lParam == WM_RBUTTONUP){
				int cTotal = getTotalChapters(vp) > 1;
				int eTotal = epgGetTotalProgrammes(vp) > 1;
				int sTotal = getSubtitleTotal(vp) > 0;
				int isDifferent = (cTotal ^ tray->context.menuChaptersAdded);
				isDifferent |= (eTotal ^ tray->context.menuProgrammeAdded);
				isDifferent |= (sTotal ^ tray->context.menuSubtitlesAdded);
				
				//printf("WM_SYSTRAY %i %i %i, %i\n", cTotal, eTotal, sTotal, isDifferent);
				if (isDifferent){
					contextMenuHide(tray);
					contextMenuDestroy(tray);
					contextMenuCreate(tray, vp, cTotal, eTotal, sTotal);
				}
				
				if (getPlayState(vp)/* && getPlayState(vp) != 8*/){
					contextMenuEnableItem(tray, TRAY_MENU_POSITION);
					contextMenuEnableItem(tray, TRAY_MENU_VOLUME);
					contextMenuEnableItem(tray, TRAY_MENU_MUTE);
					contextMenuEnableItem(tray, TRAY_MENU_PLAYINVLC);
					
					if (getPlayState(vp) == 8)	// is stopped at end of track
						contextMenuDisableItem(tray, TRAY_MENU_STOP);
					else
						contextMenuEnableItem(tray, TRAY_MENU_STOP);

					if (getPlayState(vp) == 1)
						contextMenuSetImage(tray, TRAY_MENU_PLAY, BM_PAUSE);
					else
						contextMenuSetImage(tray, TRAY_MENU_PLAY, BM_PLAY);

					if (getMute(vp))
						contextMenuSetImage(tray, TRAY_MENU_MUTE, BM_MUTED);
					else
						contextMenuSetImage(tray, TRAY_MENU_MUTE, BM_MUTE);
				}else{
					contextMenuSetImage(tray, TRAY_MENU_PLAY, BM_PLAY);
					contextMenuDisableItem(tray, TRAY_MENU_POSITION);
					contextMenuDisableItem(tray, TRAY_MENU_VOLUME);
					contextMenuDisableItem(tray, TRAY_MENU_MUTE);
					contextMenuDisableItem(tray, TRAY_MENU_PLAYINVLC);
					contextMenuDisableItem(tray, TRAY_MENU_STOP);
				}

				contextMenuShow(tray, 1);

				return 0;
			
			//}else if (lParam == WM_LBUTTONDOWN || lParam == WM_RBUTTONDOWN){
				//timerSet(vp, TIMER_TASKBARTITLE_UPDATE, 100);
			}
		}
		break;
	  case WM_TRACKTIMESTAMPNOTIFY:{
		TVLCSTRAY *tray = &vp->gui.tray;
		if (!tray->enabled) return 0;
  		contextMenuSetTrackbarPosition(tray, getConfig(vp)->position);
	  	break;
	  }
	  //case WM_TRACKCHAPTERUPDATE:{
	  //	TVLCSTRAY *tray = &vp->gui.tray;
	  //	if (!tray->enabled) return 0;
		//printf("WM_TRACKCHAPTERUPDATE %i %i\n", (int)wParam, (int)lParam);
		//chaptersMenuDestroy(tray);
		//chaptersMenuCreate(tray, vp);
		//chaptersMenuShow(tray, 0);
	  //}
	  //break;
	  case WM_TRACKPLAYNOTIFY:{
		//printf("WM_TRACKPLAYNOTIFY %p %X %X\n", (void*)hwnd, (int)wParam, (int)lParam);
	  	taskbarTrackRedraw(vp);
	  	TVLCSTRAY *tray = &vp->gui.tray;
		if (!tray->enabled) return 0;

		if ((int)wParam != -1){
			playlistMenuSetInfoTrack(vp, tray, (int)wParam, (int)lParam);
			playlistMenuSetTipTrack(vp, tray, NULL, (int)wParam, (int)lParam);
			break;
		}

		if (getPlayState(vp) && getPlayState(vp) != 8){	// 8 == media is loaded but stopped at end of play
			if (getMute(vp))
				contextMenuSetImage(tray, TRAY_MENU_MUTE, BM_MUTED);
			else
				contextMenuSetImage(tray, TRAY_MENU_MUTE, BM_MUTE);
		
			if (getPlayState(vp) == 1)
				contextMenuSetImage(tray, TRAY_MENU_PLAY, BM_PAUSE);
			else
				contextMenuSetImage(tray, TRAY_MENU_PLAY, BM_PLAY);

			contextMenuEnableItem(tray, TRAY_MENU_STOP);
			contextMenuEnableItem(tray, TRAY_MENU_MUTE);
			contextMenuEnableItem(tray, TRAY_MENU_VOLUME);
			contextMenuEnableItem(tray, TRAY_MENU_POSITION);
			contextMenuEnableItem(tray, TRAY_MENU_PLAYINVLC);
			
			contextMenuDisableItem(tray, TRAY_MENU_PLAY);	// force icon to change
			contextMenuEnableItem(tray, TRAY_MENU_PLAY);
		}else{
			//printf("WM_TRACKPLAYNOTIFY stopped: %i\n", getPlayState(vp));
			
			contextMenuDisableItem(tray, TRAY_MENU_STOP);
			contextMenuDisableItem(tray, TRAY_MENU_MUTE);
			contextMenuDisableItem(tray, TRAY_MENU_VOLUME);
			contextMenuDisableItem(tray, TRAY_MENU_POSITION);
			contextMenuDisableItem(tray, TRAY_MENU_PLAYINVLC);

			contextMenuSetImage(tray, TRAY_MENU_PLAY, BM_PLAY);
			contextMenuDisableItem(tray, TRAY_MENU_PLAY);	// force icon to change
			contextMenuEnableItem(tray, TRAY_MENU_PLAY);
		}
		break;
	  }
	  case WM_VSCROLL:{
	  	TVLCSTRAY *tray = &vp->gui.tray;
	  	if ((HWND)lParam != tray->volume.hwnd)
	  		break;
	  	const int pos = HIWORD(wParam);
	  	const int code = LOWORD(wParam);
	  	
	  	switch (code){
		case TB_PAGEDOWN:{
			int pos = (int)SendMessage(tray->volume.hwnd, TBM_GETPOS, 0, 0);
			contextMenuHandletTrackbarVolume(vp, pos);
			break;
		}
		case TB_THUMBPOSITION:
		case TB_THUMBTRACK:
			contextMenuHandletTrackbarVolume(vp, pos);
			break;
		case TB_TOP:
			contextMenuHandletTrackbarVolume(vp, TRACK_VOLUME_RANGE-1);
			break;
		case TB_BOTTOM:
			contextMenuHandletTrackbarVolume(vp, 0);
			break;
		}
		break; 
	  }
	  case WM_HSCROLL:{
	  	TVLCSTRAY *tray = &vp->gui.tray;
	  	if ((HWND)lParam != tray->position.hwnd)
	  		break;

	  	const int pos = HIWORD(wParam);
	  	const int code = LOWORD(wParam);
	  	
	  	switch (code){
		case TB_PAGEDOWN:{
			int pos = (int)SendMessage(tray->position.hwnd, TBM_GETPOS, 0, 0);
			contextMenuHandletTrackbarPosition(vp, pos);
			break;
		}
		case TB_THUMBPOSITION:
		case TB_THUMBTRACK:
			contextMenuHandletTrackbarPosition(vp, pos);
			break;
		case TB_TOP:
			contextMenuHandletTrackbarPosition(vp, 0);
			break;
		case TB_BOTTOM:
			contextMenuHandletTrackbarPosition(vp, TRACK_POSITION_RANGE-1);
			break;
		}
		break;  
	  }         

	  /*case WM_NOTIFY:{
		NMHDR *nm = (NMHDR*)lParam;
		printf("notify: %i %i %p\n", nm->code, nm->idFrom, nm->hwndFrom);
		break;
	  }*/
	  case WM_COMMAND:{
	  	TVLCSTRAY *tray = &vp->gui.tray;
	  	//printf("WM_COMMAND %p %X %X\n", (void*)hwnd, (int)wParam, (int)lParam);
	  	
	  	int persistantMenu = 0;
	  	settingsGet(vp, "systray.persistantMenus", &persistantMenu);
	  	
  		if (wParam >= TRAY_MENU_CONTEXT_BASE && wParam <= TRAY_MENU_CONTEXT_UPPER)			// context menu (right click)
	  		contextMenuHandleSelection(vp, tray, wParam, persistantMenu);
		else if (wParam >= TRAY_MENU_CHAPTERS_BASE && wParam <= TRAY_MENU_CHAPTERS_UPPER)	// chapters menu
			chaptersMenuHandleSelection(vp, tray, wParam, persistantMenu);
		else if (wParam >= TRAY_MENU_SUBTITLES_BASE && wParam <= TRAY_MENU_SUBTITLES_UPPER)	// subtitle menu
			subtitlesMenuHandleSelection(vp, tray, wParam, persistantMenu);
		else if (wParam >= TRAY_MENU_PROGRAMME_BASE && wParam <= TRAY_MENU_PROGRAMME_UPPER)	// epg channel menu
			programmeMenuHandleSelection(vp, tray, wParam, persistantMenu);
		else if (wParam >= (PLAYLIST_UID_BASE+TRAY_PLY_BASEID)-1)							// playlist menu (left click)
			playlistMenuItemSelected(vp, tray, wParam, persistantMenu);

	  	break;
	  //case WM_MOUSEMOVE:
		//printf("mouse move: %p, %i %i %i\n", hwnd, message, (int)wParam, (int)lParam);
		//break;
	  }	
	  case WM_INPUT:
	  	mouseRawInputProcess(vp, (HRAWINPUT)lParam);
	  	//printf("## mouse WM_INPUT: %p, %i %i %i\n", hwnd, message, (int)wParam, (int)lParam);
	  	break;
	  
	  
	  //  WM_DD_ events generated by the libmylcd DirectDraw driver
	  case WM_DD_LBUTTONDOWN:
	  	mouseCapState = MOUSE_CAP_ON;
	  	ALLOW_FALLTHROUGH;
	  case WM_DD_RBUTTONDOWN:
	  case WM_DD_MBUTTONDOWN:
	  	//printf("\n\n");
	  	vp->gui.cursor.dx = lParam&0xFFFF;
	  	vp->gui.cursor.dy = (lParam>>16)&0xFFFF;
	  	
	  	//printf("WM_DD_xBUTTON DOWN %i %i\n",  vp->gui.cursor.dx, vp->gui.cursor.dy);
	  	mouseButton(vp, message-WM_MM, vp->gui.cursor.dx, vp->gui.cursor.dy);
	  	break;
	  	
	  // up event handled via rawInput, used only to capture in focus but non client area MOUSEUP
	  case WM_DD_LBUTTONUP:
	  	vp->gui.cursor.dx = lParam&0xFFFF;
	  	vp->gui.cursor.dy = (lParam>>16)&0xFFFF;
	  	break;
	  	
	  case WM_DD_RBUTTONUP:
	  case WM_DD_MBUTTONUP:
	  	vp->gui.cursor.dx = lParam&0xFFFF;
	  	vp->gui.cursor.dy = (lParam>>16)&0xFFFF;
	  	
	  	if (mouseCapState == MOUSE_CAP_OFF){
	  		//printf("WM_DD_xBUTTON UP %i %i\n", vp->gui.cursor.dx, vp->gui.cursor.dy);
	  		mouseButton(vp, message-WM_MM, vp->gui.cursor.dx, vp->gui.cursor.dy);
	  	}
	  	break;
	  	
	  case WM_DD_MOUSEMOVE:{
	  	int x = lParam&0xFFFF;
	  	int y = (lParam>>16)&0xFFFF;
		mouseMove(vp, (int)wParam&0x7, x, y, 2);
	  	//printf("WM_DD_MOUSEMOVE %i %i %i\n",  vp->gui.cursor.dx, vp->gui.cursor.dy, vp->gui.cursor.LBState);
	  	
		if (ccHoverRenderSigGetState(vp->cc)){	// do we want a render update on each mouse move event
	  		int objId = ccIsHoveredMM(vp->cc, pageInputGetTop(vp->pages), x, y, 1);
	  		//printf("WM_DD_MOUSEMOVE %i, %i %i\n", objId, x, y);
			if (objId){
				const double t0 = getTime(vp);
				//printf("ccHoverRenderSigGetFPS(vp->cc) %f\n", ccHoverRenderSigGetFPS(vp->cc));
 				if (t0 - vp->lastRenderTime > ((1.0/ccHoverRenderSigGetFPS(vp->cc))*1000.0))
	  				renderSignalUpdate(vp);
	  		}
		}
	  	break;
	  }
	  case WM_DD_MOUSEHWHEEL:{
	  	int x = (int)lParam&0xFFFF;
	  	int y = (int)(lParam>>16)&0xFFFF;
		const int delta = (wParam>>16)&0xFF;
		
		if (delta == WHEEL_DELTA)
			wParam = WM_MWHEEL_RIGHT;
		else if (delta == WHEEL_DELTA+16)
			wParam = WM_MWHEEL_LEFT;
		else
			break;
			
		HWND wnd = GetForegroundWindow();
		RECT rc, clientrc;
		GetWindowRect(wnd, &rc);
		GetClientRect(wnd, &clientrc);
			
		x -= rc.left;
		y -= rc.top;
		if (x < 0 || x >= clientrc.right) break;
		if (y < 0 || y >= clientrc.bottom) break;

		mouseWheel(vp, wParam, x, y);
		break;
	  }
	  case WM_DD_MOUSEWHEEL:{
	  	int x = (int)lParam&0xFFFF;
	  	int y = (int)(lParam>>16)&0xFFFF;
	  	//printf("%i %i, %i\n", x, y, (int)(wParam>>16)&0xFFFF);
	  
		if (((int)wParam>>16) > 0)
			wParam = WM_MWHEEL_FORWARD;
		else if (((int)wParam>>16) < 0)
			wParam = WM_MWHEEL_BACK;
		else
			break;

		HWND wnd = GetForegroundWindow();
		RECT rc, clientrc;
		GetWindowRect(wnd, &rc);
		GetClientRect(wnd, &clientrc);
			
		x -= rc.left;
		y -= rc.top;
		if (x < 0 || x >= clientrc.right) break;
		if (y < 0 || y >= clientrc.bottom) break;

		mouseWheel(vp, wParam, x, y);
		break;
	  }
	  case WM_TOUCHIN:
	  	if (wParam && vp->applState){
	  		TTOUCHINPOS *tin = (TTOUCHINPOS*)wParam;

	  		//printf("tin->flags %i\n", tin->flags);
			vp->gui.cursor.dx = tin->pos.x;
			vp->gui.cursor.dy = tin->pos.y;
			touchSimulate(&tin->pos, tin->flags, tin->ptr);
			my_free(tin);
		}
		return 0;

	  case WM_HOOKPOINTER:
	    lParam = 'Q' << 16;
		ALLOW_FALLTHROUGH;
	    // continue..
	  case WM_HOTKEY:{
	  	//vp = (TVLCPLAYER*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	  	if (vp == NULL) return 0;

	  	const int key = lParam>>16;
	  	
	  	//printf("WM_HOTKEY: %i, 0x%x 0x%x\n", key, (int)wParam, (int)lParam);
	  	
	  	if (!pageDispatchMessage(vp->pages, PAGE_MSG_HOTKEY, key, wParam, (void*)lParam))
	  		return 0;
	  	
	  	// not handled so apply default actions
	  	/*if (key == VK_FULLSCREEN){
	  		if (pageGet(vp) != PAGE_HOME)
	  			pageSet(vp, PAGE_HOME);
	  		else
	  			page2SetPrevious(page2PageStructGet(vp->pages, pageRenderGetTop(vp->pages)));
	  		
	  	}else if (key == VK_F1){
	  		
	  	}else if (key == VK_F2){
	  		
	  	}else if (key == VK_F3){
	  		
	  	}else if (key == VK_F4){
	  		
	  	}else*/ if (key == VK_MEDIA_SEEK_BACK){
			timerSet(vp, TIMER_REWIND, 0);
			
	  	}else if (key == VK_MEDIA_SEEK_FOWARD){
	  		timerSet(vp, TIMER_FASTFORWARD, 0);
	  		
	  	}else if (key == VK_MEDIA_PREV_TRACK){
		  	timerSet(vp, TIMER_PREVTRACK, 0);
		  	
		}else if (key == VK_MEDIA_NEXT_TRACK){
		  	timerSet(vp, TIMER_NEXTTRACK, 0);

		}else if (key == VK_MEDIA_PLAY_PAUSE){
			if (getPlayState(vp) == 1 || getPlayState(vp) == 2)		// is paused
	  	  		timerSet(vp, TIMER_PLAYPAUSE, 0);
	  	  	else
	  	  		timerSet(vp, TIMER_PLAY, 0);

		}else if (key == VK_MEDIA_STOP){
	  	  	timerSet(vp, TIMER_STOP, 0);

		}else if (key == VK_VOLUME_UP){
		  	timerSet(vp, TIMER_VOL_APP_UP, 0);

		}else if (key == VK_VOLUME_DOWN){
		  	timerSet(vp, TIMER_VOL_APP_DN, 0);

		}else if (key == vp->gui.hotkeys.cursor || key == 'Q'){
			if (!vp->renderState) return 1;
	  	  	wakeup(vp);
	  	  	mouseLBUp(vp, 0, 0);

	  		if (!mHookGetState()/* && !vp->gui.hooked*/){
	  			mHookInstall(hwnd, vp);
		  		captureMouse(vp, 1);
	  		}else{
	  			captureMouse(vp, 0);
		  		mHookUninstall();
			}
			vp->gui.frameCt = 0;
		  	renderSignalUpdate(vp);
		  	
		}else if (key == vp->gui.hotkeys.console || key == 'P'){
	  	  	if (!vp->renderState) return 1;
	  	  	wakeup(vp);
	  	  	captureKeyboard(vp, !kHookGetState());
		  	renderSignalUpdate(vp);
		}
	  	return 1;
	  }
	  case WM_CHAR:{
	  	setAwake(vp);
	  	if (page2RenderGetState(vp->pages, PAGE_TETRIS)){	// is page visable
	  		//hijack the hooked kb input for tetris control
	  		tetrisInputProc(vp, pageGetPtr(vp, PAGE_TETRIS), wParam);
	  		renderSignalUpdate(vp);
	  		return 0;
	  	}

	  	int ret = editBoxInputProc(&vp->input, vp->gui.hMsgWin, wParam);
	  	if (ret == 2){
			wchar_t *txt = editboxGetString(&vp->input);
			if (txt){
				const int ilen = wcslen(txt);
				if (ilen){
					addWorkingBuffer(&vp->input);
					nextHistoryBuffer(&vp->input);
					clearWorkingBuffer(&vp->input);
					captureKeyboard(vp, 0);
					if (editboxProcessString(&vp->input, txt, ilen, vp)){
						captureKeyboard(vp, 1);
					}
				}
				my_free(txt);
			}
	  	}
	  	renderSignalUpdate(vp);
	  	break;
	  }
	  case WM_DD_CHARDOWN:
	  	setModifierKeyStates(vp, lParam);
	  	wakeup(vp);
 		//printf("## WM_DD_CHARDOWN: 0x%X %X, 0x%X\n", (int)wParam, (int)lParam, getModifierKeyStates(vp));
	  	
	  	if (hasPageBeenAccessed(vp, PAGE_VKEYBOARD) && pageInputGetTop(vp->pages) == PAGE_VKEYBOARD)
 			pageSendMessage(vp->pages, PAGE_VKEYBOARD, PAGE_MSG_CHAR_DOWN, wParam, getModifierKeyStates(vp), NULL);
 		else if (!getModifierKeyStates(vp)/* && wParam != 0x11*/)	// 0x11 is the CtrlKey key code
			pageSendMessage(vp->pages, pageInputGetTop(vp->pages), PAGE_MSG_CHAR_DOWN, wParam, getModifierKeyStates(vp), NULL);
	  	break;

	  case WM_DD_KEYDOWN:
		setModifierKeyStates(vp, lParam);
		wakeup(vp);
	  	//printf("## WM_DD_KEYDOWN: 0x%X %X, 0x%X\n", (int)wParam, (int)lParam, getModifierKeyStates(vp));
	  	
	  	// keypad control has priority over the keyboard
	  	if (hasPageBeenAccessed(vp, PAGE_VKEYBOARD) && pageInputGetTop(vp->pages) == PAGE_VKEYBOARD)
  			pageSendMessage(vp->pages, PAGE_VKEYBOARD, PAGE_MSG_KEY_DOWN, wParam, getModifierKeyStates(vp), NULL);
	  	else
	  		keyboardHandleModifierPress(vp, wParam, getModifierKeyStates(vp));
	  	break;

	  case WM_KEYDOWN:
	  	//setAwake(vp);
		if (wParam == 27)
		  	captureKeyboard(vp, 0);
    	else if (wParam >= VK_PRIOR && wParam <= VK_DELETE)
   			editBoxInputProc(&vp->input, vp->gui.hMsgWin, wParam|0x1000);

   		vp->gui.frameCt = 0;
   		renderSignalUpdate(vp);		
	  	return 0;

	  case WM_EXTCMD:{
	  	//vp = (TVLCPLAYER*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (vp)
			extCommandFunc(vp, (int)wParam, (int)lParam, NULL, NULL);

		return 0;
	  }

#if SINGLEINSTANCE_USE_CDS
	  case WM_COPYDATA:
		cds_receive(vp, wParam, (COPYDATASTRUCT*)lParam);
	  	return 1;
#else
	  case WM_ADDTRACKINIT:{
	  	if (vp == NULL) return 0;

		unsigned int hash = wParam;
		int len = lParam;
	  	
	  	if (hash && len > 2){
	  		vp->pathmsg.hash = hash;
	  		vp->pathmsg.len = len;
	  		if (vp->pathmsg.path)
	  			my_free(vp->pathmsg.path);

  			vp->pathmsg.path = my_calloc(len+4, sizeof(unsigned char));
  			if (vp->pathmsg.path)
  				vp->pathmsg.state = 1;
  			else
  				vp->pathmsg.state = 0;
	  	}
		break;
	  }
	  case WM_ADDTRACKCHAR:
	  	if (vp == NULL) return 0;
	  	unsigned int hash = wParam;

	  	if (vp->pathmsg.state == 1 && hash == vp->pathmsg.hash){
			unsigned int pos = lParam>>16;
			
			if (pos == vp->pathmsg.charPos && pos < vp->pathmsg.len){
				unsigned char chr = lParam&0xFF;
				
				if (!chr && pos == vp->pathmsg.len-1){
					if (getHash(vp->pathmsg.path) == hash){
						dbprintf(vp, "Importing '%s'", vp->pathmsg.path);
						playlistResetRetries(vp);
						playlistImportPath(vp, getPrimaryPlaylist(vp), vp->pathmsg.path);
					}
					vp->pathmsg.state = 2;

				}else if (chr){
					vp->pathmsg.path[pos] = chr;
					vp->pathmsg.path[pos+1] = 0;
					vp->pathmsg.charPos++;					
				}else{
					vp->pathmsg.state = 2;
				}
			}else{
				vp->pathmsg.state = 2;
			}
		}
		
	  	if (vp->pathmsg.state == 2){
	  		if (vp->pathmsg.path)
	  			my_free(vp->pathmsg.path);
	  		vp->pathmsg.path = NULL;
	  		vp->pathmsg.hash = 0;
	  		vp->pathmsg.len = 0;
	  		vp->pathmsg.state = 0;
	  		vp->pathmsg.action = 0;
	  		vp->pathmsg.charPos = 0;
	  	}
		break;
#endif

	  case WM_WAKEUP:
	  	if (vp == NULL) return 0;

		setAwake(vp);
		updateTickerStart(vp, UPDATERATE_ALIVE);
		vp->gui.frameCt = 0;
	  	renderSignalUpdate(vp);
	  	SetEvent(vp->instanceEvent);
	  	pageDispatchMessage(vp->pages, PAGE_MSG_WAKEUP, 0, 0, NULL);
	  	break;
	  	
	  case WM_VAUDIO_VOL_CHANGE:
	  	//const float vol = (wParam/10.0f)+0.1f;
	  	//pageDispatchMessage(vp->pages, PAGE_MSG_VOLUME_CHANGE, VOLUME_MASTER, 0, &vol);	// when no one is listening..
	  	
		if (!getIdle(vp)){
	  		//int mute = lParam;
	  		const float vol = (wParam/10.0f)+0.1f;
			//printf("volume %.2f %i\n", vol, mute);
			overlaySetOverlay(vp);
			setVolumeDisplay(vp, vol);
			renderSignalUpdate(vp);
		}
	  	break;
	  	
	  case WM_MEDIA_CHANGE:{
		long lEvent = 0;
        char drivePath[MAX_PATH+1];
        LPITEMIDLIST *ppidl = NULL;
    
        HANDLE hLock = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);
		if (hLock && ppidl){
			if (!SHGetPathFromIDList(ppidl[0], drivePath))
				break;
			SHChangeNotification_Unlock(hLock);

			//printf("SHCNE event %X/%i '%s'\n", (int)lEvent, (int)lEvent, drivePath);

			switch (lEvent){
		  	case SHCNE_DRIVEADD:
				pageDispatchMessage(vp->pages, PAGE_MSG_DRIVE_ARRIVE, drivePath[0], fbIsUsbDrive(drivePath[0]), drivePath);
				break; 
		  	case SHCNE_MEDIAINSERTED:
		  		pageDispatchMessage(vp->pages, PAGE_MSG_MEDIA_ARRIVE, drivePath[0], fbIsUsbDrive(drivePath[0]), drivePath);
				break;
			case SHCNE_DRIVEREMOVED:
				pageDispatchMessage(vp->pages, PAGE_MSG_DRIVE_DEPART, drivePath[0], fbIsUsbDrive(drivePath[0]), drivePath);
				break;
		  	case SHCNE_MEDIAREMOVED:
		  		pageDispatchMessage(vp->pages, PAGE_MSG_MEDIA_DEPART, drivePath[0], fbIsUsbDrive(drivePath[0]), drivePath);
				break;
			}
          	//if (ppidl[0]) ILFree(ppidl[0]);
		}
        break;
	  }	

	  case WM_DEVICECHANGE:{
	  	DEV_BROADCAST_HDR *hdr = (DEV_BROADCAST_HDR*)lParam;
	  	//if (!hdr)
	  	//	printf("WM_DEVICECHANGE %X\n", wParam);
	  	//else
	  	//	printf("WM_DEVICECHANGE %i %i\n", (int)hdr->dbch_devicetype, (int)hdr->dbch_size);
	  	
        switch (wParam){
          case DBT_DEVICEARRIVAL:
          	//printf("WM_DEVICECHANGE ARRIVE %i, %d\n", (int)hdr->dbch_devicetype, (int)hdr->dbch_size);
          	
          	/*if (hdr->dbch_devicetype == DBT_DEVTYP_VOLUME){
          		DEV_BROADCAST_VOLUME *dbv = (DEV_BROADCAST_VOLUME*)hdr;
          		printf("# DBT_DEVTYP_VOLUME %X %i\n", (int)dbv->dbcv_unitmask, (int)dbv->dbcv_flags);
          		
          	}else*/ if (hdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE){
          		DEV_BROADCAST_DEVICEINTERFACE_A *dbi = (DEV_BROADCAST_DEVICEINTERFACE_A*)hdr;
          		//printf("# DBT_DEVTYP_INTERFACE %X %s\n", ((int*)&dbi->dbcc_classguid)[0], dbi->dbcc_name);
          		
          		int vid, pid;
          		if (guidToId(dbi->dbcc_name, &vid, &pid)){
          			onDeviceArrive(vp, ((int*)&dbi->dbcc_classguid)[0], vid, pid);
          			return 0;
          		}
          	}
          	break;
          	
          case DBT_DEVICEREMOVECOMPLETE:
          	//printf("WM_DEVICECHANGE REMOVE %i, %d\n", (int)hdr->dbch_devicetype, (int)hdr->dbch_size);
          	
          	/*if (hdr->dbch_devicetype == DBT_DEVTYP_VOLUME){
          		DEV_BROADCAST_VOLUME *dbv = (DEV_BROADCAST_VOLUME*)hdr;
          		printf("~ DBT_DEVTYP_VOLUME %X %i\n", (int)dbv->dbcv_unitmask, (int)dbv->dbcv_flags);
          		
          	}else*/ if (hdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE){
          		DEV_BROADCAST_DEVICEINTERFACE_A *dbi = (DEV_BROADCAST_DEVICEINTERFACE_A*)hdr;
          		//printf("~ DBT_DEVTYP_INTERFACE %X %s\n", ((int*)&dbi->dbcc_classguid)[0], dbi->dbcc_name);
          		int vid, pid;
          		if (guidToId(dbi->dbcc_name, &vid, &pid)){
          			onDeviceDepart(vp, ((int*)&dbi->dbcc_classguid)[0], vid, pid);
          			return 0;
          		}
          	}
          	break;
          	
          /*case DBT_DEVNODES_CHANGED:
			printf("WM_DEVICECHANGE CHANGED %i\n", (int)hdr->dbch_devicetype);
          	break;*/
         }//switch
       }//case
       break;

	  case WM_CREATE:
	  	SetupVistaVolume(hwnd);
		break;
	  case WM_QUIT: 
		DestroyWindow(hwnd);
		ALLOW_FALLTHROUGH;
	  case WM_CLOSE: 
		return 0;
 	  case WM_DESTROY:
		PostQuitMessage(0);
	    return 0;
	  case WM_DD_CLOSE:
		if (hasPageBeenAccessed(vp, PAGE_SEARCH))
			searchForceStop(vp);

	  	wakeup(vp);
		timerSet(vp, TIMER_SHUTDOWN, 0);
		PostMessage(hwnd, WM_QUIT, 0, 0);
	  	return 0;
	}
	
	
	if (message == taskbarRestartMsg[0] || message == taskbarRestartMsg[1]){
		//printf("taskbarRestartMsg %i\n", message);
		taskbarTrayClose(&vp->gui.tray);
		taskbarToolbarRelease(&vp->gui.taskbar);
		initTray(vp, hwnd);
		timerSet(vp, TIMER_TASKBARTITLE_UPDATE, 100);
	}
	
	
	return DefWindowProc(hwnd, message, wParam, lParam);
}

int gdiPlusInit (TGUI *gui)
{
	GdiplusStartupInput gsi = {0};
	gsi.GdiplusVersion = 1;
	return GdiplusStartup(&gui->gdip.token, &gsi, NULL) == 0;
}

void gdiPlusClose (TGUI *gui)
{
	GdiplusShutdown(gui->gdip.token);
}

static inline HANDLE initGUI (TVLCPLAYER *vp)
{
	InitCommonControls();
	gdiPlusInit(&vp->gui);

	const char *szClassName = NAME_WINMSG;
    WNDCLASSEX wincl;
    wincl.cbSize = sizeof(WNDCLASSEX);
    wincl.hInstance = vp->instanceModule;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;
    wincl.hIcon = NULL;
    wincl.hIconSm = NULL;
    wincl.hCursor = NULL;
    wincl.hbrBackground = NULL;
    wincl.style = CS_DBLCLKS;
    wincl.lpszMenuName = NULL;
    wincl.cbClsExtra = 0;
    wincl.cbWndExtra = 0;
    if (!RegisterClassEx (&wincl))
        return NULL;

    HWND hMsgWin = CreateWindow(szClassName, szClassName, WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,\
	  0, 0, HWND_DESKTOP, NULL, wincl.hInstance, NULL);

	if (hMsgWin){
    	SetWindowLongPtr(hMsgWin, GWLP_USERDATA, (LONG_PTR)vp);
    	ShowWindow(hMsgWin, SW_HIDE);
		initTray(vp, hMsgWin);
    }

    return hMsgWin;
}

int captureKeyboard (TVLCPLAYER *vp, const int state)
{
	if (state){
		if (!kHookGetState() && kHookInstall(vp->gui.hMsgWin, vp)){
			kHookOn();
 			if (!kHookGetState())
 				kHookUninstall();
 			else
 				pageDispatchMessage(vp->pages, PAGE_MSG_INPUT_KCAP, 1, 0, NULL);
		}
	}else{
		if (kHookGetState()){
			kHookOff();
			kHookUninstall();
			pageDispatchMessage(vp->pages, PAGE_MSG_INPUT_KCAP, 0, 0, NULL);
		}
	}
	
	return kHookGetState();
}

int captureMouse (TVLCPLAYER *vp, const int state)
{
	if (state){
		if (!mHookGetState()){
			GetCursorPos(&vp->gui.cursor.pt);
			mHookSetCB(vp->gui.hMsgWin, (void*)hookCB);
			mHookOn();
		
			if (mHookGetState()){
				vp->gui.cursor.dx = vp->ctx.working->width/2;
 				vp->gui.cursor.dy = vp->ctx.working->height/2;
				vp->gui.cursor.isHooked = -1;
		
				pageDispatchMessage(vp->pages, PAGE_MSG_INPUT_MCAP, 1, 0, NULL);
				wakeup(vp);
			}
		}
	}else{
		vp->gui.cursor.isHooked = 0;
		if (mHookGetState()){
			mHookOff();
			pageDispatchMessage(vp->pages, PAGE_MSG_INPUT_MCAP, 0, 0, NULL);
		}
		SetCursorPos(vp->gui.cursor.pt.x, vp->gui.cursor.pt.y);
	}
	return mHookGetState();
}

int captureMouseToggleState (TVLCPLAYER *vp)
{
	return captureMouse(vp, (vp->gui.cursor.isHooked == 0));
}

#if 1
void RegisterDeviceInterfaceToHwnd (GUID InterfaceClassGuid, HWND hWnd, HDEVNOTIFY *hDeviceNotify)
{
  DEV_BROADCAST_DEVICEINTERFACE nf;
  memset(&nf, 0, sizeof(nf));
  
  nf.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
  nf.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
  nf.dbcc_classguid = InterfaceClassGuid;

  *hDeviceNotify = RegisterDeviceNotificationW(hWnd, &nf, DEVICE_NOTIFY_WINDOW_HANDLE);
}
#endif


unsigned int RegisterShellNotify (HWND hWnd/*, SHChangeNotifyEntry *shcne*/, unsigned int *notifyId)
{
#if 1
	HANDLE hLib = GetModuleHandle("shell32.dll");
   	pSHChangeNotification_Lock = (void*)GetProcAddress(hLib, "SHChangeNotification_Lock");
   	pSHChangeNotification_Unlock = (void*)GetProcAddress(hLib, "SHChangeNotification_Unlock");
   	pSHChangeNotifyRegister = (void*)GetProcAddress(hLib, "SHChangeNotifyRegister");
   	pSHChangeNotifyDeregister = (void*)GetProcAddress(hLib, "SHChangeNotifyDeregister");
#endif

	SHChangeNotifyEntry shcne;
	memset(&shcne, 0, sizeof(shcne));
	
	shcne.pidl = NULL;
   	shcne.fRecursive = TRUE;
   	long fEvents = SHCNE_DRIVEADD | SHCNE_DRIVEREMOVED | SHCNE_MEDIAREMOVED | SHCNE_MEDIAINSERTED;
   	//fEvents |= SHCNE_DISKEVENTS | SHCNE_ALLEVENTS | SHCNE_INTERRUPT;
    	
   	//if (pSHChangeNotifyRegister)
   		*notifyId = SHChangeNotifyRegister(hWnd, SHCNF_ACCEPT_INTERRUPTS|SHCNF_ACCEPT_NON_INTERRUPTS|SHCNF_NO_PROXY, fEvents, WM_MEDIA_CHANGE, 1, &shcne);
   		
   	return *notifyId;
}

static inline unsigned int __stdcall winMessageThread (void *ptr)
{
	//printf("winMessageThread %i\n", (int)GetCurrentThreadId());

	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;
	ATOM *kid = vp->gui.hotkeys.kid;
	//memset(kid, 0, sizeof(vp->gui.kid));

	SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
		
	settingsGet(vp, "hotkeys.global.enabled", &vp->gui.hotkeys.globalEnabled);
	settingsGet(vp, "hotkeys.local.enabled", &vp->gui.hotkeys.localEnabled);
	settingsGet(vp, "hotkeys.local.cursor", &vp->gui.hotkeys.cursor);
	settingsGet(vp, "hotkeys.local.console", &vp->gui.hotkeys.console);

	if ((vp->gui.hMsgWin=initGUI(vp))){
		taskbarRestartMsg[0] = RegisterWindowMessage("TaskbarCreated");
		taskbarRestartMsg[1] = RegisterWindowMessage("TaskbarButtonCreated");

		if (isVirtual(vp))
			mouseRawInputInit(vp->gui.hMsgWin);
		
		if (vp->gui.hotkeys.localEnabled){
			kid[0] = GlobalAddAtom("vlcshk_cursor");
			kid[1] = GlobalAddAtom("vlcshk_console");
			
			// set mouse hook control hotkeys
			if (!RegisterHotKey(vp->gui.hMsgWin, kid[0], MOD_CONTROL|MOD_SHIFT, vp->gui.hotkeys.cursor))
				RegisterHotKey(vp->gui.hMsgWin, kid[0], MOD_CONTROL|MOD_SHIFT, 'Q');

			// set keyboard hook control hotkeys
			if (!RegisterHotKey(vp->gui.hMsgWin, kid[1], MOD_CONTROL|MOD_SHIFT, vp->gui.hotkeys.console))
				RegisterHotKey(vp->gui.hMsgWin, kid[1], MOD_CONTROL|MOD_SHIFT, 'P');
		}

		if (vp->gui.hotkeys.globalEnabled){
			kid[2] = GlobalAddAtom("vlcshk_prevtrack");
			kid[3] = GlobalAddAtom("vlcshk_nexttrack");
			kid[4] = GlobalAddAtom("vlcshk_playpause");
			kid[5] = GlobalAddAtom("vlcshk_stop");
			kid[6] = GlobalAddAtom("vlcshk_volumeup");
			kid[7] = GlobalAddAtom("vlcshk_volumedn");
			
			kid[8] = GlobalAddAtom("vlcshk_f1_red");
			kid[9] = GlobalAddAtom("vlcshk_f2_orange");
			kid[10] = GlobalAddAtom("vlcshk_f3_blue");
			kid[11] = GlobalAddAtom("vlcshk_f4_yellow");
			
			kid[12] = GlobalAddAtom("vlcshk_seekfw");
			kid[13] = GlobalAddAtom("vlcshk_seekbk");
			//kid[14] = GlobalAddAtom("vlcshk_fullscreen");
			

			RegisterHotKey(vp->gui.hMsgWin, kid[2], 0, VK_MEDIA_PREV_TRACK);
			RegisterHotKey(vp->gui.hMsgWin, kid[3], 0, VK_MEDIA_NEXT_TRACK);
			RegisterHotKey(vp->gui.hMsgWin, kid[4], 0, VK_MEDIA_PLAY_PAUSE);
			RegisterHotKey(vp->gui.hMsgWin, kid[5], 0, VK_MEDIA_STOP);
			RegisterHotKey(vp->gui.hMsgWin, kid[6], 0, VK_VOLUME_UP);
			RegisterHotKey(vp->gui.hMsgWin, kid[7], 0, VK_VOLUME_DOWN);
			
			RegisterHotKey(vp->gui.hMsgWin, kid[8], MOD_CONTROL|MOD_ALT, VK_RED);				// F1
			RegisterHotKey(vp->gui.hMsgWin, kid[9], MOD_CONTROL|MOD_ALT, VK_ORANGE);			// F2
			RegisterHotKey(vp->gui.hMsgWin, kid[10], MOD_CONTROL|MOD_ALT, VK_BLUE);				// F3
			RegisterHotKey(vp->gui.hMsgWin, kid[11], MOD_CONTROL|MOD_ALT, VK_YELLOW);			// F4
			
			RegisterHotKey(vp->gui.hMsgWin, kid[12], MOD_CONTROL|MOD_SHIFT, VK_SEEK_BACK);		// 'B'
			RegisterHotKey(vp->gui.hMsgWin, kid[13], MOD_CONTROL|MOD_SHIFT, VK_SEEK_FORWARD);	// 'F'
			//RegisterHotKey(vp->gui.hMsgWin, kid[14], MOD_CONTROL, VK_FULLSCREEN);				// '3'
		}
		
		// register usb device removable/insertion for the browser page and AntPlus dongle detection
		HDEVNOTIFY hDeviceNotify[4] = {0};
		RegisterDeviceInterfaceToHwnd(GUID_DEVINTERFACE_USB_HUB, vp->gui.hMsgWin, &hDeviceNotify[0]);
		RegisterDeviceInterfaceToHwnd(GUID_DEVINTERFACE_USB_DEVICE, vp->gui.hMsgWin, &hDeviceNotify[1]);
		RegisterDeviceInterfaceToHwnd(GUID_DEVINTERFACE_USB_HID, vp->gui.hMsgWin, &hDeviceNotify[2]);

		unsigned int shellNotifyId = 0;
    	RegisterShellNotify(vp->gui.hMsgWin, &shellNotifyId);
		
		// enable WM_DD_ callbacks
		if (vp->ml->enableVirtualDisplay)
			lSetDisplayOption(vp->ml->hw, vp->ml->virtualDisplayId, lOPT_DDRAW_HWNDTARGET, (intptr_t*)vp->gui.hMsgWin);

		
		while (vp->applState)
			processWindowMessages(vp);


		// disable WM_DD_ callbacks
		if (vp->ml->enableVirtualDisplay)
			lSetDisplayOption(vp->ml->hw, vp->ml->virtualDisplayId, lOPT_DDRAW_HWNDTARGET, NULL);

		if (shellNotifyId)
			SHChangeNotifyDeregister(shellNotifyId);
		for (int i = 0; i < 4; i++){
			if (hDeviceNotify[i])
				UnregisterDeviceNotification(hDeviceNotify[i]);
		}
		
		if (vp->gui.hotkeys.globalEnabled || vp->gui.hotkeys.localEnabled){
			for (int i = 0; i < (sizeof(kid)/sizeof(ATOM)); i++){
				if (kid[i]){
					UnregisterHotKey(vp->gui.hMsgWin, kid[i]);
					GlobalDeleteAtom(kid[i]);
				}
			}
		}
		
		if (mHookGetState())
			mHookUninstall();
		if (kHookGetState())
			kHookUninstall();

		if (vp->ctx.winRender.enable)
			RedrawWindow(vp->ctx.winRender.wnd, NULL, 0, RDW_INVALIDATE|RDW_UPDATENOW);

		taskbarTrayClose(&vp->gui.tray);
		taskbarToolbarRelease(&vp->gui.taskbar);
		gdiPlusClose(&vp->gui);

	}
	//printf("winMessageThread out\n");
	
	_endthreadex(1);
	return 1;
}

int startMouseCapture (TVLCPLAYER *vp)
{
	vp->gui.hWinMsgThread = _beginthreadex(NULL, THREADSTACKSIZE, winMessageThread, vp, 0, &vp->gui.winMsgThreadID);
	vp->gui.hWinDispatchThread = _beginthreadex(NULL, THREADSTACKSIZE, inputDispatchThread, vp, 0, &vp->gui.winDispatchThreadID);
	vp->gui.hDispatchLock = lockCreate("mouseDispatch");
	vp->gui.hDispatchEvent = CreateEvent(NULL, 0, 0, NULL);
	return (int)vp->gui.winMsgThreadID;
}

void endMouseCapture (TVLCPLAYER *vp)
{
	PostMessage(vp->gui.hMsgWin, WM_QUIT,0,0); // wakeup the message thread
	WaitForSingleObject((HANDLE)vp->gui.hWinMsgThread, INFINITE);
	CloseHandle((HANDLE)vp->gui.hWinMsgThread);
	
	SetEvent(vp->gui.hDispatchEvent);
	WaitForSingleObject((HANDLE)vp->gui.hWinDispatchThread, INFINITE);
	CloseHandle((HANDLE)vp->gui.hWinDispatchThread);
	vp->gui.hWinDispatchThread = 0;
	
	CloseHandle(vp->gui.hDispatchEvent);
	vp->gui.hDispatchEvent = NULL;
	
	lockClose(vp->gui.hDispatchLock);
	vp->gui.hDispatchLock = NULL;
}

void consoleToggle (TVLCPLAYER *vp)
{
	PostMessage(vp->gui.hMsgWin, WM_HOTKEY, 0, 'P'<<16);
}

#endif		// MOUSEHOOKCAP


#if SINGLEINSTANCE_USE_CDS

static inline void cds_send_path (HWND hWin, char *path, const int slen)
{
	COPYDATASTRUCT cds;
	cds.dwData = WM_CDS_ADDTRACK;
	cds.lpData = path;
	cds.cbData = slen + 1;
	SendMessage(hWin, WM_COPYDATA, (WPARAM)generateHash(cds.lpData, cds.cbData), (LPARAM)&cds);
}

int playlistImportPath (TVLCPLAYER *vp, PLAYLISTCACHE *plc, char *pathIn)
{
	int itemsImported = 0;
	
	if (isMediaPlaylist(pathIn)){
		char name[MAX_PATH_UTF8+1] = {0, 0};
		char ext[MAX_PATH_UTF8+1] = {0, 0};
		_splitpath(pathIn, NULL, NULL, name, ext);
								
		if (*name){
			PLAYLISTCACHE *child = playlistManagerCreatePlaylist(vp->plm, name, 0);
			if (child){
				tagFlushOrfhensPlc(vp->tagc, child);
				itemsImported = importPlaylist(vp->plm, child, vp->tagc, vp->am, pathIn, pageGetPtr(vp, PAGE_FILE_PANE));
				resetCurrentDirectory();
				dbprintf(vp, "%i items imported from '%s%s'", itemsImported, name, ext);

				if (itemsImported > 1)
					playlistSort(child, vp->tagc, MTAG_PATH, SORT_ASCENDING);

				playlistAddPlc(plc, child);
				//playlistGetTrackLengths(vp, child, 1, 0);

				timerSet(vp, TIMER_PLYTV_REFRESH, 10);
				timerSet(vp, TIMER_PLYALB_REFRESH, 10);
				timerSet(vp, TIMER_PLYPAN_REBUILD, 10);
			}
		}
	}else{
		wchar_t *path = converttow(pathIn);
		if (path){
			plc->pr->selectedItem = 0;
			setDisplayPlaylist(vp, plc);
			
			int totalBefore = playlistGetTotal(plc);
			cmd_import(path, wcslen(path), vp, 0, 1);		// rewrite/fix this hack
			itemsImported = playlistGetTotal(plc) - totalBefore;
			//playlistGetTrackLengths(vp, plc, 1, 0);
			
			if (itemsImported > 0){
				timerSet(vp, TIMER_PLYTV_REFRESH, 10);
				timerSet(vp, TIMER_PLYALB_REFRESH, 10);
				timerSet(vp, TIMER_PLYPAN_REBUILD, 10);
			}
			my_free(path);
		}
	}
	
	return itemsImported;
}

void cds_send (HWND hWin, const int msg, void *ptr, const int data)
{
	if (msg == WM_CDS_ADDTRACK)
		cds_send_path(hWin, ptr, data);
}

void cds_receive (TVLCPLAYER *vp, WPARAM wParam, COPYDATASTRUCT *cds)
{
	const int type = cds->dwData;
	
	if (type == WM_CDS_ADDTRACK){
		extReceiveCdsPath(vp, WM_CDS_ADDTRACK, wParam, cds->lpData, cds->cbData);
		
	}else if (type == WM_CDS_ADDTRACK_DSP){
		extReceiveCdsPath(vp, WM_CDS_ADDTRACK_DSP, wParam, cds->lpData, cds->cbData);
		
	}else if (type == WM_CDS_ADDTRACK_PLY){
		extReceiveCdsPath(vp, WM_CDS_ADDTRACK_PLY, wParam, cds->lpData, cds->cbData);

	}else if (type == WM_CDS_CMDCTRL){
		const int op = cds->cbData;
		const char *var = cds->lpData;
		//printf("cds receive: %i '%s'\n", op, var);
		
		extReceiveCdsCmd(vp, op, var);
	}
}

#endif	// SINGLEIN