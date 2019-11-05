
// myLCD
// An LCD framebuffer library
// Michael McElligott
// okio@users.sourceforge.net

//  Copyright (c) 2005-2010  Michael McElligott
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU LIBRARY GENERAL PUBLIC LICENSE
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU LIBRARY GENERAL PUBLIC LICENSE for details.


#include <stdio.h>
#include <windows.h>
#include "hook.h"

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
/*

#ifdef __GNUC__
#define SHARED(T,X) T X __attribute__((section(".shared"), shared)) = (T)0
#endif
SHARED(unsigned, WM_ShellHook);
*/



LRESULT CALLBACK mHookProc (int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK kHookProc (int nCode, WPARAM wParam, LPARAM lParam);
//LRESULT CALLBACK sHookProc (int nCode, WPARAM wParam, LPARAM lParam);

typedef int (*hookCB) (void *ptr, int msg1, int msg2, int msg3);

typedef struct{
	HANDLE hNotifyWnd;
	HANDLE hHook;
	HANDLE hDllInstance;
	void *lptr;
	hookCB cbPtr;
	int hookState;
	
//	HANDLE hSrcWnd;
}THOOK;

static THOOK mhook;
static THOOK khook;
//static THOOK shook;


BOOL WINAPI DllMain (HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
		mhook.hDllInstance = hInstDLL;
		khook.hDllInstance = hInstDLL;
		//shook.hDllInstance = hInstDLL;
		DisableThreadLibraryCalls(hInstDLL);
	}
	return 1;
}

__declspec (dllexport) __cdecl BOOL mHookSetCB (HWND hWnd, void *userPtr)
{
	mhook.cbPtr = (hookCB)userPtr;
	//shook.cbPtr = (hookCB)userPtr;
}

__declspec (dllexport) __cdecl BOOL mHookInstall (HWND hWnd, void *userPtr)
{
	if (hWnd == NULL) return FALSE;

	if (mhook.hNotifyWnd != NULL)
		return FALSE;

#if 0

	WM_ShellHook = RegisterWindowMessage("SHELLHOOK");
	shook.hHook = SetWindowsHookEx(WH_SHELL, (HOOKPROC)sHookProc, shook.hDllInstance, 0);
	printf("shook.hHook %p\n", shook.hHook);
	if (shook.hHook != NULL){
		shook.hookState = 1;
		shook.lptr = userPtr;
		shook.cbPtr = NULL;
		shook.hNotifyWnd = hWnd;
	}
#endif
	mhook.hHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)mHookProc, mhook.hDllInstance, 0);
	if (mhook.hHook != NULL){
		mhook.hookState = 0;
		mhook.lptr = userPtr;
		mhook.cbPtr = NULL;
		mhook.hNotifyWnd = hWnd;
		return TRUE;
	}else{
		return FALSE;
	}
}

__declspec (dllexport) __cdecl BOOL mHookUninstall ()
{
	BOOL unHooked = TRUE;
	if (mhook.hNotifyWnd != NULL && mhook.hHook != NULL)
		unHooked = UnhookWindowsHookEx(mhook.hHook);

	mhook.cbPtr = NULL;
	mhook.hHook = NULL;
	mhook.hNotifyWnd = NULL;
	mhook.hookState = 0;
	return unHooked;
}

__declspec (dllexport) __cdecl int mHookGetState ()
{
	return (mhook.hookState && mhook.hHook);
}

__declspec (dllexport) __cdecl void mHookOn ()
{
	mhook.hookState = 1;
}

__declspec (dllexport) __cdecl void mHookOff ()
{
	mhook.hookState = 0;
}

LRESULT CALLBACK mHookProc (int nCode, WPARAM wParam, LPARAM lParam)
{	

	if (nCode == HC_ACTION && mhook.hookState){
		MSLLHOOKSTRUCT *mh = (MSLLHOOKSTRUCT*)lParam;
		//printf("%X %i %i %i, %i\n", (int)mh->mouseData, (int)nCode, (int)wParam, (int)lParam, mh->mouseData>>16);
		
		if (mhook.cbPtr){
			if (wParam == WM_MOUSEWHEEL){
				if (((int)mh->mouseData>>16) > 0)
					wParam = WM_MWHEEL_FORWARD;
				else if (((int)mh->mouseData>>16) < 0)
					wParam = WM_MWHEEL_BACK;
			}else if (wParam == WM_MOUSEHWHEEL){
				const int delta = (mh->mouseData>>16)&0xFF;
				if (delta == WHEEL_DELTA)
					wParam = WM_MWHEEL_RIGHT;
				else if (delta == WHEEL_DELTA+16)
					wParam = WM_MWHEEL_LEFT;
				//printf("mh->mouseData %i %i %i\n", wParam, lParam, delta);
			}
			//printf("mh->mouseData %i %i %i,%i\n", wParam, lParam, (int)mh->pt.x, (int)mh->pt.y);
			return mhook.cbPtr(mhook.lptr, (int)wParam, (int)mh->pt.x, (int)mh->pt.y);
		}
	}
	return CallNextHookEx(mhook.hHook, nCode, wParam, lParam);
}


/*
####################################################################################################
####################################################################################################
####################################################################################################
*/


__declspec (dllexport) __cdecl BOOL kHookInstall (HWND hWnd, void *userPtr)
{
	if (hWnd == NULL)
		return FALSE;

	if (khook.hNotifyWnd != NULL)
		return FALSE;

	khook.hHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)kHookProc, khook.hDllInstance, 0);
	if (khook.hHook != NULL){
		khook.hookState = 0;
		khook.lptr = userPtr;
		khook.cbPtr = NULL;
		khook.hNotifyWnd = hWnd;
		return TRUE;
	}else{
		return FALSE;
	}
}

__declspec (dllexport) __cdecl BOOL kHookUninstall ()
{
	BOOL unHooked = TRUE;
	if (khook.hNotifyWnd != NULL && khook.hHook != NULL)
		unHooked = UnhookWindowsHookEx(khook.hHook);

	khook.hHook = NULL;
	khook.hNotifyWnd = NULL;
	khook.hookState = 0;
	return unHooked;
}

__declspec(dllexport) __cdecl int kHookGetState ()
{
	return (khook.hookState && khook.hHook);
}

__declspec (dllexport) __cdecl void kHookOn ()
{
	khook.hookState = 1;
}

__declspec (dllexport) __cdecl void kHookOff ()
{
	khook.hookState = 0;
}

LRESULT CALLBACK kHookProc (int nCode, WPARAM wParam, LPARAM lParam)
{	
	//printf("%d %d\n", wParam, lParam);
		
	if (nCode == HC_ACTION && khook.hookState){
		LPKBDLLHOOKSTRUCT kbhs = (LPKBDLLHOOKSTRUCT)lParam;

		if (kbhs->vkCode == VK_LSHIFT || kbhs->vkCode == VK_RSHIFT){
			PostMessage(khook.hNotifyWnd, wParam, kbhs->vkCode, (LPARAM)khook.lptr);
			
		}else if (kbhs->vkCode != VK_LWIN && kbhs->vkCode != VK_RWIN && kbhs->vkCode != VK_APPS && kbhs->vkCode != VK_LMENU && kbhs->vkCode != VK_RMENU){
			if (wParam == WM_KEYDOWN){
				PostMessage(khook.hNotifyWnd, WM_KEYDOWN, kbhs->vkCode, (LPARAM)khook.lptr);
				return 1;
			}else if (wParam == WM_KEYUP && kbhs->vkCode != VK_OEM_2 && kbhs->vkCode != VK_RCONTROL && kbhs->vkCode != VK_LCONTROL && kbhs->vkCode != VK_CONTROL){
				return 1;
			}
		}
	}
	return CallNextHookEx(khook.hHook, nCode, wParam, lParam);
}

/*
####################################################################################################
####################################################################################################
####################################################################################################
*/
#if 0
LRESULT CALLBACK sHookProc (int nCode, WPARAM wParam, LPARAM lParam)
{	
	if (1 || nCode == HC_ACTION && shook.hookState){
		SHELLHOOKINFO *sh = (SHELLHOOKINFO*)lParam;
		printf("shellhook: %p %i %i\n", sh->hwnd, (int)nCode, (int)wParam);
		
		//shook.cbPtr(shook.lptr, (int)wParam, 1234, 5678);
		
		PostMessage(shook.hNotifyWnd, WM_ShellHook, wParam, lParam);
		PostMessage(shook.hNotifyWnd, WM_SHELL_HOOK, wParam, lParam);

	}
	return CallNextHookEx(shook.hHook, nCode, wParam, lParam);
}
#endif
/*
####################################################################################################
####################################################################################################
####################################################################################################
*/

#if 0
LRESULT CALLBACK tHookProc (int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0 && thook.hookState){	//HC_ACTION 
		CWPSTRUCT *cwp = (CWPSTRUCT*)lParam;
	
		//if (cwp->hwnd == thook.hSrcWnd){
			printf("%i %p %i %d %d\n", nCode, cwp->hwnd, cwp->message, wParam, lParam);
			PostMessage(thook.hNotifyWnd, WM_TRAY_HOOK, nCode, (LPARAM)cwp);
		//}
	}

	return CallNextHookEx(thook.hHook, nCode, wParam, lParam);
}

#define WINTOOLBAR_NANELENGTH  63

static inline HWND taskbarGetHWND ()
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
				if (!strcmp(classname, "vlcS"))
					return hwndChild2;
				hwndChild2 = GetWindow(hwndChild2, GW_HWNDNEXT);
			}
			return NULL;
		}
		hwndChild = GetWindow(hwndChild, GW_HWNDNEXT);
	}
	return NULL;
}

__declspec (dllexport) __cdecl BOOL tHookInstall (HWND hWnd, void *userPtr)
{
	if (hWnd == NULL)
		return FALSE;

	if (thook.hNotifyWnd != NULL)
		return FALSE;

	thook.hSrcWnd = taskbarGetHWND();
	thook.hHook = SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)tHookProc, thook.hDllInstance, 0);
	if (thook.hHook != NULL){
		thook.hookState = 1;
		thook.lptr = userPtr;
		thook.cbPtr = NULL;
		thook.hNotifyWnd = hWnd;
		
		PostMessage(FindWindow("Shell_TrayWnd", NULL), WM_SIZE, SIZE_RESTORED, 0);
		PostMessage(thook.hSrcWnd, WM_SIZE, SIZE_RESTORED, 0);
		
		return TRUE;
	}else{
		return FALSE;
	}
}

__declspec (dllexport) __cdecl BOOL tHookUninstall ()
{
	BOOL unHooked = TRUE;
	if (thook.hNotifyWnd != NULL && thook.hHook != NULL)
		unHooked = UnhookWindowsHookEx(thook.hHook);

	thook.hHook = NULL;
	thook.hNotifyWnd = NULL;
	thook.hookState = 0;
	return unHooked;
}

#endif
