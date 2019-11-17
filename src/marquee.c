
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



void marqueeDelete (TMARQUEE *marquee)
{
	if (marquee){
		lockWait(marquee->hLock, INFINITE);
		lockClose(marquee->hLock);
		//my_free(marquee->entry);
		my_free(marquee);
	}
}

TMARQUEE *marqueeNew (const int tLines, const unsigned int flags, const int font)
{
	TMARQUEE *marquee = my_calloc(1, sizeof(TMARQUEE));
	if (marquee){
	//	marquee->entry = (TMARQUEELINE*)my_calloc(tLines, sizeof(TMARQUEELINE));
	//	if (marquee->entry){
			marquee->total = tLines;
			marquee->font = font;
			
			marquee->hLock = lockCreate("marqueeNew");
			marquee->flags = flags;
	//	}else{
	//		my_free(marquee);
	//		marquee = NULL;
	//	}
	}
	return marquee;
}

void marqueeStringFlags (TVLCPLAYER *vp, TMARQUEE *mq, const int line, const int flags)
{
	if (lockWait(mq->hLock, INFINITE)){
		if (line < mq->total)
			mq->entry[line].flags = flags;
		lockRelease(mq->hLock);
	}
}

void marqueeClear (TMARQUEE *mq)
{
	if (lockWait(mq->hLock, INFINITE)){
		mq->ready = 0;
		for (int i = 0; i < mq->total; i++)
			mq->entry[i].time = 0;
		lockRelease(mq->hLock);
	}
}

int marqueeAdd (TVLCPLAYER *vp, TMARQUEE *mq, const char *str, const unsigned int timeout)
{
	if (lockWait(mq->hLock, INFINITE)){
		my_memcpy(&mq->entry[0], &mq->entry[1], sizeof(TMARQUEELINE) * (mq->total-1));

		strncpy(mq->entry[mq->total-1].line, str, MAX_PATH_UTF8);
		mq->entry[mq->total-1].time = timeout;
		mq->ready = 1;
		lockRelease(mq->hLock);
		return mq->total-1;
	}
	return -1;
}

int marqueeDraw (TVLCPLAYER *vp, TFRAME *frame, TMARQUEE *_mq, int x, int y)
{
	if (!_mq || !_mq->ready) return 0;

	TMARQUEE *mq = my_calloc(1, sizeof(TMARQUEE));
	if (!mq) return 0;
	
	if (lockWait(_mq->hLock, INFINITE)){
		my_memcpy(mq, _mq, sizeof(TMARQUEE));
		_mq->ready = 0;
		vp->gui.frameCt = 0;
		
		for (int i = 0; i < _mq->total; i++){
			if (_mq->entry[i].time > vp->fTime){
				_mq->ready = 1;
				break;
			}
		}
		lockRelease(_mq->hLock);
	}else{
		my_free(mq);
		return 0;
	}

	const unsigned int *col = swatchGetPage(vp, PAGE_OVERLAY);
	setForegroundColourIdx(vp, PAGE_OVERLAY, SWH_OVR_MARQTEXT);
	setBackgroundColourIdx(vp, PAGE_OVERLAY, SWH_OVR_MARQTEXTBK);
	lSetCharacterEncoding(vp->ml->hw, CMT_UTF8);
	lSetRenderEffect(vp->ml->hw, LTR_SHADOW);
	lSetFilterAttribute(vp->ml->hw, LTR_SHADOW, 0, LTRA_SHADOW_S|LTRA_SHADOW_E | LTRA_SHADOW_S4 | LTRA_SHADOW_OS(0) | LTRA_SHADOW_TR(170));

	
	const int flagsExtra = PF_CLIPDRAW|PF_IGNOREFORMATTING;
	int colour = 0;//COL_BLUE_SEA_TINT;
	int alpha = 1.0 * 1000.0;			// alpha range of 0 to 1000
	int radius = 3;						// 0 - 255
	int blur = LTR_BLUR4;
	lRenderEffectReset(frame->hw, mq->font, blur);	// reset previous blur4 state
	lSetRenderEffect(frame->hw, blur);
	lSetFilterAttribute(frame->hw, blur, LTRA_BLUR_COLOUR, colour);
	lSetFilterAttribute(frame->hw, blur, LTRA_BLUR_RADIUS, radius);
	lSetFilterAttribute(frame->hw, blur, LTRA_BLUR_SETTOP, 0);
	lSetFilterAttribute(frame->hw, blur, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, blur, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, blur, LTRA_BLUR_ALPHA, alpha);

	TLPRINTR rect = {0,0,frame->width-1,frame->height-1,0,0,0,0};
	
	for (int i = 0; i < mq->total; i++){
		if (mq->entry[i].time > vp->fTime){
			if (mq->flags&MARQUEE_CENTER){
				lSetRenderEffect(vp->ml->hw, LTR_SHADOW);
				printSingleLineShadow(frame, mq->font, x, y, col[SWH_OVR_MARQTEXT], col[SWH_OVR_MARQTEXTBK], mq->entry[i].line);
				//lPrintEx(frame, &rect,  mq->font, PF_MIDDLEJUSTIFY|PF_CLIPWRAP|PF_IGNOREFORMATTING, LPRT_OR, mq->entry[i].line);
				rect.ey += 25;
			}else{
				rect.sx = x;
				rect.sy = y;
				rect.ey = rect.sy;
				lSetRenderEffect(frame->hw, blur);
				
				lPrintEx(frame, &rect,  mq->font, flagsExtra|mq->entry[i].flags, LPRT_OR, mq->entry[i].line);
				rect.sx = x; rect.sy = y;
				lPrintEx(frame, &rect,  mq->font, flagsExtra|mq->entry[i].flags, LPRT_OR, mq->entry[i].line);
				
				rect.sx = x; rect.sy = y;
				rect.ey = rect.sy;
				lSetRenderEffect(vp->ml->hw, LTR_DEFAULT);
				lPrintEx(frame, &rect,  mq->font, flagsExtra|mq->entry[i].flags, LPRT_OR, mq->entry[i].line);
				
				rect.ey -= 3;
			}
			y = rect.ey;
		}
	}
	
	lSetRenderEffect(vp->ml->hw, LTR_DEFAULT);
	my_free(mq);
	return (y > 2);
}

