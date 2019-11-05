
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


static inline int picQueueAddImage (TPICQUEUE *pq, const wchar_t *imgBtm, const wchar_t *imgTop, const uint64_t timeEnd)
{
	int success = 0;

	
	// find empty pic
	TPICTURE *picture = NULL;
	for (int i = 0; i < PICQUEUE_MAX; i++){
		if (!pq->queue[i].status){
			picture = &pq->queue[i];
			break;
		}
	}

	// if no empty slots then find and overwrite oldest added
	if (!picture){
		uint64_t time0 = getTickCount()+60000;
		
		for (int i = 0; i < PICQUEUE_MAX; i++){
			if (pq->queue[i].status){
				if (pq->queue[i].timeAdded < time0){
					time0 = pq->queue[i].timeAdded;
					picture = &pq->queue[i];
				}
			}
		}
		if (picture) pq->total--;
	}
	
	if (picture){
		if (imgBtm){
			picture->imgBtmId = imageManagerImageAdd(pq->im, imgBtm);
			if (picture->imgBtmId)
				success = imgLoaderAddImage(pq->imgLoader, picture->imgBtmId);
		}
	
		if (imgTop){
			picture->imgTopId = imageManagerImageAdd(pq->im, imgTop);
			if (picture->imgTopId)
				success |= imgLoaderAddImage(pq->imgLoader, picture->imgTopId);
		}
		
		if (success){
			picture->timeAdded = getTickCount();
			picture->timeEnd = timeEnd;
			picture->pos.x1 = 0;
			picture->pos.y1 = 0;
			picture->pos.x2 = 0;
			picture->pos.y2 = 0;
			picture->status = 1;
			pq->total++;
		}
	}
	
	
	return success;
}

int picQueueAdd (TPICQUEUE *pq, const wchar_t *imgBtm, const wchar_t *imgTop, const uint64_t timeEnd)
{
	int ret = 0;
	if (lockWait(pq->hLock, INFINITE)){
		ret = picQueueAddImage(pq, imgBtm, imgTop, timeEnd);
		lockRelease(pq->hLock);
	}
	return ret;
}

int picQueueGetTotal (TPICQUEUE *pq)
{
	return pq->total;
}

static inline int picQueueRenderPicture (TPICQUEUE *pq, TPICTURE *picture, TFRAME *frame, int *x, int *y)
{
	int maxWidth = 0;
	int maxHeight = 0;
	int drawCt = 0;
	
	if (picture->imgBtmId){
		TFRAME *img = imageManagerImageAcquire(pq->im, picture->imgBtmId);
		if (img){
			drawCt++;
			drawImage(img, frame, *x, *y, img->width-1, img->height-1);
			//printf("picQueueRenderPicture btm, %i, %i %i\n", picture->imgBtmId, *x, *y);
			
			maxWidth = img->width;
			maxHeight = img->height;
			imageManagerImageRelease(pq->im, picture->imgBtmId);
		}
	}

	if (picture->imgTopId){
		TFRAME *img = imageManagerImageAcquire(pq->im, picture->imgTopId);
		if (img){
			drawCt++;
			int xc = 0, yc = 0;
			if (picture->imgBtmId){
				xc = abs(maxWidth - img->width)/2;
				yc = abs(maxHeight - img->height)/2;
			}
			
			drawImage(img, frame, *x + xc, *y + yc, img->width-1, img->height-1);
			//printf("picQueueRenderPicture top, %i, %i %i\n", picture->imgTopId, *x, *y);
			
			if (img->width > maxWidth) maxWidth = img->width;
			if (img->height > maxHeight) maxHeight = img->height;
			imageManagerImageRelease(pq->im, picture->imgTopId);
		}
	}
	
	if (drawCt){
		picture->pos.x1 = *x;
		picture->pos.y1 = *y;
		*x += maxWidth-1;
		*y += maxHeight-1;
		picture->pos.x2 = *x;
		picture->pos.y2 = *y;
		
	}
	return drawCt;
}

static inline int picQueuePictureGetMetrics (TPICQUEUE *pq, TPICTURE *picture, int *width, int *height)
{
	int maxWidth = 0;
	int maxHeight = 0;
	int ct = 0;
	
	if (picture->imgBtmId){
		ct = imageManagerImageGetMetrics(pq->im, picture->imgBtmId, &maxWidth, &maxHeight);
		*width = maxWidth;
		*height = maxHeight;
	}
	if (picture->imgTopId)
		ct |= imageManagerImageGetMetrics(pq->im, picture->imgTopId, width, height);
	
	if (maxWidth > *width) *width = maxWidth;
	if (maxHeight > *height) *height = maxHeight;
	
	return ct;
}

int picQueueRender (TPICQUEUE *pq, TFRAME *frame, const uint64_t time0, const int colX, const int rowY)
{
	int ct = 0;
	
	if (lockWait(pq->hLock, INFINITE)){
		if (pq->total){
			int x = colX, y = rowY;
			int lastRowY = y;

			TPICTURE *picOrder[PICQUEUE_MAX];
			
			for (int i = 0; i < PICQUEUE_MAX; i++)
				picOrder[i] = &pq->queue[i];

			
			for (int i = 0; i < PICQUEUE_MAX; i++){
				for (int j = 0; j < PICQUEUE_MAX-1; j++){
					if (picOrder[j]->timeAdded > picOrder[j+1]->timeAdded){
						TPICTURE *picture = picOrder[j];
						picOrder[j] = picOrder[j+1];
						picOrder[j+1] = picture;

					}
				}
			}
			
			for (int i = 0; i < PICQUEUE_MAX; i++){
				TPICTURE *picture = picOrder[i];
				if (picture->status){
					if (picture->timeEnd >= time0){
						int width = 0, height = 0;
						if (picQueuePictureGetMetrics(pq, picture, &width, &height)){
							if (x + width > frame->width){
								x = colX;
								y = lastRowY + pq->imageSpaceRow;
							}

							int row = y;
							if (picQueueRenderPicture(pq, picture, frame, &x, &row)){
								if (row > lastRowY) lastRowY = row;
								x += pq->imageSpaceCol;
							}
						}
					}else{
						picture->status = 0;
						pq->total--;
					}
				}
			}
		}
		lockRelease(pq->hLock);
	}
	return ct;
}

TPICQUEUE *picQueueNew (TIMAGEMANAGER *im, const int spaceCol, const int spaceRow)
{
	TPICQUEUE *pq = my_calloc(1, sizeof(TPICQUEUE));
	if (pq){
		pq->hLock = lockCreate("picQueueNew");
		pq->imageSpaceCol = spaceCol;
		pq->imageSpaceRow = spaceRow;
		pq->im = im;
		pq->imgLoader = imgLoaderNew(pq->im, 2);
	}
	return pq;
}

void picQueueDelete (TPICQUEUE *pq)
{
	if (pq){
		lockWait(pq->hLock, INFINITE);
		imgLoaderShutdown(pq->imgLoader);
		lockClose(pq->hLock);

		my_free(pq);
	}
}

