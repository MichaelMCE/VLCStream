
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






// keep this in sync with tags.h:enum _tags_meta and cmdparser.c:wtagStrLookup[], and tags.c
static const char *tagStrLookup[] = {
	"Title",
    "Artist",
    "Genre",
    "Copyright",
    "Album",
    "Track #",
    "Description",
    "Rating",
    "Date",
    "Setting",
    "URL",
    "Language",
    "Now Playing",
    "Publisher",
    "Encoded By",
    "Path",
    "TrackID",
    
#if (LIBVLC_VERSION_MAJOR >= 2 && LIBVLC_VERSION_MINOR >= 2)
    "TrackTotal",
    "Director",
    "Season",
    "Episode",
    "Showname",
    "Actors",
#endif

    "Length",
    "Filename",
    "Position",
    "Path",
    ""
};



static inline int getNextTrack (PLAYLISTCACHE *plc, const int pos)
{
	int next = playlistGetNextItem(plc, PLAYLIST_OBJTYPE_TRACK, pos);
	if (next == -1) next = playlistGetNextItem(plc, PLAYLIST_OBJTYPE_TRACK, -1);
	return next;
}

static inline int getPrevTrack (PLAYLISTCACHE *plc, const int pos)
{
	int prev = playlistGetPrevItem(plc, PLAYLIST_OBJTYPE_TRACK, pos);
	if (prev == -1) prev = playlistGetPrevItem(plc, PLAYLIST_OBJTYPE_TRACK, playlistGetTotal(plc));
	return prev;
}

static inline int metaButtonPress (TMETA *meta, TCCBUTTON *btn, const int id, const TTOUCHCOORD *pos)
{
	TVLCPLAYER *vp = meta->com->vp;
	meta->btns->t0 = getTickCount();
	TMETADESC *desc = &meta->desc;

	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, desc->uid);
	/*if (pageGetPrevious(vp) == PAGE_OVERLAY){
	  	plc = getQueuedPlaylist(vp);
	  	if (!plc)
	  		plc = getDisplayPlaylist (vp);
	}else{
		plc = getDisplayPlaylist (vp);
	}*/
	if (!plc) return 0;


	switch (id){
	  case METABUTTON_CLOSE:
  		//if (page2RenderGetState(vp->pages, PAGE_META))
  		//	pageSetSec(vp, -1);
  		//else
  			page2Set(vp->pages, PAGE_META, 0);
  			page2SetPrevious(meta);
		return -1;

	  case METABUTTON_UP:
		desc->trackPosition = getPrevTrack(plc, desc->trackPosition);

		plc->pr->selectedItem = desc->trackPosition;
		playlistMetaGetMeta(vp, plc, plc->pr->selectedItem-CACHEREADAHEAD, plc->pr->selectedItem, NULL);
	  	break;	  
	  
	  case METABUTTON_LEFT:
		desc->trackPosition = getPrevTrack(plc, desc->trackPosition);
		
		plc->pr->selectedItem = desc->trackPosition;
		plc->pr->playingItem = desc->trackPosition;
		startPlaylistTrack(vp, plc, desc->trackPosition);
		trackLoadEvent(vp, plc, desc->trackPosition);
		playlistMetaGetMeta(vp, plc, plc->pr->selectedItem-CACHEREADAHEAD, plc->pr->selectedItem, NULL);
	  	break;
	  
	  case METABUTTON_DOWN:
		desc->trackPosition = getNextTrack(plc, desc->trackPosition);

		plc->pr->selectedItem = desc->trackPosition;
		playlistMetaGetMeta(vp, plc, plc->pr->selectedItem, plc->pr->selectedItem+CACHEREADAHEAD, NULL);
	  	break;
	  
	  case METABUTTON_RIGHT:
		desc->trackPosition = getNextTrack(plc, desc->trackPosition);
		
		plc->pr->selectedItem = desc->trackPosition;
		plc->pr->playingItem = desc->trackPosition;
		startPlaylistTrack(vp, plc, desc->trackPosition);
		trackLoadEvent(vp, plc, desc->trackPosition);
		playlistMetaGetMeta(vp, plc, plc->pr->selectedItem, plc->pr->selectedItem+CACHEREADAHEAD, NULL);
	  
	  	break;
	 };
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
		return metaButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	return 1;
}

static inline int drawArtwork (TVLCPLAYER *vp, TFRAME *frame, TMETAARTWORK *mart, const int artId, const int x, const int y, const int w, const int h, const int drawui)
{
	int ret = 0;
	
	if (!artId)
		return ret;
	else if (artId)
		mart->artId = artId;
		
	TFRAME *img = artManagerImageAcquire(vp->am, artId);
	if (img){
		const int gap = abs(h - img->height)/2.0;
		const int fh = img->height;
		const int fw = img->width;

		if (drawui > 0){
			//doBlur(frame, (x+w)-fw-1, y, x+w, y+gap, 3);		// blur area above art
			//doBlur(frame, (x+w)-fw-1, y+gap+fh+1, x+w, y+h, 3);	// blur below art
			//doBlur(frame, x+w, y+gap+1, x+w, y+gap+fh, 3);		// blur a one pixel column to right of art
		}else if (drawui != -1){
			drawShadowUnderlay(vp, frame, (x+w)-fw, y+gap+1, fw, fh, SHADOW_BLACK);
		}
		fastFrameCopy(img, frame, (x+w)-fw, y+gap+1);
		artManagerImageRelease(vp->am, artId);
		ret = fw+1;
	}
	return ret;
}

int drawAVTrack (TFRAME *frame, TLPRINTR *rt, const int font, const int blurOp, const unsigned int *col, const char *name, TAVTRACKS *avts, int idx)
{
	if (idx < 0 || !avts->totalTracks) return 0;

	if (idx >= avts->totalTracks) idx--;
	const int id = avts->track[idx].id;
	//if (id){
		const char *text = avts->track[idx].name;
		lSetForegroundColour(frame->hw, col[SWH_META_METANAME]);
		lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, COL_ORANGE);
		lPrintEx(frame, rt, font, PF_FORCEAUTOWIDTH|PF_CLIPWRAP|PF_CLIPDRAW|PF_IGNOREFORMATTING, LPRT_CPY, name);
		lSetForegroundColour(frame->hw, col[SWH_META_METAVALUE]);
		lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, COL_CYAN);
		lPrintEx(frame, rt, font, PF_FORCEAUTOWIDTH|PF_USELASTX|PF_CLIPWRAP|PF_CLIPDRAW, LPRT_CPY, "%i - %s ", id, text);
	//}
	return 1;
}

void metaCopyDesc (TMETADESC *desc, TMETA *meta)
{
	my_memcpy(desc, &meta->desc, sizeof(TMETADESC));
}


static inline void metaUpdateLocalTrackMeta (TVLCPLAYER *vp, TVLCCONFIG *vlc)
{
	char *path = getPlayingPath(vp);
	if (path){
		if (*path){
			if (isMediaLocal(path)){
				for (int i = 0; i <= MTAG_TrackID; i++){
					if (i == MTAG_ArtworkPath){
						continue;
						
					}else if (i == MTAG_Title){
						char *tag = vlc_getMeta(vlc, i);
						if (tag){
							if (*tag){
								if (!isMediaLocal(tag)){
									removeTrailingSpaces(tag);
									//printf("# tag %i '%s'\n", i, tag);
									tagAdd(vp->tagc, path, i, tag, 1);
									PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
									if (plc)
										playlistSetTitle(plc, getPlayingItem(vp), tag, 1);
								}
							}
							my_free(tag);
						}
					}else{
						char *tag = vlc_getMeta(vlc, i);
						if (tag){
							if (*tag){
								removeTrailingSpaces(tag);
								//printf("# tag %i '%s'\n", i, tag);
								tagAdd(vp->tagc, path, i, tag, 1);
							}
							my_free(tag);
						}
					}
				}
			}
		}
		my_free(path);
	}
}

static inline void metaUpdateRemoteTrackMeta (TVLCPLAYER *vp, TVLCCONFIG *vlc)
{
	char *path = getPlayingPath(vp);
	if (path){
		if (*path){
			if (isMediaRemote(path)){
				for (int i = 0; i <= MTAG_TrackID; i++){
					char *tag = vlc_getMeta(vlc, i);
					if (tag){
						if (*tag)
							tagAdd(vp->tagc, path, i, tag, 1);
						my_free(tag);
					}
				}
			}
		}
		my_free(path);
	}
}

// TIMER_META_UPDATE
void metaGetUpdate (TVLCPLAYER *vp)
{
	if (!getPlayState(vp)) return;
	
	//printf("TIMER_META_UPDATE %i\n", getPlayState(vp));
	
	TMETA *meta = pageGetPtr(vp, PAGE_META);
	TVLCCONFIG *vlc = vp->vlc;
	
	if (loadLock(vp)){
		if (meta->extra) vlc_metaExtraFree(meta->extra);
		meta->extra = vlc_metaExtraGet(vlc);
		loadUnlock(vp);
	}
	metaUpdateRemoteTrackMeta(vp, vlc);
	metaUpdateLocalTrackMeta(vp, vlc);
	
	//printf("TIMER_META_UPDATE out\n");
}

static inline void buildPlaylistRoute (TSTACK *stack, PLAYLISTCACHE *plc)
{
	if (plc){
		stackPush(stack, plc->uid);
		
		if (plc->parent)
			buildPlaylistRoute(stack, plc->parent);
	}
}

char *metaGetMetaAll (TVLCPLAYER *vp, PLAYLISTCACHE *plc, const int trackId, const char *lineBreak)
{
	//printf("metaGetMetaAll\n");
	
	const unsigned int hash = playlistGetHashById(plc, trackId);
	if (!hash) return NULL;
	
	int len = MAX_PATH_UTF8*2;		// 4meg should be enough to cover everything
	char *meta = my_calloc(len+1, sizeof(char));
	if (!meta) return NULL;


	TSTACK *stack = stackCreate(8);
	buildPlaylistRoute(stack, plc);

	intptr_t uid = 0;
	if (stackPop(stack, &uid)){
		strncat(meta, "Route: ", len);
		while(uid){
			if (uid > PLAYLIST_UID_BASE+1){
				char *name = playlistManagerGetName(vp->plm, uid);
				if (name){
					strncat(meta, "/", len);
					strncat(meta, name, len);
					my_free(name);
				}
			}
			if (!stackPop(stack, &uid))
				break;
		}
		strncat(meta, lineBreak, len);
	}
	stackDestroy(stack);

	
	char *playlist = playlistGetNameDup(plc);
	if (playlist){
		strncat(meta, "Playlist: ", len);
		strncat(meta, playlist, len);
		strncat(meta, lineBreak, len);
		my_free(playlist);
	}
			
	for (int i = 0; i < MTAG_TOTAL; i++){
		char *tag = tagRetrieveDup(vp->tagc, hash, i);
		if (tag){
			strncat(meta, tagStrLookup[i], len);
			strncat(meta, ": ", len);
			strncat(meta, tag, len);
			strncat(meta, lineBreak, len);
			my_free(tag);
		}
	}
	
	int imgId = playlistGetArtIdById(plc, trackId);
	if (imgId){
		char *path = artManagerImageGetPath8(vp->am, imgId);
		if (path){
			strncat(meta, "Art: ", len);
			strncat(meta, path, len);
			strncat(meta, lineBreak, len);
			my_free(path);
		}
	}
	
	if (isPlayingItemId(vp, plc->uid, trackId)){
		char buffer[128];
		libvlc_media_stats_t stats;
		
		if (vlc_getStats(vp->vlc, &stats)){
			__mingw_snprintf(buffer, 127, "%.0f kb/s", stats.f_demux_bitrate*1000.0*8.0);
			strncat(meta, "Bitrate: ", len);
			strncat(meta, buffer, len);
			strncat(meta, lineBreak, len);
		}
		
		if (vp->vlc->hasAttachments > 0){
			__mingw_snprintf(buffer, 127, "%i", vp->vlc->hasAttachments);
			strncat(meta, "Attachments: ", len);
			strncat(meta, buffer, len);
			strncat(meta, lineBreak, len);
		}

		TMETA *metaPage = pageGetPtr(vp, PAGE_META);
		if (metaPage->extra && metaPage->extra->total){
			for (int i = 0; i < metaPage->extra->total; i++){
				if (*metaPage->extra->meta[i]->value){
					if (metaPage->extra->meta[i]->name)
						strncat(meta, metaPage->extra->meta[i]->name, len);
					strncat(meta, ": " , len);
					if (metaPage->extra->meta[i]->value)
						strncat(meta, metaPage->extra->meta[i]->value, len);
					strncat(meta, lineBreak, len);
				}
			}
		}
		
		char *opts = playlistGetOptionsDup(plc, trackId);
		if (opts){
			strncat(meta, "Options: ", len);
			strncat(meta, opts, len);
			strncat(meta, lineBreak, len);
			my_free(opts);
		}

		if (pageGetPtr(vp, PAGE_ES)){
			int total = 0;
			TCATEGORY *es = getCategories(vp->vlc, &total);
			if (es){
				// find expected string length to re-allocating buffer to correct size
				int esLength = 0;
				for (int i = 0; i < total; i++){
					if (es[i].name) esLength += strlen(es[i].name) + 8;
					
					for (int j = 0; es[i].infos && j < es[i].tInfos; j++){
						if (es[i].infos[j].name) esLength += strlen(es[i].infos[j].name) + 2;
						if (es[i].infos[j].value) esLength += strlen(es[i].infos[j].value) + 4;
					}
				}
				
				if (esLength){
					len = esLength + strlen(meta) + 1;
					//printf("esLength %i %i\n", esLength, len);
					
					meta = my_realloc(meta, len);
					if (meta){
						for (int i = 0; i < total; i++){
							if (es[i].name){
								strncat(meta, "  ::", len);
								strncat(meta, es[i].name, len);
								strncat(meta, lineBreak, len);
							}
                	
							for (int j = 0; es[i].infos && j < es[i].tInfos; j++){
								if (es[i].infos[j].name)
									strncat(meta, es[i].infos[j].name, len);
								strncat(meta, ": ", len);
								if (es[i].infos[j].value)
									strncat(meta, es[i].infos[j].value, len);
								strncat(meta, lineBreak, len);
							}
						}
					}
				}
				esFreeCategories(es, total);
			}
		}
	}

	//printf("metaGetMetaAll EXIT %i %i\n", len, strlen(meta)+1);
	return meta;
}

int metaRender (TVLCPLAYER *vp, TFRAME *frame, TMETA *meta, TMETADESC *desc, const int font, const int drawui)
{

	//printf("metaRender\n");

	int x = desc->x;
	int y = desc->y;
	int w = desc->w;
	int h = desc->h;
		
	const unsigned int *col = swatchGetPage(vp, PAGE_META);
	
	if (!desc->textHeight){
		lGetFontMetrics(frame->hw, font, NULL, &desc->textHeight, NULL, NULL, NULL);
		desc->textHeight += 1;
	}
	
	if (drawui > 0){
		transBlur(frame, 6);
		buttonsRenderAll(meta->btns, frame, BUTTONS_RENDER_HOVER);
	}


	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, desc->uid);
	int pos;
	
	if (desc->trackPosition >= 0){
		pos = desc->trackPosition;
	}else{
		pos = plc->pr->playingItem;
		if (pos < 0) pos = 0;
	}
	
	//printf("metaRender %i '%s'\n", pos, plc->title);
	//printf("'%s' %i\n", plc->name, playlistGetItemType(plc, pos) == PLAYLIST_OBJTYPE_PLC);


	TMETAARTWORK *mart = (TMETAARTWORK*)&((TMETA*)pageGetPtr(vp, PAGE_META))->desc.art;
	/*int xw = */drawArtwork(vp, frame, mart, playlistGetArtId(plc, pos), desc->x, desc->y, desc->w, desc->h, drawui);
	if (drawui > 0){
		//doBlur(frame, x, y, (x+w)-xw, y+h, 3);
		lDrawRectangle(frame, x-1, y-1, x+w+1, y+h+1, col[SWH_META_PAGEBORDER]);
	}

	const unsigned int hash = playlistGetHash(plc, pos);
	if (!hash) return 0;
	
	char tag[MAX_PATH_UTF8+1];
	char plname[MAX_PATH_UTF8+1];
	playlistGetName(plc, plname, MAX_PATH_UTF8);

	lSetCharacterEncoding(vp->ml->hw, CMT_UTF8);
#if 0
	lSetRenderEffect(vp->ml->hw, LTR_SHADOW);
	lSetFilterAttribute(vp->ml->hw, LTR_SHADOW, 0, LTRA_SHADOW_S|LTRA_SHADOW_E | LTRA_SHADOW_S3 | LTRA_SHADOW_OS(1) | LTRA_SHADOW_TR(127));
#else
	const int blurOp = LTR_BLUR6;
	lSetRenderEffect(vp->ml->hw, blurOp);
	//lRenderEffectReset(vp->ml->hw, font, blurOp);
	lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_COLOUR, COL_HOVER);
	lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_RADIUS, 4);
	lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_SETTOP, 1);
	lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_Y, 1);
	lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_ALPHA, 950);
#endif

	// confine text rendering to the inner rectangle of the meta frame
	TLPRINTR rt;
	memset(&rt, 0, sizeof(rt));
	rt.bx1 = x+3;
	rt.by1 = y+2;
	rt.bx2 = (x+w)-1;
	rt.by2 = (y+h)-1;
	rt.sx = rt.bx1;
	rt.sy = rt.by1;

	const int isPlaylist = (playlistGetItemType(plc, pos) == PLAYLIST_OBJTYPE_PLC);

	for (int i = 0; i < MTAG_TOTAL; i++){
		if (i == MTAG_ArtworkPath) continue;

		tagRetrieveByHash(vp->tagc, hash, i, tag, MAX_PATH_UTF8);
		if (*tag){
			lSetForegroundColour(vp->ml->hw, col[SWH_META_METANAME]);
			lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_COLOUR, COL_ORANGE);
			lPrintEx(frame, &rt, font, PF_CLIPWRAP|PF_CLIPDRAW|PF_FORCEAUTOWIDTH, LPRT_CPY, "%s: ",tagStrLookup[i]);
			lSetForegroundColour(vp->ml->hw, 0xFF<<24 | col[SWH_META_METAVALUE]);
			lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_COLOUR, COL_CYAN);

			if (i == MTAG_POSITION){
				if (!isPlaylist){
					if (*plname)	// we may never actually reach here..
						lPrintEx(frame, &rt, font, PF_FORCEAUTOWIDTH|PF_USELASTX|PF_CLIPWRAP|PF_CLIPDRAW, LPRT_CPY, "%s  - \u300C%s\u300D", tag, plname);
				}else{
					lPrintEx(frame, &rt, font, PF_FORCEAUTOWIDTH|PF_USELASTX|PF_CLIPWRAP|PF_CLIPDRAW, LPRT_CPY, "\u300C%s\u300D", plname);
				}
			}else{
				lPrintEx(frame, &rt, font, PF_FORCEAUTOWIDTH|PF_USELASTX|PF_CLIPWRAP|PF_CLIPDRAW|PF_IGNOREFORMATTING, LPRT_CPY, tag);
			}
			
			rt.sx = rt.bx1;
			rt.sy += desc->textHeight;		// new line
		}
	}

	if (isPlayingItem(vp, plc, pos)){
		libvlc_media_stats_t stats;
		if (vlc_getStats(vp->vlc, &stats)){
			lSetForegroundColour(vp->ml->hw, col[SWH_META_METANAME]);
			lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_COLOUR, COL_ORANGE);
			lPrintEx(frame, &rt, font, PF_CLIPWRAP|PF_CLIPDRAW|PF_FORCEAUTOWIDTH|PF_IGNOREFORMATTING, LPRT_CPY, "Bitrate: ");
			lSetForegroundColour(vp->ml->hw, 0xFF<<24 | col[SWH_META_METAVALUE]);
			lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_COLOUR, COL_CYAN);
			
			lPrintEx(frame, &rt, font, PF_FORCEAUTOWIDTH|PF_USELASTX|PF_CLIPWRAP|PF_CLIPDRAW, LPRT_CPY, "%.0f kb/s", stats.f_demux_bitrate*1000.0*8.0);
				rt.sy += desc->textHeight;
		}
		
		if (vp->vlc->hasAttachments > 0){
			lSetForegroundColour(vp->ml->hw, col[SWH_META_METANAME]);
			lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_COLOUR, COL_ORANGE);
			lPrintEx(frame, &rt, font, PF_FORCEAUTOWIDTH|PF_CLIPWRAP|PF_CLIPDRAW|PF_IGNOREFORMATTING, LPRT_CPY, "Attachments: ");
			lSetForegroundColour(vp->ml->hw, col[SWH_META_METAVALUE]);
			lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_COLOUR, COL_CYAN);
			lPrintEx(frame, &rt, font, PF_FORCEAUTOWIDTH|PF_USELASTX|PF_CLIPWRAP|PF_CLIPDRAW, LPRT_CPY, "%i", vp->vlc->hasAttachments);
			rt.sy += desc->textHeight;
		}		


		if (meta->extra && meta->extra->total){
			for (int i = 0; i < meta->extra->total; i++){
				if (*meta->extra->meta[i]->value){
					lSetForegroundColour(vp->ml->hw, col[SWH_META_METANAME]);
					lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_COLOUR, COL_ORANGE);
					lPrintEx(frame, &rt, font, PF_CLIPWRAP|PF_CLIPDRAW|PF_FORCEAUTOWIDTH, LPRT_CPY, "%s: ", meta->extra->meta[i]->name);
					lSetForegroundColour(vp->ml->hw, 0xFF<<24 | col[SWH_META_METAVALUE]);
					lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_COLOUR, COL_CYAN);
					lPrintEx(frame, &rt, font, PF_FORCEAUTOWIDTH|PF_USELASTX|PF_CLIPWRAP|PF_CLIPDRAW|PF_IGNOREFORMATTING, LPRT_CPY, meta->extra->meta[i]->value);
				
					rt.sx = rt.bx1;
					rt.sy += desc->textHeight;		// new line
				}
			}
		}

		if (hasPageBeenAccessed(vp, PAGE_ES)){
			TSTREAMINFO *sinfo = pageGetPtr(vp, PAGE_ES);
			if (sinfo->atracks){
				if (drawAVTrack(frame, &rt, font, blurOp, col, "Audio: ", sinfo->atracks, sinfo->currentAudioTrack))
					rt.sy += desc->textHeight;
			}
			if (sinfo->vtracks){
				if (drawAVTrack(frame, &rt, font, blurOp, col, "Video: ", sinfo->vtracks, sinfo->currentVideoTrack))
					rt.sy += desc->textHeight;
			}
		}
	}

	if (playlistGetOptions(plc, pos, tag, MAX_PATH_UTF8)){
		if (*tag){
			lSetForegroundColour(vp->ml->hw, col[SWH_META_METANAME]);
			lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_COLOUR, COL_ORANGE);
			lPrintEx(frame, &rt, font, PF_FORCEAUTOWIDTH|PF_CLIPWRAP|PF_CLIPDRAW|PF_IGNOREFORMATTING, LPRT_CPY, "Options: ");
			lSetForegroundColour(vp->ml->hw, col[SWH_META_METAVALUE]);
			lSetFilterAttribute(vp->ml->hw, blurOp, LTRA_BLUR_COLOUR, COL_CYAN);
			lPrintEx(frame, &rt, font, PF_FORCEAUTOWIDTH|PF_USELASTX|PF_CLIPWRAP|PF_CLIPDRAW|PF_DONTFORMATBUFFER, LPRT_CPY, tag);
			rt.sy += desc->textHeight;
		}
	}

	shadowTextDisable(frame->hw);

	if (drawui) return 1;

	if (vp->gui.drawMetaTrackbar && meta->trackSlider && !getIdle(vp)){
		const int playState = getPlayState(vp);
		if (playState == 1 || playState == 2){
			sliderSetValueFloat(meta->trackSlider, vp->vlc->position);
			ccRender(meta->trackSlider, frame);
		}
	}

	return 1;
}

static inline int page_metaRender (TMETA *meta, TVLCPLAYER *vp, TFRAME *frame)
{
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, meta->desc.uid);
	/*if (pageGetPrevious(vp) == PAGE_OVERLAY){		// fix this, remove the need for this hack
	  	plc = getQueuedPlaylist(vp);				// add setter methods for playlist and track items
	  	if (!plc)
	  		plc = getDisplayPlaylist(vp);
	}else if (pageGet(vp) == PAGE_HOME){
		plc = getQueuedPlaylist(vp);
	}*/
	

	//if (!plc) plc = getDisplayPlaylist(vp);
	if (!plc){
		//printf("page_metaRender: invalid playlist %X\n", meta->desc.uid);
		return 0;
	}
	
	//printf("metaDraw  %X, %i\n", plc->uid, meta->desc.trackPosition);
	return metaRender(vp, frame, meta, &meta->desc, META_FONT, 1);
}

static inline int page_metaInput (TMETA *meta, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	//printf("@ page_metaInput %i %i %p\n", msg, flags, pos);

	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
		metaButtonPress(meta, NULL, METABUTTON_UP, NULL);
		break;
			
	  case PAGE_IN_WHEEL_BACK:
		metaButtonPress(meta, NULL, METABUTTON_DOWN, NULL);
		break;

	  case PAGE_IN_WHEEL_LEFT:
		metaButtonPress(meta, NULL, METABUTTON_LEFT, NULL);
		break;
		
	  case PAGE_IN_WHEEL_RIGHT:
		metaButtonPress(meta, NULL, METABUTTON_RIGHT, NULL);
		break;
		
	  case PAGE_IN_TOUCH_DOWN:
	  	page2Set(vp->pages, PAGE_META, 0);
		page2SetPrevious(meta);
		break;
		
	  /*case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  	break;*/
	}
		
	return 1;
}

static inline int64_t metaCc_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	return 1;
}

static inline int page_metaStartup (TMETA *meta, TVLCPLAYER *vp, const int fw, const int fh)
{
	
	//printf("page_metaStartup %i %i\n", PAGE_META, METABUTTON_TOTAL);
	
	TMETADESC *desc = &meta->desc;
	desc->trackPosition = 0;
	desc->x = 10;
	desc->y = 10;
	desc->w = fw-1 - (desc->x*2);
	desc->h = fh-1 - (desc->y*2);
	desc->art.img = lNewFrame(getFrontBuffer(vp)->hw, 8, 8, LFRM_BPP_32);
	desc->art.hashTrack = 0;
	desc->art.hashArt = 0;
	desc->uid = 0;
	
	meta->extra = NULL;
	meta->btns = buttonsCreate(vp->cc, PAGE_META, METABUTTON_TOTAL, ccbtn_cb);
	
	TCCBUTTON *btn = buttonsCreateButton(meta->btns, L"meta/mdown.png", L"meta/mdownhl.png", METABUTTON_DOWN, 1, 0, 0, 0);
	ccSetPosition(btn, abs(fw - ccGetWidth(btn))/2, (fh-1) - ccGetHeight(btn));
	buttonFaceAutoSwapEnable(btn, BUTTON_FACE_SWAP_SELECTED);

	btn = buttonsCreateButton(meta->btns, L"meta/mright.png", L"meta/mrighthl.png", METABUTTON_RIGHT, 1, 0, 0, 0);
	ccSetPosition(btn, (fw-1) - ccGetWidth(btn), abs(fh - ccGetHeight(btn))/2);
	buttonFaceAutoSwapEnable(btn, BUTTON_FACE_SWAP_SELECTED);

	btn = buttonsCreateButton(meta->btns, L"meta/mup.png", L"meta/muphl.png", METABUTTON_UP, 1, 0, 0, 0);
	ccSetPosition(btn, abs(fw - ccGetWidth(btn))/2, 0);
	buttonFaceAutoSwapEnable(btn, BUTTON_FACE_SWAP_SELECTED);
	
	btn = buttonsCreateButton(meta->btns, L"meta/mleft.png", L"meta/mlefthl.png", METABUTTON_LEFT, 1, 0, 0, 0);
	ccSetPosition(btn, 0, abs(fh - ccGetHeight(btn))/2);
	buttonFaceAutoSwapEnable(btn, BUTTON_FACE_SWAP_SELECTED);

	btn = buttonsCreateButton(meta->btns, L"meta/mclose.png", NULL, METABUTTON_CLOSE, 1, 0, 0, 0);
	ccSetPosition(btn, abs(fw - ccGetWidth(btn))/2, abs(fh - ccGetHeight(btn))/2);
	btn->flags.disableRender = 1;
	
	settingsGet(vp, "meta.drawTrackbar", &vp->gui.drawMetaTrackbar);
	//if (vp->gui.drawMetaTrackbar){	// display a passive track position (input is disabled)
		int x1 = 12;
		int width = fw - (x1*2);
		int height = 24;
		int y1 = fh - height - 48;
		
		TSLIDER *slider = meta->trackSlider = ccCreate(vp->cc, PAGE_NONE, CC_SLIDER_HORIZONTAL, metaCc_cb, NULL, width, height);
		slider->isChild = 1;	// foobar the inputchain
		slider->canDrag = 0;	// disable input
		slider->pad.top = 8;
		slider->pad.btm = 8;
		sliderFaceSet(slider, SLIDER_FACE_LEFT, L"cc/slider_h_solid_left.png");
		sliderFaceSet(slider, SLIDER_FACE_RIGHT, L"cc/slider_h_solid_right.png");
		sliderFaceSet(slider, SLIDER_FACE_MID, L"cc/slider_h_solid_mid.png");
		sliderFaceSet(slider, SLIDER_FACE_TIP, L"cc/slider_h_solid_tip.png");
		sliderFacesApply(slider);
		sliderHoverDisable(slider);
		ccSetMetrics(slider, x1, y1, -1, -1);
		sliderSetRange(slider, 0, 10000000);
		sliderSetValue(slider, 0);
		ccInputDisable(slider);
		ccEnable(slider);
	//}

	return 1;
}

static inline int page_metaInitalize (TMETA *meta, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_META);
	swatchSetColour(vp, PAGE_META, SWH_META_METANAME, 0xFFF0F0F0);

	return 1;
}

static inline int page_metaShutdown (TMETA *meta, TVLCPLAYER *vp)
{

	if (meta->trackSlider)
		ccDelete(meta->trackSlider);
	
	if (meta->extra)
		vlc_metaExtraFree(meta->extra);

	buttonsDeleteAll(meta->btns);
	
	if (meta->desc.art.img)
		lDeleteFrame(meta->desc.art.img);

	return 1;
}

static inline int page_metaRenderStart (TMETA *meta, TVLCPLAYER *vp, TFRAME *frame, void *opaquePtr)
{
	lRenderEffectReset(vp->ml->hw, META_FONT, LTR_BLUR6);
	
	// don't open the meta page if track is no longer valid
	TMETADESC *desc = &meta->desc;
	PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(vp->plm, desc->uid);
	if (plc)
		return (playlistGetItemType(plc, desc->trackPosition) == PLAYLIST_OBJTYPE_TRACK);
	
	return 0;
}

int page_metaCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TMETA *meta = (TMETA*)pageStruct;
	
	//if (msg != PAGE_CTL_RENDER)
	//	 printf("# page_metaCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_metaRender(meta, meta->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_metaInput(meta, meta->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_RENDER_START){
		return page_metaRenderStart(meta, meta->com->vp, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){

	}else if (msg == PAGE_CTL_STARTUP){
		return page_metaStartup(meta, meta->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_metaInitalize(meta, meta->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_metaShutdown(meta, meta->com->vp);
		
	}
	
	return 1;
}

