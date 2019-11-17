
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





static inline TFILEEXT *fileext_alloc ()
{
	return my_calloc(1, sizeof(TFILEEXT));
}

static inline void fileext_destroy (TFILEEXT *fileext)
{
	if (fileext)
		my_free(fileext);
}

static inline int chrct (char *str, const char chr)
{
	int ct = 0;
	while (*str)
		ct += (*(str)++ == chr);
	return ct;
}

static inline int fileext_extlistLineExtract (char *str, TFILEEXTLINE *extList)
{
	//str = strdup("<.exe.msi.msc.cpl.scr.cmd.elf.vbs.pif>module32.png");

	char *start = strchr(str, '<')+1;
	if (!start || !*start) return 0;
	char *end = strchr(start, '>');
	if (!end || !*end) return 0;
	char *image = end+1;
	if (!image || !*image) return 0;
	*end = 0;

	int total = chrct(start, '.');
	if (!total) return 0;

	extList->list = my_calloc(total, sizeof(char*));
	if (!extList->list) return 0;

	int ct = 0;
	char *exts = start;
	
	for (int i = 0; i < total; i++){
		char *ext = strrchr(exts, '.');
		if (!ext || !ext[1])
			printf("invalid file extension: '%s'<-\n", start);
		else
			extList->list[ct++] = my_strdup(ext);
		*ext = 0;
	}
	
	extList->total = ct;
	if (!extList->total)
		my_free(extList->list);
	else
		extList->image = converttow(image);
	
	return extList->total;
}

static inline void fileext_extlistLineFree (TFILEEXTLINE *extList)
{
	if (extList){
		if (extList->total){
			for (int i = 0; i < extList->total; i++)
				my_free(extList->list[i]);
			my_free(extList->list);
			my_free(extList->image);
		}
		my_free(extList);
	}
}

static inline void fileext_extlistFree (TFILEEXT *fileext)
{
	if (fileext->total){
		for (int i = 0; i < fileext->total; i++)
			fileext_extlistLineFree(fileext->exts[i]);
		my_free(fileext->exts);
	}
}

static inline int fileext_extlistBuild (TFILEEXT *fileext)
{
	fileext->total = 0;

	str_list *strList = NULL;
	cfg_keyGet(fileext->config, "file.ext.", &strList);
	
	if (strList){
		if (strList->total){
			fileext->total = strList->total;
			fileext->exts = my_calloc(fileext->total, sizeof(TFILEEXTLINE*));
		
			for (int i = 0; i < fileext->total; i++){
				fileext->exts[i] = my_calloc(1, sizeof(TFILEEXTLINE));
				if (!fileext->exts[i]){
					fileext->total = i;
					break;
				}
			
				char *str = cfg_configStrListItem(strList, i);
				if (!fileext_extlistLineExtract(str, fileext->exts[i])){
					my_free(fileext->exts[i]);
					fileext->total = i;
					break;
				}
			}

			if (!fileext->total)
				my_free(fileext->exts);
		}
		cfg_configStrListFree(strList);
	}

	return fileext->total;
}

static inline void fileext_commentsSetDefault (TCFGENTRY **config)
{
	cfg_commentSet(config, "ext.folder", "icon subdirectory");
}

static inline TCFGENTRY **fileext_cfgCreate (TFILEEXT *fileext)
{
	const TCFGENTRY config_cfg[] = {
		
		{"ext.folder", 				V_STR("pane"),				(void*)&fileext->cfg.folder},
		
		{" ", V_BRK(0), NULL},
		
		// set a few examples
		{"file.ext.",  V_SLIST5("<.mid>midi32.png", \
								"<.iso>archive32.png",\
								"<.exe.msi.msc.cpl.scr.cmd.elf.vbs.pif>module32.png",\
								"<.dll.sys.drv.vxd.acm.ax.ocx>moduledll32.png",\
								"<.pk3.pk4.bsp.cin.md2.md3.dm2.dm3.wad>quake32.png"),\
																(void*)&fileext->cfg.extlist},
		
		{NULL, V_INT32(0), NULL, 0, NULL}
	};


	TCFGENTRY **config = cfg_configDup(config_cfg);
	fileext_commentsSetDefault(config);
	return config;
}

static inline void fileext_cfgFree (TCFGENTRY **config)
{
	cfg_configFree(config);
}

static inline int fileext_cfgRead (TCFGENTRY **config, const wchar_t *configfile)
{
	int entries = cfg_configRead(config, configfile);
	if (!entries){
		cfg_configWrite(config, configfile);
		entries = cfg_configRead(config, configfile);
		if (!entries)
			__mingw_wprintf(L"problem reading fileext config '%ls'\n", configfile);
	}

	return entries;
}

TFILEEXT *fileext_load (const wchar_t *configfile)
{
	TFILEEXT *fileext = fileext_alloc();
	
	fileext->config = fileext_cfgCreate(fileext);
	cfg_configApplyDefaults(fileext->config);
	fileext_cfgRead(fileext->config, configfile);
	fileext_extlistBuild(fileext);
	//cfg_configWrite(fileext->config, configfile);
	return fileext;
}

void fileext_free (TFILEEXT *fileext)
{
	fileext_cfgFree(fileext->config);
	fileext_extlistFree(fileext);
	fileext_destroy(fileext);
}
