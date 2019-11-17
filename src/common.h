
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



#ifndef _COMMON_H_
#define _COMMON_H_



#include <shlwapi.h>
#include <inttypes.h>
#include <windows.h>
#include <time.h>
#include <math.h>
#include <mylcd.h>
//#include <shfolder.h>
#include <shlobj.h>
#include <vlc/libvlc_version.h>


#define RELEASEBUILD			0				// disables console input when set
#define MOUSEHOOKCAP			1				// enable mouse hooking capability. keys: shift+control+A or Q, L or P (creates a new thread and hidden window)
#define ASYNCHRONOUSREFRESH 	1
#define USEEXTMEMFUNC			0				// define to 1 to use libmylcd's memory alloc and trace routines
#define USE_MMX_MEMCPY			1
#define USEINTERNALMEMMANGER	0				//
#define ALLOWDEBUGGER			0

#define DRAWBUTTONRECTS			0
#define DRAWTOUCHRECTS			0				// draw rectangles around buttons and touch contact areas
#define DRAWMISCDETAIL			0
#define DRAWCCOBJBOUNDS			0				// draw rectangles around enabled cc objects
#define DRAWCCOBJBOUNDSFILLED	0
#define DRAWGLYPHBOUNDS			0
#define DRAWSTRINGBOUNDS		0
#define DRAWCURSORCROSS			0
#define DRAWBUTTONRECTCOL		(0xFF0000FF)	// blue
#define DRAWTOUCHRECTCOL		(0xFFFFFF00)	// yellow
#define DRAWCCOBJCOL			(0x00FFFF00)

#define ENABLE_REGMETAUPDATE	1				// write current track meta info to registry
#define ENABLE_ENFORCEMINSPEC	0				// ensure we're dealing with sse2 and a dual core cpu
#define ENFORCE_VLCVERSION		0
#define ENABLE_SINGLEINSTANCE	0				// single instance only, pass args to first instance. required when using RzHome.exe
#define ENABLE_FILEEXT_CONFIG	1

#define ENABLE_ANTPLUS			1				// garmin ant+ heart rate display. requires libusb installed Ant+ dongle
#define ENABLE_GARMINTCX		1
#define ENABLE_BRIGHTNESS		0				// hardware brightness support where supported
#define ENABLE_CMDFUNSTUFF		0

#define SINGLEINSTANCE_USE_CDS	1
#define ENABLE_BASS				(1/* || !RELEASEBUILD*/)

#define DEVICE_DEFAULT_NAME		"switchbladefio"
#define DEVICE_DEFAULT_WIDTH	SBUI_PAD_WIDTH
#define DEVICE_DEFAULT_HEIGHT	SBUI_PAD_HEIGHT

#define NSEX_LEFT				(0x01)
#define NSEX_RIGHT				(0x02)
#define DS_MIDDLEJUSTIFY		(0x04)

//#define ALLOW_FALLTHROUGH	
#define ALLOW_FALLTHROUGH		__attribute__ ((fallthrough))		// C and C++03
//#define ALLOW_FALLTHROUGH		[[gnu::fallthrough]]				// C++11 and C++14
//#define ALLOW_FALLTHROUGH		[[fallthrough]]						// C++17 and above

#ifndef	_ANSIDECL_H
#undef VA_OPEN
#undef VA_CLOSE
#undef VA_FIXEDARG

#define VA_OPEN(AP, VAR)	{ va_list AP; va_start(AP, VAR); { struct Qdmy
#define VA_CLOSE(AP)		} va_end(AP); }
#define VA_FIXEDARG(AP, T, N)	struct Qdmy
#endif


#ifndef DEGTORAD
#define DEGTORAD 0.0174532925195
#endif

#include "memory.h"
#include "lock.h"
#include "libmylcd.h"
#include "stack.h"
#include "ui.h"
#include "vlsprocess.h"
#include "list.h"
#include "settings.h"
#include "tags.h"
#include "fileal.h"
#include "playlistsort.h"
#include "editbox.h"
#include "funstuff.h"
#include "input.h"
#include "tree.h"
#include "artc.h"
#include "imagec.h"
#include "jobcon.h"
#include "cc/cc.h"
#include "page.h"
#include "cmdparser.h"
#include "fileext.h"
#include "filebrowser.h"
#include "filepane.h"
#include "exppanel.h"
#include "playlist.h"
#include "playlistc.h"
#include "playlistManager.h"
#include "m3u.h"
#include "stringcache.h"

#if (ENABLE_BASS || !RELEASEBUILD)
#include "bass.h"
#endif

#include "picqueue.h"
#include "swatch.h"
#include "timer.h"
//#include "artworkqueue.h"
#include "marquee.h"
#include "winm.h"
#include "vlcstream.h"
#include "cpu.h"
#include "drawvolume.h"
#include "shelf.h"
#include "plm.h"
#include "album.h"
#include "m3u.h"
//#include "winm.h"
#include "keyboard.h"
#include "search.h"
#include "videofilter.h"
#include "transform.h"
#include "videotransform.h"
#include "ctrloverlay.h"
#include "fileio.h"
#include "meta.h"
#include "vlc.h"
#include "vlceventcb.h"
#include "mediastats.h"
#include "textoverlay.h"
#include "chapter.h"
#include "es.h"
#include "imgovr.h"
#include "exit.h"
#include "cfg.h"
#include "sub.h"
#include "libvlceq.h"
#include "eq.h"
#include "alarm.h"
#include "epg.h"
#include "plytv.h"
#include "plypanel.h"
#include "plypane.h"
#include "imgpane.h"
#include "clock.h"
#include "tetris.h"
#include "taskman.h"
#include "home.h"
#include "hotkeys.h"
#include "config.h"
#include "hook/hook.h"
#include "sbui.h"
#include "vaudio.h"
#if ENABLE_GARMINTCX
#include "garmin.h"
#endif
#if ENABLE_ANTPLUS
#include "antplus.h"
#endif
//#include "../../src/sbui153/sbuicb.h"
#include <sbuicb.h>
#include "ext.h"





#if 0

//#define HAVE_SNPRINTF
//#define HAVE_VSNPRINTF

#undef _snprintf
#define _snprintf __mingw_snprintf

#ifdef _snwprintf
#undef _snwprintf
#define _snwprintf __mingw_snwprintf
#endif

#ifdef _swprintf
#undef _swprintf
#define _swprintf __mingw_swprintf
#endif

#ifdef _vswprintf
#undef _vswprintf
#define _vswprintf __mingw_vswprintf
#endif

#ifdef _vsprintf
#undef _vsprintf
#define _vsprintf __mingw_vsprintf
#endif

#ifdef _vsnprintf
#undef _vsnprintf
#define _vsnprintf __mingw_vsnprintf
#endif

#endif



#define clipFloat(x)\
	if ((x) > 1.0) (x) = 1.0;\
	else if ((x) < 0.0) (x) = 0.0

#define getTickCount GetTickCount64




char *jsonGetTag (char *json, const char *tag);
char *getUrl (const char *url, size_t *totalRead);


int getShutdownPriv ();
void workstationLogoff ();
void workstationReboot ();
void workstationLock ();
void workstationShutdown ();
void workstationShutdownAbort();


TFRAME *newImage (TVLCPLAYER *vp, const wchar_t *filename, const int bpp);
int loadImage (TFRAME *frame, const wchar_t *filename);

MYLCD_EXPORT uint64_t rdtsc ();

wchar_t *buildSkinD (TVLCPLAYER *vp, wchar_t *buffer, wchar_t *path);
wchar_t *buildSkinDEx (TVLCPLAYER *vp, wchar_t *buffer, wchar_t *dir, wchar_t *file);

void strchrreplace (char *str, const char replaceThis, const char withThis);
char *stristr (const char *String, const char *Pattern);
wchar_t *wcsistr (const wchar_t *String, const wchar_t *Pattern);
int UTF8ToUTF16 (const char *in, const size_t ilen, wchar_t *out, size_t olen);
int UTF16ToUTF8 (const wchar_t *in, const size_t ilen, char *out, size_t olen);
char *convertto8 (const wchar_t *wide);
wchar_t *converttow (const char *utf8);

// extract directory from path\filename.ext
char *getDirectory (const char *indir);
wchar_t *getDirectoryW (const wchar_t *indir);

unsigned int getHash (const char *str);		// ascii/utf8
unsigned int getHashW (const wchar_t *strw);	// utf16

int timeToString (const libvlc_time_t t, char *buffer, const size_t bufferLen);
libvlc_time_t stringToTime (const char *sztime, size_t len);
libvlc_time_t stringToTimeW (wchar_t *sztime, size_t len);

wchar_t *removeLeadingSpacesW (wchar_t *var);
wchar_t *removeTrailingSpacesW (wchar_t *var);
char *removeLeadingSpaces (char *var);
char *removeTrailingSpaces (char *var);

TFRAME *newStringEx  (THWD *hw,       TMETRICS *metrics, const int bpp, const int flags, const int font, const char *text, const int maxW, const int nsex_flags);
TFRAME *newStringEx2 (THWD *hw, const TMETRICS *metrics, const int bpp, const int flags, const int font, const char *text, int x, int y, const int maxW, const int maxH, const int nsex_flags);
TFRAME *newStringEx3 (THWD *hw, const TMETRICS *metrics, const int bpp, const int flags, const int font, const char *text, int x, int y, const int maxW, const int maxH);
TFRAME *newStringListEx (THWD *hw, const int bpp, const int flags, const int font, const unsigned int *glist, const int gtotal, const int maxW, const int nsex_flags);
TFRAME *newStringList (THWD *hw, const TMETRICS *metrics, const int bpp, const int flags, const int font, const unsigned int *glist, const int gtotal, int x, int y, const int maxW, const int maxH, const int nsex_flags);
void printSingleLineShadow (TFRAME *frame, const int font, const int x, const int y, const int foreCol, const int backCol, const char *str);

void rotateFrameL90 (TFRAME *frm);
void rotateFrameR90 (TFRAME *frm);
void rotateFrame180 (TFRAME *frm);

void imageBestFit (const int bg_w, const int bg_h, int fg_w, int fg_h, int *w, int *h);


#if 0

TFRAME *newFrameFromFile (TVLCPLAYER *vp, const wchar_t *filename);
TFRAME *newFrameFromImageList (TVLCPLAYER *vp, const HIMAGELIST imgList, const int imgIdx);
TFRAME *newFrameFromHICON (TVLCPLAYER *vp, const HICON icon);
TFRAME *newFrameFromFileEx (TVLCPLAYER *vp, const wchar_t *filename, const int height);
TFRAME *newFrameFromHICONEx (TVLCPLAYER *vp, const HICON icon, const int height);
TFRAME *getFileIcon (TVLCPLAYER *vp, const wchar_t *filename);
TFRAME *getFileIconEx (TVLCPLAYER *vp, const wchar_t *filename, const int height);
#endif

void copyArea (TFRAME *from, TFRAME *to, int dx, int dy, int x1, int y1, int x2, int y2);
void copyAreaNoBlend (TFRAME *from, TFRAME *to, int dx, int dy, int x1, int y1, int x2, int y2);
void copyAreaScaled (TFRAME *from, TFRAME *to, const int src_x, const int src_y, const int src_width, const int src_height, const int dest_x, const int dest_y, const int dest_width, const int dest_height);
void drawImageScaledCenter (TFRAME *img, TFRAME *dest, const double scale, const double offsetX, const double offsetY);
void drawImageScaledOpacity (TFRAME *from, TFRAME *to, const int src_x, const int src_y, const int src_width, const int src_height, const int dest_x, const int dest_y, const int dest_width, const int dest_height, const float opacityAlpha, const float opacityRGB);
void drawImageOpacity (TFRAME *from, TFRAME *to, const int dest_x, const int dest_y, double opacity);
void drawImg (TVLCPLAYER *vp, TFRAME *frame, const int imgId, const int x, const int y);

#define drawImage(s,d,dx,dy,x2,y2) copyArea((s),(d),(dx),(dy),0,0,(x2),(y2))
#define drawImageNoBlend(s,d,dx,dy,x2,y2) copyAreaNoBlend((s),(d),(dx),(dy),0,0,(x2),(y2))
#define drawImageOpaque(s,d,dx,dy,x2,y2) copyAreaNoBlend((s),(d),(dx),(dy),0,0,x2,(y2))

void rotate (TFRAME *src, TFRAME *des, const TMETRICS *metrics, double angle, const int w);
void rotateAroundZ (TFRAME *src, TFRAME *des, const TMETRICS *metrics, double angle);

void outlineTextEnable (THWD *hw, const int colour);
void outlineTextDisable (THWD *hw);
void shadowTextEnable (THWD *hw, const int colour, const unsigned char trans);
void shadowTextDisable (THWD *hw);

int drawStr (TFRAME *frame, const int x, const int y, const int font, const int colour, const char *str, const int nsex_justify);
int drawStrB (TFRAME *frame, const int x, const int y, const char *str, const int colour);
int drawStrFlt (TFRAME *frame, const int x, const int y, const char *str, const double var, const int colour);
int drawStrFltB (TFRAME *frame, const int x, const int y, const int font, const char *str, const double var);
int drawStrInt (TFRAME *frame, const int x, const int y, const char *str, const int var, const int colour);
int drawStrIntB (TFRAME *frame, const int x, const int y, const int font, const char *str, const int var);
int drawIntStr (TFRAME *frame, const int x, const int y, const int font, const int var, const char *str, const int colour);
void drawInt (TFRAME *frame, const int x, const int y, const int var, const int colour);
void drawHex (TFRAME *frame, const int x, const int y, const int font, const int var, const int colour);

int countChr (const char *str, const int chr);
int hexToInts (const char *_str, const char sep, int **val);
int hexToInt (const char *hex);
int hexToIntW (const wchar_t *hex);
int hexToInt2W (const wchar_t *hex, int *val1, int *val2);
int hexIntToInt2 (const char *hex, int *val1, int *val2);
int hexHexToInt2 (const char *hex, int *val1, int *val2);
int decToInt (const char *str);
char *intToHex (const int val);
int decToIntDouble (const char *str, int *val1, double *val2);

void drawShadowedImage (TFRAME *img1, TFRAME *dest, const int x, const int y, unsigned int colour, const int radius, const int offsetX, const int offsetY);
void drawShadowedImageAlpha (TFRAME *img1, TFRAME *dest, const int x, const int y, unsigned int colour, const int radius, const int offsetX, const int offsetY, const double alpha);

void drawShadowedImageComputed (TFRAME *img1, TFRAME *dest, TFRAME *img2, const int x, const int y, const int offsetX, const int offsetY);
void drawShadowedImageComputedScaled (TFRAME *img1, TFRAME *dest, TFRAME *img2, const int x, const int y, const int offsetX, const int offsetY, const float scale);
TFRAME *drawShadowedImageCreateBlurMask (TFRAME *img1, unsigned int colour, const int radius);


void drawShadowUnderlay (TVLCPLAYER *vp, TFRAME *frame, const int x, const int y, const int w, const int h, const int shadow);
void deleteShadows (TSHADOWUNDER *shadow);
void invalidateShadows (TSHADOWUNDER *shadow);

void drawLine_NB (TFRAME *frame, int x, int y, int x2, int y2, const int colour);

void clearFrame (TFRAME *frame);
void clearFrameColour (TFRAME *frame, const int colour);
void fillFrameColour (TFRAME *frame, const int colour);
void fillFrameAreaColour (TFRAME *frame, const int x1, const int y1, const int x2, const int y2, const int colour);
void setBackgroundColourIdx (TVLCPLAYER *vp, const int pageIdx, const int colIdx);
void setForegroundColourIdx (TVLCPLAYER *vp, const int pageIdx, const int colIdx);

int decodeURIEx (const char *arturl, const int len, char *buffer);
void *decodeURI (const char *arturl, const int len);
void *decodeURI_noprefix (const char *arturl, const int len);
int encodeURI (const char *in, char *out, const size_t len);

int doesFileExistW (const wchar_t *path);
int doesFileExist8 (const char *path8);
int isDirectoryW (const wchar_t *path);
int isDirectory (const char *path);
int isPlaylistW (const wchar_t *path);
int isPlaylist (const char *path);

//unsigned int generateHash (const void *data, const size_t dlen);
//int vaswprintf (wchar_t **result, const wchar_t *format, va_list args);
//int _asprintf (char **, const char *, ...);
//int vasprintf (char **, const char *, va_list);
void dbwprintf (TVLCPLAYER *vp, const wchar_t *str, ...);
void dbprintf (TVLCPLAYER *vp, const char *str, ...);
void dbprintfEx (TVLCPLAYER *vp, const int flags, const char *fmt, ...);
void dbwprintfEx (TVLCPLAYER *vp, const int flags, const wchar_t *fmt, ...);
void clearConsole (TVLCPLAYER *vp);


int stripOptionsW (wchar_t *path);

const char *aspectIdxToStr (const int idx);
int aspectStrToIdx (const char *str);
char *filterIdxToStr (const int idx);
wchar_t *filterIdxToStrW (const int idx);
int filterStrToIdx (const char *str);
char *sortIdxToStr (const int idx);
int sortStrToIdx (const char *str);

wchar_t *ellipsiizeStringPath (wchar_t *String, int DesiredCount);

int hasPathExt (const wchar_t *in, const wchar_t **/*restrict*/ exts);
int hasPathExtA (const char *path, const char **/*restrict*/ exts);

int hkModiferToKey (const char *name);


int isVideoFile (wchar_t *path);
int isMediaVideo (wchar_t *name);
int isAudioFile8 (const char *path8);
int isMediaAudio8 (const char *name8);
int isMediaVideo8 (const char *path8);
int isMediaImage8 (const char *name8);
int isMediaPlaylist8 (const char *path8);
int isDVDLocationW (const wchar_t *path);
int isDVDLocation (const char *path);
int isTsVideo8 (const char *path8);
int isAyFile8 (const char *name);
int isGaminFile8 (const char *name);

#if ENABLE_CMDFUNSTUFF
void botQuoteRandom (TVLCPLAYER *vp, TCMDREPLY *sheets, const int bot);
#endif


#if USE_MMX_MEMCPY

#ifdef USE_MMX
#define MIN_LEN 		(size_t)(64) 		 /* 64-byte blocks */
#define MMX1_MIN_LEN	(size_t)(64)

#ifndef USE_MMX2
#define _mmx_memcpy(to,from,len) \
{\
  if (len >= MMX1_MIN_LEN)\
  {\
    size_t __i = len >> 6;\
    len &= 63;\
    for(; __i>0; __i--)\
    {\
      __asm__ __volatile__ (\
      "movq (%0), %%mm0\n"\
      "movq 8(%0), %%mm1\n"\
      "movq 16(%0), %%mm2\n"\
      "movq 24(%0), %%mm3\n"\
      "movq 32(%0), %%mm4\n"\
      "movq 40(%0), %%mm5\n"\
      "movq 48(%0), %%mm6\n"\
      "movq 56(%0), %%mm7\n"\
      "movq %%mm0, (%1)\n"\
      "movq %%mm1, 8(%1)\n"\
      "movq %%mm2, 16(%1)\n"\
      "movq %%mm3, 24(%1)\n"\
      "movq %%mm4, 32(%1)\n"\
      "movq %%mm5, 40(%1)\n"\
      "movq %%mm6, 48(%1)\n"\
      "movq %%mm7, 56(%1)\n"\
      :: "r" (from), "r" (to) : "memory");\
      from += 64;\
      to += 64;\
    }\
	__asm__ __volatile__ ("sfence":::"memory");\
    __asm__ __volatile__ ("emms":::"memory");\
  }\
  if (len) memcpy(to, from, len);\
}
#endif

#endif

#ifdef USE_MMX2
#define _mmx2_memcpy(to,from,len) \
{\
	__asm__ __volatile__ (\
    "   prefetch (%0)\n"\
    "   prefetch 64(%0)\n"\
    : : "r" (from) );\
	if (len >= MIN_LEN){\
        size_t __i = len >> 6;\
    len &= 63;\
    for(; __i>0; __i--){\
      __asm__ __volatile__ (\
      "prefetch 64(%0)\n"\
      "movq (%0), %%mm0\n"\
      "movq 8(%0), %%mm1\n"\
      "movq 16(%0), %%mm2\n"\
      "movq 24(%0), %%mm3\n"\
      "movq 32(%0), %%mm4\n"\
      "movq 40(%0), %%mm5\n"\
      "movq 48(%0), %%mm6\n"\
      "movq 56(%0), %%mm7\n"\
      "movntq %%mm0, (%1)\n"\
      "movntq %%mm1, 8(%1)\n"\
      "movntq %%mm2, 16(%1)\n"\
      "movntq %%mm3, 24(%1)\n"\
      "movntq %%mm4, 32(%1)\n"\
      "movntq %%mm5, 40(%1)\n"\
      "movntq %%mm6, 48(%1)\n"\
      "movntq %%mm7, 56(%1)\n"\
      :: "r" (from), "r" (to) : "memory");\
      from += 64;\
      to += 64;\
    }\
    __asm__ __volatile__ ("sfence":::"memory");\
    __asm__ __volatile__ ("emms":::"memory");\
  }\
  if (len) memcpy(to, from, len);\
}

#define mmx_memcpy(_to,_from,_len)											\
{																			\
	unsigned char * restrict __to = (unsigned char * restrict)(_to);		\
  	unsigned char * restrict __from = (unsigned char * restrict)(_from);	\
  	size_t __len = (size_t)(_len);											\
	_mmx2_memcpy(__to,__from,__len);										\
}

#else
#define mmx_memcpy(_to, _from, _len) \
{\
  	unsigned char * restrict __to = (unsigned char * restrict)(_to);\
  	unsigned char * restrict __from = (unsigned char * restrict)(_from);\
	size_t __len = (size_t)(_len);\
 	if (__len < 128){\
		if (__len&3){\
			for (int i = 0; i < __len; i++)\
				__to[i] = __from[i];\
		}else{\
			unsigned int *restrict __src = (unsigned int *restrict)__from;\
			unsigned int *restrict __des = (unsigned int *restrict)__to;\
			__len >>= 2;\
			for (int i = 0; i < __len; i++)\
				__des[i] = __src[i];\
		}\
	}else{\
		_mmx_memcpy(__to,__from,__len);\
	}\
}
#endif

#endif	// #if USE_MMX_MEMCPY


// frame copy with vertical bound clipping. src alpha is copied (no blending)
#define fastFrameCopy(_src, _des, _dx, _dy) \
{\
	const TFRAME *const __src = (TFRAME*)(_src);\
	const TFRAME *__des = (TFRAME*)(_des);\
	int __dx = (int)(_dx);\
	int __dy = (int)(_dy);\
\
	const int __spitch = __src->pitch;\
	const int __dpitch = __des->pitch;\
	void * restrict __psrc;\
	int __dys = 0;\
\
	if (__dy < 0){\
		__dys = abs(__dy);\
		if (__dys >= __src->height) goto copyEnd;\
		__psrc = lGetPixelAddress(__src, 0, __dys);\
		__dy = 0;\
	}else{\
		__psrc = lGetPixelAddress(__src, 0, 0);\
	}\
\
	if (__dy > __des->height-1)\
		__dy = __des->height-1;\
	void *restrict __pdes = lGetPixelAddress(__des, __dx, __dy);\
\
	int __r = __src->height - __dys;\
	if (__dy + __r > __des->height-1){\
		__r = (__des->height - __dy) - 1;\
		if (__r > 1) __r++;\
	}\
\
	while(__r-- > 0){\
		my_memcpy(__pdes, __psrc, __spitch);\
		__psrc += __spitch;\
		__pdes += __dpitch;\
	}\
copyEnd:;\
}


#endif

