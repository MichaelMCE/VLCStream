
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






typedef struct{
	int yRenderOffset;	// won't be required once TLABEL text render is fixed
	char *name;
}TLABELSTRINGS;


static const TLABELSTRINGS sortModes[] = {
	{0, "Unsorted"},
	{0, "Name \u02C4"},
	{0, "Name \u02C5"},
	{9, "Type \u02C4"},
	{9, "Type \u02C5"},
	{0, "Modified \u02C4"},
	{0, "Modified \u02C5"},
	{0, "Created \u02C4"},
	{0, "Created \u02C5"},
	{0, "Size \u02C4"},
	{0, "Size \u02C5"},
	{0, ""}
};


static const TLABELSTRINGS extFilter[] = {
	{0, "All"},
	{0, "Audio"},
	{0, "Video"},
	{0, "m3u8"},
	{9, "Image"},
	{0, "Media"},
	{0, ""}
};

static wchar_t *extAudio[] = {
	EXTAUDIO,
	L""
};

static wchar_t *extVideo[] = {
	EXTVIDEO,
	L""
};

static wchar_t *extPlaylists[] = {
	EXTPLAYLISTS,
	L""
};

static wchar_t *extImage[] = {
	EXTIMAGE,
	L""
};

static wchar_t *extMedia[] = {
	EXTAUDIO,
	EXTVIDEO,
#if 0
	EXTIMAGE,
	EXTPLAYLISTS,
#endif
	L""
};


typedef struct{
	const wchar_t *icon;
	int imgId;
	const char *exts[44];
}TFILETYPE;


static TFILETYPE extTypes[] = {
	
#if ENABLE_FILEEXT_CONFIG
	{L"pane/audio32.png", 	0, {EXTAUDIOA,""}},
	{L"pane/video32.png", 	0, {EXTVIDEOA,""}},
	{L"pane/image32.png", 	0, {EXTIMAGEA,""}},
	{L"pane/playlist32.png",0, {EXTPLAYLISTSA,""}},
#else
	{L"pane/midi32.png", 0, {".mid",""}},		// put this before EXTAUDIOA to override it
	{L"pane/audio32.png", 0, {EXTAUDIOA,""}},
	{L"pane/archive32.png", 0, {".iso",""}},
	{L"pane/video32.png", 0, {EXTVIDEOA,""}},
	{L"pane/image32.png", 0, {EXTIMAGEA,""}},
	{L"pane/module32.png", 0, {".exe",".msi",".msc",".cpl",".scr",".cmd",".elf",".vbs",".pif",""}},
	{L"pane/7zip32.png", 0, {".7z",""}},
	{L"pane/rar32.png", 0, {".rar",""}},
	{L"pane/zip32.png", 0, {".zip",""}},
	{L"pane/gzip32.png", 0, {".gzip",".gz",""}},
	{L"pane/text32.png", 0, {".txt",".log",".cfg",".doc",".xml",".srt",".nfo",".rtf",".ini",".csv",".conf",".docx",""}},
	{L"pane/code32.png", 0, {".c",".h",".cpp",".hpp",".def",".lua",".js",".x",".cs",".prj",".php",".asm",".sh",".pl",".dec",".sed",".tcl",".tck",".asc",".inc",""}},
	{L"pane/font32.png", 0, {".ttf",".otf",".bdf",".ttc",".pfm",".fon",".pcf",".sfd",".fnt",".otb",".pgf",".font",""}},
	{L"pane/moduledll32.png", 0, {".dll",".sys",".drv",".vxd",".acm",".ax",".ocx",""}},
	{L"pane/archive32.png", 0, {".001",".002",".003",".cab",".lzma",".tgz",".tar",".ace",".dmg",".bz2",".bzip",".bzip2",".z",".pk",".pkz",".xz",".arj",".xz",".taz",".lha",".lzh",".taz",".tpz",".tbz",".tbz2",""}},
	{L"pane/java32.png",0, {".java",".jar",".class",""}},
	{L"pane/gpx32.png", 0, {".gpx",""}},
	{L"pane/fit32.png", 0, {".fit",""}},
	{L"pane/trk32.png", 0, {".trk",""}},
	{L"pane/tcx32.png", 0, {".tcx",""}},
	{L"pane/garmin32.png", 0, {".hst",".nlf",""}},
	{L"pane/excel32.png", 0, {".xls",".xlt",".xlsx",".xlsb",".xlsm",".xltx",".xltm",""}}, 
	{L"pane/url32.png", 0, {".url",""}},
	{L"pane/html32.png", 0, {".htm",".html",".css",""}},
	{L"pane/dos32.png", 0, {".com",".bat",""}},
	{L"pane/python32.png", 0, {".py",""}},
	{L"pane/unhandledimage32.png", 0, {".tif",".tiff",".dib",".sgi",".rgb",".icn",".ico",".pic",".psp",".xif",".wmf",".xbm",".xpm",""}},
	{L"pane/reg32.png", 0, {".reg",".key",""}},
	{L"pane/visualstudio32.png", 0, {".vcproj",".dsp",".dsw",".csproj",".vcxproj",""}},
	{L"pane/codeblocks32.png", 0, {".cbp",""}},
	{L"pane/adobeps32.png", 0, {".psd",".psb",".pdd",".grd",".abr",""}},
	{L"pane/adobepdf32.png", 0, {".pdf",""}},
	{L"pane/adobepsstyle32.png", 0, {".asl",""}},
	{L"pane/adobepsshape32.png", 0, {".csh",""}},
	{L"pane/adobeeps32.png",0, {".eps",""}},
	{L"pane/adobeai32.png", 0, {".ai",""}},
	{L"pane/adobefw32.png", 0, {".fw",""}},
	{L"pane/adobedw32.png", 0, {".dw",""}},
	{L"pane/adobeae32.png", 0, {".ae",""}},
	{L"pane/adobefl32.png", 0, {".fl",".fla",""}},
	{L"pane/adobeswf32.png", 0, {".swf",""}},
	{L"pane/playlist32.png", 0, {EXTPLAYLISTSA,""}},
	{L"pane/link32.png", 0, {".lnk",".link",""}},
	{L"pane/stuffit32.png", 0, {".sit",".sitx",""}},
	{L"pane/audacity32.png", 0, {".aud",""}},
	{L"pane/android32.png", 0, {".apk",""}},
	{L"pane/utorrent32.png", 0, {".ut",".utorrent",".torrent",".bt",""}},
	{L"pane/devc32.png", 0, {".dev",".pak",""}},
	{L"pane/ppt32.png", 0, {".ppt",".pptx",""}},
	{L"pane/svg32.png", 0, {".svg",".cad",""}},
	{L"pane/model32.png", 0, {".dae",".obj",".u3d",".kmz",""}},
	{L"pane/autocad32.png", 0, {".dwg",".dxf",".dwf",".dwt",""}},
	{L"pane/autodesk32.png", 0, {".max",".3ds",".blend",""}},
	{L"pane/help32.png", 0, {".hlp",".chm",""}},
	{L"pane/vlc32.png", 0, {".xspf",""}},
	{L"pane/quake32.png", 0, {".pk3",".pk4",".bsp",".cin",".md2",".md3",".dm2",".dm3",".wad",""}},
	{L"pane/fits32.png", 0, {".fits",".sif",""}},
	{L"pane/razer32.png", 0, {".rzr",""}},
#endif	
	{L" ", -1, {""}}
};


static inline int hasPathExt8 (const char *pathExt, const char **restrict exts)
{
	for (int i = 0; *exts[i] && *exts[i] == '.'; i++){
		if (!stricmp(pathExt, exts[i]))
			return 1;
	}
	return 0;
}

#if ENABLE_FILEEXT_CONFIG
static inline int filepaneFiletypeExtsToIcon (TFILEEXT *exts, const char *name)
{
	for (int i = 0; i < exts->total; i++){
		TFILEEXTLINE *extList = exts->exts[i];
		for (int j = 0; j < extList->total; j++){
			if (!stricmp(extList->list[j], name))
				return extList->imgId;
		}
	}
	return 0;
}

static inline int filepaneRegisterExtsIcons (TFILEPANE *filepane, TFILEEXT *exts)
{
	TVLCPLAYER *vp = filepane->com->vp;
	wchar_t bufferw[MAX_PATH+1];
	
	wchar_t *folder = NULL;
	cfg_keyGetW(exts->config, "ext.folder", &folder);
	if (!folder) return 0;
	
	int ct = 0;
	for (int i = 0; i < exts->total; i++){
		TFILEEXTLINE *extList = exts->exts[i];
		wchar_t *path = buildSkinDEx(vp, bufferw, folder, extList->image);
		extList->imgId = artManagerImageAdd(vp->am, path);
		ct += (extList->imgId != 0);
	}

	my_free(folder);

	return ct;
}

#endif

static inline int filepaneFiletypeToIcon (TFILEPANE *filepane, const char *name)
{
	const char *fileExt = strrchr(name, '.');
	if (!fileExt) return filepane->icons.other;
	
#if ENABLE_FILEEXT_CONFIG
	int imgId = filepaneFiletypeExtsToIcon(filepane->exts, fileExt);
	if (imgId > 0) return imgId;
#endif

	for (int i = 0; extTypes[i].imgId != -1; i++){
		if (hasPathExt8(fileExt, extTypes[i].exts))
			return extTypes[i].imgId;
	}

	return filepane->icons.other;
}

static inline void filePaneRegisterIcons (TFILEPANE *filepane)
{
	TVLCPLAYER *vp = filepane->com->vp;
	wchar_t bufferw[MAX_PATH+1];
	
	filepane->icons.folder = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/folder32.png"));
	filepane->icons.back = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/back32.png"));
	filepane->icons.other = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/other32.png"));
	//filepane->icons.closePane = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/close32.png"));
	filepane->icons.drive = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/drive32.png"));
	filepane->icons.driveSystem = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/drivesystem32.png"));
	filepane->icons.driveUSB = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/driveusb32.png"));
	filepane->icons.driveRemote = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/driveremote32.png"));
	filepane->icons.driveNoMedia = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/drivenomedia32.png"));
	filepane->icons.driveRemovable = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/driveremovable32.png"));
		
	for (int i = 0; extTypes[i].imgId != -1; i++)
		extTypes[i].imgId = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,(wchar_t*)extTypes[i].icon));
		
#if ENABLE_FILEEXT_CONFIG
	filepaneRegisterExtsIcons(filepane, filepane->exts);
#endif
	filepane->icons.areRegistered = 1;
}

int filepaneBuildPlaylistDir (TFILEPANE *filepane, PLAYLISTCACHE *plc, wchar_t *pathw, const int filterMode, const int recursive)
{

	int slen = wcslen(pathw);
	wchar_t location[slen+2];
		
	if (pathw[slen-1] != L'\\')
		__mingw_snwprintf(location, MAX_PATH, L"%ls\\", pathw);
	else
		__mingw_snwprintf(location, MAX_PATH, L"%ls", pathw);

	//wprintf(L"filepaneBuildPlaylistDir #%s#\n", location);
	
	int total = 0;
	int count = 0;
	int depth = 1;
		
	TFB *fb = fbNew();
	fbInit(fb, "playlist_dir");
	
	total = fbFindFiles(fb, fb->rootId, location, L"*.*", &depth, filepane->filterMasks[filterMode]);
	if (!total){
		fbRelease(fb);
		return 0;
	}
	
	fbSort(fb, fb->rootId, filepane->sortMode);

	TFBITEM *item = fbGetFirst(fb);
	if (item){
		char *location8 = convertto8(location);
		char path[MAX_PATH_UTF8+1];

		while(item){
			if (fbIsLeaf(fb, item)){
				snprintf(path, MAX_PATH_UTF8, "%s%s", location8, fbGetName(fb, item));
				int position = playlistAdd(plc, path);
				if (position >= 0){
					char *title = fbGetName(fb,item);
					playlistSetTitle(plc, position, title, 1);
					unsigned int hash = getHash(path);
					tagAddByHash(filepane->com->vp->tagc, hash, MTAG_PATH, path, 0);
					tagAddByHash(filepane->com->vp->tagc, hash, MTAG_FILENAME, title, 0);
					tagAddByHash(filepane->com->vp->tagc, hash, MTAG_Title, title, 0);
					count++;
				}
			}else if (recursive && fbIsBranch(fb, item)){
				PLAYLISTCACHE *plcN = playlistManagerCreatePlaylist(filepane->com->vp->plm, fbGetName(fb, item), 0);
				if (plcN){
					wchar_t buffer[MAX_PATH_UTF8+1];
					wchar_t *name = converttow(fbGetName(fb, item));
					if (name){
						__mingw_snwprintf(buffer, MAX_PATH, L"%ls%ls", location, name);
						my_free(name);
						count += filepaneBuildPlaylistDir(filepane, plcN, buffer, filterMode, recursive);
						
						if (!playlistGetTotal(plcN))
							playlistManagerDeletePlaylist(filepane->com->vp->plm, plcN, 1);
						else
							playlistAddPlc(plc, plcN);
					}
				}
			}
			item = fbGetNext(fb, item);
		}
		
		my_free(location8);
	}
	
	fbRelease(fb);
	return count;
}


/*
#############################################################################################
#############################################################################################
#############################################################################################
#############################################################################################
*/

static inline TFB *filepaneGetPaneFB (TFILEPANE *filepane)
{
	intptr_t data = 0;
	stackPeek(filepane->stack, &data);	// active directory will always be top most
	return (TFB*)data;
}

static inline void buildTitleBar (TFILEPANE *filepane, TLABEL *title)
{
	char *name;
	TFB *fb = filepaneGetPaneFB(filepane);
	if (fb)
		name = fb->rootName;
	else
		name = "  ";
	
	int itemId = ccGetUserDataInt(title);
	if (!itemId){
		itemId = labelTextCreate(title, name, 0, PANE_TITLE_FONT, 0, 5);
		ccSetUserDataInt(title, itemId);
		labelRenderFilterSet(title, itemId, 0);
		labelStringRenderFlagsSet(title, itemId, PF_MIDDLEJUSTIFY);
		//labelRenderColourSet(title, itemId, 240<<24 | COL_BLACK, 200<<24 | COL_BLUE_SEA_TINT, (180<<24) | COL_WHITE);
		labelRenderColourSet(title, itemId, 250<<24 | COL_WHITE, 200<<24 | COL_BLUE_SEA_TINT, 140<<24 | COL_BLACK);
	}else{
	 	labelStringSet(title, itemId, name);
	}

	labelItemDataSet(title, itemId, 0);
}

static inline void buildLocBar (TFILEPANE *filepane, TLABEL *bar)
{
	labelItemsDelete(bar);
	
	int length;
	TFB **stack = stackCopyPtr(filepane->stack, &length);
	if (!stack || !length) return;
	
	//printf("locBar %p %i\n", stack, length);
	
	if (length < 2){		// my computer
		char *name = filepane->myComputerName;

		int itemId = labelTextCreate(bar, name, 0, PANE_LOCBAR_FONT, 0, 0);
		labelRenderFilterSet(bar, itemId, 0);
		labelStringRenderFlagsSet(bar, itemId, PF_MIDDLEJUSTIFY);
		labelItemDataSet(bar, itemId, FB_OBJTYPE_MYCOM<<16);
		labelRenderColourSet(bar, itemId, 255<<24 | COL_WHITE, 255<<24 | COL_BLUE_SEA_TINT, (177<<24) | COL_BLUE_SEA_TINT);
		labelItemPositionSet(bar, itemId, 0, -1);
	}
	
	if (!stack || !length) return;

	for (int i = 0; i < length; i++) stack[i]->refCt++;
	
	const char *sep = "\\";
	int x = 0;
	int y = -1;
	
	for (int i = 0; i < length-1; i++){
		TFB *fb = stack[i];
		//printf("@@ %i, #%s#\n", i, fb->rootName);

		char *name = my_strdup(fb->rootName);
		int len = strlen(name);
		if (name[len-1] == '\\') name[len-1] = 0;
			
		int id = i;
		if (name[1] == ':')
			id = (FB_OBJTYPE_DRIVE<<16) | (i&0xFFFF); //name[0];
		else
			id = (FB_OBJTYPE_FOLDER<<16) | (i&0xFFFF);
			
		int itemId = labelTextCreate(bar, name, 0, PANE_LOCBAR_FONT, 0, 0);
		labelRenderFilterSet(bar, itemId, 0);
		labelStringRenderFlagsSet(bar, itemId, PF_LEFTJUSTIFY);
		labelItemDataSet(bar, itemId, id);
		labelRenderColourSet(bar, itemId, 255<<24 | COL_WHITE, 255<<24 | COL_BLUE_SEA_TINT, (177<<24) | COL_BLUE_SEA_TINT);
		my_free(name);
		
		int width;
		labelStringGetMetrics(bar, itemId, NULL, NULL, &width, NULL);
		labelItemPositionSet(bar, itemId, x, y);
		x += width;

		
		itemId = labelTextCreate(bar, sep, 0, PANE_LOCBAR_FONT, 0, 0);
		labelRenderFilterSet(bar, itemId, 0);
		labelStringRenderFlagsSet(bar, itemId, PF_LEFTJUSTIFY);
		labelItemDataSet(bar, itemId, id);
		labelRenderColourSet(bar, itemId, 255<<24 | COL_WHITE, 255<<24 | COL_BLUE_SEA_TINT, (177<<24) | COL_YELLOW);
		
		labelStringGetMetrics(bar, itemId, NULL, NULL, &width, NULL);
		labelItemPositionSet(bar, itemId, x, y-1);
		x += width;

	}
	
	
	for (int i = 0; i < length; i++) stack[i]->refCt--;
	my_free(stack);
}

static inline void filepaneSetSortLabel (TFILEPANE *filepane, const int mode)
{
	labelItemPositionSet(filepane->sortLabel, filepane->sortStrId, 0, sortModes[mode].yRenderOffset-4);
	labelStringSet(filepane->sortLabel, filepane->sortStrId, sortModes[mode].name);
}

void filepaneSetSortMode (TFILEPANE *filepane, const int mode)
{
	//printf("filepaneSetSortMode %i\n", mode);
	filepane->sortMode = mode;
	filepaneSetSortLabel(filepane, filepane->sortMode);
}

static inline void filepaneSetFilterLabel (TFILEPANE *filepane, const int type)
{
	labelItemPositionSet(filepane->filterLabel, filepane->filterStrId, 0, extFilter[type].yRenderOffset-4);
	labelStringSet(filepane->filterLabel, filepane->filterStrId, extFilter[type].name);
}

void filepaneSetFilterMask (TFILEPANE *filepane, const int type)
{
	//printf("filepaneSetFilterMask %i\n", type);
	filepane->filterMask = type;
	filepaneSetFilterLabel(filepane, filepane->filterMask);
}

static inline void filepaneReleaseFB (TFB *fb)
{
	if (fb->refCt == 1){
		void *data = fbGetUserData(fb);
		fbSetUserData(fb, NULL);
		if (data) my_free(data);
	}
	fbRelease(fb);
}

static inline void filepaneStackDestroy (TFILEPANE *filepane)
{
	stackDestroy(filepane->stack);
}

static inline void filepaneStackEmpty (TFILEPANE *filepane)
{
	intptr_t data = 0;
	while (stackPop(filepane->stack, &data))
		filepaneReleaseFB((TFB*)data);
}

static inline void filepaneStackRewind (TFILEPANE *filepane, void *until)
{
	intptr_t data = 0;
	while (stackPeek(filepane->stack, &data)){
		if (data != (intptr_t)until){
			if (stackPop(filepane->stack, &data)){
				filepaneReleaseFB((TFB*)data);
			}else{
				return;
			}
		}else{
			return;
		}
	};
}

static inline void driveSpaceFormat (char *buffer, const char *drive, const uint64_t free64, const uint64_t total64)
{
	double free = free64/1024.0/1024.0/1024.0;
	double total = total64/1024.0/1024.0/1024.0;
	char format[32]; // =  "%s  (%.1f : %.1f)";

	format[0] = 0;
	strcat(format, "%s  [");
	if (free >= 99)
		strcat(format, "%.0f / ");
	else if (free < 1){
		strcat(format, "%3.0fm / ");
		free = free64/1024.0/1024.0;
	}else if (free < 10)
		strcat(format, "%.2f / ");
	else
		strcat(format, "%.1f / ");
		
	if (total >= 99)
		strcat(format, "%.0f]");
	else if (total < 1){
		strcat(format, "%3.0fm]");
		total = total64/1024.0/1024.0;
	}else if (total < 10)
		strcat(format, "%.2f]");
	else
		strcat(format, "%.1f]");

	__mingw_snprintf(buffer, MAX_PATH, format, drive, free, total);
}

static inline int filepaneAddLogicalDrives (TFILEPANE *filepane)
{
	TPANE *pane = filepane->pane;

	//printf("@@ filepaneAddLogicalDrives\n");
	
	filepane->drives.total = 0;
	if (filepane->drives.list) fbGetLogicalDrivesRelease(filepane->drives.list);
	filepane->drives.list = fbGetLogicalDrives(&filepane->drives.total);
	char drive[MAX_PATH_UTF8+1];

	for (int i = 0; i < filepane->drives.total; i++){
		//double free = filepane->drives.list[i].totalNumberOfFreeBytes/1024.0/1024.0/1024.0;
		uint64_t total = filepane->drives.list[i].totalNumberOfBytes;
		
		if (!total)
			strcpy(drive, filepane->drives.list[i].drive);
		else
			driveSpaceFormat(drive, filepane->drives.list[i].drive, filepane->drives.list[i].totalNumberOfFreeBytes, filepane->drives.list[i].totalNumberOfBytes);

		int imgId = filepane->icons.drive;
		if (filepane->drives.list[i].isSystemDrive){
			imgId = filepane->icons.driveSystem;
		}else if (filepane->drives.list[i].driveType == DRIVE_USB){
			if (total)
				imgId = filepane->icons.driveUSB;
			else
				imgId = filepane->icons.driveNoMedia;
		}else if (filepane->drives.list[i].driveType == DRIVE_REMOVABLE){
			if (total)
				imgId = filepane->icons.driveRemovable;
			else
				imgId = filepane->icons.driveNoMedia;
		}else if (filepane->drives.list[i].driveType == DRIVE_REMOTE){
			imgId = filepane->icons.driveRemote;
		}
		paneTextAdd(pane, imgId, 0.0, drive, PANE_FONT, (FB_OBJTYPE_DRIVE<<16)|(filepane->drives.list[i].drive[0]));
	}

	return filepane->drives.total;
}

static inline char *filepaneFBGetItem (TFB *fb, const int idx)
{
	int addCount = 0;
	TFBITEM *item = fbGetFirst(fb);
	while (item){
		if (addCount++ == idx){
			/*TTREEENTRY *entry = fbGetEntry(fb, item);
			TFB_ITEM_DESC *desc = treeEntryGetStorage(entry);
			printf("%i, %I64d %I64d %I64d\n", entry->id, desc->fileSize, desc->creationDate, desc->modifiedDate);
			*/
			char *str = fbGetName(fb,item);
			if (str)
				return my_strdup(str);
			else
				return NULL;
		}
		item = fbGetNext(fb, item);
	}
	return NULL;
}

static inline int filepaneAddFB (TFILEPANE *filepane, TFB *fb)
{
	TPANE *pane = filepane->pane;
	
	//printf("@@ filepaneAddFB'\n");
	
	if (!filepane->icons.areRegistered)
		filePaneRegisterIcons(filepane);
	
#if 1
	paneTextAdd(pane, filepane->icons.back, 0.0, " ..     ", PANE_FONT, (FB_OBJTYPE_BACK<<16));
#endif

	char nameOnly[MAX_PATH_UTF8+1];
	char buffer[MAX_PATH_UTF8+1];
	int addCount = 0;
	int addLeafCount = 0;
	int addBranchCount = 0;
	char *text;

	//printf("filepaneAddFB -> '%s' ", fb->rootName);
	//double t0 = getTime(filepane->com->vp);
	
	
	int showFileSize = 0;
	settingsGet(pane->cc->vp, "browser.showFileSize", &showFileSize);
		
	TFBITEM *item = fbGetFirst(fb);
	while (item){
		char *name = fbGetName(fb, item);
		
		if (fbIsLeaf(fb, item)){
			addLeafCount++;
			addCount++;

			if (!filepane->showExtensions){
				_splitpath(name, NULL, NULL, nameOnly, NULL);
				text = nameOnly;
			}else{
				text = name;
			}

			char *filename = text;
			if (showFileSize){
				char sizeStr[32];
				fbFormatSize(sizeStr, fbGetFilesize(fb,item));
				__mingw_snprintf(buffer, MAX_PATH_UTF8, "%s (%s)", text, sizeStr);
				filename = buffer;
			}

			int imgId = filepaneFiletypeToIcon(filepane, name);
			paneTextAdd(pane, imgId, 0.0, filename, PANE_FONT, (FB_OBJTYPE_FILE<<16)|addCount);
			
		}else if (fbIsBranch(fb, item)){
			addBranchCount++;
			addCount++;
			paneTextAdd(pane, filepane->icons.folder, 0.0, name, PANE_FONT, (FB_OBJTYPE_FOLDER<<16)|addCount);
		}
		item = fbGetNext(fb, item);
	}

	if (addCount > 12){
		addCount++;
		paneTextAdd(pane, filepane->icons.back, 0.0, " ..     ", PANE_FONT, (FB_OBJTYPE_BACK<<16)|addCount);
	}
	
#if 0
	addCount++;
	paneTextAdd(pane, filepane->icons.closePane, 0.0, "       ", PANE_FONT, (FB_OBJTYPE_BACKRIGHT<<16));
#endif

	//double t1 = getTime(filepane->com->vp);
	//printf("%.2f\n", t1-t0);
	
	fb->branchCount = addBranchCount;
	fb->leafCount = addLeafCount;
	return addLeafCount+addBranchCount;
}

static inline TFB *filepaneCreatePath (TFILEPANE *filepane, const char *name, const char *path)
{
	//printf("filepaneCreatePath '%s' '%s'\n", name, path);
	
	wchar_t *pathw = converttow(path);
	if (!pathw) return 0;

	TFB *fb = fbNew();
	fbInit(fb, name);
	
	int depth = 0;

	fbSetUserData(fb, my_strdup(path));

	fbFindFiles(fb, fb->rootId, pathw, L"*.*", &depth, filepane->filterMasks[filepane->filterMask]);
	fbSort(fb, fb->rootId, filepane->sortMode);
	
	fb->sortedMode = filepane->sortMode;
	fb->filteredMode = filepane->filterMask;
	
	my_free(pathw);

	return fb;
}

static inline void filepaneEmpty (TFILEPANE *filepane)
{
	TPANE *pane = filepane->pane;
	
	paneRemoveAll(pane);
	pane->offset.x = 0;
	pane->offset.y = 0;
}

static inline int filepaneImport_File (TFILEPANE *filepane, PLAYLISTCACHE *plc, char *path, char *title)
{
	int position = playlistAdd(plc, path);
	if (position >= 0){
		if (!title) title = path;
		playlistSetTitle(plc, position, title, 1);
		unsigned int hash = getHash(path);
		tagAddByHash(filepane->com->vp->tagc, hash, MTAG_PATH, path, 0);
		tagAddByHash(filepane->com->vp->tagc, hash, MTAG_Title, title, 0);

		dbprintf(filepane->com->vp, "Track %i added: %s", position+1, title);
	}
	return position;
}

static inline int filepaneImport_Folder (TFILEPANE *filepane, PLAYLISTCACHE *plcParent, char *name, const int includeSubDir)
{
	//printf("filepaneImport_Folder '%s'\n", name);

	TVLCPLAYER *vp = filepane->com->vp;

	const int playlistExists = (playlistManagerGetPlaylistByName(vp->plm, name) != NULL);
	if (playlistExists){
		dbprintf(vp, "Can not import folder to an existing playlist with the same name in to the same location");
		return 0;
	}
	
	PLAYLISTCACHE *plc = playlistManagerCreatePlaylist(vp->plm, name, 1);
	if (!plc->parent){	// should always be NULL anyway
		if (plcParent)
			playlistAddPlc(plcParent, plc);
		else
			playlistAddPlc(getPrimaryPlaylist(vp), plc);
	}

	//playlistDeleteRecords(plc);	// if pre-existing, remove tracks already in playlist

	if (plc->parent)
		plc->parent->pr->selectedItem = playlistManagerGetPlaylistIndex(vp->plm, plc);
	
	int total = 0;

	wchar_t *folder = converttow(name);
	if (folder){
		total = filepaneBuildPlaylistDir(filepane, plc, folder, filepane->filterMask, includeSubDir);
		my_free(folder);
	}

	if (total > 0)
		dbprintf(vp, "Playlist created with %i items: (%X) %s", total, plc->uid, name);
	else
		dbprintf(vp, "Empty playlist generated: (%X) %s", plc->uid, name);

	return total;
}

void filepanePlay_DoPlay (TFILEPANE *filepane, char *path)
{

	TVLCPLAYER *vp = filepane->com->vp;
	playlistResetRetries(vp);
	
	char drive[MAX_PATH_UTF8+1];
	char dir[MAX_PATH_UTF8+1];
	char name[MAX_PATH_UTF8+1];
	_splitpath(path, drive, dir, NULL, NULL);
	__mingw_snprintf(name, strlen(path), "%s%s", drive, dir);

	const int playlistExists = (playlistManagerGetPlaylistByName(vp->plm, name) != NULL);
	PLAYLISTCACHE *plc = playlistManagerCreatePlaylist(vp->plm, name, 1);
	if (!plc->parent)
		playlistAddPlc(getPrimaryPlaylist(vp), plc);

	playlistDeleteRecords(plc);	// if pre-existing, remove tracks already in playlist

	tagFlushOrfhensPlc(vp->tagc, plc);	// shouldn't be here
	setDisplayPlaylist(vp, plc);
	setQueuedPlaylist(vp, plc);
	
	if (plc->parent)
		plc->parent->pr->selectedItem = playlistManagerGetPlaylistIndex(vp->plm, plc);

	setPlaybackMode(vp, isMediaVideo8(path) == 0);
	if (isMediaVideo8(path))
		page2Set(vp->pages, PAGE_NONE, 1);

	int trkStart = 0;
	if (isMediaScreen(path)){
		playlistSetName(plc, path);
		trkStart = playlistAdd(plc, path);
		playlistSetTitle(plc, trkStart, "Desktop", 0);
	}else if (isMediaDShow(path)){
		playlistSetName(plc, path);
		trkStart = playlistAdd(plc, path);
		playlistSetTitle(plc, trkStart, "DirectShow", 0);
	}else if (isMediaHTTP(path)){
		trkStart = playlistAdd(plc, path);
		playlistSetTitle(plc, trkStart, "HTTP", 0);
	}else if (isMediaMMS(path)){
		trkStart = playlistAdd(plc, path);
		playlistSetTitle(plc, trkStart, "MMS", 0);
	}else if (isMediaUDP(path)){
		trkStart = playlistAdd(plc, path);
		playlistSetTitle(plc, trkStart, "UDP", 0);
	}else if (isMediaRTSP(path)){
		trkStart = playlistAdd(plc, path);
		playlistSetTitle(plc, trkStart, "RTSP", 0);
	}else if (isMediaDVD(path)){
		trkStart = playlistAdd(plc, path);
		playlistSetTitle(plc, trkStart, "DVD", 0);
	}else if (isMediaDVB(path)){
		trkStart = playlistAdd(plc, path);
		playlistSetTitle(plc, trkStart, "DVB", 0);
	}else{
		wchar_t *folder = converttow(name);
		int total = filepaneBuildPlaylistDir(filepane, plc, folder, filepane->filterMask, 0);
		my_free(folder);
		
		if (total){
			trkStart = playlistGetPositionByHash(plc, getHash(path));
		}else{
			if (trkStart >= 0)
				playlistSetTitle(plc, trkStart, name, 1);
		}
	}
	if (trkStart >= 0){
		tagAdd(vp->tagc, path, MTAG_PATH, path, 1);
		int pos = setPlaylistPlayingItem(vp, plc, trkStart, playlistGetHash(plc, trkStart));
		if (pos >= 0) startPlaylistTrack(vp, plc, pos);

		if (!playlistExists)
			dbprintf(vp, "Playlist generated (%X)", plc->uid);
		else
			dbprintf(vp, "Playlist updated");

		timerSet(vp, TIMER_PLYTV_REFRESH, 100);
	}
}

int filepaneSetPath (TFILEPANE *filepane, char *path)
{
	if (!ccLock(filepane->pane)) return 0;
	int total = 0;
		
	TDECOMPOSEPATH *decomp = decomposePath(path);
	if (decomp){
		if (decomp->total){
			
			filepaneEmpty(filepane);
			filepaneStackEmpty(filepane);
			
			int start = 0;
			if (decomp->dirs[0].folderType == FB_OBJTYPE_DRIVE){
				TFB *fb = filepaneCreatePath(filepane, decomp->dirs[0].drive, decomp->dirs[0].drive);
				stackPush(filepane->stack, (intptr_t)fb);
				
				start++;
				total++;
			}

			for (int i = start; i < decomp->total; i++){
				if (decomp->dirs[i].folderType == FB_OBJTYPE_FOLDER){
					TFB *fb = filepaneCreatePath(filepane, decomp->dirs[i].folder, decomp->dirs[i].dirComplete);
					//filepaneAddFB(filepane, fb);
					stackPush(filepane->stack, (intptr_t)fb);
					total++;
				}
			}
			
			if (start){
				filepaneEmpty(filepane);
				TFB *fb = filepaneGetPaneFB(filepane);
				if (fb)
					filepaneAddFB(filepane, fb);
			}

			buildLocBar(filepane, filepane->locBar);
			buildTitleBar(filepane, filepane->title);
		}
		decomposePathFree(decomp);
	}
	
	ccUnlock(filepane->pane);
	return total;
}


static inline void filepaneSelection_Media (TFILEPANE *filepane, TFB *fb, const char *filename, const char *completePath, const wchar_t *folder)
{
	//printf("media\n");
	filepanePlay_DoPlay(filepane, (char*)completePath);
}

static inline void filepaneSelection_Audio (TFILEPANE *filepane, TFB *fb, const char *filename, const char *completePath, const wchar_t *folder)
{
	//printf("audio\n");
	
	filepaneSelection_Media(filepane, fb, filename, completePath, folder);
}

static inline void filepaneSelection_Video (TFILEPANE *filepane, TFB *fb, const char *filename, const char *completePath, const wchar_t *folder)
{
	//printf("video\n");
	
	filepaneSelection_Media(filepane, fb, filename, completePath, folder);
}

static inline void filepaneSelection_Image (TFILEPANE *filepane, TFB *fb, const char *filename, const char *completePath, const wchar_t *folder)
{
	TIMGPANE *imgpane = pageGetPtr(filepane->com->vp, PAGE_IMGPANE);
	imgPaneEmpty(imgpane);
	
	// open folder 'folder'
	// find then focus on image 'filename'
	//printf("filename '%s'\n", filename);
	//wprintf(L"folder '%s'\n", folder);
	int found = imgPaneAddPath(imgpane, imgpane->pane, folder, filename, imgpane->nameToggleState, 0.35);
	imgPaneSetImageFocus(imgpane, found);

	pageSet(filepane->com->vp, PAGE_IMGPANE);
}

void filepaneViewImage (TFILEPANE *filepane, wchar_t *path)
{
	char *path8 = convertto8(path);
	if (path8){
		char filename[MAX_PATH_UTF8+1];
		char ext[MAX_PATH_UTF8+1];
		char file[MAX_PATH_UTF8+1];
		_splitpath(path8, NULL, NULL, filename, ext);
		__mingw_snprintf(file, MAX_PATH, "%s%s", filename, ext);

		wchar_t drive[MAX_PATH+1];
		wchar_t folder[MAX_PATH+1];
		wchar_t folderw[MAX_PATH+1];
		_wsplitpath(path, drive, folder, NULL, NULL);
		__mingw_snwprintf(folderw, MAX_PATH, L"%ls%ls", drive, folder);
		
		filepaneSelection_Image(filepane, NULL, file, NULL, folderw);
		my_free(path8);
	}
}


static inline void filepaneSelection_Playlist (TFILEPANE *filepane, TFB *fb, const char *filename, const char *completePath, const wchar_t *folder)
{
	//printf("playlist\n");
	
	int total = 0;
	TVLCPLAYER *vp = filepane->com->vp;
	
	PLAYLISTCACHE *plc = playlistManagerCreatePlaylist(vp->plm, filename, 0);
	if (plc){
		total = importPlaylist(vp->plm, plc, vp->tagc, vp->am, completePath, filepane);
		resetCurrentDirectory();
		if (!playlistGetTotal(plc)){
			playlistManagerDeletePlaylist(vp->plm, plc, 1);
		}else{
			playlistAddPlc(getPrimaryPlaylist(vp), plc);
			if (plc->parent)
				plc->parent->pr->selectedItem = playlistManagerGetPlaylistIndex(vp->plm, plc);
		}
	}
	
	dbprintf(vp, " %i tracks loaded from '%s'", total, filename);
}

void filepanePlay_loadPlaylist (TFILEPANE *filepane, const wchar_t *path, const char *filename)
{
	filepaneSelection_Playlist(filepane, NULL, filename, filename, NULL);
}

int filepaneSetLogicalDriveRoot (TFILEPANE *filepane, const char driveLetter)
{
	char drive[4] = {driveLetter, ':', '\\', 0};
			
	TFB *fb = filepaneCreatePath(filepane, drive, drive);
	filepaneEmpty(filepane);
	filepaneStackEmpty(filepane);
	int addCount = filepaneAddFB(filepane, fb);
	stackPush(filepane->stack, (intptr_t)fb);
	
	buildLocBar(filepane, filepane->locBar);
	buildTitleBar(filepane, filepane->title);
	//printf("added %i\n", addCount);
	return addCount;
}

int filepaneImportPlaylistByDirUID (TFILEPANE *filepane, wchar_t *path, const int uid, const int recursive)
{
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(filepane->com->vp->plm, uid);
	if (plc)
		return filepaneBuildPlaylistDir(filepane, plc, path, FILEMASKS_MEDIA, 1);
	else
		return 0;
}

// generate a new playlist in root
// add contents of directory (including its subdirectories) containing selected file,
// then initiate playback beginning with selected file
static inline int filepaneImport (TFILEPANE *filepane, TFB *fb, const int fbType, char *path, char *item)
{
	if (fbType == FB_OBJTYPE_FILE){
		char pathFull8[MAX_PATH_UTF8+1];
		__mingw_snprintf(pathFull8, MAX_PATH_UTF8, "%s%s", path, item);
		
		wchar_t *folder = converttow(fbGetUserData(fb));
		if (folder){
			if (isMediaAudio8(item)){
				filepaneSelection_Audio(filepane, fb, item, pathFull8, folder);
					
			}else if (isMediaVideo8(item)){
				filepaneSelection_Video(filepane, fb, item, pathFull8, folder);
					
			}else if (isMediaPlaylist8(item)){
				filepaneSelection_Playlist(filepane, fb, item, pathFull8, folder);
			
			}else if (isMediaImage8(item)){
				filepaneSelection_Image(filepane, fb, item, pathFull8, folder);
			}else{
				//printf("# UNHANDLED FILE '%s'\n", path8);
				//printf("\t %I64d %I64d %p, %i %i\n", data1, data2, dataPtr, fbType, fbIndex);
			}
			my_free(folder);
		}
	}else if (fbType == FB_OBJTYPE_FOLDER){
		char pathFull8[MAX_PATH_UTF8+1];
		__mingw_snprintf(pathFull8, MAX_PATH_UTF8, "%s%s\\", path, item);
				
		fb = filepaneCreatePath(filepane, item, pathFull8);
		filepaneEmpty(filepane);
		filepaneAddFB(filepane, fb);
		stackPush(filepane->stack, (intptr_t)fb);
		buildLocBar(filepane, filepane->locBar);
		buildTitleBar(filepane, filepane->title);
	}
	
	return 1;	// or something useful
}

static inline int64_t filepane_pane_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT/* || msg == CC_MSG_HOVER*/) return 1;
	if (msg < 200) return 1;
	
	TPANE *pane = (TPANE*)object;
	//printf("filepane_pane_cb: %i %i %I64d %I64d %p\n", pane->id, msg, data1, data2, dataPtr);
	
	
	if (msg == PANE_MSG_BASE_SELECTED_RELEASE){
		//printf("PANE_MSG_BASE_SELECTED_RELEASE: %i %i %I64d %I64d %p\n", pane->id, msg, data1, data2, dataPtr);
		TFILEPANE *filepane = ccGetUserData(pane);
		ccDisable(filepane->pane->input.drag.label);
		filepane->import.heldItemId = 0;
		filepane->import.releasedItemId = 0;
		
	}else if (msg == PANE_MSG_SLIDE_HELD){
#if 0
		TFILEPANE *filepane = ccGetUserData(pane);
		TFB *fb = filepaneGetPaneFB(filepane);
		if (!fb) return 1;

		int id = data2&0xFFFF;
		char *name = filepaneFBGetItem(fb, --id);
		if (name){
			//printf("filepane_pane Held: %I64d %I64d '%s%s'\n", data1, data2, (char*)fbGetUserData(fb), name);
			my_free(name);
		}
	}else if (msg == PANE_MSG_SLIDE_HOVER){
		TFILEPANE *filepane = ccGetUserData(pane);
		TFB *fb = filepaneGetPaneFB(filepane);
		if (!fb) return 1;

		int id = data2&0xFFFF;
		char *name = filepaneFBGetItem(fb, --id);
		if (name){
			int itemHoveredOver = data1>>32;
			//int itemHovering = data1&0xFFFFFFF;
			//printf("filepane_pane Hover: %i:%i %I64d '%s%s'\n", itemHoveredOver, itemHovering, data2, (char*)fbGetUserData(fb), name);
			
			if (labelItemGetType(pane->base, itemHoveredOver) == LABEL_OBJTYPE_TEXT){
				char *str = labelStringGet(pane->base, itemHoveredOver);
				if (str){
				//	printf("filepane_pane HoveredOver: '%s%s'\n", (char*)fbGetUserData(fb), str);
					my_free(str);
				}
			}
			my_free(name);
		}
	}else if (msg == PANE_MSG_SLIDE_RELEASE){
		TFILEPANE *filepane = ccGetUserData(pane);
		TFB *fb = filepaneGetPaneFB(filepane);
		if (!fb) return 1;

		int id = data2&0xFFFF;
		char *name = filepaneFBGetItem(fb, --id);
		if (name){
			//int itemReleasedOver = data1>>32;  // if any
			//int itemReleased = data1&0xFFFFFFF;
			//printf("filepane_pane Release: %i:%i %I64d '%s%s'\n", itemReleasedOver, itemReleased, data2, (char*)fbGetUserData(fb), name);
			my_free(name);
			renderSignalUpdate(pane->cc->vp);
		}
#endif
	}else if (msg == PANE_MSG_TEXT_SELECTED || msg == PANE_MSG_IMAGE_SELECTED){
		//int itemId = data1 + (msg == PANE_MSG_IMAGE_SELECTED);
		TFILEPANE *filepane = ccGetUserData(pane);

		const int fbType = (data2>>16)&0x0F;
		int fbIndex = (data2&0xFFFF);
		
		if (fbType == FB_OBJTYPE_FILE){
			//printf("# FILE\n");

			TFB *fb = filepaneGetPaneFB(filepane);
			if (!fb) return 1;

			char *name = filepaneFBGetItem(fb, --fbIndex);
			if (name){
				//printf("filepane selected '%s' #%s#\n", (char*)fbGetUserData(fb), (char*)name);
				if (pageDispatchMessage(pane->cc->vp->pages, PAGE_MSG_FILE_SELECTED, (intptr_t)fbGetUserData(fb), (intptr_t)name,  NULL))
					filepaneImport(filepane, fb, fbType, fbGetUserData(fb), name);
				my_free(name);
			}
		}else if (fbType == FB_OBJTYPE_FOLDER){
			//printf("# FOLDER in\n");

			TFB *fb = filepaneGetPaneFB(filepane);
			if (!fb) return 1;
			
			char *name = filepaneFBGetItem(fb, --fbIndex);
			if (name){
				filepaneImport(filepane, fb, fbType, fbGetUserData(fb), name);
				my_free(name);
			}
			//printf("# FOLDER out\n");
		}else if (fbType == FB_OBJTYPE_BACK){
			//printf("# BACK\n");
			
			intptr_t data;
			stackPop(filepane->stack, &data);
			filepaneReleaseFB((TFB*)data);
			
			data = 0;			// check if root
			stackPeek(filepane->stack, &data);
			
			if (!data){		// we're at root so add logical drives once again
				filepaneEmpty(filepane);
				filepaneAddLogicalDrives(filepane);
			}else{
				TFB *fb = (TFB*)data;
				filepaneEmpty(filepane);
				filepaneAddFB(filepane, fb);
				filepaneSetSortLabel(filepane, fb->sortedMode);
				filepaneSetFilterLabel(filepane, fb->filteredMode);
				//printf("added %i\n", addCount);
			}
			
			buildLocBar(filepane, filepane->locBar);
			buildTitleBar(filepane, filepane->title);
			
		}else if (fbType == FB_OBJTYPE_BACKRIGHT){
			//printf("# CLOSE\n");
			page2SetPrevious(filepane);
			
		}else if (fbType == FB_OBJTYPE_DRIVE){
			//printf("# DRIVE %c:\n", fbIndex);
			filepaneSetLogicalDriveRoot(filepane, fbIndex);

		}
	}
	
	return 1;
}

int64_t filepane_titlebar_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
#if 0
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT || msg == CC_MSG_HOVER) return 1;
	
	TLABEL *title = (TLABEL*)object;
	//printf("filepane_titlebar_cb in %p, %i %I64d %I64d %p\n", bar, msg, data1, data2, dataPtr);
	
	if (msg == LABEL_MSG_TEXT_SELECTED_PRESS){
		const int data = labelItemDataGet(title, data2);
		//printf("title id %i\n", data);
		
	}
#endif
	return 1;
}

int64_t filepane_locbar_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT || msg == CC_MSG_HOVER) return 1;
	
	TLABEL *bar = (TLABEL*)object;
	//printf("filepane_locbar_cb:  %i, %i %I64d %I64d %p\n", bar->id, msg, data1, data2, dataPtr);

	
	if (msg == LABEL_MSG_TEXT_SELECTED_PRESS){
		TFILEPANE *filepane = ccGetUserData(bar);	
		//TPANE *pane = filepane->pane;
		const int data = labelItemDataGet(bar, data2);
		//printf("locBar id %i\n", data);

		const int type = (data>>16)&0x0F;
		const int fbIndex = data&0xFFFF;
		//printf("locBar id %i, %i %i\n", data, type, fbIndex);
		
		if (type == FB_OBJTYPE_FOLDER || type == FB_OBJTYPE_DRIVE){
			//printf("## FOLDER %i\n", fbIndex);
			
			int length;
			TFB **stack = stackCopyPtr(filepane->stack, &length);
			if (stack){
				TFB *fb = stack[fbIndex];
				//printf("fb: %i '%s'\n", fbIndex, fb->rootName);
					
				filepaneStackRewind(filepane, fb);
				filepaneEmpty(filepane);
				filepaneAddFB(filepane, fb);
				
				filepaneSetSortLabel(filepane, fb->sortedMode);
				filepaneSetFilterLabel(filepane, fb->filteredMode);
				buildLocBar(filepane, filepane->locBar);
				buildTitleBar(filepane, filepane->title);
				//printf("added %i\n", addCount);
					
				my_free(stack);
			}else{
				//printf("locbar: stack empty\n");
			}
		}else if (type == FB_OBJTYPE_MYCOM){
			//printf("## MYCOM %i\n", fbIndex);

			filepaneEmpty(filepane);
			filepaneStackEmpty(filepane);
#if 0
			filepaneAddLogicalDrives(filepane);
			buildLocBar(filepane, filepane->locBar);
			buildTitleBar(filepane, filepane->title);
#endif
			page2SetPrevious(filepane);
		}
	}

	return 1;
}

static inline void drawFolderStats (TFILEPANE *filepane, TFRAME *frame, const int font, const int x, const int y)
{
	TFB *fb = filepaneGetPaneFB(filepane);
	if (!fb) return;
	
#if 1
	THWD *hw = filepane->com->vp->ml->hw;
	const int blurOp = LTR_BLUR6;
	lSetRenderEffect(hw, blurOp);
	//lRenderEffectReset(hw, font, blurOp);
	lSetForegroundColour(hw, 0xFF<<24 | 0xFFFFFF);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_COLOUR, COL_CYAN);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_RADIUS, 4);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_SETTOP, 1);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_Y, 1);
	lSetFilterAttribute(hw, blurOp, LTRA_BLUR_ALPHA, 975);
#endif

	TLPRINTR rt;
	//memset(&rt, 0, sizeof(rt));
	rt.bx1 = 0;
	rt.by1 = 0;
	rt.bx2 = frame->width-1;
	rt.by2 = frame->height-1;
	rt.sx = x;
	rt.sy = y;
	lPrintEx(frame, &rt, font, PF_CLIPWRAP|PF_CLIPDRAW, LPRT_CPY, "%i\n %i", fb->leafCount, fb->branchCount);
}

static inline int page_filePaneRender (TFILEPANE *filepane, TFRAME *frame)
{
//	printf("paneRender -> %i ", filepane->pane->id);

	ccRender(filepane->filterLabel, frame);
	ccRender(filepane->sortLabel, frame);
	drawFolderStats(filepane, frame, PANE_FODLERSTATS_FONT, 8, 0);

	buttonsRenderAll(filepane->btns, frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);
	ccRender(filepane->locBar, frame);
	labelStrRender(filepane->import.media, frame);
	ccRender(filepane->title, frame);
	
	//double t0 = getTime(filepane->com->vp);	
	ccRender(filepane->pane, frame);
	//double t1 = getTime(filepane->com->vp);
	//printf("%.2f\n", t1-t0);	
	
	return 1;
}

static inline void page_filePaneRenderBegin (TFILEPANE *filepane, int64_t destId, int64_t data2, void *opaquePtr)
{
	ccEnable(filepane->filterLabel);
	ccEnable(filepane->sortLabel);
	ccEnable(filepane->locBar);
	ccEnable(filepane->title);
	ccEnable(filepane->pane);
	labelStrDisable(filepane->import.media);
	
	buttonsStateSet(filepane->btns, FILEPANE_IMPORT_CONTENTS, 0);
	buttonsStateSet(filepane->btns, FILEPANE_IMPORT_TRACK, 0);
	
	paneDragEnable(filepane->pane);
}

static inline void page_filePaneRenderEnd (TFILEPANE *filepane, int64_t destId, int64_t data2, void *opaquePtr)
{
	paneDragDisable(filepane->pane);

	buttonsStateSet(filepane->btns, FILEPANE_IMPORT_TRACK, 0);
	buttonsStateSet(filepane->btns, FILEPANE_IMPORT_CONTENTS, 0);
	
	ccDisable(filepane->filterLabel);
	ccDisable(filepane->sortLabel);
	ccDisable(filepane->locBar);
	ccDisable(filepane->title);
	ccDisable(filepane->pane);
}

static inline int page_filepaneRenderInit (TFILEPANE *filepane, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	//filepaneSetFilterMask(filepane, FILEMASKS_MEDIA);
	//filepaneSetSortMode(filepane, SORT_NAME_A);
	
	if (!filepane->icons.areRegistered)
		filePaneRegisterIcons(filepane);
	
	if (!filepaneGetPaneFB(filepane))
		filepaneAddLogicalDrives(filepane);
	buildLocBar(filepane, filepane->locBar);
	buildTitleBar(filepane, filepane->title);
	return 1;
}


static inline void page_filePaneObjHover (TFILEPANE *filepane, int64_t data1, int64_t data2, void *dataPtr)
{
	//printf("### page_filePaneObj Hover: %I64d %I64d %p\n", data1, data2, dataPtr);
	
	if (data1 == PAGE_OBJ_HOVER_NONE){
		buttonsStateSet(filepane->btns, FILEPANE_SHOWEXT, 1);
		buttonsStateSet(filepane->btns, FILEPANE_REFRESH, 1);
		buttonsStateSet(filepane->btns, FILEPANE_BACK, 1);
		buttonsStateSet(filepane->btns, FILEPANE_IMPORT, 1);
		ccEnable(filepane->sortLabel);
		ccEnable(filepane->filterLabel);
		buttonsStateSet(filepane->btns, FILEPANE_IMPORT_CONTENTS, 0);
		buttonsStateSet(filepane->btns, FILEPANE_IMPORT_TRACK, 0);

		labelStrDisable(filepane->import.media);
		ccEnable(filepane->locBar);

	}else if (data1 == PAGE_OBJ_HOVER_HELD){
		filepane->import.heldItemId = (data2&0xFFFFFFFF);
		filepane->import.releasedItemId = 0;
		
		buttonsStateSet(filepane->btns, FILEPANE_SHOWEXT, 0);
		buttonsStateSet(filepane->btns, FILEPANE_REFRESH, 0);
		buttonsStateSet(filepane->btns, FILEPANE_BACK, 0);
		buttonsStateSet(filepane->btns, FILEPANE_IMPORT, 0);
		ccDisable(filepane->sortLabel);
		ccDisable(filepane->filterLabel);
		buttonsStateSet(filepane->btns, FILEPANE_IMPORT_CONTENTS, 1);
		buttonsStateSet(filepane->btns, FILEPANE_IMPORT_TRACK, 1);
		
		ccDisable(filepane->locBar);
		
		TFB *fb = filepaneGetPaneFB(filepane);
		if (!fb) return;
		const int item = labelItemDataGet(filepane->pane->base, filepane->import.heldItemId);
		const int fbIndex = (item&0xFFFF)-1;
		char *itemStr = filepaneFBGetItem(fb, fbIndex);
		if (itemStr){
			labelStrUpdate(filepane->import.media, itemStr);
			labelStrEnable(filepane->import.media);
			my_free(itemStr);
		}else{
			paneDragDisable(filepane->pane);
			paneDragEnable(filepane->pane);
		}
		
		//printf("page_filePaneObjHover Held: %I64d %i %i\n", data1, (int)(data2>>32), (int)(data2&0xFFFFFFFF));
	}else if (data1 == PAGE_OBJ_HOVER_RELEASE){
		filepane->import.releasedItemId = (data2>>32);
		labelStrDisable(filepane->import.media);
		ccEnable(filepane->locBar);
		//printf("page_filePaneObjHover Release: %i %i\n", (int)(data2>>32), (int)(data2&0xFFFFFFFF));
	}
	
}

static inline int filepaneRefreshContents (TFILEPANE *filepane)
{
	TFB *fb = filepaneGetPaneFB(filepane);

	if (fb){
		
		//if (ccLock(filepane->pane)){
		void *udata = fbGetUserData(fb);
		if (!udata) return 0;

		char *path = my_strdup(udata);
		char *name = my_strdup(fb->rootName);

		if (name && path){
			stackPop(filepane->stack, NULL);
			filepaneReleaseFB(fb);

			fb = filepaneCreatePath(filepane, name, path);
			filepaneEmpty(filepane);
	//double t0 = getTime(filepane->com->vp);
			filepaneAddFB(filepane, fb);
	//double t1 = getTime(filepane->com->vp);
			stackPush(filepane->stack, (intptr_t)fb);
			filepaneSetFilterLabel(filepane, fb->filteredMode);
			buildLocBar(filepane, filepane->locBar);
			buildTitleBar(filepane, filepane->title);


		//	printf("buildTime %.2f\n", t1-t0);
		}
		if (name) my_free(name);
		if (path) my_free(path);
		//ccUnlock(filepane->pane);
		//}
		return 1;
	}
	return 0;
}

static inline int filepaneButtonPress (TFILEPANE *filepane, TCCBUTTON *btn, const int id, const TTOUCHCOORD *pos)
{
	//TVLCPLAYER *vp = btn->cc->vp;
	filepane->btns->t0 = getTickCount();


	switch (id){
	case FILEPANE_SHOWEXT:
		filepane->showExtensions ^= 1;
		if (!filepane->showExtensions)
			buttonFaceActiveSet(btn, BUTTON_SEC);
		else
			buttonFaceActiveSet(btn, BUTTON_PRI);

		TFB *fb = filepaneGetPaneFB(filepane);
		if (fb){
			fb->refCt++;
			filepaneEmpty(filepane);
			filepaneAddFB(filepane, fb);
			fb->refCt--;
		}
		break;
    case FILEPANE_IMPORT:{
    	TPANEINPUT *input = &filepane->pane->input;
    	if (input->slideMode == PANE_SLIDEMODE_PANE){
    		buttonFaceActiveSet(btn, BUTTON_SEC);
    		input->slideMode = PANE_SLIDEMODE_ITEM;
    	}else{
			buttonFaceActiveSet(btn, BUTTON_PRI);
    		input->slideMode = PANE_SLIDEMODE_PANE;
    	}
    	break;

    }case FILEPANE_REFRESH:
    	filepaneRefreshContents(filepane);
    	renderSignalUpdate(btn->cc->vp);
    	break;
    case FILEPANE_BACK:
		page2SetPrevious(filepane);
		break;
	}
	return 1;
}


static inline int64_t ccbtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{		
	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//if (msg != CC_MSG_RENDER)
	//	printf("ccBtn_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);

	TCCBUTTON *btn = (TCCBUTTON*)object;
	TFILEPANE *filepane = pageGetPtr(btn->cc->vp, ccGetOwner(btn));

	//if (msg != CC_MSG_RENDER && msg != CC_MSG_ENABLED && msg != CC_MSG_DISABLED)
	//	printf("ccBtn_cb, id:%i, msg:%i, data1:%I64d, data2:%I64d, ptr:%p\n", btn->id, msg, data1, data2, dataPtr);


	if (btn->id == filepane->import.ccAddTrackId){
		if (msg == BUTTON_MSG_SELECTED_RELEASE){
			ccDisable(filepane->pane->input.drag.label);
			
			TFB *fb = filepaneGetPaneFB(filepane);
			if (!fb) return 1;
				
			const int item = labelItemDataGet(filepane->pane->base, filepane->import.heldItemId);
			const int fbType = (item>>16)&0x0F;
			const int fbIndex = (item&0xFFFF)-1;

			char *itemStr = filepaneFBGetItem(fb, fbIndex);
			if (itemStr){
				char *path = fbGetUserData(fb);
				if (path){
					//printf("import single released: %i: %i %i, '#%s#%s#'\n", filepane->import.heldItemId, fbType, fbIndex, path, itemStr);
					
					PLAYLISTCACHE *plc = getQueuedPlaylist(btn->cc->vp);
					if (!plc) plc = getDisplayPlaylist(btn->cc->vp);
					if (!plc) plc = getPrimaryPlaylist(btn->cc->vp);
					
					if (plc){
						char pathFull8[MAX_PATH_UTF8+1];
						__mingw_snprintf(pathFull8, MAX_PATH_UTF8, "%s%s", path, itemStr);
						
						if (fbType == FB_OBJTYPE_FILE){
							filepaneImport_File(filepane, plc, pathFull8, itemStr);
							
						}else if (fbType == FB_OBJTYPE_FOLDER){
							strncat(pathFull8, "\\", MAX_PATH_UTF8);
							filepaneImport_Folder(filepane, plc, pathFull8, 0);
						}
					}
				}
				my_free(itemStr);
			}
		
			filepane->import.heldItemId = 0;
			filepane->import.releasedItemId = 0;
		}
	}else if (btn->id == filepane->import.ccGenPlaylistId){
		if (msg == BUTTON_MSG_SELECTED_RELEASE){
			ccDisable(filepane->pane->input.drag.label);

			TFB *fb = filepaneGetPaneFB(filepane);
			if (!fb) return 1;
				
			const int item = labelItemDataGet(filepane->pane->base, filepane->import.heldItemId);
			const int fbType = (item>>16)&0x0F;
			const int fbIndex = (item&0xFFFF)-1;
					
			//char *itemStr = labelStringGet(filepane->pane->base, filepane->import.heldItemId, 0);
			// or, and should be equal to
			char *itemStr = filepaneFBGetItem(fb, fbIndex);
			if (itemStr){
				char *path = fbGetUserData(fb);
				if (path){
					//printf("import nulti released: %i: %i %i, '#%s#%s#'\n", filepane->import.heldItemId, fbType, fbIndex, path, itemStr);
					
					if (fbType == FB_OBJTYPE_FILE){
						filepaneImport(filepane, fb, fbType, path, itemStr);
					}else if (fbType == FB_OBJTYPE_FOLDER){
						char pathFull8[MAX_PATH_UTF8+1];
						__mingw_snprintf(pathFull8, MAX_PATH_UTF8, "%s%s\\", path, itemStr);
						filepaneImport_Folder(filepane, NULL, pathFull8, 1);
					}
				}
				my_free(itemStr);
			}
		
			filepane->import.heldItemId = 0;
			filepane->import.releasedItemId = 0;
		}
	}else if (msg == BUTTON_MSG_SELECTED_PRESS){
		return filepaneButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	}

	return 1;
}

static inline int64_t filepane_label_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER/* || msg == CC_MSG_INPUT*/) return 1;

	TLABEL *label = (TLABEL*)object;
	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//if (msg != CC_MSG_RENDER)
	//	printf("filepane_label_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);
	
	switch (msg){
	case LABEL_MSG_BASE_SELECTED_PRESS:{
		TTOUCHCOORD *pos = (TTOUCHCOORD*)dataPtr;
		if (pos->pen || pos->dt < 80) break;
	}
		ALLOW_FALLTHROUGH;
	case LABEL_MSG_TEXT_SELECTED_PRESS:{
		TFILEPANE *filepane = ccGetUserData(label);
		if (label->id == filepane->sortLabelId){
			TFB *fb = filepaneGetPaneFB(filepane);
			if (fb){
				if (++filepane->sortMode >= SORT_TOTAL)
					filepane->sortMode = SORT_NOSORT;
				
				fb->sortedMode = filepane->sortMode;
				fbSort(fb, fb->rootId, fb->sortedMode);
				filepaneEmpty(filepane);
				filepaneAddFB(filepane, fb);
				filepaneSetSortLabel(filepane, fb->sortedMode);
				//buildLocBar(filepane, filepane->locBar);
				//buildTitleBar(filepane, filepane->title);
			}
		}else if (label->id == filepane->filterLabelId){
			const int oldMask = filepane->filterMask;
			if (++filepane->filterMask >= FILEMASKS_TOTAL)
				filepane->filterMask = FILEMASKS_ALL;

			if (!filepaneRefreshContents(filepane))
				filepane->filterMask = oldMask;
		}
	}
	};
	return 1;
}


static inline int page_filePaneStartup (TFILEPANE *filepane, TVLCPLAYER *vp, const int width, const int height)
{
	filepane->btns = buttonsCreate(vp->cc, PAGE_FILE_PANE, FILEPANE_TOTAL, ccbtn_cb);
	
	const int gap = 3;
	int x = 90;
		
	// file type; audio, video, etc..
	filepane->filterLabel = ccCreateEx(vp->cc, PAGE_FILE_PANE, CC_LABEL, filepane_label_cb, &filepane->filterLabelId, 120, 50, filepane);
	filepane->filterStrId = labelTextCreate(filepane->filterLabel, " ", 0, FILEPANE_FILTER_FONT, 0, 0);
	labelRenderFlagsSet(filepane->filterLabel, LABEL_RENDER_TEXT|LABEL_RENDER_HOVER);
	labelRenderBlurRadiusSet(filepane->filterLabel, filepane->filterStrId, 3);
	labelRenderFilterSet(filepane->filterLabel, filepane->filterStrId, 2);
	labelRenderFontSet(filepane->filterLabel, filepane->filterStrId, FILEPANE_FILTER_FONT);
	labelRenderColourSet(filepane->filterLabel, filepane->filterStrId, 255<<24|COL_WHITE, 255<<24|COL_BLUE_SEA_TINT, 255<<24|COL_BLUE_SEA_TINT);
	filepane->filterLabel->canDrag = 0;
	ccSetMetrics(filepane->filterLabel, x, 0, -1, -1);
	x += ccGetWidth(filepane->filterLabel) + gap;
	
	// sort type; by date, name,  etc..
	filepane->sortLabel = ccCreateEx(vp->cc, PAGE_FILE_PANE, CC_LABEL, filepane_label_cb, &filepane->sortLabelId, 200, 50, filepane);
	filepane->sortStrId = labelTextCreate(filepane->sortLabel, " ", 0, FILEPANE_SORT_FONT, 0, 0);
	labelRenderFlagsSet(filepane->sortLabel, LABEL_RENDER_TEXT|LABEL_RENDER_HOVER);
	labelRenderBlurRadiusSet(filepane->sortLabel, filepane->sortStrId, 3);
	labelRenderFilterSet(filepane->sortLabel, filepane->sortStrId, 2);
	labelRenderFontSet (filepane->sortLabel, filepane->sortStrId, FILEPANE_SORT_FONT);
	labelRenderColourSet(filepane->sortLabel, filepane->sortStrId, 255<<24|COL_WHITE, 255<<24|COL_BLUE_SEA_TINT, 255<<24|COL_BLUE_SEA_TINT);
	filepane->sortLabel->canDrag = 0;
	ccSetMetrics(filepane->sortLabel, x, 0, -1, -1);
	x += ccGetWidth(filepane->sortLabel) + gap;
	
	
	TCCBUTTON *btn = buttonsCreateButton(filepane->btns, L"filepane/exton.png", L"filepane/extoff.png", FILEPANE_SHOWEXT, 1, 0, 0, 0);
	buttonFaceHoverSet(btn, 0, 0, 0.0);
	ccSetPosition(btn, x, 0);

	x = width-1;
	btn = buttonsCreateButton(filepane->btns, L"filepane/backright.png", NULL, FILEPANE_BACK, 1, 0, 0, 0);
	x -= ccGetWidth(btn);
	ccSetPosition(btn, x, 0);
	
	btn = buttonsCreateButton(filepane->btns, L"filepane/refresh.png", NULL, FILEPANE_REFRESH, 1, 1, 0, 0);
	x -= ccGetWidth(btn) + gap;
	ccSetPosition(btn, x, 0);

	btn = buttonsCreateButton(filepane->btns, L"filepane/import2.png", L"filepane/import1.png", FILEPANE_IMPORT, 1, 0, 0, 0);
	buttonFaceHoverSet(btn, 0, 0, 0.0);
	x -= ccGetWidth(btn) + gap;
	ccSetPosition(btn, x, 0);
	
		
	btn = buttonsCreateButton(filepane->btns, L"filepane/importcontents.png", NULL, FILEPANE_IMPORT_CONTENTS, 0, 1, 0, 0);
	ccSetPosition(btn, (width/2) - ccGetWidth(btn) - 64, 0);
	buttonFaceHoverSet(btn, 1, COL_HOVER, 0.95);
	filepane->import.ccGenPlaylistId = btn->id;

	btn = buttonsCreateButton(filepane->btns, L"filepane/importtrack.png", NULL, FILEPANE_IMPORT_TRACK, 0, 1, 0, 0);
	ccSetPosition(btn, (width/2) + 32 , 0);
	buttonFaceHoverSet(btn, 1, COL_HOVER, 0.95);
	filepane->import.ccAddTrackId = btn->id;
		
	TLABEL *locBar = ccCreateEx(vp->cc, PAGE_FILE_PANE, CC_LABEL, filepane_locbar_cb, &vp->gui.ccIds[CCID_LABEL_PANELOCBAR], width, 36, filepane);
	labelRenderFlagsSet(locBar, LABEL_RENDER_HOVER_OBJ | LABEL_RENDER_TEXT);
	ccSetPosition(locBar, 0, 56);

	TLABEL *title = ccCreateEx(vp->cc, PAGE_FILE_PANE, CC_LABEL, filepane_titlebar_cb, NULL, width, 36, filepane);
	labelRenderFlagsSet(title, LABEL_RENDER_TEXT);
	ccSetPosition(title, 0, 32+55);

	filepane->import.media = labelStrCreate(filepane, " ", PANE_LOCBAR_FONT, 7, 58, width, filepane, NSEX_LEFT);
	labelStrDisable(filepane->import.media);

	filepane->pane = ccCreateEx(vp->cc, PAGE_FILE_PANE, CC_PANE, filepane_pane_cb, NULL, width, height-(32+32+56), filepane);
	ccSetPosition(filepane->pane, 0, 32+32+56);
	paneSetLayout(filepane->pane, PANE_LAYOUT_HORI);
	paneSetAcceleration(filepane->pane, PANE_ACCELERATION_X, 0.0);
	
	filepane->locBar = locBar;
	filepane->title = title;
	filepane->stack = stackCreate(8);
	filepane->myComputerName = fbGetMyComputerName();
	filepane->showExtensions = 1;
	return 1;
}


static inline int page_filePaneInitalize (TFILEPANE *filepane, TVLCPLAYER *vp, const int width, const int height)
{

	setPageAccessed(vp, PAGE_FILE_PANE);
#if ENABLE_FILEEXT_CONFIG	
	filepane->exts = fileext_load(EXTS_CONFIGFILE);
#endif
	
	char *values = NULL;
	settingsGet(vp, "browser.filterBy", &values);
	if (values){
		int extType = filterStrToIdx(values);
		filepaneSetFilterMask(filepane, extType);
		my_free(values);
	}

	values = NULL;
	settingsGet(vp, "browser.sortBy", &values);
	if (values){
		int sortMode = sortStrToIdx(values);
		filepaneSetSortMode(filepane, sortMode);
		my_free(values);
	}

	filepane->filterMasks[FILEMASKS_ALL] = NULL;
	filepane->filterMasks[FILEMASKS_AUDIO] = extAudio;
	filepane->filterMasks[FILEMASKS_VIDEO] = extVideo;
	filepane->filterMasks[FILEMASKS_PLAYLISTS] = extPlaylists;
	filepane->filterMasks[FILEMASKS_IMAGE] = extImage;
	filepane->filterMasks[FILEMASKS_MEDIA] = extMedia;
	
	filepaneSetFilterMask(filepane, FILEMASKS_MEDIA);
	filepaneSetSortMode(filepane, SORT_NAME_A);

	return 1;
}

static inline int page_filePaneShutdown (TFILEPANE *filepane)
{
	
	buttonsDeleteAll(filepane->btns);
		
	ccDelete(filepane->filterLabel);
	ccDelete(filepane->sortLabel);
	ccDelete(filepane->title);
	ccDelete(filepane->locBar);
	ccDelete(filepane->pane);
	labelStrDelete(filepane->import.media);

	if (filepane->drives.list)
		fbGetLogicalDrivesRelease(filepane->drives.list);

	filepaneStackEmpty(filepane);
	filepaneStackDestroy(filepane);	
	
	if (filepane->myComputerName)
		my_free(filepane->myComputerName);
		
#if ENABLE_FILEEXT_CONFIG
	fileext_free(filepane->exts);
#endif
	return 1;
}


static inline int page_filePaneInput (TFILEPANE *filepane, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
		if (ccGetState(filepane->pane))
			paneScroll(filepane->pane, FILEPANE_SCROLL_DELTA);
		break;
		
	  case PAGE_IN_WHEEL_BACK:
		if (ccGetState(filepane->pane))
			paneScroll(filepane->pane, -FILEPANE_SCROLL_DELTA);
		break;
	}
		
	return 1;
}

int page_filePaneCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TFILEPANE *filepane = (TFILEPANE*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_filePaneCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_filePaneRender(filepane, dataPtr);

	}else if (msg == PAGE_MSG_OBJ_HOVER){
		page_filePaneObjHover(filepane, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_START){
		page_filePaneRenderBegin(filepane, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		page_filePaneRenderEnd(filepane, dataInt1, dataInt2, opaquePtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_filePaneInput(filepane, dataInt1, dataInt2, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_filepaneRenderInit(filepane, dataInt1, dataInt2, dataPtr, opaquePtr);
				
	}else if (msg == PAGE_CTL_STARTUP){
		return page_filePaneStartup(filepane, filepane->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_filePaneInitalize(filepane, filepane->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_filePaneShutdown(filepane);
		
	}
	
	return 1;
}

const float PulseScale = 8;		// ratio of "tail" to "acceleration"

float PulseNormalize = 1;
void ComputePulseScale();

// viscous fluid with a pulse for part and decay for the rest
static inline float Pulse_(float x)
{
	float val;

	// test
	x = x * PulseScale;
	if (x < 1) {
		val = x - (1 - exp(-x));
	} else {
		// the previous animation ended here:
		float start = exp(-1);

		// simple viscous drag
		x -= 1;
		float expx = 1 - exp(-x);
		val = start + (expx * (1.0 - start));
	}

	return val * PulseNormalize;
}

void ComputePulseScale()
{
	PulseNormalize = 1.f / Pulse_(1);
}

// viscous fluid with a pulse for part and decay for the rest
static inline float Pulse(float x)
{
	if (x >= 1) return 1;
	if (x <= 0) return 0;

	if (PulseNormalize == 1) {
		ComputePulseScale();
	}

	return Pulse_(x);
}
