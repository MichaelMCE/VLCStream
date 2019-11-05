
// libmylcd - http://mylcd.sourceforge.net/
// An LCD framebuffer and text rendering API
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


#define BLEN (MAX_PATH_UTF8+1)



static unsigned int extHash;
static int extLen;



static inline int isLinePlaylist (const char *path)
{
	const int len = strlen(path);
	if (len > extLen)
		return (getHash(&path[(len-extLen)]) == extHash);
	return 0;
}

static inline int isLineDirectory (const char *var)
{
	wchar_t conbuffer[MAX_PATH+1] = {0};
	
	if (UTF8ToUTF16(var, strlen(var), conbuffer, MAX_PATH)){
		//wprintf(L"%i #%s#\n", PathIsDirectoryW(conbuffer) != 0, conbuffer);
		return PathIsDirectoryW(conbuffer) != 0;
	}else{
		return 0;
	}
}

static inline char *removeTrailingSpace (char *var)
{
#if 1
	char *eos = var + strlen(var) - 1;
	while (eos >= var && (/**eos == ' ' || *eos == '\t' ||*/ /**eos == '\r' ||*/ *eos == '\n'))
		*eos-- = 0;
#else
	char *chr = strrchr(line, var);
	if (chr) newLine = 0;
#endif
	return var;
}

static inline char *removeLeadingSpace (char *var)
{
#if 0
	int i = strspn(var, " \t");
	if (i) var += i;
	return var;
#else
	return var+strspn(var, " \t");
#endif
}

TM3U *m3uNew ()
{
	if (!extHash){
		extHash = getHash(VLCSPLAYLISTEXT);
		extLen = strlen(VLCSPLAYLISTEXT);
	}

	return my_calloc(1, sizeof(TM3U));
}

void m3uFree (TM3U *m3u)
{
	if (m3u)
		my_free(m3u);
}	

static inline int m3uOpenRead (TM3U *m3u, const wchar_t *name)
{
	//if (m3u->al)
	//	freeASCIILINE(m3u->al);
	//m3u->al = readFileW(name);
	m3u->hFile = _wfopen(name, L"rb");
//	wprintf(L"'%s' %p\n", name, m3u->hFile);

	return (m3u->hFile != NULL);
	//return (m3u->al != NULL);
}

static inline void m3uCloseRead (TM3U *m3u)
{
	if (m3u){
		//if (m3u->al){
		//	freeASCIILINE(m3u->al);
		//	m3u->al = NULL;
		//}
		if (m3u->hFile){
			fclose(m3u->hFile);
			m3u->hFile = NULL;
		}
	}
}

static inline int m3uOpenWrite (TM3U *m3u, const wchar_t *name)
{
	m3u->hFile = _wfopen(name, L"w+b");
	if (m3u->hFile){
		fseek(m3u->hFile, 0, SEEK_SET);
		fprintf(m3u->hFile,"﻿#EXTM3U\n");
	}
	return (m3u->hFile != NULL);
}

static inline void m3uCloseWrite (TM3U *m3u)
{
	if (m3u->hFile){
		fclose(m3u->hFile);
		m3u->hFile = NULL;
	}
}

int m3uOpen (TM3U *m3u, const wchar_t *name, const int action)
{
	switch (action){
	  case M3U_OPENREAD:
	  	m3u->mode = M3U_OPENREAD;
	  	return m3uOpenRead(m3u, name);
	  case M3U_OPENWRITE:
	  	m3u->mode = M3U_OPENWRITE;
	  	return m3uOpenWrite(m3u, name);
	}
	return -1;	
}

void m3uClose (TM3U *m3u)
{
	switch (m3u->mode){
	  case M3U_OPENREAD:
	  	m3uCloseRead(m3u);
	  	break;
	  case M3U_OPENWRITE:
	  	m3uCloseWrite(m3u);
	  	break;
	}
}

int m3uWritePlaylists (TM3U *m3u, TPLAYLISTMANAGER *plm, TMETATAGCACHE *tagc, TARTMANAGER *am)
{
	//char buffer[MAX_PATH_UTF8];
	//char *name;
	int itemsWritten = 0;

	if (playlistManagerLock(plm)){
		const int total = playlistManagerGetTotal(plm);
		int  i = 0;
		
		while(i < total){
			PLAYLISTCACHE *plc = playlistManagerGetPlaylist(plm, i++);
			if (plc && !plc->parent){
				//name = playlistGetName(plc, buffer, sizeof(buffer));
				//if (name){
					//fprintf(m3u->hFile,"#EXTPYL:%s\n", name);
					itemsWritten += m3uWritePlaylist(m3u, plc, tagc, am, 0);
				//}
			}
		}
		playlistManagerUnlock(plm);
	}

	return itemsWritten;
}

static inline void writeWhiteSpace (FILE *file, int width)
{
	while (width--) fprintf(file," ");
}

int m3uWritePlaylist (TM3U *m3u, PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, TARTMANAGER *am, int state)
{
	char path[BLEN];
	char opts[BLEN];
	char title[BLEN];

	if (!plc){
		printf("error writing '%p'\n", plc);
		return 0;
	}

	if (state){
		char *name = my_malloc(MAX_PATH_UTF8+1);
		if (name){
			playlistGetName(plc, name, MAX_PATH_UTF8);
			if (*name){
				//strchrreplace(name, '_', ' ');
				writeWhiteSpace(m3u->hFile, state-1);
				fprintf(m3u->hFile,"#EXTPYL:%s\n", name);
			}
			my_free(name);
		}
	}
	
	int itemsWritten = 0;
	int trk = 0;
	TPLAYLISTITEM *item = playlistGetItem(plc, trk);
	
	int artId = 0;
	char *path8 = NULL;
	
	while (item){
		if (item->objType == PLAYLIST_OBJTYPE_PLC){
			if (item->obj.plc){
				if (playlistGetTotal(item->obj.plc))
					itemsWritten += m3uWritePlaylist(m3u, item->obj.plc, tagc, am, state+1);
			}
		}else if (item->objType == PLAYLIST_OBJTYPE_TRACK){
			playlistGetPath(plc, trk, path, BLEN-1);
			if (*path){
				unsigned int hash = getHash(path);
				tagRetrieveByHash(tagc, hash, MTAG_Title, title, sizeof(title));
				if (!*title) playlistGetTitle(plc, trk, title, BLEN-1);
#if 0
				strchrreplace(title, '_', ' ');
				char *ay = stristr(title, ".ay");
				if (ay) *ay = 0;
#endif
				if (artId == item->obj.track.artId && artId){
					if (path8 && *path8){
						writeWhiteSpace(m3u->hFile, state);
						fprintf(m3u->hFile,"#EXTART:%s\n", path8);
					}
				}else if (artId != item->obj.track.artId || !artId){
					if (item->obj.track.artId){
						artId = item->obj.track.artId;
						wchar_t *path = artManagerImageGetPath(am, artId);
						if (path){
							if (path8) my_free(path8);
							path8 = convertto8(path);
							if (path8 && *path8){
								writeWhiteSpace(m3u->hFile, state);
								fprintf(m3u->hFile,"#EXTART:%s\n", path8);
							}
							my_free(path);
						}
					}
				}
				
				if (tagLock(tagc)){
					TMETAITEM *tagItem = g_tagFindEntryByHash(tagc, hash);
					if (tagItem){
#if 0
						if (tagItem->tag[MTAG_ArtworkURL]){
							writeWhiteSpace(m3u->hFile, state);
							fprintf(m3u->hFile,"#EXTART:%s\n", tagItem->tag[MTAG_ArtworkURL]);
						}
#endif
						if (tagItem->tag[MTAG_Album]){
							writeWhiteSpace(m3u->hFile, state);
							fprintf(m3u->hFile,"#EXTALB:%s\n", tagItem->tag[MTAG_Album]);
						}
				
						int len = 0;
						if (tagItem->tag[MTAG_LENGTH])
							len = (int)stringToTime(tagItem->tag[MTAG_LENGTH], strlen(tagItem->tag[MTAG_LENGTH]));

						if (tagItem->tag[MTAG_Artist]){
							writeWhiteSpace(m3u->hFile, state);
							fprintf(m3u->hFile,"#EXTINF:%i^%s^%s\n", len, tagItem->tag[MTAG_Artist], title);
						}else{
							writeWhiteSpace(m3u->hFile, state);
							fprintf(m3u->hFile,"#EXTINF:%i^^%s\n", len, title);
						}
					}else{
						writeWhiteSpace(m3u->hFile, state);
						fprintf(m3u->hFile,"#EXTINF:0^^%s\n", title);
					}
					tagUnlock(tagc);
				}
						
				if (playlistGetOptions(plc, trk, opts, MAX_PATH_UTF8)){
					writeWhiteSpace(m3u->hFile, state);
					fprintf(m3u->hFile,"%s %s\n", path, opts);
				}else{
					writeWhiteSpace(m3u->hFile, state);
					fprintf(m3u->hFile,"%s\n", path);
				}
			
				itemsWritten++;
			}
		}
		item = playlistGetItem(plc, ++trk);
	}

	if (path8) my_free(path8);
	
	if (state){
#if 0
		char *name = my_malloc(MAX_PATH_UTF8+1);
		if (name){
			playlistGetName(plc, name, MAX_PATH_UTF8);
			if (*name){
				writeWhiteSpace(m3u->hFile, state-1);
				fprintf(m3u->hFile,"#EXTPYLEND:%s\n", name);
			}
			my_free(name);
		}
#else
		if (playlistLock(plc)){
			writeWhiteSpace(m3u->hFile, state-1);
			if (*plc->title)
				fprintf(m3u->hFile,"#EXTPYLEND:%s\n", plc->title);
			else
				fprintf(m3u->hFile,"#EXTPYLEND: \n");
			playlistUnlock(plc);
		}
#endif
	}

	return itemsWritten;
}

static inline int m3uParseEXTINF (char *in, char *_time, char *_artist, char *_title, const int bsize)
{
	*_time = 0;
	*_artist = 0;
	*_title = 0;
	
	//char *time = strtok(in, ":");
	//time = strtok(NULL, ",");

	char *time = strtok(in, "^");
	char *artist = strtok(NULL, "^\n");
	char *title = strtok(NULL, "\n");
	//printf("'%s' '%s' '%s'\n", time, artist, title);
	
	if (!title || (title && !*title)) title = artist;
	
	if (title){
		if (*(title-2) == ' ') *(title-2) = '\0';	// remove space before -
		if (*title == ' ') title++;					// remove space after -
	}else if (artist){
		title = artist;
		artist = NULL;
	}
	if (time) strncpy(_time, time, bsize);
	if (artist) strncpy(_artist, artist, bsize);
	if (title) strncpy(_title, title, bsize);
	return (time || artist || title);
}

static char *readNextLine (char *text, FILE *fp)
{
	*text = 0;
	return fgets(text, MAX_PATH_UTF8*2, fp);
}

int m3uReadPlaylist (TM3U *m3u, TPLAYLISTMANAGER *plm, PLAYLISTCACHE *plc, TMETATAGCACHE *tagc, TARTMANAGER *am, TFILEPANE *filepane)
{
	char time[BLEN];
	char artist[BLEN];
	char title[BLEN];
	char album[BLEN];
	char artwork[BLEN];
	char readBuffer[MAX_PATH_UTF8*2];
	char *buffer = readBuffer;
	
	int tagIdx = -999;
	char *line;
	int ct = 0;

	*artwork = 0;
	*album = 0;
	*time = 0;
	*artist = 0;
	*title = 0;

	PLAYLISTCACHE *root = plc;
	int state = 0;

	buffer = readNextLine(readBuffer, m3u->hFile);
	if (!buffer) return 0;
	if (!strncmp(buffer, "﻿", 3)){	// skip utf8 header
		buffer = readNextLine(readBuffer, m3u->hFile);
		if (!buffer) return 0;
	}
	line = removeLeadingSpace(buffer);


	int i = 0;
	//for (int i = 0; i < m3u->al->tlines; i++){
	do{
		i++;
		//line = removeLeadingSpace((char*)m3u->al->line[i]);
		if (*line == '#'){
			line++;
			
			if (!strncmp(line, "EXTINF:", 7)){
				if (m3uParseEXTINF(line+7, time, artist, title, BLEN-1))
					tagIdx = i;
				//printf("\n'%s'\n'%s'\n'%s'\n\n",time, artist, title);
				
			}else if (!strncmp(line, "EXTART:", 7)){
				char *art = strtok(line+7, "\n");
				if (art)
					strncpy(artwork, art, BLEN-1);

			}else if (!strncmp(line, "EXTALB:", 7)){
				char *alb = strtok(line+7, "\n");
				if (alb)
					strncpy(album, alb, BLEN-1);
				
			}else if (!strncmp(line, "EXTPYL:", 7)){
				char *playlist = strtok(line+7, "\n");
				if (playlist && *playlist){
					PLAYLISTCACHE *plcN = playlistManagerCreatePlaylist(plm, playlist, 0);
					if (plcN){
						//if (state)
							playlistAddPlc(plc, plcN);
						plc = plcN;
						state++;
					}
				}
			}else if (!strncmp(line, "EXTPYLEND:", 10)){
				if (--state < 0){
					state = 0;
					//printf("m3uReadPlaylist: playlist invalid or incorrectly formatted: '%s'\n", line+10);
				}
				if (plc->parent)
					plc = plc->parent;
				else
					plc = root;
			}
			goto next;
		}

		if (isLineDirectory(line) && filepane){
			char *atitle;
			if (*album) atitle = album;
			else atitle = line;
			
			PLAYLISTCACHE *plcN = playlistManagerCreatePlaylist(plm, atitle, 0);
			if (plcN){
				wchar_t *inpath = converttow(line);
				if (inpath){
					/*int importTotal = */filepaneBuildPlaylistDir(filepane, plcN, inpath, FILEMASKS_MEDIA, 1);
					//printf("m3u import #%s# %i\n", line, importTotal);
					my_free(inpath);
				}
					
				int total = playlistGetTotal(plcN);
				if (!total){
					playlistManagerDeletePlaylist(plm, plcN, 1);
				}else{
					ct += total;
					playlistAddPlc(plc, plcN);
				}
			}
		}else if (isLinePlaylist(line)){
			char *atitle;
			if (*album) atitle = album;
			else atitle = line;
			
			PLAYLISTCACHE *plcN = playlistManagerCreatePlaylist(plm, atitle, 0);
			if (plcN){
				ct += importPlaylist(plm, plcN, tagc, am, line, filepane);
				if (!playlistGetTotal(plcN))
					playlistManagerDeletePlaylist(plm, plcN, 1);
				else
					playlistAddPlc(plc, plcN);
			}
		}else if (plc){
			removeTrailingSpace(line);
			const int pos = playlistAdd(plc, line);

			if (pos >= 0 && tagIdx == i-1){
				if (*title || *artist || *time || *album){
					if (tagLock(tagc)){
						TMETAITEM *item = g_tagCreateNew(tagc, /*line,*/ getHash(line));
						if (!item->tag[MTAG_PATH])
							item->tag[MTAG_PATH] = my_strdup(line);
	
						if (*title){
							if (item->tag[MTAG_Title])
								my_free(item->tag[MTAG_Title]);
							item->tag[MTAG_Title] = my_strdup(title);
							//item->tag[MTAG_Title] = decodeURI_noprefix(title, strlen(title));
							
							playlistSetTitle(plc, pos, title, 1);
							item->hasTitle = 1;
							*title = 0;
						}

						if (*artist){
							if (item->tag[MTAG_Artist])
								my_free(item->tag[MTAG_Artist]);
							item->tag[MTAG_Artist] = my_strdup(artist);
							//printf("##%s##\n", item->tag[MTAG_Artist]);
							//item->tag[MTAG_Artist] = decodeURI_noprefix(artist, strlen(artist));
							*artist = 0;
						}

						if (*time){
							int tsec;
							if ((tsec=atol(time)) > 0){
								timeToString((libvlc_time_t)tsec, time, BLEN-1);
								if (item->tag[MTAG_LENGTH])
									my_free(item->tag[MTAG_LENGTH]);
								item->tag[MTAG_LENGTH] = my_strdup(time);
							}
							*time = 0;
						}

						if (*album){
							if (item->tag[MTAG_Album])
								my_free(item->tag[MTAG_Album]);
							item->tag[MTAG_Album] = my_strdup(album);
						}
						tagUnlock(tagc);
					}
				}
										
				if (*artwork){
					char *out;
					if (!strncmp(artwork, "file:///", 8))
						out = decodeURI(artwork, strlen(artwork));
					else
						out = decodeURI_noprefix(artwork, strlen(artwork));
					
					wchar_t *artw = converttow(out);
					if (artw){
						if (*artw){
							int artId = artManagerImageAddEx(am, artw, tagc->maxArtWidth, tagc->maxArtHeight); // bestfit artwork in to this rect
							if (artId){
#if 0
								int w, h;
								artManagerImageGetMetrics(am, artId, &w, &h);
								imageBestFit(tagc->maxArtWidth, tagc->maxArtHeight, w, h, &w, &h);
								artManagerImageResize(am, artId, w, h);
#endif
								playlistSetArtId(plc, pos, artId, 1);
							}
						}
						my_free(artw);
					}

					my_free(out);
					*artwork = 0;
				}
			}
			ct++;
		}
		*album = 0;
		
next:
		buffer = readNextLine(readBuffer, m3u->hFile);
		if (buffer)
			line = removeLeadingSpace(buffer);
	}while(buffer != NULL);
	
	//printf("state %i, ct %i\n", state, ct);
	
	return ct;
}
