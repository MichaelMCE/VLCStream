            
// libmylcd
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
//  GNU LIBRARY GENERAL PUBLIC LICENSE for details.



#include "common.h"



typedef struct{
	TPLAYLISTRECORD *rec;
	TMETAITEM *mitem;
	int mtag;
	intptr_t number;
}tqsortcb;


// sorts by number in to ascending order.
static inline int playlistSortCB_number_A (const void *a, const void *b)
{
	const tqsortcb *qscb1 = (tqsortcb*)a;
	const tqsortcb *qscb2 = (tqsortcb*)b;

	return qscb1->number - qscb2->number;
}

static inline int playlistSortCB_number_D (const void *a, const void *b)
{
	const tqsortcb *qscb1 = (tqsortcb*)a;
	const tqsortcb *qscb2 = (tqsortcb*)b;

	return qscb2->number - qscb1->number;
}

static inline int playlistSortCB_tag_A (const void *a, const void *b)
{
	const tqsortcb *qscb1 = (tqsortcb*)a;
	const tqsortcb *qscb2 = (tqsortcb*)b;
	char *str1, *str2;
	const int mtag = qscb1->mtag;
	
	if (qscb1->mitem && qscb1->mitem->tag[mtag]){
		str1 = qscb1->mitem->tag[mtag];
		if (!str1) return 1;
	}else{
		return 1;
	}

	if (qscb2->mitem && qscb2->mitem->tag[mtag]){
		str2 = qscb2->mitem->tag[mtag];
		if (!str2) return 1;
	}else{
		return 1;
	}

	return stricmp(str1, str2);
}

static inline int playlistSortCB_tag_D (const void *a, const void *b)
{
	const tqsortcb *qscb1 = (tqsortcb*)a;
	const tqsortcb *qscb2 = (tqsortcb*)b;
	char *str1, *str2;
	const int mtag = qscb1->mtag;
	
	if (qscb1->mitem && qscb1->mitem->tag[mtag]){
		str1 = qscb1->mitem->tag[mtag];
		if (!str1) return 1;
	}else{
		return 1;
	}

	if (qscb2->mitem && qscb2->mitem->tag[mtag]){
		str2 = qscb2->mitem->tag[mtag];
		if (!str2) return 1;
	}else{
		return 1;
	}

	return stricmp(str2, str1);
}

// sorts by filepath in to ascending order.
static inline int playlistSortCB_path_A (const void *a, const void *b)
{
	const tqsortcb *qscb1 = (tqsortcb*)a;
	const tqsortcb *qscb2 = (tqsortcb*)b;
	const char *str1, *str2;
	
//	if (qscb1->rec->item->objType == PLAYLIST_OBJTYPE_TRACK)
		str1 = qscb1->rec->item->obj.track.path;
//	else if (qscb1->rec->item->objType == PLAYLIST_OBJTYPE_PLC)
//		str1 = qscb1->rec->item->obj.plc->name;

//	if (qscb2->rec->item->objType == PLAYLIST_OBJTYPE_TRACK)
		str2 = qscb2->rec->item->obj.track.path;
//	else if (qscb2->rec->item->objType == PLAYLIST_OBJTYPE_PLC)
//		str2 = qscb2->rec->item->obj.plc->name;

	return stricmp(str1, str2);
}

// sorts by filepath in to descending order.
static inline int playlistSortCB_path_D (const void *a, const void *b)
{
	const tqsortcb *qscb1 = (tqsortcb*)a;
	const tqsortcb *qscb2 = (tqsortcb*)b;
	const char *str1, *str2;
	
//	if (qscb1->rec->item->objType == PLAYLIST_OBJTYPE_TRACK)
		str1 = qscb1->rec->item->obj.track.path;
//	else if (qscb1->rec->item->objType == PLAYLIST_OBJTYPE_PLC)
//		str1 = qscb1->rec->item->obj.plc->name;

//	if (qscb2->rec->item->objType == PLAYLIST_OBJTYPE_TRACK)
		str2 = qscb2->rec->item->obj.track.path;
//	else if (qscb2->rec->item->objType == PLAYLIST_OBJTYPE_PLC)
//		str2 = qscb2->rec->item->obj.plc->name;

	return stricmp(str2, str1);
}

// sorts by title in to ascending order.
// if a title is unavailable then use its path
static inline int playlistSortCB_title_A (const void *a, const void *b)
{
	const tqsortcb *qscb1 = (tqsortcb*)a;
	const tqsortcb *qscb2 = (tqsortcb*)b;
	const char *str1, *str2;

	if (qscb1->rec->item->objType == PLAYLIST_OBJTYPE_TRACK){
		str1 = qscb1->rec->item->obj.track.title;
		if (!str1)
			str1 = qscb1->rec->item->obj.track.path;
	}else if (qscb1->rec->item->objType == PLAYLIST_OBJTYPE_PLC){
		str1 = qscb1->rec->item->obj.plc->title;
	}else{
		return 1;
	}
	
	if (qscb2->rec->item->objType == PLAYLIST_OBJTYPE_TRACK){
		str2 = qscb2->rec->item->obj.track.title;
		if (!str2)
			str2 = qscb2->rec->item->obj.track.path;
	}else if (qscb2->rec->item->objType == PLAYLIST_OBJTYPE_PLC){
		str2 = qscb2->rec->item->obj.plc->title;
	}else{
		return 1;
	}
	
	return stricmp(str1, str2);
}

// sorts by title in to descending order.
// if a title is unavailable then use its path
static inline int playlistSortCB_title_D (const void *a, const void *b)
{
	const tqsortcb *qscb1 = (tqsortcb*)a;
	const tqsortcb *qscb2 = (tqsortcb*)b;
	const char *str1, *str2;

	if (qscb1->rec->item->objType == PLAYLIST_OBJTYPE_TRACK){
		str1 = qscb1->rec->item->obj.track.title;
		if (!str1)
			str1 = qscb1->rec->item->obj.track.path;
	}else if (qscb1->rec->item->objType == PLAYLIST_OBJTYPE_PLC){
		str1 = qscb1->rec->item->obj.plc->title;
	}else{
		return 1;
	}
	
	if (qscb2->rec->item->objType == PLAYLIST_OBJTYPE_TRACK){
		str2 = qscb2->rec->item->obj.track.title;
		if (!str2)
			str2 = qscb2->rec->item->obj.track.path;
	}else if (qscb2->rec->item->objType == PLAYLIST_OBJTYPE_PLC){
		str2 = qscb2->rec->item->obj.plc->title;
	}else{
		return 1;
	}

	return stricmp(str2, str1);
}

void playlistSort (PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, const int mtag, const int direction)
{
	if (mtag < 0) return;

	if (playlistLock(plc)){
		TPLAYLISTRECORD *rec = plc->first;
		const int total = plc->total;
		
		if (total > 1 && rec){
			if (tagLock(tagc)){
				tqsortcb *qsortcb = my_calloc(total, sizeof(tqsortcb));
				if (qsortcb){

					// add record handles to an array for processing through qsort()
					for (int i = 0; i < total; i++, rec=rec->next){
						qsortcb[i].rec = rec;
						
						if (rec->item->objType == PLAYLIST_OBJTYPE_PLC){
							playlistSort(rec->item->obj.plc, tagc, mtag, direction);
							qsortcb[i].mitem = NULL;
							qsortcb[i].mtag = -1;
							
						}else if (/*rec->item->objType == PLAYLIST_OBJTYPE_TRACK &&*/ mtag != MTAG_PATH && mtag != MTAG_Title){
							qsortcb[i].mitem = g_tagFindEntryByHash(tagc, rec->item->obj.track.hash);
							qsortcb[i].mtag = mtag;
							
							if (mtag == MTAG_TrackNumber || mtag == MTAG_LENGTH){
								if (qsortcb[i].mitem && qsortcb[i].mitem->tag[mtag] && qsortcb[i].mitem->tag[mtag][0])
									qsortcb[i].number = atoi(qsortcb[i].mitem->tag[mtag]);
								else
									qsortcb[i].number = 0;
							}
						}else{
							qsortcb[i].mitem = NULL;
							qsortcb[i].mtag = -1;
						}
					}

					// do the sort
					if (direction == SORT_ASCENDING){
						if (mtag == MTAG_PATH)
							qsort(qsortcb, total, sizeof(tqsortcb), playlistSortCB_path_A);	
						else if (mtag == MTAG_Title)
							qsort(qsortcb, total, sizeof(tqsortcb), playlistSortCB_title_A);
						else if (mtag == MTAG_TrackNumber || mtag == MTAG_LENGTH)
							qsort(qsortcb, total, sizeof(tqsortcb), playlistSortCB_number_A);
						else
							qsort(qsortcb, total, sizeof(tqsortcb), playlistSortCB_tag_A);

					}else if (direction == SORT_DESCENDING){
						if (mtag == MTAG_PATH)
							qsort(qsortcb, total, sizeof(tqsortcb), playlistSortCB_path_D);
						else if (mtag == MTAG_Title)
							qsort(qsortcb, total, sizeof(tqsortcb), playlistSortCB_title_D);
						else if (mtag == MTAG_TrackNumber || mtag == MTAG_LENGTH)
							qsort(qsortcb, total, sizeof(tqsortcb), playlistSortCB_number_D);
						else
							qsort(qsortcb, total, sizeof(tqsortcb), playlistSortCB_tag_D);
					}

					// sorting completed so relink the records
					plc->first = qsortcb[0].rec;
					for (int i = 0; i < total-1; i++)
						qsortcb[i].rec->next = qsortcb[i+1].rec;
					qsortcb[total-1].rec->next = NULL;

					my_free(qsortcb);
				}
				tagUnlock(tagc);
			}
		}
		playlistUnlock(plc);
	}
}
