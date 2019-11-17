
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



#ifndef _FILEPANE_H_
#define _FILEPANE_H_



#define FILEPANE_SCROLL_DELTA	80



#define FB_OBJTYPE_FILE			1
#define FB_OBJTYPE_FOLDER		2
#define FB_OBJTYPE_DRIVE		3
#define FB_OBJTYPE_BACK			4		// parent folder
#define FB_OBJTYPE_MYCOM		5
#define FB_OBJTYPE_BACKRIGHT	6		// close pane





enum _filterMasks {
	FILEMASKS_ALL,
	FILEMASKS_AUDIO,
	FILEMASKS_VIDEO,
	FILEMASKS_PLAYLISTS,
	FILEMASKS_IMAGE,
	FILEMASKS_MEDIA,
	FILEMASKS_TOTAL,
	
	FILEMASKS_DEFAULT = FILEMASKS_MEDIA
};


typedef struct{
	TLOGICALDRIVE *list;
	int total;
}TDRIVELIST;


typedef struct{
	TPAGE2COM *com;

	TLABEL *locBar;
	TLABEL *title;
	TPANE *pane;	
	TCCBUTTONS *btns;
	TDRIVELIST drives;
	TSTACK *stack;
#if ENABLE_FILEEXT_CONFIG
	TFILEEXT *exts;		// list of user added file extensions with asscoiated image icon
#endif
	char *myComputerName;
	unsigned int showExtensions;
	
	struct{
		int areRegistered;
		int other;
		int folder;
		int drive;
		int driveSystem;
		int driveRemovable;
		int driveRemote;
		int driveUSB;
		int driveNoMedia;
		int back;
		int closePane;
	}icons;

	// file and folder sorting
	TLABEL *sortLabel;
	int     sortLabelId;
	int     sortStrId;
	int		sortMode;
		
	// file filter (.ext)
	TLABEL *filterLabel;
	int     filterLabelId;
	int     filterStrId;
	int		filterMask;
	wchar_t **filterMasks[FILEMASKS_TOTAL];
	
	struct {
		int ccGenPlaylistId;
		int ccAddTrackId;

		int heldItemId;			// pickedup/dragged/held item
		int releasedItemId;		// dropped on to this item
		TLABELSTR *media;		// string displaying item held/being imported
	}import;					// drag drop
}TFILEPANE;



int page_filePaneCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);


int filepaneBuildPlaylistDir (TFILEPANE *filepane, PLAYLISTCACHE *plc, wchar_t *pathw, const int filterMode, const int recursive);
int filepaneImportPlaylistByDirUID (TFILEPANE *filepane, wchar_t *path, const int uid, const int recursive);


void filepaneSetFilterMask (TFILEPANE *filepane, const int type);
void filepaneSetSortMode (TFILEPANE *filepane, const int mode);
int filepaneSetLogicalDriveRoot (TFILEPANE *filepane, const char driveLetter);

void filepanePlay_loadPlaylist (TFILEPANE *filepane, const wchar_t *path, const char *filename);
void filepanePlay_DoPlay (TFILEPANE *filepane, char *path);

void filepaneViewImage (TFILEPANE *filepane, wchar_t *path);

int filepaneSetPath (TFILEPANE *filepane, char *path);



#endif

