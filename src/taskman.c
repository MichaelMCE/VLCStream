
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


#define TASKMAN_MENU_BASEIDX			8192
#define TASKMAN_MENU_ENDPROCESS			(TASKMAN_MENU_BASEIDX+1)
#define TASKMAN_MENU_ENDPROCESS_YES		(TASKMAN_MENU_BASEIDX+2)
#define TASKMAN_MENU_ENDPROCESS_NO		(TASKMAN_MENU_BASEIDX+3)
#define TASKMAN_MENU_OPENLOCATION		(TASKMAN_MENU_BASEIDX+4)


#define TASKMAN_PROCESS_FONT			LFTW_B24
#define TASKMAN_PANE_SCROLL_DELTA		40

#define TASKMAN_CONTEXT_LINEHEIGHT		24
#define TASKMAN_CONTEXT_SCROLL_DELTA	TASKMAN_CONTEXT_LINEHEIGHT

#define	TASKMAN_PATH_INVALID		0
#define	TASKMAN_PATH_SYSTEM32		1			//	Windows/System32/
#define	TASKMAN_PATH_SYSTEM64		2			//	Windows/SysWOW64/
#define	TASKMAN_PATH_WINDOWS		3			//	Windows/
#define	TASKMAN_PATH_PARTIAL		4			//	match by imagename only (eg; '\firefox.exe')
#define	TASKMAN_PATH_LOGICAL		5			//	exact path match
#define	TASKMAN_PATH_PROCESS		6			//	hardcoded by process id ('Idle' and 'System')



extern int SHUTDOWN;


typedef struct {
	int type;
	union {
		int pid;
		wchar_t *path;
	}u;
	wchar_t *image;
}tm_icon;

#define MAKE_PATH(a,b,c)		{(a),{.path=L"\\"b""}, L""c""}
#define MAKE_PARTIAL(b,c)		{(TASKMAN_PATH_PARTIAL),{.path=L"\\"b""},L""c""}
#define MAKE_LOGICAL(b,c)		{(TASKMAN_PATH_LOGICAL),{.path=L""b""},L""c""}
#define MAKE_PROCESS(b,c)		{(TASKMAN_PATH_PROCESS),{.pid=(b)},L""c""}
#define MAKE_END()				{(TASKMAN_PATH_INVALID),{.path=NULL},NULL}


static const tm_icon tm_icons[] = {
	MAKE_PROCESS(0, "idle.png"),
	MAKE_PROCESS(4, "system.png"),
	MAKE_PATH(TASKMAN_PATH_WINDOWS,  "explorer.exe",					"explorer.png"),
	MAKE_PATH(TASKMAN_PATH_WINDOWS,  "regedit.exe",						"regedit.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "rundll32.exe",					"dllhost.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "dllhost.exe",						"dllhost.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM64, "dllhost.exe",						"dllhost.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM64, "fontview.exe",					"fontview.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM64, "mmc.exe",							"mmc.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "mmc.exe",							"mmc.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "fontview.exe",					"fontview.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "cmd.exe",							"cmd.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "conhost.exe",						"conhost.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "msiexec.exe",						"msiexec.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "SndVol.exe",						"SndVol.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "services.exe",					"services.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "winlogon.exe",					"winlogon.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "wininit.exe",						"wininit.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "taskmgr.exe",						"taskmgr.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "DeviceDisplayObjectProvider.exe",	"devices.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "SystemPropertiesAdvanced.exe",	"SystemProperties.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "notepad.exe",						"notepad.png"),
	MAKE_PATH(TASKMAN_PATH_SYSTEM32, "vds.exe",							"vds.png"),
	MAKE_LOGICAL(					 "M:\\RamDiskTemp\\vlc\\vlc.exe",	"vlc.png"),
	MAKE_PARTIAL(					 "vlc.exe",							"vlc.png"),
	MAKE_PARTIAL(					 "alarm_clock.exe",					"clock.png"),
	MAKE_PARTIAL(					 "SynTPEnh.exe",					"SynTPEnh.png"),
	MAKE_PARTIAL(					 "SynTPHelper.exe",					"SynTPHelper.png"),
	MAKE_PARTIAL(					 "gcc.exe",							"gcc.png"),
	MAKE_PARTIAL(					 "make.exe",						"make.png"),
	MAKE_PARTIAL(					 "procexp.exe",						"procexp.png"),
	MAKE_PARTIAL(					 "uedit32.exe",						"uedit.png"),
	MAKE_PARTIAL(					 "acdsee.exe",						"acdsee.png"),
	MAKE_PARTIAL(					 "photoshop.exe",					"photoshop.png"),
	MAKE_PARTIAL(					 "illustrator.exe",					"illustrator.png"),
	MAKE_PARTIAL(					 "Training Center.exe",				"GarminTC.png"),
	MAKE_PARTIAL(					 "psp.exe",							"paintshoppro.png"),
	MAKE_PARTIAL(					 "firefox.exe",						"firefox.png"),
	MAKE_PARTIAL(					 "lcdmisc64.exe",					"lcdmisc.png"),
	MAKE_PARTIAL(					 "ramdiskws.exe",					"ramdisk.png"),
	MAKE_PARTIAL(					 "7zFM.exe",						"7zip.png"),
	MAKE_PARTIAL(					 "SearchMyFiles.exe",				"search.png"),
	MAKE_PARTIAL(					 "RzAppManager.exe",				"razer.png"),
	MAKE_PARTIAL(					 "RzDKManager.exe",					"razer.png"),

	MAKE_END(),
};




static inline const process_list_extended *taskmanSnaphotGetPrevious (TTASKMAN *taskman)
{
	return taskman->list.previous->ple;
}

static inline const process_list_extended *taskmanSnaphotGetCurrent (TTASKMAN *taskman)
{
	return taskman->list.current->ple;
}

static inline int taskmanContextAddImage (TTASKMAN *taskman, const int imgId, const int mId)
{
	TPANE *pane = taskman->context.pane;

	int itemId = paneImageAdd(pane, imgId, 1.0, PANE_IMAGE_EAST, 0, 0, mId);
	if (itemId){
		//taskman->context.tItems++;
		paneFocusSet(pane, 0);
		taskman->context.imgId = imgId;
		/*int w, h;
		artManagerImageGetMetrics(taskman->am, imgId, &w, &h);
		int x = abs(pageGetSurfaceWidth(taskman) - w) / 2;
		int y = abs(pageGetSurfaceHeight(taskman) - h) / 2;
		ccSetMetrics(pane, x, y, w, h);*/
	}

	return itemId;
}

static inline int taskmanContextAddItem (TTASKMAN *taskman, const char *str, const int imgId, const int mId)
{
	TPANE *pane = taskman->context.pane;

	int itemId = paneTextAdd(pane, 0, 0.0, str, taskman->context.font, mId);
	if (itemId){
		taskman->context.tItems++;
		paneFocusSet(pane, 0);
		const int pageHeight = pageGetSurfaceHeight(taskman);

		int h = (taskman->context.tItems * pane->vertLineHeight) + 2;
		//if (h > ccGetHeight(taskman->pane)-1) h = ccGetHeight(taskman->pane)-1;
		if (h > pageHeight) h = pageHeight-2;
		int w = pane->tItemWidth + 1;
		if (w > ccGetWidth(taskman->pane)-1) w = ccGetWidth(taskman->pane)-1;

		if (!taskman->context.imgId && imgId)
			taskman->context.imgId = imgId;
		if (taskman->context.imgId){
			int iw, ih;
			artManagerImageGetMetrics(taskman->am, taskman->context.imgId, &iw, &ih);
			w = MAX(w, iw); h = MAX(h, ih);
		}
		
		//int x = abs(pageGetSurfaceWidth(taskman) - w) / 2;
		int x = pageGetSurfaceWidth(taskman) - w - 5;
		if (x < 0) x = 0;
		int y = abs(pageHeight - h) / 2;
		if (y < 0) y = 0;
		ccSetMetrics(pane, x, y, w, h);
		//ccSetMetrics(pane, 0, 0, pageGetSurfaceWidth(taskman), pageGetSurfaceHeight(taskman));
	}

	return itemId;
}

static inline void taskmanContextRender (TTASKMAN *taskman, TFRAME *frame)
{
	TPANE *pane = taskman->context.pane;

	ccRender(pane, frame);
}

static inline void taskmanContextShow (TTASKMAN *taskman)
{
	//printf("taskmanContextShow\n");
	
	TPANE *pane = taskman->context.pane;

	ccEnable(pane);
	pageUpdate(taskman);
}

static inline void taskmanContextHide (TTASKMAN *taskman)
{
	//printf("taskmanContextHide %i\n", taskman->selected.itemHighlighted);
	
	TPANE *pane = taskman->context.pane;
	//ccHoverRenderSigEnable(pane->cc, 20.0);
	paneScrollReset(pane);	
	//paneSwipeDisable(pane);
	//paneTextMulityLineDisable(pane);

	ccDisable(pane);
}

static inline int taskmanContextIsVisable (TTASKMAN *taskman)
{
	TPANE *pane = taskman->context.pane;
	
	return ccGetState(pane);
}

static inline void taskmanContextClear (TTASKMAN *taskman)
{
	TPANE *pane = taskman->context.pane;
	
	paneRemoveAll(pane);
	taskman->context.tItems = 0;
	pane->vertLineHeight = TASKMAN_CONTEXT_LINEHEIGHT;
	labelBaseColourSet(pane->base, 220<<24 | COL_BLACK);
	paneSetLayout(pane, PANE_LAYOUT_VERT);
}

static inline void taskmanContextSetPosition (TTASKMAN *taskman, int x, int y)
{
	TPANE *pane = taskman->context.pane;
	
	const int fw = pageGetSurfaceWidth(taskman);
	const int fh = pageGetSurfaceHeight(taskman);
	const int cw = ccGetWidth(pane);
	const int ch = ccGetHeight(pane);
	const int padding = 3;
	
	if (x+cw+padding > fw) x = (fw - cw)-padding;
	if (x < padding) x = padding;
	
	if (y+ch+padding > fh) y = (fh - ch)-padding;
	if (y < padding) y = padding;
	
	ccSetPosition(pane, x, y);
}

static inline const tm_icon *taskmanFindIcon (TTASKMAN *taskman, process_list_extended *ple)
{
	const tm_icon *icon = tm_icons;
	wchar_t *system32 = taskman->folders.system32;
	wchar_t *system64 = taskman->folders.system64;
	wchar_t *windows = taskman->folders.windows;
	wchar_t imageName[MAX_PATH+1];
	
	imageName[0] = 0;
	wcscat(imageName, L"\\");
	wcscat(imageName, ple->info.imageName);
	
	while (icon->type != TASKMAN_PATH_INVALID){
		//if (icon->type != TASKMAN_PATH_PROCESS)
			//wprintf(L"find %i '%s' '%s'\n", icon->type, ple->info.path, icon->u.path);
		//	wprintf(L"find %i '%s' '%s', %i\n", icon->type, ple->info.path, icon->u.path, wcsicmp(ple->info.path, icon->u.path));
		
		if (icon->type == TASKMAN_PATH_PROCESS){
			if (icon->u.pid == ple->info.processId)
				return icon;
		}else if (icon->type == TASKMAN_PATH_SYSTEM32){
			if (wcsistr(ple->info.path, system32) && !wcsicmp(imageName, icon->u.path))
				return icon;
		}else if (icon->type == TASKMAN_PATH_SYSTEM64){
			if (wcsistr(ple->info.path, system64) && !wcsicmp(imageName, icon->u.path))
				return icon;
		}else if (icon->type == TASKMAN_PATH_WINDOWS){
			if (wcsistr(ple->info.path, windows) && !wcsicmp(imageName, icon->u.path))
				return icon;
		}else if (icon->type == TASKMAN_PATH_PARTIAL){
			if (!wcsicmp(imageName, icon->u.path))
				return icon;
		}else if (icon->type == TASKMAN_PATH_LOGICAL){
			if (!wcsicmp(ple->info.path, icon->u.path))
				return icon;
		}
		icon++;
	}
	
	return NULL;
}

static inline wchar_t *taskmanFindNewIcon (TTASKMAN *taskman, process_list_extended *ple)
{
	const tm_icon *icon = taskmanFindIcon(taskman, ple);
	if (icon){
		wchar_t buffer[MAX_PATH+1];
		buildSkinDEx(taskman->com->vp, buffer, L"common\\icons\\", icon->image);
		return my_wcsdup(buffer);
	}
		
	return NULL;
}

static inline ple_snapshot *taskmanSnapshotAlloc ()
{
	return my_calloc(1, sizeof(ple_snapshot));
}

static inline void taskmanSnapshotFree (ple_snapshot *snapshot)
{
	processFreeProcessListExtended(snapshot->ple);
	my_free(snapshot);
}

static inline ple_snapshot *taskmanSnapshotAcquire (TTASKMAN *taskman)
{
	ple_snapshot *snapshot = taskmanSnapshotAlloc();
	if (snapshot){
		snapshot->ple = processGetProcessListExtended(&snapshot->count);
		if (snapshot->ple){
			snapshot->time = snapshot->ple->snapshotTime;
			
			//printf("taskmanSnapshotAcquire total:%i\n", snapshot->count);
			
		}else{
			my_free(snapshot);
			snapshot = NULL;
		}
	}
	return snapshot;
}

static inline void taskmanSnapshotSwap (TTASKMAN *taskman, ple_snapshot *snapshot)
{
	ple_snapshot *old = taskman->list.previous;
	taskman->list.previous = taskman->list.current;
	taskman->list.current = snapshot;
	taskmanSnapshotFree(old);
}

static inline void taskmanPaneClean (TTASKMAN *taskman)
{
	paneRemoveAll(taskman->pane);
}

#if 0
static inline int taskmanPaneFindPid (TTASKMAN *taskman, const int pid)
{
	TPANE *pane = taskman->pane;
	
	//printf("\n\n %i \n\n", paneIndexToItemId(pane, 0));
	
	const int count = pane->flags.total.text;
	for (int i = 0; i < count; i++){
		int itemId = paneIndexToItemId(pane, i);
		if (itemId){
			int64_t itemPid;
			if (paneItemGetData(pane, itemId, &itemPid)){
				if ((int)itemPid == pid)
					return itemId;
			}
		}
	}
	
	return 0;
}
#endif

static inline int taskmanPaneAddSnapshot (TTASKMAN *taskman, ple_snapshot *snapshot, const int byDiff)
{
	//printf("taskmanPaneAddSnapshot snapshot %i\n", snapshot->count);
	
	TPANE *pane = taskman->pane;
	process_list_extended *ple = snapshot->ple;
	int totalAdded = 0;
	char buffer[MAX_PATH_UTF8+1];


	const int count = snapshot->count;
	for (int i = 0; i < count; i++, ple++){
		//printf("proc->info.processId %i %i\n", i, ple->info.processId);
		
		if (byDiff){
			const process_list_extended *previous = processFindProcess(taskmanSnaphotGetPrevious(taskman), ple->info.processId);
			if (previous){
				//int textId = taskmanPaneFindPid(taskman, ple->info.processId);
				int textId = previous->udata;
				if (textId){
					ple->udata = textId;
					//printf("AddProcess: %i %i %i\n", i, process->info.processId, (int)(ple->cpuTime*10));
					
					if ((int)(previous->cpuTime*100.0) != (int)(ple->cpuTime*100.0)){
						//printf("updateProcess: %i %i %i %i\n", i, ple->info.processId, (int)(previous->cpuTime*100) , (int)(ple->cpuTime*100));
						
						if (ple->cpuTime > 0.5)
							__mingw_snprintf(buffer, MAX_PATH_UTF8, "%.1f\n%i\n%s", ple->cpuTime, ple->info.processId, ple->info.imageName8);
						else
							__mingw_snprintf(buffer, MAX_PATH_UTF8, "0\n%i\n%s", ple->info.processId, ple->info.imageName8);				
						paneTextReplace(pane, textId, buffer);
					}
				}
				continue;
			}
		}
		
		//wprintf(L"AddProcess: %i %i '%s'\n", i, ple->info.processId, ple->info.path);

		int imgId = 0;
		wchar_t *image = taskmanFindNewIcon(taskman, ple);
		if (image){
			imgId = artManagerImageAddEx(taskman->am, image, taskman->layout.imageSize, taskman->layout.imageSize);
			my_free(image);
		}else{
			if (doesFileExistW(ple->info.path))
				imgId = artManagerImageAddEx(taskman->am, ple->info.path, taskman->layout.imageSize, taskman->layout.imageSize);
		}

		if (!imgId){
			if (wcsistr(ple->info.path, taskman->folders.system32) || wcsistr(ple->info.path, taskman->folders.system64))
				imgId = taskman->imageIds.sysImg;
			if (!imgId) imgId = taskman->imageIds.noIcon;
		}

		if (imgId){
			if (ple->cpuTime > 0.5)
				snprintf(buffer, MAX_PATH_UTF8, "%.1f\n%i\n%s", ple->cpuTime, ple->info.processId, ple->info.imageName8);
			else
				snprintf(buffer, MAX_PATH_UTF8, "0\n%i\n%s", ple->info.processId, ple->info.imageName8);

			//printf("ple->info.deltaTime %i %I64d\n", i, ple->info.deltaTime);
			
			int id = paneTextAdd(pane, -imgId, 1.0, buffer, TASKMAN_PROCESS_FONT, ple->info.processId);
			if (id > 0){
				ple->udata = id;
				//printf("addedItem for PID %i, = %i\n", ple->info.processId, id);
				labelRenderColourSet(pane->base, id, 255<<24|COL_WHITE, 255<<24|COL_BLACK, 255<<24|COL_BLACK);
				totalAdded++;
			}
			//wprintf(L".. %X %i\n", imgId, totalAdded);
		}
	}
	
	return totalAdded;
}

static inline int taskmanPaneRemoveOldPIDS (TTASKMAN *taskman, ple_snapshot *snapshot)
{
	TPANE *pane = taskman->pane;
	
	int itemsRemoved = 0;
	process_list_extended *ple = snapshot->ple;
	
	for (int i = 0; i < pane->flags.total.text; i++, ple++){
		int itemId = paneIndexToItemId(taskman->pane, i);
		if (itemId){
			int64_t pid;
			if (paneItemGetData(taskman->pane, itemId, &pid)){
				if (!processFindProcess(ple, (int)pid)){
					paneRemoveItem(taskman->pane, itemId);
					itemsRemoved++;
				}
			}
		}
	}
	
	return itemsRemoved;
}
	
static inline void taskmanCPUGraphRender (TTASKMAN *taskman, TFRAME *frame)
{
	TGRAPH *graph = taskman->cpugraph.graph;
	uint64_t t1 = getTime64(taskman->com->vp);
	if (t1 - taskman->cpugraph.lastUpdateTime >= 980){
		taskman->cpugraph.lastUpdateTime = t1;

		if (taskman->cpugraph.mode == 1){
			TGRAPHSHEET *sheet = graphSheetAcquire(graph, NULL, taskman->cpugraph.sheetIds[0]);
			if (sheet){
				graphSheetAddData(sheet, cpuGetProcessorUsage(taskman->com->vp)*20.0, 0);
				graphSheetRelease(sheet);
			}
		}else if (taskman->cpugraph.mode == 2){
			const int count = cpuGetProcessorCount();
			for (int i = 0; i < count; i++){
				TGRAPHSHEET *sheet = graphSheetAcquire(graph, NULL, taskman->cpugraph.sheetIds[i]);
				if (sheet){
					graphSheetAddData(sheet, cpuGetCoreUsage(taskman->com->vp,i)*20.0, 0);
					graphSheetRelease(sheet);
				}
			}
		}
	}
	ccRender(graph, frame);
}

static inline int page_taskmanRender (TTASKMAN *taskman, TFRAME *frame)
{
	//printf("page_taskmanRender %i\n");

	if (taskman->cpugraph.enabled)
		taskmanCPUGraphRender(taskman, frame);

	ccRender(taskman->pane, frame);
	taskmanContextRender(taskman, frame);

	return 1;
}

static inline int page_taskmanRenderInit (TTASKMAN *taskman, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	wchar_t bufferW[MAX_PATH+1];
	buildSkinD(taskman->com->vp, bufferW, L"common\\icons\\defaultapp.png");
	taskman->imageIds.noIcon = artManagerImageAddEx(taskman->am, bufferW, taskman->layout.imageSize, taskman->layout.imageSize);
	buildSkinD(taskman->com->vp, bufferW, L"common\\icons\\systemapp.png");
	taskman->imageIds.sysImg = artManagerImageAddEx(taskman->am, bufferW, taskman->layout.imageSize, taskman->layout.imageSize);

	
	taskman->list.previous = taskmanSnapshotAcquire(taskman);
	taskman->list.current = taskmanSnapshotAcquire(taskman);
	taskmanPaneAddSnapshot(taskman, taskman->list.current, 0);
	
	//ccEnable(taskman->pane);
	return 1;
}

static inline char *taskmanSnapshotGetImageName (TTASKMAN *taskman, const int pid)
{
	const process_list_extended *list = taskmanSnaphotGetCurrent(taskman);
	if (list){
		const process_list_extended *ple = processFindProcess(list, pid);
		if (ple)	
			return convertto8(ple->info.imageName);
	}
	
	return NULL;
}

static inline char *taskmanSnapshotGetPath (TTASKMAN *taskman, const int pid)
{
	const process_list_extended *list = taskmanSnaphotGetCurrent(taskman);
	if (list){
		const process_list_extended *ple = processFindProcess(list, pid);
		if (ple)	
			return convertto8(ple->info.path);
	}
	
	return NULL;
}

static inline wchar_t *taskmanSnapshotGetPathW (TTASKMAN *taskman, const int pid)
{
	const process_list_extended *list = taskmanSnaphotGetCurrent(taskman);
	if (list){
		const process_list_extended *ple = processFindProcess(list, pid);
		if (ple)	
			return my_wcsdup(ple->info.path);
	}
	
	return NULL;
}

static inline void taskmanFormatSize (char *buffer, const uint64_t filesize)
{
	const int len = 32;
	if (filesize >= (uint64_t)100*1024*1024*1024)
		__mingw_snprintf(buffer, len, "%i GB", (int)(filesize/1024/1024/1024));
	else if (filesize >= (uint64_t)1024*1024*1024)
		__mingw_snprintf(buffer, len, "%.1f GB", filesize/1024.0/1024.0/1024.0);
	else if (filesize >= 100*1024*1024)
		__mingw_snprintf(buffer, len, "%i MB", (int)(filesize/1024/1024));
	else if (filesize >= 1024*1024)
		__mingw_snprintf(buffer, len, "%.1f MB", filesize/1024.0/1024.0);
	else if (filesize >= 10*1024)
		__mingw_snprintf(buffer, len, "%i KB", (int)(filesize/1024));
	else if (filesize >= 1024)
		__mingw_snprintf(buffer, len, "%.1f KB", filesize/1024.0);
	else
		__mingw_snprintf(buffer, len, "%i B", (int)(filesize));
}


#define printN(a,b)		{__mingw_snprintf(buffer,sizeof(buffer)-1, ""a": %i", (int)(b));				\
						taskmanContextAddItem(taskman, buffer, 0, 1+taskman->context.tItems);}

#define printTime(a,b)	{ft = *(FILETIME*)&(b);															\
						FileTimeToSystemTime(&ft, &stime);												\
						__mingw_snprintf(buffer, sizeof(buffer)-1, ""a": %.2i:%.2i:%.2i", stime.wHour, stime.wMinute, stime.wSecond);	\
						taskmanContextAddItem(taskman, buffer, 0, 1+taskman->context.tItems);}

#define printTimeD(a,b)	{FileTimeToLocalFileTime((FILETIME*)&(b), &ft);									\
						FileTimeToSystemTime(&ft, &stime);												\
						__mingw_snprintf(buffer,sizeof(buffer)-1, ""a": %.2i:%.2i:%.2i %.2i/%s/%.4i",	\
								stime.wHour, stime.wMinute, stime.wSecond, stime.wDay, clockGetMonthShortname(stime.wMonth), stime.wYear);	\
						taskmanContextAddItem(taskman, buffer, 0, 1+taskman->context.tItems);}

#define printLine()		{taskmanContextAddItem(taskman, " \n", 0, -1);}

#define print64(a,b)	{taskmanFormatSize(bufferSize, b);												\
						__mingw_snprintf(buffer,sizeof(buffer)-1, ""a": %s", bufferSize);				\
						taskmanContextAddItem(taskman, buffer, 0, 1+taskman->context.tItems);}

					
static inline int taskmanContextSetPle (TTASKMAN *taskman, const process_list_extended *ple)
{

	char buffer[MAX_PATH_UTF8+1];
	char bufferSize[64];
	SYSTEMTIME stime;
	FILETIME ft;

	//taskmanContextAddItem(taskman, ple->info.imageName8, 0, 1+taskman->context.tItems);

#if 0
	char *path = convertto8(ple->info.path);
	if (path){
		taskmanContextAddItem(taskman, path, 0, 1+taskman->context.tItems);
		//taskmanContextAddItem(taskman, ple->info.imageName8, 0, 1+taskman->context.tItems);
		my_free(path);
	}
#else
	wchar_t *pathe = ellipsiizeStringPath(ple->info.path, 45);
	if (pathe){
		char *path = convertto8(pathe);
		if (path){
			taskmanContextAddItem(taskman, path, 0, 1+taskman->context.tItems);
			my_free(path);
		}
		my_free(pathe);
	}else{
		char *path = convertto8(ple->info.path);
		if (path){
			taskmanContextAddItem(taskman, path, 0, 1+taskman->context.tItems);
			my_free(path);
		}
	}
#endif


	//printLine();
	printN("PID", ple->info.processId);
	printTime("User time", ple->info.sumTime);
	printTime("Kernel time", ple->info.kernelTime);
	if (ple->info.processId)
		printTimeD("Created", ple->info.createTime);
	printN("Threads", ple->info.totalThreads);
	if (ple->info.inheritedFromProcessId && processFindProcess(ple, ple->info.inheritedFromProcessId))
		printN("Parent Process", ple->info.inheritedFromProcessId);
	printN("TotalHandles", ple->info.totalHandles);
	printN("Priority", ple->info.basePriority);
	printN("Affinity", ple->info.affinity.total);

	//printLine();
	//printTimeD("Created", ple->info.createTime);

	//printLine();
	print64("PeakVirtualSize", ple->info.vm_counters.PeakVirtualSize);
	print64("VirtualSize", ple->info.vm_counters.VirtualSize);
	printN("PageFaultCount", ple->info.vm_counters.PageFaultCount);
	print64("PeakWorkingSetSize", ple->info.vm_counters.PeakWorkingSetSize);
	print64("WorkingSetSize", ple->info.vm_counters.WorkingSetSize);
	print64("QuotaPeakPagedPoolUsage", ple->info.vm_counters.QuotaPeakPagedPoolUsage);
	print64("QuotaPagedPoolUsage", ple->info.vm_counters.QuotaPagedPoolUsage);
	print64("QuotaPeakNonPagedPoolUsage", ple->info.vm_counters.QuotaPeakNonPagedPoolUsage);
	print64("QuotaNonPagedPoolUsage", ple->info.vm_counters.QuotaNonPagedPoolUsage);
	print64("PagefileUsage", ple->info.vm_counters.PagefileUsage);
	print64("PeakPagefileUsage", ple->info.vm_counters.PeakPagefileUsage);

    print64("ReadOperationCount", ple->info.io_counters.ReadOperationCount);
    print64("WriteOperationCount", ple->info.io_counters.WriteOperationCount);
    print64("OtherOperationCount", ple->info.io_counters.OtherOperationCount);
    print64("ReadTransferCount", ple->info.io_counters.ReadTransferCount);
    print64("WriteTransferCount", ple->info.io_counters.WriteTransferCount);
    print64("OtherTransferCount", ple->info.io_counters.OtherTransferCount);

	printLine();
	printLine();
	ccSetUserDataInt(taskman->context.pane, ple->info.processId);
	taskmanContextAddItem(taskman, "Open file location", 0, TASKMAN_MENU_OPENLOCATION);
	printLine();
	printLine();
	printLine();
	taskmanContextAddItem(taskman, "Terminate process", 0, TASKMAN_MENU_ENDPROCESS);
	printLine();
	
	return taskman->context.tItems;
}

static inline int taskmanContextSetPid (TTASKMAN *taskman, const int pid, const int imgId)
{
	ccSetUserDataInt(taskman->pane, pid);
	taskmanContextAddImage(taskman, imgId, pid);
		
	const process_list_extended *ple = processFindProcess(taskmanSnaphotGetCurrent(taskman), pid);
	if (ple)
		taskmanContextSetPle(taskman, ple);
	else
		ccSetUserDataInt(taskman->pane, 0);
	
	return ccGetUserDataInt(taskman->pane);
}

static inline int64_t taskman_pane_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (msg == CC_MSG_RENDER|| msg == CC_MSG_INPUT || msg == CC_MSG_HOVER) return 1;
	
	TPANE *pane = (TPANE*)object;
	
	if (msg == PANE_MSG_TEXT_SELECTED || msg == PANE_MSG_IMAGE_SELECTED){
		const int itemId = data1;
		const int pid = data2;
		int imgId = 0;
		paneItemGetDetail(pane, itemId, NULL, &imgId);
		
		//printf("taskman_pane_cb pid selected: %i %X\n", pid, imgId);
		
		TTASKMAN *taskman = ccGetUserData(pane);
		taskmanContextClear(taskman);
		taskmanContextSetPid(taskman, pid, imgId);
		taskmanContextShow(taskman);

	}else if (msg == PANE_MSG_BASE_SELECTED){
		TTASKMAN *taskman = ccGetUserData(pane);
		ccDisable(taskman->context.pane);
	}

	return 1;
}

static inline void taskmanCenterControl (TTASKMAN *taskman, void *object)
{
	int x = abs(pageGetSurfaceWidth(taskman) - ccGetWidth(object)) / 2;
	int y = abs(pageGetSurfaceHeight(taskman) - ccGetHeight(object)) / 2;
	ccSetPosition(object, x, y);
}

static inline void taskmanContextCreateMenuEndProcess (TTASKMAN *taskman, const int pid)
{
	char buffer[MAX_PATH_UTF8+1];
	
	taskmanContextClear(taskman);
	paneScrollReset(taskman->context.pane);
	ccHoverRenderSigEnable(taskman->com->vp->cc, 20.0);
	taskman->context.font = LFTW_B26;
	taskman->context.pane->vertLineHeight = 30;
	labelBaseColourSet(taskman->context.pane->base, 240<<24 | 0x6F0000);
	paneSetLayout(taskman->context.pane, PANE_LAYOUT_VERTCENTER);


	taskmanContextAddImage(taskman, taskman->context.imgId, pid);

	char *path = taskmanSnapshotGetPath(taskman, pid);
	if (path){
		taskmanContextAddItem(taskman, path, 0, 1+taskman->context.tItems);
		my_free(path);
	}

	printLine();
	__mingw_snprintf(buffer, sizeof(buffer)-1, "Terminate %i?", pid);	
	taskmanContextAddItem(taskman, buffer, 0, 1);
	printLine();
	taskmanContextAddItem(taskman, "  Yes  ", 0, TASKMAN_MENU_ENDPROCESS_YES);
	printLine();
	printLine();
	printLine();
	taskmanContextAddItem(taskman, "  No  ", 0, TASKMAN_MENU_ENDPROCESS_NO);
	printLine();

	taskman->context.font = TASKMAN_PROCESS_FONT;
}

static inline void taskmanContextMenuDoCB (TTASKMAN *taskman, const int menuId, const int pid)
{
	if (menuId == TASKMAN_MENU_ENDPROCESS){
		taskmanContextCreateMenuEndProcess(taskman, pid);
		taskmanCenterControl(taskman, taskman->context.pane);
		taskmanContextShow(taskman);
		
	}else if (menuId == TASKMAN_MENU_ENDPROCESS_YES){
		processKillRemoteThread(pid);
		taskmanContextClear(taskman);
		taskmanContextHide(taskman);
		
	}else if (menuId == TASKMAN_MENU_ENDPROCESS_NO){
		taskmanContextClear(taskman);
		taskmanContextHide(taskman);
		
	}else if (menuId == TASKMAN_MENU_OPENLOCATION){
		wchar_t *path = taskmanSnapshotGetPathW(taskman, pid);
		if (path){
			fbOpenExplorerLocationW(path);
			my_free(path);
		}
	}
}

static inline int64_t taskman_context_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (msg == CC_MSG_RENDER|| msg == CC_MSG_INPUT || msg == CC_MSG_HOVER) return 1;
	
	TPANE *pane = (TPANE*)object;
	
	if (msg == PANE_MSG_TEXT_SELECTED || msg == PANE_MSG_IMAGE_SELECTED){
		TTASKMAN *taskman = ccGetUserData(pane);
		const int itemId = data1;
		const int pid = ccGetUserDataInt(pane);
		const int plePid = ccGetUserDataInt(taskman->pane);
		const int menuId = data2;
		int imgId = 0;
		paneItemGetDetail(pane, itemId, NULL, &imgId);
		//printf("taskman_context_cb pid selected: %i %i %i/%i %X\n", itemId, menuId, plePid, pid, imgId);
		
		if (plePid == pid){
			taskmanContextMenuDoCB(taskman, menuId, pid);
			pageUpdate(taskman);
		}
	}else if (msg == PANE_MSG_BASE_SELECTED){
		TTASKMAN *taskman = ccGetUserData(pane);
		taskmanContextHide(taskman);

	}else if (msg == CC_MSG_DISABLED){
		ccHoverRenderSigDisable(pane->cc);
	}
	
	return 1;
}

static inline int page_taskmanStartup (TTASKMAN *taskman, TVLCPLAYER *vp, const int fw, const int fh)
{
	int paneY = 66;

	settingsGet(vp, "taskman.layout.rows", &taskman->layout.rows);
	settingsGet(vp, "taskman.layout.offset", &taskman->layout.textOffset);
	
	TPANE *pane = ccCreateEx(vp->cc, PAGE_TASKMAN, CC_PANE, taskman_context_cb, NULL, fw/2, fh/2, taskman);
	taskman->context.pane = pane;
	ccSetPosition(pane, 100, 0);
	pane->flags.readAhead.enabled = 0;
	paneSetLayout(pane, PANE_LAYOUT_VERT);
	paneSetAcceleration(pane, 0.0, 0.0);
	paneSwipeEnable(pane);
	paneDragDisable(pane);
	paneTextMulityLineDisable(pane);
	paneTextWordwrapDisable(pane);
	labelRenderFlagsSet(pane->base, LABEL_RENDER_BASE|LABEL_RENDER_HOVER_OBJ|LABEL_RENDER_IMAGE|LABEL_RENDER_TEXT|LABEL_RENDER_BORDER_POST);
	//labelBaseColourSet(pane->base, 80<<24 | COL_BLUE_SEA_TINT);
	labelBaseColourSet(pane->base, 220<<24 | COL_BLACK);
	unsigned int col_post[] = {200<<24 | COL_BLACK, 80<<24 | COL_BLACK};
	labelBorderProfileSet(pane->base, LABEL_BORDER_SET_POST, col_post, 2);
	pane->vertLineHeight = TASKMAN_CONTEXT_LINEHEIGHT;
	taskman->context.font =  TASKMAN_PROCESS_FONT;
	
	
		
	pane = ccCreateEx(vp->cc, PAGE_TASKMAN, CC_PANE, taskman_pane_cb, NULL, fw, fh-paneY, taskman);
	taskman->pane = pane;
	ccSetPosition(pane, 0, paneY);
	paneSetAcceleration(pane, 2.0, 0.0);
	pane->flags.readAhead.enabled = 0;
	taskman->layout.imageSize = ((ccGetHeight(pane)-((taskman->layout.rows-1.0)*2.0)) / (double)taskman->layout.rows);
	pane->horiColumnSpace = 10;
	pane->vertLineHeight = taskman->layout.imageSize + pane->horiColumnSpace;	
	paneSetLayout(pane, PANE_LAYOUT_HORI);
	paneTextMulityLineEnable(pane);
	pane->flags.text.globalOffset.y = taskman->layout.textOffset;

	taskman->am = ccGetImageManager(pane->cc, CC_IMAGEMANAGER_ART);


	return 1;
}


static inline int64_t taskman_graph_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (msg == CC_MSG_RENDER) return 1;
	
	//TGRAPH *graph = (TGRAPH*)object;

	return 1;
}

static inline void taskmanCPUGraphInitalize (TTASKMAN *taskman, TVLCPLAYER *vp, const int width, const int height)
{
	settingsGet(vp, "taskman.cpugraph.enable", &taskman->cpugraph.enabled);
	if (taskman->cpugraph.enabled){
		settingsGet(vp, "taskman.cpugraph.mode", &taskman->cpugraph.mode);
		
		str_list *clrList = NULL;
		settingsGet(vp, "taskman.cpugraph.colour.", &clrList);
		if (clrList){
			for (int i = 0; i < clrList->total; i++)
				taskman->cpugraph.colours[i] = hexToInt(cfg_configStrListItem(clrList,i));
				
			cfg_configStrListFree(clrList);
			my_free(clrList);
		}
		
		int graphHeight = height / 2.4;
		TGRAPH *graph = ccCreate(vp->cc, PAGE_TASKMAN, CC_GRAPH, taskman_graph_cb, NULL, width*0.9, graphHeight);
		taskman->cpugraph.graph = graph;
		ccSetPosition(graph, (width-(width*0.9))/2.0, height-graphHeight-10);
		ccEnable(graph);
		
		int count = cpuGetProcessorCount();
		for (int i = 0; i < count; i++){
			taskman->cpugraph.sheetIds[i] = graphNewSheet(graph, "CPU Usage", 0);
    	
			TGRAPHSHEET *sheet = graphSheetAcquire(graph, NULL, taskman->cpugraph.sheetIds[i]);
			if (sheet){
				//sheet->render.mode = GRAPH_SPECTRUM;
				//sheet->render.mode = GRAPH_BAR;
				//sheet->render.mode |= GRAPH_POINTS;
				//sheet->render.mode = GRAPH_SCOPE;
				//sheet->render.mode |= GRAPH_AUTOSCALE;
				sheet->render.mode = GRAPH_POLYLINE;
				sheet->render.scale = 0.1;
				sheet->stats.min = 0;
				sheet->stats.max = 100;
				//sheet->graph.canGrow = 0;
				//sheet->render.hints = GRAPH_HINT_BASE_FILL|GRAPH_HINT_BASE_BLUR|GRAPH_HINT_BORDER0;
				//sheet->render.palette[GRAPH_PAL_POLYLINE] = 255<<24 | COL_ORANGE;
				//sheet->render.palette[GRAPH_PAL_SCOPE] = 255<<24 | COL_WHITE;
    	
				sheet->render.palette[GRAPH_PAL_POLYLINE] = 255<<24 | taskman->cpugraph.colours[i];
				
				//printf("%p, %i %i %.6X\n", sheet, i, graphSheetIds[i], colours[i]);
				graphSheetRelease(sheet);
			}
		}
	}
}

static inline int page_taskmanInitalize (TTASKMAN *taskman, TVLCPLAYER *vp, const int width, const int height)
{
	wchar_t buffer[MAX_PATH+1];
	buildSkinD(vp, buffer, L"common\\icons\\");
	taskman->folders.icons = my_wcsdup(buffer);	
	taskman->folders.system32 = fbGetSystem32Folder();
	taskman->folders.system64 = fbGetSystem64Folder();		// syswow64
	taskman->folders.windows = fbGetWindowsFolder();

	taskmanCPUGraphInitalize(taskman, vp, width, height);
	
	return 1;
}

static inline int page_taskmanShutdown (TTASKMAN *taskman)
{

	if (taskman->list.previous)
		taskmanSnapshotFree(taskman->list.previous);
	if (taskman->list.current && taskman->list.current != taskman->list.previous)
		taskmanSnapshotFree(taskman->list.current);
	if (taskman->folders.system32)
		my_free(taskman->folders.system32);
	if (taskman->folders.system64)
		my_free(taskman->folders.system64);
	if (taskman->folders.icons)
		my_free(taskman->folders.icons);
		
	if (taskman->cpugraph.enabled)
		ccDelete(taskman->cpugraph.graph);
		
	ccDelete(taskman->pane);
	ccDelete(taskman->context.pane);

	return 1;
}

static inline int page_taskmanInput (TTASKMAN *taskman, const int msg, const int flags, TTOUCHCOORD *pos)
{
	//printf("page_taskmanInput %i\n", msg);
	
  	const int x = flags >> 16;
  	const int y = flags & 0xFFFF;
	
	switch(msg){
	  case PAGE_IN_TOUCH_DOWN:
	  	page2SetPrevious(taskman);
	  	break;
	 /* case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  case PAGE_IN_WHEEL_FORWARD:
	  case PAGE_IN_WHEEL_BACK:
	  case PAGE_IN_WHEEL_LEFT:
	  case PAGE_IN_WHEEL_RIGHT:*/
	  
	  case PAGE_IN_WHEEL_FORWARD:
		if (ccGetState(taskman->context.pane) && ccPositionIsOverlapped(taskman->context.pane, x, y))
			paneScroll(taskman->context.pane, TASKMAN_CONTEXT_SCROLL_DELTA);
		else if (ccGetState(taskman->pane) && ccPositionIsOverlapped(taskman->pane, x, y))
			paneScroll(taskman->pane, TASKMAN_PANE_SCROLL_DELTA*2);

		break;
	  case PAGE_IN_WHEEL_BACK:
		if (ccGetState(taskman->context.pane) && ccPositionIsOverlapped(taskman->context.pane, x, y))
			paneScroll(taskman->context.pane, -(TASKMAN_CONTEXT_SCROLL_DELTA));
		else if (ccGetState(taskman->pane) && ccPositionIsOverlapped(taskman->pane, x, y))
			paneScroll(taskman->pane, -(TASKMAN_PANE_SCROLL_DELTA*2));
	}
	
	return 1;
}
		
void (CALLBACK taskmanProcUpdateTimerCB )(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	TTASKMAN *taskman = (TTASKMAN*)dwUser;
	
	if (!SHUTDOWN && taskman->canRender && getApplState(taskman->com->vp)){
		if (ccLock(taskman->pane)){
			ple_snapshot *current = taskmanSnapshotAcquire(taskman);
			taskmanSnapshotSwap(taskman, current);
			processSubtractProcessList(current->ple, taskmanSnaphotGetPrevious(taskman));
			processMultiplyProcessList(current->ple, current->ple->multiplier);
			taskmanPaneRemoveOldPIDS(taskman, current);

			//taskmanPaneClean(taskman);
			if (taskman->canRender)
				taskmanPaneAddSnapshot(taskman, current, 1);
			ccUnlock(taskman->pane);
		}
	}
}

static inline int page_taskmanRenderBegin (TTASKMAN *taskman, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	taskman->idleDisabled = taskman->com->vp->gui.idleDisabled;
	taskman->com->vp->gui.idleDisabled = 1;
	taskman->layout.lineSpacing = lSetFontLineSpacing(taskman->com->vp->ml->hw, TASKMAN_PROCESS_FONT, -7);
	taskman->procTimerId = (int)timeSetEvent(1000, 10, taskmanProcUpdateTimerCB, (DWORD_PTR)taskman, TIME_PERIODIC|TIME_KILL_SYNCHRONOUS);
	taskman->canRender = 1;
	ccEnable(taskman->pane);
	
	return 1;
}

static inline int page_taskmanRenderEnd (TTASKMAN *taskman, int64_t time0, int64_t zDepth, void *opaquePtr)
{
	taskman->canRender = 0;
	if (taskman->procTimerId){
		timeKillEvent(taskman->procTimerId);
		taskman->procTimerId = 0;
	}

	if (taskmanContextIsVisable(taskman))
		taskmanContextHide(taskman);
			
	taskman->com->vp->gui.idleDisabled = taskman->idleDisabled;
	lSetFontLineSpacing(taskman->com->vp->ml->hw, TASKMAN_PROCESS_FONT, taskman->layout.lineSpacing);
	return 1;
}

int page_taskmanCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TPAGE2COMOBJ *page = (TPAGE2COMOBJ*)pageStruct;
	
	// printf("# page_Callback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_taskmanRender(pageStruct, dataPtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_taskmanInput(pageStruct, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_START){
		return page_taskmanRenderBegin(pageStruct, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		return page_taskmanRenderEnd(pageStruct, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_taskmanRenderInit(pageStruct, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_taskmanStartup(pageStruct, page->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_taskmanInitalize(pageStruct, page->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_taskmanShutdown(pageStruct);
		
	}
	
	return 1;
}

