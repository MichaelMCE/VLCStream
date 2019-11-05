
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




static inline const char *clockTypeToString (const int type)
{
	if (type == CLOCK_ANALOGUE)
		return "Analogue";
	else if (type == CLOCK_BOXDIGITAL)
		return "BoxDigital";
	else if (type == CLOCK_BUTTERFLY)
		return "Butterfly";
	else if (type == CLOCK_POLAR)
		return "Polar";
	else if (type == CLOCK_PREDATOR)
		return "Predator";
	else
		return "Digital";
}

int configSave (TVLCPLAYER *vp, const wchar_t *name)
{
	__mingw_wprintf(L"configSave '%ls'\n", name);
	

	//settingsSet(vp, "general.visual", &vp->gui.visual);
	settingsSet(vp, "general.showStats", &vp->gui.drawStats);
	int valuei = (vp->gui.idleTime/1000);
	settingsSet(vp, "general.idleTimeout", &valuei);
	settingsSet(vp, "general.idleFps", &vp->gui.idleFPS);
	settingsSet(vp, "general.overlayPeriod", &vp->gui.mOvrTime);
	settingsSet(vp, "general.randomTrack", &vp->vlc->playRandom);
	settingsSet(vp, "general.runCount", &vp->gui.runCount);

	settingsSet(vp, "volume.last", &vp->vlc->volume);
	
#if ENABLE_BRIGHTNESS
	int brightness = vp->gui.brightness*10;
	if (brightness > 100) brightness = 100;
	settingsSet(vp, "display.backlight", &brightness);
#endif

	settingsSet(vp, "artwork.searchDepth", &vp->gui.artSearchDepth);
	settingsSet(vp, "artwork.maxWidth", &vp->gui.artMaxWidth);
	settingsSet(vp, "artwork.maxHeight", &vp->gui.artMaxHeight);

	
	TPLYPANEL *plypan = pageGetPtr(vp, PAGE_PLY_PANEL);
	settingsSet(vp, "artwork.panelImgSize", &plypan->imgArtSize);

	uint64_t hash;
	int lasttrack = getPlayingItem(vp);
	if (lasttrack >= 0){
		hash = getPlayingHash(vp);
	}else{
		hash = getSelectedHash(vp);
		lasttrack = getSelectedItem(vp);
	}

	settingsSet(vp, "lasttrack.track", &lasttrack);
	int idx = playlistUIDToIdx(vp->plm, getQueuedPlaylistUID(vp));
	settingsSet(vp, "lasttrack.playlist", &idx);
	settingsSet(vp, "lasttrack.hash", &hash);
	
	TFILEPANE *filepane = pageGetPtr(vp, PAGE_FILE_PANE);
	settingsSet(vp, "browser.filterBy", filterIdxToStr(filepane->filterMask));
	settingsSet(vp, "browser.sortBy", sortIdxToStr(filepane->sortMode));
	
	
	
	TEQ *eq = pageGetPtr(vp, PAGE_EQ);
	double amp = eqBandGet(eq, 0);
	settingsSet(vp, "equalizer.preamp", &amp);
	settingsSet(vp, "equalizer.preset", &eq->preset);
	
	str_list *strList = cfg_configStrListNew(eq->tBands-1);
	if (strList){
		char buffer[64];
		for (int i = 0; i < strList->total; i++){
			double value = eqBandGet(eq, i+1);
		  	if (value == (int)value)
		  		__mingw_snprintf(buffer, sizeof(buffer), "%i.000", (int)value);
		  	else
		  		__mingw_snprintf(buffer, sizeof(buffer), "%.3f", value);
			strList->strings[i] = my_strdup(buffer);
		}
		settingsSet(vp, "equalizer.band.", strList);
	}

	settingsSet(vp, "meta.drawTrackbar", &vp->gui.drawMetaTrackbar);
	//settingsSet(vp, "device.virtual.onScreenCtrlIcons", &vp->gui.drawControlIcons);
	
	int var = vp->ml->killRzDKManagerOnConnectFailRetry;
	settingsSet(vp, "device.sbui.killRzDKManagerOnConnectFailRetry", &var);
	
	const char *str = aspectIdxToStr(vp->ctx.aspect.preset);
	settingsSet(vp, "video.aspect.preset", (char*)str);
	settingsSet(vp, "video.filter.rotate", &vp->vlc->rotateAngle);
	settingsSet(vp, "video.filter.scale", &vp->vlc->scaleFactor);
	settingsSet(vp, "video.filter.blur",	&vp->vlc->blurRadius);
	settingsSet(vp, "video.filter.pixelize", &vp->vlc->pixelize);
	settingsSet(vp, "video.filter.brightness", &vp->vlc->brightness);
	settingsSet(vp, "video.filter.contrast", &vp->vlc->contrast);
	settingsSet(vp, "video.filter.saturation", &vp->vlc->saturation);
	settingsSet(vp, "video.filter.gamma", &vp->vlc->gamma);
	settingsSet(vp, "video.filter.rotateOp", &vp->vlc->rotateOp);
	settingsSet(vp, "video.filter.scaleOp", &vp->vlc->scaleOp);

	settingsSet(vp, "video.subtitle.delay", &vp->vlc->subtitleDelay);
	settingsSet(vp, "video.swapRB", &vp->vlc->swapColourBits);
	setRBSwap(vp, vp->vlc->swapColourBits);
	
	settingsSet(vp, "audio.desync", &vp->vlc->audioDesync);
	settingsSet(vp, "audio.visuals", &vp->gui.drawVisuals);

	if (hasPageBeenAccessed(vp, PAGE_HOTKEYS)){
		TGLOBALHOTKEYS *ghk = pageGetPtr(vp, PAGE_HOTKEYS);
		settingsSet(vp, "hotkeys.global.showLabels", &ghk->showNames);
	}
	settingsSet(vp, "home.hotkeys.alwaysAccessible", &vp->gui.hotkeys.alwaysAccessible);

	settingsSet(vp, "hotkeys.local.cursor", &vp->gui.hotkeys.cursor);
	settingsSet(vp, "hotkeys.local.console", &vp->gui.hotkeys.console);

	if (hasPageBeenAccessed(vp, PAGE_CLOCK)){
		TCLK *clk = pageGetPtr(vp, PAGE_CLOCK);
		settingsSet(vp, "clock.type", (void*)clockTypeToString(vp->gui.clockType));
		settingsSet(vp, "clock.face.dial.x", &clk->butterfly.cx);
		settingsSet(vp, "clock.face.dial.y", &clk->butterfly.cy);
	}else if (hasPageBeenAccessed(vp, PAGE_CFG)){
		TCFG *cfg = pageGetPtr(vp, PAGE_CFG);
		settingsSet(vp, "clock.type", (void*)clockTypeToString(cfg->clockType));
	}

	if (hasPageBeenAccessed(vp, PAGE_ALARM)){
		char buffer[ALARM_MAXTIMEDIGITS+1] = {0};
		TALARM *alarm = pageGetPtr(vp, PAGE_ALARM);
		int status = alarmUiGetStatus(alarm);
		
		alarmUiGetAlarmTime(alarm, buffer);
		settingsSet(vp, "alarm.time", &buffer);
		settingsSet(vp, "alarm.enabled", &status);
		settingsSet(vp, "alarm.period", (void*)alarmCfgGetPeriodStr(alarm));
		
		str_list *strList = cfg_configStrListNew(7);	// 7 days per week (don't foresee this changing anytime soon)
		if (strList){
			const int days = alarmCfgGetWhen(alarm);
			int daysTotal = 0;

			for (int i = 0; i < 7; i++){
				if (days&(1<<i))
					strList->strings[daysTotal++] = my_strdup(clkDayToName(i));
			}
			strList->total = daysTotal;
			settingsSet(vp, "alarm.weekly.", strList);
		}

		int action = alarmActionGet(alarm, alarm->active.aid);
		if (action == ALARM_ACT_PLYSTART){
			alarm_plystart media = {0};
			if (alarmActionGetDesc(alarm, alarm->active.aid, action, &media)){
				const char *mode = "Playtrack";
				settingsSet(vp, "alarm.action.mode", (void*)mode);
				if (media.title)
					settingsSet(vp, "alarm.action.playtrack.title", (void*)media.title);
				settingsSet(vp, "alarm.action.playtrack.uid", &media.uid);
				settingsSet(vp, "alarm.action.playtrack.track", &media.track);
				settingsSet(vp, "alarm.action.playtrack.volume", &media.volume);				
			}
		}
	}

	//const double t0 = getTime(vp);
	int ret = settingsSave(&vp->settings, name);
	//const double t1 = getTime(vp);
	
	//printf("configSave: time %.3fms, size: %i\n", t1-t0, ret);
	return ret;
}

int configLoad (TVLCPLAYER *vp, const wchar_t *name)
{
	//const double t0 = getTime(vp);
	int ret = settingsLoad(&vp->settings, name);
	//const double t1 = getTime(vp);
	
	//printf("configLoad: time %.3fms, entries: %i\n", t1-t0, ret);
	return ret;
}

void configApply (TVLCPLAYER *vp)
{

/*
	settingsGet(vp, "general.visual", &vp->gui.visual);	
	
#if (LIBVLC_VERSION_MAJOR < 2)
	settingsGet(vp, "general.visual", &vp->gui.visual);	
	cfgButton(NULL, NULL, (vp->gui.visual+CFGBUTTON_VIS_DISABLED)-1, 0, vp);
#else
	vp->gui.visual = 0;//BTN_CFG_VIS_DISABLED;
#endif
*/

	settingsGet(vp, "general.runCount", &vp->gui.runCount);
	settingsGet(vp, "general.showStats", &vp->gui.drawStats);
	setShowStats(vp, vp->gui.drawStats);


#if ENABLE_BRIGHTNESS
	vp->gui.brightness *= 10;
	settingsGet(vp, "display.backlight", &vp->gui.brightness);
	if (vp->gui.brightness > 100) vp->gui.brightness = 80;
	setBrightness(vp, (vp->gui.brightness/10)+CFGBUTTON_BRN_0);
#endif
	
	vp->gui.idleTime /= 1000;
	settingsGet(vp, "general.idleTimeout", &vp->gui.idleTime);
	vp->gui.idleTime *= 1000;

	settingsGet(vp, "general.randomTrack", &vp->vlc->playRandom);
	settingsGet(vp, "general.idleFps", &vp->gui.idleFPS);
	settingsGet(vp, "general.overlayPeriod", &vp->gui.mOvrTime);

	settingsGet(vp, "volume.last", &vp->vlc->volume);

	settingsGet(vp, "artwork.searchDepth", &vp->gui.artSearchDepth);
	settingsGet(vp, "artwork.maxWidth", &vp->gui.artMaxWidth);
	vp->tagc->maxArtWidth = vp->gui.artMaxWidth;
	settingsGet(vp, "artwork.maxHeight", &vp->gui.artMaxHeight);
	vp->tagc->maxArtHeight = vp->gui.artMaxHeight;
	
#if 0
	int total = 0;
	settingsGet(vp, "artwork.threads", &total);
	total = MIN(MAX(total, 0), 8);
	if (total){
		if (!vp->jc)
			vp->jc = jobControllerNew(vp, jobThreadWorkerFunc);
		while (total--)
			jobThreadAdd(vp->jc);
	}
#endif

	/*TPLYPANEL *plypan = pageGetPtr(vp, PAGE_PLY_PANEL);
	settingsGet(vp, "artwork.panelImgSize", &plypan->imgArtSize);
	if (plypan->imgArtSize < 10) plypan->imgArtSize = 110;*/
	
	/*TPLYTV *plytv = pageGetPtr(vp, PAGE_PLY_TV);
	*(plytv->imgArtSize) = plypan->imgArtSize;*/
	
	int displayPlaylist = playlistManagerGetIndexByUID(vp->plm, vp->playlist.display);
	settingsGet(vp, "lasttrack.playlist", &displayPlaylist);

	if (displayPlaylist > 0){
		//vp->queuedPlaylist = vp->displayPlaylist = displayPlaylist;
		vp->playlist.queued = playlistManagerGetUIDByIndex(vp->plm, displayPlaylist);
		vp->playlist.display = vp->playlist.queued;
		//printf("idx:%i, uid:%i\n", displayPlaylist, vp->playlist.queued);
	}
	

	uint64_t hash = 0;
	settingsGet(vp, "lasttrack.hash", &hash);
	if (hash) settingsGet(vp, "lasttrack.track", &vp->gui.lastTrack);
	
#if 0
	PLAYLISTCACHE *plc = getDisplayPlaylist(vp);
	if (plc){
		TVIDEOOVERLAY *plyctrl = pageGetPtr(vp, PAGE_OVERLAY);

		if (overlayPlaylistListboxFill(vp, plyctrl->lbPlaylist, plc)){
			//vp->queuedPlaylist = playlistManagerGetPlaylistIndex(vp->plm, plc);
			setQueuedPlaylist(vp, plc);
			if (plc->parent)
				plc->parent->pr->selectedItem = playlistManagerGetPlaylistIndex(vp->plm, plc);
		}
#ifndef _DEBUG_
		playlistChangeEvent(vp, plc, 0);
#endif
	}
#endif

#if 1
	TFILEPANE *filepane = pageGetPtr(vp, PAGE_FILE_PANE);
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
#endif

	/*TEXPPANEL *expan = pageGetPtr(vp, PAGE_EXP_PANEL);
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
	}*/
	
	TEQ *eq = pageGetPtr(vp, PAGE_EQ);
	settingsGet(vp, "equalizer.preset", &eq->preset);
	if (eq->preset < 0 || eq->preset >= eqPresetGetTotal(eq))
		eq->preset = 0;
	eqBuild(eq, eq->preset);
	
	double valuef = 0.0;
	settingsGet(vp, "equalizer.preamp", &valuef);
	eqBandSet(eq, 0, valuef);

	str_list *strList = NULL;
	settingsGet(vp, "equalizer.band.", &strList);
	if (strList){
		for (int i = 0; i < strList->total && i < eq->tBands && i < EQBANDS_MAX; i++){
			char *name = cfg_configStrListItem(strList, i);
			if (name) eqBandSet(eq, i+1, atof(name));
		}
		cfg_configStrListFree(strList);
		my_free(strList);
	}

	settingsGet(vp, "meta.drawTrackbar", &vp->gui.drawMetaTrackbar);
	settingsGet(vp, "device.virtual.onScreenCtrlIcons", &vp->gui.drawControlIcons);
	vp->gui.drawControlIcons |= isVirtual(vp);

	int var = 0;
	settingsGet(vp, "device.sbui.killRzDKManagerOnConnectFailRetry", &var);
	vp->ml->killRzDKManagerOnConnectFailRetry = var&0x01;

	settingsGet(vp, "video.aspect.preset", &values);
	if (values){
		setAR(vp, aspectStrToIdx(values));
		my_free(values);
	}
	settingsGet(vp, "video.aspect.custom.ratio", &vp->ctx.aspect.ratio);
	settingsGet(vp, "video.aspect.custom.x", &vp->ctx.aspect.x);
	settingsGet(vp, "video.aspect.custom.y", &vp->ctx.aspect.y);
	settingsGet(vp, "video.aspect.custom.width", &vp->ctx.aspect.width);
	settingsGet(vp, "video.aspect.custom.height", &vp->ctx.aspect.height);
	settingsGet(vp, "video.aspect.custom.cleanSurface", &vp->ctx.aspect.clean);
	if (vp->ctx.aspect.ratio < 0.01) vp->ctx.aspect.ratio = 0.0;
	if (vp->ctx.aspect.x < 0) vp->ctx.aspect.x = 0;
	if (vp->ctx.aspect.y < 0) vp->ctx.aspect.y = 0;
	if (vp->ctx.aspect.width < 0) vp->ctx.aspect.width = 0;
	if (vp->ctx.aspect.height < 0) vp->ctx.aspect.height = 0;


	settingsGet(vp, "video.filter.rotate", &vp->vlc->rotateAngle);
	settingsGet(vp, "video.filter.scale", &vp->vlc->scaleFactor);
	settingsGet(vp, "video.filter.blur",	&vp->vlc->blurRadius);
	settingsGet(vp, "video.filter.pixelize", &vp->vlc->pixelize);
	settingsGet(vp, "video.filter.brightness", &vp->vlc->brightness);
	settingsGet(vp, "video.filter.contrast", &vp->vlc->contrast);
	settingsGet(vp, "video.filter.saturation", &vp->vlc->saturation);
	settingsGet(vp, "video.filter.gamma", &vp->vlc->gamma);
	settingsGet(vp, "video.filter.rotateOp", &vp->vlc->rotateOp);
	settingsGet(vp, "video.filter.scaleOp", &vp->vlc->scaleOp);
	if (vp->vlc->rotateOp < ROTATE_BILINEAR || vp->vlc->rotateOp > ROTATE_NEIGHBOUR)
		vp->vlc->rotateOp = ROTATE_BILINEAR;
	if (vp->vlc->scaleOp < SCALE_BILINEAR || vp->vlc->scaleOp > SCALE_NEIGHBOUR)
		vp->vlc->scaleOp = SCALE_BILINEAR;

	settingsGet(vp, "video.subtitle.delay", &vp->vlc->subtitleDelay);
	settingsGet(vp, "video.swapRB", &vp->vlc->swapColourBits);
	
	settingsGet(vp, "audio.desync", &vp->vlc->audioDesync);
	settingsGet(vp, "audio.visuals", &vp->gui.drawVisuals);
	if (vp->gui.drawVisuals)
		setBaseUpdateRate(UPDATERATE_BASE_VISUALS);
	else
		setBaseUpdateRate(UPDATERATE_BASE_DEFAULT);


	/*TGLOBALHOTKEYS *ghk = pageGetPtr(vp, PAGE_HOTKEYS);
	settingsGet(vp, "hotkeys.alwaysAccessible", &ghk->alwaysAccessible);
	settingsGet(vp, "hotkeys.showNames", &ghk->showNames);
	settingsGet(vp, "hotkeys.local.cursor", &vp->gui.hk_cursor);
	settingsGet(vp, "hotkeys.local.console", &vp->gui.hk_console);
	settingsGet(vp, "hotkeys.local.snapshot", &vp->gui.hk_snapshot);

	
	strList = NULL;
	settingsGet(vp, "hotkeys.global.", &strList);
	if (strList){
		ghkFreeKeyList(ghk->keys, ghk->totalKeys);
		ghk->keys = ghkAllocKeyList(strList->total);
		ghk->totalKeys = strList->total;
		int validKeys = 0;

		THOTKEY hk;
		char modifierStrA[64];
		char modifierStrB[64];
		char hkname[256];
		char imagePath[MAX_PATH_UTF8+1];		

		for (int i = 0; i < strList->total; i++){
			char *name = cfg_configStrListItem(strList, i);
			if (name){
				memset(&hk, 0, sizeof(hk));

				//     modifierA,modifierB,key,image_path
				// eg; CTRL,ALT,A,hotkeys/image1.png

				int slen = strlen(name);
				int c  = strcspn(name, ",");
				*modifierStrA = 0;
				strncpy(modifierStrA, name, c);
				if (*modifierStrA){
					modifierStrA[c] = 0;
					hk.modifierA = hkModiferToKey(modifierStrA);
				}
				int i = c + 1;
				
				if (i >= slen-1) continue;
				c = strcspn(name+i, ",");
				*modifierStrB = 0;
				strncpy(modifierStrB, name+i, c);
				if (*modifierStrB){
					modifierStrB[c] = 0;
					hk.modifierB = hkModiferToKey(modifierStrB);
				}
				i += c + 1;
				
				if (i >= slen-1) continue;
				c = strcspn(name+i, ",");
				hk.key = *(name+i);
				i += c + 1;
				
				
				if (i >= slen-1) continue;
				c = strcspn(name+i, ",");
				strncpy(hkname, name+i, c);
				if (*hkname){
					hkname[c] = 0;
					hk.name = my_strdup(hkname);
				}
				i += c + 1;
				
				if (i >= slen-1) continue;
				*imagePath = 0;
				strncpy(imagePath, name+i, slen-i);
				if (*imagePath){
					imagePath[slen-i] = 0;
					hk.imagePath = my_strdup(imagePath);
				}else{
					continue;
				}
				
				ghk->keys[validKeys] = ghkAllocKey();
				my_memcpy(ghk->keys[validKeys], &hk, sizeof(THOTKEY));

				//printf("name:'%s'\nmodA:%s/%i\nmodB:%s/%i\nkey:%c\nimage:%s\n\n", hk.name,
				//		  modifierStrA, hk.modifierA, modifierStrB, hk.modifierB, hk.key, hk.imagePath);

				validKeys++;
			}
			ghk->totalKeys = validKeys;
		}
		cfg_configStrListFree(strList);
		my_free(strList);
	}*/
	
	
	values = NULL;
	settingsGet(vp, "clock.type", &values);
	if (values){
		setClockType(vp, clockStringToType(values));
		my_free(values);
	}
	/*TCLK *clk = pageGetPtr(vp, PAGE_CLOCK);
	settingsGet(vp, "clock.face.dial.x", &clk->bfFace.cx);
	settingsGet(vp, "clock.face.dial.y", &clk->bfFace.cy);*/
	
	
	
	//eqApply(eq, vp->vlc, 1);
	//expanTimerPanelRebuild(vp);
	//configLoadBackground(vp);
	//configLoadSwatch(vp);
}

int settingsLoad (TSETTINGS *cfg, const wchar_t *filename)
{

	cfg->config = cfg_configCreate(cfg);
	cfg_configApplyDefaults(cfg->config);
	int entries = cfg_configRead(cfg->config, filename);

	if (!entries){
		//printf("config read failed. generating fresh config\n");
		
		cfg_configWrite(cfg->config, filename);
		entries = cfg_configRead(cfg->config, filename);

		//if (!entries)
		//	printf("config read failed\n");
		//else
		//	printf("config read complete (%i)\n", entries);
	}else{
		//printf("config read complete (%i)\n", entries);
	}

	return entries;
}

int settingsSave (TSETTINGS *cfg, const wchar_t *filename)
{
	int len = cfg_configWrite(cfg->config, filename);
	/*if (!len)
		printf("config write failed\n");
	else
		printf("config write complete (%i)\n", len);*/
	return len;
}

void configFree (TVLCPLAYER *vp)
{
	cfg_configFree(vp->settings.config);
}

