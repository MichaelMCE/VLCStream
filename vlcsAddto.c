

#include <ctype.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include "ext.h"


#define SINGLEINSTANCE_USE_CDS	1
#define my_malloc	malloc
#define my_calloc	calloc
#define my_free		free








// 32 bit magic FNV-1a prime
// Fowler/Noll/Vo hash
#define FNV_32_PRIME ((unsigned int)0x01000193)
#define FNV1_32_INIT ((unsigned int)0x811c9dc5)

static inline unsigned int fnv_32a_buf (const void *buf, const size_t len, unsigned int hval)
{
    unsigned char *restrict bp = (unsigned char*)buf;	/* start of buffer */
    unsigned char *restrict be = bp + len;				/* beyond end of buffer */

    /*
     * FNV-1a hash each octet in the buffer
     */
    while (bp < be){
		/* xor the bottom with the current octet */
		hval ^= (unsigned int)*bp++;

		/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
		hval *= FNV_32_PRIME;
#else
		hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif
    }

    /* return our new hash value */
    return hval;
}

unsigned int generateHash (const void *data, const size_t dlen)
{
	return fnv_32a_buf(data, dlen, FNV_32_PRIME);
}

inline unsigned int getHash (const char *path)
{
	return generateHash(path, strlen(path));
}

static inline int UTF16ToUTF8 (const wchar_t *in, const size_t ilen, char *out, size_t olen)
{
	LPSTR abuf = NULL;
	int len = WideCharToMultiByte(CP_UTF8, 0, in, ilen, NULL, 0,  0, 0);
	if (len > 0)
		abuf = out;
	else
		return 0;

	int ret = WideCharToMultiByte(CP_UTF8, 0, in, ilen, abuf, len,  0, 0);
	if (ret > 0 && ret <= (int)olen)
		out[ret] = 0;

	return (ret > 0);
}

static inline char *UTF16ToUTF8Alloc (const wchar_t *in, const size_t ilen)
{
	const size_t olen = 8 * (ilen+2);
	char *out = my_malloc(1+olen+sizeof(char));
	if (out){
		if (UTF16ToUTF8(in, ilen, out, olen)){
			//out = my_realloc(out, strlen(out)+1);
		}else{
			my_free(out);
			return NULL;
		}
	}
	return out;
}

static inline char *convertto8 (const wchar_t *wide)
{
	char *out = NULL;
	if (wide && *wide){
		out = UTF16ToUTF8Alloc(wide, wcslen(wide));
		if (!out) abort();
	}
	return out;
}

static inline void cds_send_path (HWND hWin, const int op, char *path, const int slen)
{
	COPYDATASTRUCT cds;
	cds.dwData = op;
	cds.lpData = path;
	cds.cbData = slen + 1;
	SendMessage(hWin, WM_COPYDATA, (WPARAM)generateHash(cds.lpData, cds.cbData), (LPARAM)&cds);
}

static inline void cds_send (HWND hWin, const int op, char *path, int len)
{
	if (len > 2){	// fixup a Windows bug
		if (path[len-1] == '\"'){
			path[len-1] = 0;
			len--;
		}
	}

	cds_send_path(hWin, op, path, len);
}

static inline HANDLE getWindowHandle ()
{
	return FindWindowA(NAME_WINMSG, NULL);
}

static inline int singleInstanceCheck ()
{
	HANDLE handle = CreateEvent(NULL, 0, 0, NAME_INSTANCEEVENT);
	int ret = (GetLastError() == ERROR_ALREADY_EXISTS);
	if (handle)
		CloseHandle(handle);
	return ret;
}

static inline void singleInstancePassCmdLine (const int argc, const char *argv[])
{
	HANDLE hWin = getWindowHandle();
	if (hWin){
		PostMessage(hWin, WM_WAKEUP, 0, 0);

		int argcw = 0;
		wchar_t **argvw = CommandLineToArgvW(GetCommandLineW(), &argcw);
		if (argvw){
			HANDLE hEvent = CreateEvent(NULL, 0, 0, NAME_INSTANCEEVENT);
			if (hEvent){
				SendMessage(hWin, WM_WAKEUP, 0, 0);

				if (WaitForSingleObject(hEvent, 5000) == WAIT_OBJECT_0){	// don't hang around if remote thread doesn't wake or respond, try anyways
					int where = WM_CDS_ADDTRACK;
					int startIdx = 1;
					
					//if (argcw > 2){
						if (!wcsicmp(argvw[1], L"dsp")){
							where = WM_CDS_ADDTRACK_DSP;
							startIdx = 2;
						}else if (!wcsicmp(argvw[1], L"ply")){
							where = WM_CDS_ADDTRACK_PLY;
							startIdx = 2;
						}
					//}
					
					for (int i = startIdx; i < argcw; i++){
						char *path = convertto8(argvw[i]);
						if (path){
#if SINGLEINSTANCE_USE_CDS
							cds_send(hWin, where, path, strlen(path));
#else
							const unsigned int hash = getHash(path);
							const int len = strlen(path);
							SendMessage(hWin, WM_ADDTRACKINIT, hash, len+1);
							for (int i = 0; i <= len; i++)
								SendMessage(hWin, WM_ADDTRACKCHAR, hash, (i<<16)|(unsigned char)path[i]);
#endif
							my_free(path);
						}
					}
				}
				CloseHandle(hEvent);
			}
			LocalFree(argvw);
		}
	}
}

int main (const int argc, const char *argv[])
{
	if (argc > 1 && singleInstanceCheck()){
		singleInstancePassCmdLine(argc, argv);
		return 0;
	}
	
	return 1;		
}
