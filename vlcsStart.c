

#include <windows.h>
#include <stdio.h>
#include "reg.h"


static inline int startProgram (wchar_t *path)
{
	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));
	
	STARTUPINFOW si;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	
	return CreateProcessW(NULL, path, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
}

static inline wchar_t *getInstallPath (wchar_t *buffer, const size_t blen)
{
	HKEY hKey = 0;
	DWORD type = REG_SZ;
	wchar_t *ret = NULL;
	
	if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, LIBMYLCDVLCSTREAM, 0, KEY_READ, &hKey))){
		if ((ERROR_SUCCESS == RegQueryValueExW(hKey, L"Path", 0, &type, (LPBYTE)buffer, (PDWORD)&blen)))
			ret = buffer;
		RegCloseKey(hKey);
	}	
	return ret;
}

int main (const int argc, const char *argv[])
{
	int ret = 0;
	wchar_t path[MAX_PATH+1];
	
	if (getInstallPath(path, sizeof(path)-1)){
		//wprintf(L"path:'%s'\n",path);
		ret = startProgram(path);
		//printf("ret = %i\n", ret);
	}
	return ret;
}
