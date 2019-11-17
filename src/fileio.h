
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



#ifndef _FILEIO_H_
#define _FILEIO_H_


#ifndef QWORD
typedef unsigned __int64 QWORD;
#endif


// HKEY_CURRENT_USER
int regCuGetDword (const char *key, const char *name);
char *regCuGetString (const char *key, const char *name, char *buffer, size_t blen);

int regCuGetDwordW (const wchar_t *key, const wchar_t *name);
wchar_t *regCuGetStringW (const wchar_t *key, const wchar_t *value, wchar_t *buffer, size_t blen);



int shGetFolderPath (unsigned int csidl, wchar_t *path);

wchar_t *getDesktopName (wchar_t *buffer, size_t blen);
wchar_t *getMyDocumentsName (wchar_t *buffer, size_t blen);


// 64bit
int regSetQword (const wchar_t *name, const int64_t value);
int64_t regGetQword (const wchar_t *name);


// 32bit
int regSetDword (const wchar_t *name, const int value);
int regGetDword (const wchar_t *name);

// utf8
int regSetString8 (const char *name, const char *string);
// utf16
int regSetString (const wchar_t *name, const wchar_t *string);

wchar_t *regGetString (const wchar_t *name, wchar_t *buffer, size_t blen);

char *regGetDriveNetworkLocation8 (const int drive);
wchar_t *regGetDriveNetworkLocation (const int drive);

wchar_t *getInstallPath (wchar_t *buffer, size_t blen);
int setInstallPath (wchar_t *buffer, size_t blen);

int writeBin (void *data, const size_t len, const char *path);

#endif