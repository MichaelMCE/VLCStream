
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



#ifndef _SEARCH_H_
#define _SEARCH_H_

/*
		#nnn	contents of playlist nnn (eg; #2C8)
		#:		all files, only
		#*		all playlists, only
		#:*		all files then playlists
		#*:		all playlists then files
		#.		everything, unordered as and when found
*/

#define SEARCH_CMD_CHAR				"#"

#define SEARCH_CMD_Help				SEARCH_CMD_CHAR"help"
#define SEARCH_CMD_Root				SEARCH_CMD_CHAR"root"
#define SEARCH_CMD_Tag				SEARCH_CMD_CHAR"tag:"
#define SEARCH_CMD_Path				SEARCH_CMD_CHAR"path"
#define SEARCH_CMD_Covers			SEARCH_CMD_CHAR"covers"
#define SEARCH_CMD_NoCovers			SEARCH_CMD_CHAR"nocovers"
#define SEARCH_CMD_Playlists		SEARCH_CMD_CHAR"playlists"
#define SEARCH_CMD_Playing			SEARCH_CMD_CHAR"playing"

#define SEARCH_DEFAULT_STRING		SEARCH_CMD_CHAR		// hash symbol
#define SEARCH_DEFAULT_DEPTH		2		// as per ::flags.metaSearchDepth


#define SEARCH_OBJTYPE_NULL			0
#define SEARCH_OBJTYPE_PLC			1		// a playlist was selected
#define SEARCH_OBJTYPE_TRACK		2		// a track was selected
#define SEARCH_OBJTYPE_STRING		3		// used with #covers, a string item was selected
#define SEARCH_OBJTYPE_IMAGE		4		// used with #covers, an image was selected
#define SEARCH_OBJTYPE_PLAYLIST		5

#define SEARCH_PANEOBJ_IMAGE		1		// there are only two types of 'pane' objects; a string or an image
#define SEARCH_PANEOBJ_TEXT			2		// 

#define SEARCH_INCLUDE_PLAYLIST		0x01
#define SEARCH_INCLUDE_AUDIO		0x02
#define SEARCH_INCLUDE_VIDEO		0x04
#define SEARCH_INCLUDE_OTHER		0x08
#define SEARCH_INCLUDE_TRACK		(SEARCH_INCLUDE_AUDIO|SEARCH_INCLUDE_VIDEO|SEARCH_INCLUDE_OTHER)
#define SEARCH_INCLUDE_ALL			(SEARCH_INCLUDE_PLAYLIST|SEARCH_INCLUDE_TRACK)

#define SEARCH_HEADER_PLAYLIST		1
#define SEARCH_HEADER_AUDIO			2
#define SEARCH_HEADER_VIDEO			3
#define SEARCH_HEADER_OTHER			4

#define SEARCH_CASE_DEFAULT			PLAYLIST_SEARCH_CASE_INSENSITIVE


#define SEARCH_LAYOUT_HORIZONTAL	1
#define SEARCH_LAYOUT_VERTICAL		2

#define SEARCH_APPEND_NEW			1
#define SEARCH_APPEND_ADD			2
#define SEARCH_SCROLL_DELTA			40
#define SEARCH_PANE_VPITCH			36

enum _menuType {
	SEARCH_MENU_FOLDER = 1,
	SEARCH_MENU_TRACKPLAY,
	SEARCH_MENU_TRACKSTOP,
	SEARCH_MENU_OTHER,
	SEARCH_MENU_META,
	SEARCH_MENU_EDIT,
	SEARCH_MENU_FORMAT,
	SEARCH_MENU_SEARCH,
	SEARCH_MENU_IMAGE,
	SEARCH_MENU_ROWS,
	SEARCH_MENU_INFO,
	SEARCH_MENU_ARTWORK
};


			
enum _cmenu {
	SEARCH_MENU_ITEM_PLAY = 200,
	SEARCH_MENU_ITEM_STOP,
	SEARCH_MENU_ITEM_OPEN,
	SEARCH_MENU_ITEM_INFO,
	SEARCH_MENU_ITEM_OPENLOCATION,
	SEARCH_MENU_ITEM_OPENUID,
	
	SEARCH_MENU_ITEM_OTHER_STOPSEARCH,
	SEARCH_MENU_ITEM_OTHER_FINDSTRING,
	SEARCH_MENU_ITEM_OTHER_BACK,	// previous search
		
	SEARCH_MENU_ITEM_NOACTION,
	SEARCH_MENU_ITEM_CLOSEMENU,
	SEARCH_MENU_ITEM_COPY,

	SEARCH_MENU_ITEM_META_COPYTITLE,
	SEARCH_MENU_ITEM_META_COPYTRACKPATH,
	SEARCH_MENU_ITEM_META_COPYARTPATH,
	SEARCH_MENU_ITEM_META_COPYALL,
	SEARCH_MENU_ITEM_META_COPYTRACKPATHS,
	SEARCH_MENU_ITEM_META_COPYTRACKTITLES,
	SEARCH_MENU_ITEM_META_SETARTWORK,
	SEARCH_MENU_ITEM_META_OPENARTPATH,
	SEARCH_MENU_ITEM_META_VIEWARTWORK,

	SEARCH_MENU_ITEM_ARTWORK_CLOSE,
	
	SEARCH_MENU_ITEM_FORMAT,
	SEARCH_MENU_ITEM_FORMAT_ARTIST_TITLE,
	SEARCH_MENU_ITEM_FORMAT_PLAYLIST_TITLE,
	SEARCH_MENU_ITEM_FORMAT_ALBUM_TITLE,
	SEARCH_MENU_ITEM_FORMAT_UID_TITLE,
	SEARCH_MENU_ITEM_FORMAT_ARTIST_FILENAME,
	SEARCH_MENU_ITEM_FORMAT_TITLE,
	SEARCH_MENU_ITEM_FORMAT_FILENAME,
	SEARCH_MENU_ITEM_FORMAT_PATH,
	SEARCH_MENU_ITEM_FORMAT_PARENT_PLAYLIST,
	SEARCH_MENU_ITEM_FORMAT_UID_PLAYLIST,
	SEARCH_MENU_ITEM_FORMAT_PLAYLIST,
	SEARCH_MENU_ITEM_FORMAT_TRACKNO,
	
	SEARCH_MENU_ITEM_EDIT,
	SEARCH_MENU_ITEM_EDIT_NOACTION,
	SEARCH_MENU_ITEM_EDIT_CUT,
	SEARCH_MENU_ITEM_EDIT_COPY,
	SEARCH_MENU_ITEM_EDIT_PASTE,
	SEARCH_MENU_ITEM_EDIT_DELETE,
	SEARCH_MENU_ITEM_EDIT_RENAME,
	SEARCH_MENU_ITEM_EDIT_RENAMESAVE,
	
	SEARCH_MENU_ITEM_SEARCH,
	SEARCH_MENU_ITEM_SEARCH_PARENT,
	SEARCH_MENU_ITEM_SEARCH_CLEAR,
	SEARCH_MENU_ITEM_SEARCH_APPEND,
	SEARCH_MENU_ITEM_SEARCH_MATCHHOW,
	SEARCH_MENU_ITEM_SEARCH_HELP,
	SEARCH_MENU_ITEM_SEARCH_OPENUID,
	SEARCH_MENU_ITEM_SEARCH_TITLE,
	SEARCH_MENU_ITEM_SEARCH_ARTIST,
	SEARCH_MENU_ITEM_SEARCH_ALBUM,
	
	SEARCH_MENU_ITEM_IMAGE,
	SEARCH_MENU_ITEM_IMAGE_ROWS,
	SEARCH_MENU_ITEM_IMAGE_ROWS_1,
	SEARCH_MENU_ITEM_IMAGE_ROWS_2,
	SEARCH_MENU_ITEM_IMAGE_ROWS_3,
	SEARCH_MENU_ITEM_IMAGE_ROWS_4,
	SEARCH_MENU_ITEM_IMAGE_ROWS_5,
	SEARCH_MENU_ITEM_IMAGE_ROWS_6,
	SEARCH_MENU_ITEM_IMAGE_ORDER_ASCEND,
	SEARCH_MENU_ITEM_IMAGE_ORDER_DESCEND,
	SEARCH_MENU_ITEM_IMAGE_FLUSH

};

enum _editOp {
	EDIT_OP_NONE,
	EDIT_OP_CUT,
	EDIT_OP_COPY,
	EDIT_OP_PASTE
};

enum _resultFormat {
	// track
	SEARCH_FORMAT_ARTIST_TITLE = 1,
	SEARCH_FORMAT_PLAYLIST_TITLE,
	SEARCH_FORMAT_ALBUM_TITLE,
	SEARCH_FORMAT_UID_TITLE,
	SEARCH_FORMAT_ARTIST_FILENAME,
	SEARCH_FORMAT_TITLE,
	SEARCH_FORMAT_FILENAME,
	SEARCH_FORMAT_PATH,
	
	// playlist
	SEARCH_FORMAT_PARENT_PLAYLIST,
	SEARCH_FORMAT_UID_PLAYLIST,
	SEARCH_FORMAT_PLAYLIST,
	
	SEARCH_FORMAT_TOTAL,
	
	SEARCH_FORMAT_TRACK_DEFAULT = SEARCH_FORMAT_PLAYLIST_TITLE,
	SEARCH_FORMAT_PLAYLIST_DEFAULT = SEARCH_FORMAT_UID_PLAYLIST
};






enum _searchBtns {
	SEARCH_BTN_BACK,
	SEARCH_BTN_REFRESH,
	SEARCH_BTN_STOP,
	SEARCH_BTN_LAYOUT,
	
	SEARCH_BTN_TOTAL
};

typedef struct{
	TPAGE2COM *com;
	TCCBUTTONS *btns;
	TPANE *pane;
	plm_search plms;
	 
	struct{
		TLABEL *box;
		int boxStrId;		// search string is here
		unsigned int strHash;
		int flags;			// search for bit flags (playlists' but not tracks, audio only, etc..)
		int matchHow;		// PLAYLIST_SEARCH_xxxx
		
		volatile int state;
		unsigned int tid;
		HANDLE hthread;
		int count;			// number of searches performed thus far

		struct{				// SEARCH_FORMAT_XXXX
			int playlist;
			int track;		// anything which isn't a playlist
			int trackNo;
			char *buffer;
		}format;
	}search;

	struct{
		int rows;
		int order;
		double scale;
	}image;

	struct{
		TSTACK *stack;
		int xoffset;
		int yoffset;
		int stub3;
		int stub4;
		int stub5;
	}history;
	
	struct{
		int itemId;
		int recType;				// PLAYLIST_OBJTYPE_xxx
		int uid;
		int track;					// trackId
		int imgId;					// selected image when in image search mode
		int objType;				// SEARCH_PANEOBJ_xxx. what was selected: text or image
		
		int itemHighlighted;		// 1:highlight selected item, 0: don't
		struct{
			unsigned int fore;
			unsigned int back;
			unsigned int out;
		}colour;
		
		struct{
			int op;					// _editOp
			struct{
				int uid;
				int trackId;
				int itemId;
				int recType;		// (playlist or track)
			}src;
		}edit;
		
		struct{
			int uid;
		}open;
	}selected;
	
	struct{
		TPANE *pane;
		int id;
		int tItems;
		int font;
	}context;

	struct{
		int type;
		double t0;
		int inputId;
	}menu;
	
	struct{
		int displayMode;			// display pane layout mode
		int appendMode;				// append new items to existing results, or create new search
		double inProcessAngle;
		int metaSearchDepth;		// how many playlists (children) deep should the meta engine search per query
	}flags;
	
	struct{
		int playlist;
		int audio;
		int video;
		int other;
	}stats;

	struct{
		TPANE *pane;
		struct{
			int playlist;
			int audio;
			int video;
			int other;
		}ids;			// pane references
	}header;
	
	struct{
		int searchInProgress;
		TFRAME *imgProgress;
		
		int playlist;
		int audio;
		int video;
		int other;
		int playlistX;
		int audioX;
		int videoX;
		int otherX;
		
		int play;
		int close;
		int info;
		int stop;		// 
		int open;		// open location
		int find;
		int editMenu;	// edit menu header underlay		
		int tag;
		int dvb;
		int ay;
		int noart;
	}icons;
}TSEARCH;


int page_searchCb (void *pageObj, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

void searchForceStop (TVLCPLAYER *vp);
void searchFindNewString(TSEARCH *search, const char *str);

void timer_searchEnded (TVLCPLAYER *vp);
void timer_searchUpdateHeader (TVLCPLAYER *vp);

void searchMetaCb (TVLCPLAYER *vp, const int msg, const int uid, const int track, void *dataPtr1, void *dataPt2);
void timer_metaCb (TVLCPLAYER *vp);

#endif


