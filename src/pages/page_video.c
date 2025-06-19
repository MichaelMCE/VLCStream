

// https://github.com/MichaelMCE/VLCStream
// 

// okio@users.sourceforge.net

//  Copyright (c) 2005-2025  
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


#include "../common.h"
#include <rgbmasks.h>


void video_saveSnapshot (TVLCPLAYER *vp, TFRAME *frame, wchar_t *filename, const int annouce)
{
	if (lSaveImage(frame, filename, IMG_PNG, 0, 0) && annouce)
		dbwprintf(vp, L"Snapshot written to %ls", filename);
}

void video_copySourceFrame (TVLCPLAYER *vp)
{
	my_memcpy((uint32_t*)vp->ctx.pixelBuffer, (uint32_t*)vp->ctx.pixels, vp->ctx.bufferSize);
}

void video_copyToDesktop (TVLCPLAYER *vp, TFRAME *front, const int cx, const int cy)
{
	HDC hdc = GetDC(vp->ctx.winRender.wnd);		// render to desktop

	SetDIBitsToDevice(hdc, cx, cy, front->width, front->height, 0, 0, 0, front->height, front->pixels, vp->ctx.winRender.bitHdr, DIB_RGB_COLORS);
	ReleaseDC(vp->ctx.winRender.wnd, hdc);
}

void video_drawFPSOverlay (TVLCPLAYER *vp, TFRAME *frame, const float fps, const int x, int y)
{
	//const unsigned int *col = swatchGetPage(vp, PAGE_OVERLAY);
	lSetForegroundColour(frame->hw, 0xFFFFFFFF);
	lSetCharacterEncoding(frame->hw, CMT_UTF8);
	outlineTextEnable(frame->hw, 0xFF000000);

	const int showVideoFps = getPlaybackMode(vp) == PLAYBACKMODE_VIDEO/* || (getPlaybackMode(vp) == PLAYBACKMODE_AUDIO && vp->gui.visual)*/;
	y -= 72;
	if (showVideoFps) y -= 22;

	const uint64_t mem = processGetMemUsage(vp->pid);
	lPrintf(frame, x-10, y += 22, UPDATERATE_FONT, LPRT_CPY, "%.1f", (double)mem/1024.0/1024.0);

	if (showVideoFps){
		double t = 0.0;
		for (int i = 0; i < 30; i++) t += vp->vlc->dTime[i];
		t /= 30.0;
		if (t > 1.0)
			lPrintf(frame, x, y += 22, UPDATERATE_FONT, LPRT_CPY, "%.1f", 1000.0/t);
	}

	lPrintf(frame, x-4, y += 22, UPDATERATE_FONT, LPRT_CPY, "%.2f", vp->rTime);
	lPrintf(frame, x, y += 22, UPDATERATE_FONT, LPRT_CPY, "%.1f", fps);
	outlineTextDisable(frame->hw);
}

static inline void _imageBestFit (const int bg_w, const int bg_h, int fg_w, int fg_h, int *w, int *h)
{
	const int fg_sar_num = 1; const int fg_sar_den = 1;
	const int bg_sar_den = 1; const int bg_sar_num = 1;

	if (fg_w < 1 || fg_w > 8191) fg_w = bg_w;
	if (fg_h < 1 || fg_h > 8191) fg_h = bg_h;
	*w = bg_w;
	*h = (bg_w * fg_h * fg_sar_den * bg_sar_num) / (float)(fg_w * fg_sar_num * bg_sar_den);
	if (*h > bg_h){
		*w = (bg_h * fg_w * fg_sar_num * bg_sar_den) / (float)(fg_h * fg_sar_den * bg_sar_num);
		*h = bg_h;
	}
}

static inline void copyImage (TFRAME *src, TFRAME *des, const int sw, const int sh, int dx, int dy, const int dw, const int dh)
{
	#define getPixelAddr32(f,x,y)	(f->pixels+(((y)*(const int)f->pitch)+((x)<<2)))

	int *sp, *dp;
	int y2;
	const double scaley = (double)dh / (double)sh;
	const double scalex = (double)dw / (double)sw;
	if (dx < 0) dx = 0;
	if (dy < 0) dy = 0;


	int xlookup[dw];
	for (int i = 0; i < dw; i++)
		xlookup[i] = i/scalex;

	for (int y = dy; y < dy+dh; y++){
		y2 = (y-dy)/scaley;
		sp = (int*)getPixelAddr32(src, 0, y2);
		dp = (int*)getPixelAddr32(des, dx, y);

		for (int x = 0; x < dw; x++)
			dp[x] = sp[xlookup[x]];
	}
}

static inline void copyVideo (TVLCPLAYER *vp, TVLCCONFIG *vlc, TFRAME *src, TFRAME *des, int ratio)
{

#define CW(a) ((double)(dh)*((double)a))
#define CH(a) ((double)(dw)/((double)a))
#define CX(a) ((double)((dw)-CW((a)))/(double)2.0)
#define CY(a) ((double)((dh)-CH((a)))/(double)2.0)

#define copy(a)													\
	if (CY((a)) < 0.0)											\
		copyImage(src, des, dw, dh, CX((a)), 0, CW((a)), dh);	\
	else														\
		copyImage(src, des, dw, dh, 0, CY((a)), dw, CH((a)))


	const int dw = des->width;
	const int dh = des->height;


	switch (ratio){
	  case BTN_CFG_AR_AUTO:{
		int w, h;
		_imageBestFit(dw, dh, vlc->videoWidth, vlc->videoHeight, &w, &h);

		if (w < dw-2 || h < dh-2){
			int x = 0, y = 0;
			if (w < dw-2) x = (dw-w)/2.0;
			if (h < dh-2) y = (dh-h)/2.0;

			memset(des->pixels, 0, des->frameSize);
			copyImage(src, des, dw, dh, x, y, w, h);
			break;
		}else{
			my_memcpy((uint32_t*)des->pixels, (uint32_t*)src->pixels, des->frameSize);
			break;
		}
	  }
	  case BTN_CFG_AR_CUSTOM:
	  	if (vp->ctx.aspect.clean)
	  		memset(des->pixels, 0, des->frameSize);

	  	if (vp->ctx.aspect.ratio > 0.1){
	  		copy(vp->ctx.aspect.ratio);
	  	}else{
	  		int width = vp->ctx.aspect.width;
	  		if (width > dw) width = dw;
	  		int x = vp->ctx.aspect.x;
	  		if (x+width > dw) x = 0;

	  		int height = vp->ctx.aspect.height;
	  		if (height > dh) height = dh;
	  		int y = vp->ctx.aspect.y;
	  		if (y+height > dh) y = 0;

			if (dw == width && dh == height && !x && !y){
				my_memcpy((uint32_t*)des->pixels, (uint32_t*)src->pixels, des->frameSize);
			}else{
	  			copyImage(src, des, dw, dh, x, y, width, height);
	  		}
	  	}
		break;
	  case BTN_CFG_AR_177:
		memset(des->pixels, 0, des->frameSize);
	  	copy(1.77777);
	  	break;
	  case BTN_CFG_AR_125:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.25);
		break;
	  case BTN_CFG_AR_133:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.333);
		break;
	  case BTN_CFG_AR_122:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.222);
		break;
	  case BTN_CFG_AR_15:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.5);
		break;
	  case BTN_CFG_AR_16:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.6);
		break;
	  case BTN_CFG_AR_167:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.6667);
		break;
	  case BTN_CFG_AR_143:
	  	memset(des->pixels, 0, des->frameSize);
		copy(1.43);
		break;
	  case BTN_CFG_AR_185:
	  	//printf("%i %i, %.1f %.1f :%f %f %f\n",dw, dh, CW(1.85), CH(1.85), CY(1.85), (dw/dh) * (CW(1.85)/CH(1.85)),(dw/dh) / (CW(1.85)/CH(1.85)) );
	  	memset(des->pixels, 0, des->frameSize);
	  	copy(1.85);
		break;
	  case BTN_CFG_AR_220:
	  	memset(des->pixels, 0, des->frameSize);
		copy(2.20);
		break;
	  case BTN_CFG_AR_233:
	  	memset(des->pixels, 0, des->frameSize);
		copy(2.33);
		break;
	  case BTN_CFG_AR_240:
	  	memset(des->pixels, 0, des->frameSize);
		copy(2.40);
		break;
	  case BTN_CFG_AR_155:
		copyAreaScaled(src, des, 0, (dh/100.0)*7.2, dw, (dh/100.0)*85.8, 0, 0, dw, dh);
		break;
	};
}

static void applyVideoTransformations (TFRAME *frame, TFRAME *working, TVLCCONFIG *vlc)
{

	if (vlc->swapColourBits){
		uint32_t *restrict pixels = (uint32_t*)lGetPixelAddress(frame, 0, 0);
		const int pxs = frame->frameSize>>2;
		for (int i = 0; i < pxs; i++)
			pixels[i] = ((pixels[i]&RGB_32_RED)>>16) | (pixels[i]&RGB_32_GREEN) | ((pixels[i]&RGB_32_BLUE)<<16);
	}

	if (vlc->pixelize)
		transPixelize(frame, vlc->pixelize);

	if (vlc->scaleFactor < 0.999){
		const int x = (frame->width-(frame->width*vlc->scaleFactor))/2;
		const int y = (frame->height-(frame->height*vlc->scaleFactor))/2;

		transScale(frame, working, frame->width*vlc->scaleFactor, frame->height*vlc->scaleFactor, x, y, vlc->scaleOp|SCALE_CLEANDES);
		if (vlc->rotateAngle){
			memset(frame->pixels, 0, frame->frameSize);
			transRotate(working, frame, vlc->rotateAngle, vlc->rotateOp);

		}else{
			my_memcpy(frame->pixels, working->pixels, frame->frameSize);
		}
	}else if (vlc->scaleFactor > 1.001){
		if (vlc->rotateAngle){
			memset(working->pixels, 0, working->frameSize);

			transRotate(frame, working, vlc->rotateAngle, vlc->rotateOp);
			transScale(working, frame, frame->width*vlc->scaleFactor, frame->height*vlc->scaleFactor, 0, 0, vlc->scaleOp|SCALE_CLEANDES);
		}else{
			transScale(frame, working, frame->width*vlc->scaleFactor, frame->height*vlc->scaleFactor, 0, 0, vlc->scaleOp);
			my_memcpy(frame->pixels, working->pixels, frame->frameSize);
		}
	}else{
		if (vlc->rotateAngle){
			memset(working->pixels, 0, working->frameSize);

			transRotate(frame, working, vlc->rotateAngle, vlc->rotateOp);
			my_memcpy(frame->pixels, working->pixels, frame->frameSize);
		}
	}

	if (vlc->blurRadius)
		transBlur(frame, vlc->blurRadius);
}

static int drawUnderlayMeta (TVLCPLAYER *vp, TFRAME *frame)
{
	/*const int noneState = page2RenderGetState(vp->pages, PAGE_NONE);
	const int mediaState = page2RenderGetState(vp->pages, PAGE_MEDIASTATS);
	const int metaState = page2RenderGetState(vp->pages, PAGE_META);
	printf("drawUnderlayMeta %i %i %i, %i %i\n", noneState, metaState, mediaState, pageGet(vp), pageInputGetTop(vp->pages));
	*/
	
	if (pageInputGetTop(vp->pages) == PAGE_NONE){
		TVIDEOOVERLAY *playctrl = pageGetPtr(vp, PAGE_OVERLAY);
		if (!playctrl->marquee->ready){
			PLAYLISTCACHE *plc = getQueuedPlaylist(vp);
			if (!plc) plc = getDisplayPlaylist(vp);

	  		if (plc){
	  			TMETA *meta = pageGetPtr(vp, PAGE_META);
				TMETADESC desc;
				metaCopyDesc(&desc, meta);

	  			desc.x = 6; desc.y = 2;
	  			desc.w = frame->width - (desc.x*4);
	  			desc.h = frame->height - (desc.y*2);
	  			desc.trackPosition = plc->pr->playingItem;
	  			desc.textHeight = 0;
	  			desc.uid = playlistManagerGetPlaylistUID(vp->plm, plc);
	  			
	  			//printf("metaRender\n");
				metaRender(vp, frame, meta, &desc, META_FONT, 0);
				return 1;
			}
		}
	}
	return 0;
}

static void renderEditbox (TVLCPLAYER *vp, TFRAME *frame, int x, int y, const int width, const int height_unused)
{
	const unsigned int *col = swatchGetPage(vp, PAGE_OVERLAY);
	lSetForegroundColour(frame->hw, col[SWH_OVR_EBOXTEXT]);
	lSetBackgroundColour(frame->hw, col[SWH_OVR_EBOXTEXTBK]);

	lSetCharacterEncoding(frame->hw, CMT_UTF16);
	PLAYLISTCACHE *plcD = getDisplayPlaylist(vp);
	TSPL *spl = pageGetPtr(vp, PAGE_PLY_SHELF);

	if (vp->input.iOffset > vp->input.caretPos-1)
		vp->input.iOffset = vp->input.caretPos;
	addCaret(&vp->input, vp->input.workingBuffer, vp->input.caretBuffer, EDITBOXIN_INPUTBUFFERLEN-1);
	y = drawEditBox(&vp->input, frame, x, y, width, vp->input.caretBuffer, &vp->input.iOffset);

	lSetCharacterEncoding(frame->hw, CMT_UTF8);
	x += 24+drawStrInt(frame, x+8, y-=16, "#", spl->from+1, col[SWH_OVR_EBOXTEXTBK]);
	x += 24+drawStrInt(frame, x, y, "", playlistGetTotal(plcD), col[SWH_OVR_EBOXTEXTBK]);
	drawStrInt(frame, x, y, "", playlistManagerGetTotal(vp->plm), col[SWH_OVR_EBOXTEXTBK]);
	//drawStrInt(frame, frame->width-100, y, "", artworkGetTotal(vp->tagc), col[SWH_OVR_EBOXTEXTBK]);
}

static inline void drawCursor (TVLCPLAYER *vp, TFRAME *frame, TGUI *gui)
{
	TFRAME *cur = imageManagerImageAcquire(vp->im, vp->gui.image[IMGC_POINTER]);
	//if (cur){
		int x = (gui->cursor.dx + MOFFSETX) - cur->width;		// right pointing for left handed
		//int x = gui->cursor.dx - MOFFSETX;					// left pointing for right handed
		drawImage(cur, frame, x, gui->cursor.dy-MOFFSETY, cur->width-1, cur->height-1);
		imageManagerImageRelease(vp->im, vp->gui.image[IMGC_POINTER]);
	//}
}

void video_composeFrame (TVLCPLAYER *vp, TFRAME *frame)
{
	if (getPlayState(vp) && (getPlaybackMode(vp) == PLAYBACKMODE_VIDEO /*|| (getPlaybackMode(vp) == PLAYBACKMODE_AUDIO && vp->gui.visual)*/)){
		copyVideo(vp, vp->vlc, vp->ctx.working, frame, vp->ctx.aspect.preset);
		applyVideoTransformations(frame, vp->ctx.workingTransform, vp->vlc);

	}else{
		if (getIdle(vp)){
			memset(frame->pixels, 0, frame->frameSize);

		}else if (vp->gui.skin.bg){
			TFRAME *base = vp->gui.skin.bg;
			const size_t blen = MIN(base->frameSize, frame->frameSize);
			TVIDEOOVERLAY *playctrl = pageGetPtr(vp, PAGE_OVERLAY);

			if (!playctrl->marquee->ready && !vp->gui.marquee->ready && !vp->gui.picQueue->total
			  && !page2RenderGetState(vp->pages, PAGE_EXIT) && !page2RenderGetState(vp->pages, PAGE_MEDIASTATS)
			  && (pageRenderGetTop(vp->pages) == PAGE_VIDEO) && !mHookGetState() && !kHookGetState()){
			  	
			  	const int playState = getPlayState(vp);
				//int draw = (vp->gui.drawMetaTrackbar && ((playState == 1 && vp->gui.frameCt&0x01) || ((playState == 2 || !playState) && vp->gui.frameCt < 3)));
				//draw |= (!vp->gui.drawMetaTrackbar && (vp->gui.frameCt < 3));
				//draw |= (playState && vp->gui.drawVisuals);

				const int draw =
					(vp->gui.frameCt < 3) ||
					(vp->gui.drawMetaTrackbar && (playState == 1 && vp->gui.frameCt&0x01));

				if (draw){
					my_memcpy((uint32_t*)frame->pixels, (uint32_t*)base->pixels, blen);
					drawUnderlayMeta(vp, frame);
				}
			}else{
				my_memcpy((uint32_t*)frame->pixels, (uint32_t*)base->pixels, blen);
				//memset(frame->pixels, 0, frame->frameSize);
				drawUnderlayMeta(vp, frame);
			}
		}
	}

	page2Render(vp->pages, frame, PAGE_TEXTOVERLAY);
	

#if (1 && ENABLE_BASS)
	if (!getIdle(vp) && getPlayState(vp))
		bass_render(&vp->bass, frame, ((frame->width - vp->bass.vwidth)/2)-27, frame->height-10);
#endif

	marqueeDraw(vp, frame, vp->gui.marquee, 4, 2);
	if (picQueueGetTotal(vp->gui.picQueue))
		picQueueRender(vp->gui.picQueue, frame, getTickCount(), 2, 2);

	if (kHookGetState() && !page2RenderGetState(vp->pages, PAGE_TETRIS) && !vp->gui.snapshot.save)
		renderEditbox(vp, frame, 3, frame->height-3, frame->width-6, 0);

	TGUIINPUT *cursor = &vp->gui.cursor;
	if (cursor->dragRectIsEnabled && cursor->LBState == 2){
		int x1 = cursor->dragRect0.x;
		int y1 = cursor->dragRect0.y;
		int x2 = cursor->dragRect1.x;
		int y2 = cursor->dragRect1.y;
		lDrawRectangleFilled(frame, x1, y1, x2, y2, 60<<24|COL_PURPLE_GLOW);
		lDrawRectangle(frame, x1-1, y1-1, x2+1, y2+1, 180<<24|COL_PURPLE_GLOW);
	}


#if MOUSEHOOKCAP
	if (vp->gui.cursor.isHooked && cursorGetState(&vp->gui.cursor)){
#if DRAWCURSORCROSS
		//drawCursor(vp, frame, &vp->gui);
		lDrawLine(frame, vp->gui.cursor.dx, 0, vp->gui.cursor.dx, frame->height-1, 0xFF<<24 | COL_RED);
		lDrawLine(frame, 0, vp->gui.cursor.dy, frame->width-1, vp->gui.cursor.dy,  0xFF<<24 | COL_RED);
#else
		drawCursor(vp, frame, &vp->gui);
#endif
	}
#endif

	if (vp->gui.snapshot.save){
		if (vp->gui.snapshot.save == 2){
			vp->gui.snapshot.save = 0;
			video_saveSnapshot(vp, frame, vp->gui.snapshot.filename, vp->gui.snapshot.annouce);
			vp->gui.snapshot.annouce = 0;
		}else if (vp->gui.snapshot.save == 1){
			vp->gui.snapshot.save = 2;
			renderSignalUpdate(vp);
		}
	}
}

static void initCS (TVLCPLAYER *vp)
{
	vp->gui.hUpdateEvent = CreateEvent(NULL, 0, 0, NULL);
	vp->ctx.hEvent = CreateEvent(NULL, 0, 0, NULL);
	vp->ctx.hVideoLock = lockCreate("videoLock");
	vp->ctx.hVideoCBLock = lockCreate("vlcEventsLock");
}

static void deleteCS (TVLCPLAYER *vp)
{
	CloseHandle(vp->gui.hUpdateEvent);
	CloseHandle(vp->ctx.hEvent);
	
	lockClose(vp->ctx.hVideoLock);
	lockClose(vp->ctx.hVideoCBLock);
}

static int createVideoBuffers (TVLCPLAYER *vp)
{
	// we read from this, don't write. read only upon vlc signals so
 	vp->ctx.vmem = lNewFrame(vp->ml->hw, vp->vlc->width, vp->vlc->height, vp->vlc->bpp);

 	// copy of the above with write permitted
 	vp->ctx.working = lNewFrame(vp->ml->hw, vp->vlc->width, vp->vlc->height, vp->vlc->bpp);
	vp->ctx.workingTransform = lNewFrame(vp->ml->hw, vp->vlc->width, vp->vlc->height, vp->vlc->bpp);
   	vp->ctx.bufferSize = vp->ctx.working->frameSize;
   	vp->ctx.pixels = lGetPixelAddress(vp->ctx.vmem, 0, 0);
 	vp->ctx.pixelBuffer = lGetPixelAddress(vp->ctx.working, 0, 0);

   	return (vp->ctx.pixels && vp->ctx.pixelBuffer);
}

static inline void freeVideoBuffers (TVLCPLAYER *vp)
{
	lDeleteFrame(vp->ctx.vmem);
	lDeleteFrame(vp->ctx.working);
	lDeleteFrame(vp->ctx.workingTransform);
	
	vp->ctx.pixels = NULL;
	vp->ctx.pixelBuffer = NULL;
	vp->ctx.bufferSize = 0;
}

static int page_videoStartup (TPAGEVIDEO *video, TVLCPLAYER *vp, const int width, const int height)
{
	
	// set a few defaults
	TGUIINPUT *cursor = &vp->gui.cursor;
	cursor->isHooked = 0;
	cursor->slideHoverEnabled = 0;
	cursor->draw = 1;
	cursor->x = 0;
	cursor->y = 0;
	cursor->dx = 0;
	cursor->dy = 0;
	cursor->LBState = 0;
	cursor->MBState = 0;
	cursor->RBState = 0;
	cursor->dragRect0.x = 0;
	cursor->dragRect0.y = 0;
	cursor->dragRect1.x = 0;
	cursor->dragRect1.y = 0;
	cursor->dragRectIsEnabled = 0;

	vp->gui.padctrlMode = BTN_CFG_PADCTRL_ON;
	vp->gui.hotkeys.cursor = 'A';
	vp->gui.hotkeys.console = 'L';
	vp->gui.idleTime = IDLETIME;
	vp->gui.idleFPS = UPDATERATE_IDLE;
	vp->gui.targetRate = UPDATERATE_ALIVE;
	vp->gui.mOvrTime = MCTRLOVERLAYPERIOD;
	vp->gui.artSearchDepth = 4;
	vp->gui.artMaxWidth = width/1.500;
	vp->gui.artMaxHeight = height/1.083;
	vp->gui.runCount = 0;
	vp->gui.lastTrack = -1;
	vp->gui.page_gl = PAGE_NONE;
	vp->vlc->volume = 50;

	vp->lastRenderTime = 0.0;
	return 1;
}


static int page_videoInitalize (TPAGEVIDEO *video, TVLCPLAYER *vp, const int width, const int height)
{
	setPlaybackMode(vp, PLAYBACKMODE_AUDIO);

	vp->gui.image[IMGC_SHADOW_BLK] = imageManagerImageAdd(vp->im, L"common/artshadow_blk.png");
	vp->gui.image[IMGC_SHADOW_BLU] = imageManagerImageAdd(vp->im, L"common/artshadow_blu.png");
	vp->gui.image[IMGC_SHADOW_GRN] = imageManagerImageAdd(vp->im, L"common/artshadow_grn.png");
	
	vp->gui.image[IMGC_NOART_SHELF_SELECTED] = imageManagerImageAdd(vp->im, L"shelf/noartselected.png");
	vp->gui.image[IMGC_NOART_SHELF_PLAYING] = imageManagerImageAdd(vp->im, L"shelf/noartplaying.png");
	vp->gui.image[IMGC_POINTER] = imageManagerImageAdd(vp->im, L"common/cursor.png");

	createVideoBuffers(vp);
	initCS(vp);
	wcscpy(vp->gui.snapshot.filename, SNAPSHOTFILE);
	vp->gui.marquee = marqueeNew(height/21.0, MARQUEE_LEFT, DMSG_FONT);
	vp->gui.picQueue = picQueueNew(vp->im, 16, 16);
	return 1;
}

static int page_videoShutdown (TPAGEVIDEO *video, TVLCPLAYER *vp)
{

	//lDeleteFrame(vp->gui.cursorUnderlay);
	deleteCS(vp);
	freeVideoBuffers(vp);
	picQueueDelete(vp->gui.picQueue);
	marqueeDelete(vp->gui.marquee);

	lockClose(vp->gui.hRenderLock);
	vp->gui.hRenderLock = NULL;
	lockClose(vp->gui.hLoadLock);
	vp->gui.hLoadLock = NULL;

	return 1;
}

static int page_videoInput (TPAGEVIDEO *video, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
#if 1
	if (0 && vp->gui.cursor.isHooked && getPlayState(vp) && !flags){
		vlc_cursorSet(vp->vlc->mp, vp->gui.cursor.dx, vp->gui.cursor.dy);
		vlc_cursorClicked(vp->vlc->mp, vp->gui.cursor.dx, vp->gui.cursor.dy);
	}else{
	
		switch(msg){
		  case PAGE_IN_TOUCH_DOWN:
		  case PAGE_IN_TOUCH_SLIDE:
			overlaySetOverlay(vp);
			break;
			
		  case PAGE_IN_WHEEL_FORWARD:
	  		setVolumeDisplay(vp, setVolume(vp, getVolume(vp, VOLUME_APP)+3, VOLUME_APP));
			break;
			
		  case PAGE_IN_WHEEL_BACK:
	  		setVolumeDisplay(vp, setVolume(vp, getVolume(vp, VOLUME_APP)-3, VOLUME_APP));
			break;
		}
	}
#endif
	return 1;
}

static int page_videoRender (TPAGEVIDEO *video, TVLCPLAYER *vp,  TFRAME *frame)
{
#if 1
	if (pageRenderGetTop(vp->pages) != PAGE_META){
		TVIDEOOVERLAY *playctrl = pageGetPtr(vp, PAGE_OVERLAY);
		marqueeDraw(vp, frame, playctrl->marquee, 2, 2);

		if (0 && vp->gui.cursor.isHooked && getPlayState(vp))
			vlc_cursorSet(vp->vlc->mp, vp->gui.cursor.dx, vp->gui.cursor.dy);
	}
#endif
	return 1;
}

void page_videoRenderStart (TPAGEVIDEO *video, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	if (playlistManagerGetTotal(vp->plm) <= 1){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylist(vp->plm, 0);
		if (!plc || playlistGetCount(plc, PLAYLIST_OBJTYPE_TRACK) < 1)
			page2Set(vp->pages, PAGE_HOME, 0);
	}
	vp->gui.frameCt = 0;
}

static void onDriveNotification (TPAGEVIDEO *video, const int event, const char drive, const int isUsb, const char *drivePath)
{
	wchar_t imgTop[MAX_PATH+1];
	__mingw_snwprintf(imgTop, MAX_PATH, L"common/letters/%lc.png", drive);
	wchar_t *imgBtm = NULL;
	
	switch (event){
	case PAGE_MSG_DRIVE_ARRIVE:
		if (isUsb)
			imgBtm = L"common/drives/driveadded_usb.png";
		else
			imgBtm = L"common/drives/driveadded.png";
		break; 

	case PAGE_MSG_DRIVE_DEPART:
		if (isUsb)		// .. or was
			imgBtm = L"common/drives/driveremoved_usb.png";
		else
			imgBtm = L"common/drives/driveremoved.png";
		break;
		              
	case PAGE_MSG_MEDIA_ARRIVE:
		if (isUsb)
			imgBtm = L"common/drives/mediaadded_usb.png";
		else
			imgBtm = L"common/drives/mediaadded.png";
		break;

	case PAGE_MSG_MEDIA_DEPART:
		if (isUsb)
			imgBtm = L"common/drives/mediaremoved_usb.png";
		else
			imgBtm = L"common/drives/mediaremoved.png";
		break;
	}
	
    if (imgBtm){
    	TVLCPLAYER *vp = video->com->vp;
    	wakeup(vp);
		picQueueAdd(vp->gui.picQueue, imgBtm, imgTop, getTickCount()+10000);
		timerSet(vp, TIMER_EXPPAN_REBUILD, 1);
		renderSignalUpdate(vp);
	}
}


int page_videoCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TPAGEVIDEO *video = (TPAGEVIDEO*)pageStruct;

	if (msg == PAGE_CTL_RENDER){
		return page_videoRender(video, video->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){
		page_videoRenderStart(video, video->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);

	}else if (msg == PAGE_CTL_RENDER_END){
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_videoInput(video, video->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_videoStartup(video, video->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_videoInitalize(video, video->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_videoShutdown(video, video->com->vp);
	
	}else if (msg == PAGE_MSG_DRIVE_ARRIVE || msg == PAGE_MSG_MEDIA_ARRIVE || msg == PAGE_MSG_DRIVE_DEPART || msg == PAGE_MSG_MEDIA_DEPART){
		onDriveNotification(video, msg, dataInt1%0xFF, dataInt2, dataPtr);
		return 0;
	
	}else if (msg == PAGE_MSG_DEVICE_ARRIVE || msg == PAGE_MSG_DEVICE_DEPART){
		int vid = dataInt1;
		int pid = dataInt2;

		if (vid == SBUI_ISV_VID && pid == SBUI_ISV_PID){
			if (msg == PAGE_MSG_DEVICE_ARRIVE)
				timerSet(video->com->vp, TIMER_SBUI_CONNECTED, 1000);
			else
				timerSet(video->com->vp, TIMER_SBUI_DISCONNECTED, 1000);
		}
	}
	
	return 1;
}
