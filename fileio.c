
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
#include "reg.h"


#define MY_DOCUMENTS_LOCALE		MY_DOCUMENTS_2K
#define DESKTOP_LOCALE			DESKTOP_2K


int regCuGetDword (const char *key, const char *name)
{
	HKEY hKey = 0;
	DWORD type = REG_DWORD;
	DWORD value = 0;
	DWORD blen = sizeof(DWORD);
	
	if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, key, 0, KEY_READ, &hKey))){
		RegQueryValueEx(hKey, name, 0, &type, (LPBYTE)&value, (PDWORD)&blen);
		RegCloseKey(hKey);
	}	
	return value;
}

int regCuGetDwordW (const wchar_t *key, const wchar_t *name)
{
	HKEY hKey = 0;
	DWORD type = REG_DWORD;
	DWORD value = 0;
	DWORD blen = sizeof(DWORD);
	
	if ((ERROR_SUCCESS == RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_READ, &hKey))){
		RegQueryValueExW(hKey, name, 0, &type, (LPBYTE)&value, (PDWORD)&blen);
		RegCloseKey(hKey);
	}	
	return value;
}


char *regCuGetString (const char *key, const char *name, char *buffer, size_t blen)
{
	HKEY hKey = 0;
	DWORD type = REG_SZ;
	char *ret = NULL;
	
	if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, key, 0, KEY_READ, &hKey))){
		if ((ERROR_SUCCESS == RegQueryValueEx(hKey, name, 0, &type, (LPBYTE)buffer, (PDWORD)&blen)))
			ret = buffer;
		RegCloseKey(hKey);
	}
	return ret;
}

wchar_t *regCuGetStringW (const wchar_t *key, const wchar_t *value, wchar_t *buffer, size_t blen)
{
	HKEY hKey = 0;
	DWORD type = REG_SZ;
	wchar_t *ret = NULL;
	
	if ((ERROR_SUCCESS == RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_READ, &hKey))){
		if ((ERROR_SUCCESS == RegQueryValueExW(hKey, value, 0, &type, (LPBYTE)buffer, (PDWORD)&blen)))
			ret = buffer;
		RegCloseKey(hKey);
	}
	return ret;
}

wchar_t *regGetDriveNetworkLocation (const int drive)
{
	HKEY hKey = 0;
	DWORD type = REG_SZ;
	wchar_t *ret = NULL;
	wchar_t buffer[MAX_PATH+1];
	size_t blen = MAX_PATH;

	char key[10] = {'N','e','t','w','o','r','k','\\', drive&0xFF, 0};
	const wchar_t *drivew = L"RemotePath";

	if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, key, 0, KEY_READ, &hKey))){
		if ((ERROR_SUCCESS == RegQueryValueExW(hKey, drivew, 0, &type, (LPBYTE)&buffer, (PDWORD)&blen)))
			ret = my_wcsdup(buffer);
		RegCloseKey(hKey);
	}
	return ret;
}

char *regGetDriveNetworkLocation8 (const int drive)
{
	HKEY hKey = 0;
	DWORD type = REG_SZ;
	char *ret = NULL;
	wchar_t buffer[MAX_PATH+1];
	size_t blen = MAX_PATH;

	char key[10] = {'N','e','t','w','o','r','k','\\', drive&0xFF, 0};
	const wchar_t *drivew = L"RemotePath";

	if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, key, 0, KEY_READ, &hKey))){
		if ((ERROR_SUCCESS == RegQueryValueExW(hKey, drivew, 0, &type, (LPBYTE)&buffer, (PDWORD)&blen)))
			ret = convertto8(buffer);
		RegCloseKey(hKey);
	}
	return ret;
}

int regSetString8 (const char *name, const char *string)
{
	int ret = 0;
	DWORD type = REG_SZ;
	HKEY hKey = 0;
	DWORD disp;

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, LIBMYLCDVLCSTREAM, 0, NULL, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &hKey, &disp) == ERROR_SUCCESS){
		ret = RegSetValueEx(hKey, name, 0, type, (BYTE*)string, (DWORD)strlen(string)*sizeof(char)) == ERROR_SUCCESS;
		RegCloseKey(hKey);
	}
	return ret;
}

int regSetString (const wchar_t *name, const wchar_t *string)
{
	int ret = 0;
	DWORD type = REG_SZ;
	HKEY hKey = 0;
	DWORD disp;

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, LIBMYLCDVLCSTREAM, 0, NULL, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &hKey, &disp) == ERROR_SUCCESS){
		ret = RegSetValueExW(hKey, name, 0, type, (BYTE*)string, (DWORD)wcslen(string)*sizeof(wchar_t)) == ERROR_SUCCESS;
		RegCloseKey(hKey);
	}
	return ret;
}

wchar_t *regGetString (const wchar_t *name, wchar_t *buffer, size_t blen)
{
	HKEY hKey = 0;
	DWORD type = REG_SZ;
	wchar_t *ret = NULL;
	
	if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, LIBMYLCDVLCSTREAM, 0, KEY_READ, &hKey))){
		if ((ERROR_SUCCESS == RegQueryValueExW(hKey, name, 0, &type, (LPBYTE)buffer, (PDWORD)&blen)))
			ret = buffer;
		RegCloseKey(hKey);
	}	
	return ret;
}

int regSetQword (const wchar_t *name, const int64_t value)
{
	int ret = 0;
	DWORD type = REG_QWORD;
	HKEY hKey = 0;
	DWORD disp;

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, LIBMYLCDVLCSTREAM, 0, NULL, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &hKey, &disp) == ERROR_SUCCESS){
		ret = RegSetValueExW(hKey, name, 0, type, (BYTE*)&value, sizeof(QWORD)) == ERROR_SUCCESS;
		RegCloseKey(hKey);
	}
	return ret;
}

int64_t regGetQword (const wchar_t *name)
{
	HKEY hKey = 0;
	DWORD type = REG_QWORD;
	QWORD value = 0;
	DWORD blen = sizeof(int64_t);
	
	if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, LIBMYLCDVLCSTREAM, 0, KEY_READ, &hKey))){
		RegQueryValueExW(hKey, name, 0, &type, (LPBYTE)&value, (PDWORD)&blen);
		RegCloseKey(hKey);
	}	
	return value;
}

int regSetDword (const wchar_t *name, const int value)
{
	int ret = 0;
	DWORD type = REG_DWORD;
	HKEY hKey = 0;
	DWORD disp;

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, LIBMYLCDVLCSTREAM, 0, NULL, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &hKey, &disp) == ERROR_SUCCESS){
		ret = RegSetValueExW(hKey, name, 0, type, (BYTE*)&value, (DWORD)sizeof(DWORD)) == ERROR_SUCCESS;
		RegCloseKey(hKey);
	}
	return ret;
}

int regGetDword (const wchar_t *name)
{
	HKEY hKey = 0;
	DWORD type = REG_DWORD;
	DWORD value = 0;
	DWORD blen = sizeof(DWORD);
	
	if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, LIBMYLCDVLCSTREAM, 0, KEY_READ, &hKey))){
		RegQueryValueExW(hKey, name, 0, &type, (LPBYTE)&value, (PDWORD)&blen);
		RegCloseKey(hKey);
	}	
	return value;
}

int setInstallPath (wchar_t *buffer, size_t blen)
{
	return regSetString(L"Path", buffer);
}

wchar_t *getInstallPath (wchar_t *buffer, size_t blen)
{
	return regGetString(L"Path", buffer, blen);
}

wchar_t *getDesktopName (wchar_t *buffer, size_t blen)
{
	HKEY hKey = 0;
	DWORD type = REG_SZ;
	wchar_t *ret = MYDESKTOP;
	
	if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, DESKTOP_LOCALE, 0, KEY_READ, &hKey))){
		if ((ERROR_SUCCESS == RegQueryValueExW(hKey, L"", 0, &type, (LPBYTE)buffer, (PDWORD)&blen)))
			ret = buffer;
		RegCloseKey(hKey);
	}	
	return ret;
}

wchar_t *getMyDocumentsName (wchar_t *buffer, size_t blen)
{
	HKEY hKey = 0;
	DWORD type = REG_SZ;
	wchar_t *ret = MYDOCUMENTS;
	
	if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, MY_DOCUMENTS_LOCALE, 0, KEY_READ, &hKey))){
		if ((ERROR_SUCCESS == RegQueryValueExW(hKey, L"", 0, &type, (LPBYTE)buffer, (PDWORD)&blen)))
			ret = buffer;
		RegCloseKey(hKey);
	}	
	return ret;
}



#define COMCALL0(P,F) (P)->lpVtbl->F(P)
#define COMCALL1(P,F,A1) (P)->lpVtbl->F(P,A1)

void shMallocFree (void * pidl)
{
    IMalloc *pMalloc;

    if (pidl && SUCCEEDED(SHGetMalloc(&pMalloc))){
        COMCALL1(pMalloc, Free, pidl);
        COMCALL0(pMalloc, Release);
    }
}

int shGetFolderPath (unsigned int csidl, wchar_t *path)
{
#if 1
	int result = SHGetFolderPathW(NULL, csidl, NULL, 0, path);
	//wprintf(L"##%s##\n", path);
	return SUCCEEDED(result);
	
#else
    LPITEMIDLIST item;

    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &item))){
        BOOL result = SHGetPathFromIDListW(item, path);
        //wprintf(L"##%s##\n", path);
        shMallocFree(item);
        return FALSE != result;
    }
    return false;
#endif
}

int writeBin (void *data, const size_t len, const char *path)
{
	int ret = 0;
	
	if (!doesFileExist8(path)){
		FILE *fp = fopen(path, "w+b");
		if (fp){
			ret = fwrite(data, len, 1, fp);
			fclose(fp);
		}
	}
	return ret;
}


