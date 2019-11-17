
// anthrm - http://mylcd.sourceforge.net/
// An LCD framebuffer and text rendering API
// Michael McElligott
// okio@users.sourceforge.net

//  Copyright (c) 2005-2011  Michael McElligott
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


#if ENABLE_ANTPLUS


extern int SHUTDOWN;

#define GRAPHSCALE 1.5



struct sheets {
	char *title;
	char *titleAlt;
	int colour;
};

static const struct sheets sheetColours[SHEET_TOTAL] = {
	{"Current rate (Red)",		 "Current rate",	 COL_ALPHA(190)|COL_RED},
	{"Complete history (Orange)","Complete history", COL_ALPHA(255)|COL_ORANGE},
	{"Mode histroy (White)",	 "Mode histroy",	 COL_ALPHA(255)|COL_WHITE}
};

#ifdef drawImage
#undef drawImage
#endif
#define drawImage(s,d,x,y) copyArea(s,d,x,y,0,0,s->width-1,d->height-1)





static inline int getMode (const unsigned char *bpm, const int len)
{
	unsigned char hist[HRBMP_BUFFERLENGTH];
	memset(hist, 0, HRBMP_BUFFERLENGTH);
		
	for (int i = (HRBMP_BUFFERLENGTH-len-1); i < HRBMP_BUFFERLENGTH; i++)
		hist[bpm[i]]++;
	
	int mode = 0;
	int most = 1;
	const int minBpm = 20;
	const int maxBpm = 150;
	
	for (int i = minBpm; i < HRBMP_BUFFERLENGTH && i <= maxBpm; i++){
		if (hist[i] > most){
			most = hist[i];
			mode = i;
		}
	}
	//printf("getMode %i %i\n", most, mode);
	return mode;
}

static inline void antPulseCb (const THRBUFFER *rate, const void *opaque)
{
	if (SHUTDOWN || rate->currentBpm < 25) return;	// if it's less than 25 then i've probably died 
	//static uint64_t preTime;

	//printf("antPulseCb %i HR: %i %i %i\n", rate->currentSequence, rate->currentBpm, ant->stats.mode, ant->stats.average);
	//printf("antPulseCb %i\n", (int)(rate->time0 - preTime));
	//preTime = rate->time0;


	TANTPLUS *ant = (TANTPLUS*)opaque;
	
	ant->stats.current = rate->currentBpm;
	ant->stats.average = rate->average;
	ant->stats.mode = getMode(rate->bpm, 400);
			
	for (int i = 0; i < SHEET_TOTAL && ant->graphSheetIds[i]; i++){
		TGRAPHSHEET *sheet = graphSheetAcquire(ant->graph, NULL, ant->graphSheetIds[i]);
		if (sheet){
			if (i == SHEET_MODE){
				if (ant->stats.mode){
					graphSheetAddData(sheet, ant->stats.mode, 0);
					regSetDword(L"HRM_mode", ant->stats.mode);
				}
				regSetDword(L"HRM_bpm", rate->currentBpm);
				regSetDword(L"HRM_bpmPrevious", rate->bpm[HRBMP_BUFFERLENGTH-2]);
				regSetDword(L"HRM_low", rate->low);
				regSetDword(L"HRM_high", rate->high);
				regSetDword(L"HRM_average", ant->stats.average);
				
				regSetQword(L"HRM_bpmTime", rate->time0);
			}else{
				//for (int i = 0; i < 100; i++)
				graphSheetAddData(sheet, rate->currentBpm, 0);
			}
			graphSheetRelease(sheet);
		}
	}

	if (ant->enableOverlay){
		char buffer[16];
		__mingw_snprintf(buffer, sizeof(buffer), "%i", ant->stats.current);
		lsStrReplace(ant, ant->lsId, buffer);
	}
	if (page2RenderGetState(ant->com->vp->pages, PAGE_HOME))
		renderSignalUpdate(ant->com->vp);
}

static inline int antHrmStart (TANTPLUS *ant)
{
	ant->hr = hrmStart(ant->device.vid, ant->device.pid, ant->device.index, ant->device.key, ant->device.id, ant->com->vp, antPulseCb, ant, &ant->images);
	return (ant->hr != NULL);
}

static inline void antHrmShutdown (TANTPLUS *ant)
{
	if (ant->hr){
		hrmShutdown(ant->hr);
		ant->hr = NULL;
	}
}

static inline void antCtrlPaneClear (TANTPLUS *ant, TPANE *pane)
{
	paneRemoveAll(pane);
}

void antCtrlPaneAddSet (TANTPLUS *ant, TPANE *pane, const int itemSet)
{
	wchar_t path[MAX_PATH+1];
	TVLCPLAYER *vp = ant->com->vp;

	ant->paneSet = itemSet;	
	if (itemSet == ANTPANEL_SET_FIND){
		int imgcId = artManagerImageAdd(vp->am, buildSkinD(vp, path, L"hrm/find.png"));
		paneTextAdd(pane, imgcId, 1.0, " ", MFONT, ANTPLUS_FIND);
		
	}else if (itemSet == ANTPANEL_SET_START){
		int imgcId = artManagerImageAdd(vp->am, buildSkinD(vp, path, L"hrm/find.png"));
		paneTextAdd(pane, imgcId, 1.0, " ", MFONT, ANTPLUS_FIND);
		
		imgcId = artManagerImageAdd(vp->am, buildSkinD(vp, path, L"hrm/start.png"));
		paneTextAdd(pane, imgcId, 1.0, " ", MFONT, ANTPLUS_START);

	}else if (itemSet == ANTPANEL_SET_STOP){
		int imgcId = artManagerImageAdd(vp->am, buildSkinD(vp, path, L"hrm/stop.png"));
		paneTextAdd(pane, imgcId, 1.0, " ", MFONT, ANTPLUS_STOP);	
		
		imgcId = artManagerImageAdd(vp->am, buildSkinD(vp, path, L"hrm/sfocus.png"));
		paneTextAdd(pane, imgcId, 1.0, " ", MFONT, ANTPLUS_SHEETFOCUS);

		imgcId = artManagerImageAdd(vp->am, buildSkinD(vp, path, L"hrm/clear.png"));
		paneTextAdd(pane, imgcId, 1.0, " ", MFONT, ANTPLUS_CLEAR);		
	}
	
	paneFocusSet(pane, pane->firstEnabledImgId);
	int width = pane->tItemWidth;
	int x = (pageGetSurfaceWidth(ant) - width)/2;
	int y = ccGetPositionY(pane);
	ccSetMetrics(pane, x, y, width, ccGetHeight(pane));
}

int antIsAntEnabled (TVLCPLAYER *vp)
{
	int state = 0;
	settingsGet(vp, "home.enableHRM", &state);
	return state;
}

int antConfigIsAntEnabled (void *pageStruct)
{
	TPAGE2COMOBJ *page = (TPAGE2COMOBJ*)pageStruct;
	return antIsAntEnabled(page->com->vp);
}

int antConfigGetDeviceIds (void *pageStruct, int *vid, int *pid)
{
	TPAGE2COMOBJ *page = (TPAGE2COMOBJ*)pageStruct;
	
	*vid = 0, *pid = 0;
	settingsGet(page->com->vp, "hrm.device.vid", vid);
	settingsGet(page->com->vp, "hrm.device.pid", pid);
	return vid && pid;
}

int antGetDeviceCount (void *pageStruct)
{
	TPAGE2COMOBJ *page = (TPAGE2COMOBJ*)pageStruct;

	int vid, pid;
	antConfigGetDeviceIds(page, &vid, &pid);

	if (vid && pid)
		return libantplus_GetDeviceCount(vid, pid);
	else
		return 0;
}

static inline int doDeviceDiscovery (TANTPLUS *ant)
{
	int tDevice = antGetDeviceCount(ant);
	if (!tDevice){
		antCtrlPaneClear(ant, ant->pane);
		antCtrlPaneAddSet(ant, ant->pane, ANTPANEL_SET_FIND);
	}else{
		antCtrlPaneClear(ant, ant->pane);
		antCtrlPaneAddSet(ant, ant->pane, ANTPANEL_SET_START);
	}
	return tDevice;
}

static inline void onButtonDiscoverDevices (TANTPLUS *ant)
{
	int tDevice = doDeviceDiscovery(ant);
	if (!tDevice){
		dbprintf(ant->com->vp, "AntPlus USB stick not found");
	}else{
		char *plu = " ";
		if (tDevice > 1) plu = "s ";
		dbprintf(ant->com->vp, "%i device%slocated", tDevice, plu);
	}
}

static inline void onButtonStart (TANTPLUS *ant)
{
	antConfigGetDeviceIds(ant, &ant->device.vid, &ant->device.pid);
	//printf("doButtonStart %X %X\n", ant->device.vid, ant->device.pid);

	if (antHrmStart(ant)){
		hrmStatsSet(ant->hr, &ant->stats.rate);
		
		if (ant->enableOverlay)
			ant->lsId = lsItemCreate(ant, " ", TEXTOVERLAY_FONT, 22, 40, NSEX_RIGHT);
		antCtrlPaneClear(ant, ant->pane);
		antCtrlPaneAddSet(ant, ant->pane, ANTPANEL_SET_STOP);
		renderSignalUpdate(ant->com->vp);
	}
}

static inline void onButtonStop (TANTPLUS *ant)
{
	//printf("doButtonStop\n");
	if (ant->lsId)
		lsItemRemove(ant, ant->lsId);
	hrmStatsGet(ant->hr, &ant->stats.rate);
	antHrmShutdown(ant);
	doDeviceDiscovery(ant);
}

static inline void onButtonClear (TANTPLUS *ant)
{
	//printf("doButtonClear\n");
	
	if (ant->hr){
		graphClear(ant->graph, 0);
		hrmStatsReset(ant->hr);
		renderSignalUpdate(ant->com->vp);
	}
}

static inline void onButtonSheetFocus (TANTPLUS *ant)
{
	if (++ant->sheetFocus >= SHEET_TOTAL)
		ant->sheetFocus = 0;
	dbprintf(ant->com->vp, sheetColours[ant->sheetFocus].title);
}

static inline int antButtonPress (TANTPLUS *ant, const int btnId)
{
	//TVLCPLAYER *vp = ant->com->vp;

	switch (btnId){
	case ANTPLUS_FIND:
		onButtonDiscoverDevices(ant);
		break;
	case ANTPLUS_START:
		onButtonStart(ant);
		break;
	case ANTPLUS_STOP:
		onButtonStop(ant);
		break;
	case ANTPLUS_CLEAR:
		onButtonClear(ant);
		break;
	case ANTPLUS_SHEETFOCUS:
		onButtonSheetFocus(ant);
		break;

	//case ANTPLUS_CLOSE:
	//	page2SetPrevious(ant);
	//	break;
	};
	
	
	return 1;
}

static inline void antDrawGraphKey (TANTPLUS *ant, TFRAME *frame)
{
	if (ant->paneSet != ANTPANEL_SET_STOP) return;
	
	lSetBackgroundColour(frame->hw, 0x00FFFFFF);
	lSetRenderEffect(frame->hw, LTR_OUTLINE2);
	lSetFilterAttribute(frame->hw, LTR_OUTLINE2, 0, 160<<24 | COL_BLACK);
	const int font = LFTW_B28;
	const int y[SHEET_TOTAL] = {(frame->height/2) + 80, (frame->height/2) + 80+40, (frame->height/2) + 80+80};
	
	for (int i = 0; i < SHEET_TOTAL; i++){
		lSetForegroundColour(frame->hw, 255<<24|sheetColours[i].colour);
		TFRAME *str = lNewString(frame->hw, frame->bpp, PF_IGNOREFORMATTING, font, sheetColours[i].titleAlt);
		if (str){
			drawImage(str, frame, 4, y[i]);
			lDeleteFrame(str);
		}
	}
	
	lSetRenderEffect(frame->hw, LTR_DEFAULT);
}

static inline int64_t ant_pane_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT || msg == CC_MSG_HOVER) return 1;
	
	TPANE *pane = (TPANE*)object;
	//printf("pane_cb in %p, %i %I64d %I64d %p\n", pane, msg, data1, data2, dataPtr);
	
	if (msg == CC_MSG_RENDER){
		antDrawGraphKey(ccGetUserData(pane), dataPtr);
		
	}else if (msg == PANE_MSG_TEXT_SELECTED || msg == PANE_MSG_IMAGE_SELECTED){
		//int itemId = data1 + (msg == PANE_MSG_IMAGE_SELECTED);
		//TVLCPLAYER *vp = pane->cc->vp;
		//TANTPLUS *ant = pageGetPtr(vp, PAGE_ANTPLUS);	
		
		//if (ant){
			//printf("ant_pane_cb in %p, %i %I64d %I64d %p\n", pane, msg, data1, data2, dataPtr);
			antButtonPress(ccGetUserData(pane), data2&0xFF);
		//}
	}
	
	return 1;
}

static inline int64_t ccGraph_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//printf("ccGraph_cb in %p, %i %I64d %I64d %p\n", object, msg, data1, data2, dataPtr);
	
	return 1;
}

static inline int antCreateGraph (TANTPLUS *ant, const int x, const int y, const int width, const int height)
{
	TGRAPH *graph = ccCreate(ant->com->vp->cc, PAGE_ANTPLUS, CC_GRAPH, ccGraph_cb, NULL, width, height);
	ccSetPosition(graph, x, y);
	ant->graph = graph;
	ant->sheetFocus = 0;

	for (int i = 0; i < SHEET_TOTAL; i++){
		ant->graphSheetIds[i] = graphNewSheet(graph, "HRM graph", 0);

		TGRAPHSHEET *sheet = graphSheetAcquire(graph, NULL, ant->graphSheetIds[i]);
		if (sheet){
			if (i == SHEET_CURRENT){
				sheet->render.mode = GRAPH_SPECTRUM | GRAPH_POLYLINE;						// current rate and 800 seconds of history (moving right (latest) to left (oldest))
				sheet->render.palette[GRAPH_PAL_SPECTRUM] = COL_ALPHA(80)  | 0xF01010;
				sheet->render.palette[GRAPH_PAL_POLYLINE] = sheetColours[SHEET_CURRENT].colour;
				sheet->render.scale = GRAPHSCALE;
				sheet->stats.min = 0;
				sheet->stats.max = 100;
			}else if (i == SHEET_HISTORY){
				sheet->render.mode = GRAPH_SCOPE | GRAPH_AUTOSCALE;							// rate history over time
				sheet->render.scale = 1.0;
				sheet->render.hints = GRAPH_HINT_SHADOW2;
				sheet->render.palette[GRAPH_PAL_SCOPE] = sheetColours[SHEET_HISTORY].colour;
				sheet->stats.min = 9999999;
				sheet->stats.max = 0;
				sheet->graph.canGrow = 1;
			}else if (i == SHEET_MODE){
				sheet->render.mode = GRAPH_SCOPE | GRAPH_AUTOSCALE;							// mode history over time
				sheet->render.scale = 1.0;
				sheet->render.hints = GRAPH_HINT_SHADOW1;
				sheet->render.palette[GRAPH_PAL_SCOPE] = sheetColours[SHEET_MODE].colour;
				sheet->stats.min = 9999999;
				sheet->stats.max = 0;
				sheet->graph.canGrow = 1;
			}
			graphSheetRelease(sheet);
		}
	}
	
	ccEnable(ant->graph);	
	return 1;
}

static inline void drawCursor (TANTPLUS *ant, TFRAME *frame, const int focus, const int x, const int y)
{
	TFRAME *img = imageManagerImageAcquire(ant->com->vp->im, ant->cursorIds[focus]);
	if (img){
		drawImage(img, frame, x-(img->width/2), y-(img->height/2));
		imageManagerImageRelease(ant->com->vp->im, ant->cursorIds[focus]);
	}
}

static inline void antRenderGraphSheet (TANTPLUS *ant, TGRAPH *graph, TGRAPHSHEET *sheet, TFRAME *frame)
{
			
	TMETRICS metrics;
	ccGetMetrics(graph, &metrics);
	int y = metrics.y + (metrics.height - (ant->stats.mode * sheet->render.scale/*GRAPHSCALE*/));
	lDrawLineDotted(frame, metrics.x, y, metrics.x+metrics.width-1, y, COL_ALPHA(255)|COL_YELLOW);

	ccRender(graph, frame);
		
	if (ccIsHovered(graph)){
		//ant->com->vp->gui.cursor.draw = 0;
		//int x = ant->com->vp->gui.cursor.dx;
		//int y = ant->com->vp->gui.cursor.dy;
		graph->cc->cursor->draw = 0;
		int x = graph->cc->cursor->dx;
		int y = graph->cc->cursor->dy;
		
		drawCursor(ant, frame, ant->sheetFocus, x, y);

		const double range = (metrics.height/sheet->render.scale)/100.0;
		const double value = (graph->hoveredPt.y * range) + (sheet->stats.min * (sheet->render.mode&GRAPH_AUTOSCALE));

		lSetRenderEffect(frame->hw, LTR_OUTLINE2);
		TFRAME *val = lNewString(frame->hw, frame->bpp, 0, ANT_HEADING_FONT, "%.0f", value);	
		if (val){
			drawImage(val, frame, (metrics.x+metrics.width-val->width)-16, metrics.y-val->height);
			lDeleteFrame(val);
		}
		lSetRenderEffect(frame->hw, LTR_DEFAULT);
	}else{
		//ant->com->vp->gui.cursor.draw = 1;
		graph->cc->cursor->draw = 1;
	}
}

static inline int page_antRender (TANTPLUS *ant, TVLCPLAYER *vp, TFRAME *frame)
{
	if (ant->hr){
		ant->connectStatus = hrmRender(ant->hr, frame);
		//printf("page_antRender hrmRender():%i\n", ant->connectStatus);
		if (ant->connectStatus > 0){
			TGRAPHSHEET *sheet = graphSheetAcquire(ant->graph, NULL, ant->graphSheetIds[ant->sheetFocus]);
			if (sheet){
				antRenderGraphSheet(ant, ant->graph, sheet, frame);
				graphSheetRelease(sheet);
			}
		}
	}
	
	//TGRAPH *graph = ant->graph;
	//printf("graph %i %i %i %i\n", graph->metrics.x, graph->metrics.y, graph->metrics.width, graph->metrics.height);
	
	ccRender(ant->pane, frame);
	return 1;
}

static inline int page_antInput (TANTPLUS *ant, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	//printf("# page_antInput: %p %i %I64d %I64d %p %p\n", msg, flags, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_IN_TOUCH_DOWN){
		int y = ccGetPositionY(ant->pane) - 16;
		if (pos->y < y){
			page2SetPrevious(ant);
		}else{
			if (ant->hr){
		  		if (!ccGetState(ant->pane))
		  			ccEnable(ant->pane);
	  			else if (ant->connectStatus > 0)
	  				ccDisable(ant->pane);
	  		}
	  	}
	}
	
	return 1;
}

static inline int page_antStartup (TANTPLUS *ant, TVLCPLAYER *vp, const int width, const int height)
{
	if (!antConfigIsAntEnabled(ant))
		return 0;
	
	char *strKey = NULL;
	settingsGet(vp, "hrm.sensor.id", &ant->device.id);
	settingsGet(vp, "hrm.enableOverlay", &ant->enableOverlay);
	settingsGet(vp, "hrm.activateOnInsertion", &ant->device.activateOnInsertion);
	settingsGet(vp, "hrm.device.index", &ant->device.index);
	settingsGet(vp, "hrm.device.key", &strKey);
	if (strKey){
		//printf("key '%s'\n",strKey);
		char *str = strKey;
		for (int i = 0; i < 8 && *str; i++){
			ant->device.key[i] = hexToInt(str)&0xFF;
			str += strcspn(str, ",")+1;
		}
		//for (int i = 0; i < 8; i++)
		//	printf("%X\n", ant->device.key[i]);
		my_free(strKey);
	}
	
	return 1;
}

static inline int page_antInitalize (TANTPLUS *ant, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_ANTPLUS);
	
	TPANE *pane = ccCreate(vp->cc, PAGE_ANTPLUS, CC_PANE, ant_pane_cb, &vp->gui.ccIds[CCID_PANE_ANTCTRL], 115*5, 132);
	ant->pane = pane;
	paneSwipeDisable(pane);
	ccSetUserData(pane, ant);
	ccSetPosition(pane, 100, (height-128)/2);
	paneSetLayout(pane, PANE_LAYOUT_HORICENTER);
	
	unsigned int col_pre[] = {
		160<<24 | COL_WHITE,
		100<<24 | COL_RED,
		 70<<24 | COL_WHITE
	  };
	labelBorderProfileSet(pane->base, LABEL_BORDER_SET_PRE, col_pre, 3);
	
	/*unsigned int col_post[] = {
		255<<24 | COL_RED,
		180<<24 | COL_WHITE,
		 70<<24 | COL_RED
	  };
	labelBorderProfileSet(pane->base, LABEL_BORDER_SET_POST, col_post, 1);*/
	
	
	int flags = LABEL_RENDER_IMAGE | LABEL_RENDER_BLUR;
	flags |= LABEL_RENDER_BASE;
	flags |= LABEL_RENDER_BORDER_PRE;
	//flags |= LABEL_RENDER_BORDER_POST;
	flags |= LABEL_RENDER_HOVER_OBJ;
	
	labelRenderFlagsSet(pane->base, flags);
	labelBaseColourSet(pane->base, 40<<24 | COL_BLACK);
	
	ccEnable(pane);	
	hrmStatsClear(&ant->stats.rate);
	antCreateGraph(ant, 1, 270, width-2, 200);
	doDeviceDiscovery(ant);

	return 1;
}

static inline int page_antShutdown (TANTPLUS *ant, TVLCPLAYER *vp)
{
	antHrmShutdown(ant);
	ccDelete(ant->pane);
	ccDelete(ant->graph);

	return 1;
}

static inline int page_antRenderInit (TANTPLUS *ant, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	vp->gui.image[IMGC_HRM_SIGNAL] = imageManagerImageAdd(vp->im, L"hrm/signal.png");
	ant->cursorIds[0] = imageManagerImageAdd(vp->im, L"hrm/cursor0.png");
	ant->cursorIds[1] = imageManagerImageAdd(vp->im, L"hrm/cursor1.png");
	ant->cursorIds[2] = imageManagerImageAdd(vp->im, L"hrm/cursor2.png");

	wchar_t *digitsFolder;
	settingsGetW(vp, "hrm.digits", &digitsFolder);
	if (!digitsFolder) return 0;
	
	wchar_t buffer[MAX_PATH+1];
	for (int i = 0; i < 10; i++){
		__mingw_snwprintf(buffer, MAX_PATH, L"common/digits/%ls/%i.png", digitsFolder, i);
		vp->gui.image[IMGC_ANT_DIGIT_0+i] = imageManagerImageAdd(vp->im, buffer);
	}

	my_free(digitsFolder);
	return 1;
}

static inline int page_antDeviceArrive (TANTPLUS *ant, TVLCPLAYER *vp, const int msg)
{
#if 0
	wchar_t *img;
	if (msg == PAGE_MSG_DEVICE_ARRIVE)
		img = L"home/antpluson.png";
	else
		img = L"home/antplusoff.png";

	picQueueAdd(vp->gui.picQueue, img, img, getTickCount()+8000);
#endif
	int tDevices = doDeviceDiscovery(ant);
	if (!tDevices){
		// todo: if device was activated then suspend/close thread
		if (!SHUTDOWN && ant->hr && ant->hr->msgThreadState)
			antHrmShutdown(ant);
	}else if (msg == PAGE_MSG_DEVICE_ARRIVE && ant->device.activateOnInsertion){
		//hrmResync(vp);
	}

	return 1;
}

static inline int page_antRenderBegin (TANTPLUS *ant, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	for (int i = 0; i < 10; i++)
		ant->images.list[i] = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_ANT_DIGIT_0+i]);

	ant->images.list[10] = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_HRM_SIGNAL]);

	if (!ant->hr && !ccGetState(ant->pane))
		ccEnable(ant->pane);
	return 1;
}

static inline int page_antRenderEnd (TANTPLUS *ant, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, void *opaquePtr)
{
	//printf("page_hrmRenderEnd %i %p\n", hrm->state, hrm->context);
	
	for (int i = 0; i < 10; i++)
		imageManagerImageRelease(vp->im, vp->gui.image[IMGC_ANT_DIGIT_0+i]);
	
	imageManagerImageRelease(vp->im, vp->gui.image[IMGC_HRM_SIGNAL]);

	//ant->com->vp->gui.cursor.draw = 1;
	ant->graph->cc->cursor->draw = 1;
	return 1;
}	
	
int page_antCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TANTPLUS *ant = (TANTPLUS*)pageStruct;
	
	//printf("# page_antCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_antRender(ant, ant->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_antInput(ant, ant->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_START){
		return page_antRenderBegin(ant, ant->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		return page_antRenderEnd(ant, ant->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_antRenderInit(ant, ant->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_antStartup(ant, ant->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_antInitalize(ant, ant->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_antShutdown(ant, ant->com->vp);
		
	}else if (msg == PAGE_MSG_DEVICE_ARRIVE || msg == PAGE_MSG_DEVICE_DEPART){
		int vid = dataInt1;
		int pid = dataInt2;
		
		if (vid == ANTSTICK_VID && pid == ANTSTICK_PID){
			return page_antDeviceArrive(ant, ant->com->vp, msg);
		}
	}
	
	return 1;
}

#endif

