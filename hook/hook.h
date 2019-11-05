// mouse hooking
//
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


#include "../ext.h"


#if 0
#define WM_VLCSTREAMUSER		(WM_USER+2000)
#define WM_MWHEEL_FORWARD		(WM_VLCSTREAMUSER+50)
#define WM_MWHEEL_BACK			(WM_VLCSTREAMUSER+51)
#define WM_MWHEEL_LEFT			(WM_VLCSTREAMUSER+52)
#define WM_MWHEEL_RIGHT			(WM_VLCSTREAMUSER+53)

#define WM_TRAY_HOOK			(WM_VLCSTREAMUSER+100)
#endif


__declspec (dllexport) __cdecl BOOL mHookInstall (HWND hWnd, void *ptr);
__declspec (dllexport) __cdecl BOOL mHookUninstall ();
__declspec (dllexport) __cdecl void mHookOn ();
__declspec (dllexport) __cdecl void mHookOff ();
__declspec (dllexport) __cdecl int mHookGetState ();
__declspec (dllexport) __cdecl BOOL mHookSetCB (HWND hWnd, void *ptr);

__declspec (dllexport) __cdecl BOOL kHookInstall (HWND hWnd, void *ptr);
__declspec (dllexport) __cdecl BOOL kHookUninstall ();
__declspec (dllexport) __cdecl void kHookOn ();
__declspec (dllexport) __cdecl void kHookOff ();
__declspec (dllexport) __cdecl int kHookGetState ();


__declspec (dllexport) __cdecl BOOL tHookInstall (HWND hWnd, void *ptr);
__declspec (dllexport) __cdecl BOOL tHookUninstall ();

