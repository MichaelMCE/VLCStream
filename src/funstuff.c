
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




#if (ENABLE_CMDFUNSTUFF)




static inline void sheetFree (TREPLYSHEET *sheet)
{
	my_free(sheet->name);
	freeASCIILINE(sheet->file);
	my_free(sheet);
}

void sheetsFree (TCMDREPLY *sheets)
{
	for (int i = 0; i < sheets->total; i++){
		if (sheets->sheet[i])
			sheetFree(sheets->sheet[i]);
	}
	my_free(sheets->sheet);
	my_free(sheets);
}

TCMDREPLY *sheetsNew (const int total)
{
	TCMDREPLY *sheets = my_calloc(1, sizeof(TCMDREPLY));
	if (sheets){
		sheets->sheet = my_calloc(total, sizeof(TREPLYSHEET*));
		if (sheets->sheet){
			sheets->total = total;
			return sheets;
		}
		my_free(sheets);
	}
	return NULL;
}

static inline int sheetInsert (TCMDREPLY *sheets, const char *name, TASCIILINE *data)
{
	for (int i = 0; i < sheets->total; i++){
		if (!sheets->sheet[i]){
			TREPLYSHEET *sheet = my_calloc(1, sizeof(TREPLYSHEET));
			if (sheet){
				sheets->sheet[i] = sheet;
				sheet->file = data;
				sheet->total = data->tlines;
				sheet->last = -1;
				sheet->name = my_strdup(name);
				return i;
			}
		}
	}
	return -1;
}

int sheetAdd (TCMDREPLY *sheets, const char *name, const wchar_t *path)
{
	TASCIILINE *data = readFileW(path);
	if (data)
		return (sheetInsert(sheets, name, data) >= 0);
	return 0;
}

static inline TREPLYSHEET *sheetsFind (TCMDREPLY *sheets, const char *name)
{
	for (int i = 0; i < sheets->total; i++){
		if (sheets->sheet[i]){
			if (!strcmp(name, sheets->sheet[i]->name))
				return sheets->sheet[i];
		}
	}
	return NULL;
}

char *sheetGetLine (TCMDREPLY *sheets, const char *name, const int line)
{
	TREPLYSHEET *sheet = sheetsFind(sheets, name);
	if (sheet){
		if (line >= 0 && line < sheet->total){
			sheet->last = line;
			return my_strdup((char*)sheet->file->line[line]);
		}
	}
	return NULL;
}

char *sheetGetLineRand (TCMDREPLY *sheets, const char *name)
{
	TREPLYSHEET *sheet = sheetsFind(sheets, name);
	if (sheet){
		int line = (rand()+(rand()*rand())) % clock() % sheet->total;
		if (line >= 0 && line < sheet->total){
			sheet->last = line;
			return my_strdup((char*)sheet->file->line[line]);
		}
	}
	return NULL;
}


#endif


