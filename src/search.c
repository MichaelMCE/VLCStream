
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




// for casting 32bit int to 64bit ptr, eg; .data2Ptr = (intPtr)ihash32
#define intPtr void*)(intptr_t
#define intToStr(val) itoa((val),buffer,10)


extern int SHUTDOWN;




static inline int searchPlaylistMetaGetMeta (TSEARCH *search, PLAYLISTCACHE *plc, const int fromIdx, const int toIdx)
{
	return playlistMetaGetMeta(search->com->vp, plc, fromIdx, toIdx, NULL);
}

static inline int searchPlaylistInitiateMetaRetrieval (TSEARCH *search, const int uid, int depth)
{
	if (depth < 1) return depth;
	
	TVLCPLAYER *vp = search->com->vp;
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	if (!plc) return 0;

	const int total = playlistGetTotal(plc);
	if (total){
		for (int i = 0; i < total; i++){
			if (playlistGetItemType(plc, i) == PLAYLIST_OBJTYPE_PLC){
				if (searchPlaylistInitiateMetaRetrieval(search, playlistGetPlaylistUID(plc, i), depth-1) < 0)
					return depth;
			}
		}

		playlistGetTrackLengths(vp, plc, 0, 0);
		searchPlaylistMetaGetMeta(search, plc, 0, total-1);
	}
	return --depth;
}

static inline int searchInitiateMetaRetrieval (TSEARCH *search, const int uid, const int depth)
{
	return searchPlaylistInitiateMetaRetrieval(search, uid, depth);
}

static inline int searchAddItem (TSEARCH *search, const int imgId, const char *str, const int64_t data64)
{
	return paneTextAdd(search->pane, imgId, 1.0, str, SEARCH_HELP_FONT, data64);
}

static inline void searchAddHelp (TSEARCH *search)
{
	static const char *help[] = {
	"Enter something to search:",
	"eg; 'Hello world' will perform a standard query using the",
	"	search order as detailed below",
	"&#12288;\n",
	"Alternatively there are a few commands available to help things along",
	" # is a the command character (anything else is considered a search query)",
	" eg; '#tag:date:2005' will initiate a metadata search of all tracks for the year 2005",
	"&#12288;&#12288; ",
	"\t#nnn		             Display contents of playlist nnn (eg; #2C8)",
	"\t#:  	  	              Display all tracks",
	"\t#*  	  	             Display all playlists'",
	"\t#:* 	  	             All files then playlists",
	"\t#*: 	  	             All playlists' then files",
	"\t#.  	  	              Everything; Unordered as and when found",
	"\t#tag:type 		     Search media for this metatag (eg; #tag:album)",
	"\t#tag:type:value     As above but search for string within tag (#tag:title:hello)   ",
	"\t#covers	 	       Search by track/album cover",
	"\t#nocovers	 	   Find playlists without cover art",
	"\t#playlists		      Search for tracks by playlist cover",
	"\t#playing 		      Search the queued playlist",
	"\t#root    		       Search from root of playlist tree",
	"\t#help     		      This page",
	" \n",
	"Standard query search order:",
	"\t\tTitle \u2192 Path &#8594; Artist &#8594; Album &#8594;",
	"\t\tGenre &#8594; Description &#8594; Date &#8594; Length",
	" \n"};

	int total = (sizeof(help) / sizeof(char*));
	for (int i = 0; i < total; i++)
		searchAddItem(search, 0, help[i], (int64_t)SEARCH_OBJTYPE_NULL<<48);
		

	searchAddItem(search, 0, "Tag types:", (int64_t)SEARCH_OBJTYPE_NULL<<48);
	char buffer[256];
	buffer[0] = 0;
	strncat(buffer, "\t\t", 16);
	const char *tag = tagToString(0);
	strncat(buffer, tag, 16);
	
	int i = 1;
	do{
		if (i == MTAG_ArtworkPath || i == MTAG_POSITION) i++;
		const char *tag = tagToString(i++);
		if (tag){
			strncat(buffer, ", ", 16);

			//if (i && !(i%6)){
			if (i == 7 || i == 13 || i == 18 || i == 23){
				searchAddItem(search, 0, buffer, (int64_t)SEARCH_OBJTYPE_NULL<<48);
				*buffer = 0;
				strncat(buffer, "\t\t", 16);
			}
			strncat(buffer, tag, 16);
		}
	}while(i < MTAG_TOTAL);

	searchAddItem(search, 0, buffer, (int64_t)SEARCH_OBJTYPE_NULL<<48);
}

static inline void searchFindNewUID (TSEARCH *search, const int uid)
{
	//printf("searchFindNewUID %X\n", uid);
	
	char buffer[32];
	__mingw_snprintf(buffer, 31, "#%X", uid);
	searchFindNewString(search, buffer);
}

static inline int searchDisplayIsPlaylistsMode (TSEARCH *search)
{
	int ret = 0;
	
	char *str = labelStringGet(search->search.box, search->search.boxStrId);
	if (str){
		ret = !stricmp(str, SEARCH_CMD_Playlists);
		my_free(str);
	}
	return ret;
}

static inline int searchDisplayIsNoCoverMode (TSEARCH *search)
{
	int ret = 0;
	
	char *str = labelStringGet(search->search.box, search->search.boxStrId);
	if (str){
		ret = !stricmp(str, SEARCH_CMD_NoCovers);
		my_free(str);
	}
	return ret;
}

static inline int searchDisplayIsCoverMode (TSEARCH *search)
{
	int ret = 0;
	
	char *str = labelStringGet(search->search.box, search->search.boxStrId);
	if (str){
		ret = !stricmp(str, SEARCH_CMD_Covers);
		my_free(str);
	}
	return ret;
}

static inline int calcBinLen (TMETATAGCACHE *tagc)
{
	int ct = 1;
	
	if (tagLock(tagc)){
		TMETARECORD *rec = tagc->first;
		if (rec){
			while((rec=rec->next))
				ct++;
		}
		tagUnlock(tagc);
	}
	return ct;
}

static inline int addBinHash (unsigned int *bin, const int len, const int where, const unsigned int hash)
{
	for (int i = 0; i < where && i < len; i++){
		if (bin[i] == hash) return 1;
	}
	bin[where] = hash;
	return 0;
}

static inline int searchAddTags (TSEARCH *search, const int mtag, const char *value)
{
	TVLCPLAYER *vp = search->com->vp;
	TMETATAGCACHE *tagc = vp->tagc;

	int binLength = calcBinLen(tagc);
	unsigned int hashBin[binLength];
	int ct = 0;
	const int64_t recType = (int64_t)SEARCH_OBJTYPE_STRING<<48;
	
	if (tagLock(tagc)){
		TMETARECORD *rec = tagc->first;
		if (rec){
			do{
				TMETAITEM *item = rec->item;
				if (item && item->tag){
					if (item->tag[mtag]){
						char *str = item->tag[mtag];
						if (!value || (value && stristr(str, value))){
							unsigned int hash = getHash(str);
							
							// don't allow duplicate strings, apart from substring searching (value)
							if (value || !addBinHash(hashBin, binLength, ct, hash)){
								if (item->tag[MTAG_PATH])
									hash = getHash(item->tag[MTAG_PATH]);
								else
									hash = 0;

								searchAddItem(search, search->icons.tag, str, recType|hash);
								ct++;
							}
						}
					}
				}
			}while((rec=rec->next));
		}
		tagUnlock(tagc);
	}
	//printf("total %i\n", ct);
	search->search.count++;
	return ct;
}

static inline void searchScrollReset (TSEARCH *search)
{
	paneScrollReset(search->pane);
	search->history.xoffset = 0;
	search->history.yoffset = 0;
}

static inline void searchHistroyCreate (TSEARCH *search, const int ssize)
{
	search->history.stack = stackCreate(ssize);
}

static inline void searchHistroyDestroy (TSEARCH *search)
{
	stackDestroy(search->history.stack);
}

static inline void searchHistroyClean (TSEARCH *search)
{
	char *str = NULL;
	while (stackPop(search->history.stack, (intptr_t*)&str))
		my_free(str);
}

static inline int searchHistroyAdd (TSEARCH *search, const char *str)
{
	int ret = 0;

	int count = stackCount(search->history.stack);
	if (!count)
		return stackPush(search->history.stack, (intptr_t)my_strdup(str));

	// don't re-add the same item
	char *stackStr = NULL;
	stackPeek(search->history.stack, (intptr_t*)&stackStr);
	if (stricmp(stackStr, str))
		ret = stackPush(search->history.stack, (intptr_t)my_strdup(str));
		
	return ret;
}

static inline int searchHistroyIsAvailable (TSEARCH *search)
{
	return stackCount(search->history.stack) > 1;
}

static inline char *searchHistroyPreview (TSEARCH *search)
{
	char *stackStr = NULL;
	stackPeekEx(search->history.stack, (intptr_t*)&stackStr, 1);
	if (stackStr)
		return my_strdup(stackStr);
	return NULL;
}

static inline char *searchHistroyGet (TSEARCH *search)
{
	int count = stackCount(search->history.stack);
	if (count < 2) return NULL;
				
	char *str = NULL;
	stackPop(search->history.stack, (intptr_t*)&str);	// we don't want the present
	if (str) my_free(str);
		
	str = NULL;
	stackPop(search->history.stack, (intptr_t*)&str);	// before present, history
	return str;
}

static inline void searchSetLineHeight (TSEARCH *search, const int height)
{
	TPANE *pane = search->pane;
	//printf("searchSetLineHeight %i (was: %i)\n", height, pane->vertLineHeight);

	pane->vertLineHeight = height;
}

static inline int searchGetLineHeight (TSEARCH *search)
{
	TPANE *pane = search->pane;
	
	return pane->vertLineHeight;
}

static inline void searchContextItemSetColour (TSEARCH *search, const int itemId, const int _fore, const int _back, const int _out)
{
	unsigned int fore = 0, back = 0, out = 0;
	if (labelRenderColourGet(search->context.pane->base, itemId, &fore, &back, &out)){
		int foreNew = (fore&0xFF000000)|(_fore&0xFFFFFF);
		int outNew = (out&0xFF000000)|(_out&0xFFFFFF);
		int backNew = (back&0xFF000000)|(_back&0xFFFFFF);
		labelRenderColourSet(search->context.pane->base, itemId, foreNew, backNew, outNew);
	}
}

static inline void searchItemSetColour (TSEARCH *search, const int itemId, const int _fore, const int _back, const int _out)
{
	unsigned int fore = 0, back = 0, out = 0;
	if (labelRenderColourGet(search->pane->base, itemId, &fore, &back, &out)){
		int foreNew = (fore&0xFF000000)|(_fore&0xFFFFFF);
		int outNew = (out&0xFF000000)|(_out&0xFFFFFF);
		int backNew = (back&0xFF000000)|(_back&0xFFFFFF);
		labelRenderColourSet(search->pane->base, itemId, foreNew, backNew, outNew);
	}
}

static inline void searchItemSetHighlight (TSEARCH *search, const int itemId)
{
	unsigned int fore = 0, back = 0, out = 0;
	if (labelRenderColourGet(search->pane->base, itemId, &fore, &back, &out)){
		int foreNew = (fore&0xFF000000)|0xFFFFAA;
		int outNew = (out&0xFF000000)|0xFFAA00;
		int backNew = (back&0xFF000000)|COL_BLUE;
		labelRenderColourSet(search->pane->base, itemId, foreNew, backNew, outNew);
			
		search->selected.colour.fore = fore;
		search->selected.colour.back = back;
		search->selected.colour.out = out;
		search->selected.itemHighlighted = 1;
	}
}

static inline void searchItemRemoveHighlight (TSEARCH *search, const int itemId)
{
	search->selected.itemHighlighted = 0;
	labelRenderColourSet(search->pane->base, itemId, search->selected.colour.fore, search->selected.colour.back, search->selected.colour.out);
}

static inline int searchContextIsVisable (TSEARCH *search)
{
	TPANE *pane = search->context.pane;
	
	return ccGetState(pane);
}

static inline void searchContextSetLineHeight (TSEARCH *search, const int height)
{
	TPANE *pane = search->context.pane;
	//printf("searchContextSetLineHeight %i (was: %i)\n", height, pane->vertLineHeight);

	pane->vertLineHeight = height;
}

static inline int searchContextGetLineHeight (TSEARCH *search)
{
	TPANE *pane = search->context.pane;
	
	return pane->vertLineHeight;
}

static inline void searchContextSetFont (TSEARCH *search, const int font)
{
	search->context.font = font;
}

static inline void searchContextSetPosition (TSEARCH *search, int x, int y)
{
	TPANE *pane = search->context.pane;
	
	const int fw = pageGetSurfaceWidth(search);
	const int fh = pageGetSurfaceHeight(search);
	const int cw = ccGetWidth(pane);
	const int ch = ccGetHeight(pane);
	const int padding = 3;
	
	if (x+cw+padding > fw) x = (fw - cw)-padding;
	if (x < padding) x = padding;
	
	if (y+ch+padding > fh) y = (fh - ch)-padding;
	if (y < padding) y = padding;
	
	ccSetPosition(pane, x, y);
}

static inline void searchContextShow (TSEARCH *search)
{
	//printf("searchContextShow\n");
	
	TPANE *pane = search->context.pane;

	ccEnable(pane);
	ccSetModal(search->com->vp->cc, search->context.id);
	pageUpdate(search);
}

static inline void searchContextHide (TSEARCH *search)
{
	//printf("searchContextHide %i\n", search->selected.itemHighlighted);
	
	TPANE *pane = search->context.pane;
	ccHoverRenderSigEnable(pane->cc, 20.0);
	paneScrollReset(pane);	
	paneSwipeDisable(pane);
	//paneTextMulityLineDisable(pane);
	
	if (search->selected.itemHighlighted)
		searchItemRemoveHighlight(search, search->selected.itemId);

	int id = 0;
	ccGetModal(search->com->vp->cc, &id);
	if (id == search->context.id)
		ccSetModal(search->com->vp->cc, 0);
	ccDisable(pane);
}

static inline void searchContextClear (TSEARCH *search)
{
	TPANE *pane = search->context.pane;
	
	paneRemoveAll(pane);
	searchContextSetLineHeight(search, SEARCH_PANE_VPITCH);
	searchContextSetFont(search, SEARCH_CONTEXT_FONT);
	search->context.tItems = 0;
}

static inline void drawRectangleFilled (TFRAME *frame, TMETRICS *met, const unsigned int colour, const int blur)
{
	lDrawRectangleFilled(frame, met->x, met->y, met->x+met->width-1, met->y+met->height-1, colour);
	//if (blur)
	//	lBlurArea(frame, met->x, met->y, met->x+met->width-1, met->y+met->height-1, 2);
}

static inline void searchContextRender (TSEARCH *search, TFRAME *frame)
{
	TPANE *pane = search->context.pane;

	ccRender(pane, frame);
}

static inline int searchContextAddImage (TSEARCH *search, const int imgId, const int mId)
{
	TPANE *pane = search->context.pane;

	int itemId = paneImageAdd(pane, imgId, 1.0, PANE_IMAGE_CENTRE, 0, 0, mId);
	if (itemId){
		search->context.tItems++;
		paneFocusSet(pane, 0);

		int w, h;
		artManagerImageGetMetrics(pane->cc->vp->am, imgId, &w, &h);
		int x = abs(pageGetSurfaceWidth(search) - w) / 2;
		int y = abs(pageGetSurfaceHeight(search) - h) / 2;
		ccSetMetrics(pane, x, y, w, h);
	}

	return itemId;
}

static inline int searchContextAddItem (TSEARCH *search, const char *str, const int imgId, const int mId)
{
	TPANE *pane = search->context.pane;

	int itemId = paneTextAdd(pane, imgId, 1.0, str, search->context.font, mId);
	if (itemId){
		search->context.tItems++;
		paneFocusSet(pane, 0);

		int h = (search->context.tItems * pane->vertLineHeight) + 2;
		if (h > ccGetHeight(search->pane)-4) h = ccGetHeight(search->pane)-4;
		int w = pane->tItemWidth + 3;
		if (w > ccGetWidth(search->pane)-6) w = ccGetWidth(search->pane)-6;

		ccSetMetrics(pane, -1, -1, w, h);
	}

	return itemId;
}

static inline void searchUpdateHeaderStats (TSEARCH *search, const int tPly, const int tAud, const int tVid, const int tOtr)
{
	char buffer[16];
	TPANE *pane = search->header.pane;

	if (ccLock(pane)){
		paneTextReplace(pane, search->header.ids.playlist, intToStr(tPly));
		paneTextReplace(pane, search->header.ids.audio, intToStr(tAud));
		paneTextReplace(pane, search->header.ids.video, intToStr(tVid));
		paneTextReplace(pane, search->header.ids.other, intToStr(tOtr));
		ccUnlock(pane);
	}
}

static inline char *getFilename (TVLCPLAYER *vp, const unsigned int hash, PLAYLISTCACHE *plc, const int trackIdx)
{
	char *fname = tagRetrieveDup(vp->tagc, hash, MTAG_FILENAME);
	if (!fname || !*fname){
		char *path = playlistGetPathDup(plc, trackIdx);
		if (!path) return NULL;
		
		const int len = strlen(path);
		char filename[len*2];
		char ext[len+1];

		_splitpath(path, NULL, NULL, filename, ext);

		fname = my_malloc((len*2)+1);
		__mingw_snprintf(fname, len*2, "%s%s", filename, ext);
		tagAddByHash(vp->tagc, hash, MTAG_FILENAME, fname, 1);
		my_free(path);
	}
	
	return fname;
}


static inline int plms_searchCb (void *uptr, const char *searchedFor, const int uid, const int trackIdx, const int foundInMTag, const unsigned int hash, const int recType, const int count)
{
	//printf("%s: %X:%i %i %X %i %i\n", searchedFor, uid, trackIdx, foundInMTag, hash, recType, count);
	
	TVLCPLAYER *vp = (TVLCPLAYER*)uptr;
	TSEARCH *search = pageGetPtr(vp, PAGE_SEARCH);
	TPANE *pane = search->pane;
	
	const int searchFlags = search->search.flags;
	if (recType == PLAYLIST_OBJTYPE_PLC){
		if (!(searchFlags&SEARCH_INCLUDE_PLAYLIST))
			return search->search.state;
	}else if (recType == PLAYLIST_OBJTYPE_TRACK){
		if (!(searchFlags&SEARCH_INCLUDE_TRACK))
			return search->search.state;
	}
	
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	if (!plc) return search->search.state;
	
	double scale = 1.0;
	int imgId = 0;
	
	if (recType == PLAYLIST_OBJTYPE_PLC){
		imgId = search->icons.playlist;
		search->stats.playlist++;
		
	}else if (recType == PLAYLIST_OBJTYPE_TRACK){
		if ((imgId=playlistGetArtId(plc, trackIdx)))
			scale = 33.0/(double)vp->gui.artMaxHeight;

		char *path = playlistGetPathDup(plc, trackIdx);
		if (path){
			if (isMediaAudio8(path)){
				if (!(searchFlags&SEARCH_INCLUDE_AUDIO)){
					my_free(path);
					return search->search.state;
				}
				search->stats.audio++;
				
				if (!imgId){
					if (isAyFile8(path))
						imgId = search->icons.ay;
					else
						imgId = search->icons.audio;
				}
			}else if (isMediaVideo8(path)){
				if (!(searchFlags&SEARCH_INCLUDE_VIDEO)){
					my_free(path);
					return search->search.state;
				}
				search->stats.video++;
				
				if (!imgId){
					if (isMediaDVB(path))
						imgId = search->icons.dvb;
					else
						imgId = search->icons.video;
				}
			}else{
				if (!(searchFlags&SEARCH_INCLUDE_OTHER)){
					my_free(path);
					return search->search.state;
				}
				search->stats.other++;
				//printf("other: %i '%s'\n", search->stats.other, path);
			}
			my_free(path);
		}
	}
	
	//if (imgId == 0x70)
	//	printf("%s: %X:%i %i %X %i %i\n", searchedFor, uid, trackIdx, foundInMTag, hash, recType, count);
	
	if (!imgId) imgId = search->icons.other;
	char *strA = NULL;
	char *strB = NULL;
	
	if (recType == PLAYLIST_OBJTYPE_PLC){
		if (search->search.format.playlist == SEARCH_FORMAT_PARENT_PLAYLIST){
			strA = playlistGetNameDup(plc);
			strB = playlistGetTitleDup(plc, trackIdx);
		}else if (search->search.format.playlist == SEARCH_FORMAT_UID_PLAYLIST){
			strA = intToHex(playlistGetPlaylistUID(plc, trackIdx));
			strB = playlistGetTitleDup(plc, trackIdx);
		}else{
			strA = playlistGetTitleDup(plc, trackIdx);
		}
	}else{ //if (recType == PLAYLIST_OBJTYPE_TRACK){
		if (search->search.format.track == SEARCH_FORMAT_ARTIST_TITLE){
			strA = tagRetrieveDup(vp->tagc, hash, MTAG_Artist);
			if (!strA) strA = playlistGetNameDup(plc);
			strB = playlistGetTitleDup(plc, trackIdx);
		}else if (search->search.format.track == SEARCH_FORMAT_PLAYLIST_TITLE){
			strA = playlistGetNameDup(plc);
			strB = playlistGetTitleDup(plc, trackIdx);
		}else if (search->search.format.track == SEARCH_FORMAT_UID_TITLE){
			strA = intToHex(plc->uid);
			strB = playlistGetTitleDup(plc, trackIdx);
		}else if (search->search.format.track == SEARCH_FORMAT_ALBUM_TITLE){
			strA = tagRetrieveDup(vp->tagc, hash, MTAG_Album);
			if (!strA) strA = playlistGetNameDup(plc);
			strB = playlistGetTitleDup(plc, trackIdx);
		}else if (search->search.format.track == SEARCH_FORMAT_ARTIST_FILENAME){
			strA = tagRetrieveDup(vp->tagc, hash, MTAG_Artist);
			if (!strA) strA = playlistGetNameDup(plc);
			strB = getFilename(vp, hash, plc, trackIdx);
			if (!strB) strB = playlistGetPathDup(plc, trackIdx);
		}else if (search->search.format.track == SEARCH_FORMAT_TITLE){
			strA = playlistGetTitleDup(plc, trackIdx);
			if (!strA) strA = playlistGetPathDup(plc, trackIdx);
		}else if (search->search.format.track == SEARCH_FORMAT_FILENAME){
			strA = getFilename(vp, hash, plc, trackIdx);
			if (!strA) strA = playlistGetPathDup(plc, trackIdx);
		}else if (search->search.format.track == SEARCH_FORMAT_PATH){
			strA = playlistGetPathDup(plc, trackIdx);
		}
	}

	char *str = search->search.format.buffer;
	if (strA && strB){
		if (search->search.format.trackNo)
			__mingw_snprintf(str, MAX_PATH_UTF8, "%i: %s :: %s", trackIdx+1, strA, strB);
		else
			__mingw_snprintf(str, MAX_PATH_UTF8, "%s :: %s", strA, strB);
		my_free(strA);
		my_free(strB);
	}else if (strA){
		if (search->search.format.trackNo)
			__mingw_snprintf(str, MAX_PATH_UTF8, "%i: %s", trackIdx+1, strA);
		else
			strncpy(str, strA, MAX_PATH_UTF8);
		my_free(strA);
	}else{
		if (strB) my_free(strB);
		return search->search.state;
	}
	

	int trackId = playlistGetId(plc, trackIdx);
	int64_t id = ((int64_t)recType<<48)|((int64_t)uid<<32)|trackId;
	//printf("searchCb: %I64X %i %X\n", id, trackIdx, trackId);
	
	paneTextAdd(pane, imgId, scale, str, SEARCH_RESULT_FONT, id);
	timerSet(vp, TIMER_SEARCH_UPDATEHEADER, 90);
	
	
	return search->search.state;
}

// TIMER_SEARCH_UPDATEHEADER
void timer_searchUpdateHeader (TVLCPLAYER *vp)
{
	TSEARCH *search = pageGetPtr(vp, PAGE_SEARCH);
	searchUpdateHeaderStats(search, search->stats.playlist, search->stats.audio, search->stats.video, search->stats.other);
	renderSignalUpdate(vp);
}

// TIMER_SEARCH_ENDED
void timer_searchEnded (TVLCPLAYER *vp)
{
	TSEARCH *search = pageGetPtr(vp, PAGE_SEARCH);
	buttonsStateSet(search->btns, SEARCH_BTN_REFRESH, 1);
	buttonsStateSet(search->btns, SEARCH_BTN_STOP, 0);
	renderSignalUpdate(vp);
}

static inline void searchSetSearchString (TSEARCH *search, const char *str)
{
	unsigned int hash = getHash(str);
	if (hash != search->search.strHash){
		search->search.strHash = hash;
		labelStringSet(search->search.box, search->search.boxStrId, str);
	}
}

unsigned int __stdcall searchThread (void *ptr)
{
	//const int tid = GetCurrentThreadId();
	//printf("searchThread start %X\n", tid);

	plm_search *plms = (plm_search*)ptr;
	TVLCPLAYER *vp = plms->uptr;
	TSEARCH *search = pageGetPtr(vp, PAGE_SEARCH);


	char *searchFor = labelStringGet(search->search.box, search->search.boxStrId);
	if (searchFor){
		searchHistroyAdd(search, searchFor);

		plms->string = searchFor;
		plms->activeState = &search->search.state;

		//dbprintf(vp, "Searching for '%s'..", searchFor);
		//double t0 = getTime(vp);
		/*int found =*/ playlistManagerSearchEx(vp->plm, plms, plms->from, plms->to);
		/*double t1 = getTime(vp);
		printf("%i matches found for '%s' in %.0fms\n", found, searchFor, t1-t0);*/
		my_free(searchFor);
	}

	search->search.state = 0;
	searchUpdateHeaderStats(search, search->stats.playlist, search->stats.audio, search->stats.video, search->stats.other);
	timerSet(vp, TIMER_SEARCH_ENDED, 0);

	//printf("searchThread end %X\n", tid);
	_endthreadex(1);
	return 1;
}

static inline int searchCtrlWait (TSEARCH *search)
{
	if (search->search.state)
		return (WaitForSingleObject(search->search.hthread, 4*60*1000) == WAIT_OBJECT_0);

	return 0;
}

static inline void searchCtrlStop (TSEARCH *search)
{
	//printf("searchCtrlStop in %i\n", search->search.state);

	if (search->search.state){
		search->search.state = 0;
		if ((WaitForSingleObject(search->search.hthread, 3*60*1000) == WAIT_OBJECT_0)){
			//CloseHandle(search->search.hthread);
		}
	}
	//printf("searchCtrlStop out %i\n", search->search.state);
}

static inline void searchClearSearch (TSEARCH *search)
{
	//printf("searchClearSearch\n");
	
	search->stats.playlist = 0;
	search->stats.audio = 0;
	search->stats.video = 0;
	search->stats.other = 0;
	searchUpdateHeaderStats(search, 0, 0, 0, 0);
	
	paneRemoveAll(search->pane);
	paneScrollReset(search->pane);
	labelRenderFlagsSet(search->pane->base, labelRenderFlagsGet(search->pane->base)|LABEL_RENDER_TEXT);
	
	searchCtrlStop(search);
	//search->selected.edit.op = EDIT_OP_NONE;
}

static inline void searchMenuInvalidate (TSEARCH *search)
{
	search->menu.type = 0;
}

static inline int searchPreloadItems (TSEARCH *search)
{
	int n = search->image.rows * 6;
	panePreloadItems(search->pane, ((search->image.rows + n)*2));
	search->pane->flags.readAhead.number = n;
	return n;
}

static inline int searchAddPlaylistWithNoCover (TSEARCH *search, PLAYLISTCACHE *plc, const double scale, const int64_t recType)
{
	int totalAdded = 0;
	
	if (playlistLock(plc)){
		if (!plc->artId){
			if (playlistGetCount(plc, PLAYLIST_OBJTYPE_TRACK) > 0)
				totalAdded += paneTextAdd(search->pane, -search->icons.noart, scale, plc->title, SEARCH_HELP_FONT, recType|((int64_t)plc->uid<<32)) > 0;
		}
		playlistUnlock(plc);
	}
	return totalAdded;
}

static inline int searchAddPlaylistsWithNoCover (TSEARCH *search)
{
	//printf("searchAddPlaylistsWithNoCover\n");
	
	TPLAYLISTMANAGER *plm = search->com->vp->plm;
	
	search->image.scale = (double)(ccGetHeight(search->pane)-(1+search->image.rows)) / (double)search->image.rows / (double)search->com->vp->gui.artMaxHeight;
	clipFloat(search->image.scale);
	
	double scale = search->image.scale*1.84;	// 1.84 = maxArtHeight/noArtImageHeight = 442/240
	clipFloat(scale);
	const int64_t recType = (int64_t)SEARCH_OBJTYPE_PLAYLIST<<48;
	int totalAdded = 0;


	if (!search->image.order){
		for (int i = 0; i < plm->p_total; i++){
			if (plm->plc[i])
				totalAdded += searchAddPlaylistWithNoCover(search, plm->plc[i], scale, recType);
		}
	}else{
		for (int i = plm->p_total-1; i >= 0; i--){
			if (plm->plc[i])
				totalAdded += searchAddPlaylistWithNoCover(search, plm->plc[i], scale, recType);
		}
	}
	
	if (totalAdded)
		search->search.count++;
		
	return totalAdded;
}

static inline int searchAddPlaylistCover (TSEARCH *search, PLAYLISTCACHE *plc, const int64_t recType)
{
	int totalAdded = 0;
	
	if (playlistLock(plc)){
		if (plc->artId){
			if (playlistGetCount(plc, PLAYLIST_OBJTYPE_TRACK) > 0)
				totalAdded += paneTextAdd(search->pane, -plc->artId, search->image.scale, " ", SEARCH_HELP_FONT, recType|((int64_t)plc->uid<<32)|plc->artId) > 0;
		}
		playlistUnlock(plc);
	}
	
	return totalAdded;
}

static inline int searchAddPlaylistCovers (TSEARCH *search)
{
	//printf("searchAddPlaylistCovers\n");
	
	TPLAYLISTMANAGER *plm = search->com->vp->plm;
	
	search->image.scale = (double)(ccGetHeight(search->pane)-(1+search->image.rows)) / (double)search->image.rows / (double)search->com->vp->gui.artMaxHeight;
	clipFloat(search->image.scale);
	
	const int64_t recType = (int64_t)SEARCH_OBJTYPE_PLAYLIST<<48;
	int totalAdded = 0;
	int flags = labelRenderFlagsGet(search->pane->base);
	flags &= ~LABEL_RENDER_TEXT;
	labelRenderFlagsSet(search->pane->base, flags);
	

	if (!search->image.order){
		for (int i = 0; i < plm->p_total; i++){
			if (plm->plc[i])
				totalAdded += searchAddPlaylistCover(search, plm->plc[i], recType);
		}
	}else{
		for (int i = plm->p_total-1; i >= 0; i--){
			if (plm->plc[i])
				totalAdded += searchAddPlaylistCover(search, plm->plc[i], recType);
		}
	}
	
	if (totalAdded)
		search->search.count++;
	else
		labelRenderFlagsSet(search->pane->base, flags|LABEL_RENDER_TEXT);
		
	return totalAdded;
}

static inline int sortCB_intA (const void *a, const void *b)
{
	const int *id1 = (int*)a;
	const int *id2 = (int*)b;

	return *id1 - *id2;
}

static inline int sortCB_intD (const void *a, const void *b)
{
	const int *id1 = (int*)a;
	const int *id2 = (int*)b;

	return *id2 - *id1;
}

static inline int searchAddArtworkCache (TSEARCH *search)
{
	//printf("searchAddArtworkCache\n");
	
	int totalAdded = 0;
	int total = 0;

	int flags = labelRenderFlagsGet(search->pane->base);
	flags &= ~LABEL_RENDER_TEXT;
	labelRenderFlagsSet(search->pane->base, flags);


	int *list = artManagerGetIds(search->com->vp->am, &total);
	if (list){
		if (total){
			if (!search->image.order)
				qsort(list, total, sizeof(int), sortCB_intA);
			else
				qsort(list, total, sizeof(int), sortCB_intD);
		
			const int64_t recType = (int64_t)SEARCH_OBJTYPE_IMAGE<<48;
			search->image.scale = (double)(ccGetHeight(search->pane)-(1+search->image.rows)) / (double)search->image.rows / (double)search->com->vp->gui.artMaxHeight;
			//printf("searchAddArtworkCache: %i %f\n", search->image.rows, search->image.scale);
			clipFloat(search->image.scale);
			
			for (int i = 0; i < total && list[i]; i++)
				totalAdded += paneTextAdd(search->pane, -list[i], search->image.scale, "[]", SEARCH_HELP_FONT, recType|list[i]) > 0;
		}
		my_free(list);
	}

	//printf("searchAddArtworkCache: %i %i, %f\n", total, totalAdded, search->image.scale);

	if (totalAdded)
		search->search.count++;
	else
		labelRenderFlagsSet(search->pane->base, flags|LABEL_RENDER_TEXT);
	return totalAdded;
}

static inline void searchSetDisplayMode (TSEARCH *search, const int mode)
{
	if (mode == SEARCH_LAYOUT_HORIZONTAL){
  		search->flags.displayMode = SEARCH_LAYOUT_HORIZONTAL;
  		if (!searchGetLineHeight(search))
  			searchSetLineHeight(search, SEARCH_PANE_VPITCH);
  		
		buttonFaceActiveSet(buttonsButtonGet(search->btns, SEARCH_BTN_LAYOUT), BUTTON_PRI);
		paneScrollReset(search->pane);
  		paneSetLayout(search->pane, PANE_LAYOUT_HORI);
  		paneSetAcceleration(search->pane, PANE_ACCELERATION_X, 0.0);
  		
	}else if (mode == SEARCH_LAYOUT_VERTICAL){
		search->flags.displayMode = SEARCH_LAYOUT_VERTICAL;
		
		buttonFaceActiveSet(buttonsButtonGet(search->btns, SEARCH_BTN_LAYOUT), BUTTON_SEC);
		paneScrollReset(search->pane);
		if (searchDisplayIsCoverMode(search) || searchDisplayIsPlaylistsMode(search) || searchDisplayIsNoCoverMode(search)){
			searchSetLineHeight(search, 0);
  			paneSetLayout(search->pane, PANE_LAYOUT_VERTCENTER);
  		}else{
  			if (!searchGetLineHeight(search))
  				searchSetLineHeight(search, SEARCH_PANE_VPITCH);
  			paneSetLayout(search->pane, PANE_LAYOUT_VERT);
  		}
  		
  		paneSetAcceleration(search->pane, 0.0, PANE_ACCELERATION_Y);
	}
}

static inline int searchCmdProcess (TSEARCH *search, const char *str)
{
	//printf("searchCmdProcess '%s'\n", str);
	
	search->pane->horiColumnSpace = 8;

	if (str[0] != '#' || str[1] == '$') return 0;
	// "#$" is the 'search via artwork reference' command


	if (!strnicmp(str, SEARCH_CMD_Playlists, 10)){
		searchClearSearch(search);
		searchSetDisplayMode(search, SEARCH_LAYOUT_HORIZONTAL);
		paneScrollSet(search->pane, search->history.xoffset, 0, 0);

		int total = searchAddPlaylistCovers(search);
		if (total){
			search->pane->horiColumnSpace = 0;
			searchHistroyAdd(search, str);
			searchUpdateHeaderStats(search, total, 0, 0, 0);
			searchPreloadItems(search);
		}
		//dbprintf(search->com->vp, "Playlists with covers found: %i", total);
		
	}else if (!strnicmp(str, SEARCH_CMD_NoCovers, 9)){
		searchClearSearch(search);
		searchSetDisplayMode(search, SEARCH_LAYOUT_HORIZONTAL);
		paneScrollSet(search->pane, search->history.xoffset, 0, 0);

		int total = searchAddPlaylistsWithNoCover(search);
		if (total){
			search->pane->horiColumnSpace = 17;
			searchHistroyAdd(search, str);
			searchUpdateHeaderStats(search, total, 0, 0, 0);
			searchPreloadItems(search);
		}
		//dbprintf(search->com->vp, "Playlists without a covers: %i", total);
				
	}else if (!strnicmp(str, SEARCH_CMD_Covers, 7)){
		searchClearSearch(search);
		searchSetDisplayMode(search, SEARCH_LAYOUT_HORIZONTAL);
		paneScrollSet(search->pane, search->history.xoffset, 0, 0);

		int total = searchAddArtworkCache(search);
		if (total){
			search->pane->horiColumnSpace = 0;
			searchHistroyAdd(search, str);
			searchUpdateHeaderStats(search, 0, 0, 0, total);
			searchPreloadItems(search);
		}
		//dbprintf(search->com->vp, "Covers found: %i", total);
		
	}else if (!strnicmp(str, SEARCH_CMD_Help, 5)){
		searchClearSearch(search);
		searchHistroyAdd(search, str);
		searchSetSearchString(search, str);
		searchAddHelp(search);

	}else if (!strnicmp(str, SEARCH_CMD_Playing, 8)){
		const int uid = getQueuedPlaylistUID(search->com->vp);
		//printf("uid %X %X\n", uid, search->com->vp->playlist.queued);
		
		if (uid > PLAYLIST_UID_BASE)
			searchFindNewUID(search, uid);
		else
			dbprintf(search->com->vp, "Nothing queued");

	}else if (!strnicmp(str, SEARCH_CMD_Root, 5)){
		searchFindNewUID(search, PLAYLIST_UID_BASE+1);
		
	}else if (!strnicmp(str, SEARCH_CMD_Tag, 5)){	// #tag:
		char *tag = strstr(str, ":")+1;
		if (!tag || !*tag) return 0;

		char *value = strstr(tag, ":");
		if (value){
			*value++ = 0;
			if (!*value) value = NULL;
		}
		if (!*tag) return 0;
		
		//printf("tag:'%s'  value:'%s'\n", tag, value);

		int mtag = tagLookup(tag);
		if (mtag >= 0){
			searchClearSearch(search);
			int total = searchAddTags(search, mtag, value);
			if (value) *--value = ':';
			searchHistroyAdd(search, str);
			searchUpdateHeaderStats(search, 0, 0, 0, total);
			dbprintf(search->com->vp, "Tags found: %i", total);
		}else{
			dbprintf(search->com->vp, "'%s' is not a recognised meta-tag", tag);
			return 0;
		}
	}else{
		return 0;
	}

	searchMenuInvalidate(search);
	return 1;
}

static inline void searchStartAsync (TSEARCH *search, TVLCPLAYER *vp, plm_search *plms)
{
	
	labelRenderFlagsSet(search->pane->base, labelRenderFlagsGet(search->pane->base)|LABEL_RENDER_TEXT);
	buttonsStateSet(search->btns, SEARCH_BTN_REFRESH, 0);
	buttonsStateSet(search->btns, SEARCH_BTN_STOP, 1);
	searchUpdateHeaderStats(search, 0, 0, 0, 0);

	searchSetDisplayMode(search, search->flags.displayMode);
	
	if (search->flags.appendMode == SEARCH_APPEND_NEW){
		search->stats.playlist = 0;
		search->stats.audio = 0;
		search->stats.video = 0;
		search->stats.other = 0;
	}

	//search->selected.edit.op = EDIT_OP_NONE;
	search->search.count++;
	search->search.state = 1;

	plms->tagc = vp->tagc;
	plms->uptr = vp;
	plms->funcCb = plms_searchCb;
	plms->from = 0;
	plms->to = -1;	// last playlist
	plms->wantHash = 1;
	plms->searchHow = search->search.matchHow;
	plms->data = search->selected.imgId;

	search->search.hthread = (HANDLE)_beginthreadex(NULL, 0, searchThread, plms, 0, &search->search.tid);
	SetThreadPriority(search->search.hthread, IDLE_PRIORITY_CLASS);
	pageUpdate(search);
}

static inline void searchCtrlStart (TSEARCH *search, TVLCPLAYER *vp)
{
	char *str = labelStringGet(search->search.box, search->search.boxStrId);
	if (str){
		int ret = searchCmdProcess(search, str);
		my_free(str);
		if (ret) return;
	}

	// string isn't a command so perform a standard search
	searchStartAsync(search, vp, &search->plms);
}

void searchForceStop (TVLCPLAYER *vp)
{
	//printf("searchForceStop\n");
	
	searchCtrlStop(pageGetPtr(vp,PAGE_SEARCH));
}

void searchFindNewString (TSEARCH *search, const char *str)
{
	//printf("searchFindNewString '%s'\n", str);
	
	searchClearSearch(search);
	searchSetSearchString(search, str);
	searchCtrlStop(search);
	searchCtrlStart(search, search->com->vp);
}

static inline void searchOpenSearchKeypad (TSEARCH *search, const int listenerCcId)
{
	
	searchCtrlStop(search);
		
	char *searchFor = NULL;
	if (search->search.count)
		searchFor = labelStringGet(search->search.box, search->search.boxStrId);
	if (!searchFor) searchFor = my_strdup(SEARCH_DEFAULT_STRING);

	TKEYPAD *kp = keyboardGetKeypad(search);
	ccEnable(kp);	// ensure pad is built by enabling it before use
	keypadListenerRemoveAll(kp);
	keypadListenerAdd(kp, listenerCcId, KP_INPUT_OPENED|KP_INPUT_CLOSED|KP_INPUT_COMPLETE8/*|KP_RENDER_PRE*/, 0);
	keypadEditboxSetBuffer8(&kp->editbox, searchFor);
	keypadEditboxSetUndoBuffer8(&kp->editbox, searchFor);
	keypadEditboxCaretMoveEnd(&kp->editbox);
	//keypadEditboxSetUserData(&kp->editbox, 0); // only sets the hex value displayed
	
	pageSet(search->com->vp, /*PAGE_RENDER_CONCURRENT|*/PAGE_VKEYBOARD);
	
	my_free(searchFor);
}

static inline int64_t search_box_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT || msg == CC_MSG_HOVER) return 1;
	
	TLABEL *label = (TLABEL*)object;
	//printf("search_box_cb: %i %I64d %I64d %p\n", msg, data1, data2, dataPtr);

	TVLCPLAYER *vp = label->cc->vp;
	
	if (msg == LABEL_MSG_TEXT_SELECTED_PRESS || msg == LABEL_MSG_BASE_SELECTED_PRESS){
		searchOpenSearchKeypad(ccGetUserData(label), label->id);

	}else if (msg == KP_MSG_PAD_ENTER){		// search box callback
		//printf("search KP_MSG_PAD_ENTER: %I64d %X '%s'\n", data1, (int)data2, (char*)dataPtr);
		
		TSEARCH *search = ccGetUserData(label);
		TPANE *pane = search->pane;

		if (search->flags.appendMode == SEARCH_APPEND_NEW)
			paneRemoveAll(pane);
		paneScrollReset(pane);
		ccEnable(pane);

		searchSetSearchString(search, dataPtr);
		searchCtrlStop(search);
		searchCtrlStart(search, vp);
		
	}else if (msg == KP_MSG_PAD_OPENED){
		//printf("search KP_MSG_PAD_OPENED: %I64d %X %p\n", data1, (int)data2, dataPtr);
		ccInputDisable(label);
		
	}else if (msg == KP_MSG_PAD_RENDER){
		//printf("search KP_MSG_PAD_RENDER: %I64d %X %p\n", data1, (int)data2, dataPtr);

		TFRAME *frame = dataPtr;
		//lBlurImage(frame, lBLUR_STACKFAST, 5);
		lDrawRectangleFilled(frame, 0, 0, frame->width-1, frame->height-1, (170<<24)|COL_BLACK);

	}else if (msg == KP_MSG_PAD_CLOSED){
		//printf("search KP_MSG_PAD_CLOSED: %I64d %X %p\n", data1, (int)data2, dataPtr);
		
		TKEYBOARD *vkey = pageGetPtr(vp, PAGE_VKEYBOARD);

		keypadListenerRemoveAll(vkey->kp);
		ccInputEnable(label);
						
		page2Set(vp->pages, PAGE_VKEYBOARD, 0);		// remove concurrent
	  	if (page2RenderGetState(vp->pages, PAGE_SEARCH))
	  		page2RenderDisable(vp->pages, PAGE_VKEYBOARD);
	  	else
	  		page2SetPrevious(vkey);

	}else if (msg == LABEL_MSG_BASE_SELECTED_RELEASE){
		TSEARCH *search = ccGetUserData(label);
		ccDisable(search->pane->input.drag.label);
	}

	return 1;
}

static inline void searchCreateMenuTrackPlay (TSEARCH *search)
{
	if (search->menu.type != SEARCH_MENU_TRACKPLAY){
		search->menu.type = SEARCH_MENU_TRACKPLAY;
		
		searchContextClear(search);
		searchContextAddItem(search, "Play           ", search->icons.play, SEARCH_MENU_ITEM_PLAY);
		searchContextAddItem(search, "Open Location"  , search->icons.open, SEARCH_MENU_ITEM_OPENLOCATION);
		searchContextAddItem(search, "Info           ", search->icons.info, SEARCH_MENU_ITEM_INFO);
		searchContextAddItem(search, "Edit...        ", 0, SEARCH_MENU_ITEM_EDIT);
		searchContextAddItem(search, "Meta...        ", 0, SEARCH_MENU_ITEM_COPY);
		searchContextAddItem(search, "Format...      ", 0, SEARCH_MENU_ITEM_FORMAT);
		searchContextAddItem(search, "Search...      ", 0, SEARCH_MENU_ITEM_SEARCH);
	}
}

static inline void searchCreateMenuTrackStop (TSEARCH *search)
{
	if (search->menu.type != SEARCH_MENU_TRACKSTOP){
		search->menu.type = SEARCH_MENU_TRACKSTOP;
		
		searchContextClear(search);
		searchContextAddItem(search, "Stop           ", search->icons.stop, SEARCH_MENU_ITEM_STOP);
		searchContextAddItem(search, "Open Location", search->icons.open, SEARCH_MENU_ITEM_OPENLOCATION);
		searchContextAddItem(search, "Info           ", search->icons.info, SEARCH_MENU_ITEM_INFO);
		searchContextAddItem(search, "Edit...        ", 0, SEARCH_MENU_ITEM_EDIT);
		searchContextAddItem(search, "Meta...        ", 0, SEARCH_MENU_ITEM_COPY);
		searchContextAddItem(search, "Format...      ", 0, SEARCH_MENU_ITEM_FORMAT);
		searchContextAddItem(search, "Search...      ", 0, SEARCH_MENU_ITEM_SEARCH);
	}
}

static inline void searchCreateMenuArtwork (TSEARCH *search)
{
	if (search->menu.type != SEARCH_MENU_ARTWORK){
		search->menu.type = SEARCH_MENU_ARTWORK;
		searchContextClear(search);
		
		searchContextAddImage(search, search->selected.imgId, SEARCH_MENU_ITEM_ARTWORK_CLOSE);
	}
}

static inline void searchCreateMenuMeta (TSEARCH *search)
{
	if (search->menu.type != SEARCH_MENU_META){
		search->menu.type = SEARCH_MENU_META;
		searchContextClear(search);
		
		searchContextAddItem(search, "Copy to clipboard: ", 0, SEARCH_MENU_ITEM_NOACTION);
		searchContextAddItem(search, ".. Title           ", 0, SEARCH_MENU_ITEM_META_COPYTITLE);
		if (search->selected.recType == PLAYLIST_OBJTYPE_TRACK){
			searchContextAddItem(search, ".. Track path      ", 0, SEARCH_MENU_ITEM_META_COPYTRACKPATH);
			searchContextAddItem(search, ".. Art path        ", 0, SEARCH_MENU_ITEM_META_COPYARTPATH);
			searchContextAddItem(search, ".. All             ", 0, SEARCH_MENU_ITEM_META_COPYALL);
			searchContextAddItem(search, "Set title and save ", 0, SEARCH_MENU_ITEM_EDIT_RENAMESAVE);
			searchContextAddItem(search, "Set artwork path   ", 0, SEARCH_MENU_ITEM_META_SETARTWORK);
			
			PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(search->com->vp->plm, search->selected.uid);
			if (plc){
				int imgId = playlistGetArtIdById(plc, search->selected.track);
				if (imgId){
					search->selected.imgId = imgId;
					searchContextAddItem(search, "View artwork         ", 0, SEARCH_MENU_ITEM_META_VIEWARTWORK);
					searchContextAddItem(search, "Open artwork location", 0, SEARCH_MENU_ITEM_META_OPENARTPATH);
				}
			}
		}else{
			PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(search->com->vp->plm, search->selected.uid);
			if (plc){
				if (playlistGetArtIdById(plc, search->selected.track))
					searchContextAddItem(search, ".. Art path         ", 0, SEARCH_MENU_ITEM_META_COPYARTPATH);
			}
			searchContextAddItem(search, ".. All track paths  ", 0, SEARCH_MENU_ITEM_META_COPYTRACKPATHS);
			searchContextAddItem(search, ".. All track titles ", 0, SEARCH_MENU_ITEM_META_COPYTRACKTITLES);
		}
	}
}

static inline void searchCreateMenuFormat (TSEARCH *search)
{
	//printf("searchCreateMenuFormat\n");
	
	if (search->menu.type != SEARCH_MENU_FORMAT){
		search->menu.type = SEARCH_MENU_FORMAT;
		searchContextClear(search);
		
		int id[SEARCH_FORMAT_TOTAL];
		
		if (!search->search.format.trackNo)
			searchContextAddItem(search, "Hide track No.", 0, SEARCH_MENU_ITEM_FORMAT_TRACKNO);
		else
			searchContextAddItem(search, "Show track No.", 0, SEARCH_MENU_ITEM_FORMAT_TRACKNO);

		if (search->selected.recType == PLAYLIST_OBJTYPE_PLC){
			id[SEARCH_FORMAT_PARENT_PLAYLIST]  = searchContextAddItem(search, "Parent :: Playlist", 0, SEARCH_MENU_ITEM_FORMAT_PARENT_PLAYLIST);
			id[SEARCH_FORMAT_UID_PLAYLIST]     = searchContextAddItem(search, "UID :: Playlist     ", 0, SEARCH_MENU_ITEM_FORMAT_UID_PLAYLIST);
			id[SEARCH_FORMAT_PLAYLIST]         = searchContextAddItem(search, ":: Playlist         ", 0, SEARCH_MENU_ITEM_FORMAT_PLAYLIST);
			
			searchContextItemSetColour(search, id[search->search.format.playlist], 0xFFFFAA, COL_BLUE, 0xFFAA00);
		}else{
			/*if (!search->search.format.trackNo)
				searchContextAddItem(search, "Hide track No.", 0, SEARCH_MENU_ITEM_FORMAT_TRACKNO);
			else
				searchContextAddItem(search, "Show track No.", 0, SEARCH_MENU_ITEM_FORMAT_TRACKNO);
			*/
			id[SEARCH_FORMAT_ARTIST_TITLE]     = searchContextAddItem(search, "Artist :: Title     ", 0, SEARCH_MENU_ITEM_FORMAT_ARTIST_TITLE);
			id[SEARCH_FORMAT_PLAYLIST_TITLE]   = searchContextAddItem(search, "Playlist :: Title   ", 0, SEARCH_MENU_ITEM_FORMAT_PLAYLIST_TITLE);
			id[SEARCH_FORMAT_ALBUM_TITLE]      = searchContextAddItem(search, "Album :: Title      ", 0, SEARCH_MENU_ITEM_FORMAT_ALBUM_TITLE);
			id[SEARCH_FORMAT_UID_TITLE]        = searchContextAddItem(search, "UID :: Title        ", 0, SEARCH_MENU_ITEM_FORMAT_UID_TITLE);
			id[SEARCH_FORMAT_ARTIST_FILENAME]  = searchContextAddItem(search, "Artist :: Filename ", 0, SEARCH_MENU_ITEM_FORMAT_ARTIST_FILENAME);
			id[SEARCH_FORMAT_TITLE]            = searchContextAddItem(search, ":: Title            ", 0, SEARCH_MENU_ITEM_FORMAT_TITLE);
			id[SEARCH_FORMAT_FILENAME]         = searchContextAddItem(search, ":: Filename         ", 0, SEARCH_MENU_ITEM_FORMAT_FILENAME);
			id[SEARCH_FORMAT_PATH]             = searchContextAddItem(search, ":: Path             ", 0, SEARCH_MENU_ITEM_FORMAT_PATH);

			searchContextItemSetColour(search, id[search->search.format.track], 0xFFFFAA, COL_BLUE, 0xFFAA00);
		}
		//searchContextAddItem(search, "Image rows...", 0, SEARCH_MENU_ITEM_IMAGE_ROWS);
	}
}

static inline void searchCreateMenuPlaylist (TSEARCH *search)
{
	//printf("searchCreateMenuPlaylist: %i\n", search->selected.edit.op);
	
	if (search->menu.type != SEARCH_MENU_FOLDER){
		search->menu.type = SEARCH_MENU_FOLDER;
		
		searchContextClear(search);
		searchContextAddItem(search, "Search this    ", search->icons.find, SEARCH_MENU_ITEM_OPENUID);
		
		int uid = playlistManagerGetPlaylistParentByUID(search->com->vp->plm, search->selected.uid);
		if (uid > PLAYLIST_UID_BASE)
			searchContextAddItem(search, "Search Parent   ", search->icons.find, SEARCH_MENU_ITEM_SEARCH_PARENT);
		
		searchContextAddItem(search, "Open playlist  ", search->icons.playlist, SEARCH_MENU_ITEM_OPEN);
		searchContextAddItem(search, "Edit...        ", 0, SEARCH_MENU_ITEM_EDIT);
		searchContextAddItem(search, "Meta...        ", 0, SEARCH_MENU_ITEM_COPY);
		searchContextAddItem(search, "Format...      ", 0, SEARCH_MENU_ITEM_FORMAT);
		searchContextAddItem(search, "Search...      ", 0, SEARCH_MENU_ITEM_SEARCH);
	}
}

static inline void searchCreateMenuSearch (TSEARCH *search)
{
	if (search->menu.type != SEARCH_MENU_SEARCH){
		search->menu.type = SEARCH_MENU_SEARCH;
		searchContextClear(search);

		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(search->com->vp->plm, search->selected.uid);
		if (plc){
			char buffer[64];
			if (search->selected.recType == PLAYLIST_OBJTYPE_PLC)
				search->selected.open.uid = playlistGetPlaylistUIDById(plc, search->selected.track);
			else
				search->selected.open.uid = search->selected.uid;

			__mingw_snprintf(buffer, sizeof(buffer), "Search #%X", search->selected.open.uid);
			searchContextAddItem(search, buffer, search->icons.find, SEARCH_MENU_ITEM_SEARCH_OPENUID);
		}

		int uid = playlistManagerGetPlaylistParentByUID(search->com->vp->plm, search->selected.uid);
		if (uid > PLAYLIST_UID_BASE)
			searchContextAddItem(search, "Search Parent   ", search->icons.find, SEARCH_MENU_ITEM_SEARCH_PARENT);
		
		if (search->selected.uid)
			searchContextAddItem(search, "Find this Title", 0, SEARCH_MENU_ITEM_SEARCH_TITLE);
		
		if (search->selected.recType == PLAYLIST_OBJTYPE_TRACK){
			searchContextAddItem(search, "Find this Artist", 0, SEARCH_MENU_ITEM_SEARCH_ARTIST);
			searchContextAddItem(search, "Find this Album", 0, SEARCH_MENU_ITEM_SEARCH_ALBUM);
		}
		
		if (search->flags.appendMode == SEARCH_APPEND_ADD)
			searchContextAddItem(search, "Results: Append", 0, SEARCH_MENU_ITEM_SEARCH_APPEND);
		else
			searchContextAddItem(search, "Results: New  ", 0, SEARCH_MENU_ITEM_SEARCH_APPEND);

		if (search->search.matchHow == PLAYLIST_SEARCH_CASE_SENSITIVE)
			searchContextAddItem(search, "Case sensitive: On", 0, SEARCH_MENU_ITEM_SEARCH_MATCHHOW);
		else
			searchContextAddItem(search, "Case sensitive: Off", 0, SEARCH_MENU_ITEM_SEARCH_MATCHHOW);

		searchContextAddItem(search, "Clear        ", 0, SEARCH_MENU_ITEM_SEARCH_CLEAR);
		searchContextAddItem(search, "Help         ", 0, SEARCH_MENU_ITEM_SEARCH_HELP);
	}
}

static inline void searchCreateMenuInfo (TSEARCH *search, PLAYLISTCACHE *plc, const int trackId, const int addArt)
{
	search->menu.type = SEARCH_MENU_INFO;

	TPANE *pane = search->context.pane;
		
	char *meta = metaGetMetaAll(pane->cc->vp, plc, trackId, "\n");
	if (!meta) return;

	const int titems = search->context.tItems;
	searchContextClear(search);
	paneSwipeEnable(pane);
	searchContextSetFont(search, LFTW_B24);
	searchContextSetLineHeight(search, 25);
	ccHoverRenderSigDisable(pane->cc);


	int artId = playlistGetArtIdById(plc, trackId);
	if (artId && titems){
		searchContextSetPosition(search, ccGetPositionX(search->context.pane), search->pane->metrics.y);
		double scale = (ccGetHeight(pane)/(double)pane->cc->vp->gui.artMaxHeight) * 0.98;
		clipFloat(scale);
		paneImageAdd(pane, artId, scale, PANE_IMAGE_EAST, -4, 0, SEARCH_MENU_ITEM_CLOSEMENU);
	}

	char *tag = strtok(meta, "\n");
	while (tag && *tag){
		searchContextAddItem(search, tag, 0, SEARCH_MENU_ITEM_CLOSEMENU);
		tag = strtok(NULL, "\n");
	}
	my_free(meta);
	

	
	// ensure art fits box
	if (artId && (titems != search->context.tItems)){
		searchCreateMenuInfo(search, plc, trackId, 1);
		return;
	}

	int x = abs(getFrontBuffer(search->com->vp)->width - ccGetWidth(pane))/2;
	int y = abs(getFrontBuffer(search->com->vp)->height - ccGetHeight(pane))/2;
	searchContextSetPosition(search, x, y);
	pageUpdate(search);
}
	
static inline void searchCreateMenuRows (TSEARCH *search)
{
	if (search->menu.type != SEARCH_MENU_ROWS){
		search->menu.type = SEARCH_MENU_ROWS;
		searchContextClear(search);
		
		int ids[8];
		ids[1] = searchContextAddItem(search, " 1 row  ", 0, SEARCH_MENU_ITEM_IMAGE_ROWS_1);
		ids[2] = searchContextAddItem(search, " 2 rows ", 0, SEARCH_MENU_ITEM_IMAGE_ROWS_2);
		ids[3] = searchContextAddItem(search, " 3 rows ", 0, SEARCH_MENU_ITEM_IMAGE_ROWS_3);
		ids[4] = searchContextAddItem(search, " 4 rows ", 0, SEARCH_MENU_ITEM_IMAGE_ROWS_4);
		ids[5] = searchContextAddItem(search, " 5 rows ", 0, SEARCH_MENU_ITEM_IMAGE_ROWS_5);
		ids[6] = searchContextAddItem(search, " 6 rows ", 0, SEARCH_MENU_ITEM_IMAGE_ROWS_6);
		
		searchContextItemSetColour(search, ids[search->image.rows], 0xFFFFAA, COL_BLUE, 0xFFAA00);
	}
}

static inline void searchCreateMenuImage (TSEARCH *search)
{
	if (search->menu.type != SEARCH_MENU_IMAGE){
		search->menu.type = SEARCH_MENU_IMAGE;
		searchContextClear(search);
		
		//printf("searchHistroyIsAvailable %i\n", searchHistroyIsAvailable(search));
		if (searchHistroyIsAvailable(search))
			searchContextAddItem(search, "Back              ", search->icons.find, SEARCH_MENU_ITEM_OTHER_BACK);
			
		if (!search->image.order)
			searchContextAddItem(search, "Order: Ascending ", 0, SEARCH_MENU_ITEM_IMAGE_ORDER_DESCEND);
		else
			searchContextAddItem(search, "Order: Descending", 0, SEARCH_MENU_ITEM_IMAGE_ORDER_ASCEND);

		searchContextAddItem(search, "Rows...            ", 0, SEARCH_MENU_ITEM_IMAGE_ROWS);
		searchContextAddItem(search, "Search...          ", 0, SEARCH_MENU_ITEM_SEARCH);
		searchContextAddItem(search, "Flush				 ", 0, SEARCH_MENU_ITEM_IMAGE_FLUSH);		
	}
}

static inline void searchCreateMenuOther (TSEARCH *search)
{
	//printf("searchCreateMenuOther\n");

	if (search->menu.type != SEARCH_MENU_OTHER){
		search->menu.type = SEARCH_MENU_OTHER;
		searchContextClear(search);
		
		if (search->search.state)
			searchContextAddItem(search, "Stop search", search->icons.close, SEARCH_MENU_ITEM_OTHER_STOPSEARCH);
		
		if (search->selected.recType == SEARCH_OBJTYPE_STRING){
			searchContextAddItem(search, "Find string", search->icons.find, SEARCH_MENU_ITEM_OTHER_FINDSTRING);
			
			search->selected.open.uid = search->selected.uid;
			if (search->selected.open.uid)
				searchContextAddItem(search, "Find Parent", search->icons.find, SEARCH_MENU_ITEM_SEARCH_OPENUID);
		}

		//printf("searchHistroyIsAvailable %i\n", searchHistroyIsAvailable(search));
		if (searchHistroyIsAvailable(search)){
#if 1
			searchContextAddItem(search, "Back   ", search->icons.find, SEARCH_MENU_ITEM_OTHER_BACK);
#else
			char *str = searchHistroyPreview(search);
			if (str){
				char buffer[strlen(str)+4];
				__mingw_snprintf(buffer, sizeof(buffer), " %s", str);

				searchContextAddItem(search, str, search->icons.find, SEARCH_MENU_ITEM_OTHER_BACK);
				my_free(str);
			}
#endif
		}
	
		searchContextAddItem(search, "Format...    ", 0, SEARCH_MENU_ITEM_FORMAT);
		searchContextAddItem(search, "Search...    ", 0, SEARCH_MENU_ITEM_SEARCH);
#if 0
		if (search->selected.edit.op != EDIT_OP_NONE){
			if (search->selected.edit.src.recType == SEARCH_OBJTYPE_PLC)
				searchContextAddItem(search, "Paste Playlist    ", 0, SEARCH_MENU_ITEM_EDIT_PASTE);
			else if (search->selected.edit.src.recType == SEARCH_OBJTYPE_TRACK)
				searchContextAddItem(search, "Paste Track       ", 0, SEARCH_MENU_ITEM_EDIT_PASTE);
		}
#endif
	}
}

static inline void searchCreateMenuEdit (TSEARCH *search)
{
	//printf("searchCreateMenuEdit: %i\n", search->selected.edit.op);
	
	if (search->menu.type != SEARCH_MENU_EDIT){
		search->menu.type = SEARCH_MENU_EDIT;
		searchContextClear(search);
		
		char buffer[64];
		__mingw_snprintf(buffer, 63, " %X:%X", search->selected.uid, search->selected.track&0xFFFF);
		searchContextAddItem(search, buffer, -search->icons.editMenu, SEARCH_MENU_ITEM_EDIT_NOACTION);
		
		searchContextAddItem(search, "Remove  ", search->icons.close, SEARCH_MENU_ITEM_EDIT_DELETE);
		searchContextAddItem(search, "Cut     ", 0, SEARCH_MENU_ITEM_EDIT_CUT);
		
		if (search->selected.recType == PLAYLIST_OBJTYPE_TRACK){
			searchContextAddItem(search, "Copy    ", 0, SEARCH_MENU_ITEM_EDIT_COPY);
			searchContextAddItem(search, "Rename  ", 0, SEARCH_MENU_ITEM_EDIT_RENAME);
			
			if (search->selected.edit.op != EDIT_OP_NONE)
				searchContextAddItem(search, "Paste here", 0, SEARCH_MENU_ITEM_EDIT_PASTE);
				
		}else if (search->selected.recType == PLAYLIST_OBJTYPE_PLC){
			searchContextAddItem(search, "Rename  ", 0, SEARCH_MENU_ITEM_EDIT_RENAME);
			if (search->selected.edit.op != EDIT_OP_NONE)
				searchContextAddItem(search, "Paste here", 0, SEARCH_MENU_ITEM_EDIT_PASTE);
		}
	}
}

static inline int searchMenuSelectImage (TSEARCH *search, const int item)
{
	//printf("searchMenuSelectImage %X %X\n", search->selected.open.uid, search->selected.uid);
	
	
	switch(item){
	case SEARCH_MENU_ITEM_OTHER_BACK:{
		char *str = searchHistroyGet(search);
		if (str){
			searchFindNewString(search, str);
			my_free(str);
			return 1;
		}else{
			return 0;
		}
	}
	case SEARCH_MENU_ITEM_IMAGE_ROWS:
		searchCreateMenuRows(search);
		searchContextSetPosition(search, ccGetPositionX(search->context.pane), ccGetPositionY(search->context.pane));
		searchContextShow(search);
		return 0;
	
	case SEARCH_MENU_ITEM_IMAGE_ORDER_ASCEND:
		search->image.order = 0;
		break;

	case SEARCH_MENU_ITEM_IMAGE_ORDER_DESCEND:
		search->image.order = 1;
		break;

	case SEARCH_MENU_ITEM_IMAGE_FLUSH:
		//artManagerFlush(search->com->vp->am);
		timerSet(search->com->vp, TIMER_FLUSH, 0);
		return 1;

	case SEARCH_MENU_ITEM_IMAGE_ROWS_1:
		search->image.rows = 1;
		break;

	case SEARCH_MENU_ITEM_IMAGE_ROWS_2:
		search->image.rows = 2;
		break;

	case SEARCH_MENU_ITEM_IMAGE_ROWS_3:
		search->image.rows = 3;
		break;

	case SEARCH_MENU_ITEM_IMAGE_ROWS_4:
		search->image.rows = 4;
		break;

	case SEARCH_MENU_ITEM_IMAGE_ROWS_5:
		search->image.rows = 5;
		break;

	case SEARCH_MENU_ITEM_IMAGE_ROWS_6:
		search->image.rows = 6;
		break;

	case SEARCH_MENU_ITEM_SEARCH:
		searchCreateMenuSearch(search);
		searchContextSetPosition(search, ccGetPositionX(search->context.pane), ccGetPositionY(search->context.pane));
		searchContextShow(search);
		return 0;
	}

	searchClearSearch(search);

	int total;
	if (searchDisplayIsCoverMode(search)){
		if ((total=searchAddArtworkCache(search))){
			searchUpdateHeaderStats(search, 0, 0, 0, total);
			searchPreloadItems(search);
		}
	}else if (searchDisplayIsPlaylistsMode(search)){
		if ((total=searchAddPlaylistCovers(search))){
			searchUpdateHeaderStats(search, total, 0, 0, 0);
			searchPreloadItems(search);
		}
	}else if (searchDisplayIsNoCoverMode(search)){
		if (search->image.rows == 1) search->image.rows = 2;
		if ((total=searchAddPlaylistsWithNoCover(search))){
			searchUpdateHeaderStats(search, total, 0, 0, 0);
			searchPreloadItems(search);
		}
	}
		
	if ((uint64_t)processGetMemUsage(search->com->vp->pid) > (uint64_t)(100*1024*1024))		// flush at 100meg
		timerSet(search->com->vp, TIMER_FLUSH, 30);
	return 1;
}

static inline int searchMenuSelectSearch (TSEARCH *search, const int item)
{
	//printf("searchMenuSelectSearch %X %X\n", search->selected.open.uid, search->selected.uid);
	
	
	switch(item){
	case SEARCH_MENU_ITEM_SEARCH_TITLE:
		if (search->selected.recType == PLAYLIST_OBJTYPE_PLC){
			char *title = playlistManagerGetName(search->com->vp->plm, search->selected.open.uid);
			if (title){
				searchFindNewString(search, title);
				my_free(title);
			}
		}else if (search->selected.recType == PLAYLIST_OBJTYPE_TRACK){
			PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(search->com->vp->plm, search->selected.open.uid);
			if (plc){
				char *title = playlistGetTitleDupById(plc, search->selected.track);
				if (title){
					searchFindNewString(search, title);
					my_free(title);
				}
			}
		}
		break;
	case SEARCH_MENU_ITEM_SEARCH_ARTIST:{
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(search->com->vp->plm, search->selected.open.uid);
		if (plc){
			unsigned int hash = playlistGetHashById(plc, search->selected.track);
			char *tag = tagRetrieveDup(search->com->vp->tagc, hash, MTAG_Artist);
			if (tag){
				searchFindNewString(search, tag);
				my_free(tag);
			}
		}
		break;
	}
	case SEARCH_MENU_ITEM_SEARCH_ALBUM:{
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(search->com->vp->plm, search->selected.open.uid);
		if (plc){
			unsigned int hash = playlistGetHashById(plc, search->selected.track);
			char *tag = tagRetrieveDup(search->com->vp->tagc, hash, MTAG_Album);
			if (tag){
				searchFindNewString(search, tag);
				my_free(tag);
			}
		}
		break;
	}
	case SEARCH_MENU_ITEM_SEARCH_OPENUID:
		searchFindNewUID(search, search->selected.open.uid);
		return 0;

	case SEARCH_MENU_ITEM_SEARCH_HELP:
		searchClearSearch(search);
		searchHistroyAdd(search, SEARCH_CMD_Help);
		searchSetSearchString(search, SEARCH_CMD_Help);
		searchAddHelp(search);
		return 0;
		
	case SEARCH_MENU_ITEM_SEARCH_PARENT:{
		int uid = playlistManagerGetPlaylistParentByUID(search->com->vp->plm, search->selected.uid);
		if (uid > PLAYLIST_UID_BASE)
			searchFindNewUID(search, uid);
		return 0;
	}
	case SEARCH_MENU_ITEM_SEARCH_CLEAR:
		searchClearSearch(search);
		searchSetSearchString(search, SEARCH_DEFAULT_STRING);
		buttonsStateSet(search->btns, SEARCH_BTN_REFRESH, 0);
		buttonsStateSet(search->btns, SEARCH_BTN_STOP, 0);
		return 0;

	case SEARCH_MENU_ITEM_SEARCH_MATCHHOW:
		if (search->search.matchHow == PLAYLIST_SEARCH_CASE_SENSITIVE)
			search->search.matchHow = PLAYLIST_SEARCH_CASE_INSENSITIVE;
		else
			search->search.matchHow = PLAYLIST_SEARCH_CASE_SENSITIVE;

		return 1;
	case SEARCH_MENU_ITEM_SEARCH_APPEND:
		if (search->flags.appendMode == SEARCH_APPEND_ADD)
			search->flags.appendMode = SEARCH_APPEND_NEW;
		else
			search->flags.appendMode = SEARCH_APPEND_ADD;

		return 1;
	}
	return 0;
}

static inline void searchMenuSelectOther (TSEARCH *search, const int item)
{
	switch (item){
#if 0
	case SEARCH_MENU_ITEM_EDIT_PASTE:{
		printf("SEARCH_MENU_ITEM_EDIT_PASTE (%X %X) -> %X %X, op:%i\n", search->selected.edit.src.uid, search->selected.edit.src.trackId, uid, trackId, search->selected.edit.op);
		int success = 0;

		if (search->selected.edit.op == EDIT_OP_CUT){
			PLAYLISTCACHE *plcFrom = playlistManagerGetPlaylistByUID(vp->plm, search->selected.edit.src.uid);
			PLAYLISTCACHE *plcTo = plc;
			if (plcFrom && plcTo)
				success = playlistMoveById(plcFrom, plcTo, search->selected.edit.src.trackId, trackId) > 0;

		}else if (search->selected.edit.op == EDIT_OP_COPY){
			PLAYLISTCACHE *plcFrom = playlistManagerGetPlaylistByUID(vp->plm, search->selected.edit.src.uid);
			PLAYLISTCACHE *plcTo = plc;
			if (plcFrom && plcTo)
				success = playlistCopyTrackById(plcFrom, search->selected.edit.src.trackId, plcTo, trackId) > 0;
		}
		
		search->selected.edit.op = EDIT_OP_NONE;
		searchItemSetColour(search, search->selected.edit.src.itemId, COL_WHITE, COL_BLACK, COL_BLUE_SEA_TINT);

		const char *msg8[2] = {"track copy failed", "track copied successfully"};
		dbprintf(vp, "%X:%X %s", search->selected.edit.src.uid, search->selected.edit.src.trackId&0xFFFF, msg8[success]);
		break;
	}
#endif
	case SEARCH_MENU_ITEM_OTHER_BACK:{
		char *str = searchHistroyGet(search);
		if (str){
			searchFindNewString(search, str);
			my_free(str);
		}
		break;
	}
	case SEARCH_MENU_ITEM_OTHER_FINDSTRING:{
		char *str = labelStringGet(search->pane->base, search->selected.itemId);
		if (str){
			searchFindNewString(search, str);
			my_free(str);
		}
		break;
	}
	case SEARCH_MENU_ITEM_OTHER_STOPSEARCH:
		searchCtrlStop(search);
		break;
		
	case SEARCH_MENU_ITEM_SEARCH_OPENUID:
		searchFindNewUID(search, search->selected.open.uid);
		break;

	case SEARCH_MENU_ITEM_FORMAT:
		searchCreateMenuFormat(search);
		searchContextSetPosition(search, ccGetPositionX(search->context.pane), ccGetPositionY(search->context.pane));
		searchContextShow(search);
		break;
	
	case SEARCH_MENU_ITEM_SEARCH:
		searchCreateMenuSearch(search);
		searchContextSetPosition(search, ccGetPositionX(search->context.pane), ccGetPositionY(search->context.pane));
		searchContextShow(search);
		break;
	}
}

static inline int searchMenuSelectFormat (TSEARCH *search, const int item)
{
	//printf("searchMenuSelectFormat %i\n", item);
	
	switch (item){
	case SEARCH_MENU_ITEM_IMAGE_ROWS:
		searchCreateMenuRows(search);
		searchContextSetPosition(search, ccGetPositionX(search->context.pane), ccGetPositionY(search->context.pane));
		searchContextShow(search);
		return 0;

	case SEARCH_MENU_ITEM_FORMAT:
		searchCreateMenuFormat(search);
		searchContextSetPosition(search, ccGetPositionX(search->context.pane), ccGetPositionY(search->context.pane));
		searchContextShow(search);
		break;

	case SEARCH_MENU_ITEM_FORMAT_TRACKNO:
		search->search.format.trackNo ^= 0x01;
		break;
		
	case SEARCH_MENU_ITEM_FORMAT_ARTIST_TITLE:
		search->search.format.track = SEARCH_FORMAT_ARTIST_TITLE;
		break;
		
	case SEARCH_MENU_ITEM_FORMAT_PLAYLIST_TITLE:
		search->search.format.track = SEARCH_FORMAT_PLAYLIST_TITLE;
		break;
		
	case SEARCH_MENU_ITEM_FORMAT_ALBUM_TITLE:
		search->search.format.track = SEARCH_FORMAT_ALBUM_TITLE;
		break;
		
	case SEARCH_MENU_ITEM_FORMAT_UID_TITLE:
		search->search.format.track = SEARCH_FORMAT_UID_TITLE;
		break;
		
	case SEARCH_MENU_ITEM_FORMAT_ARTIST_FILENAME:
		search->search.format.track = SEARCH_FORMAT_ARTIST_FILENAME;
		break;
		
	case SEARCH_MENU_ITEM_FORMAT_TITLE:
		search->search.format.track = SEARCH_FORMAT_TITLE;
		break;
		
	case SEARCH_MENU_ITEM_FORMAT_FILENAME:
		search->search.format.track = SEARCH_FORMAT_FILENAME;
		break;
		
	case SEARCH_MENU_ITEM_FORMAT_PATH:
		search->search.format.track = SEARCH_FORMAT_PATH;
		break;
		
	case SEARCH_MENU_ITEM_FORMAT_UID_PLAYLIST:
		search->search.format.playlist = SEARCH_FORMAT_UID_PLAYLIST;
		break;
		
	case SEARCH_MENU_ITEM_FORMAT_PARENT_PLAYLIST:
		search->search.format.playlist = SEARCH_FORMAT_PARENT_PLAYLIST;
		break;
		
	case SEARCH_MENU_ITEM_FORMAT_PLAYLIST:
		search->search.format.playlist = SEARCH_FORMAT_PLAYLIST;
		break;
	}
	return 1;
}

static inline void searchMenuSelection (TSEARCH *search, const int item, const int menuType, const int recType, int uid, const int trackId)
{
	//printf("## searchMenuSelection in %i %i %i %X %X\n", item, menuType, recType, uid, trackId);

	TVLCPLAYER *vp = search->com->vp;
	
	if (menuType == SEARCH_MENU_OTHER){
		searchMenuSelectOther(search, item);
		return;
		
	}else if (menuType == SEARCH_MENU_FORMAT){
		if (searchMenuSelectFormat(search, item)){
			searchMenuInvalidate(search);
			searchCreateMenuFormat(search);
			searchContextShow(search);
		}
		return;
		
	}else if (menuType == SEARCH_MENU_SEARCH){
		if (searchMenuSelectSearch(search, item)){
			searchItemSetHighlight(search, search->selected.itemId);
			searchMenuInvalidate(search);
			searchCreateMenuSearch(search);
			searchContextShow(search);
		}
		return;
	}else if (menuType == SEARCH_MENU_IMAGE || menuType == SEARCH_MENU_ROWS){
		searchMenuSelectImage(search, item);
		return;
	}
	
	
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
	if (!plc) return;
	int trackIdx = playlistIdToIdx(plc, trackId);
	//printf("@@ searchMenuSelection %X:%X %i\n", uid, trackId, trackIdx);
	if (trackIdx < 0) return;	

	
	switch (item){
	case SEARCH_MENU_ITEM_EDIT:
		//printf("SEARCH_MENU_ITEM_EDIT %X %X\n", uid, trackId);
		searchItemSetHighlight(search, search->selected.itemId);
		searchCreateMenuEdit(search);
		searchContextSetPosition(search, ccGetPositionX(search->context.pane), ccGetPositionY(search->context.pane));
		searchContextShow(search);
		break;

	case SEARCH_MENU_ITEM_EDIT_NOACTION:
		searchItemSetColour(search, search->selected.edit.src.itemId, COL_WHITE, COL_BLACK, COL_BLUE_SEA_TINT);	
		search->selected.edit.op = EDIT_OP_NONE;
		break;

	case SEARCH_MENU_ITEM_EDIT_CUT:
		//printf("SEARCH_MENU_ITEM_EDIT_CUT %X %X\n", uid, trackId);
		
		if (search->selected.edit.src.itemId != search->selected.itemId){
			if (/*search->selected.edit.op != EDIT_OP_CUT &&*/ search->selected.edit.op != EDIT_OP_NONE)
				searchItemSetColour(search, search->selected.edit.src.itemId, COL_WHITE, COL_BLACK, COL_BLUE_SEA_TINT);	
		}
		
		search->selected.edit.op = EDIT_OP_CUT;
		search->selected.edit.src.recType = search->selected.recType;
		search->selected.edit.src.uid = uid;
		search->selected.edit.src.trackId = trackId;
		search->selected.edit.src.itemId = search->selected.itemId;
		searchItemSetColour(search, search->selected.edit.src.itemId, COL_AQUA, COL_BLACK, COL_BLACK);
		break;

	case SEARCH_MENU_ITEM_EDIT_COPY:
		//printf("SEARCH_MENU_ITEM_EDIT_COPY %X %X\n", uid, trackId);
		if (search->selected.edit.src.itemId != search->selected.itemId){
			if (/*search->selected.edit.op != EDIT_OP_COPY &&*/ search->selected.edit.op != EDIT_OP_NONE)
				searchItemSetColour(search, search->selected.edit.src.itemId, COL_WHITE, COL_BLACK, COL_BLUE_SEA_TINT);	
		}

		search->selected.edit.op = EDIT_OP_COPY;
		search->selected.edit.src.recType = search->selected.recType;
		search->selected.edit.src.uid = uid;
		search->selected.edit.src.trackId = trackId;
		search->selected.edit.src.itemId = search->selected.itemId;
		searchItemSetColour(search, search->selected.edit.src.itemId, 0xf0bDC5, COL_BLACK, COL_BLACK);
		break;

	case SEARCH_MENU_ITEM_EDIT_PASTE:{
		//printf("SEARCH_MENU_ITEM_EDIT_PASTE (%X %X) -> %X %X, op:%i\n", search->selected.edit.src.uid, search->selected.edit.src.trackId, uid, trackId, search->selected.edit.op);
		int success = 0;

		if (search->selected.edit.src.uid != uid || search->selected.edit.src.trackId != trackId){
			if (search->selected.edit.op == EDIT_OP_CUT){
				PLAYLISTCACHE *plcFrom = playlistManagerGetPlaylistByUID(vp->plm, search->selected.edit.src.uid);
				PLAYLISTCACHE *plcTo = plc;
				if (plcFrom && plcTo){
					success = playlistMoveById(plcFrom, plcTo, search->selected.edit.src.trackId, trackId) > 0;
					if (success && search->selected.edit.src.recType == SEARCH_OBJTYPE_PLC){
						uid = playlistGetPlaylistUIDById(plcTo, search->selected.edit.src.trackId);
						if (uid){
							plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
							if (plc) plc->parent = plcTo;
						}
					}
				}
			}else if (search->selected.edit.op == EDIT_OP_COPY){
				PLAYLISTCACHE *plcFrom = playlistManagerGetPlaylistByUID(vp->plm, search->selected.edit.src.uid);
				PLAYLISTCACHE *plcTo = plc;
				if (plcFrom && plcTo)
					success = playlistCopyTrackById(plcFrom, search->selected.edit.src.trackId, plcTo, trackId) > 0;
			}
		}else{
			success = 2;
		}
		
		search->selected.edit.op = EDIT_OP_NONE;
		searchItemSetColour(search, search->selected.edit.src.itemId, COL_WHITE, COL_BLACK, COL_BLUE_SEA_TINT);

		const char *msg8[4] = {"Copy failed", "Copy successfull", "Source and distination must be different", "Track copy failed"};
		dbprintf(vp, "%X:%X %s", search->selected.edit.src.uid, search->selected.edit.src.trackId&0xFFFF, msg8[success&0x03]);
		break;
	}
	
	case SEARCH_MENU_ITEM_SEARCH:
		searchItemSetHighlight(search, search->selected.itemId);
		searchCreateMenuSearch(search);
		searchContextSetPosition(search, ccGetPositionX(search->context.pane), ccGetPositionY(search->context.pane));
		searchContextShow(search);
		break;
		
	case SEARCH_MENU_ITEM_SEARCH_PARENT:{
		int uid = playlistManagerGetPlaylistParentByUID(search->com->vp->plm, search->selected.uid);
		if (uid > PLAYLIST_UID_BASE)
			searchFindNewUID(search, uid);
		break;
	}
	case SEARCH_MENU_ITEM_OPENUID:{
		int uid = playlistGetPlaylistUIDById(plc, search->selected.track);
		if (uid){
			searchInitiateMetaRetrieval(search, uid, search->flags.metaSearchDepth);
			searchFindNewUID(search, uid);
		}
		break;
	}
	case SEARCH_MENU_ITEM_OPENLOCATION:{
#if 1
		char *path = playlistGetPathDup(plc, trackIdx);
		if (path){
			strchrreplace(path, '/', '\\');
			fbOpenExplorerLocationA(path);
        	my_free(path);
        }
#else
		char *path = playlistGetPathDup(plc, trackIdx);
		if (path){
			strchrreplace(path, '/', '\\');
			char *dir = getDirectory(path);
			if (dir){
				char cmdLine[MAX_PATH_UTF8+1];
				__mingw_snprintf(cmdLine, MAX_PATH_UTF8, "explorer %s", dir);
				processCreate(cmdLine);
				my_free(dir);
			}
			my_free(path);
		}
#endif
		break;
	}
	case SEARCH_MENU_ITEM_INFO:{
		search->context.tItems = 0;
		searchPlaylistMetaGetMeta(search, plc, trackIdx, trackIdx);
		searchCreateMenuInfo(search, plc, trackId, 0);
		
		TMETACOMPLETIONCB mccb = {0};
		mccb.cb = searchMetaCb;
		mccb.dataInt1 = uid;
		mccb.dataInt2 = trackId;
		mccb.dataPtr1 = (intPtr)trackIdx;
		mccb.dataPtr2 = search;
		initiateAlbumArtRetrievalEx(vp, plc, trackIdx, trackIdx, 2, &mccb);

		searchContextShow(search);
		searchPlaylistMetaGetMeta(search, plc, 0, playlistGetTotal(plc)-1);
		break;
	}
	case SEARCH_MENU_ITEM_PLAY:
		 startPlaylistTrackUID(vp, uid, trackIdx);
		break;

	case SEARCH_MENU_ITEM_STOP:
		timerSet(vp, TIMER_STOP, 0);
		break;

	case SEARCH_MENU_ITEM_OPEN:{
		pageSet(vp, PAGE_PLY_PANE);

		uid = playlistGetPlaylistUID(plc, trackIdx);
		setDisplayPlaylistByUID(vp, uid);
		plypaneSetPlaylist(vp, uid);
		break;
	}
	case SEARCH_MENU_ITEM_EDIT_DELETE:{
		if (recType == PLAYLIST_OBJTYPE_PLC){
			int uidChild = playlistGetPlaylistUID(plc, trackIdx);

			/*char *name = playlistManagerGetName(vp->plm, uidChild);
			if (name){
				printf("delete: '%s'\n", name);
				my_free(name);
			}*/
			
			playlistManagerDeletePlaylistByUID(vp->plm, uidChild, 0);
			playlistDeleteRecord(plc, trackIdx);
			paneRemoveItem(search->pane, search->selected.itemId);
			dbprintf(vp, "%X:%X playlist (%X) removed", uid, trackId&0xFFFF, uidChild);
			
		}else if (recType == PLAYLIST_OBJTYPE_TRACK){
			playlistDeleteRecord(plc, trackIdx);
			paneRemoveItem(search->pane, search->selected.itemId);
			dbprintf(vp, "%X:%X track removed from playlist", uid, trackId&0xFFFF);
		}
		break;
	}
	case SEARCH_MENU_ITEM_EDIT_RENAME:
	case SEARCH_MENU_ITEM_EDIT_RENAMESAVE:{
		if (pageGet(vp) == PAGE_VKEYBOARD) break;
		
		searchItemSetColour(search, search->selected.edit.src.itemId, COL_WHITE, COL_BLACK, COL_BLUE_SEA_TINT);	
		search->selected.edit.op = EDIT_OP_NONE;
		TKEYPAD *kp = keyboardGetKeypad(search);

  		char *title = playlistGetTitleDup(plc, trackIdx);
		if (title){
			TPANE *pane = search->header.pane;	// header control isn't doing much else, so use it for the callback
  			uint64_t id = ((uint64_t)search->selected.itemId<<32) | (uid<<16) | (trackIdx+1);

  			keypadListenerRemoveAll(kp);
  			keypadListenerAdd(kp, pane->id, KP_INPUT_OPENED|KP_INPUT_CLOSED|KP_INPUT_COMPLETE8, id);
  			keypadEditboxSetBuffer8(&kp->editbox, title);
  			keypadEditboxSetUndoBuffer8(&kp->editbox, title);
  			keypadEditboxSetUserData(&kp->editbox, item); // only sets the hex value displayed
  			keypadEditboxCaretMoveEnd(&kp->editbox);
			pageSet(vp, PAGE_VKEYBOARD);
			my_free(title);
		}
		break;
	}
	case SEARCH_MENU_ITEM_FORMAT:
		//searchItemSetHighlight(search, search->selected.itemId);
		searchCreateMenuFormat(search);
		searchContextSetPosition(search, ccGetPositionX(search->context.pane), ccGetPositionY(search->context.pane));
		searchContextShow(search);
		break;

	case SEARCH_MENU_ITEM_COPY:
		searchPlaylistMetaGetMeta(search, plc, trackIdx, trackIdx);
		searchItemSetHighlight(search, search->selected.itemId);
		searchCreateMenuMeta(search);
		searchContextSetPosition(search, ccGetPositionX(search->context.pane), ccGetPositionY(search->context.pane));
		searchContextShow(search);
		break;

	case SEARCH_MENU_ITEM_CLOSEMENU:
		searchContextHide(search);
		break;

	case SEARCH_MENU_ITEM_NOACTION:
		search->selected.edit.op = EDIT_OP_NONE;
		break;

	case SEARCH_MENU_ITEM_META_COPYTITLE:{
		char *title = playlistGetTitleDup(plc, trackIdx);
		if (title){
			keypadClipBoardSet8(NULL, vp->gui.hMsgWin, title);
			dbprintf(vp, "%X:%X track title copied to clipboard", uid, trackId&0xFFFF);
			my_free(title);
			return;
		}
		dbprintf(vp, "%X:%X does not contain a title", uid, trackId&0xFFFF);
		break;
	  }
	case SEARCH_MENU_ITEM_META_COPYTRACKPATH:{
		char *path = playlistGetPathDup(plc, trackIdx);
		if (path){
			keypadClipBoardSet8(NULL, vp->gui.hMsgWin, path);
			dbprintf(vp, "%X:%X track path copied to clipboard", uid, trackId&0xFFFF);
			my_free(path);
			return;
		}
		dbprintf(vp, "%X:%X does not contain a path", uid, trackId&0xFFFF);
		break;
	  }
	case SEARCH_MENU_ITEM_META_COPYARTPATH:{
		int imgId = playlistGetArtId(plc, trackIdx);
		if (imgId){
			wchar_t *path = artManagerImageGetPath(vp->am, imgId);
			if (path){
				keypadClipBoardSet(NULL, vp->gui.hMsgWin, path);
				dbprintf(vp, "%X:%X cover art location copied to clipboard", uid, trackId&0xFFFF);
				my_free(path);
				return;
			}
		}
		dbprintf(vp, "%X:%X does not contain cover art", uid, trackId&0xFFFF);
		break;
	  }
	case SEARCH_MENU_ITEM_ARTWORK_CLOSE:
		// take no action
		break;

	case SEARCH_MENU_ITEM_META_VIEWARTWORK:
		searchCreateMenuArtwork(search);
		searchContextSetPosition(search, ccGetPositionX(search->context.pane), ccGetPositionY(search->context.pane));
		searchContextShow(search);
		break;

	case SEARCH_MENU_ITEM_META_OPENARTPATH:
		if (search->selected.imgId){
			char *path = artManagerImageGetPath8(vp->am, search->selected.imgId);
			if (path){
#if 1
				strchrreplace(path, '/', '\\');
				//printf("openArt Path #%s#\n", path);
				fbOpenExplorerLocationA(path);
#else
				strchrreplace(path, '/', '\\');
				char *dir = getDirectory(path);
				if (dir){
					//printf("openArt Path #%s#\n", path);
					//printf("openArt Dir #%s#\n", dir);
					char cmdLine[MAX_PATH_UTF8+1];
					__mingw_snprintf(cmdLine, MAX_PATH_UTF8, "explorer %s", dir);
					processCreate(cmdLine);
					my_free(dir);
				}
#endif
				my_free(path);
			}
		}
		break;
	case SEARCH_MENU_ITEM_META_SETARTWORK:{
		if (pageIsDisplayed(vp, PAGE_VKEYBOARD)) break;
 		TKEYPAD *kp = keyboardGetKeypad(search);

		int artId = playlistGetArtIdById(plc, trackId);
		if (artId){
			wchar_t *path = artManagerImageGetPath(vp->am, artId);
			if (path){
				keypadEditboxSetBufferW(&kp->editbox, path);
  				keypadEditboxSetUndoBufferW(&kp->editbox, path);
				my_free(path);
			}
		}else{
			keypadEditboxSetBufferW(&kp->editbox, L"");
  			keypadEditboxSetUndoBufferW(&kp->editbox, L"");
		}
		
		uint64_t id = (uid<<16) | (trackIdx+1);
		keypadListenerRemoveAll(kp);
  		keypadListenerAdd(kp, search->pane->id, KP_INPUT_OPENED|KP_INPUT_CLOSED|KP_INPUT_COMPLETEW, id);
  		keypadEditboxCaretMoveEnd(&kp->editbox);
		pageSet(vp, PAGE_VKEYBOARD);
		break;
	}
	case SEARCH_MENU_ITEM_META_COPYALL:{
		char *meta = metaGetMetaAll(vp, plc, trackId, "\r\n");
		if (meta){
			keypadClipBoardSet8(NULL, vp->gui.hMsgWin, meta);
			my_free(meta);
		}
		break;
	}
	case SEARCH_MENU_ITEM_META_COPYTRACKTITLES:{
		plc = playlistGetPlaylist(plc, trackIdx);
		if (!plc) break;
		const int total = playlistGetTotal(plc);
		if (!total) break;
		
		const int len = MAX_PATH_UTF8 * total;
		char paths[len+1];
		paths[0] = 0;
		char *buffer = paths;
		int count = 0;

		for (int i = 0; i < total; i++){
			char *title = playlistGetTitleDup(plc, i);
			if (title){
				strncat(buffer, title, len);
				strncat(buffer, "\r\n", len);
				my_free(title);
				count++;
			}
		}
		if (count){
			keypadClipBoardSet8(NULL, vp->gui.hMsgWin, paths);
			dbprintf(vp, "%i titles added to clipboard", count);
			break;
		}
		if (!count)
			dbprintf(vp, "Not a playlist or playlist empty", count);		

		break;
	}
	case SEARCH_MENU_ITEM_META_COPYTRACKPATHS:{
		plc = playlistGetPlaylist(plc, trackIdx);
		if (!plc) break;
		const int total = playlistGetTotal(plc);
		if (!total) break;

		const int len = MAX_PATH_UTF8 * total;
		char paths[len+1];
		paths[0] = 0;
		char *buffer = paths;
		int count = 0;
		
		for (int i = 0; i < total; i++){
			char *path = playlistGetPathDup(plc, i);
			if (path){
				strncat(buffer, path, len);
				strncat(buffer, "\r\n", len);
				my_free(path);
				count++;
			}
		}
		if (count){
			keypadClipBoardSet8(NULL, vp->gui.hMsgWin, paths);
			dbprintf(vp, "%i tracks added to clipboard", count);
			break;
		}

		if (!count)
			dbprintf(vp, "Not a playlist or playlist empty", count);
		break;
	  }
	};
}

static inline int64_t search_pane_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER|| msg == CC_MSG_INPUT || msg == CC_MSG_HOVER) return 1;
	
	TPANE *pane = (TPANE*)object;
	//printf("search_pane_cb in %i %I64d %I64X %p\n", msg, data1, data2, dataPtr);
	

	/*if (msg == PANE_MSG_VALIDATED){
		

	}else */if (msg == PANE_MSG_BASE_SELECTED){
		TSEARCH *search = ccGetUserData(pane);
		TVLCPLAYER *vp = pane->cc->vp;
		
		//int x = (data1>>16)&0xFFFF;
		//int y = data1&0xFFFF;
		//printf("base Selected %i,%i %I64X\n", x, y, data2);
		
		search->selected.itemId = 0;
		search->selected.recType = 0;
		search->selected.uid = 0;
		search->selected.track = 0;
		
		int x, y;
		searchMenuInvalidate(search);
		if (searchDisplayIsCoverMode(search) || searchDisplayIsPlaylistsMode(search) || searchDisplayIsNoCoverMode(search))
			searchCreateMenuImage(search);
		else
			searchCreateMenuOther(search);
		inputGetCursorPosition(vp, &x, &y);
		searchContextSetPosition(search, x+1, y);
		searchContextShow(search);
		
	}else if (msg == PANE_MSG_TEXT_SELECTED || msg == PANE_MSG_IMAGE_SELECTED){
		TSEARCH *search = ccGetUserData(pane);
		if (search->search.state /*|| !search->search.count */|| !data2) return 1;
		TVLCPLAYER *vp = pane->cc->vp;
					
		search->selected.itemId = data1 + (msg == PANE_MSG_IMAGE_SELECTED);
		search->selected.recType = (int64_t)(data2>>48)&0x07;
		search->selected.uid = (data2>>32)&0xFFFF;
		search->selected.track = (data2&0xFFFFFFFF);
		if (msg == PANE_MSG_IMAGE_SELECTED)
			search->selected.objType = SEARCH_PANEOBJ_IMAGE;
		else
			search->selected.objType = SEARCH_PANEOBJ_TEXT;
		
		//printf("data1 %I64d %I64d, %i %i\n", data1, data2, search->selected.recType, search->selected.objType);
		//printf("search->selected.track %X %i\n", search->selected.track, search->selected.recType == PLAYLIST_OBJTYPE_TRACK);
		

		if (search->selected.recType == SEARCH_OBJTYPE_PLAYLIST){
			paneScrollGet(pane, &search->history.xoffset, &search->history.yoffset);
			search->selected.imgId = search->selected.track;
			//printf("SEARCH_OBJTYPE_PLAYLIST: uid:%X imgId:%X\n", search->selected.uid, search->selected.imgId);

			searchFindNewUID(search, search->selected.uid);
			return 1;
			
		}else if (search->selected.recType == SEARCH_OBJTYPE_IMAGE){
			paneScrollGet(pane, &search->history.xoffset, &search->history.yoffset);
			search->selected.imgId = search->selected.track;
			//printf("search->selected.imgId %X\n", search->selected.imgId);
			searchFindNewString(search, "#$");
			return 1;
			
		}else if (search->selected.recType == SEARCH_OBJTYPE_STRING){
			const unsigned int hash = search->selected.track;
			search->selected.uid = playlistManagerGetPlaylistByTrackHash(pane->cc->vp->plm, hash);
			//printf("SEARCH_OBJTYPE_STRING, uid:%X, hash:%X\n", search->selected.uid, hash);
			/*
			char *tag = tagRetrieveDup(search->com->vp->tagc, hash, MTAG_PATH);
			if (tag){
				printf("  '%s'\n", tag);
				my_free(tag);
			}*/
			
			searchItemSetHighlight(search, search->selected.itemId);
			searchMenuInvalidate(search);
			searchCreateMenuOther(search);
			
		}else if (search->selected.recType == SEARCH_OBJTYPE_NULL){
			return 1;

		}else if (search->selected.recType == SEARCH_OBJTYPE_TRACK){
			searchItemSetHighlight(search, search->selected.itemId);
			int menu = SEARCH_MENU_TRACKPLAY;
			
			if (getPlayState(vp) == 1){
				if (isPlayingItemId(vp, search->selected.uid, search->selected.track))
		  			menu = SEARCH_MENU_TRACKSTOP;
			}
			if (menu == SEARCH_MENU_TRACKPLAY)
				searchCreateMenuTrackPlay(search);
			else
				searchCreateMenuTrackStop(search);

		}else if (search->selected.recType == SEARCH_OBJTYPE_PLC){
			searchItemSetHighlight(search, search->selected.itemId);
			searchMenuInvalidate(search);
			searchCreateMenuPlaylist(search);
			
		}else{
			//searchItemSetHighlight(search, search->selected.itemId);
			searchCreateMenuOther(search);
		}
		
		int x, y;
		inputGetCursorPosition(vp, &x, &y);
		//printf("search_pane_cb %i %X:%i\n", search->selected.recType, search->selected.uid, search->selected.track);
		
		searchCtrlStop(search);
		searchContextSetPosition(search, x+1, y+1);
		searchContextShow(search);
		
	}else if (msg == KP_MSG_PAD_OPENED){	// set artwork
		
	}else if (msg == KP_MSG_PAD_CLOSED){
		TKEYBOARD *vkey = pageGetPtr(pane->cc->vp, PAGE_VKEYBOARD);
		keypadListenerRemoveAll(vkey->kp);
		
	}else if (msg == KP_MSG_PAD_ENTER){		// set artwork callback
		//wprintf(L"pane KP_MSG_PAD_ENTER: %I64d %i '%s'\n", data1, (int)data2, (wchar_t*)dataPtr);
		
		const int kpMsg = data1;
		if (kpMsg == KP_INPUT_COMPLETEW){		// user pressed 'Enter' key
			wchar_t *path = (wchar_t*)dataPtr;
			if (doesFileExistW(path)){
				int artId = artManagerImageAdd(pane->cc->vp->am, path);
				if (artId){
					int uid = (data2>>16)&0xFFFF;
					int trackIdx = (data2&0xFFFF)-1;

					PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(pane->cc->vp->plm, uid);
					if (plc){
						playlistSetArtId(plc, trackIdx, artId, 1);
						dbwprintf(pane->cc->vp, L"Artwork set for %X:%i", uid, trackIdx+1);
					}
				}
			}else{
				dbwprintf(pane->cc->vp, L"File not found: '%s'", path);
			}
		}
	}

	return 1;
}

static inline int64_t search_header_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TPANE *pane = (TPANE*)object;
		
	if (msg == PANE_MSG_TEXT_SELECTED || msg == PANE_MSG_IMAGE_SELECTED){
		TSEARCH *search = ccGetUserData(pane);
		//printf("search_header_cb %i %i %i, %i\n", msg, (int)data1, (int)data2, search->header.ids.audio);
		
		int newImgId = 0;
		int imgId;
		if (msg == PANE_MSG_TEXT_SELECTED)
			imgId = data1-1;
		else	
			imgId = data1;
		
		const int selectedFlag = data2&0xFF;
		if (selectedFlag == SEARCH_HEADER_PLAYLIST){
			if (search->search.flags&SEARCH_INCLUDE_PLAYLIST){
				search->search.flags &= ~SEARCH_INCLUDE_PLAYLIST;
				newImgId = search->icons.playlistX;
			}else{
				search->search.flags |= SEARCH_INCLUDE_PLAYLIST;
				newImgId = search->icons.playlist;
			}
			
		}else if (selectedFlag == SEARCH_HEADER_AUDIO){
			if (search->search.flags&SEARCH_INCLUDE_AUDIO){
				search->search.flags &= ~SEARCH_INCLUDE_AUDIO;
				newImgId = search->icons.audioX;
			}else{
				search->search.flags |= SEARCH_INCLUDE_AUDIO;
				newImgId = search->icons.audio;
			}
				
		}else if (selectedFlag == SEARCH_HEADER_VIDEO){
			if (search->search.flags&SEARCH_INCLUDE_VIDEO){
				search->search.flags &= ~SEARCH_INCLUDE_VIDEO;
				newImgId = search->icons.videoX;
			}else{
				search->search.flags |= SEARCH_INCLUDE_VIDEO;
				newImgId = search->icons.video;
			}
		}else if (selectedFlag == SEARCH_HEADER_OTHER){
			if (search->search.flags&SEARCH_INCLUDE_OTHER){
				search->search.flags &= ~SEARCH_INCLUDE_OTHER;
				newImgId = search->icons.otherX;
			}else{
				search->search.flags |= SEARCH_INCLUDE_OTHER;
				newImgId = search->icons.other;
			}
		}

		paneImageReplace(pane, imgId, newImgId);
		
	}else if (msg == KP_MSG_PAD_OPENED){
		
	}else if (msg == KP_MSG_PAD_CLOSED){
		TKEYBOARD *vkey = pageGetPtr(pane->cc->vp, PAGE_VKEYBOARD);
		keypadListenerRemoveAll(vkey->kp);
		
	}else if (msg == KP_MSG_PAD_ENTER){		// edit menu callback
		const int dataType = data1;
		if (dataType == KP_INPUT_COMPLETE8){
			//printf("header KP_MSG_PAD_ENTER: %I64d %X '%s'\n", data1, (int)data2, (char*)dataPtr);
			
			char *itemStr = dataPtr;
			if (strlen(itemStr) < 1) return 0;
			
			uint64_t id = data2;
			int itemId = (id>>32)&0xFFFFF;
			int uid = (id>>16)&0xFFFF;
			int pos = (id&0xFFFF)-1;
			
			TSEARCH *search = ccGetUserData(pane);
			TVLCPLAYER *vp = pane->cc->vp;
			PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, uid);
			if (!plc) return 1;
			//printf("%x %x\n", uid, pos);
			
			int objType = playlistGetItemType(plc, pos);
			if (objType == PLAYLIST_OBJTYPE_TRACK){
				//printf("a track\n");
				tagAddByHash(vp->tagc, playlistGetHash(plc, pos), MTAG_Title, itemStr, 1);
				playlistSetTitle(plc, pos, itemStr, MAX_PATH_UTF8);
				
				char str[MAX_PATH_UTF8+1];
				char *ply = playlistGetNameDup(plc);
				__mingw_snprintf(str, MAX_PATH_UTF8, "%s :: %s", ply, itemStr);
				paneTextReplace(search->pane, itemId, str);
				my_free(ply);

				TKEYPAD *kp = keyboardGetKeypad(search);
				
				int saved = 0;
				uint64_t saveMeta = keypadEditboxGetUserData(&kp->editbox);
				if (saveMeta == SEARCH_MENU_ITEM_EDIT_RENAMESAVE){
					char *path = playlistGetPathDup(plc, pos);
					if (path){
						libvlc_media_t *m = libvlc_media_new_path(vp->vlc->hLib, path);
						if (m){
							libvlc_media_set_meta(m, MTAG_Title, itemStr);
							saved = libvlc_media_save_meta(m);
							libvlc_media_release(m);
						}
						my_free(path);
					}
				}
				if (!saved)
					dbprintf(vp, "%X:%i title not saved", uid, pos+1);
				else
					dbprintf(vp, "%X:%i title saved", uid, pos+1);
				
			}else if (objType == PLAYLIST_OBJTYPE_PLC){
				int childUid = playlistGetPlaylistUID(plc, pos);
				if (childUid){
					PLAYLISTCACHE *plcChild = playlistManagerGetPlaylistByUID(vp->plm, childUid);
					if (plc){
						playlistSetName(plcChild, itemStr);
					
						char str[MAX_PATH_UTF8+1];
						char *ply = playlistGetNameDup(plc);
						__mingw_snprintf(str, MAX_PATH_UTF8, "%s :: %s", ply, itemStr);
						paneTextReplace(search->pane, itemId, str);
						my_free(ply);
						
						dbprintf(vp, "%X:%i playlist renamed", uid, pos+1);
					}
				}
			}
		}
	}
	return 1;
}

// TIMER_SEARCH_METACB
void timer_metaCb (TVLCPLAYER *vp)
{
	//printf("timer_metaCb\n");
	TSEARCH *search = pageGetPtr(vp, PAGE_SEARCH);
	if (search->menu.type != SEARCH_MENU_INFO) return;
	
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, search->selected.uid);
	if (plc)
		searchCreateMenuInfo(search, plc, search->selected.track, 0);
}

void searchMetaCb (TVLCPLAYER *vp, const int msg, const int uid, const int trackId, void *dataPtr1, void *dataPtr2)
{
	if (SHUTDOWN) return;
	//printf("searchMetaCb: %X %i, %p %p\n", uid, trackId, dataPtr1, dataPtr2);
		
	TSEARCH *search = dataPtr2;
	if (search->menu.type != SEARCH_MENU_INFO) return;

	timerSet(vp, TIMER_SEARCH_METACB, 5);
}

static inline void searchRepeatSearch (TSEARCH *search)
{
	TPANE *pane = search->pane;
	
	paneRemoveAll(pane);
	paneScrollReset(pane);

	search->selected.edit.op = EDIT_OP_NONE;
	if (searchDisplayIsCoverMode(search) || searchDisplayIsPlaylistsMode(search) || searchDisplayIsNoCoverMode(search))
		search->history.xoffset = 0;

	searchCtrlStop(search);
	searchCtrlStart(search, search->com->vp);
}

static inline int searchButtonPress (TSEARCH *search, TCCBUTTON *btn, const int id, const TTOUCHCOORD *pos)
{
	search->btns->t0 = getTickCount();


	switch (id){
	  case SEARCH_BTN_REFRESH:
	  	searchRepeatSearch(search);
		pageUpdate(search);
		break;
	  case SEARCH_BTN_STOP:
	  	searchCtrlStop(search);
	  	pageUpdate(search);
	  	break;

	  case SEARCH_BTN_LAYOUT:
	  	if (buttonFaceActiveGet(btn) == BUTTON_PRI)
	  		searchSetDisplayMode(search, SEARCH_LAYOUT_VERTICAL);
	  	else
	  		searchSetDisplayMode(search, SEARCH_LAYOUT_HORIZONTAL);
	  	pageUpdate(search);
	  	break;

	  case SEARCH_BTN_BACK:
	  	//searchCtrlStop(search);
		page2SetPrevious(search);
		break;
	}
	return 1;
}

static inline int64_t search_btn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;

	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//printf("search_btn_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);

	TCCBUTTON *btn = (TCCBUTTON*)object;
	//const int id = (int)data2;

	if (msg == BUTTON_MSG_SELECTED_RELEASE){
		TSEARCH *search = pageGetPtr(btn->cc->vp, ccGetOwner(btn));
		ccDisable(search->pane->input.drag.label);
		
	}else if (msg == BUTTON_MSG_SELECTED_PRESS){
		return searchButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	}
	return 1;
}  

static inline int page_searchRender (TSEARCH *search, TVLCPLAYER *vp, TFRAME *frame)
{
	outlineTextEnable(frame->hw, 0xDD000000);

	ccRender(search->header.pane, frame);
	buttonsRenderAll(search->btns, frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);	
	ccRender(search->search.box, frame);
	ccRender(search->pane, frame);

	int y = ccGetPositionY(search->search.box)+ccGetHeight(search->search.box);
	lDrawLineDotted(frame, 0, y, frame->width-1, y, 0xFFFFFFFF);

	searchContextRender(search, frame);
	
	if (search->search.state){
		search->flags.inProcessAngle += 7.3;
		TFRAME *src = search->icons.imgProgress;
		TMETRICS rmetrics = {(frame->width-src->width)/2, (frame->height-src->height)/2, src->width-1, src->height-1};		
		rotateAroundZ(src, frame, &rmetrics, search->flags.inProcessAngle);
	}

	return 1;
}

static inline int64_t search_context_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT || msg == CC_MSG_HOVER) return 1;
	
	TPANE *pane = (TPANE*)object;
	//printf("search_context_cb in %p, %i %I64d %I64d %p\n", pane, msg, data1, data2, dataPtr);


	if (msg == CC_MSG_RENDER){
		int colour = 0x00B7EB;
		float mul = 0.7f;
		int alpha = 215;
				
		TSEARCH *search = ccGetUserData(pane);
		if (search->menu.type == SEARCH_MENU_INFO){
			colour = 0x508DC5;
			mul = 0.3;
			alpha = 220;
		}
#if 1
		int r = ((colour&0xFF0000)>>16);
		int g = ((colour&0x00FF00)>>8);
		int b = ( colour&0x0000FF);
		r = (int)(r*mul)&0xFF;
		g = (int)(g*mul)&0xFF;
		b = (int)(b*mul)&0xFF;
		colour = (r<<16)|(g<<8)|b;
		//printf("search_context_cb %.6X\n", colour);
#endif
		drawRectangleFilled(dataPtr, &pane->metrics, (alpha<<24)|colour, 0);
		
	}else if (msg == PANE_MSG_TEXT_SELECTED || msg == PANE_MSG_IMAGE_SELECTED){
		//printf("data1 %i %i\n", (int)data1, (msg == PANE_MSG_IMAGE_SELECTED));

		//TPANE *pane = (TPANE*)object;		
		TSEARCH *search = ccGetUserData(pane);
		searchContextHide(search);
		searchMenuSelection(search, data2, search->menu.type, search->selected.recType, search->selected.uid, search->selected.track);
	}

	return 1;
}

static inline int page_searchStartup (TSEARCH *search, TVLCPLAYER *vp, const int fw, const int fh)
{
	
	searchHistroyCreate(search, 16);

	search->btns = buttonsCreate(vp->cc, PAGE_SEARCH, SEARCH_BTN_TOTAL, search_btn_cb);
	
	TPANE *pane = ccCreateEx(vp->cc, PAGE_SEARCH, CC_PANE, search_pane_cb, NULL, fw, fh-66, search);
	search->pane = pane;
	ccSetPosition(pane, 0, 66);
	paneSetAcceleration(pane, 0.0, PANE_ACCELERATION_Y);
	pane->horiColumnSpace = 8;
	pane->flags.readAhead.enabled = 1;
	pane->flags.readAhead.number = 3 * 4;
	pane->vertLineHeight = SEARCH_PANE_VPITCH;
	//labelRenderFlagsSet(pane->base,LABEL_RENDER_HOVER_OBJ|LABEL_RENDER_IMAGE|LABEL_RENDER_TEXT);
	

	pane = ccCreateEx(vp->cc, PAGE_SEARCH, CC_PANE, search_header_cb, NULL, 480, 34, search);
	search->header.pane = pane;
	//ccSetPosition(pane, (fw-ccGetWidth(pane))/2, 0);
	int x = (fw/3.0) - 50;
	ccSetPosition(pane, x, 0);
	paneSetLayout(pane, PANE_LAYOUT_HORI);
	paneSetAcceleration(pane, 0.0, 2.0);
	paneSwipeDisable(pane);
	//ccInputDisable(pane);
	pane->horiColumnSpace = 20;
	ccEnable(pane);
	

	search->search.box = ccCreateEx(vp->cc, PAGE_SEARCH, CC_LABEL, search_box_cb, NULL, fw*0.62, 63, search);
	ccSetPosition(search->search.box, 0, 0);
	labelRenderFlagsSet(search->search.box, LABEL_RENDER_TEXT);
	search->search.boxStrId = labelTextCreateEx(search->search.box, "Search box", PF_LEFTJUSTIFY, SEARCH_EDITBOX_FONT, 4, 32, 0, 0);
	searchSetSearchString(search, "Search for..");
	ccEnable(search->search.box);


	x = 4;
	int y = 0;
	const int gap = 12;

	TCCBUTTON *btn = buttonsCreateButton(search->btns, L"search/back.png", NULL, SEARCH_BTN_BACK, 1, 0, x, y);
	x = (fw - ccGetWidth(btn)) - x;
	ccSetPosition(btn, x, y);

	btn = buttonsCreateButton(search->btns, L"search/layout_hori.png", L"search/layout_vert.png", SEARCH_BTN_LAYOUT, 1, 0, x, y);
	x -= ccGetWidth(btn) + gap;
	ccSetPosition(btn, x, y);
		
	btn = buttonsCreateButton(search->btns, L"search/refresh.png", NULL, SEARCH_BTN_REFRESH, 0, 1, x, y);
	x -= ccGetWidth(btn) + gap;
	ccSetPosition(btn, x, y);

	btn = buttonsCreateButton(search->btns, L"search/stop64.png", NULL, SEARCH_BTN_STOP, 0, 0, x, y);
	//x -= ccGetWidth(btn) + 16;
	ccSetPosition(btn, x, y);


	pane = ccCreateEx(vp->cc, PAGE_SEARCH, CC_PANE, search_context_cb, NULL, 250, 6, search);
	search->context.pane = pane;
	search->context.id = pane->id;
	paneSetLayout(pane, PANE_LAYOUT_VERT);
	paneSetAcceleration(pane, 0.0, 0.0);
	paneSwipeDisable(pane);
	paneDragDisable(pane);
	paneTextMulityLineDisable(pane);
	paneTextWordwrapDisable(pane);
	labelRenderFlagsSet(pane->base, LABEL_RENDER_BASE/*|LABEL_RENDER_BLUR*/|LABEL_RENDER_HOVER_OBJ|LABEL_RENDER_IMAGE|LABEL_RENDER_TEXT|LABEL_RENDER_BORDER_POST);
	//labelBaseColourSet(pane->base, 80<<24 | COL_BLUE_SEA_TINT);
	labelBaseColourSet(pane->base, 80<<24 | COL_BLACK);
	unsigned int col_post[] = {200<<24 | COL_BLUE_SEA_TINT, 80<<24 | COL_BLUE_SEA_TINT};
	labelBorderProfileSet(pane->base, LABEL_BORDER_SET_POST, col_post, 2);
	searchContextSetLineHeight(search, SEARCH_PANE_VPITCH);
	searchContextSetFont(search, SEARCH_CONTEXT_FONT);


	return 1;
}

static inline int page_searchInitalize (TSEARCH *search, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_SEARCH);
	
	wchar_t bufferw[MAX_PATH+1];
	search->icons.searchInProgress = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/progress.png"));
	search->icons.playlist = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/folder.png"));
	search->icons.audio = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/audio.png"));	
	search->icons.video = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/video.png"));
	search->icons.other = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/other.png"));
	search->icons.playlistX = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/folderx.png"));
	search->icons.audioX = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/audiox.png"));	
	search->icons.videoX = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/videox.png"));
	search->icons.otherX = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/otherx.png"));
	search->icons.play = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/play.png"));
	search->icons.stop = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/stop.png"));
	search->icons.close = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/close.png"));
	search->icons.info = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/info.png"));
	search->icons.open = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/open.png"));
	search->icons.editMenu = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/editheader.png"));
	search->icons.find = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/find.png"));
	search->icons.tag = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/tag.png"));
	search->icons.dvb = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/dvb.png"));
	search->icons.ay = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"search/ay.png"));
	search->icons.noart  = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"shelf/noart.png"));

	search->search.flags = SEARCH_INCLUDE_ALL;
	search->flags.metaSearchDepth = SEARCH_DEFAULT_DEPTH;
	search->flags.appendMode = SEARCH_APPEND_NEW;
	searchSetDisplayMode(search, SEARCH_LAYOUT_VERTICAL);
	return 1;
}

static inline int page_searchShutdown (TSEARCH *search, TVLCPLAYER *vp)
{
	//printf("page_searchShutdown\n");
	searchCtrlStop(search);

	if (search->search.format.buffer)
		my_free(search->search.format.buffer);

	buttonsDeleteAll(search->btns);
	ccDelete(search->context.pane);
	ccDelete(search->header.pane);
	ccDelete(search->pane);
	ccDelete(search->search.box);


	searchHistroyClean(search);
	searchHistroyDestroy(search);
	return 1;
}

static inline int page_searchRenderInit (TSEARCH *search, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	settingsGet(vp, "search.metaDepth", &search->flags.metaSearchDepth);
	settingsGet(vp, "search.ignorecase", &search->search.matchHow);
	
	if (search->search.matchHow == 1)
		search->search.matchHow = PLAYLIST_SEARCH_CASE_INSENSITIVE;
	else
		search->search.matchHow = PLAYLIST_SEARCH_CASE_SENSITIVE;

	search->image.order = 0;		// 0: front to back, 1: back to front
	search->image.rows = 3;
	search->pane->flags.readAhead.number = search->image.rows * 6;
	
	search->search.format.buffer = my_calloc(sizeof(char), MAX_PATH_UTF8+1);
	search->search.format.playlist = SEARCH_FORMAT_PLAYLIST_DEFAULT;
	search->search.format.track = SEARCH_FORMAT_TRACK_DEFAULT;
	search->search.format.trackNo = 0;
	//search->search.matchHow = SEARCH_CASE_DEFAULT;
		
	lSetForegroundColour(vp->ml->hw, 0xFFFFFFFF);
	char buffer[16];
	TPANE *pane = search->header.pane;
	search->header.ids.playlist = paneTextAdd(pane, search->icons.playlist, 1.0, intToStr(0), SEARCH_HEADER_FONT, SEARCH_HEADER_PLAYLIST);
	search->header.ids.audio = paneTextAdd(pane, search->icons.audio, 1.0, intToStr(0), SEARCH_HEADER_FONT, SEARCH_HEADER_AUDIO);
	search->header.ids.video = paneTextAdd(pane, search->icons.video, 1.0, intToStr(0), SEARCH_HEADER_FONT, SEARCH_HEADER_VIDEO);
	search->header.ids.other = paneTextAdd(pane, search->icons.other, 1.0, intToStr(0), SEARCH_HEADER_FONT, SEARCH_HEADER_OTHER);

	//searchUpdateHeaderStats(search, 1000, 1000, 1000, 1000);

	searchAddHelp(search);
	ccEnable(search->pane);
	return 1;
}

static inline int page_searchRenderBegin (TSEARCH *search, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	// return if theres nothing to search
	if (playlistManagerGetTotal(vp->plm) <= 1){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, 0);
		if (!plc || playlistGetCount(plc, PLAYLIST_OBJTYPE_TRACK) < 1){
			page2Set(vp->pages, PAGE_HOME, 0);
			page2RenderDisable(vp->pages, PAGE_SEARCH);
			return 0;
		}
	}
	
	lSetForegroundColour(frame->hw, 0xFFFFFFFF);
	ccHoverRenderSigEnable(vp->cc, 20.0);
	search->icons.imgProgress = artManagerImageAcquire(vp->am, search->icons.searchInProgress);

	return search->icons.imgProgress != NULL;
}

static inline int page_searchRenderEnd (TSEARCH *search, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, void *opaquePtr)
{
	ccHoverRenderSigDisable(vp->cc);
	artManagerImageRelease(vp->am, search->icons.searchInProgress);
	searchCtrlStop(search);
	shadowTextDisable(vp->ml->hw);
	
	if (searchContextIsVisable(search))
		searchContextHide(search);

	//artManagerFlush(vp->am);
	return 1;
}

static inline int page_searchInput (TSEARCH *search, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	//printf("input msg %i %.4f %.4f \n", msg, pos->time, pos->dt);
	
	switch(msg){
	  case PAGE_IN_TOUCH_DOWN:
	  	search->menu.t0 = pos->time;
	  	search->menu.inputId = pos->id;
	  	
	  	if (searchContextIsVisable(search))
			searchContextHide(search);
		break;
	  case PAGE_IN_TOUCH_UP:
	  	if (pos->id == search->menu.inputId && (pos->time - search->menu.t0 > 1000.0)){
	  		if (!search->search.state && !searchContextIsVisable(search)){
		  		searchCreateMenuOther(search); 
	  			int x, y;
				inputGetCursorPosition(vp, &x, &y);
	  			searchContextSetPosition(search, x+1, y+1);
				searchContextShow(search);
			}
	  	}
		pageUpdate(search);
	  	break;
	  case PAGE_IN_WHEEL_FORWARD:
		if (ccGetState(search->context.pane))
	  		paneScroll(search->context.pane, SEARCH_SCROLL_DELTA);
		else if (ccGetState(search->pane))
			paneScroll(search->pane, SEARCH_SCROLL_DELTA);

		break;
	  case PAGE_IN_WHEEL_BACK:
	  	if (ccGetState(search->context.pane))
	  		paneScroll(search->context.pane, -SEARCH_SCROLL_DELTA);
		else if (ccGetState(search->pane))
			paneScroll(search->pane, -SEARCH_SCROLL_DELTA);

		break;
	}
	
	return 1;
}

static inline int page_searchInputKey (TSEARCH *search, TVLCPLAYER *vp, const int key, const int modifier)
{	
	//printf("page_searchInputKey key:%i(%X)  modifier:0x%.2X\n", key, key, modifier);

		
	if (key == VK_F3 || (key == 'F' && modifier&KP_VK_CONTROL)){
		searchOpenSearchKeypad(search, search->search.box->id);

	}else if (key == VK_F5){
		if (search->search.count)
			searchRepeatSearch(search);
		
	}else if (key == 0xDE && modifier&KP_VK_CONTROL){			// hash/sharp/SEARCH_CMD_CHAR
		searchHistroyAdd(search, SEARCH_CMD_CHAR);
		searchSetSearchString(search, SEARCH_CMD_CHAR);
		searchOpenSearchKeypad(search, search->search.box->id);
		
	}else{
		// pass on event
		return 1;
	}

	// don't pass on
	return 0;
}

static inline int page_searchInputChar (TSEARCH *search, TVLCPLAYER *vp, const int key)
{	
	printf("page_searchInputChar key:%i\n", key);
		
	if (key == VK_BACK){		// backspace
		if (!searchMenuSelectImage(search, SEARCH_MENU_ITEM_OTHER_BACK)){
			if (search->search.count)
				searchMenuSelectSearch(search, SEARCH_MENU_ITEM_SEARCH_HELP);
		}
#if 0
	}else if (key == SEARCH_CMD_CHAR[0]){
		searchHistroyAdd(search, SEARCH_CMD_CHAR);
		searchSetSearchString(search, SEARCH_CMD_CHAR);
		searchOpenSearchKeypad(search, search->search.box->id);
#endif
	}else{
		return 1;
	}

	pageUpdate(search);
	return 0;	
}

int page_searchCb (void *pageObj, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TPAGE2COMOBJ *page = (TPAGE2COMOBJ*)pageObj;
	
	//printf("# page_searchCallback: %p %i %I64d %I64d %p %p\n", page, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_searchRender((TSEARCH*)page, page->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_searchInput((TSEARCH*)page, page->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_START){
		return page_searchRenderBegin((TSEARCH*)page, page->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		return page_searchRenderEnd((TSEARCH*)page, page->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_searchRenderInit((TSEARCH*)page, page->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_searchStartup((TSEARCH*)page, page->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_searchInitalize((TSEARCH*)page, page->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_searchShutdown((TSEARCH*)page, page->com->vp);
	
	}else if (msg == PAGE_MSG_CHAR_DOWN){	
		return page_searchInputChar((TSEARCH*)page, page->com->vp, dataInt1);
		
	}else if (msg == PAGE_MSG_KEY_DOWN){
		return page_searchInputKey((TSEARCH*)page, page->com->vp, dataInt1, dataInt2);

	}else if (msg == PAGE_MSG_IMAGE_FLUSH){
		if (dataInt1 == PAGE_SEARCH){
			//printf("PAGE_MSG_IMAGE_FLUSH\n");
			if (searchDisplayIsCoverMode((TSEARCH*)page) || searchDisplayIsPlaylistsMode((TSEARCH*)page))
				searchPreloadItems((TSEARCH*)page);
		}
	}
	
	return 1;
}
