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

/*
#ifndef _strtoui64
_CRTIMP unsigned __int64 __cdecl _strtoui64(const char *_String,char **_EndPtr,int _Radix);
#endif
*/





static inline int calctotal (const TCFGENTRY *config_cfg)
{
	int ct = 0;
	while (config_cfg[ct].key) ct++;
	return ++ct; // +1 for NULL last item marker
}

static inline int calcTotal (const TCFGENTRY **config_cfg)
{
	int ct = 0;
	while (config_cfg[ct]->key) ct++;
	return ++ct; // +1 for NULL last item marker
}

int settingsGetW (TVLCPLAYER *vp, const char *key, void *value)
{
	int found = cfg_keyGetW(vp->settings.config, key, value);
	if (!found)
		printf("settingsGetW: invalid key: '%s'\n", key);	
	return found;
}

int settingsGet (TVLCPLAYER *vp, const char *key, void *value)
{
	int found = cfg_keyGet(vp->settings.config, key, value);
	if (!found)
		printf("settingsGet: invalid key: '%s'\n", key);	
	return found;
}

int settingsSet (TVLCPLAYER *vp, const char *key, void *value)
{
	int found = cfg_keySet(vp->settings.config, key, value);
	if (!found)
		printf("settingsSet: invalid key: '%s'\n", key);	
	return found;
}

char *settingsGetStr (TVLCPLAYER *vp, const char *key)
{
	char *value = NULL;
	settingsGet(vp, key, &value);
	return value;
}

wchar_t *settingsGetStrW (TVLCPLAYER *vp, const char *key)
{
	wchar_t *value = NULL;
	settingsGet(vp, key, &value);
	return value;
}


// get...
char *cfg_configStrListItem (str_list *strList, const int index)
{
	if (index < strList->total)
		return strList->strings[index];
	else
		return NULL;
}

void cfg_configStrListFree (str_list *strList)
{
	if (strList){
		for (int j = 0; j < strList->total; j++){
			if (strList->strings[j])
				my_free(strList->strings[j]);
		}
	}
}

str_list *cfg_configStrListNew (const int total)
{
	str_list *strList = my_calloc(1, sizeof(str_list));
	if (strList)
		strList->total = total;
	return strList;
}

str_list *cfg_configStrListDup (const str_list *strList)
{
	str_list *newList = cfg_configStrListNew(strList->total);
	if (newList){
		for (int i = 0; i < strList->total && strList->strings[i]; i++)
			newList->strings[i] = my_strdup(strList->strings[i]);
	}
	return newList;
}

str_list *cfg_configStrListDupW (str_list *strList)
{
	str_list *newList = cfg_configStrListNew(strList->total);
	if (newList){
		for (int i = 0; i < strList->total; i++)
			newList->strings[i] = (char*)converttow(strList->strings[i]);
	}
	return newList;
}

TCFGENTRY *cfg_entryDup (const TCFGENTRY *item)
{
	TCFGENTRY *entry = my_calloc(1, sizeof(TCFGENTRY));
	//printf("entry %p\n", entry);


	if (item->key){
		entry->key = my_strdup(item->key);
		entry->type = item->type;
		entry->hash = item->hash;
		if (!entry->hash)
			entry->hash = generateHash(entry->key, strlen(entry->key));
		if (item->comment)
			entry->comment = my_strdup(item->comment);

		switch (entry->type){
		  case CFG_INT:
			entry->u.val32 = item->u.val32;
			entry->ptr = item->ptr;
			break;

		  case CFG_INT64:
		  	entry->u.val64 = item->u.val64;
		  	entry->ptr = item->ptr;
			break;

		  case CFG_HEX:
		  	entry->u.valu64 = item->u.valu64;
		  	entry->ptr = item->ptr;
			break;

		  case CFG_STRING:
		  	entry->u.valStr = my_strdup(item->u.valStr);
		  	entry->ptr = my_strdup(item->u.valStr);
			break;

		  case CFG_FLOAT:
		  	entry->u.valFloat = item->u.valFloat;
		  	entry->ptr = item->ptr;
			break;

		  case CFG_DOUBLE:
		  	entry->u.valDouble = item->u.valDouble;
		  	entry->ptr = item->ptr;
			break;

		  case CFG_CHAR:
		  	entry->u.valChar = item->u.valChar;
		  	entry->ptr = item->ptr;
			break;

		  case CFG_STRLIST:
	  		entry->u.strList.total = item->u.strList.total;
		  	for (int i = 0; i < item->u.strList.total && item->u.strList.strings[i]; i++)
		  		entry->u.strList.strings[i] = my_strdup(item->u.strList.strings[i]);

		  	entry->ptr = cfg_configStrListDup(&item->u.strList);
			break;

		  case CFG_BREAK:
		  	break;
		}
	}
	return entry;
}

TCFGENTRY **cfg_configDup (const TCFGENTRY *config_cfg)
{
	const int total = calctotal(config_cfg);

	TCFGENTRY **config = my_calloc(total+1, sizeof(TCFGENTRY*));
    if (config){
    	for (int i = 0; i < total; i++)
	    	config[i] = cfg_entryDup(&config_cfg[i]);
	}
    return config;
}

void cfg_configFree (TCFGENTRY **config)
{
	int ct = 0;	// find upper NUL entry so we cna free it manually
	while (config[ct]->key) ct++;

	for (int i = 0; config[i]->key; i++){
		TCFGENTRY *entry = config[i];
		if (entry){
			if (entry->key){
				if (entry->type == CFG_STRING){
					if (entry->u.valStr)
						my_free(entry->u.valStr);
					if (entry->ptr)
						my_free(entry->ptr);

				}else if (entry->type == CFG_STRLIST){
					cfg_configStrListFree(&entry->u.strList);

					if (entry->ptr){
		  				cfg_configStrListFree(entry->ptr);
		  				my_free(entry->ptr);
				  	}
				}
				my_free(entry->key);
			}
			if (entry->comment)
				my_free(entry->comment);
			my_free(entry);
		}
	}
	my_free(config[ct]);
	my_free(config);
}

#if 0
void cfg_configDump (TCFGENTRY **config)
{
	for (int i = 0; config[i]->key; i++){
		if (config[i]->type == CFG_BREAK)
			printf("\n");
		else
			printf("%i: %s, %i, %p %X\n", i, config[i]->key, config[i]->type, config[i]->ptr, config[i]->hash);

		if (config[i]->type == CFG_INT){
			printf("\t%i %i\n", config[i]->u.val32, *(int32_t*)config[i]->ptr);

		}else if (config[i]->type == CFG_FLOAT){
			printf("\t%f %f\n", config[i]->u.valFloat, *(float*)config[i]->ptr);

		}else if (config[i]->type == CFG_DOUBLE){
			printf("\t%f %f\n", config[i]->u.valDouble, *(double*)config[i]->ptr);

		}else if (config[i]->type == CFG_STRING){
			if (config[i]->u.valStr)
				printf("\t'%s' '%s'\n", config[i]->u.valStr, (char*)config[i]->ptr);

		}else if (config[i]->type == CFG_STRLIST){
			str_list *strList = config[i]->ptr;
			for (int j = 0; j < strList->total; j++){
				if (strList->strings[j])
					printf("\t%i:%s\n",j, strList->strings[j]);
			}
		}
	}
}
#endif

void cfg_configApplyDefaults (TCFGENTRY **config)
{
	for (int i = 0; config[i]->key; i++){
		TCFGENTRY *entry = config[i];
	
		switch (entry->type){
		  case CFG_INT:
			*(int32_t*)entry->ptr = entry->u.val32;
			break;

		  case CFG_INT64:
		  	*(int64_t*)entry->ptr = entry->u.val64;
			break;

		  case CFG_HEX:
		  	*((uint64_t*)entry->ptr) = entry->u.valu64;
			break;

		  case CFG_STRING:
		  	if (entry->ptr) my_free(entry->ptr);
		  	entry->ptr = my_strdup(entry->u.valStr);
			break;

		  case CFG_FLOAT:
		  	*(float*)entry->ptr = entry->u.valFloat;
			break;

		  case CFG_DOUBLE:
		  	*(double*)entry->ptr = entry->u.valDouble;
			break;

		  case CFG_CHAR:
		  	*(char*)entry->ptr = entry->u.valChar;
			break;

		  case CFG_STRLIST:
		  	if (entry->ptr){
		  		cfg_configStrListFree(entry->ptr);
		  		my_free(entry->ptr);
		  	}
		  	str_list *strList = cfg_configStrListDup(&entry->u.strList);
		  	entry->ptr = strList;
			break;
		}
	}
}

str_keyvalue *findKey (const char *key, str_keyvalue *keys, const int ktotal)
{
#if 1
	const int keylen = strlen(key);

	for (int i = 0; i < ktotal; i++){
		if (!strncmp(key, keys[i].key, keylen))
			return &keys[i];
	}
#else
	for (int i = 0; i < ktotal; i++){
		if (keys[i].hash == hash)
			return &keys[i];
	}
#endif
	return NULL;
}

str_keyvalue *findKeyNext (const char *key, str_keyvalue *keys, const int ktotal, str_keyvalue *next)
{
	for (int i = 0; i < ktotal-1; i++){
		if (&keys[i] == next){
			i++;
			return findKey(key, &keys[i], ktotal-i);
		}
	}
	return NULL;
}

int findKeyListTotal (const char *key, str_keyvalue *keys, const int ktotal)
{
	int total = 0;
	int actual = 0;
	for (int i = 0; i < ktotal-1; i++){
		str_keyvalue *found = findKey(key, &keys[i], ktotal-i);
		if (found){
			total++;

			char *pt = strrchr(found->key, '.');
			if (pt && *pt){
				pt++;
				if (*pt && isdigit(*pt))
					actual++;
			}
			while((found=findKeyNext(key, keys, ktotal, found))){
				char *pt = strrchr(found->key, '.');
				if (pt && *pt){
					pt++;
					if (*pt && isdigit(*pt))
						actual++;
				}
				total++;
			}
			break;
		}
	}
	return actual;
}

int cfg_configRead (TCFGENTRY **config, const wchar_t *filename)
{

	TASCIILINE *al = readFileW(filename);
	if (!al) return 0;
	if (al->tlines < 10){
		freeASCIILINE(al);
		return 0;
	}

	const int tlines = al->tlines;
	str_keyvalue keys[tlines];
	memset(keys, 0, sizeof(keys));

	int i = 0;
	for (int j = 0; j < tlines; j++){
		if (!*al->line[j] || *al->line[j] == CFG_COMMENTCHAR || *al->line[j] == ' ')
			continue;

		keys[i].key = strtok((char*)al->line[j], CFG_SEPARATOR);
		if (keys[i].key){
			keys[i].key = removeLeadingSpaces(removeTrailingSpaces(keys[i].key));
			//keys[i].hash = generateHash(keys[i].key, strlen(keys[i].key));
			keys[i].value = strtok(NULL, CFG_COMMENT);
			if (keys[i].value){
				keys[i].value = removeLeadingSpaces(removeTrailingSpaces(keys[i].value));
				if (*keys[i].value) i++;
			}
		}
	}
	const int ktotal = i;


	for (int i = 0; config[i]->key; i++){
		TCFGENTRY *entry = config[i];

		str_keyvalue *key = findKey(entry->key, keys, ktotal);
		if (!key) continue;

		//printf("%i '%s'\n", entry->hash, entry->key);

		switch (entry->type){
		  case CFG_INT:
			*(int32_t*)entry->ptr = atoi(key->value);
			break;

		  case CFG_INT64:
		  	*(int64_t*)entry->ptr = _atoi64(key->value);
			break;

		  case CFG_HEX:
		  	sscanf(key->value, "%I64X", (uint64_t*)entry->ptr);
			break;

		  case CFG_STRING:
		  	if (entry->ptr) my_free(entry->ptr);
		  	entry->ptr = my_strdup(key->value);
			break;

		  case CFG_FLOAT:
		  	*(float*)entry->ptr = (float)atof(key->value);
			break;

		  case CFG_DOUBLE:
		  	*(double*)entry->ptr = (double)atof(key->value);
			break;

		  case CFG_CHAR:
		  	*(char*)entry->ptr = key->value[0];
			break;

		  case CFG_STRLIST:{
		  	int total = findKeyListTotal(entry->key, keys, ktotal);
		  	//printf("strList total %i %X '%s'\n", total, entry->hash, entry->key);

		  	if (total){
		  		str_list *strList = my_calloc(1, sizeof(str_list));
		  		if (strList){
			  		strList->total = total;

			  		for (int i = 0; i < total && key; i++){
			  			strList->strings[i] = my_strdup(key->value);
		  				key = findKeyNext(entry->key, keys, ktotal, key);
		  			}

		  			if (entry->ptr){
		  				cfg_configStrListFree(entry->ptr);
		  				my_free(entry->ptr);
		  			}
		  			entry->ptr = strList;
		  		}
		  	}
			break;
		  }
		}
	}

	freeASCIILINE(al);
	return tlines;
}

static inline int settingsWriteUtf8Marker (FILE *fp)
{
	return fprintf(fp,"﻿");
}

static inline int settingsWriteBreak (FILE *fp)
{
	return fprintf(fp, "\r\n");
}

static inline int settingsWriteString (FILE *fp, const char *buffer)
{
	fprintf(fp, "%s", buffer);
	return settingsWriteBreak(fp);
}

static inline int settingsWriteLine (FILE *fp, char *buffer, const int blen, const char *comment)
{
	if (!comment){
		return settingsWriteString(fp, buffer);
	}else{
		int flen = CFG_COMMENTCOL - blen;
		if (flen <= 0) flen = 4;

		char filler[flen+1];
		memset(filler, 32, flen);

		if (!blen){
			buffer = "";
			flen = 0;
		}
		filler[flen] = 0;
		fprintf(fp, "%s%s%c  %s", buffer, filler, CFG_COMMENTCHAR, comment);
		return settingsWriteBreak(fp);
	}
}

static inline int settingsWriteStrList (FILE *fp, const char *key, const str_list *strList)
{
	char buffer[MAX_PATH_UTF8*2];
	int written = 0;

	for (int j = 0; j < strList->total && strList->strings[j]; j++){
		int slen = __mingw_snprintf(buffer, sizeof(buffer), "%s%i"CFG_SEPARATOR" %s", key, j+1, strList->strings[j]);
		written += settingsWriteLine(fp, buffer, slen, NULL);
	}
	return written;
}

int settingsWriteKeys (FILE *fp, TCFGENTRY **config)
{
	char buffer[MAX_PATH_UTF8+1];
	int slen = 0;

	for (int i = 0; config[i]->key; i++){
		TCFGENTRY *entry = config[i];

		switch (entry->type){
		  case CFG_INT:
		  	slen = __mingw_snprintf(buffer, sizeof(buffer), "%s"CFG_SEPARATOR" %i", entry->key, *(int32_t*)entry->ptr);
		  	settingsWriteLine(fp, buffer, slen, entry->comment);
			break;

		  case CFG_INT64:{
		  	char buffer64[64];
		  	_i64toa(*(int64_t*)entry->ptr, buffer64, 10);
		  	//slen = _snprintf(buffer, sizeof(buffer), "%s"CFG_SEPARATOR" %I64i", entry->key, *(int64_t*)entry->ptr);
		  	slen = __mingw_snprintf(buffer, sizeof(buffer), "%s"CFG_SEPARATOR" %s", entry->key, buffer64);
		  	settingsWriteLine(fp, buffer, slen, entry->comment);
			break;
		  }
		  case CFG_HEX:{
		  	uint64_t val = *((uint64_t*)entry->ptr);
		  	ULARGE_INTEGER ui = (ULARGE_INTEGER)val;
		  	//slen = _snprintf(buffer, sizeof(buffer), "%s"CFG_SEPARATOR" %I64X", entry->key, val);
		  	if (ui.HighPart)
		  		slen = __mingw_snprintf(buffer, sizeof(buffer), "%s"CFG_SEPARATOR" %X%X", entry->key, (unsigned int)ui.HighPart, (unsigned int)ui.LowPart);
		  	else
		  		slen = __mingw_snprintf(buffer, sizeof(buffer), "%s"CFG_SEPARATOR" %X", entry->key, (unsigned int)ui.LowPart);
		  	//printf("i64 #%s# %I64X\n", buffer, val);
		  	settingsWriteLine(fp, buffer, slen, entry->comment);
		  }
			break;

		  case CFG_STRING:{
		  	char *str = entry->ptr;
		  	if (!str) str = entry->u.valStr;

		  	slen = __mingw_snprintf(buffer, sizeof(buffer), "%s"CFG_SEPARATOR" %s", entry->key, str);
		  	settingsWriteLine(fp, buffer, slen, entry->comment);
			break;
		  }
		  case CFG_FLOAT:
		  	slen = __mingw_snprintf(buffer, sizeof(buffer), "%s"CFG_SEPARATOR" %f", entry->key, *(float*)entry->ptr);
		  	settingsWriteLine(fp, buffer, slen, entry->comment);
			break;

		  case CFG_DOUBLE: 
		  	slen = __mingw_snprintf(buffer, sizeof(buffer), "%s"CFG_SEPARATOR" %f", entry->key, *(double*)entry->ptr);
		  	//printf("writeDouble: %s: %f '%s' #%s#\n", entry->key, *(double*)entry->ptr, buffer, CFG_SEPARATOR);
		  	settingsWriteLine(fp, buffer, slen, entry->comment);
			break;

		  case CFG_CHAR:
		  	slen = __mingw_snprintf(buffer, sizeof(buffer), "%s"CFG_SEPARATOR" %c", entry->key, *(char*)entry->ptr);
		  	settingsWriteLine(fp, buffer, slen, entry->comment);
			break;

		  case CFG_STRLIST:{
		  	str_list *strList = entry->ptr;
			if (!strList) strList = &entry->u.strList;

			settingsWriteStrList(fp, entry->key, strList);
			break;
		  }
		  case CFG_BREAK:
		  	settingsWriteBreak(fp);
		  	break;
		}
	}
	return ftell(fp);
}

int cfg_configWrite (TCFGENTRY **config, const wchar_t *filename)
{

	FILE *fp = _wfopen(filename, L"w+b");
	if (!fp) return 0;

	settingsWriteUtf8Marker(fp);
	settingsWriteBreak(fp);
	settingsWriteBreak(fp);
	settingsWriteLine(fp, NULL, 0, "All paths' are UTF8 encoded");
	settingsWriteBreak(fp);
	settingsWriteBreak(fp);
	settingsWriteKeys(fp, config);
	settingsWriteBreak(fp);

	int len = ftell(fp);
	fclose(fp);
	return len;
}

TCFGENTRY *cfg_keyFind (TCFGENTRY **config, const char *key)
{
	const int hash = generateHash(key, strlen(key));
	for (int i = 0; config[i]->key; i++){
		TCFGENTRY *entry = config[i];
		if (hash == entry->hash)
			return entry;
	}
	return NULL;
}

int cfg_keyGet (TCFGENTRY **config, const char *key, void *value)
{
	TCFGENTRY *entry = cfg_keyFind(config, key);

	//printf("cfg_keyGet '%s' %p\n", key, entry);

	if (entry){
		switch (entry->type){
		  case CFG_INT:
			*(int32_t*)value = *(int32_t*)entry->ptr;
			break;

		  case CFG_INT64:
		  	*(int64_t*)value = *(int64_t*)entry->ptr;
			break;

		  case CFG_HEX:
		  	*(uint64_t*)value = *(uint64_t*)entry->ptr;
			break;

		  case CFG_STRING:
		  	*(char**)value = my_strdup(entry->ptr);
		  	//printf("CFG_STRING #%s# %p\n", key, *(char**)value);
			break;

		  case CFG_FLOAT:
		  	*(float*)value = *(float*)entry->ptr;
			break;

		  case CFG_DOUBLE:
		  	*(double*)value = *(double*)entry->ptr;
			break;

		  case CFG_CHAR:
		  	*(char*)value = *(char*)entry->ptr;
			break;

		  case CFG_STRLIST:
		  	*(str_list**)value = cfg_configStrListDup(entry->ptr);
			break;
		}
	}
	return entry != NULL;
}

int cfg_keyGetW (TCFGENTRY **config, const char *key, void *value)
{
	TCFGENTRY *entry = cfg_keyFind(config, key);
	if (entry){
		switch (entry->type){
		  case CFG_INT:
			*(int32_t*)value = *(int32_t*)entry->ptr;
			break;

		  case CFG_INT64:
		  	*(int64_t*)value = *(int64_t*)entry->ptr;
			break;

		  case CFG_HEX:
		  	*(uint64_t*)value = *(uint64_t*)entry->ptr;
			break;

		  case CFG_STRING:
		  	*(wchar_t**)value = converttow(entry->ptr);
			break;

		  case CFG_FLOAT:
		  	*(float*)value = *(float*)entry->ptr;
			break;

		  case CFG_DOUBLE:
		  	*(double*)value = *(double*)entry->ptr;
			break;

		  case CFG_CHAR:
		  	*(wchar_t*)value = *(char*)entry->ptr;
			break;

		  case CFG_STRLIST:
		  	*(str_list**)value = cfg_configStrListDupW(entry->ptr);
			break;
		}
	}
	return entry != NULL;
}

int cfg_keySet (TCFGENTRY **config, const char *key, void *value)
{
	TCFGENTRY *entry = cfg_keyFind(config, key);
	if (entry){
		switch (entry->type){
		  case CFG_INT:
			*(int32_t*)entry->ptr = *(int32_t*)value;
			break;

		  case CFG_INT64:
		  	*(int64_t*)entry->ptr = *(int64_t*)value;
			break;

		  case CFG_HEX:
		  	*(uint64_t*)entry->ptr = *(uint64_t*)value;
			break;

		  case CFG_STRING:
		  	if (entry->ptr) my_free(entry->ptr);
		  	entry->ptr = my_strdup(value);
		  	//printf("string set for '%s'\n", (char*)entry->ptr);
			break;

		  case CFG_FLOAT:
		  	*(float*)entry->ptr = *(float*)value;
			break;

		  case CFG_DOUBLE:
		  	*(double*)entry->ptr = *(double*)value;
		  	//printf("setting double %p %f\n", entry->ptr, (double)*(double*)entry->ptr);
			break;

		  case CFG_CHAR:
		  	*(char*)entry->ptr = *(char*)value;
			break;

		  case CFG_STRLIST:
		  	if (entry->ptr){
		  		cfg_configStrListFree(entry->ptr);
		  		my_free(entry->ptr);
		  	}
		  	//entry->ptr = cfg_configStrListDup(value);
		  	entry->ptr = value;
			break;
		}
	}

	return (entry != NULL);
}

char *cfg_commentGet (TCFGENTRY **config, char *key)
{
	TCFGENTRY *entry = cfg_keyFind(config, key);
	if (entry && entry->comment)
		return  my_strdup(entry->comment);
	return NULL;
}

int cfg_commentSet (TCFGENTRY **config, char *key, char *comment)
{
	TCFGENTRY *entry = cfg_keyFind(config, key);
	if (entry){
		if (entry->comment)
			my_free(entry->comment);
		entry->comment = my_strdup(comment);
	}else{
		printf("cfg_commentSet: invalid key: '%s'\n", key);
	}
	return (entry != NULL);
}

void cfg_commentsSetDefault (TCFGENTRY **config)
{
	//cfg_commentSet(config, "general.visual", "Visualization to use during audio only playback (0-9, 0 = Display meta information)");
	cfg_commentSet(config, "general.idleTimeout", "Inactivity time before idle mode is enabled, in seconds");
	cfg_commentSet(config, "general.idleFps", "Update rate during idle period");
	cfg_commentSet(config, "general.overlayPeriod", "Overlay period in milliseconds");
	cfg_commentSet(config, "general.randomTrack", "Random track playback");
	
	cfg_commentSet(config, "volume.last", "Application volume 0-100 (0 = mute)");
	cfg_commentSet(config, "volume.digits", "Folder within /common/digits/ containing digit bitmaps");
	cfg_commentSet(config, "volume.changeDelta", "Scrollwheel adjustment amount");

	cfg_commentSet(config, "vlc.playbackArguments", "Pass these options when diverting playback to VLC");
	cfg_commentSet(config, "vlc.startArguments", "Pass these options when launching VLC (without playback)");
	cfg_commentSet(config, "vlc.stopOnVlcPlayback", "Halt local playback when diverted to VLC");

	cfg_commentSet(config, "home.allowKeypad", "Allow access to keypad from home page (won't do very much)");
	cfg_commentSet(config, "home.enableHRM", "Enable ANT+ HRM display. Ant+ USB dongle required");
	cfg_commentSet(config, "home.hotkeys.alwaysAccessible", "Should the hotkeys page be available when VLC is not running");
	
	cfg_commentSet(config, "taskman.cpugraph.enable", "Display a realtime CPU usuage graph");
	cfg_commentSet(config, "taskman.cpugraph.mode", "1:Single graph per CPU, 2:Individual graph per core (default:2)");
	
	cfg_commentSet(config, "artwork.searchDepth", "Search depth for album artwork");
	cfg_commentSet(config, "artwork.maxWidth", "Maximum width of artwork");
	cfg_commentSet(config, "artwork.maxHeight", "Maximum height of artwork");
	//cfg_commentSet(config, "artwork.threads", "Number of threads dedicated to acquring and loading artwork");
	cfg_commentSet(config, "artwork.panelImgSize", "Panel playlist image size");

	cfg_commentSet(config, "lasttrack.playlist", "Playlist index");
	cfg_commentSet(config, "lasttrack.track", "Track position");
	cfg_commentSet(config, "lasttrack.hash", "Path hash");
	
	cfg_commentSet(config, "systray.enabled", "Enable systray icon, useful for playlist browsing");
	
	cfg_commentSet(config, "taskbar.enabled", "Print track meta on to a Windows toolbar/taskbar");
	cfg_commentSet(config, "taskbar.toolbarName", "Print track meta on to this tookbar. (Default: "WINTOOLBAR_NAME")");
	cfg_commentSet(config, "taskbar.pos.x", "Print location along the horiztonal axis (Ideally n=icon count*icon width)");
	cfg_commentSet(config, "taskbar.pos.y", "Print location along the vertical axis. (May be negative)");
	cfg_commentSet(config, "taskbar.string.trackNo", "Print track number");
	cfg_commentSet(config, "taskbar.string.title", "Print track title");
	cfg_commentSet(config, "taskbar.string.artist", "Print artist, if available");
	cfg_commentSet(config, "taskbar.string.album", "Print album, if available");
	cfg_commentSet(config, "taskbar.string.description", "Print track meta description, if any");
	cfg_commentSet(config, "taskbar.string.path", "Print complete path of item");
	cfg_commentSet(config, "taskbar.colour.fore", "Ink colour (RGB:0xRRGGBB)");
	cfg_commentSet(config, "taskbar.colour.back", "Background rectangle colour when in Opaque mode (RGB)");
	cfg_commentSet(config, "taskbar.colour.mode", "1:Transparent background, 2:Opaque background");
	cfg_commentSet(config, "taskbar.font.name", "Complete Windows font name");
	cfg_commentSet(config, "taskbar.font.width", "Width of font when pointsize is 0");
	cfg_commentSet(config, "taskbar.font.height", "Height of font when pointsize is 0");
	cfg_commentSet(config, "taskbar.font.point", "Font pointsize. 0:Use width/height. (18 works for me)");
	cfg_commentSet(config, "taskbar.font.weight", "Text thickness. (100 to 900, Default: 500)");
	cfg_commentSet(config, "taskbar.font.quality", "3:Non AA, 4:AA, 5:Cleartype, 6:Natural Cleartype. (Default: 6)");

	cfg_commentSet(config, "browser.filterBy", "audio, video, playlists, image, media and all");
	cfg_commentSet(config, "browser.sortBy", "none, namea/d, modifieda/d, createda/d, sizea/d and typea/d (a=ascending:d=descending. eg; named)");
	cfg_commentSet(config, "browser.showFileSize", "Display filesize next to filename");
	cfg_commentSet(config, "browser.showRemotePath", "Display complete network path of remote drives/folders");
	cfg_commentSet(config, "browser.showRemoteComputer", "Display remote computer name in place of drive letter");
	cfg_commentSet(config, "browser.showLocalComputerName", "Display computer name (as found on workgroup) with my computer name");
	cfg_commentSet(config, "browser.showDriveFreeSpace", "Display available drive space next to drive letter");

	cfg_commentSet(config, "skin.folder", "Skin location");
	
	
	cfg_commentSet(config, "search.metaDepth", "Preload depth. Derfault is 2 branches deep");
	cfg_commentSet(config, "search.ignorecase", "String matching should be case [in]sensitive. Default is 1");
	
	

#if ENABLE_ANTPLUS
	cfg_commentSet(config, "hrm.device.vid", "USB vender Id of device. Default: 4047 (decimal)");
	cfg_commentSet(config, "hrm.device.pid", "USB product Id of device. Default: 4104 (decimal)");
	cfg_commentSet(config, "hrm.device.index", "ANT+ device (0 = first device, 1 = next device, etc..)");
	cfg_commentSet(config, "hrm.device.key", "ANT+ device key (default:"ANT_DEFAULTKEY")");
	cfg_commentSet(config, "hrm.sensor.id", "Search for and pair with this sensor. Default: 0 = anything/anyone");
	cfg_commentSet(config, "hrm.activateOnInsertion", "Attempt to claim dongle upon insertion");
	cfg_commentSet(config, "hrm.enableOverlay", "Display HR value on every page");
#endif

	cfg_commentSet(config, "equalizer.preset", "0=Off/Flat, 1=Classical, 2=Club, Dance, Full bass, Full bass treble, Full treble, Headphones, Large hall, Live, Party, Pop, Reggae, Rock, Ska, Soft, Soft rock, 17=Techno");

	cfg_commentSet(config, "video.aspect.preset", "auto, 16:9, 1.33, etc..");
	cfg_commentSet(config, "video.filter.rotate", "Degree: -180.0 to +180.0");
	cfg_commentSet(config, "video.filter.scale", "0.03 to 4.0");
    cfg_commentSet(config, "video.filter.blur", "Radius: 0 to 64 (op:stackfast)");
    cfg_commentSet(config, "video.filter.pixelize", "Block size: 0 to 16");
    cfg_commentSet(config, "video.filter.brightness", "0.0 to 2.0");
    cfg_commentSet(config, "video.filter.contrast", "0.0 to 4.0");
    cfg_commentSet(config, "video.filter.saturation", "0.0 to 4.0");
    cfg_commentSet(config, "video.filter.gamma", "0.0 to 4.0");
	cfg_commentSet(config, "video.filter.rotateOp", "1:Bilinear, 2:Bicubic, 3:Nearest Neighbour");
	cfg_commentSet(config, "video.filter.scaleOp", "1:Bilinear, 2:Bicubic, 3:Nearest Neighbour");

	cfg_commentSet(config, "video.subtitle.delay", "Delay subtitle(s) apperence by n milliseconds");
	cfg_commentSet(config, "video.swapRB", "Swap Blue and Red colour components of video playback");

	cfg_commentSet(config, "audio.desync", "delay audio by this amount in milliseconds. (usually renderTime+displayTime)");
	cfg_commentSet(config, "audio.visuals", "en[dis]able audio spectrum (is not compiled in)");

	cfg_commentSet(config, "hotkeys.local.cursor", "Hook mouse for UI control (+ Ctrl + Shift)");
	cfg_commentSet(config, "hotkeys.local.console", "Enable internal console (+ Ctrl + Shift)");
	cfg_commentSet(config, "hotkeys.global.showLabels", "Displays the hotkey name below icon");

	cfg_commentSet(config, "display.device", "Use this (libmylcd) driver. Defaults to 'ddraw' if unavailable");
	cfg_commentSet(config, "display.width", "W/H of external display (where not fixed) otherwise W/H of virtual (desktop) display");
	cfg_commentSet(config, "display.height", "Bad things happen when either W/H is larger than Desktop size, applies to virtual display only");
	cfg_commentSet(config, "display.backlight", "Where supported, Sets the backlight level of display. eg; USBD480");
	
	cfg_commentSet(config, "device.sbui.killRzDKManagerOnConnectFailRetry", "Sometimes RzDKManager.exe falls over, help it back up");
	cfg_commentSet(config, "device.virtual.onScreenCtrlIcons", "Overlay control icons on the playback screen when in virtual mode");
	cfg_commentSet(config, "device.virtual.restrictWindowSize", "Constrain window size to within desktop limits");
	
	cfg_commentSet(config, "clock.type", "Clock type. Digital, BoxDigital, Analogue, Butterfly, Polar or Predator");
	cfg_commentSet(config, "clock.digital.overlap", "Set by how many pixels chars will overlap (Default is 20)");
	cfg_commentSet(config, "clock.digital.digits", "Image data location");
	
	//cfg_commentSet(config, "alarm.enabled", "");
	cfg_commentSet(config, "alarm.time", "24hr clock time. Defaults to "ALARM_INITIALTIME);
	cfg_commentSet(config, "alarm.period", "Once only, Daily (every day) or Weekly (as configured below with 'alarm.weekly.1: Wed', etc..)");
	cfg_commentSet(config, "alarm.action.mode", "What happens when fired. Only 'Playtrack' supported thus far");
	cfg_commentSet(config, "alarm.action.playtrack.title", "Name of playlist entry, _not_ the album name");
	cfg_commentSet(config, "alarm.action.playtrack.uid", "Playlist Id. UID will only change if playlists' are added/removed. Use either this or Title");
	cfg_commentSet(config, "alarm.action.playtrack.track", "Start playing at this track. First track is 1, 2'nd is 2, etc..");
	cfg_commentSet(config, "alarm.action.playtrack.volume", "Sets media playback volume");

	cfg_commentSet(config, "alarm.action.starttask.path", "Complete path to module");
	cfg_commentSet(config, "alarm.action.endtask.pid", "End this process");
	cfg_commentSet(config, "alarm.action.endtask.module", "End all processes containing this module (path)");
	cfg_commentSet(config, "alarm.action.flash.colour1", "ARGB");
	cfg_commentSet(config, "alarm.action.flash.colour2", "ARGB");
	cfg_commentSet(config, "alarm.action.flash.period", "Display/show each colour for this length of time (ms)");
	cfg_commentSet(config, "alarm.action.flash.repeat", "Loop flash n times (eg; repeat:25/period:200 = 10Second flash)");

}

TCFGENTRY **cfg_configCreate (TSETTINGS *cfg)
{
	// _must_ be const
	const TCFGENTRY config_cfg[] = {
    	{"display.device",						V_STR(DEVICE_DEFAULT_NAME),		&cfg->display.device},
    	{"display.width",						V_INT32(DEVICE_DEFAULT_WIDTH),	&cfg->display.width},
    	{"display.height",						V_INT32(DEVICE_DEFAULT_HEIGHT),	&cfg->display.height},
    	{"display.backlight",					V_INT32(80),					&cfg->display.backlight},

    	{" ", V_BRK(0), NULL},

		{"device.sbui.killRzDKManagerOnConnectFailRetry", V_INT32(1), 			&cfg->device.sbui.killRzDKManagerOnConnectFailRetry},
		{"device.virtual.onScreenCtrlIcons",	V_INT32(1),			 			&cfg->device.virtual.enableCtrlButtons},
		{"device.virtual.restrictWindowSize",	V_INT32(1),						&cfg->device.virtual.restrictSize},
		
#if 0
    								  // function <|> key up <|> key down/pressed
    	{"device.sbui.dk.",	  V_SLIST10("PrevTrack<|>prev_up.png<|>prev_down.png",\
										"PlayPause<|>play_up.png<|>play_down.png",\
										"Stop<|>stop_up.png<|>stop_down.png",\
										"NextTrack<|>next_up.png<|>next_down.png",\
										"Album<|>playlist_up.png<|>playlist_down.png",\
										"Home<|>home_up.png<|>home_down.png",\
										"MCtrl<|>ctrl_up.png<|>ctrl_down.png",\
										"Browser<|>browser_up.png<|>browser_down.png",\
										"VLC<|>svlc_up.png<|>svlc_down.png",\
										"Quit<|>exit_up.png<|>exit_down.png"), (void*)&cfg->device.sbui.dkCfg},

#endif
    	{" ", V_BRK(0), NULL},
    	
    	{"skin.folder",				V_STR(SKINDEFAULTA),			&cfg->skin.folder},
    	{"skin.swatch",				V_STR("common\\swatch.png"),	&cfg->skin.swatch},
		{"skin.bgimage.", V_SLIST12("backgrounds\\bgimage1.png",
									"backgrounds\\bgimage2.png",
									"backgrounds\\bgimage3.png",
									"backgrounds\\bgimage4.png",
									"backgrounds\\bgimage5.png",
									"backgrounds\\bgimage6.png",
									"backgrounds\\bgimage7.png",
									"backgrounds\\bgimage8.png",
									"backgrounds\\bgimage9.png",
									"backgrounds\\bgimage10.png",
									"backgrounds\\bgimage11.png",
									"backgrounds\\bgimage12.png"), (void*)&cfg->skin.bgImageList},

    	{" ", V_BRK(0), NULL},


    	//{"general.visual",   	    	V_INT32(0),					&cfg->general.visual},
    	{"general.showStats",      		V_INT32(0),					&cfg->general.showStats},
    	{"general.idleTimeout",			V_INT32((IDLETIME/1000)),	&cfg->general.idleTimeout},
    	{"general.idleFps",				V_DBL(UPDATERATE_IDLE),		&cfg->general.idleFps},
    	{"general.overlayPeriod",		V_INT32(MCTRLOVERLAYPERIOD),&cfg->general.overlayPeriod},
    	{"general.randomTrack",			V_INT32(0),					&cfg->general.randomTrack},
    	{"general.runCount",			V_INT32(0),					&cfg->general.runCount},
    			
    	{" ", V_BRK(0), NULL},
    	
    	{"volume.last",					V_INT32(75),				&cfg->volume.last},
    	{"volume.changeDelta",			V_INT32(3),					&cfg->volume.changeDelta},
		{"volume.digits",				V_STR(VOL_DIGITS),			&cfg->volume.digits},
    	
    	{" ", V_BRK(0), NULL},
    	
    	{"vlc.startArguments",  		V_STR("--qt-system-tray --vout=any"), &cfg->vlc.startVlcArguments},
    	{"vlc.playbackArguments",  		V_STR("--qt-system-tray --vout=any --play-and-exit"), &cfg->vlc.extVlcArguments},
    	{"vlc.stopOnVlcPlayback",		V_INT32(1),					&cfg->vlc.stopLocalPlayback},

		{" ", V_BRK(0), NULL},

    	{"home.allowKeypad",			 V_INT32(0),			&cfg->home.showKeypad},
    	{"home.enableHRM",				 V_INT32(0),			&cfg->home.showAntplus},
    	{"home.enableTCX",				 V_INT32(0),			&cfg->home.showTcx},
		{"home.hotkeys.alwaysAccessible",V_INT32(0),			&cfg->home.hotkeysAlwaysAccessible},
		
		{"home.layout.rowSpace",		 V_INT32(24),			&cfg->home.layout.rowSpace},
		{"home.layout.columnSpace",		 V_INT32(24),			&cfg->home.layout.columnSpace},
		{"home.layout.btnsPerRow",		 V_INT32(5),			&cfg->home.layout.btnsPerRow},
		{"home.layout.autoExpand",		 V_INT32(1),			&cfg->home.layout.autoExpand},
		
    	{" ", V_BRK(0), NULL},

    	{"browser.filterBy",			 V_STR("media"),		&cfg->browser.filterBy},
    	{"browser.sortBy",				 V_STR("namea"),		&cfg->browser.sortBy},
    	{"browser.showFileSize",		 V_INT32(0),			&cfg->browser.showFileSize},
    	{"browser.showRemotePath",		 V_INT32(0),			&cfg->browser.showRemotePath},
    	{"browser.showRemoteComputer",	 V_INT32(0),			&cfg->browser.showRemoteComputer},
    	{"browser.showLocalComputerName",V_INT32(0),			&cfg->browser.showLocalComputerName},
    	{"browser.showDriveFreeSpace",	 V_INT32(0),			&cfg->browser.showDriveFreeSpace},
    	
    	{"browser.shortcut.",
#if RELEASEBUILD
    						   V_SLIST3("Music (example)<|>c:\\my music",\
    									"Movies (eg)<|>c:\\my movies",\
    									"Downloads (eg)<|>c:\\my downloads"),
#else
    						   V_SLIST3("Music<|>o:\\music",\
    									"Movies<|>s:\\movies",\
    									"Downloads<|>p:\\downloads"),
#endif
    									(void*)&cfg->browser.shortcutList},

    	{"browser.module.",
#if RELEASEBUILD
							   V_SLIST8("DirectShow<|>dshow://",\
    									"Desktop<|>screen://",\
    									"&#65520;&#65521;&#65522;:BBC (eg)<|>dvb-t://frequency=522000000 <bandwidth=8|program=4221|no-spu>",\
    									"A video (eg)<|>c:\\my videos\\a video.mkv",\
    									"An image (eg)<|>vsskin\\FlatLite800\\backgrounds\\bgimage9.png",\
    									"An audio track<|>c:\\my music\\audio.mp3",\
    									"Import folder<|>c:\\my music\\",\
										"Import playlist<|>stations.m3u8"),
#else
    						  V_SLIST11("DirectShow<|>dshow://",\
    									"Desktop<|>screen://",\
    									"&#65520;&#65521;&#65522;:BBC 1<|>dvb-t://frequency=522000000 <bandwidth=8|program=4221|no-spu>",\
    									"&#65520;&#65521;&#65522;:BBC 2<|>dvb-t://frequency=522000000 <bandwidth=8|program=4285|no-spu>",\
    									"&#65520;&#65521;&#65522;:UTV<|>dvb-t://frequency=474166000 <bandwidth=8|program=8276|no-spu>",\
    									"&#65520;&#65521;&#65522;:Channel 4<|>dvb-t://frequency=474166000 <bandwidth=8|program=8384|no-spu>",\
    									"An image<|>P:\\Downloads\\walls\\aya007.jpg",\
    									"Video track<|>S:\\Random\\5 Centimeters Per Second\\5_Centimeters_Per_Second.mkv",\
    									"Audio track<|>O:\\Music\\Misc\\Oasis_Do you Know What I Mean.mp3",\
    									"A folder<|>O:\\Unsorted\\",\
    									"A playlist<|>o:\\stations.m3u8"),
#endif
    									(void*)&cfg->browser.moduleList},

    	{" ", V_BRK(0), NULL},

    	{"lasttrack.playlist",					V_INT32(-1),				&cfg->lasttrack.playlist},
    	{"lasttrack.track",						V_INT32(-1),				&cfg->lasttrack.track},
    	{"lasttrack.hash",						V_HEX(0),					&cfg->lasttrack.hash},

    	{" ", V_BRK(0), NULL},
    	
    	{"systray.enabled",						V_INT32(1),					&cfg->systray.enabled},
    	{"systray.persistantMenus",				V_INT32(1),					&cfg->systray.persistantMenu},
    	{"systray.tips.enabled",				V_INT32(1),					&cfg->systray.tipsEnabled},
    	{"systray.info.enabled",				V_INT32(1),					&cfg->systray.infoEnabled},
    	{"systray.playlist.showUID",			V_INT32(0),					&cfg->systray.showPlaylistUID},
    	{"systray.playlist.maxStringLength",	V_INT32(45),				&cfg->systray.maxTextLength},
    	{"systray.playlist.track.showNo",		V_INT32(0),					&cfg->systray.showPlaylistTrackNo},
    	{"systray.playlist.track.showLength",	V_INT32(0),					&cfg->systray.showPlaylistTrackLength},
    	{"systray.dvb.showPID",					V_INT32(0),					&cfg->systray.showDvbPid},
    	
    	{" ", V_BRK(0), NULL},
    	
    	{"taskbar.enabled",						V_INT32(1),					&cfg->taskbar.enabled},
    	{"taskbar.toolbarName",					V_STR(WINTOOLBAR_NAME),		&cfg->taskbar.tbName},
    	{"taskbar.string.trackNo",				V_INT32(0),					&cfg->taskbar.string.track},
    	{"taskbar.string.title",				V_INT32(1),					&cfg->taskbar.string.title},
    	{"taskbar.string.artist",				V_INT32(1),					&cfg->taskbar.string.artist},
    	{"taskbar.string.album",				V_INT32(0),					&cfg->taskbar.string.album},
    	{"taskbar.string.description",			V_INT32(0),					&cfg->taskbar.string.description},
    	{"taskbar.string.path",					V_INT32(0),					&cfg->taskbar.string.path},
    	{"taskbar.colour.fore",					V_HEX(COL_TASKBARFR),		&cfg->taskbar.colour.fore},
    	{"taskbar.colour.back",					V_HEX(COL_TASKBARBK),		&cfg->taskbar.colour.back},
    	{"taskbar.colour.mode",					V_INT32(TB_TRANSPARENT),	&cfg->taskbar.colour.bkMode},
    	{"taskbar.font.name",					V_STR("Tahoma"),			&cfg->taskbar.font.name},
    	{"taskbar.font.width",					V_INT32(9),					&cfg->taskbar.font.width},
    	{"taskbar.font.height",					V_INT32(30),				&cfg->taskbar.font.height},
    	{"taskbar.font.point",					V_INT32(18),				&cfg->taskbar.font.point},
    	{"taskbar.font.weight",					V_INT32(FW_MEDIUM),			&cfg->taskbar.font.weight},
    	{"taskbar.font.quality",	V_INT32(CLEARTYPE_NATURAL_QUALITY),		&cfg->taskbar.font.quality},
    	{"taskbar.pos.x",			V_INT32(WINTOOLBAR_ICONS*WINTOOLBAR_ICONWIDTH),	&cfg->taskbar.pos.x},
    	{"taskbar.pos.y",						V_INT32(-5),				&cfg->taskbar.pos.y},
    	    	    	
    	{" ", V_BRK(0), NULL},

    	{"artwork.searchDepth",					V_INT32(4),					&cfg->artwork.searchDepth},
    	{"artwork.maxWidth",					V_INT32(720),				&cfg->artwork.maxWidth},
    	{"artwork.maxHeight",					V_INT32(442),				&cfg->artwork.maxHeight},
    	//{"artwork.threads",					V_INT32(ARTWORKTHREADS),	&cfg->artwork.threads},
    	{"artwork.panelImgSize",				V_INT32(112),				&cfg->artwork.panelImgSize},

    	{" ", V_BRK(0), NULL},

    	{"equalizer.preamp",					V_DBL(12.0),				&cfg->eq.preamp},
    	{"equalizer.preset",					V_INT32(0),					&cfg->eq.preset},
    	{"equalizer.band.",		V_SLIST10("0.0", "0.0", "0.0", "0.0", "0.0",\
    									  "0.0", "0.0", "0.0", "0.0", "0.0"), (void*)&cfg->eq.freq},

		{" ", V_BRK(0), NULL},

    	{"video.aspect.preset",					V_STR("auto"),				&cfg->video.aspect.preset},
    	{"video.aspect.custom.ratio",			V_DBL(1.0),					&cfg->video.aspect.ratio},
    	{"video.aspect.custom.x",	  			V_INT32(0),					&cfg->video.aspect.x},
    	{"video.aspect.custom.y",	  			V_INT32(0),					&cfg->video.aspect.y},
    	{"video.aspect.custom.width",			V_INT32(DEVICE_DEFAULT_WIDTH),	&cfg->video.aspect.width},
    	{"video.aspect.custom.height",			V_INT32(DEVICE_DEFAULT_HEIGHT),	&cfg->video.aspect.height},
    	{"video.aspect.custom.cleanSurface",	V_INT32(1),					&cfg->video.aspect.clean},
    	{"video.filter.rotate",					V_DBL(0.0),					&cfg->video.filter.scale},
    	{"video.filter.scale",					V_DBL(1.0),					&cfg->video.filter.rotate},
    	{"video.filter.blur",					V_INT32(0),					&cfg->video.filter.blur},
    	{"video.filter.pixelize",				V_INT32(0),					&cfg->video.filter.pixelize},
    	{"video.filter.brightness",				V_DBL(1.0),					&cfg->video.filter.brightness},
    	{"video.filter.contrast",				V_DBL(1.0),					&cfg->video.filter.contrast},
    	{"video.filter.saturation",				V_DBL(1.0),					&cfg->video.filter.saturation},
    	{"video.filter.gamma",					V_DBL(1.0),					&cfg->video.filter.gamma},
    	{"video.filter.rotateOp",				V_INT32(1),					&cfg->video.filter.rotateOp},
    	{"video.filter.scaleOp",				V_INT32(1),					&cfg->video.filter.scaleOp},
		{"video.subtitle.delay",				V_INT32(0),					&cfg->video.subtitle.delay},
    	{"video.swapRB",						V_INT32(0),					&cfg->video.swapRB},
                		                	
		{" ", V_BRK(0), NULL},          		                	
                                        		                	
		{"audio.desync",						V_INT32(15),				&cfg->audio.desync},
		{"audio.visuals",						V_INT32(0),					&cfg->audio.visuals},
                                        		                	
		{" ", V_BRK(0), NULL},          		                	
                                        		                	
		{"meta.drawTrackbar",					V_INT32(0),					&cfg->meta.drawTrackbar},

		{" ", V_BRK(0), NULL},
		
		{"search.metaDepth",			V_INT32(SEARCH_DEFAULT_DEPTH),		&cfg->search.metaDepth},
		{"search.ignorecase",			V_INT32(SEARCH_CASE_DEFAULT),		&cfg->search.ignoreCase},

		{" ", V_BRK(0), NULL},

		{"album.shelf.sigma",			V_DBL(0.505),			&cfg->album.sigma},
		{"album.shelf.rho",				V_DBL(1.0),				&cfg->album.rho},
		{"album.shelf.expMult",			V_DBL(4.0),				&cfg->album.expMult},
		{"album.shelf.rate",			V_DBL(0.017*1.50),		&cfg->album.rate},
		{"album.shelf.spacing",			V_DBL(0.340),			&cfg->album.spacing},
		{"album.shelf.art.scaleMult",	V_DBL(1.10),			&cfg->album.artScaleMult},
		{"album.shelf.art.opacityMult",	V_DBL(0.90),			&cfg->album.artOpacityMult},
		{"album.slider.x",				V_DBL(0.648),			&cfg->album.sliderX},
		{"album.slider.y",				V_DBL(0.868),			&cfg->album.sliderY},
		{"album.slider.width",			V_DBL(0.33),			&cfg->album.sliderW},
		{"album.titleY",				V_DBL(0.750),			&cfg->album.titleY},
		{"album.subTextSpace",			V_DBL(0.082),			&cfg->album.textVSpace},

		{" ", V_BRK(0), NULL},

		{"plm.shelf.sigma",				V_DBL(0.550),			&cfg->plm.sigma},
		{"plm.shelf.rho",				V_DBL(1.0),				&cfg->plm.rho},
		{"plm.shelf.expMult",			V_DBL(4.0),				&cfg->plm.expMult},
		{"plm.shelf.rate",				V_DBL(0.017*1.50),		&cfg->plm.rate},
		{"plm.shelf.spacing",			V_DBL(0.340),			&cfg->plm.spacing},
		{"plm.shelf.art.scaleMult",		V_DBL(1.10),			&cfg->plm.artScaleMult},
		{"plm.shelf.art.opacityMult",	V_DBL(0.90),			&cfg->plm.artOpacityMult},
		{"plm.slider.x",				V_DBL(0.663),			&cfg->plm.sliderX},
		{"plm.slider.y",				V_DBL(0.868),			&cfg->plm.sliderY},
		{"plm.slider.width",			V_DBL(0.30),			&cfg->plm.sliderW},
		{"plm.titleY",					V_DBL(0.760),			&cfg->plm.titleY},
		                                                                	
		{" ", V_BRK(0), NULL},                                  	
                                                                	
		{"clock.type",						V_STR("Digital"),				 &cfg->clock.type},
		{"clock.face.dial.x",				V_INT32(189),					 &cfg->clock.bfFaceCx},
		{"clock.face.dial.y",				V_INT32(210),					 &cfg->clock.bfFaceCy},
		{"clock.digital.digits",			V_STR(CLK_DIGITS),				 &cfg->clock.digitalDigits},
		{"clock.digital.overlap",			V_INT32(20),					 &cfg->clock.digitalCharOverlap},

		{"clock.date.analogue.font",		V_INT32(CLK_SBDK_DATE_FONT), &cfg->clock.date.analogue.font},
		{"clock.date.analogue.foreColour",	V_HEX(COL_BLACK),			 &cfg->clock.date.analogue.foreColour},
		{"clock.date.analogue.foreAlpha",	V_INT32(173),				 &cfg->clock.date.analogue.foreAlpha},
		{"clock.date.analogue.backColour",	V_HEX(COL_BLUE_SEA_TINT),	 &cfg->clock.date.analogue.backColour},
		{"clock.date.analogue.backRadius",	V_INT32(8),					 &cfg->clock.date.analogue.backRadius},
		{"clock.date.analogue.backAlpha",	V_INT32(255),				 &cfg->clock.date.analogue.backAlpha},
		{"clock.date.analogue.clearUnused",	V_INT32(1),					 &cfg->clock.date.analogue.clearUnused},
		
		{"clock.date.digital.font",			V_INT32(CLK_SBDK_DATE_FONT), &cfg->clock.date.digital.font},
		{"clock.date.digital.foreColour",	V_HEX(0xE6CCEA),			 &cfg->clock.date.digital.foreColour},
		{"clock.date.digital.foreAlpha",	V_INT32(250),				 &cfg->clock.date.digital.foreAlpha},
		{"clock.date.digital.backColour",	V_HEX(0xFA51CD),			 &cfg->clock.date.digital.backColour},
		{"clock.date.digital.backRadius",	V_INT32(15),				 &cfg->clock.date.digital.backRadius},
		{"clock.date.digital.backAlpha",	V_INT32(255),				 &cfg->clock.date.digital.backAlpha},
		{"clock.date.digital.clearUnused",	V_INT32(1),					 &cfg->clock.date.digital.clearUnused},

		{"clock.date.boxdigital.font",			V_INT32(CLK_SBDK_DATE_FONT), &cfg->clock.date.boxdigital.font},
		{"clock.date.boxdigital.foreColour",	V_HEX(0x000000),			 &cfg->clock.date.boxdigital.foreColour},
		{"clock.date.boxdigital.foreAlpha",		V_INT32(173),				 &cfg->clock.date.boxdigital.foreAlpha},
		{"clock.date.boxdigital.backColour",	V_HEX(0xCFB000),			 &cfg->clock.date.boxdigital.backColour},
		{"clock.date.boxdigital.backRadius",	V_INT32(12),				 &cfg->clock.date.boxdigital.backRadius},
		{"clock.date.boxdigital.backAlpha",		V_INT32(255),				 &cfg->clock.date.boxdigital.backAlpha},
		{"clock.date.boxdigital.clearUnused",	V_INT32(1),					 &cfg->clock.date.boxdigital.clearUnused},

		{"clock.date.butterfly.font",		V_INT32(CLK_SBDK_DATE_FONT), &cfg->clock.date.butterfly.font},
		{"clock.date.butterfly.foreColour",	V_HEX(0xEEEEEE),			 &cfg->clock.date.butterfly.foreColour},
		{"clock.date.butterfly.foreAlpha",	V_INT32(173),				 &cfg->clock.date.butterfly.foreAlpha},
		{"clock.date.butterfly.backColour",	V_HEX(0xD99738),			 &cfg->clock.date.butterfly.backColour},
		{"clock.date.butterfly.backRadius",	V_INT32(12),				 &cfg->clock.date.butterfly.backRadius},
		{"clock.date.butterfly.backAlpha",	V_INT32(255),				 &cfg->clock.date.butterfly.backAlpha},
		{"clock.date.butterfly.clearUnused",V_INT32(1),					 &cfg->clock.date.butterfly.clearUnused},

		{"clock.date.polar.font",			V_INT32(CLK_SBDK_DATE_FONT), &cfg->clock.date.polar.font},
		{"clock.date.polar.foreColour",		V_HEX(COL_BLUE_SEA_TINT),	 &cfg->clock.date.polar.foreColour},
		{"clock.date.polar.foreAlpha",		V_INT32(173),				 &cfg->clock.date.polar.foreAlpha},
		{"clock.date.polar.backColour",		V_HEX(0xFFFFFF),			 &cfg->clock.date.polar.backColour},
		{"clock.date.polar.backRadius",		V_INT32(12),				 &cfg->clock.date.polar.backRadius},
		{"clock.date.polar.backAlpha",		V_INT32(255),				 &cfg->clock.date.polar.backAlpha},
		{"clock.date.polar.clearUnused",	V_INT32(1),					 &cfg->clock.date.polar.clearUnused},

		{"clock.date.predator.font",		V_INT32(CLK_SBDK_DATE_FONT), &cfg->clock.date.predator.font},
		{"clock.date.predator.foreColour",	V_HEX(0x530204),			 &cfg->clock.date.predator.foreColour},
		{"clock.date.predator.foreAlpha",	V_INT32(173),				 &cfg->clock.date.predator.foreAlpha},
		{"clock.date.predator.backColour",	V_HEX(0xF30204),			 &cfg->clock.date.predator.backColour},
		{"clock.date.predator.backRadius",	V_INT32(12),				 &cfg->clock.date.predator.backRadius},
		{"clock.date.predator.backAlpha",	V_INT32(255),				 &cfg->clock.date.predator.backAlpha},
		{"clock.date.predator.clearUnused",	V_INT32(0),					 &cfg->clock.date.predator.clearUnused},		
		
		{" ", V_BRK(0), NULL},

		{"alarm.enabled",				V_INT32(0),				&cfg->alarm.status},
		{"alarm.time",					V_STR("06:55"),			&cfg->alarm.time},
		{"alarm.period",				V_STR("Weekly"),		&cfg->alarm.period},
		{"alarm.weekly.",	V_SLIST7("Sun","Mon","Tue","Wed","Thr","Fri","Sat"), (void*)&cfg->alarm.days},
		{"alarm.action.mode",			V_STR("Playtrack"),		&cfg->alarm.action.mode},
		
		{"alarm.action.playtrack.title", V_STR(" "),				&cfg->alarm.action.track.title},
		{"alarm.action.playtrack.uid",	 V_HEX(PLAYLIST_UID_BASE+1),&cfg->alarm.action.track.uid},
		{"alarm.action.playtrack.track", V_INT32(1),				&cfg->alarm.action.track.trkNo},
		{"alarm.action.playtrack.volume",V_INT32(90),				&cfg->alarm.action.track.volume},
		
		{"alarm.action.starttask.path",	V_STR(" "),				&cfg->alarm.action.starttask.path8},
		
		{"alarm.action.endtask.pid",	V_INT32(0),				&cfg->alarm.action.endtask.pid},
		{"alarm.action.endtask.module",	V_STR(" "),				&cfg->alarm.action.endtask.module},
		
		{"alarm.action.flash.colour1",	V_HEX(0xFF508DC5),		&cfg->alarm.action.flash.colour1},
		{"alarm.action.flash.colour2",	V_HEX(0xFF00FF1E),		&cfg->alarm.action.flash.colour2},
		{"alarm.action.flash.period",	V_INT32(200),			&cfg->alarm.action.flash.period},
		{"alarm.action.flash.repeat",	V_INT32(25),			&cfg->alarm.action.flash.repeat},

		
#if ENABLE_ANTPLUS
		{" ", V_BRK(0), NULL},
		
		{"hrm.device.vid",				V_INT32(ANTSTICK_VID),	&cfg->hrm.deviceVID},
		{"hrm.device.pid",				V_INT32(ANTSTICK_PID),	&cfg->hrm.devicePID},
		{"hrm.device.index",			V_INT32(0),				&cfg->hrm.deviceIndex},
		{"hrm.device.key",				V_STR(ANT_DEFAULTKEY),	&cfg->hrm.deviceKey},
		{"hrm.sensor.id",				V_INT32(0),				&cfg->hrm.sensorId},
		{"hrm.activateOnInsertion",		V_INT32(0),				&cfg->hrm.activateOnInsertion},
		{"hrm.enableOverlay",			V_INT32(1),				&cfg->hrm.enableOverlay},
		{"hrm.digits",					V_STR(HRM_DIGITS),		&cfg->hrm.digits},
#endif

#if ENABLE_GARMINTCX
		{" ", V_BRK(0), NULL},
		
		{"tcx.file",					V_STR("a .tcx file"),			&cfg->tcx.file},
		{"tcx.scale.initial",		V_DBL(TCX_RENDER_SCALE_DEFAULT),	&cfg->tcx.scaleInitial},
		{"tcx.scale.multiplier",	V_DBL(TCX_RENDER_SCALE_MULTIPLIER),	&cfg->tcx.scaleMultiplier},
		{"tcx.background.colour",		V_HEX(0xFF3C6A94),		&cfg->tcx.backgroundColour},
		{"tcx.route.colour.",   V_SLIST9("0xFFD4CAC8","0xFFFF7F11","0xFF00FF00","0xFFFFFF00",\
								"0xFFFF00FF","0xFF00B7EB","0xFFFF0000","0xFF0000FF", "0xFF000000"),
																(void*)&cfg->tcx.colourList},
#endif


    	{" ", V_BRK(0), NULL},

		{"hotkeys.local.enabled",		V_INT32(1),				&cfg->hotkeys.localEnabled},
    	{"hotkeys.local.cursor",		V_CHR('A'),				&cfg->hotkeys.cursor},
    	{"hotkeys.local.console",		V_CHR('L'),				&cfg->hotkeys.console},
		//     modifierA,modifierB,key,image_path
		// eg; CTRL,ALT,A,a name,hotkeys/image1.png
		{"hotkeys.global.",	  V_SLIST11("ALT,SHIFT,P,Play/Pause,hotkeys\\playpause.png",\
										"ALT,SHIFT,O,Stop,hotkeys\\stop.png",\
										"ALT,SHIFT,B,Previous,hotkeys\\prev.png",\
										"ALT,SHIFT,N,Next,hotkeys\\next.png",\
										"ALT,SHIFT,I,Volume Up,hotkeys\\volumeup.png",\
										"ALT,SHIFT,H,Seek Back,hotkeys\\seek-back.png",\
										"ALT,SHIFT,J,Seek Forward,hotkeys\\seek-forward.png",\
										"ALT,SHIFT,F,Fullscreen,hotkeys\\fullscreen.png",\
										"ALT,SHIFT,M,Mute,hotkeys\\mute.png",\
										"ALT,SHIFT,K,Volume Down,hotkeys\\volumedown.png",\
										"ALT,SHIFT,Q,Close VLC,hotkeys\\quit.png"),\
										(void*)&cfg->hotkeys.key},

		{"hotkeys.global.enabled",		V_INT32(1),				&cfg->hotkeys.globalEnabled},
		{"hotkeys.global.showLabels",	V_INT32(0),				&cfg->hotkeys.showNames},


		
		{" ", V_BRK(0), NULL},
		
		{"taskman.layout.rows",			V_INT32(4),			&cfg->taskman.layout.rows},
		{"taskman.layout.offset",		V_INT32(18),		&cfg->taskman.layout.offset},
		{"taskman.cpugraph.enable",		V_INT32(0),			&cfg->taskman.cpugraph.enable},
		{"taskman.cpugraph.mode",		V_INT32(2),			&cfg->taskman.cpugraph.mode},
		{"taskman.cpugraph.colour.",   V_SLIST8("0x00E000","0xFF0000","0x00FFFF","0xFFFF00",\
												"0xFF00FF","0xF0F0F0","0xF46432","0x0000F4"),
												(void*)&cfg->taskman.cpugraph.colourList},

    	{NULL, V_INT32(0), NULL, 0, NULL}
    };

	TCFGENTRY **config = cfg_configDup(config_cfg);
	cfg_commentsSetDefault(config);

	return config;
}



