
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
#include <wininet.h>

/*#include "sec_api/stdio_s.h"
#include "sec_api/tchar_s.h"
#include "sec_api/wchar_s.h"*/



#define BLUR_MAXOFFSET (16)
#define BLUR_MAXOFFSET_HALF (BLUR_MAXOFFSET/2)

#if (DRAWGLYPHBOUNDS && DRAWSTRINGBOUNDS)
#define PF_EXTRA		(PF_FORCEAUTOWIDTH|PF_GLYPHBOUNDINGBOX|PF_TEXTBOUNDINGBOX)
#elif (DRAWGLYPHBOUNDS)
#define PF_EXTRA		(PF_FORCEAUTOWIDTH|PF_GLYPHBOUNDINGBOX)
#elif (DRAWSTRINGBOUNDS)
#define PF_EXTRA		(PF_FORCEAUTOWIDTH|PF_TEXTBOUNDINGBOX)
#else
#define PF_EXTRA		(PF_FORCEAUTOWIDTH)
#endif




extern int SHUTDOWN;
	
typedef struct{
	ubyte b;
	ubyte g;
	ubyte r;
	ubyte a;
}__attribute__ ((packed))TBGRA;		// 8888

typedef struct {
	union {
		TBGRA bgra;
		int colour;
	}u;
}TCOLOUR4;
//const wchar_t *skins[] = {SKINS};


//#define m_getPixelAddr32(f,x,y)	(f->pixels+(((y)*f->pitch)+((x)<<2)))
//#define getPixel32_NB(f,x,y)	(*(uint32_t*)m_getPixelAddr32((f),(x),(y)))


static const char *extVideoTs8[] = {
	".ts",
	".ps",
	".mts",
	".m2ts",
	""
};

static const char *extVideo8[] = {
	EXTVIDEOA,
	""
};

static const char *extAudio8[] = {
	EXTAUDIOA,
	""
};


static const char *extImage8[] = {
	EXTIMAGEA,
	""
};

static const char *extPlaylists8[] = {
	EXTPLAYLISTSA,
	""
};

static const wchar_t *extVideo[] = {
	EXTVIDEO,
	L""
};

static const wchar_t *extPlaylists[] = {
	EXTPLAYLISTS,
	L""
};

static const wchar_t *extImage[] = {
	EXTIMAGE,
	L""
};

/*
static const wchar_t *extMedia[] = {
	EXTAUDIO,
	EXTVIDEO,
	L""
};*/




//	find and return single line copy of tag value
//	free with my_free()
char *jsonGetTag (char *json, const char *tag)
{
	char *found = stristr(json, tag);
	if (found){
		char *colon = strchr(found+1, ':');
		if (colon){
			colon += 2;
			char *end = strchr(colon, 0x0A);	
			if (end && end - colon > 4){
				*(--end) = 0;
				return my_strdup(colon);
			}
		}
	}
	return NULL;
}

char *getUrl (const char *url, size_t *totalRead)
{
	
	HINTERNET hOpenUrl;
	HINTERNET hSession = InternetOpen("httpGetFile", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
	if (hSession){
		hOpenUrl = InternetOpenUrl(hSession, url, 0, 0, INTERNET_FLAG_IGNORE_CERT_DATE_INVALID|INTERNET_FLAG_RELOAD|INTERNET_FLAG_EXISTING_CONNECT, 0);
		if (!hOpenUrl){
			InternetCloseHandle(hSession);
			return NULL;
		}
	}else{
		return NULL;
	}

	*totalRead = 0;
	const size_t allocStep = 1024;
	size_t allocSize = 10 * allocStep;
	char *buffer = my_calloc(allocSize, sizeof(char));
	
	if (buffer){
		DWORD bread = 0;
		int status = 0;

		do {
			status = InternetReadFile(hOpenUrl, &buffer[*totalRead], allocStep, &bread);
			//printf("status: %i %i %i %i\n", bread, *totalRead, status, allocSize);
			if (bread > 0 && status == 1){
				*totalRead += bread;
				
				if (*totalRead >= allocSize - allocStep){
					allocSize += allocStep;
					buffer = my_realloc(buffer, allocSize);
				}
			}else{
				buffer = my_realloc(buffer, *totalRead);
			}
		}while (buffer && status == 1 && bread > 0);
	}

	InternetCloseHandle(hOpenUrl);
	InternetCloseHandle(hSession);
	
	return buffer;
}

static inline int getPriv (const char *name)
{
	HANDLE hToken;
	LUID seValue;
	TOKEN_PRIVILEGES tkp;

	if (!LookupPrivilegeValue(NULL, name, &seValue) ||
		!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
			return 0;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = seValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	int res = AdjustTokenPrivileges(hToken, 0, &tkp, sizeof(tkp), NULL, NULL);

	CloseHandle(hToken);
	return res;
}

int getShutdownPriv ()
{
	return getPriv(SE_SHUTDOWN_NAME);
}

void workstationLock ()
{
	LockWorkStation();
}

void workstationReboot ()
{
	getShutdownPriv();
	InitiateSystemShutdown(NULL, "Rebooting", 60, 0, 1);
}

void workstationShutdown ()
{
	getShutdownPriv();
	InitiateSystemShutdown(NULL, "Shutting down", 20, 1, 0);
}

void workstationShutdownAbort ()
{
	getShutdownPriv();
	AbortSystemShutdownA(NULL);
}

void workstationLogoff ()
{
	enum ExitFlags {
	    Logoff = 0,
	    Shutdown = 1,
	    Reboot = 2,
	    Force = 4,
	    PowerOff = 8,
	    ForceIfHung = 16
	};

	enum Reason {
    	ApplicationIssue = 0x00040000,
    	HardwareIssue = 0x00010000,
    	SoftwareIssue = 0x00030000,
    	PlannedShutdown = 0x80000000
	};

	getShutdownPriv();
	ExitWindowsEx(Logoff|ForceIfHung, PlannedShutdown);
}

#if 0
int getBatteryLevel ()
{
  SYSTEM_POWER_STATUS SystemPowerStatus;

  if (GetSystemPowerStatus(&SystemPowerStatus) && SystemPowerStatus.BatteryLifePercent != 255)
      return SystemPowerStatus.BatteryLifePercent;

  return 0;
}

int match (const char *str, const char *pat)
{
    for (;;){
		char s = *str, p = *pat;
		if ('*' == p){
			if (s && match(str+1, pat))
                return 1;
            ++pat;
            continue;
        }
        if (0==s) return 0==p;
        if (s>='A' && s<='Z') s+=32;
        if (p>='A' && p<='Z') p+=32;
        if (p != s && p != '?') return 0;
        ++str, ++pat;
    }
}

  /* case-independent string matching, similar to strstr but matching */
char *strcasestr (const char* haystack, const char* needle)
{
    int i;
    int nlength = (int) strlen (needle);
    int hlength = (int) strlen (haystack);

    if (nlength > hlength) return NULL;
    if (hlength <= 0) return NULL;
    if (nlength <= 0) return (char *)haystack;
    /* hlength and nlength > 0, nlength <= hlength */
    for (i = 0; i <= (hlength - nlength); i++){
      if (strncasecmp (haystack + i, needle, nlength) == 0){
        return (char *)haystack + i;
      }
    }
    /* substring not found */
    return NULL;
}


// Adjust the src rectangle so that the dst is always contained in the target rectangle.
void cropSource (TLPOINTEX *src, TLPOINTEX *dst, TLPOINTEX *target)
{
  if(dst->x1 < target->x1)
  {
    src->x1 -= (dst->x1 - target->x1)
            * (src->x2 - src->x1)
            / (dst->x2 - dst->x1);
    dst->x1  = target->x1;
  }
  if(dst->y1 < target->y1)
  {
    src->y1 -= (dst->y1 - target->y1)
            * (src->y2 - src->y1)
            / (dst->y2 - dst->y1);
    dst->y1  = target->y1;
  }
  if(dst->x2 > target->x2)
  {
    src->x2 -= (dst->x2 - target->x2)
            * (src->x2 - src->x1)
            / (dst->x2 - dst->x1);
    dst->x2  = target->x2;
  }
  if(dst->y2 > target->y2)
  {
    src->y2 -= (dst->y2 - target->y2)
            * (src->y2 - src->y1)
            / (dst->y2 - dst->y1);
    dst->y2  = target->y2;
  }
  // Callers expect integer coordinates->
  src->x1 = floor(src->x1);
  src->y1 = floor(src->y1);
  src->x2 = ceil(src->x2);
  src->y2 = ceil(src->y2);
  dst->x1 = floor(dst->x1);
  dst->y1 = floor(dst->y1);
  dst->x2 = ceil(dst->x2);
  dst->y2 = ceil(dst->y2);
}
#endif


/*
int loadImage (TFRAME *frame, const wchar_t *filename)
{
	wprintf(L"loadImage '%s'\n");
	return lLoadImageEx(frame, filename, LOAD_RESIZE|LOAD_PIXEL_CPY|LOAD_SIZE_RESTRICT, 720, 442);
}*/

TFRAME *newImage (TVLCPLAYER *vp, const wchar_t *filename, const int bpp)
{
#if 0
	static volatile int ct = 0;
	static volatile size_t memused = 0;
	
	char *path = convertto8(filename);
	//printf("newImage '%s'\n", path);
	//fflush(stdout);

	const double t0 = getTime(vp);	
	TFRAME *img = lNewImage(vp->ml->hw, filename, bpp);
	if (img){
		const double t1 = getTime(vp);
		memused += img->frameSize + sizeof(TFRAME) + sizeof(void*) + sizeof(TPIXELPRIMITVES);
		
		printf("newImage %.1f (%.2f) - %i: %u %ix%i '%s'\n", t1, t1-t0, ++ct, memused/1024, img->width, img->height, path);
	}else{
		printf("newImage FAILED '%s'\n", path);
	}
	if (path)
		my_free(path);
	return img;
#else
	return lNewImage(vp->ml->hw, filename, bpp);
#endif
}


int isPlaylistW (const wchar_t *path)
{
	return hasPathExt(path, extPlaylists);
}

int isDVDLocation (const char *path)
{
	if (stristr(path, "video_ts"))
		return 1;
	else if (stristr(path, "video ts"))
		return 1;
	else if (stristr(path, "VIDEO_TS"))
		return 1;
	else if (stristr(path, "VIDEO TS"))
		return 1;
	else
		return 0;
}

int isDVDLocationW (const wchar_t *path)
{
	if (wcsistr(path, L"video_ts"))
		return 1;
	else if (wcsistr(path, L"video ts"))
		return 1;
	else if (wcsistr(path, L"VIDEO_TS"))
		return 1;
	else if (wcsistr(path, L"VIDEO TS"))
		return 1;
	else
		return 0;
}

// used by TIMER_EPG_UPDATE when deciding if it should search for an EPG
int isTsVideo8 (const char *path8)
{
	return hasPathExtA(path8, extVideoTs8);
}

int isAudioFile8 (const char *path8)
{
	return hasPathExtA(path8, extAudio8);
}

int isMediaAudio8 (const char *name8)
{
	return isAudioFile8(name8);
}

int isMediaPlaylist8 (const char *path8)
{
	return hasPathExtA(path8, extPlaylists8);
}

int isMediaVideo8 (const char *name8)
{
	int isVideo = (hasPathExtA(name8, extVideo8) ||
		isMediaDVD(name8) ||
		isMediaDVB(name8) ||
		isMediaScreen(name8) ||
		isMediaDShow(name8));

	return isVideo;
}

int isMediaImage8 (const char *name8)
{
	return hasPathExtA(name8, extImage8);
}

int isGaminFile8 (const char *name)
{
	const char *exts[] = {".tcx",""};
	return hasPathExtA(name, exts);
}

int isAyFile8 (const char *name)
{
	const char *exts[] = {".ay",""};
	return hasPathExtA(name, exts);
}

int isRadioStream (wchar_t *path)
{
	return (wcsstr(path, L"http://") != NULL);
}

int isVideoFile (wchar_t *path)
{
	if (hasPathExt(path, extVideo))
		return 1;
	else if (hasPathExt(path, extImage))
		return 1;
	else if (isDVDLocationW(path))
		return 1; 
	else if (isRadioStream(path))
		return 0;
	else
		return (wcsstr(path, L"://") != NULL);
}

int isMediaVideo (wchar_t *name)
{
	int isVideo = 0;
	char *name8 = convertto8(name);
	if (name8){
		isVideo = isMediaVideo8(name8);
		my_free(name8);
	}
	return isVideo;
}


#if 1
int hasPathExtA (const char *path, const char **restrict exts)
{
	const char *fileExt = strrchr(path, '.');
	if (!fileExt) return 0;
	
	for (int i = 0; *exts[i] && *exts[i] == '.'; i++){
		if (!stricmp(fileExt, exts[i]))
			return 1;
	}
	return 0;
}
#else
int hasPathExtA (const char *path, const char **restrict exts)
{
	const int slen = strlen(path);
	int elen;
		
	for (int i = 0; *exts[i] && *exts[i] == '.'; i++){
		elen = strlen(exts[i]);
		if (elen > slen) continue;
		if (!stricmp(path+slen-elen, exts[i]))
			return 1;
	}
	return 0;
}
#endif

int hasPathExt (const wchar_t *in, const wchar_t **restrict exts)
{
	if (!exts) return 1;
	if (!in) return 0;

	wchar_t *path = (wchar_t*)in;
	wchar_t *pathfull = my_wcsdup(in);
	if (!pathfull) return 0;
	
	if (stripOptionsW(pathfull)){
		removeTrailingSpacesW(pathfull);
		path = removeLeadingSpacesW(pathfull);
	}
		
	const int slen = wcslen(path);
	int elen;
	
	for (int i = 0; exts[i] && *exts[i] == L'.'; i++){
		elen = wcslen(exts[i]);
		if (elen > slen) continue;
		if (!wcsicmp(path+slen-elen, exts[i])){
			my_free(pathfull);
			return 1;
		}
	}
	my_free(pathfull);
	return 0;
}

char *sortIdxToStr (const int idx)
{
	switch (idx){
	  case SORT_NOSORT: return "unsorted";
	  case SORT_NAME_A: return "namea";
	  case SORT_NAME_D: return "named";
	  case SORT_DATE_MODIFIED_A: return "modifieda";
	  case SORT_DATE_MODIFIED_D: return "modifiedd";
	  case SORT_DATE_CREATION_A: return "createda";
	  case SORT_DATE_CREATION_D: return "createdd";
	  case SORT_SIZE_FILE_A: return "sizea";
	  case SORT_SIZE_FILE_D: return "sized";
	  case SORT_FILE_TYPE_A: return "typea";
	  case SORT_FILE_TYPE_D: return "typed";
	  default: return "namea";
	}
}

int hkModiferToKey (const char *name)
{
	int key = 0;
	if (!stricmp(name, "ctrl") || !stricmp(name, "control"))
		key = VK_CONTROL;
	else if (!stricmp(name, "alt"))
		key = VK_MENU;
	else if (!stricmp(name, "shift"))
		key = VK_SHIFT;
	return key;
}

int sortStrToIdx (const char *name)
{
	int sort;
	
	if (!stricmp(name, "none") || !stricmp(name, "unsorted"))
		sort = SORT_NOSORT;
	else if (!stricmp(name, "namea"))
		sort = SORT_NAME_A;
	else if (!stricmp(name, "named"))
		sort = SORT_NAME_D;
	else if (!stricmp(name, "modifieda"))
		sort = SORT_DATE_MODIFIED_A;
	else if (!stricmp(name, "modifiedd"))
		sort = SORT_DATE_MODIFIED_D;
	else if (!stricmp(name, "createda"))
		sort = SORT_DATE_CREATION_A;
	else if (!stricmp(name, "createdd"))
		sort = SORT_DATE_CREATION_D;
	else if (!stricmp(name, "sizea"))
		sort = SORT_SIZE_FILE_A;
	else if (!stricmp(name, "sized"))
		sort = SORT_SIZE_FILE_D;
	else if (!stricmp(name, "typea"))
		sort = SORT_FILE_TYPE_A;
	else if (!stricmp(name, "typed"))
		sort = SORT_FILE_TYPE_D;
	else
		sort = SORT_NAME_A;
				
	return sort;
}

wchar_t *filterIdxToStrW (const int idx)
{
	if (idx == FILEMASKS_AUDIO)
		return L"audio";
	else if (idx == FILEMASKS_VIDEO)
		return L"video";
	else if (idx == FILEMASKS_PLAYLISTS)
		return L"playlist";
	else if (idx == FILEMASKS_IMAGE)
		return L"image";
	else if (idx == FILEMASKS_MEDIA)
		return L"media";
	else if (idx == FILEMASKS_ALL)
		return L"all";
	else
		return L"audio";
}

char *filterIdxToStr (const int idx)
{
	if (idx == FILEMASKS_AUDIO)
		return "audio";
	else if (idx == FILEMASKS_VIDEO)
		return "video";
	else if (idx == FILEMASKS_PLAYLISTS)
		return "playlist";
	else if (idx == FILEMASKS_IMAGE)
		return "image";
	else if (idx == FILEMASKS_MEDIA)
		return "media";
	else if (idx == FILEMASKS_ALL)
		return "all";
	else
		return "audio";
}
	
int filterStrToIdx (const char *name)
{
	int filter;
	
	if (!stricmp(name, "audio"))
		filter = FILEMASKS_AUDIO;
	else if (!stricmp(name, "video"))
		filter = FILEMASKS_VIDEO;
	else if (!stricmp(name, "playlists") || !stricmp(name, "playlist"))
		filter = FILEMASKS_PLAYLISTS;
	else if (!stricmp(name, "image"))
		filter = FILEMASKS_IMAGE;
	else if (!stricmp(name, "media"))
		filter = FILEMASKS_MEDIA;
	else if (!stricmp(name, "all"))
		filter = FILEMASKS_ALL;
	else
		filter = FILEMASKS_DEFAULT;
			
	return filter;
}

const char *aspectIdxToStr (const int idx)
{
	switch (idx+BTN_CFG_AR_AUTO){
	  case BTN_CFG_AR_AUTO:
		return "auto";
	  case BTN_CFG_AR_CUSTOM:
		return "custom";
	  case BTN_CFG_AR_177:
		return "16:9";
	  case BTN_CFG_AR_155:
		return "14:9";
	  case BTN_CFG_AR_133:
		return "4:3";
	  case BTN_CFG_AR_143:
		return "43:30";
	  case BTN_CFG_AR_125:
		return "5:4";
	  case BTN_CFG_AR_122:
		return "22:18";
	  case BTN_CFG_AR_15:
		return "3:2";
	  case BTN_CFG_AR_16:
		return "16:10";
	  case BTN_CFG_AR_167:
		return "5:3";
	  case BTN_CFG_AR_185:
		return "3:7";
	  case BTN_CFG_AR_220:
		return "11:5";
	  case BTN_CFG_AR_233:
		return "47:20";
	  case BTN_CFG_AR_240:
		return "12:5";
	  default :
	  	return "auto";
	};
}

int aspectStrToIdx (const char *str)
{
	int ar = 0;
	
	if (!stricmp(str, "auto")){
		ar = BTN_CFG_AR_AUTO;

	}else if (!stricmp(str, "custom")){
		ar = BTN_CFG_AR_CUSTOM;
		
	}else if (!strcmp(str, "16:9") || !strcmp(str, "1.77")){
		ar = BTN_CFG_AR_177;

	}else if (!strcmp(str, "14:9") || !strcmp(str, "1.55")){
		ar = BTN_CFG_AR_155;

	}else if (!strcmp(str, "4:3") || !strcmp(str, "1.33")){
		ar = BTN_CFG_AR_133;

	}else if (!strcmp(str, "43:30") || !strcmp(str, "1.43")){
		ar = BTN_CFG_AR_143;
		
	}else if (!strcmp(str, "5:4") || !strcmp(str, "1.25")){
		ar = BTN_CFG_AR_125;

	}else if (!strcmp(str, "22:18") || !strcmp(str, "1.22")){
		ar = BTN_CFG_AR_122;

	}else if (!strcmp(str, "3:2") || !strcmp(str, "1.5") || !strcmp(str, "1.50")){
		ar = BTN_CFG_AR_15;

	}else if (!strcmp(str, "16:10") || !strcmp(str, "1.6") || !strcmp(str, "1.60")){
		ar = BTN_CFG_AR_16;

	}else if (!strcmp(str, "5:3") || !strcmp(str, "1.67")){
		ar = BTN_CFG_AR_167;

	}else if (!strcmp(str, "3:7") || !strcmp(str, "1.85")){
		ar = BTN_CFG_AR_185;

	}else if (!strcmp(str, "11:5") || !strcmp(str, "2.2") || !strcmp(str, "2.20")){
		ar = BTN_CFG_AR_220;

	}else if (!strcmp(str, "47:20") || !strcmp(str, "21:9") || !strcmp(str, "2.33") || !strcmp(str, "2.35")){
		ar = BTN_CFG_AR_233;

	}else if (!strcmp(str, "12:5") || !strcmp(str, "2.39") || !strcmp(str, "2.4") || !strcmp(str, "2.40")){
		ar = BTN_CFG_AR_240;
		
	}else{
		ar = BTN_CFG_AR_AUTO;
	}

	ar -= BTN_CFG_AR_AUTO;
	return ar;
}

wchar_t *buildSkinDEx (TVLCPLAYER *vp, wchar_t *buffer, wchar_t *dir, wchar_t *file)
{
	*buffer = 0;

	wchar_t *skin = NULL;
	if (!*vp->gui.skin.folder){
		settingsGetW(vp, "skin.folder", &skin);
		
		if (skin){
			wcscpy(vp->gui.skin.folder, skin);
			my_free(skin);
		}
	}
	skin = vp->gui.skin.folder;
	__mingw_swprintf(buffer, L"%ls/%ls/%ls/%ls", SKINDROOT, skin, dir, file);
	return buffer;
}

wchar_t *buildSkinD (TVLCPLAYER *vp, wchar_t *buffer, wchar_t *file)
{
	*buffer = 0;
	//wchar_t *skin = vp->gui.skin.dir;
	
	wchar_t *skin = NULL;
	if (!*vp->gui.skin.folder){
		settingsGetW(vp, "skin.folder", &skin);
		
		if (skin){
			wcscpy(vp->gui.skin.folder, skin);
			my_free(skin);
		}
	}
	skin = vp->gui.skin.folder;
	
	//if (skin){
		//if (skin == NULL) skin = SKINDEFAULTW;

		__mingw_swprintf(buffer, L"%ls/%ls/%ls", SKINDROOT, skin, file);
		//wprintf(L"skin #%s#\n", skin);
		//my_free(skin);
	//}else{
	//	printf("buildSkinD: skin directory setting invalid or not found\n");
	//}
	return buffer;
}

void imageBestFit (const int bg_w, const int bg_h, int fg_w, int fg_h, int *w, int *h)
{
	const int fg_sar_num = 1; const int fg_sar_den = 1;
	const int bg_sar_den = 1; const int bg_sar_num = 1;

	if (fg_w < 1 || fg_w > 8191) fg_w = bg_w;
	if (fg_h < 1 || fg_h > 8191) fg_h = bg_h;
	*w = bg_w;
	*h = (bg_w * fg_h * fg_sar_den * bg_sar_num) / (float)(fg_w * fg_sar_num * bg_sar_den);
	if (*h > bg_h){
		*w = (bg_h * fg_w * fg_sar_num * bg_sar_den) / (float)(fg_h * fg_sar_den * bg_sar_num);
		*h = bg_h;
	}
}


// replace char a with b
void strchrreplace (char *str, const char a, const char b)
{
	while(*str){
		if (*str == a) *str = b;
		str++;
	}
}

char *stristr (const char *string, const char *pattern)
{
	if (!string || !pattern) return NULL;
	
	char *pptr, *sptr, *start;
	
	for (start = (char*)string; *start != 0; start++){
		for ( ; ((*start != 0) && (toupper(*start) != toupper(*pattern))); start++){
		}
		
		if (!*start) return NULL;

		pptr = (char*)pattern;
		sptr = (char*)start;

		while (toupper(*sptr) == toupper(*pptr)){
			sptr++;
			pptr++;
			if (!*pptr)
				return (start);
		}
	}
	return NULL;
}

wchar_t *wcsistr (const wchar_t *String, const wchar_t *Pattern)
{
	if (!String) return NULL;
	
	wchar_t *pptr, *sptr, *start;
	for (start = (wchar_t *)String; *start != 0; start++){
		for ( ; ((*start != 0) && (towupper(*start) != towupper(*Pattern))); start++)
			;
		if (0 == *start)
			return NULL;

		pptr = (wchar_t *)Pattern;
		sptr = (wchar_t *)start;

		while (towupper(*sptr) == towupper(*pptr)){
			sptr++;
			pptr++;
			if (0 == *pptr)
				return (start);
		}
	}
	return NULL;
}


static inline int findLastCharIndex8 (char *str, const unsigned char Char)
{
	int i = -1;
	int found = -1;
	
	while (*str){
		i++;
		if (*str == Char) found = i;
		
		str++;
	};
	
	return found;
}

static inline int findLastCharIndex (wchar_t *str, const wchar_t Char)
{
	int i = 0;
	int found = -1;
	
	while (*str){
		if (*str == Char) found = i;
		
		i++;
		str++;
	};
	
	return found;
}


static inline wchar_t *ellipsiizeString (wchar_t *String, int DesiredCount)
{
	const int slen = wcslen(String);
    if (slen <= DesiredCount || DesiredCount < 3){
        return my_wcsdup(String);
        
    }else{
        wchar_t *string;

        string = my_calloc(DesiredCount+2, sizeof(wchar_t));
        memcpy(string, String, (DesiredCount - 3) * 2);
        memcpy(&string[DesiredCount - 3], L"...", 6);

        return string;
    }
}

wchar_t *ellipsiizeStringPath (wchar_t *String, int DesiredCount)
{
    int secondPartIndex = findLastCharIndex(String, L'\\');

	if (secondPartIndex == -1)
		secondPartIndex = findLastCharIndex(String, L'/');
	if (secondPartIndex == -1)
		return ellipsiizeString(String, DesiredCount);

	const int slen = wcslen(String);
	//printf("secondPartIndex %i %i\n", secondPartIndex, slen);
	
    if (slen <= DesiredCount || DesiredCount < 3){
        return my_wcsdup(String);
        
    }else{
        wchar_t *string;
        int firstPartCopyLength;
        int secondPartCopyLength;

        string = my_calloc(DesiredCount+2, sizeof(wchar_t));
        secondPartCopyLength = slen - secondPartIndex;

        // Check if we have enough space for the entire second part of the string.
        if (secondPartCopyLength + 3 <= DesiredCount){
            // Yes, copy part of the first part and the entire second part.
            firstPartCopyLength = DesiredCount - secondPartCopyLength - 3;
            
        }else{
            // No, copy part of both, from the beginning of the first part and
            // the end of the second part.
            firstPartCopyLength = (DesiredCount - 3) / 2;
            secondPartCopyLength = DesiredCount - 3 - firstPartCopyLength;
            secondPartIndex = slen - secondPartCopyLength;
        }

        my_memcpy(
            string,
            String,
            firstPartCopyLength * 2
            );
        my_memcpy(
            &string[firstPartCopyLength],
            L"...",
            6
            );
        my_memcpy(
            &string[firstPartCopyLength + 3],
            &String[secondPartIndex],
            secondPartCopyLength * 2
            );

        return string;
    }
    
    return NULL;
}

int UTF8ToUTF16 (const char *in, const size_t ilen, wchar_t *out, size_t olen)
{
	LPWSTR abuf = NULL;
	int len = MultiByteToWideChar(CP_UTF8, 0, in, ilen, NULL, 0);
	if (len > 0)
		abuf = out;
	else
		return 0;

	int ret = MultiByteToWideChar(CP_UTF8, 0, in, ilen, abuf, sizeof(wchar_t)*(len+1));
	if (ret > 0 && ret <= olen){
		out[ret] = 0;
		return ret;	
	}
	return 0;
}

int UTF16ToUTF8 (const wchar_t *in, const size_t ilen, char *out, size_t olen)
{
	LPSTR abuf = NULL;
	int len = WideCharToMultiByte(CP_UTF8, 0, in, ilen, NULL, 0,  0, 0);
	if (len > 0)
		abuf = out;
	else
		return 0;

	int ret = WideCharToMultiByte(CP_UTF8, 0, in, ilen, abuf, len,  0, 0);
	if (ret > 0 && ret <= olen){
		out[ret] = 0;
	}	

	return (ret > 0);
}

static inline wchar_t *UTF8ToUTF16Alloc (const char *in, const size_t ilen)
{
	const size_t olen = sizeof(wchar_t) * (ilen+2);
	wchar_t *out = my_malloc(2+olen+sizeof(wchar_t));
	if (out){
		if (UTF8ToUTF16(in, ilen, out, olen)){
			//out = my_realloc(out, (wcslen(out)+1) * sizeof(wchar_t));
		}else{
			my_free(out);
			return NULL;
		}
	}
	return out;
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

wchar_t *converttow (const char *utf8)
{
	wchar_t *out = NULL;
	if (utf8 && *utf8){
		out = UTF8ToUTF16Alloc(utf8, strlen(utf8));
		if (!out){
			//out = my_wcsdup(L"-");
			//if (!out){
				printf("no memory, bailing\n");
				abort();
			//}
		}else{
			//wprintf(L"%p '%s'\n", out, out);
		}
	}
	return out;
}

char *convertto8 (const wchar_t *wide)
{
	char *out = NULL;
	if (wide && *wide){
		out = UTF16ToUTF8Alloc(wide, wcslen(wide));
		if (!out){
			//out = my_strdup("-");
			//if (!out){
				printf("no memory, bailing\n");
				abort();
			//}
		}
	}
	return out;
}

void copyAreaNoBlend (TFRAME *from, TFRAME *to, int dx, int dy, int x1, int y1, int x2, int y2)
{
	if (dx < 0){
		x1 += abs(dx);
		dx = 0;
	}
	if (dy < 0){
		y1 += abs(dy);
		dy = 0;
	}
	if (x2 > from->width-1) x2 = from->width-1;
	if (y2 > from->height-1) y2 = from->height-1;
	
	const int w = (x2-x1)+1;
	if (dx+w >= to->width) x2 -= (dx+w - to->width);
	const int h = (y2-y1)+1;
	if (dy+h >= to->height) y2 -= (dy+h - to->height);
	
	int xx;
	int *restrict psrc; int *restrict pdes;
	 
	for (int y = y1; y <= y2; y++, dy++){
		psrc = lGetPixelAddress(from, 0, y);
		pdes = lGetPixelAddress(to, 0, dy);
		xx = dx;
		for (int x = x1; x <= x2; x++,xx++)
			pdes[xx] = psrc[x];
	}
}

TFRAME *newStringEx3 (THWD *hw, const TMETRICS *metrics, const int bpp, const int flags, const int font, const char *text, int x, int y, const int maxW, const int maxH)
{
	if (!*text) return NULL;

	TFRAME *str = lNewStringEx(hw, metrics, bpp, flags|/*PF_EXTRA|*/PF_IGNOREFORMATTING|PF_CLIPDRAW, font, text);
	//printf("newStringEx %i,%i %i '%s'\n", str->width, str->height, maxW, text);
	if (!str) return NULL;
	
#if 0
	lDrawRectangle(str, 0, 0, str->width-1, str->height-1, 255<<24|COL_YELLOW);
#endif

	int w = maxW;
	int h = maxH;
	
	/*if (y < 0){
		h += y;
		y = abs(y);
	}else{
		y = 0;
	}
	if (x < 0){
		w += x;
		x = abs(x);
	}else{
		x = 0;
	}*/
		
		if (/*y > 0 ||*/ w != str->width || h != str->height){
			TFRAME *tmp = lNewFrame(str->hw, w, h, str->bpp);
			if (tmp){
				//if (nsex_flags&NSEX_LEFT)
					copyAreaNoBlend(str, tmp, (abs(w-str->width)/2)+x, (abs(h-str->height)/2)+y, 0, 0, str->width-1, str->height-1);
				//else if (nsex_flags&NSEX_RIGHT)
				//	copyAreaNoBlend(str, tmp, 0, 0, str->width-w, 0, str->width-1, tmp->height-1);
					
				lDeleteFrame(str);
				str = tmp;
			}
		}
		//lDrawRectangle(str, 0, 0, str->width-1, str->height-1, 0xFF0000FF);
	return str;
}


TFRAME *newStringList (THWD *hw, const TMETRICS *metrics, const int bpp, const int flags, const int font, const unsigned int *glist, const int gtotal, int x, int y, const int maxW, const int maxH, const int nsex_flags)
{
	if (!glist || !gtotal){
		//printf("newStringEx: invalid string\n");
		return NULL;
	}
	
	TFRAME *str = lNewStringListEx(hw, metrics, bpp, flags|PF_EXTRA|PF_IGNOREFORMATTING|PF_CLIPDRAW/*|PF_MIDDLEJUSTIFY|PF_WORDWRAP|PF_CLIPWRAP*/, font, glist, gtotal);
	//printf("newStringEx %i,%i %i '%s'\n", str->width, str->height, maxW, text);
	if (!str) return NULL;
	
	//lSaveImage(str, L"tmp.png", IMG_PNG|IMG_KEEPALPHA, 0, 0);
	
	int w = MIN(str->width, maxW);
	int h = MIN(str->height, maxH);
	
	if (y < 0){
		h += y;
		y = abs(y);
	}else{
		y = 0;
	}
	if (x < 0){
		w += x;
		x = abs(x);
	}else{
		x = 0;
	}	
		
	if (y > 0 || w != str->width || h != str->height){
		TFRAME *tmp = lNewFrame(str->hw, w, h, str->bpp);
		if (tmp){
			if (nsex_flags&NSEX_LEFT)
				copyAreaNoBlend(str, tmp, 0, 0, x, y, str->width-1, str->height-1);
			else if (nsex_flags&NSEX_RIGHT)
				copyAreaNoBlend(str, tmp, 0, 0, str->width-w, 0, str->width-1, tmp->height-1);
				
			lDeleteFrame(str);
			str = tmp;
		}
	}
	
	//lDrawRectangle(str, 0, 0, str->width-1, str->height-1, 0xFF0000FF);
	
	return str;
}


TFRAME *newStringEx2 (THWD *hw, const TMETRICS *metrics, const int bpp, const int flags, const int font, const char *text, int x, int y, const int maxW, const int maxH, const int nsex_flags)
{
	if (!*text){
		//printf("newStringEx: invalid string\n");
		return NULL;
	}
	
	TFRAME *str = lNewStringEx(hw, metrics, bpp, flags|PF_EXTRA|PF_IGNOREFORMATTING|PF_CLIPDRAW/*|PF_MIDDLEJUSTIFY|PF_WORDWRAP|PF_CLIPWRAP*/, font, text);
	//printf("newStringEx %i,%i %i '%s'\n", str->width, str->height, maxW, text);
	if (!str) return NULL;
	
	//lSaveImage(str, L"tmp.png", IMG_PNG|IMG_KEEPALPHA, 0, 0);
	
	int w = MIN(str->width, maxW);
	int h = MIN(str->height, maxH);
	
	if (y < 0){
		h += y;
		y = abs(y);
	}else{
		y = 0;
	}
	if (x < 0){
		w += x;
		x = abs(x);
	}else{
		x = 0;
	}	
		
	if (y > 0 || w != str->width || h != str->height){
		TFRAME *tmp = lNewFrame(str->hw, w, h, str->bpp);
		if (tmp){
			if (nsex_flags&NSEX_LEFT)
				copyAreaNoBlend(str, tmp, 0, 0, x, y, str->width-1, str->height-1);
			else if (nsex_flags&NSEX_RIGHT)
				copyAreaNoBlend(str, tmp, 0, 0, str->width-w, 0, str->width-1, tmp->height-1);
				
			lDeleteFrame(str);
			str = tmp;
		}
	}
	
	//lDrawRectangle(str, 0, 0, str->width-1, str->height-1, 0xFF0000FF);
	
	return str;
}

TFRAME *newStringEx (THWD *hw, TMETRICS *metrics, const int bpp, const int flags, const int font, const char *text, const int maxW, const int nsex_flags)
{
	if (!*text){
		//printf("newStringEx: invalid string\n");
		return NULL;
	}

	metrics->width = maxW;
	TFRAME *str = lNewStringEx(hw, metrics, bpp, flags|PF_EXTRA|PF_IGNOREFORMATTING|PF_CLIPDRAW, font, text);
	//printf("newStringEx %i,%i %i '%s'\n", str->width, str->height, maxW, text);
	
	if (str){
		if (str->width > maxW){
			TFRAME *tmp = lNewFrame(str->hw, maxW, str->height, str->bpp);
			if (tmp){
				if (nsex_flags&NSEX_LEFT)
					copyAreaNoBlend(str, tmp, 0, 0, 0, 0, tmp->width-1, tmp->height-1);
				else if (nsex_flags&NSEX_RIGHT)
					copyAreaNoBlend(str, tmp, 0, 0, str->width-maxW, 0, str->width-1, str->height-1);
					
				lDeleteFrame(str);
				str = tmp;
			}
		}
		//lDrawRectangle(str, 0, 0, str->width-1, str->height-1, 0xFF0000FF);
	}
	return str;
}

TFRAME *newStringListEx (THWD *hw, const int bpp, const int flags, const int font, const unsigned int *glist, const int gtotal, const int maxW, const int nsex_flags)
{
	//printf("newStringList %p %i, %i\n", glist, gtotal, *glist);
	//printf("%p\n", glist);


	TFRAME *str = lNewStringList(hw, bpp, flags|PF_EXTRA|PF_CLIPDRAW, font, glist, gtotal);
	if (str){
		if (str->width > maxW){
			TFRAME *tmp = lNewFrame(str->hw, maxW, str->height, str->bpp);
			if (tmp){
				if (nsex_flags&NSEX_LEFT)
					copyAreaNoBlend(str, tmp, 0, 0, 0, 0, tmp->width-1, str->height-1);
				else if (nsex_flags&NSEX_RIGHT)
					copyAreaNoBlend(str, tmp, 0, 0, str->width-maxW, 0, str->width-1, str->height-1);
					
				lDeleteFrame(str);
				str = tmp;
			}
		}
		//lDrawRectangle(str, 0, 0, str->width-1, str->height-1, 0xFF0000FF);
	}
	return str;
}

void outlineTextEnable (THWD *hw, const int colour)
{
	lSetFilterAttribute(hw, LTR_OUTLINE2, 0, colour);
	lSetRenderEffect(hw, LTR_OUTLINE2);
}

void outlineTextDisable (THWD *hw)
{
	lSetRenderEffect(hw, LTR_DEFAULT);
}

void shadowTextEnable (THWD *hw, const int colour, const unsigned char trans)
{
	lSetRenderEffect(hw, LTR_SHADOW);
	// set direction to South-East, shadow thickness to 5, offset by 1 pixel(s) and transparency to trans
	lSetFilterAttribute(hw, LTR_SHADOW, 0, LTRA_SHADOW_S|LTRA_SHADOW_E|LTRA_SHADOW_N|LTRA_SHADOW_W | LTRA_SHADOW_S5 | LTRA_SHADOW_OS(0) | LTRA_SHADOW_TR(trans));
	lSetFilterAttribute(hw, LTR_SHADOW, 1, colour);
}

void shadowTextDisable (THWD *hw)
{
	lSetRenderEffect(hw, LTR_DEFAULT);
}

void printSingleLineShadow (TFRAME *frame, const int font, const int x, const int y, const int foreCol, const int backCol, const char *str)
{
	TLPRINTR rt;
	memset(&rt, 0, sizeof(rt));
	int w, h;
	
	int col = lSetForegroundColour(frame->hw, 244<<24|foreCol);
	shadowTextEnable(frame->hw, backCol, 230/*(backCol>>24)&0xFF*/);
	lGetTextMetrics(frame->hw, str, 0, font, &w, &h);
		
	if (w >= frame->width){
		rt.bx1 = 0; rt.by1 = y;
		rt.bx2 = frame->width-1;
		rt.by2 = frame->height-1;
		rt.sx = rt.bx1; rt.sy = y;
		lPrintEx(frame, &rt, font, PF_EXTRA|PF_CLIPDRAW|PF_RIGHTJUSTIFY|PF_IGNOREFORMATTING, LPRT_CPY, str);
	}else{
		rt.bx1 = (frame->width - w)/2;
		rt.by1 = y;
		rt.bx2 = frame->width-1;
		rt.by2 = frame->height-1;
		rt.sx = rt.bx1; rt.sy = y;
		lPrintEx(frame, &rt, font, PF_EXTRA|PF_CLIPWRAP|PF_CLIPDRAW|PF_IGNOREFORMATTING, LPRT_CPY, str);
	}
	shadowTextDisable(frame->hw);
	lSetForegroundColour(frame->hw, col);
}

int drawStr (TFRAME *frame, const int x, const int y, const int font, const int colour, const char *str, const int nsex_justify)
{
	TLPRINTR rt;
	
	rt.bx1 = x;
	rt.by1 = y;
	rt.bx2 = (frame->width)-1;
	rt.by2 = (frame->height)-1;
	rt.ex = rt.sx = rt.bx1;
	rt.ey = rt.sy = rt.by1;
	
	int flags = PF_EXTRA|PF_CLIPDRAW|PF_IGNOREFORMATTING|PF_CLIPTEXTH|PF_CLIPTEXTV;
	
	if (nsex_justify == NSEX_RIGHT){
		flags |= PF_CLIPWRAP|PF_RIGHTJUSTIFY;
	}else if (nsex_justify == DS_MIDDLEJUSTIFY){
		flags |= PF_CLIPWRAP|PF_MIDDLEJUSTIFY;
		rt.bx1 = 0;
		rt.ex = rt.sx = rt.bx1;
	}

	lSetForegroundColour(frame->hw, colour);
	return lPrintEx(frame, &rt, font, flags, LPRT_CPY, str);
}

int drawIntStr (TFRAME *frame, const int x, const int y, const int font, const int var, const char *str, const int colour)
{
	TFRAME *img = lNewString(frame->hw, frame->bpp, PF_EXTRA|PF_CLIPDRAW, font, "%i %s", var, str);
	if (img){
		//lDrawRectangleFilled(frame, x, y+1, x+img->width, y+img->height-2, colour);
		drawImage(img, frame, x, y, img->width-1, img->height-1);
		const int w = img->width+1;
		lDeleteFrame(img);
		return w;
	}
	return 0;
}
#if 0
int drawStrB (TFRAME *frame, const int x, const int y, const char *str, const int colour)
{
	int w = 0;
	
	TFRAME *img = lNewString(frame->hw, frame->bpp, PF_EXTRA|PF_CLIPDRAW|PF_IGNOREFORMATTING, MFONT, str);
	if (img){
		if (colour&0xFF000000)
			lDrawRectangleFilled(frame, x, y+1, x+img->width, y+img->height-2, colour);
		drawImage(img, frame, x, y, img->width-1, img->height-1);
		w = img->width+1;
		lDeleteFrame(img);
	}
	return w;	
}

int drawStrIntB (TFRAME *frame, const int x, const int y, const int font, const char *str, const int val)
{
	TLPRINTR rt;
	
	rt.bx1 = x;
	rt.by1 = y;
	rt.bx2 = (frame->width)-1;
	rt.by2 = (frame->height)-1;
	rt.ex = rt.sx = rt.bx1;
	rt.ey = rt.sy = rt.by1;
	
	int flags = PF_EXTRA|PF_CLIPDRAW|PF_CLIPTEXTH|PF_CLIPTEXTV;
	
	/*if (nsex_justify == NSEX_RIGHT){
		flags |= PF_CLIPWRAP|PF_RIGHTJUSTIFY;
	}else if (nsex_justify == DS_MIDDLEJUSTIFY){
		flags |= PF_CLIPWRAP|PF_MIDDLEJUSTIFY;
		rt.bx1 = 0;
		rt.ex = rt.sx = rt.bx1;
	}*/

	//lSetForegroundColour(frame->hw, colour);
	return lPrintEx(frame, &rt, font, flags, LPRT_CPY, "%s%i", str, val);
}

int drawStrFltB (TFRAME *frame, const int x, const int y, const int font, const char *str, const double val)
{
	TLPRINTR rt;
	
	rt.bx1 = x;
	rt.by1 = y;
	rt.bx2 = (frame->width)-1;
	rt.by2 = (frame->height)-1;
	rt.ex = rt.sx = rt.bx1;
	rt.ey = rt.sy = rt.by1;
	
	int flags = PF_EXTRA|PF_CLIPDRAW|PF_CLIPTEXTH|PF_CLIPTEXTV;
	
	/*if (nsex_justify == NSEX_RIGHT){
		flags |= PF_CLIPWRAP|PF_RIGHTJUSTIFY;
	}else if (nsex_justify == DS_MIDDLEJUSTIFY){
		flags |= PF_CLIPWRAP|PF_MIDDLEJUSTIFY;
		rt.bx1 = 0;
		rt.ex = rt.sx = rt.bx1;
	}*/

	//lSetForegroundColour(frame->hw, colour);
	return lPrintEx(frame, &rt, font, flags, LPRT_CPY, "%s%.1f", str, val);
}
#endif

int drawStrFlt (TFRAME *frame, const int x, const int y, const char *str, const double var, const int colour)
{
	int w = 0;
	
	TFRAME *img = lNewString(frame->hw, frame->bpp, PF_EXTRA|PF_CLIPDRAW, MFONT, " %s%.1f", str, var);
	if (img){
		if (colour&0xFF000000)
			lDrawRectangleFilled(frame, x, y+1, x+img->width, y+img->height-2, colour);
		drawImage(img, frame, x, y, img->width-1, img->height-1);
		w = img->width+1;
		lDeleteFrame(img);
	}
	return w;	
}

int drawStrInt (TFRAME *frame, const int x, const int y, const char *str, const int var, const int colour)
{
	int w = 0;
	
	TFRAME *img = lNewString(frame->hw, frame->bpp, PF_EXTRA|PF_CLIPDRAW, MFONT, " %s%i", str, var);
	if (img){
		if (colour&0xFF000000)
			lDrawRectangleFilled(frame, x, y+1, x+img->width, y+img->height-2, colour);
		drawImage(img, frame, x, y, img->width-1, img->height-1);
		w = img->width+1;
		lDeleteFrame(img);
	}
	return w;	
}

void drawHex (TFRAME *frame, const int x, const int y, const int font, const int var, const int colour)
{
	int x2 = x;
	if (var < 10)		// because its faster..
		x2 += 8;
	else if (var < 100)
		x2 += 15;
	else if (var < 1000)
		x2 += 22;
	else if (var < 10000)
		x2 += 29;
	else
		x2 += 36;
		
	if (colour&0xFF000000)
		lDrawRectangleFilled(frame, x-3, y+2, x2, y+14, colour/*90<<24 | (colour&0xFFFFFF)*/);
	lPrintf(frame, x, y, font, LPRT_CPY, "%X", var);
}

// rename to drawDec
void drawInt (TFRAME *frame, const int x, const int y, const int var, const int colour)
{
	int x2 = x;
	if (var < 10)
		x2 += 8;
	else if (var < 100)
		x2 += 15;
	else if (var < 1000)
		x2 += 22;
	else if (var < 10000)
		x2 += 29;
	else
		x2 += 36;
		
	lDrawRectangleFilled(frame, x-3, y+2, x2, y+14, colour/*90<<24 | (colour&0xFFFFFF)*/);
	lPrintf(frame, x, y, MFONT, LPRT_CPY, "%i", var);
}

#if 0
//slower, higher quality
static inline unsigned int ablend (const unsigned int des, const unsigned int src)
{

	const unsigned int srca = (src>>24)&0xFF;
	if (srca == 255) return src;
			
	const unsigned int srcr = (src>>16)&0xFF;
	unsigned int dstr = (des>>16)&0xFF;
	dstr = (srca * (srcr - dstr) + (dstr<<8))>>8;
	
	const unsigned int srcg = (src>>8)&0xFF;
	unsigned int dstg = (des>>8)&0xFF;
	dstg = (srca * (srcg - dstg) + (dstg<<8))>>8;
	
	const unsigned int srcb = src&0xFF;
	unsigned int dstb = des&0xFF;
	dstb = (srca * (srcb - dstb) + (dstb<<8))>>8;

	return (unsigned int)(srca<<24 | dstr<<16 | dstg<<8 | dstb);
}

#else
//faster with not much difference in quality
static inline int ablend (const unsigned int des, const unsigned int src)
{
	const unsigned int alpha = (src&0xFF000000)>>24;
	//if (alpha == 0xFF) return src;

	const unsigned int odds2 = (src>>8) & 0xFF00FF;
	const unsigned int odds1 = (des>>8) & 0xFF00FF;
	const unsigned int evens1 = des & 0xFF00FF;
	const unsigned int evens2 = src & 0xFF00FF;
	const unsigned int evenRes = ((((evens2-evens1)*alpha)>>8) + evens1)& 0xFF00FF;
	const unsigned int oddRes = ((odds2-odds1)*alpha + (odds1<<8)) &0xFF00FF00;
	return (evenRes + oddRes);
}

#endif


/*
void clearFrame (TFRAME *frame)
{
	memset(frame->pixels, 0, frame->frameSize);
}

void clearFrameColour (TFRAME *frame, const int colour)
{
	int *pixels = lGetPixelAddress(frame, 0, 0);
	int tPixels = frame->frameSize>>2;
	while(tPixels--) *pixels++ = colour;
}
*/
void fillFrameColour (TFRAME *frame, const int colour)
{
	int *pixels = lGetPixelAddress(frame, 0, 0);
	int tPixels = frame->frameSize>>2;
	
	while(tPixels--){
		*pixels = colour;
		pixels++;
	}
}

void fillFrameAreaColour (TFRAME *frame, const int x1, const int y1, const int x2, const int y2, const int colour)
{
	//const int tPixels = ((x2 - x1)+1);
	
	for (int y = y1; y <= y2; y++){
		int *pixels = lGetPixelAddress(frame, x1, y);

		for (int x = x1; x <= x2; x++){
			*pixels = ablend(*pixels, colour);
			pixels++;
		}
	}
}

#if 0
static inline int getPixel32a_BL (TFRAME *frame, float x, float y)
{

	#define PSEUDO_FLOOR( V ) ((V) >= 0 ? (int)(V) : (int)((V) - 1))
	
	const int width = frame->width;
	const int height = frame->height;
	const int pitch = frame->pitch;
	const int spp = 4;
		
	//if (y < 0) y = 0;
	//if (x < 0) x = 0;

	float x1 = PSEUDO_FLOOR(x);
	float x2 = x1+1.0;
	
	if (x2 >= width) {
		x = x2 = (float)width-1.0;
		x1 = x2 - 1;
	}
	const int wx1 = (int)((x2 - x)*256.0);
	const int wx2 = 256-wx1;
	float y1 = PSEUDO_FLOOR(y);
	float y2 = y1+1.0;
	
	if (y2 >= height) {
		y = y2 = height-1.0;
		y1 = y2 - 1;
	}
	const int wy1 = (int)(256*(y2 - y));
	const int wy2 = 256 - wy1;
	const int wx1y1 = wx1*wy1;
	const int wx2y1 = wx2*wy1;
	const int wx1y2 = wx1*wy2;
	const int wx2y2 = wx2*wy2;

	const unsigned char *px1y1 = &frame->pixels[pitch * (int)y1 + spp * (int)x1];
	const unsigned char *px2y1 = px1y1 + spp;
	const unsigned char *px1y2 = px1y1 + pitch;
	const unsigned char *px2y2 = px1y1 + pitch+spp;

	const TCOLOUR4 *restrict cx1y1 = (TCOLOUR4*)px1y1;
	const TCOLOUR4 *restrict cx2y1 = (TCOLOUR4*)px2y1;
	const TCOLOUR4 *restrict cx1y2 = (TCOLOUR4*)px1y2;
	const TCOLOUR4 *restrict cx2y2 = (TCOLOUR4*)px2y2;
	
	TCOLOUR4 c;
	c.u.bgra.r = (wx1y1 * cx1y1->u.bgra.r + wx2y1 * cx2y1->u.bgra.r + wx1y2 * cx1y2->u.bgra.r + wx2y2 * cx2y2->u.bgra.r + 32768) / 65536;
	c.u.bgra.g = (wx1y1 * cx1y1->u.bgra.g + wx2y1 * cx2y1->u.bgra.g + wx1y2 * cx1y2->u.bgra.g + wx2y2 * cx2y2->u.bgra.g + 32768) / 65536;
	c.u.bgra.b = (wx1y1 * cx1y1->u.bgra.b + wx2y1 * cx2y1->u.bgra.b + wx1y2 * cx1y2->u.bgra.b + wx2y2 * cx2y2->u.bgra.b + 32768) / 65536;
	c.u.bgra.a = (wx1y1 * cx1y1->u.bgra.a + wx2y1 * cx2y1->u.bgra.a + wx1y2 * cx1y2->u.bgra.a + wx2y2 * cx2y2->u.bgra.a + 32768) / 65536;
	return c.u.colour;
}
#else
static inline int getPixel32a_BL (TFRAME *frame, double x, double y)
{

	#define PSEUDO_FLOOR( V ) ((V) >= 0 ? (int)(V) : (int)((V) - 1))
	
	const int width = frame->width;
	const int height = frame->height;
	const int pitch = frame->pitch;
	const int spp = 4;
		
	//if (y < 0) y = 0;
	//if (x < 0) x = 0;

	double x1 = PSEUDO_FLOOR(x);
	double x2 = x1+1.0;
	
	if (x2 >= width) {
		x = x2 = (double)width-1.0;
		x1 = x2 - 1;
	}
	const int wx1 = (int)((x2 - x)*256.0);
	const int wx2 = 256-wx1;
	double y1 = PSEUDO_FLOOR(y);
	double y2 = y1+1.0;
	
	if (y2 >= height) {
		y = y2 = height-1.0;
		y1 = y2 - 1;
	}
	const int wy1 = (int)(256*(y2 - y));
	const int wy2 = 256 - wy1;
	const int wx1y1 = wx1*wy1;
	const int wx2y1 = wx2*wy1;
	const int wx1y2 = wx1*wy2;
	const int wx2y2 = wx2*wy2;

	const unsigned char *px1y1 = &frame->pixels[pitch * (int)y1 + spp * (int)x1];
	const unsigned char *px2y1 = px1y1 + spp;
	const unsigned char *px1y2 = px1y1 + pitch;
	const unsigned char *px2y2 = px1y1 + pitch+spp;

	const TCOLOUR4 *restrict cx1y1 = (TCOLOUR4*)px1y1;
	const TCOLOUR4 *restrict cx2y1 = (TCOLOUR4*)px2y1;
	const TCOLOUR4 *restrict cx1y2 = (TCOLOUR4*)px1y2;
	const TCOLOUR4 *restrict cx2y2 = (TCOLOUR4*)px2y2;
	
	TCOLOUR4 c;
	c.u.bgra.r = (wx1y1 * cx1y1->u.bgra.r + wx2y1 * cx2y1->u.bgra.r + wx1y2 * cx1y2->u.bgra.r + wx2y2 * cx2y2->u.bgra.r + 32768) / 65536;
	c.u.bgra.g = (wx1y1 * cx1y1->u.bgra.g + wx2y1 * cx2y1->u.bgra.g + wx1y2 * cx1y2->u.bgra.g + wx2y2 * cx2y2->u.bgra.g + 32768) / 65536;
	c.u.bgra.b = (wx1y1 * cx1y1->u.bgra.b + wx2y1 * cx2y1->u.bgra.b + wx1y2 * cx1y2->u.bgra.b + wx2y2 * cx2y2->u.bgra.b + 32768) / 65536;
	c.u.bgra.a = (wx1y1 * cx1y1->u.bgra.a + wx2y1 * cx2y1->u.bgra.a + wx1y2 * cx1y2->u.bgra.a + wx2y2 * cx2y2->u.bgra.a + 32768) / 65536;
	return c.u.colour;
}
#endif

// returns 0 if target lies within, 1 if not
static inline int checkbounds (const TFRAME *const frm, const int x, const int y)
{
	if (x < 0 || x >= frm->width || y >= frm->height || y < 0)
		return 1;
	else
		return 0;
}

static inline void setPixel32 (const TFRAME *restrict frm, const int x, const int y, const int value)
{
	//if (!checkbounds(frm, x, y))
		*(uint32_t*)(frm->pixels+((y*frm->pitch)+(x<<2))) = value;
}

static inline void setPixel32a (const TFRAME *restrict frm, const int x, const int y, const int value)
{
	if (!checkbounds(frm, x, y)){
		 int *des = (int32_t*)(frm->pixels+((y*frm->pitch)+(x<<2)));
		*des = ablend(*des, value);
	}
}

static inline int getPixel32_NB (const TFRAME *frm, const int x, const int y)
{
	return *(uint32_t*)(frm->pixels+((frm->pitch*y)+(x<<2)));
}

static inline int getPixel32 (const TFRAME *frm, const int x, const int y)
{
	if (!checkbounds(frm, x, y))
		return *(uint32_t*)(frm->pixels+((frm->pitch*y)+(x<<2)));
	else
		return 0;
}

//static inline void setPixel32a_addr (const TFRAME *frame, const int x, const intptr_t *addrRow, const int value)
static inline void setPixel32a_addr (const TFRAME *frame, const int x, const uintptr_t addrRow, const int value)
{
	 //int *des = (int32_t*)(frame->pixels+((y*frame->pitch)+(x<<2)));
#if 0
	 intptr_t *des = (intptr_t*)(addrRow+(x<<2));
	*des = ablend((int32_t)*des, value);
#else
	int *des = (int32_t*)(addrRow+(x<<2));
	*des = ablend(*des, value);
#endif
}

void copyArea (TFRAME *from, TFRAME *to, int dx, int dy, int x1, int y1, int x2, int y2)
{
	 
/*	if (from->hw != to->hw){
		int w = (x2 - x1);
		int h = (y2 - y1);
		printf("copyArea from %p %i %i %i %i %i %i (%i/%i)\n", from, dx, dy, x1, y1, x2, y2, w, h);
		//assert(from->hw == to->hw);
		return;
	}
*/	
	 
	if (dx < 0){
		x1 += abs(dx);
		dx = 0;
	}
	if (dy < 0){
		y1 += abs(dy);
		dy = 0;
	}
	
	if (x2 >= from->width) x2 = from->width-1;
	if (y2 >= from->height) y2 = from->height-1;
	
	const int w = (x2-x1)+1;
	if (dx+w >= to->width) x2 -= (dx+w - to->width);
	const int h = (y2-y1)+1;
	if (dy+h >= to->height) y2 -= (dy+h - to->height);


	#define getPixelAddress(f,x,y)	((f)->pixels+(((y)*(f)->pitch)+((x)<<2)))

	for (int y = y1; y <= y2; y++, dy++){
		int *psrc = (int*)getPixelAddress(from, 0, y);
		int *pdes = (int*)getPixelAddress(to, 0, dy);
		int xx = dx;

		__asm__("prefetch 64(%0)" :: "r" (psrc) : "memory");
		__asm__("prefetch 64(%0)" :: "wr" (pdes) : "memory");
		//__builtin_prefetch(psrc, 0, 1);
		//__builtin_prefetch(pdes, 1, 1);
		

		for (int x = x1; x <= x2; x++,xx++)
			pdes[xx] = ablend(pdes[xx], psrc[x]);
	}
}

void copyAreaScaled (TFRAME *from, TFRAME *to, const int src_x, const int src_y, const int src_width, const int src_height, const int dest_x, const int dest_y, const int dest_width, const int dest_height)
{
	const double scalex = dest_width / (double)src_width;
	const double scaley = dest_height / (double)src_height;
	const double dx = 1.0 / scalex;
	const double dy = 1.0 / scaley;
	double y2 = src_y;
	const int src_x2 = dest_x + dest_width;
	
	
	for (int y = dest_y; y < dest_y + dest_height; y++){
		y2 += dy;
		double x2 = src_x;
		
		for (int x = dest_x; x < src_x2; x++){
			int col = getPixel32a_BL(from, x2, y2);
			setPixel32(to, x, y, col);
			x2 += dx;
		}
	}
}

static inline void drawImageScaled (TFRAME *from, TFRAME *to, const int src_x, const int src_y, const int src_width, const int src_height, const int dest_x, const int dest_y, const int dest_width, const int dest_height)
{
	const double scaley = dest_height / (double)src_height;
	const double scalex = dest_width / (double)src_width;
	const double dx = 1.0 / scalex;
	const double dy = 1.0 / scaley;
	double y2 = src_y;
	
	for (int y = dest_y; y < dest_y + dest_height; y++){
		y2 += dy;
		double x2 = src_x;
		
		for (int x = dest_x; x < dest_x + dest_width; x++){
			int col = getPixel32_NB(from, x2, y2);
			setPixel32a(to, x, y, col);
			x2 += dx;
		}
	}
}

static inline void drawImageScaledB (TFRAME *from, TFRAME *to, const int src_x, const int src_y, const int dest_x1, const int dest_y1, const int dest_x2, const int dest_y2, const double scale)
{

	const double dx = (1.0 - dest_x1) / scale;
	const double dy = (1.0 - dest_y1) / scale;
	double y2 = src_y;
		
	for (int y = dest_y1; y <= dest_y2; y++){
		y2 += dy;
		double x2 = src_x;
	
		for (int x = dest_x1; x <= dest_x2; x++){
			setPixel32(to, x, y, getPixel32(from, x2, y2));
			x2 += dx;
		}
	}
}

void drawImageScaledCenter (TFRAME *img, TFRAME *dest, const double scale, const double offsetX, const double offsetY)
{
	double swidth = (img->width / scale);
	double sheight = (img->height / scale);
	double cx = (dest->width / 2.0) - (offsetX/scale);
	double cy = (dest->height / 2.0) - (offsetY/scale);
	double x = cx - (swidth/2.0);
	double y = cy - (sheight/2.0);
	//printf("drawISC %.2f %.2f %.5f, %.2f %.2f, %.2f %.2f\n", x, y, scale, swidth, sheight, offsetX, offsetY);
		
	drawImageScaledB(img, dest, x, y, 0, 0, dest->width-1, dest->height-1, scale);
}

void drawImg (TVLCPLAYER *vp, TFRAME *frame, const int imgId, const int x, const int y)
{
	TFRAME *image = artManagerImageAcquire(vp->am, imgId);
	if (image){
		drawImage(image, frame, x, y, image->width-1, image->height-1);
		artManagerImageRelease(vp->am, imgId);
	}
}

static inline double rot_x (const double angle, const double x, const double y)
{
    return (x * cos(angle/180.0*M_PI) + y * -sin(angle/180.0*M_PI));
}

static inline double rot_y (const double angle, const double x, const double y)
{
    return (x * sin(angle/180.0*M_PI) + y * cos(angle/180.0*M_PI));
}

void rotate (TFRAME *src, TFRAME *des, const TMETRICS *metrics, double angle, const int w)
{
    const double dx_x = rot_x(-angle, 1.0, 0.0);
    const double dx_y = rot_y(-angle, 1.0, 0.0);
    const double dy_x = rot_x(-angle, 0.0, 1.0);
    const double dy_y = rot_y(-angle, 0.0, 1.0);
    
    const double d_w = metrics->width;
   	const double d_h = metrics->height;
   	
   	//angle -= 180.0;
 	double rx1 = rot_x(-angle, -d_w/2.0, -d_h/2.0) + (/*src->width*/(double)w/2.0);
    double ry1 = rot_y(-angle, -d_w/2.0, -d_h/2.0) + (src->height/1.0);

	const int x1 = metrics->x;
	const int x2 = x1+metrics->width;
	const int y1 = metrics->y;
	const int y2 = y1+metrics->height;
	const int pitch = des->pitch;

	for (int y = y1; y < y2; y++){
		double rx2 = rx1;
		double ry2 = ry1;
		//const intptr_t *row = (intptr_t*)des->pixels + (y*pitch);
		const int row = (intptr_t)des->pixels + (y*pitch);
		
		for (int x = x1; x < x2; x++){
			if (!checkbounds(src, rx2-1, ry2-1))
				setPixel32a_addr(des, x, row, getPixel32a_BL(src, rx2, ry2));

			rx2 += dx_x;
			ry2 += dx_y;
		}
		rx1 += dy_x;
		ry1 += dy_y;
	}
} 

void rotateAroundZ (TFRAME *src, TFRAME *des, const TMETRICS *metrics, double angle)
{
    const double dx_x = rot_x(-angle, 1.0, 0.0);
    const double dx_y = rot_y(-angle, 1.0, 0.0);
    const double dy_x = rot_x(-angle, 0.0, 1.0);
    const double dy_y = rot_y(-angle, 0.0, 1.0);
    
    const double d_w = metrics->width;
   	const double d_h = metrics->height;
   	
   	//angle -= 180.0;
 	double rx1 = rot_x(-angle, -d_w/2.0, -d_h/2.0) + (src->width/2.0);
    double ry1 = rot_y(-angle, -d_w/2.0, -d_h/2.0) + (src->height/2.0);

	const int x1 = metrics->x;
	const int x2 = x1+metrics->width;
	const int y1 = metrics->y;
	const int y2 = y1+metrics->height;
	const int pitch = des->pitch;

	for (int y = y1; y < y2; y++){
		double rx2 = rx1;
		double ry2 = ry1;
		//const intptr_t *row = (intptr_t*)des->pixels + (y*pitch);
		const int row = (intptr_t)des->pixels + (y*pitch);
		
		for (int x = x1; x < x2; x++){
			if (!checkbounds(src, rx2-1, ry2-1))
				setPixel32a_addr(des, x, row, getPixel32a_BL(src, rx2, ry2));

			rx2 += dx_x;
			ry2 += dx_y;
		}
		rx1 += dy_x;
		ry1 += dy_y;
	}
}


#if 0

void drawImageScaledOpacity (TFRAME *from, TFRAME *to, const int src_x, const int src_y, const int src_width, const int src_height, const int dest_x, const int dest_y, const int dest_width, const int dest_height, const double opacity)
{

	const float scalex = dest_width / (float)src_width;
	const float scaley = dest_height / (float)src_height;
	const float opacityf = (float)opacity;

	float opacitya = opacityf * 1.30f;
	if (opacitya > 1.0f) opacitya = 1.0f;
		
	for (int y = dest_y; y < dest_y + dest_height; y++){
		float y2 = src_y + (y-dest_y) / scaley;
		for (int x = dest_x; x < dest_x + dest_width; x++){
			float x2 = src_x + (x-dest_x) / scalex;
				
			int col = getPixel32a_BL(from, x2, y2);
			int a = (float)((col>>24)&0xFF)*opacitya;
			int r = (float)((col>>16)&0xFF)*opacityf;
			int g = (float)((col>>8)&0xFF)*opacityf;
			int b = (float)(col&0xFF)*opacityf;
			setPixel32a(to, x, y, (a<<24) | (r<<16) | (g<<8) | b);
		}
	}
}

#else

void drawImageScaledOpacity (TFRAME *from, TFRAME *to, const int src_x, const int src_y, const int src_width, const int src_height, const int dest_x, const int dest_y, const int dest_width, const int dest_height, const float opacityAlpha, const float opacityRGB)
{

	const float scalex = dest_width / (float)src_width;
	const float scaley = dest_height / (float)src_height;
	
	if (opacityAlpha < 0.960f || from->udata_int == SHELF_IMGTYPE_NOART){

		float opacity = opacityAlpha;
		if (opacity > 1.0f) opacity = 1.0f;
		const float dx = (1.0f) / scalex;
		
		for (int y = dest_y; y < dest_y + dest_height; y++){
			float y2 = src_y + (y-dest_y) / scaley;
			float x2 = src_x;
			for (int x = dest_x; x < dest_x + dest_width; x++){
				//int col = getPixel32a_BL(from, x2, y2);
				int col = getPixel32_NB(from, x2, y2);
				int a = ((col>>24)&0xFF) * opacity;
				int r = ((col>>16)&0xFF) * opacityRGB;
				int g = ((col>>8)&0xFF) * opacityRGB;
				int b =  (col&0xFF) * opacityRGB;
				setPixel32a(to, x, y, (a<<24) | (r<<16) | (g<<8) | b);
				x2 += dx;
			}
		}
	}else{
	
#if 1
			// fastest. plain copy - no resampling or scaling
		fastFrameCopy(from, to, dest_x, dest_y);

#elif 0		// with bilinear sampling, high quality but slower
		for (int y = dest_y; y < dest_y + dest_height; y++){
			int y2 = src_y + (y-dest_y) / scaley;
			for (int x = dest_x; x < dest_x + dest_width; x++){
				int x2 = src_x + (x-dest_x) / scalex;
				setPixel32a(to, x, y, getPixel32a_BL(from, x2, y2));
			}
		}
#else		// faster, with scaling, but doesn't look so great
		for (int y = dest_y; y < dest_y + dest_height; y++){
			y2 = src_y + (y-dest_y) / scaley;
			int *psrc = lGetPixelAddress(from, src_x, y2);
			
			for (int x = dest_x; x < dest_x + dest_width; x++){
				x2 = (x-dest_x) / scalex;
				setPixel32a(to, x, y, psrc[(int)x2]);
			}
		}
#endif
	}
}
#endif


void drawImageOpacity (TFRAME *from, TFRAME *to, const int dest_x, const int dest_y, double opacity)
{
	
	const int dest_width = from->width;
	const int dest_height = from->height;

	if (opacity > 1.0) opacity = 1.0;
	const double opacitycol = opacity * 0.65;

	for (int y = dest_y; y < dest_y + dest_height; y++){
		int y2 = y - dest_y;

		for (int x = dest_x; x < dest_x + dest_width; x++){
			int x2 = x - dest_x;
			int col = getPixel32_NB(from, x2, y2);
				
			int a = ((col>>24)&0xFF) * opacity;
			int r = ((col>>16)&0xFF) * opacitycol;
			int g = ((col>>8)&0xFF) * opacitycol;
			int b =  (col&0xFF) * opacitycol;
				
			setPixel32a(to, x, y, (a<<24) | (r<<16) | (g<<8) | b);
		}
	}
}

void drawShadowedImage (TFRAME *img1, TFRAME *dest, const int x, const int y, unsigned int colour, const int radius, const int offsetX, const int offsetY)
{
	
	TFRAME *img2 = lNewFrame(img1->hw, img1->width+BLUR_MAXOFFSET, img1->height+BLUR_MAXOFFSET, LFRM_BPP_32A);
	if (!img2) return;

	// set shadow colour
	colour &= 0xFFFFFF;
	unsigned int *src = lGetPixelAddress(img1, 0, 0);
	unsigned int *des = lGetPixelAddress(img2, 0, 0);
	int tpixels = img2->height * img2->width;
	while(tpixels--) *des++ = colour;
	
	// create shadow mask
	des = (unsigned int*)img2->pixels;
	for (int y = BLUR_MAXOFFSET_HALF; y < img1->height+BLUR_MAXOFFSET_HALF; y++){
		des = lGetPixelAddress(img2, BLUR_MAXOFFSET_HALF, y);
		for (int x = BLUR_MAXOFFSET_HALF; x < img1->width+BLUR_MAXOFFSET_HALF; x++)
			*des++ = ((*src++)&0xFF000000) | colour;
	}

	lBlurImage(img2, lBLUR_STACKFAST, radius);
	//lBlurImage(img2, lBLUR_GAUSSIAN, radius);
	
	drawImage(img2, dest, x-BLUR_MAXOFFSET_HALF+offsetX, y-BLUR_MAXOFFSET_HALF+offsetY, img2->width-1, img2->height-1);
	drawImage(img1, dest, x, y, img1->width-1, img1->height-1);
	lDeleteFrame(img2);
}

TFRAME *drawShadowedImageCreateBlurMask (TFRAME *img1, unsigned int colour, const int radius)
{
	TFRAME *img2 = lNewFrame(img1->hw, img1->width+BLUR_MAXOFFSET, img1->height+BLUR_MAXOFFSET, LFRM_BPP_32A);
	if (!img2) return NULL;
	
	colour &= 0xFFFFFF;
	unsigned int *src = lGetPixelAddress(img1, 0, 0);
	unsigned int *des = lGetPixelAddress(img2, 0, 0);
	int tpixels = img2->height * img2->width;
	while(tpixels--) *des++ = colour;
	
	
	des = (unsigned int*)img2->pixels;
	for (int y = BLUR_MAXOFFSET_HALF; y < img1->height+BLUR_MAXOFFSET_HALF; y++){
		des = lGetPixelAddress(img2, BLUR_MAXOFFSET_HALF, y);
		for (int x = BLUR_MAXOFFSET_HALF; x < img1->width+BLUR_MAXOFFSET_HALF; x++)
			*des++ = ((*src++)&0xFF000000) | colour;
	}

	lBlurImage(img2, lBLUR_STACKFAST, radius);
	//lBlurImage(img2, lBLUR_HUHTANEN, radius);
	//lBlurImage(img2, lBLUR_GAUSSIAN, radius);
	return img2;
}

void drawShadowedImageComputed (TFRAME *img1, TFRAME *dest, TFRAME *img2, const int x, const int y, const int offsetX, const int offsetY)
{
	drawImage(img2, dest, x-BLUR_MAXOFFSET_HALF+offsetX, y-BLUR_MAXOFFSET_HALF+offsetY, img2->width-1, img2->height-1);
	drawImage(img1, dest, x, y, img1->width-1, img1->height-1);
}

void drawShadowedImageComputedScaled (TFRAME *img1, TFRAME *dest, TFRAME *img2, const int x, const int y, const int offsetX, const int offsetY, const float scale)
{
	drawImageScaled(img2, dest, 0, 0, img2->width-1, img2->height-1, x-(BLUR_MAXOFFSET_HALF*scale)+(offsetX*scale), y-(BLUR_MAXOFFSET_HALF*scale)+(offsetY*scale), img2->width*scale, img2->height*scale);
	drawImageScaled(img1, dest, 0, 0, img1->width-1, img1->height-1, x, y, img1->width*scale, img1->height*scale);
}

void drawShadowedImageAlpha (TFRAME *img1, TFRAME *dest, const int x, const int y, unsigned int colour, const int radius, const int offsetX, const int offsetY, const double alpha)
{
	TFRAME *img2 = lNewFrame(img1->hw, img1->width+BLUR_MAXOFFSET, img1->height+BLUR_MAXOFFSET, LFRM_BPP_32A);
	if (!img2) return;
	

	// set shadow colour
	colour &= 0xFFFFFF;
	unsigned int *src = lGetPixelAddress(img1, 0, 0);
	unsigned int *des = lGetPixelAddress(img2, 0, 0);
	int tpixels = img2->height * img2->width;
	while(tpixels--) *des++ = colour;
	
	// create shadow mask
	des = (unsigned int*)img2->pixels;
	for (int y = BLUR_MAXOFFSET_HALF; y < img1->height+BLUR_MAXOFFSET_HALF; y++){
		des = lGetPixelAddress(img2, BLUR_MAXOFFSET_HALF, y);
		for (int x = BLUR_MAXOFFSET_HALF; x < img1->width+BLUR_MAXOFFSET_HALF; x++){
			float a = (*src>>24) * alpha;
			*des++ = ((*src++)&(int)a<<24) | colour;
		}
	}

	lBlurImage(img2, lBLUR_STACKFAST, radius);
	//lBlurImage(img2, lBLUR_GAUSSIAN, radius);
	
	drawImage(img2, dest, x-BLUR_MAXOFFSET_HALF+offsetX, y-BLUR_MAXOFFSET_HALF+offsetY, img2->width-1, img2->height-1);
	drawImage(img1, dest, x, y, img1->width-1, img1->height-1);
	lDeleteFrame(img2);
}

void rotateFrameL90 (TFRAME *frm)
{
	TFRAME *tmp = NULL;
	tmp = lNewFrame(frm->hw, frm->height, frm->width, frm->bpp);
	if (tmp == NULL) return;

	int dx = 0;

	for (int y = 0; y < frm->height; y++){
		int dy = tmp->height-1;
		for (int x = 0; x < frm->width; x++){
			setPixel32(tmp,dx,dy, getPixel32_NB(frm,x,y));
			dy--;
		}
		dx++;
	}

	ubyte *buffer = frm->pixels;		//ensure source frame is freed
	frm->pixels = tmp->pixels;
	tmp->pixels = buffer;
	frm->height = tmp->height;
	frm->width = tmp->width;
	frm->pitch = tmp->pitch;
	frm->bpp = tmp->bpp;
	frm->style = tmp->style;

	lDeleteFrame(tmp);
}

void rotateFrame180 (TFRAME *frm)
{
	TFRAME *tmp = NULL;
	tmp = lNewFrame(frm->hw, frm->width, frm->height, frm->bpp);
	if (tmp == NULL) return;

	int dy = tmp->height-1;

	for (int y = 0; y < frm->height; y++){
		int dx = tmp->width-1;
		for (int x = 0; x < frm->width; x++){
			setPixel32(tmp,dx,dy, getPixel32_NB(frm,x,y));
			dx--;
		}
		dy--;
	}

	ubyte *buffer = frm->pixels;		//ensure source frame is freed
	frm->pixels = tmp->pixels;
	tmp->pixels = buffer;
	frm->height = tmp->height;
	frm->width = tmp->width;
	frm->pitch = tmp->pitch;
	frm->bpp = tmp->bpp;
	frm->style = tmp->style;

	lDeleteFrame(tmp);
}

void rotateFrameR90 (TFRAME *frm)
{

	TFRAME *tmp = NULL;
	tmp = lNewFrame(frm->hw, frm->height, frm->width, frm->bpp);
	if (tmp == NULL) return;
	
	int dx = tmp->width-1;
	
	for (int y = 0;y < frm->height;y++){
		int dy = 0;
		for (int x = 0;x < frm->width;x++){
			setPixel32(tmp,dx,dy, getPixel32_NB(frm,x,y));
			dy++;
		}
		dx--;
	}

	ubyte *buffer = frm->pixels;		//ensure source frame is freed
	frm->pixels = tmp->pixels;
	tmp->pixels = buffer;
	frm->height = tmp->height;
	frm->width = tmp->width;
	frm->pitch = tmp->pitch;
	frm->bpp = tmp->bpp;
	frm->style = tmp->style;

	lDeleteFrame(tmp);
}

static inline void deleteShadow (TSHADOWUNDER *s)
{
	if (!s->isBuilt) return;
	s->isBuilt = 0;
	
	lDeleteFrame(s->topLeft);
	lDeleteFrame(s->topRight);
	lDeleteFrame(s->btmLeft);
	lDeleteFrame(s->btmRight);
	lDeleteFrame(s->vertBarTop);
	lDeleteFrame(s->vertBarBtm);
	lDeleteFrame(s->horiBarLeft);
	lDeleteFrame(s->horiBarRight);	
}

void deleteShadows (TSHADOWUNDER *shadow)
{
	deleteShadow(&shadow[SHADOW_BLACK]);
	deleteShadow(&shadow[SHADOW_BLUE]);
	deleteShadow(&shadow[SHADOW_GREEN]);
}

void invalidateShadows (TSHADOWUNDER *shadow)
{
	deleteShadows(shadow);
}

static inline void genShadow (TSHADOWUNDER *shadow, TFRAME *img)
{
	const int w = img->width/2;
	const int h = img->height/2;
	
	shadow->topLeft = lNewFrame(img->hw, w, h, img->bpp);
	shadow->topRight= lNewFrame(img->hw, w, h, img->bpp);
	shadow->btmLeft = lNewFrame(img->hw, w, h, img->bpp);
	shadow->btmRight= lNewFrame(img->hw, w, h, img->bpp);
	shadow->vertBarTop = lNewFrame(img->hw, 1, h/2, img->bpp);
	shadow->vertBarBtm = lNewFrame(img->hw, 1, h/2, img->bpp);
	shadow->horiBarLeft = lNewFrame(img->hw, w/2, 1, img->bpp);
	shadow->horiBarRight = lNewFrame(img->hw, w/2, 1, img->bpp);
	
	copyAreaNoBlend(img, shadow->topLeft, 0, 0, 0, 0, w-1, h-1);
	copyAreaNoBlend(img, shadow->topRight, 0, 0, w, 0, img->width-1, h-1);
	copyAreaNoBlend(img, shadow->btmLeft, 0, 0, 0, h, w-1, img->height-1);
	copyAreaNoBlend(img, shadow->btmRight, 0, 0, w, h, img->width-1, img->height-1);
	
	copyAreaNoBlend(img, shadow->vertBarTop, 0, 0, w, 0, w, (h/2)-1);
	copyAreaNoBlend(img, shadow->vertBarBtm, 0, 0, w, img->height-shadow->vertBarBtm->height, w, img->height-1);

	copyAreaNoBlend(img, shadow->horiBarLeft, 0, 0, 0, h, shadow->horiBarLeft->width-1, h);
	copyAreaNoBlend(img, shadow->horiBarRight, 0, 0, img->width-shadow->horiBarLeft->width, h, img->width-1, h);
	
	shadow->isBuilt = 1;
	
}

static inline void buildShadow (TVLCPLAYER *vp, TSHADOWUNDER *s, const int shadow)
{
	int shadowImg;
	if (shadow == SHADOW_BLUE)
		shadowImg = vp->gui.image[IMGC_SHADOW_BLU];
	else if (shadow == SHADOW_GREEN)
		shadowImg = vp->gui.image[IMGC_SHADOW_GRN];
	else/*if (shadow == SHADOW_BLACK)*/
		shadowImg = vp->gui.image[IMGC_SHADOW_BLK];

	TFRAME *img = imageManagerImageAcquire(vp->im, shadowImg);
	if (img){
		//printf("buildShadow %i %X, %i\n", shadow, shadowImg, s->isBuilt);
		genShadow(s, img);
		imageManagerImageRelease(vp->im, shadowImg);
		imageManagerImageFlush(vp->im, shadowImg);
	}
}

void drawShadowUnderlay (TVLCPLAYER *vp, TFRAME *img, const int x, const int y, const int w, const int h, const int shadow)
{

	TSHADOWUNDER *s = &vp->gui.shadow[shadow];
	if (!s->isBuilt){
		buildShadow(vp, s, shadow);
		if (!s->isBuilt) return;
	}
		
	const int sw = s->topLeft->width;
	const int sh = s->topLeft->height;
	const int offset = sw/2;
	
	// top left
	copyArea(s->topLeft, img, x-offset+1, y-offset+1, 0, 0, sw-1, sh-1);
	
	//top+btm bar
	const int barWidth = (offset + (w - offset - offset))-1;
	for (int i = offset+1; i < barWidth; i++)
		copyArea(s->vertBarTop, img, x+i, y-offset+1, 0, 0, 1, s->vertBarTop->height-1);

	// top right
	copyArea(s->topRight, img, (x+w)-offset-1, y-offset+1, 0, 0, sw-1, sh-1);

	const int barHeight = (offset + (h - offset - offset))-1;
	for (int i = offset+1; i < barHeight; i++)
		copyArea(s->horiBarLeft, img, x-offset+1, y+i, 0, 0, s->horiBarLeft->width-1, 1);
	for (int i = offset+1; i < barHeight; i++)
		copyArea(s->horiBarRight, img, (x+w)-1, y+i, 0, 0, s->horiBarRight->width-1, 1);
		
	// btm left
	copyArea(s->btmLeft, img, x-offset+1, (y+h)-offset-1, 0, 0, sw-1, sh-1);

	for (int i = offset+1; i < barWidth; i++)
		copyArea(s->vertBarBtm, img, x+i, (y+h)-1, 0, 0, 1, s->vertBarBtm->height-1);
		
	// btm right
	copyArea(s->btmRight, img, (x+w)-offset-1, (y+h)-offset-1, 0, 0, sw-1, sh-1);
}

int timeToString (const libvlc_time_t t, char *buffer, const size_t bufferLen)
{
	*buffer = 0;
	if (!(int)t) return 0;
	
	int seconds = (int)t%60;
	int minutes = (t/60) % 60;
	int hours = (int)((int)t/60/60.0f);
	if (t && t%60 == 0 && !hours) minutes = (int)t/60;

	if (hours)
		return __mingw_snprintf(buffer, bufferLen, "%i:%02i:%02i", hours, minutes, seconds);
	else if (minutes)
		return __mingw_snprintf(buffer, bufferLen, "%i:%02i", minutes, seconds);
	else
		return __mingw_snprintf(buffer, bufferLen, "0:%.2i", seconds);
}

int countChrW (const wchar_t *str, const int chr)
{
	int i = 0;
	while (*str) i += (*str++ == chr);
	return i;
}

int countChr (const char *str, const int chr)
{
	int i = 0;
	while (*str) i += (*str++ == chr);
	return i;
}

static inline int countColonsW (const wchar_t *str)
{
	return countChrW(str, L':');
}

static inline int countColons (const char *str)
{
	return countChr(str, ':');
}

libvlc_time_t stringToTimeW (wchar_t *sztime, size_t len)
{
	int h = 0; int m = 0; int s = 0;
	
	const int colons = countColonsW(sztime);
	if (colons > 1)
		swscanf(sztime, L"%d:%d:%d", &h, &m, &s);
	else if (colons == 1)
		swscanf(sztime, L"%d:%d", &m, &s);
	else
		swscanf(sztime, L"%d", &s);
	return (libvlc_time_t)(h * 60 * 60) + (libvlc_time_t)(m * 60) + s;
}

libvlc_time_t stringToTime (const char *sztime, size_t len)
{
	int h = 0; int m = 0; int s = 0;
	
	const int colons = countColons(sztime);
	if (colons > 1)
		sscanf(sztime, "%d:%d:%d", &h, &m, &s);
	else if (colons == 1)
		sscanf(sztime, "%d:%d", &m, &s);
	else
		sscanf(sztime, "%d", &s);
	return (h * 60 * 60) + (m * 60) + s;
}

unsigned int getHash (const char *path)
{
	return generateHash(path, strlen(path));
}

unsigned int getHashW (const wchar_t *path)
{
	unsigned int hash = 0;
	char *out = UTF16ToUTF8Alloc(path, wcslen(path));
	if (out){
		hash = getHash(out);
		my_free(out);
	}
	return hash;
}


// #f024+654+086+e23 +788 + 345 +234 + ++
int hexToInts (const char *_str, const char sep, int **val)
{
	int tSep = countChr((char*)_str, sep);
	if (!tSep++) return 0;

	int ct = 0;
	
	char *str = my_strdup(_str);
	if (str){
		*val = my_calloc(tSep, sizeof(int));

		char *hex = strtok(str, "+");
		while (hex && *hex && ct < tSep){
			if (strlen(hex) > 2){
				(*val)[ct] = hexToInt(hex);
				//printf("hexToInts %i '%s' : %X\n", i, hex, (*val)[ct]);
				ct++;
			}
			hex = strtok(NULL, "+");
		};
		
		if (!ct && *val) my_free(*val);
		my_free(str);
	}
	return ct;
}

char *intToHex (const int val1)
{
	char *str = my_malloc(17);
	if (str)
		__mingw_snprintf(str, 16, "%X", val1);
	return str;
}

int hexHexToInt2 (const char *hex, int *val1, int *val2)
{
	if (!hex || !*hex) return 0;
	
	int ret = sscanf(hex, "%x:%x", val1, val2) == 2;
	if (!ret)
		ret = sscanf(hex, "%x %x", val1, val2) == 2;
	
	return ret;
}

int hexToInt2W (const wchar_t *hex, int *val1, int *val2)
{
	if (!hex || !*hex) return 0;
	
	return swscanf(hex, L"%x %x", val1, val2) == 2;
}

int hexIntToInt2 (const char *hex, int *val1, int *val2)
{
	if (!hex || !*hex) return 0;
	
	int ret = sscanf(hex, "%x:%i", val1, val2) == 2;
	if (!ret)
		ret = sscanf(hex, "%x %i", val1, val2) == 2;
	
	return ret;
}

int hexToIntW (const wchar_t *hex)
{
	if (!hex || !*hex) return 0;
	
	int val = 0;
	swscanf(hex, L"%x", &val);
	return val;
}

int hexToInt (const char *hex)
{
	if (!hex || !*hex) return 0;
	
	int val = 0;
	sscanf(hex, "%x", &val);
	return val;
}

int decToInt (const char *str)
{
	if (!str || !*str) return 0;
	
	int val = 0;
	sscanf(str, "%i", &val);
	return val;
}

int decToIntDouble (const char *str, int *val1, double *val2)
{
	if (!str || !*str) return 0;
	
	int ret = sscanf(str, "%i:%lf", val1, val2) == 2;
	if (!ret)
		ret = sscanf(str, "%i %lf", val1, val2) == 2;
	return ret;
}

int decodeURIEx (const char *arturl, const int len, char *buffer)
{
	static int hextodec[256];
     			
	if (!hextodec['A']){
    	int ct = 0;
		for (int i = '0'; i <= '9'; i++)
			hextodec[i] = ct++;
		ct = 10;
		for (int i = 'A'; i <= 'F'; i++){
			hextodec[i] = ct;
			hextodec[i+32] = ct++;
		}
	}
	if (len  < 10) return 0;
	

	int c = 0;
   	for (int i = 8; i < len; i++){
   		if (i < len-2 && arturl[i] == '%' && (isdigit(arturl[i+1])||isalpha(arturl[i+1]))){
   			buffer[c++] = hextodec[(int)arturl[i+1]]<<4|hextodec[(int)arturl[i+2]];
    			i += 2;
     		continue;
		}
    	buffer[c++] = arturl[i];
   	}     			
   	buffer[c] = 0;	
	return c;
}

void *decodeURI (const char *arturl, const int len)
{
	static int hextodec[256];
     			
	if (!hextodec['A']){
    	int ct = 0;
		for (int i = '0'; i <= '9'; i++)
			hextodec[i] = ct++;
		ct = 10;
		for (int i = 'A'; i <= 'F'; i++){
			hextodec[i] = ct;
			hextodec[i+32] = ct++;
		}
	}
	if (len  < 10) return NULL;
	
	unsigned char *tmp = my_malloc(len+2);
	if (tmp){
		int c = 0;
    	for (int i = 8; i < len; i++){
    		if (i < len-2 && arturl[i] == '%' && (isdigit(arturl[i+1])||isalpha(arturl[i+1]))){
    			tmp[c++] = hextodec[(int)arturl[i+1]]<<4|hextodec[(int)arturl[i+2]];
     			i += 2;
	     		continue;
			}
	    	tmp[c++] = arturl[i];
    	}     			
    	tmp[c] = 0;	
    }
    return tmp;
}

void *decodeURI_noprefix (const char *arturl, const int len)
{
	static int hextodec[256];
     			
	if (!hextodec['A']){
    	int ct = 0;
		for (int i = '0'; i <= '9'; i++)
			hextodec[i] = ct++;
		ct = 10;
		for (int i = 'A'; i <= 'F'; i++){
			hextodec[i] = ct;
			hextodec[i+32] = ct++;
		}
	}
	
	unsigned char *tmp = my_malloc(len+2);
	if (tmp){
		int c = 0;
    	for (int i = 0; i < len; i++){
    		if (i < len-2 && arturl[i] == '%' && (isdigit(arturl[i+1])||isalpha(arturl[i+1]))){
    			tmp[c++] = hextodec[(int)arturl[i+1]]<<4|hextodec[(int)arturl[i+2]];
     			i += 2;
	     		continue;
			}
	    	tmp[c++] = arturl[i];
    	}     			
    	tmp[c] = 0;	
    }
    return tmp;
}

#if 0
int encodeURI (const char *in, char *out, const size_t len)
{
	static const char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	
	int i = 0;
	while(i < len){
		if (*in&0x80){
			*out++ = '%';
			*out++ = hex[(in[0]&0xF0)>>4];
			*out++ = hex[ in[0]&0x0F];
			in++;
		}else{
			*out++ = *in++;
		}
		i++;
	}
	*out = '\0';
	return i;
}
#endif

int doesFileExistW (const wchar_t *path)
{
	if (!*path) return 0;
	
	FILE *fp = _wfopen(path, L"r");
	if (fp) fclose(fp);
	return (fp != NULL);
}

int doesFileExist8 (const char *path8)
{
	int ret = 0;
	wchar_t *path = converttow(path8);
	if (path){
		ret = doesFileExistW(path);
		my_free(path);
	}
	return ret;
}

int isDirectoryW (const wchar_t *path)
{

    //int isDir = (GetFileAttributesW(path) & FILE_ATTRIBUTE_DIRECTORY)>0;
	//wprintf(L"isDirectoryW #%s#  %i %i\n",path, PathIsDirectoryW(path)>0, isDir);
	
	if (path){
    	int isDir = (GetFileAttributesW(path) & FILE_ATTRIBUTE_DIRECTORY) > 0;
		return isDir && (PathIsDirectoryW(path) > 0);
	}else{
		return 0;
	}
}

int isDirectory (const char *path8)
{
	int ret = 0;
	
	wchar_t *path = converttow(path8);
	if (path){
		ret = isDirectoryW(path);
		my_free(path);
	}
	return ret;
}
/*
int isPlaylistW (const wchar_t *path)
{
	static unsigned int hash;
	static int extlen;
	
	if (!hash){
		hash = getHashW(VLCSPLAYLISTEXTW);
		extlen = wcslen(VLCSPLAYLISTEXTW);
	}

	if (path){
		const int len = wcslen(path);
		if (len > extlen)
			return (getHashW(&path[(len-extlen)]) == hash);
	}
	return 0;
}
*/
int isPlaylist (const char *path)
{
	static unsigned int hash;
	static int extlen;
	
	if (!hash){
		hash = getHash(VLCSPLAYLISTEXT);
		extlen = strlen(VLCSPLAYLISTEXT);
	}

	if (path){
		const int len = strlen(path);
		if (len > extlen)
			return (getHash(&path[(len-extlen)]) == hash);
	}
	return 0;
}

int stripOptionsW (wchar_t *path)
{
	const int len = wcslen(path);
	int ct = 0;
	for (int i = 0; i < len && path[i]; i++){
		if ((i < len-1) && path[i] == L'<' && path[i+1] && path[i+1] != L'>'){
			ct++;
			for (int j = 0; path[i] && path[i] != L'>'; j++, i++)
				path[i] = L' ';
			if (path[i] == L'>') path[i] = L' ';
		}
	}
	return ct;
}

wchar_t *removeLeadingSpacesW (wchar_t *var)
{
	int i = wcsspn(var, L" \t");
	if (i) var += i;
	return var;
}

wchar_t *removeTrailingSpacesW (wchar_t *var)
{
	wchar_t *eos = var + wcslen(var) - 1;
	while (eos >= var && (*eos == L' ' || *eos == L'\t'))
		*eos-- = L'\0';
	return var;
}

char *removeLeadingSpaces (char *var)
{
	int i = strspn(var, " \t");
	if (i) var += i;
	return var;
}

char *removeTrailingSpaces (char *var)
{
	char *eos = var + strlen(var) - 1;
	while (eos >= var && (*eos == ' ' || *eos == '\t'))
		*eos-- = '\0';
	return var;
}

void clearConsole (TVLCPLAYER *vp)
{
	marqueeClear(vp->gui.marquee);
}

static inline int addConsoleline (TVLCPLAYER *vp, const char *str)
{
	return marqueeAdd(vp, vp->gui.marquee, str, getTime(vp)+7000);
}

static inline int addConsolelineW (TVLCPLAYER *vp, const wchar_t *str)
{
	int ret = -1;

	char *txt = UTF16ToUTF8Alloc(str, wcslen(str));
	if (txt){
		ret = addConsoleline(vp, txt);
		my_free(txt);
	}
	return ret;
}

/*
int vasprintf (char **sptr, const char *fmt, va_list argv)
{
	int wanted = vsnprintf(*sptr = NULL, 0, fmt, argv);
	if ((wanted < 0) || ((*sptr = my_malloc(2 + wanted)) == NULL))
		return -1;

	return vsprintf(*sptr, fmt, argv);
}

int _asprintf (char **sptr, const char *fmt, ...)
{
	int retval;
	va_list argv;
	va_start(argv, fmt);
	retval = vasprintf(sptr, fmt, argv);
	va_end(argv);
	return retval;
}*/

void dbprintf (TVLCPLAYER *vp, const char *fmt, ...)
{
	if (SHUTDOWN) return;
	
	char buffer[MAX_PATH_UTF8+1];
	int ret = 0;
	VA_OPEN(ap, fmt);
	ret = __mingw_vsnprintf(buffer, MAX_PATH_UTF8, fmt, ap);
	VA_CLOSE(ap);
	
	if (ret > 0)
		addConsoleline(vp, buffer);
	//if (buffer)
	//	my_free(buffer);
}

void dbprintfEx (TVLCPLAYER *vp, const int flags, const char *fmt, ...)
{
	if (SHUTDOWN) return;
	
	char buffer[MAX_PATH_UTF8+1];
	int ret = 0;
	VA_OPEN(ap, fmt);
	ret = __mingw_vsnprintf(buffer, MAX_PATH_UTF8, fmt, ap);
	VA_CLOSE(ap);
	
	if (ret > 0){
		int line = marqueeAdd(vp, vp->gui.marquee, buffer, getTime(vp)+30000);
		if (line >= 0){
			if (flags&MARQUEE_WRAP)
				marqueeStringFlags(vp, vp->gui.marquee, line, PF_WORDWRAP|PF_CLIPWRAP);
		}
	}
	//if (buffer)
		//my_free(buffer);
}

#if 0
int vaswprintf (wchar_t **sptr, const wchar_t *fmt, va_list argv)
{
	int wanted = vsnwprintf(*sptr = NULL, 0, fmt, argv);
	if ((wanted < 0) || ((*sptr = my_calloc((2 + wanted),  sizeof(wchar_t))) == NULL))
		return -1;

	return _vswprintf(*sptr, /*wanted,*/ fmt, argv);
}
#endif

void dbwprintfEx (TVLCPLAYER *vp, const int flags, const wchar_t *fmt, ...)
{
	if (SHUTDOWN) return;
	
	wchar_t buffer[MAX_PATH_UTF8+1];
	int ret = 0;
	VA_OPEN(ap, fmt);
	ret = /*__mingw*/_vsnwprintf(buffer, MAX_PATH_UTF8, fmt, ap);
	VA_CLOSE(ap);
	
	if (ret > 0){
		char *txt = UTF16ToUTF8Alloc(buffer, wcslen(buffer));
		if (txt){
			int line = marqueeAdd(vp, vp->gui.marquee, txt, getTime(vp)+25000);
			if (line >= 0){
				if (flags&MARQUEE_WRAP)
					marqueeStringFlags(vp, vp->gui.marquee, line, PF_WORDWRAP|PF_CLIPWRAP);
			}
			my_free(txt);
		}
	}
	//if (buffer)
	//	my_free(buffer);
}

void dbwprintf (TVLCPLAYER *vp, const wchar_t *fmt, ...)
{
	if (SHUTDOWN) return;
	
	wchar_t buffer[MAX_PATH_UTF8+1];
	int ret = 0;
	VA_OPEN(ap, fmt);
	ret = _vswprintf(buffer, fmt, ap);
	VA_CLOSE(ap);
	
	//wprintf(L"dbwprintf '%s'\n", buffer);
	
	if (ret > 0)
		addConsolelineW(vp, buffer);
	//if (buffer)
	//	my_free(buffer);
}

wchar_t *getDirectoryW (const wchar_t *indir)
{
	wchar_t drive[MAX_PATH+1];
	wchar_t dir[MAX_PATH+1];
	wchar_t path[MAX_PATH+1];
	*drive = 0;
	*dir = 0;
	
	_wsplitpath(indir, drive, dir, NULL, NULL);
	__mingw_swprintf(path, L"%ls%ls", drive, dir);

	return my_wcsdup(path);
}

char *getDirectory (const char *indir)
{
	char drive[MAX_PATH+1];
	char dir[MAX_PATH+1];
	char path[MAX_PATH_UTF8+1];
	*drive = 0;
	*dir = 0;
	
	_splitpath(indir, drive, dir, NULL, NULL);
	__mingw_snprintf(path, MAX_PATH_UTF8, "%s%s", drive, dir);

	return my_strdup(path);
}

// without this drag'n'drop will screw with the path
void setCurrentDirectory (const wchar_t *indir)
{
	wchar_t drive[MAX_PATH+1];
	wchar_t dir[MAX_PATH+1];
	wchar_t path[MAX_PATH+1];
	*drive = 0;
	*dir = 0;
	
	_wsplitpath(indir, drive, dir, NULL, NULL);
	__mingw_swprintf(path, L"%ls%ls", drive, dir);
	SetCurrentDirectoryW(path);
}

void setForegroundColourIdx (TVLCPLAYER *vp, const int pageIdx, const int colIdx)
{
	const unsigned int *col = swatchGetPage(vp, pageIdx);
	lSetForegroundColour(vp->ml->hw, col[colIdx]);
}

void setBackgroundColourIdx (TVLCPLAYER *vp, const int pageIdx, const int colIdx)
{
	const unsigned int *col = swatchGetPage(vp, pageIdx);
	lSetBackgroundColour(vp->ml->hw, col[colIdx]);
}


#if ENABLE_CMDFUNSTUFF
void botQuoteRandom (TVLCPLAYER *vp, TCMDREPLY *sheets, const int bot)
{

	static int botsLoaded[5] = {0,0,0,0,0};
	
	
	switch (bot){
	  case BOT_BOFH:
	  	if (!botsLoaded[bot])
	  		botsLoaded[bot] = sheetAdd(sheets, "bofh", FUNSTUFFD(L"bofh"));

		if (botsLoaded[bot]){
			char *line = sheetGetLineRand(sheets, "bofh");
			if (line){
				dbprintfEx(vp, MARQUEE_WRAP, "BOFH: %s", line);
				my_free(line);
			}
		}
		break;
	  case BOT_DUBYA:
	  	if (!botsLoaded[bot])
	  		botsLoaded[bot] = sheetAdd(sheets, "dubya", FUNSTUFFD(L"dubya"));

		if (botsLoaded[bot]){
			char *line = sheetGetLineRand(sheets, "dubya");
			if (line){
				dbprintfEx(vp, MARQUEE_WRAP, "Dubya: %s", line);
				my_free(line);
			}
		}
		break;
	  case BOT_FACTS:
	  	if (!botsLoaded[bot])
	  		botsLoaded[bot] = sheetAdd(sheets, "facts", FUNSTUFFD(L"facts"));

		if (botsLoaded[bot]){
			char *line = sheetGetLineRand(sheets, "facts");
			if (line){
				dbprintfEx(vp, MARQUEE_WRAP, "Fact: %s", line);
				my_free(line);
			}
		}
		break;
	  case BOT_MORBID:
	  	if (!botsLoaded[bot])
	  		botsLoaded[bot] = sheetAdd(sheets, "morbid", FUNSTUFFD(L"morbid"));

		if (botsLoaded[bot]){
			char *line = sheetGetLineRand(sheets, "morbid");
			if (line){
				dbprintfEx(vp, MARQUEE_WRAP, "Morbidity: %s", line);
				my_free(line);
			}
		}
		break;
	};
}
#endif


#if 0
int my_vasprintf (char **result, const char *format, va_list *args)
{
	
	const char *p = format;
	int total_width = strlen(format) + sizeof(char);
	va_list ap;
	
	my_memcpy(&ap, args, sizeof(va_list));
	
	while (*p != '\0'){
		if (*p++ == '%'){
			while (strchr("-+ #0", *p))
				++p;
				
			if (*p == '*'){
				++p;
				total_width += abs(va_arg(ap, int));
			}else{
				total_width += strtoul(p, (char**)&p, 10);
			}
	          
			if (*p == '.'){
				++p;
				if (*p == '*'){
					++p;
					total_width += abs(va_arg(ap, int));
				}else{
					total_width += strtoul(p, (char**)&p, 10);
				}
			}

			while (strchr ("hlL", *p))
				++p;
	          
			/* Should be big enough for any format specifier except %s.  */
			total_width += 30;
			switch (*p){
			  case 'd':
			  case 'i':
			  case 'o':
			  case 'u':
			  case 'x':
			  case 'X':
			  case 'c':
			    (void)va_arg(ap, int);
			    break;
			  case 'f':
			  case 'e':
			  case 'E':
			  case 'g':
			  case 'G':
			    (void)va_arg(ap, double);
			    break;
			  case 's':
			    total_width += strlen(va_arg(ap, char *));
			    break;
			  case 'p':
			  case 'n':
			    (void)va_arg(ap, void*);
			    break;
			}
		}
	}
	if (!total_width) return 0;
	
	*result = my_calloc(sizeof(char), total_width+1);
	if (*result != NULL)
		return vsnprintf(*result, total_width, format, *args);
	else
		return 0;
}
#endif
