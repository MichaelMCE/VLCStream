
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

typedef struct {
	time64_t start;
}TEPGSORT;




static inline void epgSetActiveControl (TEPG *epg, const int view)
{
	switch (view){
	  case EPGBUTTON_GUIDE:
		epg->displayMode = view;
		ccEnable(epg->guide.paneChannels);
		ccEnable(epg->guide.paneContents);
		ccDisable(epg->programme.listbox);

		buttonsStateSet(epg->btns, EPGBUTTON_GENPLY, (epg->vepg.total >= 2));
		break;
		
	  case EPGBUTTON_CHANNELS:
		epg->displayMode = view;
		ccDisable(epg->guide.paneChannels);
		ccDisable(epg->guide.paneContents);
		ccEnable(epg->programme.listbox);

		buttonsStateSet(epg->btns, EPGBUTTON_GENPLY, (epg->vepg.total >= 2));
		break;
		
	  case EPGBUTTON_META:
		epg->displayMode = view;
		ccDisable(epg->guide.paneChannels);
		ccDisable(epg->guide.paneContents);
		ccDisable(epg->programme.listbox);

		buttonsStateSet(epg->btns, EPGBUTTON_GENPLY, (epg->vepg.total >= 2));
		break;
	 };
}


static inline void epgGuideDestroyEvent (TGUIDE_EVENT *event, const int freeStorage)
{
	if (event){
		//printf("epgGuideDestroyEvent %i '%s'\n", event->pid, event->name);
		
		if (event->name) my_free(event->name);
		if (event->descriptionLong) my_free(event->descriptionLong);
		if (event->descriptionShort) my_free(event->descriptionShort);
		if (freeStorage) my_free(event);
	//}else{
		//printf("epgGuideDestroyEvent NULL\n");
	}
}

// one branch per stream/channel. branch id will be the PID (eg; 27348)
static inline int epgGuideCreateBranch (TTREE *tree, const char *name, const int branchId)
{
	TTREEENTRY *entry = treeFind(tree, EPG_GUIDE_ROOTID, branchId, 1);
	if (!entry){
		TGUIDE_EVENT *guide = my_calloc(1, sizeof(TGUIDE_EVENT));
		if (guide){
			guide->pid = branchId;
			guide->name = my_strdup(name);
			
			entry = treeAddItem(tree, EPG_GUIDE_ROOTID, name, branchId, TREE_TYPE_BRANCH);
			if (entry)
				treeEntrySetStorage(entry, guide);
			else
				my_free(guide);
		}
#if 0
	}else{
		TGUIDE_EVENT *guide = treeEntryGetStorage(entry);
		char *oldTitle = guide->name;
		guide->name = my_strdup(name);
		my_free(oldTitle);
#endif
	}
	
	//if (!entry)	
	//	printf("epgGuideCreateBranch failed: %p %i:%i '%s'\n", entry, tree->root->id, branchId, name);
	return (entry != NULL);
}

// add a programme event to the branch, indexed by time (programme ::start time)
static inline int epgGuideAddEvent (TTREE *tree, const int branchId, const int64_t eventId, TGUIDE_EVENT *guide)
{
	guide->pid = branchId;
	
	TTREEENTRY *entry = treeFind(tree, branchId, eventId, 2);
	if (!entry){
		entry = treeAddItem(tree, branchId, guide->name, eventId, TREE_TYPE_LEAF);
		if (entry)
			treeEntrySetStorage(entry, guide);
		else
			epgGuideDestroyEvent(guide, 1);
	}else{
		TGUIDE_EVENT *oldEvent = treeEntryGetStorage(entry);
		guide->ui.paneExpanded = oldEvent->ui.paneExpanded;
		treeEntrySetStorage(entry, guide);
		epgGuideDestroyEvent(oldEvent, 1);
	}
	
	//if (!entry)
	//	printf("epgGuideAddEvent failed: %p %i:%I64d '%s'\n", entry, branchId, eventId, guide->name);
	return (entry != NULL);
}

static inline TGUIDE_EVENT *epgGuideFindEvent (TTREE *tree, const int branchId, const int64_t eventId)
{
	TTREEENTRY *entry = treeFind(tree, branchId, eventId, 2);
	if (entry) return treeEntryGetStorage(entry);
	
	return NULL;
}

static inline int epgGuideGetBranchTotal (TTREE *tree)
{
	return treeCountItems(tree, EPG_GUIDE_ROOTID);
}

static inline int epgGuideGetEventTotal (TTREE *tree, const int branchId)
{
	return treeCountItems(tree, branchId);
}

static inline TGUIDE_EVENT *epgGuideGetEvent (TTREE *tree, const int branchId, const int eventIdx)
{
	TTREEENTRY *entry = treeFind(tree, EPG_GUIDE_ROOTID, branchId, 2);
	if (entry){
		int ct = 0;
		TLISTITEM *item = entry->head;
		
		while(item){
			if (ct++ == eventIdx){
				TTREEENTRY *entry = treeListGetSubEntry(item);
				//printf("@@ '%s'\n", entry->name);
				
				if (entry){
					TGUIDE_EVENT *guide = treeEntryGetStorage(entry);
					//printf(":: '%s'\n", guide->name);
					return guide;
				}else{
					return NULL;
				}
			}
			item = item->next;
		}
	}
	return NULL;
}

// return number of events in selected guide
static inline int epgEventGetTotal (TEPG *epg, const int vepgIdx)
{
	if (vepgIdx < epg->vepg.total && epg->vepg.epg[vepgIdx])
		return epg->vepg.epg[vepgIdx]->i_event;
	
	return 0;
}

static inline TVLCEPGEVENT *epgEventGetEvent (TEPG *epg, const int vepgIdx, const int eventIdx)
{
	TVLCEPGEVENT *event = NULL;
	if (vepgIdx < epg->vepg.total && epg->vepg.epg[vepgIdx]){
		if (eventIdx < epg->vepg.epg[vepgIdx]->i_event && epg->vepg.epg[vepgIdx]->pp_event[eventIdx])
			event = epg->vepg.epg[vepgIdx]->pp_event[eventIdx];
	}
	return event;
}

static inline void epgGuideDestroy (TTREE *tree)
{
	int i = 0;
	TGUIDE_EVENT *guide = NULL;
	
	do{
		int j = 0;
		guide = epgGuideGetEvent(tree, EPG_GUIDE_ROOTID, i++);
		if (guide){
			TGUIDE_EVENT *event = NULL;
			do{
				event = epgGuideGetEvent(tree, guide->pid, j++);
				if (event)
					epgGuideDestroyEvent(event, 0);
			}while(event != NULL);
			
			epgGuideDestroyEvent(guide, 0);
		}
	}while(guide != NULL);
	
	treeFree(tree);
}

static inline void epgGuideSyncDatabase (TEPG *epg)
{
	TTREE *tree = epg->guide.database;

	// add streams/channels first
	for (int i = 0; i < epg->vepg.total; i++)
		epgGuideCreateBranch(tree, epg->vepg.epg[i]->psz_name, epg->vepg.epg[i]->programme);

	const int vtotal = epg->vepg.total;
	for (int vepgIdx = 0; vepgIdx < vtotal; vepgIdx++){
		const int etotal = epgEventGetTotal(epg, vepgIdx);
		if (etotal < 1) break;
		const int programme = epg->vepg.epg[vepgIdx]->programme;
		
		for (int i = 0; i < etotal; i++){
			TVLCEPGEVENT *event = epgEventGetEvent(epg, vepgIdx, i);
			if (!event) break;

			TGUIDE_EVENT *guide = my_calloc(1, sizeof(TGUIDE_EVENT));
			if (guide){
				//printf("%i %i: '%s'\n", vepgIdx, i, event->psz_name/*event->psz_short_description*/);

				guide->start = event->i_start;
				guide->duration = event->i_duration;
				guide->name = my_strdup(event->psz_name);

				if (event->psz_description)
					guide->descriptionLong = my_strdup(event->psz_description);
				if (event->psz_short_description)
					guide->descriptionShort = my_strdup(event->psz_short_description);
			    
			    //printf("%i %i: %i '%s'\n", vepgIdx, i, programme, guide->name);
				epgGuideAddEvent(tree, programme, guide->start, guide);
			}
		}
	}
	
/*	
	printf("\n\n");
	
	int i = 0;
	TGUIDE_EVENT *guide = NULL;
	
	do{
		//printf("%i\n", i);
		guide = epgGuideGetEvent(tree, EPG_GUIDE_ROOTID, i++);
		if (guide){
			printf("##  '%s'\n", guide->name);

			int j = 0;
			TGUIDE_EVENT *event = NULL;
			do{
				event = epgGuideGetEvent(tree, guide->pid, j++);
				if (event){
					printf("\t\t  '%s'\n", event->name);
				}
			}while(event != NULL);
		}
	}while(guide != NULL);
*/
}

static inline char *epgGuideBuildString (const TGUIDE_EVENT *event, const int complete)
{
	char timestr[64];
	char buffer[2][MAX_PATH_UTF8+1];

	time_t eventTime = event->start;
	struct tm *tmTime = localtime(&eventTime);
	strftime(timestr, sizeof(timestr)-1, "%H:%M", tmTime);
	__mingw_snprintf(buffer[0], MAX_PATH_UTF8-1, "%s: %s", timestr, event->name);

	if (!complete){
		return my_strdup(buffer[0]);
		
	}else{
		strftime(timestr, sizeof(timestr)-1, "%A, %d %B", tmTime);

		eventTime = event->duration;
		tmTime = localtime(&eventTime);
		
		if (tmTime->tm_hour == 1 && tmTime->tm_min == 0)
			strcpy(buffer[1], "60m");
		else if (tmTime->tm_hour)
			__mingw_snprintf(buffer[1], MAX_PATH_UTF8-1, "%ih %im", tmTime->tm_hour, tmTime->tm_min);
		else
			__mingw_snprintf(buffer[1], MAX_PATH_UTF8-1, "%im", tmTime->tm_min);

		char *desc;
		if (event->descriptionLong)
			desc = event->descriptionLong;
		else
			desc = event->descriptionShort;

		int blen = strlen(buffer[0]) + strlen(buffer[1]) + strlen(timestr) + strlen(desc) + 16;
		char *text = my_malloc(blen);
		if (text){
			__mingw_snprintf(text, blen-1, "%s\n  %s: %s\n%s\n", buffer[0], buffer[1], timestr, desc);
			return text;
		}
	}
	return NULL;
}

static inline int epgPaneAddGuide (TEPG *epg, TTREE *tree)
{
	
	int64_t pid;
	TGUIDE_EVENT *channel = epgGuideGetEvent(tree, EPG_GUIDE_ROOTID, epg->guide.programmeIdx);
	if (!channel) return 0;
	//else pid = channel->pid;

	int i = 0;

	if (epg->guide.isRoot){
		epg->guide.paneContents->vertLineHeight = 36;
		paneTextMulityLineDisable(epg->guide.paneContents);
		paneTextWordwrapDisable(epg->guide.paneContents);
		
	}else{
		epg->guide.paneContents->vertLineHeight = 0;
		paneTextMulityLineEnable(epg->guide.paneContents);
		paneTextWordwrapEnable(epg->guide.paneContents);
	}
		
	//printf("lineheight %i\n", epg->guide.paneContents->vertLineHeight);
		
	//if (epg->guide.isRoot){	// display a channel guide selection list
		pid = EPG_GUIDE_ROOTID;
		epg->guide.paneChannels->vertLineHeight = 36;
	
	
		TGUIDE_EVENT *guide = epgGuideGetEvent(tree, pid, i);
		while(guide){
			int64_t refId = ((int64_t)1<<63) | (int64_t)((int64_t)(guide->pid&0x7FFFFFFF)<<32);
			refId |= i&0xFFFF;
			
			//char buffer[MAX_PATH_UTF8+1];
			char *buffer;
			channel = epgGuideGetEvent(tree, pid, i);
			if (channel){
				TGUIDE_EVENT *event = epgGuideGetEvent(tree, channel->pid, 0);
				//snprintf(buffer, sizeof(buffer), "%s", event->name);
				buffer = event->name;
			}else{
				//snprintf(buffer, sizeof(buffer), "%s", guide->name);
				buffer = guide->name;
			}
		
			if (*guide->name){
				paneTextAdd(epg->guide.paneChannels, epg->guide.icons.pgmEvent, 1.0, guide->name, LFTW_B28, refId);
				if (epg->guide.isRoot && *buffer)
					paneTextAdd(epg->guide.paneContents, epg->guide.icons.pgmEvent, 1.0, buffer, LFTW_B28, refId);
			}
			guide = epgGuideGetEvent(tree, pid, ++i);
		}
	//}else{			// display  contents of selected channel guide
	
	if (!epg->guide.isRoot){
		channel = epgGuideGetEvent(tree, EPG_GUIDE_ROOTID, epg->guide.programmeIdx);
		pid = channel->pid;
		i = 0;
		
		/*TGUIDE_EVENT **/guide = epgGuideGetEvent(tree, pid, i);
		while(guide){
			char *text = epgGuideBuildString(guide, guide->ui.paneExpanded);
			if (text){
				if (*text){
					uint64_t refId = ((int64_t)0<<63) | (int64_t)((int64_t)(pid&0x7FFFFFFF)<<32) | (guide->start&0xFFFFFFFF);
					int id = paneTextAdd(epg->guide.paneContents, epg->guide.icons.pgmEvent, 1.0, text, LFTW_B28, refId);
					if (id){
						if (epg->vepg.playing.pid == guide->pid && epg->vepg.playing.start == guide->start)
							labelRenderColourSet(epg->guide.paneContents->base, id, 255<<24|COL_GREEN_TINT, 255<<24|COL_WHITE, 127<<24|0x28C672);
					}
				}
				my_free(text);
			}
			guide = epgGuideGetEvent(tree, pid, ++i);
		};
	}
	return i;
}

void epgBuildGuide (TEPG *epg, void *unused)
{
	if (ccLock(epg->guide.paneChannels)){
		if (ccLock(epg->guide.paneContents)){
			lSetCharacterEncoding(epg->com->vp->ml->hw, CMT_UTF8);
			paneRemoveAll(epg->guide.paneChannels);
			paneRemoveAll(epg->guide.paneContents);
		
			if (epg->guide.isRoot){
				//epg->guide.paneChannels->offset.y = epg->guide.yOffset;
				//epg->guide.paneContents->offset.y = epg->guide.yOffset;
			}

			epgPaneAddGuide(epg, epg->guide.database);
			ccUnlock(epg->guide.paneContents);
		}
		ccUnlock(epg->guide.paneChannels);
	}
}

static inline int64_t epgCcObject_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	if (msg == CC_MSG_RENDER) return 1;

 	
 	if (obj->type == CC_LISTBOX){
		TLB *lb = (TLB*)object;
		TVLCPLAYER *vp = (TVLCPLAYER*)obj->cc->vp;
		
		if (lb->id == ccGetId(vp, CCID_LISTBOX_EPG_PROGRAMME)){
			TEPG *epg = pageGetPtr(vp, PAGE_EPG);
			
			switch (msg){
			  case CC_MSG_ENABLED:
				epgFillProgrammeListbox(epg, lb);
				break;
		  	 // case CC_MSG_DISABLED:
			//	break;
		  	 // case LISTBOX_MSG_VALCHANGED:
		  	  	//printf("LISTBOX_MSG_VALCHANGED %i %i\n", data1, data2);
			//	break;
		  	  case LISTBOX_MSG_ITEMSELECTED:
		  	  	//printf("LISTBOX_MSG_ITEMSELECTED %I64d %I64d\n", data1, data2);
		  	  	vlc_setProgram(vp->vlc, data2);
		  	  	epg->vepg.playing.channelIdx = data1;
		  	  	epgFillProgrammeListbox(epg, lb);
				break;
			};			
		}
	}
	return 1;
}

static inline int epgButtonPress (TEPG *epg, TCCBUTTON *btn, const int id, const TTOUCHCOORD *pos)
{
	TVLCPLAYER *vp = epg->com->vp;
	epg->btns->t0 = getTickCount();

	
	switch (id){
	  case EPGBUTTON_CHANNELS: 
	  	epgSetActiveControl(epg, id);
		break;
		
	  case EPGBUTTON_GUIDE:
		epg->guide.isRoot = 1;
		epg->guide.paneChannels->offset.x = epg->guide.yOffset;
		epg->guide.paneContents->offset.y = epg->guide.yOffset;
		epgBuildGuide(epg, NULL);
	  	epgSetActiveControl(epg, id);
		break;

	  case EPGBUTTON_META:
		epgSetActiveControl(epg, id);
		break;

	  case EPGBUTTON_LEFT:
	  	if (--epg->guide.programmeIdx < 0) epg->guide.programmeIdx = 0;
	  	epgBuildGuide(epg, NULL);
		break;

	  case EPGBUTTON_RIGHT:{
	  	int total = epgGuideGetBranchTotal(epg->guide.database);
	  	if (++epg->guide.programmeIdx >= total)
	  		epg->guide.programmeIdx = total-1;
	  	
	  	epgBuildGuide(epg, NULL);
		break;
	  }
	  case EPGBUTTON_GENPLY:
	  	timerSet(vp, TIMER_EPG_GENDVBPLAYLIST, 0);

	  	break;
	  
	  case EPGBUTTON_BACK:
	  	page2SetPrevious(epg);
		//pageSetSec(vp, -1);
	  	break;
	 }
	return 0;
}	

static inline int64_t ccbtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;
		
	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//printf("ccbtn_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);

	TCCBUTTON *btn = (TCCBUTTON*)object;
	//const int id = (int)data2;

	if (msg == BUTTON_MSG_SELECTED_PRESS)
		return epgButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);

	return 1;
}

int epgProgrammeLbGetTotal (TEPG *epg)
{
	return epg->vepg.total;
	//return listboxGetTotalItems(epg->programme.listbox);
}

void epgProgrammeListboxRefresh (TEPG *epg)
{
	if (ccGetState(epg->programme.listbox)){
		ccDisable(epg->programme.listbox);
		ccEnable(epg->programme.listbox);
	}
}

static inline int epgFillProgrammeListboxLocked (TLB *lb, TVLCEPG **epg, int epgTotal, const int playingPID)
{
	if (!epg || epgTotal < 1) return 0;

	const int firstItem = lbGetFocus(lb);
	lbRemoveItems(lb);

	char buffer[MAX_PATH_UTF8+1];
	
	for (int i = 0; i < epgTotal; i++){
		char *name = epg[i]->psz_name;
		
		if (name && *name)
			__mingw_snprintf(buffer, MAX_PATH_UTF8, "%s (%i)", name, epg[i]->programme);
		else
			__mingw_snprintf(buffer, MAX_PATH_UTF8, "- (%i)", epg[i]->programme);

		if (i == playingPID)
			lbAddItemEx(lb, LISTBOX_ITEM_STRING, buffer, NULL, EPG_PROGRAMLISTBOX_FONT, (0xFF<<24)|COL_GREEN_TINT, epg[i]->programme);
		else
			lbAddItemEx(lb, LISTBOX_ITEM_STRING, buffer, NULL, EPG_PROGRAMLISTBOX_FONT, (0xFF<<24)|0xFFFFFF, epg[i]->programme);
	}
	
	int h = (epgTotal * lb->verticalPadding) + 1;
	if (h > getFrontBuffer(lb->cc->vp)->height-(16+64)) h = getFrontBuffer(lb->cc->vp)->height-(16+64);
	ccSetMetrics(lb, -1, -1, -1, h);
	
	lbSetFocus(lb, firstItem);
	return epgTotal;
}

int epgFillProgrammeListbox (TEPG *epg, TLB *lb)
{
	int ret = 0;
	if (ccLock(lb)){
		ret = epgFillProgrammeListboxLocked(lb, epg->vepg.epg, epg->vepg.total, epg->vepg.playing.channelIdx);
		ccUnlock(lb);
	}
	return ret;
}

static int dvbCreatePlaylistAuto (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const char *path, TVLCEPG **epg, int epgTotal)
{
	if (!epg || epgTotal < 2) return 0;

	char bpos[64];
	char buffer[MAX_PATH_UTF8];
	char options[MAX_PATH_UTF8];
	char *poptions = options;
	char *optTag;

	PLAYLISTCACHE *plcQ = getQueuedPlaylist(vp);
	int pos = plcQ->pr->playingItem;
	playlistGetOptions(plcQ, pos, poptions, MAX_PATH_UTF8);

	if (*poptions){
		optTag = strrchr(poptions, '<');
		if (optTag) poptions++;
		optTag = strrchr(poptions, '>');
		if (optTag) *optTag = 0;
	}

	if (!vp->vlc->spu.selected){
		if (*poptions)
			strcat(poptions, OPTSEPARATORSTR);
		strcat(poptions, "no-spu");
	}

	int entriesAdded = 0;
	for (int i = 0; i < epgTotal; i++){
		if (epg[i]->psz_name){
			if (*poptions)
				__mingw_snprintf(buffer, MAX_PATH_UTF8, "%s <%s%cprogram=%i>", path, poptions, OPTSEPARATOR, epg[i]->programme);
			else
				__mingw_snprintf(buffer, MAX_PATH_UTF8, "%s <program=%i>", path, epg[i]->programme);
			//printf("'%s'\n",buffer);

			pos = playlistAdd(plc, buffer);
			if (pos >= 0){
				playlistSetTitle(plc, pos, epg[i]->psz_name, 1);
				tagAdd(vp->tagc, buffer, MTAG_Title, epg[i]->psz_name, 1);
				tagAdd(vp->tagc, buffer, MTAG_PATH, buffer, 1);
				__mingw_snprintf(bpos, sizeof(bpos), "%i", pos+1);
				tagAdd(vp->tagc, buffer, MTAG_POSITION, bpos, 1);
				entriesAdded++;
			}
		}
	}
	return entriesAdded;
}

#if 0
// TIMER_EPG_DISPLAYOSD
void epgDisplayOSD (TVLCPLAYER *vp)
{
	printf("TIMER_EPG_DISPLAYOSD\n");
	/*int ret =*/ epg_displayOSD(vp->vlc->mp, 6000);
	//printf("timertest %i\n", ret);
}
#endif

// TIMER_EPG_GENDVBPLAYLIST
void epgDvbGenPlaylist (TVLCPLAYER *vp)
{
	char *path = getPlayingPath(vp);
	if (!path) return;
	
	unsigned int hash = getHash(path);
	char title[MAX_PATH_UTF8];
	tagRetrieveByHash(vp->tagc, hash, MTAG_Publisher, title, MAX_PATH_UTF8);
	if (!*title) strcpy(title, path);
		
	// don't spam the playlist by repeating entries
	int pos = -1;
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByName(vp->plm, title);
	if (plc){
		playlistDeleteRecords(plc);		// remove playlist entries only
		pos = playlistGetPlaylistIndex(getDisplayPlaylist(vp), plc);
	}else{
		plc = playlistManagerCreatePlaylist(vp->plm, title, 1);
		if (plc){
			PLAYLISTCACHE *plcQ = getDisplayPlaylist(vp);
			if (plcQ)
				pos = playlistAddPlc(plcQ, plc);
			else
				pos = playlistAddPlc(getPrimaryPlaylist(vp), plc);
		}
	}

	if (!plc){
		my_free(path);
		dbprintf(vp, "playlist creation failed");
		return;
	}
	
	TEPG *pageepg = pageGetPtr(vp, PAGE_EPG);
	int entriesAdded = dvbCreatePlaylistAuto(vp, plc, path, pageepg->vepg.epg, pageepg->vepg.total);
	dbprintf(vp, "%i programmes added", entriesAdded);
	my_free(path);
		
	if (entriesAdded){
		plc->pr->selectedItem = -1;
		//invalidateShelfAlbum(vp, pageGetPtr(vp, PAGE_PLY_SHELF), 0);
		//vp->displayPlaylist = playlistManagerGetPlaylistIndex(vp->plm, plc);
		setDisplayPlaylist(vp, plc);
		
		if (!plc->parent)
			playlistAddPlc(getPrimaryPlaylist(vp), plc);
		if (plc->parent)
			plc->parent->pr->selectedItem = pos;

		page2Set(vp->pages, PAGE_PLY_SHELF, 1);
		
		TEPG *epg = pageGetPtr(vp, PAGE_EPG);
		if (ccGetState(epg->programme.listbox))
			ccDisable(epg->programme.listbox);
		if (ccGetState(epg->guide.paneChannels))
			ccDisable(epg->guide.paneChannels);
		if (ccGetState(epg->guide.paneContents))
			ccDisable(epg->guide.paneContents);
	}
	playlistsForceRefresh(vp, 0);
}

static inline int epgGetProgrammeDetails (TVLCPLAYER *vp, TVLCCONFIG *vlc, TEPG *epg)
{
	epg->vepg.playing.channelIdx = 0;
	epg->vepg.playing.programmeIdx = 0;
	epg->vepg.playing.pid = 0;
	epg->vepg.playing.start = 0;
	
	if (epg->vepg.epg)
		epg_freeEpg(epg->vepg.epg, epg->vepg.total);
	epg->vepg.epg = epg_dupEpg(vlc, &epg->vepg.total);

	// find playing programme
	if (epg->vepg.epg && epg->vepg.total){
		char nowPlaying[MAX_PATH_UTF8+1];
		*nowPlaying = 0;
		
		char *path = getPlayingPath(vp);
		if (path){
			tagRetrieve(vp->tagc, path, MTAG_NowPlaying, nowPlaying, sizeof(nowPlaying));
			my_free(path);
		}

		// find, store and remove programme ID # title string
		for (int i = 0; i < epg->vepg.total; i++){
			if (epg->vepg.epg[i]->psz_name){
				char *bracket = strrchr(epg->vepg.epg[i]->psz_name, ' ');
				if (bracket){
					epg->vepg.epg[i]->programme = (int)strtol(bracket, NULL, 10);
					if (epg->vepg.epg[i]->programme < 0) epg->vepg.epg[i]->programme = 0;

					char *bracket = strrchr(epg->vepg.epg[i]->psz_name, '[');
					if (bracket) *(bracket-1) = 0;
				}
			}
		}

		if (*nowPlaying){
			TVLCEPGEVENT *current;
			for (int i = 0; i < epg->vepg.total; i++){
				current = epg->vepg.epg[i]->p_current;
				if (!current || !strncmp(current->psz_name, nowPlaying, MAX_PATH_UTF8)){
					for (int j = 0; j < epg->vepg.epg[i]->i_event; j++){
						if (!strncmp(epg->vepg.epg[i]->pp_event[j]->psz_name, nowPlaying, MAX_PATH_UTF8)){
							epg->vepg.playing.channelIdx = i;
							epg->vepg.playing.programmeIdx = j;
							epg->vepg.playing.pid = epg->vepg.epg[i]->programme;
							epg->vepg.playing.start = epg->vepg.epg[i]->pp_event[j]->i_start;
							//printf("now playing [%i,%i] '%s' '%s'\n", i, j, epg->vepg.epg[i]->psz_name, current->psz_name);
							return epg->vepg.total;
						}
					}
				}
			}
		}
		return epg->vepg.total;
	}
	return 0;
}

void epgUpdateCurrentProgramme (TVLCPLAYER *vp, TEPG *epg)
{
	if (!getPlayState(vp)) return;
	
	//printf("epgUpdateCurrentProgramme %i\n", getPlayState(vp));
	
	TVLCEPGEVENT *current = epg_getGetProgramme(epg->vepg.epg, epg->vepg.total, epg->vepg.playing.channelIdx, epg->vepg.playing.programmeIdx);
	if (current){
		if (current->psz_short_description){
			char bstart[128]; char bend[128]; char bdate[512];
  							
			time_t start = current->i_start;
   			time_t end = current->i_start + current->i_duration;
			struct tm *tstart = localtime(&start);
							
			strftime(bstart, sizeof(bstart), "%a %d %b %H:%M", tstart);
   			struct tm *tend = localtime(&end);
			strftime(bend, sizeof(bend), "%a %d %b %H:%M", tend);
			__mingw_snprintf(bdate, sizeof(bdate), "%s  -  %s", bstart, bend);
			__mingw_snprintf(bend, sizeof(bend), "%ix%i", vp->vlc->videoWidth, vp->vlc->videoHeight);

			char *path = getPlayingPath(vp);
			if (path){
				char *channel = epg_getChannelName(epg->vepg.epg, epg->vepg.total, epg->vepg.playing.channelIdx);
				if (channel){
					tagAdd(vp->tagc, path, MTAG_Title, channel, 1);
					my_free(channel);
				}
				int pos = getPlayingItem(vp);
				if (pos != -1){
					__mingw_snprintf(bstart, sizeof(bstart), "%i", pos+1);
					tagAdd(vp->tagc, path, MTAG_POSITION, bstart, 1);
				}

				tagAdd(vp->tagc, path, MTAG_Description, current->psz_short_description, 1);
				tagAdd(vp->tagc, path, MTAG_Date, bdate, 1);
				tagAdd(vp->tagc, path, MTAG_Setting, bend, 1);
				tagAdd(vp->tagc, path, MTAG_PATH, path, 1);
				my_free(path);
			}
		}
		epg_freeEPGEvent(current);
	}
}

static inline int epgQsStartA (const void *a, const void *b)
{
	TEPGSORT *item1 = (TEPGSORT*)a;
	TEPGSORT *item2 = (TEPGSORT*)b;

	return (int)(item1->start - item2->start);
}

static inline void epgSortProgramme (TTREE *tree, const int branchId)
{
	TTREEENTRY *entry = treeFind(tree, EPG_GUIDE_ROOTID, branchId, 1);
	if (!entry) return;

	int total = treeEntryGetSubItemCount(entry);
	if (total < 2) return;	
	
	TEPGSORT *sortlist = my_calloc(total, sizeof(TEPGSORT));
	if (sortlist){
		int ct = 0;
		TLISTITEM *item = entry->head;

		while(item && ct < total){
			TTREEENTRY *subentry = treeListGetSubEntry(item);
			if (subentry){
				TGUIDE_EVENT *guide = treeEntryGetStorage(subentry);
				if (guide)
					sortlist[ct++].start = guide->start;
			}
			item = item->next;
		}

		total = ct;
		if (total){
			qsort(sortlist, total, sizeof(TEPGSORT), epgQsStartA);
			for (int i = total-2; i >= 0; i--)
				treeEntryMoveEx(tree, branchId, sortlist[i].start, sortlist[i+1].start, 2);
		}

		my_free(sortlist);
	}
}

static inline void epgGuideSortDatabase (TEPG *epg)
{
	for (int i = 0; i < epg->vepg.total; i++){
		if (epg->vepg.epg && epg->vepg.epg[i])
			epgSortProgramme(epg->guide.database, epg->vepg.epg[i]->programme);
	}
}

static inline void epgGuideUpdatePending (TEPG *epg)
{
	if (epg->guide.updatePending){
		epg->guide.updatePending = 0;
		epgGuideSyncDatabase(epg);
		epgGuideSortDatabase(epg);
		epgBuildGuide(epg, NULL);
		
		//printf("epgGuideUpdatePending updatePending %i\n", epg->guide.updatePending);
	}
}


int epgGetTotalProgrammes (TVLCPLAYER *vp)
{
	if (!getPlayState(vp) || getPlayState(vp) == 8)
		return 0;
	if (hasPageBeenAccessed(vp, PAGE_EPG)){
		TEPG *epg = pageGetPtr(vp, PAGE_EPG);
		return epg->vepg.total;
	}
	return 0;
}


// TIMER_EPG_UPDATE
void epgGetUpdate (TVLCPLAYER *vp)
{	

	char *path = getPlayingPath(vp);
	if (!path) return;
	int wontHaveEpg = !isMediaDVB(path) || !isMediaVideo8(path);
	my_free(path);
	if (wontHaveEpg) return;

	//printf("TIMER_EPG_UPDATE\n");
	
	TEPG *epg = pageGetPtr(vp, PAGE_EPG);

	if (epgGetProgrammeDetails(vp, vp->vlc, epg)){
		epgUpdateCurrentProgramme(vp, epg);
		epg->guide.updatePending++;

		//printf("epgGetUpdate updatePending %i (vguide total %i)\n", epg->guide.updatePending, epg->vepg.total);
#if 0
		epgGuideSyncDatabase(epg);
		epgGuideSortDatabase(epg);
		epgBuildGuide(epg, NULL);
#endif
	}

	int state = (epgGuideGetBranchTotal(epg->guide.database) > 1) && (epg->displayMode == EPGBUTTON_GUIDE) && !epg->guide.isRoot;
	buttonsStateSet(epg->btns, EPGBUTTON_LEFT, state);
	buttonsStateSet(epg->btns, EPGBUTTON_RIGHT, state);
	buttonsStateSet(epg->btns, EPGBUTTON_GENPLY, epg->vepg.total >= 2);

}

static inline void epgRenderMeta (TEPG *epg, TVLCPLAYER *vp, TFRAME *frame)
{
	TMETA *meta = pageGetPtr(vp, PAGE_META);
	TMETADESC desc;
	my_memcpy(&desc, &meta->desc, sizeof(desc));
		
	PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
	if (!plc) plc = getDisplayPlaylist(vp);
	
	if (plc->pr->playingItem >= 0)
		desc.trackPosition = plc->pr->playingItem;
	else
		desc.trackPosition = 0;
	desc.uid = playlistManagerGetPlaylistUID(vp->plm, plc);

	ccGetMetrics(epg->guide.paneChannels, (TMETRICS*)&desc.x);
	desc.w = frame->width - desc.x;
	desc.h = frame->height - desc.y;
	
	//lDrawRectangleFilled(frame, desc.x-1, desc.y-1, desc.x+desc.w+1, desc.y+desc.h+1, 140<<24|COL_BLACK);
	metaRender(vp, frame, meta, &desc, META_FONT, 0);
}

static inline int page_epgRender (TEPG *epg, TVLCPLAYER *vp, TFRAME *frame)
{
	if (epg->vepg.total < 1){
		page2SetPrevious(epg);
		return 0;
	}
	
	if (epg->guide.updatePending){
		drawImg(vp, frame, epg->guide.icons.updatePending, epg->guide.icons.pendingX, epg->guide.icons.pendingY);
		buttonsStateSet(epg->btns, EPGBUTTON_GUIDE, 0);
	}
	
	buttonsRenderAll(epg->btns, frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);
	

	if (epg->displayMode == EPGBUTTON_GUIDE){
		ccRender(epg->guide.paneChannels, frame);
		ccRender(epg->guide.paneContents, frame);
	}else if (epg->displayMode == EPGBUTTON_CHANNELS){
		ccRender(epg->programme.listbox, frame);
	}else if (epg->displayMode == EPGBUTTON_META){
		epgRenderMeta(epg, vp, frame);
	}

	if (epg->guide.updatePending)
		buttonsStateSet(epg->btns, EPGBUTTON_GUIDE, 1);

	return 1;
}

static inline int page_epgInput (TEPG *epg, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
  	const int x = flags >> 16;
  	const int y = flags & 0xFFFF;
	  	
	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
		if (ccGetState(epg->programme.listbox)){
			lbScrollUp(epg->programme.listbox);
		}else if (ccGetState(epg->guide.paneContents)){
			if (ccPositionIsOverlapped(epg->guide.paneContents, x, y))
				paneScroll(epg->guide.paneContents, EPG_PANE_SCROLL_DELTA);
			else if (ccPositionIsOverlapped(epg->guide.paneChannels, x, y))
				paneScroll(epg->guide.paneChannels, EPG_PANE_SCROLL_DELTA);
		}
		
		break;
		
	  case PAGE_IN_WHEEL_BACK:
	  	if (ccGetState(epg->programme.listbox)){
			lbScrollDown(epg->programme.listbox);
		}else if (ccGetState(epg->guide.paneContents)){
			if (ccPositionIsOverlapped(epg->guide.paneContents, x, y))
				paneScroll(epg->guide.paneContents, -EPG_PANE_SCROLL_DELTA);
			else if (ccPositionIsOverlapped(epg->guide.paneChannels, x, y))
				paneScroll(epg->guide.paneChannels, -EPG_PANE_SCROLL_DELTA);
		}
	  	break;
	  
	  /*case PAGE_IN_TOUCH_DOWN:
	  case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  	break;*/
	}
		
	return 1;
}

static inline int page_epgStartup (TEPG *epg, TVLCPLAYER *vp, const int fw, const int fh)
{
	
	epg->btns = buttonsCreate(vp->cc, PAGE_EPG, EPGBUTTON_TOTAL, ccbtn_cb);
	
	TCCBUTTON *btn = buttonsCreateButton(epg->btns, L"epg/channels.png", NULL, EPGBUTTON_CHANNELS, 1, 1, 3, 0);
	btn = buttonsCreateButton(epg->btns, L"epg/guide.png", NULL, EPGBUTTON_GUIDE, 1, 1, ccGetPositionX(btn)+ccGetWidth(btn)+6, 0);
	epg->guide.icons.pendingX = ccGetPositionX(btn);
	epg->guide.icons.pendingY = ccGetPositionY(btn);
	btn = buttonsCreateButton(epg->btns, L"epg/left.png", NULL, EPGBUTTON_LEFT, 0, 1, ccGetPositionX(btn)+ccGetWidth(btn)+2, 0);
	btn = buttonsCreateButton(epg->btns, L"epg/right.png", NULL, EPGBUTTON_RIGHT, 0, 1, ccGetPositionX(btn)+ccGetWidth(btn)+2, 0);
	btn = buttonsCreateButton(epg->btns, L"epg/back.png", NULL, EPGBUTTON_BACK, 1, 0, 0, 0);
	ccSetPosition(btn, fw-1 - ccGetWidth(btn), 0);
	btn = buttonsCreateButton(epg->btns, L"epg/meta.png", NULL, EPGBUTTON_META, 1, 1, 0, 0);
	ccSetPosition(btn, buttonsPosXGet(epg->btns, EPGBUTTON_BACK) - ccGetWidth(btn) - 16, 0);
	btn = buttonsCreateButton(epg->btns, L"epg/genplaylist.png", NULL, EPGBUTTON_GENPLY, 0, 0, 0, 0);
	ccSetPosition(btn, buttonsPosXGet(epg->btns, EPGBUTTON_META) - ccGetWidth(btn) - 16, 0);

	int x1 = 10;
	int y1 = buttonsHeightGet(epg->btns, EPGBUTTON_GUIDE) + 8;
	int x2 = fw-1 - x1;
	
	epg->programme.listbox = ccCreate(vp->cc, PAGE_EPG, CC_LISTBOX, epgCcObject_cb, &vp->gui.ccIds[CCID_LISTBOX_EPG_PROGRAMME], x2 - x1, fh - y1 - 4);
	epg->programme.listbox->font = EPG_PROGRAMLISTBOX_FONT;
	epg->programme.listbox->flags.drawBaseGfx = 0;
	epg->programme.listbox->flags.drawBaseBlur = 0;
	lbScrollbarSetWidth(epg->programme.listbox, SCROLLBAR_VERTWIDTH);
	ccSetMetrics(epg->programme.listbox, x1, y1, fw-32, 32);
	epg->programme.listbox->verticalPadding += 3;
	ccDisable(epg->programme.listbox);
	
	
	return 1;
}

static inline int64_t epg_paneCont_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TPANE *pane = (TPANE*)object;
	
	//if (msg != CC_MSG_RENDER)
	//	printf("epg_paneCont_cb in %p, msg:%i %I64d %I64d %p\n", pane, msg, data1, data2, dataPtr);
	
	 if (msg == PANE_MSG_SLIDE){
		TEPG *epg = ccGetUserData(pane);
		if (epg->guide.isRoot){
			epg->guide.paneChannels->offset.y = data2;
			//epg->guide.paneChannels->isInvalidated = 1;
			paneScroll(epg->guide.paneChannels, 0);
		}
		
	}else if (msg == CC_MSG_RENDER){
		TEPG *epg = ccGetUserData(pane);
		if (!epg->guide.isRoot){
			TGUIDE_EVENT *guide = epgGuideGetEvent(epg->guide.database, EPG_GUIDE_ROOTID, epg->guide.programmeIdx);
			if (guide)
				printSingleLineShadow(dataPtr, LFTW_B34, 0, 27, 0xFFFFFFF, COL_CYAN, guide->name);
		}
	}else if (msg == PANE_MSG_TEXT_SELECTED/* || msg == PANE_MSG_IMAGE_SELECTED*/){
		if (data2 == EPG_GUIDE_EVENT_INVALID) return 1;	// should never happen, but just incase
		int isRoot = (data2>>63)&0x01;
		const int pid = (data2&0x7FFFFFFF00000000)>>32;
		const int eventId = data2&0xFFFFFFFF;

		//printf("paneCont_cb: %X %X, %i %i, %i\n", (int)(data2>>32), (int)(data2&0xFFFFFFFF), isRoot, pid, eventId);

		TEPG *epg = ccGetUserData(pane);
		if (!isRoot){
			TGUIDE_EVENT *guide = epgGuideFindEvent(epg->guide.database, pid, eventId);
			if (guide){
				guide->ui.paneExpanded ^= 1;
			
				//printf("paneCont_cb lineheight %i %i\n", guide->ui.paneExpanded, epg->guide.paneContents->vertLineHeight);
			
				char *text = epgGuideBuildString(guide, guide->ui.paneExpanded);
				if (text){
					int itemId = data1 + (msg == PANE_MSG_IMAGE_SELECTED);
					paneTextReplace(pane, itemId, text);
					my_free(text);
				}
			}
		}else{
			epg->guide.programmeIdx = eventId;
			epg->guide.isRoot = 0;
			epg->guide.yOffset = pane->offset.y;
			paneScrollReset(pane);
			epgBuildGuide(epg, NULL);
			
			int state = epgGuideGetBranchTotal(epg->guide.database) > 1;
			buttonsStateSet(epg->btns, EPGBUTTON_LEFT, state);
			buttonsStateSet(epg->btns, EPGBUTTON_RIGHT, state);
		}
	}else if (msg == CC_MSG_ENABLED){
		paneScrollReset(pane);

		TEPG *epg = ccGetUserData(pane);
		epgGuideUpdatePending(epg);
		
		int state = epgGuideGetBranchTotal(epg->guide.database) > 1 && !epg->guide.isRoot;
		buttonsStateSet(epg->btns, EPGBUTTON_LEFT, state);
		buttonsStateSet(epg->btns, EPGBUTTON_RIGHT, state);
		
	}else if (msg == CC_MSG_DISABLED){
		TEPG *epg = ccGetUserData(pane);
		buttonsStateSet(epg->btns, EPGBUTTON_LEFT, 0);
		buttonsStateSet(epg->btns, EPGBUTTON_RIGHT, 0);
	}
	return 1;
}

static inline int64_t epg_paneChan_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TPANE *pane = (TPANE*)object;
	
	//if (msg != CC_MSG_RENDER)
	//	printf("epg_paneChan_cb in %p, msg:%i %I64d %I64d %p\n", pane, msg, data1, data2, dataPtr);
	
	
	if (msg == PANE_MSG_SLIDE){
		TEPG *epg = ccGetUserData(pane);
		if (epg->guide.isRoot){
			epg->guide.paneContents->offset.y = data2;
			paneScroll(epg->guide.paneContents, 0);
		}
		
	}else if (msg == PANE_MSG_TEXT_SELECTED/* || msg == PANE_MSG_IMAGE_SELECTED*/){
		if (data2 == EPG_GUIDE_EVENT_INVALID) return 1;	// should never happen, but just incase
		//int itemId = data1 + (msg == PANE_MSG_IMAGE_SELECTED);
		//int isRoot = (data2>>63)&0x01;
		//const int pid = (data2&0x7FFFFFFF00000000)>>32;
		const int eventId = data2&0xFFFFFFFF;

		//printf("paneChan_cb: %X %X, %i %i, %i\n", (int)(data2>>32), (int)(data2&0xFFFFFFFF), isRoot, pid, eventId);

		TEPG *epg = ccGetUserData(pane);

		epg->guide.programmeIdx = eventId;
		epg->guide.isRoot = 0;
		epg->guide.yOffset = pane->offset.y;
		paneScrollReset(epg->guide.paneContents);

		epgBuildGuide(epg, NULL);
			
		int state = epgGuideGetBranchTotal(epg->guide.database) > 1;
		buttonsStateSet(epg->btns, EPGBUTTON_LEFT, state);
		buttonsStateSet(epg->btns, EPGBUTTON_RIGHT, state);

	}else if (msg == CC_MSG_ENABLED){
		paneScrollReset(pane);

		TEPG *epg = ccGetUserData(pane);
		epgGuideUpdatePending(epg);
		
		int state = epgGuideGetBranchTotal(epg->guide.database) > 1 && !epg->guide.isRoot;
		buttonsStateSet(epg->btns, EPGBUTTON_LEFT, state);
		buttonsStateSet(epg->btns, EPGBUTTON_RIGHT, state);
		
	}else if (msg == CC_MSG_DISABLED){
		TEPG *epg = ccGetUserData(pane);
		buttonsStateSet(epg->btns, EPGBUTTON_LEFT, 0);
		buttonsStateSet(epg->btns, EPGBUTTON_RIGHT, 0);
	}
	return 1;
}

static inline int page_epgInitalize (TEPG *epg, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_EPG);

	epg->displayMode = EPGBUTTON_GUIDE;
	epg->guide.programmeIdx = 0;
	epg->guide.database = treeCreate("epgroot", EPG_GUIDE_ROOTID);
	epg->guide.isRoot = 1;
	epg->guide.updatePending = 0;
		
	const int x = 2;
	const int y = 64;
	const int chanWidth = width/4.2;
	const int contWidth = (width - chanWidth) - x - 1;
	
	TPANE *pane = ccCreateEx(vp->cc, PAGE_EPG, CC_PANE, epg_paneChan_cb, NULL, chanWidth, height-y, epg);
	epg->guide.paneChannels = pane;
	paneSetLayout(pane, PANE_LAYOUT_VERT);
	paneSwipeEnable(pane);
	paneSetAcceleration(pane, 1.0, 1.7);
	labelRenderFlagsSet(pane->base, LABEL_RENDER_TEXT|LABEL_RENDER_BASE|LABEL_RENDER_HOVER_OBJ);
	labelBaseColourSet(pane->base, 90<<24|COL_BLACK);
	ccSetPosition(pane, x, y);
	ccEnable(pane);


	pane = ccCreateEx(vp->cc, PAGE_EPG, CC_PANE, epg_paneCont_cb, NULL, contWidth, height-y, epg);
	epg->guide.paneContents = pane;
	paneSetLayout(pane, PANE_LAYOUT_VERT);
	paneSwipeEnable(pane);
	paneSetAcceleration(pane, 1.0, 1.7);
	paneTextMulityLineEnable(pane);
	paneTextWordwrapEnable(pane);
	labelRenderFlagsSet(pane->base, LABEL_RENDER_TEXT|LABEL_RENDER_BASE);
	labelBaseColourSet(pane->base, 90<<24|COL_BLACK);
	ccSetPosition(pane, chanWidth + x + 1, y);
	ccEnable(pane);

	
	wchar_t bufferw[MAX_PATH+1];
	epg->guide.icons.pgmEvent = 0;//artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"pane/stream32.png"));
	epg->guide.icons.updatePending = artManagerImageAdd(vp->am, buildSkinD(vp,bufferw,L"epg/epgpending.png"));

	
	return 1;
}

static inline int page_epgShutdown (TEPG *epg, TVLCPLAYER *vp)
{

	ccDelete(epg->programme.listbox);
	ccDelete(epg->guide.paneChannels);
	ccDelete(epg->guide.paneContents);
	epgGuideDestroy(epg->guide.database);
	buttonsDeleteAll(epg->btns);

	if (epg->vepg.epg)
		epg_freeEpg(epg->vepg.epg, epg->vepg.total);
	return 1;
}

static inline int page_epgRenderBegin (TEPG *epg, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	epgSetActiveControl(epg, EPGBUTTON_GUIDE);

	if (epg->guide.isRoot){
		epg->guide.paneChannels->offset.x = epg->guide.xOffset;
		epg->guide.paneContents->offset.y = epg->guide.yOffset;
	}
	ccHoverRenderSigEnable(vp->cc, 25.0);
	return 1;
}

static inline void page_epgRenderEnd (TEPG *epg, TVLCPLAYER *vp, int64_t destId, int64_t data2, void *opaquePtr)
{
	ccHoverRenderSigDisable(vp->cc);
	if (epg->guide.isRoot){
		epg->guide.xOffset = epg->guide.paneChannels->offset.x;
		epg->guide.yOffset = epg->guide.paneContents->offset.y;
	}
}

int page_epgCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TEPG *epg = (TEPG*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_epgCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_epgRender(epg, epg->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		return page_epgRenderBegin(epg, epg->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		page_epgRenderEnd(epg, epg->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_epgInput(epg, epg->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_epgStartup(epg, epg->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_epgInitalize(epg, epg->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_epgShutdown(epg, epg->com->vp);
		
	}
	
	return 1;
}


