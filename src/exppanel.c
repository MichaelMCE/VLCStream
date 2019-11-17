
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
#include <winioctl.h>





enum _panelbtn {
	PANEL_BUTTON_1 = 0,
	PANEL_BUTTON_2,
	PANEL_BUTTON_3,
	PANEL_BUTTON_4,
	PANEL_BUTTON_5,
	PANEL_BUTTON_6,
	PANEL_BUTTON_7,
	PANEL_BUTTON_8,
	PANEL_BUTTON_TOTAL
};


#define PANEL_BUTTON_BACK		PANEL_BUTTON_1
#define PANEL_BUTTON_AUDIO		PANEL_BUTTON_2
#define PANEL_BUTTON_VIDEO		PANEL_BUTTON_3
#define PANEL_BUTTON_IMAGE		PANEL_BUTTON_4
#define PANEL_BUTTON_PLAYLIST	PANEL_BUTTON_5
#define PANEL_BUTTON_FOLDERI	PANEL_BUTTON_6
#define PANEL_BUTTON_MODULE		PANEL_BUTTON_7
#define PANEL_BUTTON_REFRESH	PANEL_BUTTON_8




/*
static const char *extVideoA[] = {
	EXTVIDEOA,
	""
};*/


static const wchar_t *extAudio[] = {
 	EXTAUDIO,
	L""
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
static wchar_t *extMedia[] = {
	EXTAUDIO,
	EXTVIDEO,
	L""
};*/




static inline void expanPanelChildrenSetState (TPANEL *panel, const int sid, const int state)
{
	TPANELSEPARATOR *ps = panelImgStorageGet(panel, sid);
	if (!ps) return;
	
	for (int i = 0; i < ps->idTotal; i++){
		TPANELIMG *img = panelImgGetItem(panel, ps->idList[i]);
		if (img){
			//printf("img %i\n", img->id);

			if (!state)
				ccDisable(img->btn);
			else
				ccEnable(img->btn);
		}
	}
}

void expanPanelDisableModuleButtons (TPANEL *panel)
{
	buttonsSetState(panel->btns, 0);
}

void expanPanelEnableModuleButton (TPANEL *panel, const int panel_btn_id)
{
	expanPanelDisableModuleButtons(panel);

	buttonsStateSet(panel->btns, panel_btn_id, 1);
	buttonsStateSet(panel->btns, PANEL_BUTTON_REFRESH, 1);
	buttonsStateSet(panel->btns, PANEL_BUTTON_BACK, 1);
	//ccButtonEnable(panel, panel_btn_id);
}

void expanPanelSeparatorDelete (TPANELSEPARATOR *ps)
{
	if (ps){
		if (ps->idList)
			my_free(ps->idList);
		my_free(ps);
	}
}

TPANELSEPARATOR *expanPanelSeparatorCreate ()
{
	TPANELSEPARATOR *ps = my_calloc(1, sizeof(TPANELSEPARATOR));
	if (ps)
		ps->state = 1;
	return ps;
}

void expanPanelSeparatorIdClearAll (TPANELSEPARATOR *ps)
{
	if (ps->idList)
		ps->idList[0] = 0;
	ps->idTotal = 0;
}

int expanPanelSeparatorIdAdd (TPANELSEPARATOR *ps, const int id)
{
	if (ps->idList == NULL){
		ps->idList = (int*)my_malloc(sizeof(int));
		if (ps->idList){
			ps->idList[0] = id;
			ps->idTotal = 1;
		}
	}else{
		ps->idList = (int*)my_realloc(ps->idList, (ps->idTotal+1) * sizeof(int));
		if (ps->idList)
			ps->idList[ps->idTotal++] = id;
	}
	return ps->idTotal;
}

int expGetEntryId (TFB *fb, const int id, const char *str)
{
	TTREEENTRY *entry = treeEntryFind(fb->tree->root, id);
	if (!entry) return 0;

	
	TLISTITEM *item = entry->head;
	//int ct = 0;
	
	while(item){
		entry = treeListGetSubEntry(item);
		if (treeEntryIsBranch(entry)){
			if (!stricmp(entry->name, str)){
				//printf("found %i, %i '%s'\n", ct, entry->id, entry->name);
				return entry->id;
			}
		}
		item = item->next;
	}

	return 0;
}
		
int64_t expan_panel_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;

	TPANEL *panel = (TPANEL*)object;
	TEXPPANEL *expan = (TEXPPANEL*)ccGetUserData(panel);
	
	//printf("expan_panel_cb: %i %i %i %p\n", msg, (int)data1, (int)data2, dataPtr);
	
	
	if (msg == PANEL_MSG_ITEMDELETE){
		//if (!ccGetState(panel)) return 0;
	
		TPANELIMG *img = (TPANELIMG*)dataPtr;
		if (img){
			//if (panelImgAttributeCheck(panel, data1, PANEL_ITEM_SEPARATOR) > 0){
			if (img->attributes&PANEL_ITEM_SEPARATOR){
				//TPANELSEPARATOR *ps = panelImgStorageGet(panel, data1);
				TPANELSEPARATOR *ps = img->storage;
				if (ps) expanPanelSeparatorDelete(ps);

			//}else if (panelImgAttributeCheck(panel, data1, PANEL_ITEM_DRIVE) > 0){
			}
		}
	}else if (msg == PANEL_MSG_ITEMSELECTED){
		//if (!ccGetState(panel)) return 0;
		
		if (panelImgAttributeCheck(panel, data1, PANEL_ITEM_DRIVE) > 0){
			TFILEPANE *filepane = pageGetPtr(panel->cc->vp, PAGE_FILE_PANE);

			intptr_t idx = (intptr_t)dataPtr;
			char *drive = expan->drives[idx].drive;
			filepaneSetLogicalDriveRoot(filepane, drive[0]);
			page2Set(filepane->com->pages, PAGE_FILE_PANE, 1);
						
		}else if (panelImgAttributeCheck(panel, data1, PANEL_ITEM_LINK) > 0){
			TFILEPANE *filepane = pageGetPtr(panel->cc->vp, PAGE_FILE_PANE);
			intptr_t idx = (intptr_t)dataPtr;
			//printf("PANEL_ITEM_LINK idx: %i\n", idx);
			
			if (idx&16384){
				idx &= ~16384;
				TSTRSHORTCUT *sc = fbShortcutsGet(&expan->userLinks, idx);
				//wprintf(L"PANEL_ITEM_LINK link: '%s' '%s'\n", sc->link, sc->path);

				char *path = convertto8(sc->path);
				if (path){
					filepaneSetPath(filepane, path);
					my_free(path);
					page2Set(filepane->com->pages, PAGE_FILE_PANE, 1);
				}
			}else{
				//wprintf(L"PANEL_ITEM_LINK shellfolder: '%s' '%s'\n", expan->shellFolders[idx].name, expan->shellFolders[idx].location);
				char *path = convertto8(expan->shellFolders[idx].location);
				if (path){
					filepaneSetPath(filepane, path);
					my_free(path);
					page2Set(filepane->com->pages, PAGE_FILE_PANE, 1);
				}
			}
		}else if (panelImgAttributeCheck(panel, data1, PANEL_ITEM_MODULE) > 0){
			intptr_t idx = (intptr_t)dataPtr;
			TSTRSHORTCUT *sc = fbShortcutsGet(&expan->userLinks, idx);
			//wprintf(L"fbShortcutsGet %i, '%s' '%s'\n", idx, sc->link, sc->path);
			
			if (!wcscmp(expan->selectedFile, sc->path)){
				expan->selectedFile[0] = 0;
				expanPanelDisableModuleButtons(panel);
				buttonsStateSet(panel->btns, PANEL_BUTTON_REFRESH, 1);
				buttonsStateSet(panel->btns, PANEL_BUTTON_BACK, 1);
				return 1;
			}
			
			wcsncpy(expan->selectedFile, sc->path, MAX_PATH);
			
			if (hasPathExt(expan->selectedFile, extAudio)){
				expanPanelEnableModuleButton(panel, PANEL_BUTTON_AUDIO);
				char *str = convertto8(expan->selectedFile);
				if (str){
					TCCBUTTON *btn = buttonsButtonGet(panel->btns, PANEL_BUTTON_AUDIO);
					buttonFaceTextUpdate(btn, BUTTON_PRI, str);
					buttonFaceTextWidthSet(btn, BUTTON_PRI, PANEL_MODULE_TEXT_MAXWIDTH);
					buttonFaceTextFlagsSet(btn, BUTTON_PRI, PF_CLIPWRAP);
					buttonFaceTextColourSet(btn, BUTTON_PRI, 255<<24|COL_BLACK, 255<<24|COL_WHITE, 127<<24|COL_BLUE_SEA_TINT);
					my_free(str);
				}

			}else if (hasPathExt(expan->selectedFile, extVideo)){
				expanPanelEnableModuleButton(panel, PANEL_BUTTON_VIDEO);
				char *str = convertto8(expan->selectedFile);
				if (str){
					TCCBUTTON *btn = buttonsButtonGet(panel->btns, PANEL_BUTTON_VIDEO);
					buttonFaceTextUpdate(btn, BUTTON_PRI, str);
					buttonFaceTextWidthSet(btn, BUTTON_PRI, PANEL_MODULE_TEXT_MAXWIDTH);
					buttonFaceTextFlagsSet(btn, BUTTON_PRI, PF_CLIPWRAP);
					buttonFaceTextColourSet(btn, BUTTON_PRI, 255<<24|COL_BLACK, 255<<24|COL_WHITE, 127<<24|COL_BLUE_SEA_TINT);
					my_free(str);
				}

			}else if (hasPathExt(expan->selectedFile, extImage)){
				expanPanelEnableModuleButton(panel, PANEL_BUTTON_IMAGE);
				char *str = convertto8(expan->selectedFile);
				if (str){
					TCCBUTTON *btn = buttonsButtonGet(panel->btns, PANEL_BUTTON_IMAGE);
					buttonFaceTextUpdate(btn, BUTTON_PRI, str);
					buttonFaceTextWidthSet(btn, BUTTON_PRI, PANEL_MODULE_TEXT_MAXWIDTH);
					buttonFaceTextFlagsSet(btn, BUTTON_PRI, PF_CLIPWRAP);
					buttonFaceTextColourSet(btn, BUTTON_PRI, 255<<24|COL_BLACK, 255<<24|COL_WHITE, 127<<24|COL_BLUE_SEA_TINT);
					my_free(str);
				}
			}else if (hasPathExt(expan->selectedFile, extPlaylists)){
				expanPanelEnableModuleButton(panel, PANEL_BUTTON_PLAYLIST);
				char *str = convertto8(expan->selectedFile);
				if (str){
					TCCBUTTON *btn = buttonsButtonGet(panel->btns, PANEL_BUTTON_PLAYLIST);
					buttonFaceTextUpdate(btn, BUTTON_PRI, str);
					buttonFaceTextWidthSet(btn, BUTTON_PRI, PANEL_MODULE_TEXT_MAXWIDTH);
					buttonFaceTextFlagsSet(btn, BUTTON_PRI, PF_CLIPWRAP);
					buttonFaceTextColourSet(btn, BUTTON_PRI, 255<<24|COL_BLACK, 255<<24|COL_WHITE, 127<<24|COL_BLUE_SEA_TINT);
					my_free(str);
				}
			}else if (isDirectoryW(expan->selectedFile)){
				expanPanelEnableModuleButton(panel, PANEL_BUTTON_FOLDERI);
				char *str = convertto8(expan->selectedFile);
				if (str){
					TCCBUTTON *btn = buttonsButtonGet(panel->btns, PANEL_BUTTON_FOLDERI);
					buttonFaceTextUpdate(btn, BUTTON_PRI, str);
					buttonFaceTextWidthSet(btn, BUTTON_PRI, PANEL_MODULE_TEXT_MAXWIDTH);
					buttonFaceTextFlagsSet(btn, BUTTON_PRI, PF_CLIPWRAP);
					buttonFaceTextColourSet(btn, BUTTON_PRI, 255<<24|COL_BLACK, 255<<24|COL_WHITE, 127<<24|COL_BLUE_SEA_TINT);
					my_free(str);
				}
			}else{
				expanPanelEnableModuleButton(panel, PANEL_BUTTON_MODULE);
				char *str = convertto8(expan->selectedFile);
				if (str){
					TCCBUTTON *btn = buttonsButtonGet(panel->btns, PANEL_BUTTON_MODULE);
					buttonFaceTextUpdate(btn, BUTTON_PRI, str);
					buttonFaceTextWidthSet(btn, BUTTON_PRI, PANEL_MODULE_TEXT_MAXWIDTH);
					buttonFaceTextFlagsSet(btn, BUTTON_PRI, PF_CLIPWRAP);
					buttonFaceTextColourSet(btn, BUTTON_PRI, 255<<24|COL_BLACK, 255<<24|COL_WHITE, 127<<24|COL_BLUE_SEA_TINT);
					my_free(str);
				}
			}

		}else if (panelImgAttributeCheck(panel, data1, PANEL_ITEM_SEPARATOR) > 0){
			if (data1 == expan->panelIds[PANEL_SID_DRIVES] || data1 == expan->panelIds[PANEL_SID_DIRS] || data1 == expan->panelIds[PANEL_SID_FILES] || data1 == expan->panelIds[PANEL_SID_REMOTE]){
				TPANELSEPARATOR *ps = panelImgStorageGet(panel, data1);
				if (ps){
					ps->state ^= 0x01;
					//printf("state %i\n",  ps->state);

					TPANELIMG *img = panelImgGetItem(panel, data1);
					if (!ps->state)
						button2FaceActiveSet(img->btn, BUTTON_SEC);
					else
						button2FaceActiveSet(img->btn, BUTTON_PRI);

					expanPanelChildrenSetState(panel, data1, ps->state);

					TMETRICS metrics = {0, 0, panel->metrics.width, panel->metrics.height};
					panel->vHeight = panelImgPositionMetricsCalc(panel->list, panel->listSize, &metrics, panel->itemHoriSpace, panel->itemVertSpace);
					renderSignalUpdate(panel->cc->vp);
				}
			}
		}
	}
	
	return 1;
}


static inline int expanPanButtonPress (TEXPPANEL *expan, TCCBUTTON *btn, const int btn_id, const TTOUCHCOORD *pos)
{
	TPANEL *panel = expan->panel;
	panel->btns->t0 = getTickCount();
	TFILEPANE *filepane = pageGetPtr(btn->cc->vp, PAGE_FILE_PANE);

	switch (btn_id){
	  case PANEL_BUTTON_AUDIO:
	  	if (hasPathExt(expan->selectedFile, extAudio)){
	  		char *filename = convertto8(expan->selectedFile);
	  		if (filename){
	  			filepanePlay_DoPlay(filepane, filename);
	  			my_free(filename);
	  		}
	  	}
		return 1;

	  case PANEL_BUTTON_VIDEO:
	  	if (hasPathExt(expan->selectedFile, extVideo)){
	  		char *filename = convertto8(expan->selectedFile);
	  		if (filename){
	  			filepanePlay_DoPlay(filepane, filename);
	  			my_free(filename);
	  		}
	  	}
		return 1;

	  case PANEL_BUTTON_IMAGE:
	  	if (hasPathExt(expan->selectedFile, extImage)){
	  		filepaneViewImage(filepane, expan->selectedFile);
	  	}
		return 1;

	  case PANEL_BUTTON_PLAYLIST:
	  	if (hasPathExt(expan->selectedFile, extPlaylists)){
	  		char *filename = convertto8(expan->selectedFile);
	  		if (filename){
	  			filepanePlay_loadPlaylist(filepane, NULL, filename);
	  			my_free(filename);
	  		}
			expan->selectedFile[0] = 0;
	  	}
		return 1;

	  case PANEL_BUTTON_FOLDERI:
		if (isDirectoryW(expan->selectedFile)){
			expanPanelDisableModuleButtons(panel);

			char *name = convertto8(expan->selectedFile);
			if (!name) return 0;

			TVLCPLAYER *vp = panel->cc->vp;
			PLAYLISTCACHE *plc = playlistManagerCreatePlaylist(vp->plm, name, 0);
			playlistAddPlc(getPrimaryPlaylist(vp), plc);

			int count = filepaneBuildPlaylistDir(filepane, plc, expan->selectedFile, FILEMASKS_MEDIA, 1);
			
			dbprintf(vp, " %i items added to %s", count, name);
			my_free(name);

		  	setDisplayPlaylist(vp, plc);

		  	if (plc->parent)
		  		plc->parent->pr->selectedItem = playlistManagerGetPlaylistIndex(vp->plm, plc);

			playlistChangeEvent(vp, plc, 0);
			//pageSet(vp, PAGE_PLY_SHELF);
			page2Set(filepane->com->pages, PAGE_PLY_SHELF, 1);
			*expan->selectedFile = 0;
		}
		return 1;

	  case PANEL_BUTTON_MODULE:{
  		char *filename = convertto8(expan->selectedFile);
  		if (filename){
  			filepanePlay_DoPlay(filepane, filename);
  			my_free(filename);
  		}
		return 1;
	  }
	  case PANEL_BUTTON_BACK:
	  	expanPanelDisableModuleButtons(panel);
	  	buttonsStateSet(panel->btns, PANEL_BUTTON_REFRESH, 1);
		buttonsStateSet(panel->btns, PANEL_BUTTON_BACK, 1);
		page2SetPrevious(expan);
		//page2RenderDisable(panel->cc->vp->pages, PAGE_FILE_PANE);

		return 1;

	  case PANEL_BUTTON_REFRESH:
		expanPanelDisableModuleButtons(panel);
		buttonsStateSet(panel->btns, PANEL_BUTTON_REFRESH, 1);
		buttonsStateSet(panel->btns, PANEL_BUTTON_BACK, 1);
	  	timerSet(panel->cc->vp, TIMER_EXPPAN_REBUILD, 0);
	    return 1;
	}

	return 0;
}


static inline int64_t expan_btn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;

	TCCBUTTON *btn = (TCCBUTTON*)object;

	if (msg == BUTTON_MSG_SELECTED_PRESS){
		return expanPanButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	}
	return 1;
}

static inline void driveSpaceFormat (char *buffer, const int drive, const uint64_t free64, const uint64_t total64)
{
	double free = free64/1024.0/1024.0/1024.0;
	double total = total64/1024.0/1024.0/1024.0;
	char format[32]; // =  "%s  (%.1f : %.1f)";

	format[0] = 0;
	strcat(format, "(%c: ");
	if (free >= 99)
		strcat(format, "%.0f/");
	else if (free < 1){
		strcat(format, "%3.0fm/");
		free = free64/1024.0/1024.0;
	}else if (free < 10)
		strcat(format, "%.2f/");
	else
		strcat(format, "%.1f/");
		
	if (total >= 99)
		strcat(format, "%.0f)");
	else if (total < 1){
		strcat(format, "%3.0fm)");
		total = total64/1024.0/1024.0;
	}else if (total < 10)
		strcat(format, "%.2f)");
	else
		strcat(format, "%.1f)");

	__mingw_snprintf(buffer, MAX_PATH, format, drive&0xFF, free, total);
}

int expanPanelBuild (TEXPPANEL *expan, TPANEL *panel, TLOGICALDRIVE *drives, const int dTotal)
{
	int sbid1 = 0, sbid2 = 0, sbid3 = 0, sbid4 = 0;
	
	int tShortcuts = fbShortcutsGetTotal(&expan->userLinks);
	
	//printf("## expanPanelBuild %i\n", dTotal);
	
	int remoteDriveTotal = 0;
	for (int i = 0; i < expan->logicalDriveTotal; i++)
		remoteDriveTotal += (expan->drives[i].driveType == DRIVE_REMOTE);

	if (expan->logicalDriveTotal){
		sbid1 = panelImgAddEx(panel, genAMId(panel,L"exppanel/vsepbar_dn.png"), genAMId(panel,L"exppanel/vsepbar_up.png"), 1.0, "Drives", expan);
		TPANELSEPARATOR *psDrive = expanPanelSeparatorCreate();
		panelImgStorageSet(panel, sbid1, psDrive);
		panelImgAttributeSet(panel, sbid1, PANEL_ITEM_SEPARATOR);

		for (intptr_t i = 0; i < expan->logicalDriveTotal; i++){
			const int driveType = expan->drives[i].driveType;
			if (driveType == DRIVE_REMOTE) continue;

			char *label = fbGetVolumeLabel(expan->drives[i].drive[0]);
			if (!label)
				label = my_strdup(expan->drives[i].drive);

			int imgid;
			if (driveType == DRIVE_FIXED){
				//printf("drive fixed %c %i\n", expan->drives[i].drive[0], expan->drives[i].isSystemDrive);
				const int busType = expan->drives[i].busType;
				if (busType == BusTypeSata){
					if (expan->drives[i].isSystemDrive)
						imgid = genAMId(panel,L"exppanel/ossata.png");
					else
						imgid = genAMId(panel,L"exppanel/sata.png");
				}else if (busType == BusTypeAta){
					if (expan->drives[i].isSystemDrive)
						imgid = genAMId(panel,L"exppanel/osata.png");
					else
						imgid = genAMId(panel,L"exppanel/ata.png");
				}else if (busType == BusType1394){
					imgid = genAMId(panel,L"exppanel/firewire.png");
				}else if (busType == BusTypeSd){
					imgid = genAMId(panel,L"exppanel/ssd.png");
				}else if (busType == BusTypeFileBackedVirtual || busType == BusTypeVirtual){
					imgid = genAMId(panel,L"exppanel/virtual.png");
				}else if (expan->drives[i].isSystemDrive){
					imgid = genAMId(panel,L"exppanel/osfixed.png");
				}else{
					imgid = genAMId(panel,L"exppanel/fixed.png");
				}
			}else if (driveType == DRIVE_USB){
				if (expan->drives[i].totalNumberOfBytes)
					imgid = genAMId(panel,L"exppanel/usb.png");
				else
					imgid = genAMId(panel,L"exppanel/usbnomedia.png");
			}else if (driveType == DRIVE_REMOVABLE){
				if (expan->drives[i].totalNumberOfBytes)
					imgid = genAMId(panel,L"exppanel/removable.png");
				else
					imgid = genAMId(panel,L"exppanel/removablenomedia.png");
			}else if (driveType == DRIVE_CDROM){
				imgid = genAMId(panel,L"exppanel/cdrom.png");
			}else if (driveType == DRIVE_RAMDISK){
				imgid = genAMId(panel,L"exppanel/ramdisk.png");
			}else{
				imgid = genAMId(panel,L"exppanel/module.png");
			}
				
			int id = panelImgAdd(panel, imgid, label, (void*)i);
			char buffer[MAX_PATH+1];
			int showDriveFreeSpace = 0;
			settingsGet(expan->com->vp, "browser.showDriveFreeSpace", &showDriveFreeSpace);
			
			if (showDriveFreeSpace && expan->drives[i].totalNumberOfBytes)
				driveSpaceFormat(buffer, expan->drives[i].drive[0], expan->drives[i].totalNumberOfFreeBytes, expan->drives[i].totalNumberOfBytes);
			else
				sprintf(buffer, "(%c:)", expan->drives[i].drive[0]);
			panelImgAddSubtext(panel, id, buffer, 245<<24|COL_WHITE);

			expanPanelSeparatorIdAdd(psDrive, id);
			panelImgAttributeSet(panel, id, PANEL_ITEM_DRIVE);
			my_free(label);
		}
	}

	if (remoteDriveTotal){
		sbid2 = panelImgAddEx(panel, genAMId(panel,L"exppanel/vsepbar_dn.png"), genAMId(panel,L"exppanel/vsepbar_up.png"), 1.0, "Remote", expan);
		TPANELSEPARATOR *psDrive = expanPanelSeparatorCreate();
		panelImgStorageSet(panel, sbid2, psDrive);
		panelImgAttributeSet(panel, sbid2, PANEL_ITEM_SEPARATOR);

		char buffer[MAX_PATH+1];
		int showRemotePath = 0;
		int showRemoteComputer = 0;
		settingsGet(expan->com->vp, "browser.showRemotePath", &showRemotePath);
		settingsGet(expan->com->vp, "browser.showRemoteComputer", &showRemoteComputer);

		for (intptr_t i = 0; i < expan->logicalDriveTotal; i++){
			if (expan->drives[i].driveType != DRIVE_REMOTE) continue;
			sprintf(buffer, "(%c:)", expan->drives[i].drive[0]);
			
			char *label = regGetDriveNetworkLocation8(expan->drives[i].drive[0]);
			if (label){
				TDECOMPOSEPATH *loc = decomposePath(label);
				if (loc){
					if (loc->total > 1){
						if (!showRemotePath){
							my_free(label);
							label = my_strdup(loc->dirs[loc->total-1].folder);
							if (!label) label = my_strdup(expan->drives[i].drive);
						}
						if (showRemoteComputer)
							sprintf(buffer, "%s", loc->dirs[1].folder);
					}
					decomposePathFree(loc);
				}
			}else{
				label = my_strdup(expan->drives[i].drive);
			}

			int id = panelImgAdd(panel, genAMId(panel,L"exppanel/remote.png"), label, (void*)i);
			panelImgAddSubtext(panel, id, buffer, 245<<24|COL_WHITE);
			expanPanelSeparatorIdAdd(psDrive, id);
			panelImgAttributeSet(panel, id, PANEL_ITEM_DRIVE);
			my_free(label);
		}
	}
	
	if (expan->shellFoldersTotal){
		sbid3 = panelImgAddEx(panel, genAMId(panel,L"exppanel/vsepbar_dn.png"), genAMId(panel,L"exppanel/vsepbar_up.png"), 1.0, "Links", expan);
		TPANELSEPARATOR *psLinks = expanPanelSeparatorCreate();
		panelImgStorageSet(panel, sbid3, psLinks);
		panelImgAttributeSet(panel, sbid3, PANEL_ITEM_SEPARATOR);

		for (intptr_t i = 0; i < expan->shellFoldersTotal; i++){
			char *name = convertto8(expan->shellFolders[i].name);
			if (name){
				int id = panelImgAdd(panel, genAMId(panel,L"exppanel/folder.png"), name, (void*)i);
				expanPanelSeparatorIdAdd(psLinks, id);
				panelImgAttributeSet(panel, id, PANEL_ITEM_LINK);
				my_free(name);
			}

		}

		for (intptr_t i = 0; i < tShortcuts; i++){
			TSTRSHORTCUT *sc = fbShortcutsGet(&expan->userLinks, i);
			if (sc->type == SYMLINK_SHORTCUT){
				//addLocationShortcut(path, sc->path, sc->link);
				char *name = convertto8(sc->link);
				if (name){
					int id = panelImgAdd(panel, genAMId(panel,L"exppanel/folder.png"), name, (void*)(i|16384));
					expanPanelSeparatorIdAdd(psLinks, id);
					panelImgAttributeSet(panel, id, PANEL_ITEM_LINK);
					my_free(name);
				}
			}

		}

	}


	if (tShortcuts){
		sbid4 = panelImgAddEx(panel, genAMId(panel,L"exppanel/vsepbar_dn.png"), genAMId(panel,L"exppanel/vsepbar_up.png"), 1.0, "Modules", exp);
		TPANELSEPARATOR *psModules = expanPanelSeparatorCreate();
		panelImgStorageSet(panel, sbid4, psModules);
		panelImgAttributeSet(panel, sbid4, PANEL_ITEM_SEPARATOR);
		
		for (intptr_t i = 0; i < tShortcuts; i++){
			TSTRSHORTCUT *sc = fbShortcutsGet(&expan->userLinks, i);
			
			if (sc->type == SYMLINK_MODULE){
				char *name = convertto8(sc->link);
				if (name){
					int id;
					if (isDirectoryW(sc->path))
						id = panelImgAdd(panel, genAMId(panel,L"exppanel/folder_import.png"), name, (void*)i);
					else if (hasPathExt(sc->path, extAudio))
						id = panelImgAdd(panel, genAMId(panel,L"exppanel/audio.png"), name, (void*)i);
					else if (hasPathExt(sc->path, extVideo))
						id = panelImgAdd(panel, genAMId(panel,L"exppanel/video.png"), name, (void*)i);
					else if (hasPathExt(sc->path, extImage))
						id = panelImgAdd(panel, genAMId(panel,L"exppanel/image.png"), name, (void*)i);
					else if (hasPathExt(sc->path, extPlaylists))
						id = panelImgAdd(panel, genAMId(panel,L"exppanel/playlist.png"), name, (void*)i);
					else
						id = panelImgAdd(panel, genAMId(panel,L"exppanel/module.png"), name, (void*)i);
					my_free(name);


					expanPanelSeparatorIdAdd(psModules, id);
					panelImgAttributeSet(panel, id, PANEL_ITEM_MODULE);

					TPANELIMG *img = panelImgGetItem(panel, id);
					button2AnimateSet(img->btn, 1);
				}
			}
		}
	}

	int showLocalComputerName = 0;
	settingsGet(expan->com->vp, "browser.showLocalComputerName", &showLocalComputerName);
	if (showLocalComputerName){
		char buffer[MAX_PATH_UTF8+1];
		char *computerName = fbGetComputerName();
		char *myComputerName = fbGetMyComputerName();
		
		if (computerName && myComputerName){
			__mingw_snprintf(buffer, MAX_PATH_UTF8, "%s - %s", computerName, myComputerName);
			TCCBUTTON *btn = buttonsButtonGet(panel->btns, PANEL_BUTTON_REFRESH);
			buttonFaceTextUpdate(btn, BUTTON_PRI, buffer);
			
			my_free(computerName);
			my_free(myComputerName);
		}
	}else{
		char *myComputerName = fbGetMyComputerName();
		if (myComputerName){
			TCCBUTTON *btn = buttonsButtonGet(panel->btns, PANEL_BUTTON_REFRESH);
			buttonFaceTextUpdate(btn, BUTTON_PRI, myComputerName);
			my_free(myComputerName);
		}
	}

	expan->panelIds[PANEL_SID_DRIVES] = sbid1;
	expan->panelIds[PANEL_SID_REMOTE] = sbid2;
	expan->panelIds[PANEL_SID_DIRS]   = sbid3;
	expan->panelIds[PANEL_SID_FILES]  = sbid4;
	
	return panelImgGetTotal(panel);
}

int expanGetPathObjectTotal (TEXPPANEL *expan)
{
	return expan->logicalDriveTotal + expan->shellFoldersTotal + expan->userLinks.total;
}

int expanPanelRebuild (TEXPPANEL *expan, TPANEL *panel)
{
	//printf("expanPanelRebuild\n");
	
	for (int i = 0; i < PANEL_SID_TOTAL && expan->panelIds[i]; i++){
		TPANELSEPARATOR *ps = panelImgStorageGet(panel, expan->panelIds[i]);
		expanPanelSeparatorIdClearAll(ps);
	}
	int total = expanGetPathObjectTotal(expan);
	//if (!total) printf("expanPanelRebuild zero object total %s:%i\n",__FILE__,__LINE__);

	panelListResize(panel, total, 0);
	int ret = expanPanelBuild(expan, panel, expan->drives, expan->logicalDriveTotal);
	panelInvalidate(panel);
	return ret;
}

// TIMER_EXPPAN_REBUILD
void expanTimerPanelRebuild (TVLCPLAYER *vp)
{
	//printf("expanTimerPanelRebuild\n");
	
	TEXPPANEL *expan = pageGetPtr(vp, PAGE_EXP_PANEL);
	if (ccLock(expan->panel)){
		fbGetLogicalDrivesRelease(expan->drives);
		expan->drives = fbGetLogicalDrives(&expan->logicalDriveTotal);

		expanPanelRebuild(expan, expan->panel);
		ccUnlock(expan->panel);
	}
}

// TIMER_EXPPAN_REBUILDSETPAGE
void expanTimerPanelRebuildSetPage (TVLCPLAYER *vp)
{
	expanTimerPanelRebuild(vp);
	page2Set(vp->pages, PAGE_EXP_PANEL, 1);
}

static inline int page_expPanRender (TEXPPANEL *expan, TVLCPLAYER *vp, TFRAME *frame)
{
	TPANEL *panel = expan->panel;
	ccRender(panel, frame);
	
	//lSetRenderEffect(frame->hw, LTR_DEFAULT);
	const int posX = panel->metrics.x + panel->itemOffset->x;
	const int posY = panel->metrics.y - panel->itemOffset->y;
	const T4POINT constraint = {panel->metrics.x, panel->metrics.y, panel->metrics.x+panel->metrics.width-1, panel->metrics.y+panel->metrics.height-1};


	// draw seperator names
	for (int i = 0; i < PANEL_SID_TOTAL; i++){
		if (!expan->panelIds[i]) continue;
		
		TPANELIMG *img = panelImgGetItem(panel, expan->panelIds[i]);
		TPANELLABEL *label = &img->labels[0];

		if (!label->strFrm){
			outlineTextEnable(frame->hw, 240<<24|COL_BLUE_SEA_TINT);
			label->strFrm = lNewString(frame->hw, frame->bpp, PF_CLIPDRAW|PF_IGNOREFORMATTING, BROWSER_PANEL_SEP_FONT, label->text);
			outlineTextDisable(frame->hw);	
		}
		if (label->strFrm){
			TFRAME *text = label->strFrm;
			int x = (frame->width - text->width) - 8;
			int y = img->area.y1 - 12;

			T4POINT pos = {posX+x, posY+y, posX+x+text->width-1, posY+y+text->height-1};

			x = 0, y = 0;
			if (pos.x1 < constraint.x1){
				x = abs(constraint.x1 - pos.x1);
				pos.x1 += abs(constraint.x1 - pos.x1);
							}
			if (pos.y1 < constraint.y1){
				y = abs(constraint.y1 - pos.y1);
				pos.y1 += abs(constraint.y1 - pos.y1);
			}
			if (pos.x2 > constraint.x2)
				pos.x2 -= abs(constraint.x2 - pos.x2);
			if (pos.y2 > constraint.y2)
				pos.y2 -= abs(constraint.y2 - pos.y2);

			copyArea(text, frame, pos.x1, pos.y1, x, y, (pos.x2 - pos.x1), y+(pos.y2 - pos.y1));
		}
	}
	
	
	return 1;
}

static inline int page_expPanInput (TEXPPANEL *expan, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
	  case PAGE_IN_WHEEL_BACK:{
	  	TTOUCHSWIPE *swipe = &expan->panel->swipe;
	  	swipeReset(swipe);
	  	
	  	swipe->t0 = getTime(vp)-100.0;
	  	swipe->i32value = expan->panel->itemOffset->y;
	  	swipe->sx = 10;
	  	swipe->sy = 150;
	  	swipe->ex = 10;
		swipe->ey = swipe->sy-25;
		swipe->dx = swipe->ex - swipe->sx;
		if (msg == PAGE_IN_WHEEL_FORWARD)
			swipe->dy = -(swipe->ey - swipe->sy);
		else
			swipe->dy = swipe->ey - swipe->sy;
		
		swipe->state = 0;
	  	
	  	swipe->adjust = swipe->dy;
	  	
	  	swipe->dt = 100.0;
		swipe->decayAdjust = abs(swipe->dy)/(double)swipe->dt;
		swipe->decayAdjust *= swipe->velocityFactor;
	  	swipe->state = 0;

	  	break;
	  }
	 /* case PAGE_IN_TOUCH_DOWN:
	  case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:*/
	}
		
	return 1;
}


static inline int buildExpanderBar (TPANEL *panel, const int fw, wchar_t *srcImg)
{
	int width, height;
	int id = genAMId(panel, srcImg);
	if (id && artManagerImageGetMetrics(panel->cc->vp->am, id, &width, &height)){
		if (width != fw)
			artManagerImageResize(panel->cc->vp->am, id, fw, height);
	}
	return id;
}

static inline int page_expPanStartup (TEXPPANEL *expan, TVLCPLAYER *vp, const int fw, const int fh)
{


	expan->shellFolders = fbGetShellFolders(&expan->shellFoldersTotal);
	expan->drives = fbGetLogicalDrives(&expan->logicalDriveTotal);
	
	TPANEL *panel = ccCreate(vp->cc, PAGE_EXP_PANEL, CC_PANEL, expan_panel_cb, &vp->gui.ccIds[CCID_PANEL_EXPLORER], PANEL_BUTTON_TOTAL, 0);
	expan->panel = panel;
	ccSetUserData(panel, expan);
	panel->itemOffset = &expan->itemOffset;
	panelSetBoundarySpace(panel, 10, 24);
	//int y = (fh * 0.122)+1;
	int y = 64;
	ccSetMetrics(panel, 0, y, fw, fh - y);
	
	buildExpanderBar(panel, fw, L"exppanel/vsepbar_up.png");
	buildExpanderBar(panel, fw, L"exppanel/vsepbar_dn.png");
	
	expanPanelBuild(expan, panel, expan->drives, expan->logicalDriveTotal);
	panelInvalidate(panel);
	
	
	TCCBUTTON *btn;
	btn = panelSetButton(panel, PANEL_BUTTON_AUDIO, L"exppanel/audio_sm.png", NULL, expan_btn_cb);
	buttonFaceTextSet(btn, BUTTON_PRI, " ", PF_CLIPWRAP, MFONT, PANEL_MODULE_TEXT_POSX, PANEL_MODULE_TEXT_POSY);
	ccSetPosition(btn, 0, 0);
	ccDisable(btn);
	
	btn = panelSetButton(panel, PANEL_BUTTON_VIDEO, L"exppanel/video_sm.png", NULL, expan_btn_cb);
	buttonFaceTextSet(btn, BUTTON_PRI, " ", PF_CLIPWRAP, MFONT, PANEL_MODULE_TEXT_POSX, PANEL_MODULE_TEXT_POSY);
	ccSetPosition(btn, 0, 0);
	ccDisable(btn);

	btn = panelSetButton(panel, PANEL_BUTTON_IMAGE, L"exppanel/image_sm.png", NULL, expan_btn_cb);
	buttonFaceTextSet(btn, BUTTON_PRI, " ", PF_CLIPWRAP, MFONT, PANEL_MODULE_TEXT_POSX, PANEL_MODULE_TEXT_POSY);
	ccSetPosition(btn, 0, 0);
	ccDisable(btn);

	btn = panelSetButton(panel, PANEL_BUTTON_PLAYLIST, L"exppanel/playlist_sm.png", NULL, expan_btn_cb);
	buttonFaceTextSet(btn, BUTTON_PRI, " ", PF_CLIPWRAP, MFONT, PANEL_MODULE_TEXT_POSX, PANEL_MODULE_TEXT_POSY);
	ccSetPosition(btn, 0, 0);
	ccDisable(btn);

	btn = panelSetButton(panel, PANEL_BUTTON_FOLDERI, L"exppanel/folder_import_sm.png", NULL, expan_btn_cb);
	buttonFaceTextSet(btn, BUTTON_PRI, " ", PF_CLIPWRAP, MFONT, PANEL_MODULE_TEXT_POSX, PANEL_MODULE_TEXT_POSY);
	ccSetPosition(btn, 0, 0);
	ccDisable(btn);

	btn = panelSetButton(panel, PANEL_BUTTON_MODULE, L"exppanel/module_sm.png", NULL, expan_btn_cb);
	buttonFaceTextSet(btn, BUTTON_PRI, " ", PF_CLIPWRAP, MFONT, PANEL_MODULE_TEXT_POSX, PANEL_MODULE_TEXT_POSY);
	ccSetPosition(btn, 0, 0);
	ccDisable(btn);

	btn = panelSetButton(panel, PANEL_BUTTON_BACK, L"common/back_right96.png", NULL, expan_btn_cb);
	ccSetPosition(btn, fw - ccGetWidth(btn)-1, 0);
	ccEnable(btn);

	btn = panelSetButton(panel, PANEL_BUTTON_REFRESH, L"exppanel/refresh.png", NULL, expan_btn_cb);
	ccSetMetrics(btn,-1, -1, 590, -1);
	ccSetPosition(btn, (fw - ccGetWidth(btn))/2, 4);
	buttonFaceTextSet(btn, BUTTON_PRI, "      ", PF_MIDDLEJUSTIFY, LFONT, 0, 0);
	labelStringSetMaxWidth(btn->btnlbl[BUTTON_PRI]->label, btn->btnlbl[BUTTON_PRI]->itemStrId, ccGetWidth(btn)-2);
	//fix me
	labelRenderBlurRadiusSet(btn->btnlbl[BUTTON_PRI]->label, btn->btnlbl[BUTTON_PRI]->itemStrId, 3);
	labelRenderFilterSet(btn->btnlbl[BUTTON_PRI]->label, btn->btnlbl[BUTTON_PRI]->itemStrId, 2);
	labelRenderColourSet(btn->btnlbl[BUTTON_PRI]->label, btn->btnlbl[BUTTON_PRI]->itemStrId, 255<<24|COL_WHITE, 255<<24|COL_BLUE_SEA_TINT, 255<<24|COL_BLUE_SEA_TINT);
	buttonFaceHoverSet(btn, 0, 0, 0);
	ccEnable(btn);	
		
	ccDisable(panel);
	
	return 1;
}

static inline int page_expPanInitalize (TEXPPANEL *expan, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_EXP_PANEL);
	
	str_list *strList = NULL;
	settingsGet(vp, "browser.shortcut.", &strList);
	if (strList){
		for (int i = 0; i < strList->total; i++){
			char *name = cfg_configStrListItem(strList, i);
			if (name && strlen(name) > 5){
				char *path = strstr(name, CFG_PATHSEPARATOR);
				if (path && strlen(path) > 4){
					*path = 0;
					path += 3;
					if (*path){
						fbShortcutsAdd8(&expan->userLinks, path, name);
					}
				}
			}
		}
		cfg_configStrListFree(strList);
		my_free(strList);
	}
	
	strList = NULL;
	settingsGet(vp, "browser.module.", &strList);
	if (strList){
		for (int i = 0; i < strList->total; i++){
			char *name = cfg_configStrListItem(strList, i);
			if (name && strlen(name) > 4){
				char *module = strstr(name, CFG_PATHSEPARATOR);
				if (module && strlen(module) > 3){
					*module = 0;
					module += 3;
					if (*module){
						fbShortcutAddModule(&expan->userLinks, module, name);
					}
				}
			}
		}
		cfg_configStrListFree(strList);
		my_free(strList);
	}
	
	return 1;
}

static inline int page_expPanShutdown (TEXPPANEL *expan, TVLCPLAYER *vp)
{
	fbGetShellFoldersRelease(expan->shellFolders);
	fbGetLogicalDrivesRelease(expan->drives);
	fbShortcutsFree(&expan->userLinks);
	ccDelete(expan->panel);
	
	return 1;
}

int page_expPanCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TEXPPANEL *expan = (TEXPPANEL*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
	//	 printf("# page_expPanCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_expPanRender(expan, expan->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_expPanInput(expan, expan->com->vp, dataInt1, dataInt2, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		ccEnable(expan->panel);
	}else if (msg == PAGE_CTL_RENDER_END){
		ccDisable(expan->panel);
		
	}else if (msg == PAGE_CTL_RENDER_INIT){
		expanTimerPanelRebuild(expan->com->vp);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_expPanStartup(expan, expan->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_expPanInitalize(expan, expan->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_expPanShutdown(expan, expan->com->vp);
		
	}
	
	return 1;
}

