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


#if 0

#include "common.h"





// see vlceventcb.c
extern volatile int SHUTDOWN;




static inline int artworkIsValid8 (const char *path)
{
	if (!path || !*path) return 0;

	int ret = 0;
	if (!strncmp(path, "file:///", 8)){
		char *out = decodeURI(path, strlen(path));
		if (out){
			ret = doesFileExistUtf8(out);
			my_free(out);
		}
	}
	return ret;
}
#if 0

int artworkLoad (TVLCPLAYER *vp, TTAGIMG *art, const wchar_t *path)
{
	if (art->aimg[0].image) return 2;
	if (!doesFileExistW(path)) return 0;
	
	//int tid = (int)GetCurrentThreadId();
	//float t0 = getTime(vp);
	//printf("loadArt %i\n", tid);
	//wprintf(L"loadart %x '%s'\n", art->hash, path);
	
	int ret = 0;

	TFRAME *newImg = newImage(vp, path, LFRM_BPP_32A);
	if (newImg){
		int w, h;
		imageBestFit(vp->gui.artMaxWidth, vp->gui.artMaxHeight, newImg->width, newImg->height, &w, &h);
		TFRAME *img = lNewFrame(vp->ml->hw, w, h, LFRM_BPP_32);
		if (img){
			//copyAreaScaled(newImg, img, 0, 0, newImg->width, newImg->height, 0, 0, w, h);	// slower
			transScale(newImg, img, w, h, 0, 0, SCALE_BILINEAR);	// faster
			art->aimg[0].image = img;
			art->aimg[0].scale = 1.000;
			art->aimg[0].swidth = img->width;
			art->aimg[0].sheight = img->height;
			art->timeAdded = getTickCount();
			art->enabled = 1;
			ret = 1;
			//float t1 = getTime(vp)-t0;
			//printf("loadArt %i Exit %.2f\n", tid, t1);
			//printf("loadArt %i Exit\n", tid);
		}
		lDeleteFrame(newImg);
	}
	
	//float t1 = getTime(vp)-t0;
	//printf("loadArt %i Exit %.2f\n", tid, t1);
	//printf("loadArt %i Exit\n", tid);
	return ret;
}
#endif

void artworkEvent_MediaParsedChanged (art_work *aw, art_job *aj, job_detail *jd)
{

	int art = 0;
	char url[MAX_PATH_UTF8+1];
		
	tagRetrieveByHash(jd->tagc, jd->hash, MTAG_ArtworkURL, url, MAX_PATH_UTF8);
	if (*url) art = artworkIsValid8(url);
		
	if (!*url || !art){
		char *artmrl = libvlc_media_get_meta(jd->m, MTAG_ArtworkURL);
		if (artmrl){
			if (!strncmp(artmrl, "file:", 5)){
				artJobTimeSet(aj, getTickCount() + 500); // got an mrl, let VLC process media before attempting read
				tagAddByHash(jd->tagc, jd->path, jd->hash, MTAG_ArtworkURL, artmrl, 1);
				artWorkSignalReady(aw);
			}
			libvlc_free(artmrl);
		}
	}
}

void artwork_EventCB (const libvlc_event_t *event, void *udata)
{
	if (SHUTDOWN) return;
	
	job_thread *jt = libvlc_media_get_user_data(event->p_obj);
	if (!jt) return;
	art_work *aw = artThreadGetWork(jt);
	if (!aw) return;

	const int juid = (int)udata;
	art_job *aj = artJobFind(aw, juid);
	if (!aj) return;
	
	//printf("%s: %i %p %p\n", vlc_EventTypeToName(event->type), juid, aw, aj);


	if (event->type == libvlc_MediaParsedChanged)
		artworkEvent_MediaParsedChanged(aw, aj, &aj->detail);
}

libvlc_media_t *artworkCreateMediaInstance (libvlc_instance_t *hLib, const char *path, void *udata)
{
	libvlc_media_t *m = libvlc_media_new_path(hLib, path);
	if (m){
		libvlc_media_set_user_data(m, udata);
		libvlc_media_add_option(m, "no-audio");
		libvlc_media_add_option(m, "no-video");
		libvlc_media_add_option(m, "album-art=2");
		//libvlc_media_add_option(m, "ignore-config");
		
		return m;
	}
	return NULL;
}

void vlcEventGetArt (libvlc_instance_t *hLib, job_controller *jc, TARTMANAGER *am, const char *path, const int hash, const TMETACOMPLETIONCB *mccb, const unsigned int uid, const int position)
{
	
	if (!artThreadLockWait(jt)) return;
	
	if (artDetailFind(artThreadGetWork(jt), hash)){
		//printf("vlcEvent GetArt found, quiting %X\n", hash);
		artThreadLockRelease(jt);
		return;
	}
	
	job_detail jd;
	memset(&jd, 0, sizeof(job_detail));
	
	char url[MAX_PATH_UTF8+1];
	tagRetrieveByHash(tagc, hash, MTAG_ArtworkURL, url, MAX_PATH_UTF8);

	if (!*url)
		jd.m = artworkCreateMediaInstance(hLib, path, jt);
	
	jd.tagc = tagc;
	jd.pageSource = 0;
	jd.hash = hash;
	jd.path = my_strdup(path);
	jd.playlistUID = uid;
	jd.trackPosition = position;
	jd.mccb = mccbDup(mccb);

	art_job *aj = jobDetailAttach(artThreadGetWork(jt), &jd);

	if (jd.m){
		jd.em = libvlc_media_event_manager(jd.m);
		libvlc_event_attach(jd.em, libvlc_MediaParsedChanged, artwork_EventCB, (void*)aj->jobId);
		libvlc_media_parse_async(jd.m);

		char *tag = libvlc_media_get_meta(jd.m, MTAG_ArtworkURL);
		if (tag) libvlc_free(tag);
	}
	
	artWorkSignalReady(artThreadGetWork(jt));
	artThreadLockRelease(jt);
}


#endif

