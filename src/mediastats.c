
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



int msGetStatDisplayState (TVLCPLAYER *vp)
{
	return page2RenderGetState(vp->pages, PAGE_MEDIASTATS);
}

void msSetStatDisplayState (TVLCPLAYER *vp, const int state)
{
	vp->gui.displayStats = state&0x01;
	if (vp->gui.displayStats){
		page2Set(vp->pages, PAGE_RENDER_CONCURRENT|PAGE_MEDIASTATS, 0);
	}else{
		page2Set(vp->pages, PAGE_MEDIASTATS, 0);		// remove concurrent
		page2RenderDisable(vp->pages, PAGE_MEDIASTATS);
	}
}

static inline void msRenderStatInt (TFRAME *frame, TMEDIASTATS *ms, const int flags, const char *str, const int val)
{
	if (!val) return;
	
	lPrintEx(frame, &ms->rt, ms->font, ms->flags|flags, LPRT_CPY, "%s: %i ", str, val);
	ms->rt.ey += 2;
}

static inline void msRenderStatInt64 (TFRAME *frame, TMEDIASTATS *ms, const int flags, const char *str, uint64_t val)
{
	if (!val) return;
	
	lPrintEx(frame, &ms->rt, ms->font, ms->flags|flags, LPRT_CPY, "%s: %I64d ", str, val);
	ms->rt.ey += 2;
}

static inline void msRenderStatFloat (TFRAME *frame, TMEDIASTATS *ms, const int flags, const char *str, const float val)
{
	if (val == 0.0f) return;
	
	lPrintEx(frame, &ms->rt, ms->font, ms->flags|flags, LPRT_CPY, "%s: %.1f ", str, val);
	ms->rt.ey += 2;
}

static inline int page_msRender (TMEDIASTATS *ms, TVLCPLAYER *vp, TFRAME *frame)
{
	if (!getPlayState(vp) || getPlayState(vp) == 8)
		return 1;
	
	libvlc_media_stats_t *stats = &ms->stats;
	
	uint64_t t0 = getTickCount();
	if (t0 - ms->time >= 990){
		ms->time = t0;
		memset(stats, 0, sizeof(libvlc_media_stats_t));
		
		if (vp->vlc->m)
			ms->isValid = libvlc_media_get_stats(vp->vlc->m, stats);
	}
	
	if (!ms->isValid) return 1;

	memset(&ms->rt, 0, sizeof(TLPRINTR));
	ms->rt.sy = ms->y;
	
	outlineTextEnable(frame->hw, 0xFF000000);
	
	msRenderStatInt(frame, ms, 0, "read bytes", stats->i_read_bytes);
	msRenderStatInt(frame, ms, PF_NEWLINE, "input bitrate", stats->f_input_bitrate*10000.0);
	
	msRenderStatInt(frame, ms, PF_NEWLINE, "demux read bytes", stats->i_demux_read_bytes);
	msRenderStatInt(frame, ms, PF_NEWLINE, "demux bitrate", stats->f_demux_bitrate*10000.0);
	msRenderStatInt(frame, ms, PF_NEWLINE, "demux corrupted", stats->i_demux_corrupted);
	msRenderStatInt(frame, ms, PF_NEWLINE, "demux discontinuity", stats->i_demux_discontinuity);
	
	
	msRenderStatInt(frame, ms, PF_NEWLINE, "decoded video", stats->i_decoded_video);
	msRenderStatInt(frame, ms, PF_NEWLINE, "decoded audio", stats->i_decoded_audio);
	
	msRenderStatInt(frame, ms, PF_NEWLINE, "displayed pictures", stats->i_displayed_pictures);
	msRenderStatInt(frame, ms, PF_NEWLINE, "lost pictures", stats->i_lost_pictures);
	
	msRenderStatInt(frame, ms, PF_NEWLINE, "played abuffers", stats->i_played_abuffers);
	msRenderStatInt(frame, ms, PF_NEWLINE, "lost abuffers", stats->i_lost_abuffers);
	
	msRenderStatInt(frame, ms, PF_NEWLINE, "sent packets", stats->i_sent_packets);
	msRenderStatInt(frame, ms, PF_NEWLINE, "sent bytes", stats->i_sent_bytes);
	msRenderStatInt(frame, ms, PF_NEWLINE, "send bitrate", stats->f_send_bitrate*10000.0);

	msRenderStatInt(frame, ms, PF_NEWLINE, "artwork registered", artManagerCount(vp->am));
	msRenderStatInt(frame, ms, PF_NEWLINE, "artwork built", artManagerSurfaceCount(vp->am));
	msRenderStatInt(frame, ms, PF_NEWLINE, "artwork acquired", artManagerUnreleasedCount(vp->am));
	msRenderStatInt(frame, ms, PF_NEWLINE, "artwork overhead (KB)", artManagerMemUsage(vp->am)/1024);
	
	outlineTextDisable(frame->hw);

	return 1;
}

static inline int page_msInput (TMEDIASTATS *ms, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	/*switch(msg){
	  case PAGE_IN_TOUCH_DOWN:
	  case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_UP:
	  	break;
	}*/
		
	return 0;
}

static inline int page_msStartup (TMEDIASTATS *ms, TVLCPLAYER *vp, const int width, const int height)
{

	return 1;
}

static inline int page_msInitalize (TMEDIASTATS *ms, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_MEDIASTATS);
	
	memset(&ms->stats, 0, sizeof(libvlc_media_stats_t));
	ms->time = 0;
	ms->flags = PF_CLIPWRAP|PF_CLIPDRAW|PF_RIGHTJUSTIFY;
	ms->font = DMSG_FONT;
	ms->y = 32;
	
	page2InputDisable(vp->pages, PAGE_MEDIASTATS);
	return 1;
}

static inline int page_msShutdown (TMEDIASTATS *ms, TVLCPLAYER *vp)
{
	return 1;
}

void page_msRenderEnd (TMEDIASTATS *ms, TVLCPLAYER *vp, int64_t destId, int64_t data2, void *opaquePtr)
{
	//printf("page_msRenderEnd %i\n", vp->gui.displayStats);
}

int page_msCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TMEDIASTATS *ms = (TMEDIASTATS*)pageStruct;
	
	// printf("# page_msCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_msRender(ms, ms->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		
	}else if (msg == PAGE_CTL_RENDER_END){
		page_msRenderEnd(ms, ms->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_msInput(ms, ms->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_msStartup(ms, ms->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_msInitalize(ms, ms->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_msShutdown(ms, ms->com->vp);
		
	}
	
	return 1;
}

