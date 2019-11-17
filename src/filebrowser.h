
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

#ifndef _FILEBROWSER_H_
#define _FILEBROWSER_H_


#define DRIVE_USB	(DRIVE_RAMDISK+32)




#define EXTAUDIO \
    L".mp3",\
    L".wav",\
    L".wma",\
    L".a52",\
    L".aac",\
    L".ac3",\
    L".flac",\
    L".mpa",\
    L".mid",\
    L".mka",\
    L".aiff",\
    L".amr",\
    L".aob",\
    L".ape",\
    L".ay",\
    L".dts",\
    L".it",\
    L".m4a",\
    L".m4p",\
    L".mlp",\
    L".mod",\
    L".mp1",\
    L".mp2",\
    L".mpc",\
    L".oga",\
    L".ogg",\
    L".oma",\
    L".rmi",\
    L".s3m",\
    L".sid",\
    L".spx",\
    L".tta",\
    L".voc",\
    L".vqf",\
    L".w64",\
    L".http",\
    L".wv",\
    L".xa",\
    L".xm"

// taken from vlc/vlc_interface.h
#define EXTVIDEO \
	L".avi",\
	L".mkv",\
	L".mp4",\
	L".mpg",\
	L".mpeg",\
	L".flv",\
	L".vob",\
	L".mov",\
	L".rmvb",\
	L".wmv",\
	L".ts",\
	L".3gp",\
	L".amv",\
	L".asf",\
	L".divx",\
	L".dv",\
	L".gxf",\
	L".iso",\
	L".ifo",\
	L".m1v",\
	L".m2v",\
	L".m2t",\
	L".m2ts",\
	L".m4v",\
	L".mp2",\
	L".mpeg1",\
	L".mpeg2",\
	L".mpeg4",\
	L".mts",\
	L".mxf",\
	L".nsv",\
	L".nuv",\
	L".ogg",\
	L".ogm",\
	L".ogv",\
	L".ogx",\
	L".ps",\
	L".rec",\
	L".rm",\
	L".tod",\
	L".tp",\
	L".vro",\
	L".webm"
  
             
#define EXTIMAGE \
	L".jpg",\
	L".png",\
	L".bmp",\
	L".psd",\
	L".tga",\
	L".gif",\
	L".pgm",\
	L".pnm",\
	L".ppm",\
	L".pbm",\
	L".jpe",\
	L".jpeg",\
	L".exe",\
	L".dll",\
	L".ico"

#define EXTPLAYLISTS \
	L".m3u8"


// taken from vlc/vlc_interface.h
#define EXTVIDEOA \
	".avi",\
	".mkv",\
	".wmv",\
	".mpg",\
	".mp4",\
	".mpeg",\
	".flv",\
	".ts",\
	".vob",\
	".mov",\
	".divx",\
	".dv",\
	".m1v",\
	".m2v",\
	".m2t",\
	".m2ts",\
	".m4v",\
	".mp2",\
	".tp",\
	".mpeg1",\
	".mpeg2",\
	".mpeg4",\
	".gxf",\
	".iso",\
	".3gp",\
	".amv",\
	".asf",\
	".mts",\
	".mxf",\
	".nsv",\
	".nuv",\
	".ogg",\
	".ogm",\
	".ogv",\
	".ogx",\
	".ps",\
	".rec",\
	".rm",\
	".rmvb",\
	".tod",\
	".vro",\
	".webm"

// taken from vlc/vlc_interface.h
#define EXTAUDIOA \
    ".mp3",\
    ".ogg",\
    ".wav",\
    ".wma",\
    ".aac",\
    ".ac3",\
    ".aiff",\
    ".dts",\
    ".flac",\
    ".m4a",\
    ".m4p",\
    ".mid",\
    ".mka",\
    ".a52",\
    ".ay",\
    ".mlp",\
    ".mod",\
    ".mp1",\
    ".mp2",\
    ".mpc",\
    ".oga",\
    ".oma",\
    ".rmi",\
    ".s3m",\
    ".sid",\
    ".spx",\
    ".it",\
    ".amr",\
    ".aob",\
    ".ape",\
    ".tta",\
    ".voc",\
    ".vqf",\
    ".w64",\
    ".http",\
    ".wv",\
    ".xa",\
    ".xm"

#define EXTIMAGEA \
	".jpg",\
	".png",\
	".bmp",\
	".psd",\
	".tga",\
	".gif",\
	".pgm",\
	".ppm",\
	".pnm",\
	".pbm",\
	".jpe",\
	".jpeg",\
	".exe",\
	".dll",\
	".ico"
	
#define EXTPLAYLISTSA \
	".m3u8"



#define SYMLINK_SHORTCUT	(1)
#define SYMLINK_MODULE		(2)


enum _fbsort
{	
	SORT_NOSORT,
	SORT_NAME_A,
	SORT_NAME_D,
	SORT_FILE_TYPE_A,
	SORT_FILE_TYPE_D,
	SORT_DATE_MODIFIED_A,
	SORT_DATE_MODIFIED_D,
	SORT_DATE_CREATION_A,
	SORT_DATE_CREATION_D,
	SORT_SIZE_FILE_A,
	SORT_SIZE_FILE_D,
	SORT_TOTAL,
	FILEPANE_SORT_TOTAL = SORT_TOTAL
};


typedef struct{
	char *dirComplete;		// complete path to and including this folder
	char *folder;			// name of this [top level] folder only
	char *dir;				// parent of above directory excluding drive
	char *drive;			// drive only (+colon);
	int folderType;			// FB_OBJTYPE_xxxx
}TDECOMPOSEDIR;


typedef struct{
	TDECOMPOSEDIR *dirs;
	int size;
	int total;
}TDECOMPOSEPATH;

typedef struct {
	unsigned int csidl;
	wchar_t *name;
	wchar_t *location;
}TSHELLFOLDEROBJS;

typedef struct {
	char drive[4];
	int driveType;
	int busType;
	int isSystemDrive;
		
	uint64_t totalNumberOfBytes;
    uint64_t totalNumberOfFreeBytes;
}TLOGICALDRIVE;

typedef struct{
	wchar_t *path;
	wchar_t *link;
	int type;
}TSTRSHORTCUT;


typedef struct{
	TSTRSHORTCUT **links;
	int total;
}TLINKSHORTCUTS;

typedef struct{
	int fileAttributes;
	uint64_t fileSize;				// file size
	uint64_t creationDate;
	uint64_t modifiedDate;
	int64_t focus;
}TFB_ITEM_DESC;


typedef struct{
	int rootId;
	char *rootName;
	TTREE *tree;
	int refCt;
	void *udata;
	
	int sortedMode;
	int filteredMode;
	int branchCount;
	int leafCount;
}TFB;

typedef struct TLISTITEM TFBITEM;




TFB *fbNew ();
int fbInit (TFB *fb, const char *rootName);
void fbClose (TFB *fb);
void fbRelease (TFB *fb);


int fbFindFiles (TFB *fb, const int nodeId, const wchar_t *path, const wchar_t *searchMask, const int *searchDepth, wchar_t **filesMasks);
int fbGetTotals (TFB *fb, const int id, int *tBranch, int *tLeaf);
int fbSort (TFB *fb, const int id, const int sortType);


// access methods
TFBITEM *fbGetFirst (TFB *fb);
TFBITEM *fbGetNext (TFB *fb, TFBITEM *item);
int fbIsLeaf (TFB *fb, TFBITEM *item);
int fbIsBranch (TFB *fb, TFBITEM *item);
char *fbGetName (TFB *fb, TFBITEM *item);
uint64_t fbGetFilesize (TFB *fb, TFBITEM *item);
//int fbGetId (TFB *fb, TFBITEM *item);
void fbSetUserData (TFB *fb, void *data);
void *fbGetUserData (TFB *fb);

void fbFormatSize (char *buffer, const uint64_t filesize);

TLOGICALDRIVE *fbGetLogicalDrives (int *tDrives);
void fbGetLogicalDrivesRelease (TLOGICALDRIVE *drives);

TSHELLFOLDEROBJS *fbGetShellFolders (int *total);
void fbGetShellFoldersRelease (TSHELLFOLDEROBJS *folders);


void fbShortcutsFree (TLINKSHORTCUTS *linksc);
int fbShortcutsGetTotal (TLINKSHORTCUTS *linksc);
TSTRSHORTCUT *fbShortcutsGet (TLINKSHORTCUTS *linksc, const int Idx);
int fbShortcutsAdd (TLINKSHORTCUTS *linksc, const char *path, const char *link, const int type);
int fbShortcutsAdd8 (TLINKSHORTCUTS *linksc, const char *path, const char *link);
int fbShortcutAddModule (TLINKSHORTCUTS *linksc, const char *module, const char *name);

wchar_t *fbGetSystem32Folder ();
wchar_t *fbGetSystem64Folder ();
wchar_t *fbGetWindowsFolder ();

char *fbGetVolumeLabel (const int drive);
char *fbGetMyComputerName ();
char *fbGetComputerName ();
int fbIsUsbDrive (const char drive);

TDECOMPOSEPATH *decomposePath (char *path);
void decomposePathFree (TDECOMPOSEPATH *decomp);

TTREEENTRY *fbGetEntry (TFB *fb, TFBITEM *item);

void fbOpenExplorerLocationA (char *path);
void fbOpenExplorerLocationW (wchar_t *path);


#endif


