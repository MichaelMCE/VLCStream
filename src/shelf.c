
// libmylcd
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
//  GNU LIBRARY GENERAL PUBLIC LICENSE for details.




#include "common.h"


#define SHELF_ANIRATE_DEFAULT (0.017*1.5)
//static const double maxArtHeight = 442.0;
static const double expMult = 4.0;

#define isNoArt(s,i)	((s)->imgNoArtId==(i))



	
	
void shelfSetClientImageLookupCB (TSHELF *shelf, void *uptr, int (*cb) (void *ptr, const int index, int *data))
{
	shelf->uptr = uptr;
	shelf->getClientImage_cb = cb;
}

void shelfSetClientFreeImageCB (TSHELF *shelf, void (*cb) (void *fptr, const int index, const int data), void *fptr)
{
	shelf->clientFreeImage_cb = cb;
	shelf->fptr = fptr;
}

static inline void shelfFreeImage (TSHELF *shelf, /*void *ptr,*/ const int index, const int data)
{
#if 0
	if (index >= 0)
		shelf->clientFreeImage_cb(shelf->fptr, /*ptr, */index, data);
#endif
}

int shelfGetClientImage (TSHELF *shelf, const int index, int *data)
{
	const int imgId = shelf->getClientImage_cb(shelf->uptr, index, data);
	if (imgId)
		shelf->ichanged = 1;
	return imgId;
}

static inline void computeEquationParts (TSHELF *shelf)
{
	shelf->expMultiplier = sqrt(2.0 * M_PI) / shelf->sigma / shelf->rho;
	shelf->expMember = expMult * shelf->sigma * shelf->sigma;
}

static inline double computeModifierUnprotected (TSHELF *shelf, const double x)
{
	return shelf->expMultiplier * exp((-x * x) / shelf->expMember);
}
    
static inline double computeModifier (TSHELF *shelf, const double x)
{
	double result = computeModifierUnprotected(shelf, x);
	if (result > 1.0)
		result = 1.0;
	else if (result < -1.0)
		result = -1.0;
	return result;
}

static inline void setSigma (TSHELF *shelf, const double sigma)
{
	shelf->sigma = sigma;
	shelf->rho = 1.0;
	computeEquationParts(shelf);
	shelf->rho = computeModifierUnprotected(shelf, 0.0);
	computeEquationParts(shelf);
}

void shelfSetSigma (TSHELF *shelf, const double sigma)
{
	setSigma(shelf, sigma);
}

void shelfSetClientRenderCB (TSHELF *shelf, int (*renderCB)(void *ptr, const int idx, TFRAME *src, TFRAME *des, const int x, const int y, const int width, const int height, double modifier), void *uRednerPtr)
{
	shelf->clientRender_cb = renderCB;
	shelf->uRenderPtr = uRednerPtr;
}

int shelfClientRender (TSHELF *shelf, const int idx, TFRAME *src, TFRAME *des, const int x, const int y, const int width, const int height, const float modifier)
{
	return shelf->clientRender_cb(shelf->uRenderPtr, idx, src, des, x, y, width, height, modifier);
}

int shelfSetImage (TSHELF *shelf, const int shelfIndex, const int imgId, int imgSrcIndex, const int data)
{
	//if (shelf->simg[shelfIndex].img)
		//shelfFreeImage(shelf, (void*)shelf->simg[shelfIndex].img, shelf->simg[shelfIndex].imgSrcIndex);
		
	//printf("setimage NULL image\n");
	if (!imgId) imgSrcIndex = -1;

	shelf->simg[shelfIndex].imgId = imgId;
	shelf->simg[shelfIndex].modifier = 0.0;
	//shelf->simg[shelfIndex].x = 0.0;
	//shelf->simg[shelfIndex].y = 0.0;
	shelf->simg[shelfIndex].zOrder = 0.0;
	shelf->simg[shelfIndex].imgSrcIndex = imgSrcIndex;
	if (data == -1)
		shelf->simg[shelfIndex].data = 0;
	else
		shelf->simg[shelfIndex].data = data;
	return shelfIndex;
}

int shelfDrawImage (TSHELF *shelf, TFRAME *frame, const int index, const int offset, double imgPosition)
{
	
	const int idx = index + offset;
	if (idx > shelf->totalImg-1) return 0;
	if (idx < 0) return 0;

	TSHELFIMG *simg = &shelf->simg[idx]; 
	int img = simg->imgId;

	if (!img){
		if (simg->imgSrcIndex >= 0){
			int data = 0;
			
			int newImg = shelfGetClientImage(shelf, simg->imgSrcIndex, &data);
			if (newImg){
				shelfSetImage(shelf, idx, newImg, simg->imgSrcIndex, data);
			}else /*if (!newImg)*/{
				shelfSetImage(shelf, idx, 0, -1, -1);
				//printf("a %i %i %f\n", index, offset, imgPosition);
				return 0;
			}
		}
		img = simg->imgId;
		if (!img){
			//simg->imgId = shelf->imgNoArtId;
			//img = simg->imgId;
			//printf("b %i %i %f\n", index, offset, imgPosition);
			return 0;
		}
	}

	if (isNoArt(shelf, img)){
		int data = 0;
		int newImg = shelfGetClientImage(shelf, simg->imgSrcIndex, &data);
		if (newImg){
			if (!isNoArt(shelf, newImg)){
				shelfFreeImage(shelf, simg->imgSrcIndex, simg->data);
				shelfSetImage(shelf, idx, newImg, simg->imgSrcIndex, data);
				img = newImg;
			}else{
				shelfFreeImage(shelf, simg->imgSrcIndex, simg->data);
			}
		}
	}	
	
	int imgWidth = 1, imgHeight = 1;
	if (!artManagerImageGetMetrics(shelf->am, img, &imgWidth, &imgHeight)){
		//printf("shelf getmetrics failed %X %i\n", img, index);
		artManagerImageDelete(shelf->am, img);

		img = shelf->imgNoArtId;
		simg->imgId = img;
		artManagerImageGetMetrics(shelf->am, img, &imgWidth, &imgHeight);
	}

	
	// got the imageId, now render it
	double scale;
	if (!isNoArt(shelf, img))
		//scale = (double)frame->height/780.0; //272.0/maxArtHeight;
		//scale = ((double)(frame->height - 120 - 80)/(double)frame->height) * shelf->imgScaleMult;
		//scale = ((double)((frame->height - 120.0 - 80.0))/((double)frame->height*shelf->imgScaleMult));
		//scale = (double)((frame->height - 120.0 - 80.0)*shelf->imgScaleMult) / (double)frame->height;
		//scale = ((double)(frame->height - 100 - 80) / 442.0) * shelf->imgScaleMult;
		//scale = (double)((frame->height - 120.0 - 80.0)*shelf->imgScaleMult) /  442.0;
		scale = (double)(frame->height - 120.0 - 80.0) /  ((frame->height*0.92)/shelf->imgScaleMult);
	else
		scale = (1.0 / (imgHeight*2.0)) * (double)frame->height;
	
	imgWidth *= scale;
	imgHeight *= scale;
	imgPosition += (offset * shelf->spacing);

	const int height = frame->height;
	const int width = frame->width;
	const double semiWidth = width / 2.0;

	simg->modifier = computeModifier(shelf, imgPosition);
	//simg->modifier = simg->zOrder;
	int newWidth = (int)(imgWidth * simg->modifier);
	if (newWidth == 0){
		//printf("c %i %i %f\n", index, offset, imgPosition);
		return 0;
	}

	int newHeight = (int)(imgHeight * simg->modifier);
	if (newHeight == 0){
		//printf("d %i %i %f\n", index, offset, imgPosition);
		return 0;
	}

	const double img_x = (/* x + */  (width - newWidth) / 2.0) + (imgPosition * semiWidth);
	const double img_y = /* y + */ (height - newHeight / 2.0) / 3.0;
	if (img_x >= width || img_x < -newWidth){
		//printf("e %i %i %f\n", index, offset, imgPosition);
	   return 0;
	}

	TLPOINTEX *pos = &simg->pos;
	pos->x1 = img_x;
	pos->y1 = img_y;
	pos->x2 = newWidth;
	pos->y2 = newHeight;
	
	int ret = 0;
	TFRAME *srcImg = artManagerImageAcquireScaled(shelf->am, img, scale/**simg->modifier*/);
	if (srcImg){
		if (!isNoArt(shelf, img))
			srcImg->udata_int = SHELF_IMGTYPE_ARTWORK;
		else
			srcImg->udata_int = SHELF_IMGTYPE_NOART;
		
		//printf("scale %i %i %f %f\n", index, simg->imgSrcIndex, scale, simg->modifier);
		
		if (shelfClientRender(shelf, simg->imgSrcIndex, srcImg, frame, img_x, img_y, newWidth, newHeight, simg->modifier)){
			if (simg->modifier < 1.0)
				drawImageScaledOpacity(srcImg, frame, 0, 0, imgWidth, imgHeight, (int)img_x, (int)img_y, newWidth, newHeight, simg->modifier, simg->modifier*shelf->imgOpacityMult);
			else if (srcImg->udata_int == SHELF_IMGTYPE_ARTWORK)
				drawImageNoBlend(srcImg, frame, (int)img_x, (int)img_y, newWidth, newHeight);
			else
				drawImage(srcImg, frame, (int)img_x, (int)img_y, newWidth, newHeight);
			ret = 1;
		}
		artManagerImageRelease(shelf->am, img);

#if DRAWTOUCHRECTS		
		lDrawRectangle(frame, pos->x1, pos->y1, pos->x1+pos->x2, pos->y1+pos->y2, DRAWTOUCHRECTCOL);
#endif
	}
	//printf("f %i %i %f\n", index, offset, imgPosition);
	return ret;
}

void shelfFlush (TSHELF *shelf)
{
	for (int i = 0; i < shelf->totalImg; i++){
		shelfFreeImage(shelf, shelf->simg[i].imgSrcIndex, shelf->simg[i].data);
		shelf->simg[i].imgId = 0;
		shelf->simg[i].imgSrcIndex = -1;
	}
}

static inline void shelfMoveShelfLeft (TSHELF *shelf)
{
	shelfFreeImage(shelf, shelf->simg[0].imgSrcIndex, shelf->simg[0].data);
	
	for (int i = 1; i < shelf->totalImg; i++){
		shelf->simg[i-1].imgId = shelf->simg[i].imgId;
		shelf->simg[i-1].imgSrcIndex = shelf->simg[i].imgSrcIndex;
		shelf->simg[i-1].data = shelf->simg[i].data;
		shelf->simg[i-1].zOrder = shelf->simg[i].zOrder;
	}
}

static inline void shelfMoveShelfRight (TSHELF *shelf)
{
	shelfFreeImage(shelf, shelf->simg[shelf->totalImg-1].imgSrcIndex, shelf->simg[shelf->totalImg-1].data);
	
	for (int i = shelf->totalImg-1; i > 0; i--){
		shelf->simg[i].imgId = shelf->simg[i-1].imgId;
		shelf->simg[i].imgSrcIndex = shelf->simg[i-1].imgSrcIndex;
		shelf->simg[i].data = shelf->simg[i-1].data;
		shelf->simg[i].zOrder = shelf->simg[i-1].zOrder;
	}
}

static inline int shelfSetImageFirst (TSHELF *shelf, const int imgId, const int imgSrcIndex)
{
	return shelfSetImage(shelf, 0, imgId, imgSrcIndex, -1);
}

static inline int shelfSetImageLast (TSHELF *shelf, const int imgId, const int imgSrcIndex)
{
	const int idx = shelf->totalImg-1;
	return shelfSetImage(shelf, idx, imgId, imgSrcIndex, -1 /*no change*/);
}

void shelfSetClientImageTotal (TSHELF *shelf, const int imgTotal)
{
	shelf->clientImageTotal = imgTotal;
}

int shelfGetClientImageTotal (TSHELF *shelf)
{
	return shelf->clientImageTotal;
}

void shelfSortZDepth (TSHELF *shelf)
{
	// not the fastest sort routine but with low call frequency it need not be
	for (int i = 0; i < shelf->totalImg; i++){
		for (int j = 0; j < shelf->totalImg-1; j++){
			int idx_a = shelf->sortlist[j];
			int idx_b = shelf->sortlist[j+1];
			if (shelf->simg[idx_a].zOrder > shelf->simg[idx_b].zOrder){
				shelf->sortlist[j] = idx_b;
				shelf->sortlist[j+1] = idx_a;
			}
		}
	}
}

int shelfCalcZDepth (TSHELF *shelf)
{
	for (int i = -3; i < shelf->totalImg-3; i++){
		double imgPosition = shelf->x + (i * shelf->spacing);
		shelf->simg[i+3].zOrder = computeModifier(shelf, imgPosition);
	}
	return 1;
}

int shelfRender (TSHELF *shelf, TFRAME *frame, const int shelfIndex)
{
	int ct = 0;
	for (int i = 0; i < shelf->totalImg; i++){
		int idx = shelf->sortlist[i];
		ct = shelfDrawImage(shelf, frame, shelfIndex, idx-3, shelf->x);
	}
	return ct;
}

double shelfSetAnimationStepRate (TSHELF *shelf, double xDelta)
{
	if (xDelta <= 0.0001) xDelta = SHELF_ANIRATE_DEFAULT;
	return (shelf->xDelta = xDelta + fmod(shelf->spacing, xDelta));
}

TSHELF *shelfNew (const int totalImg, const double sigma, const double spacing, const double xDelta, const double rho, const double expMemMult, const double imgScaleMult, const double imgOpacityMult)
{
	TSHELF *shelf = my_calloc(1, sizeof(TSHELF));
	if (!shelf) return NULL;
	
	shelf->simg = my_calloc(totalImg, sizeof(TSHELFIMG));
	shelf->sortlist = my_calloc(totalImg, sizeof(int));

	shelf->totalImg = totalImg;
	shelf->index = 0;
	shelf->spacing = spacing;
	shelf->direction = 0;
	shelf->ichanged = 1;
	shelf->from = 0;
	shelf->to = 0;
	shelf->sigma = sigma;
	shelf->rho = 0.0;
    shelf->expMultiplier = 0.0;
    shelf->expMember = 0.0;
    shelf->x = 0.0;
    shelf->y = 0.0;
    shelf->xDelta = SHELF_ANIRATE_DEFAULT;
    shelf->imgScaleMult = imgScaleMult;
    shelf->imgOpacityMult = imgOpacityMult;
    shelf->clientImageTotal = 0;
    shelf->getClientImage_cb = NULL;
    shelf->uptr = NULL;

	for (int i = 0; i < shelf->totalImg; i++){
		shelf->simg[i].imgSrcIndex = -1;
		shelf->sortlist[i] = i;
	}
	shelfSetAnimationStepRate(shelf, xDelta);
	setSigma(shelf, shelf->sigma);
	return shelf;
}

void shelfDelete (TSHELF *shelf)
{
	shelfFlush(shelf);
	/*for (int i = 0; i < shelf->totalImg; i++){
		shelfFreeImage(shelf, (void*)shelf->simg[i].img, shelf->simg[i].imgSrcIndex);
		shelf->simg[i].img = NULL;
		shelf->simg[i].imgSrcIndex = -1;
	}*/
	
	my_free(shelf->simg);
	my_free(shelf->sortlist);
	my_free(shelf);
}

int shelfSetState (TSHELF *shelf, const int from, const int to, const double x)
{

	//printf("shelfSetState %p %i %i %f\n", shelf, from, to, x);

	shelf->from = from;
	shelf->to = to;
	shelf->x = x;
	
	for (int i = 0; i < shelf->totalImg; i++){
		shelfFreeImage(shelf, shelf->simg[i].imgSrcIndex, shelf->simg[i].data);
		shelf->simg[i].imgId = 0;	// could set this to default 'no art'
		shelf->simg[i].imgSrcIndex = -1;
	}

	if (from < to){
		int data = 0;
		shelf->direction = 1;
		shelf->index = from+3;

		/*if (from > to){
			int img = shelfGetClientImage(shelf, from-1, &data);
			shelfSetImage(shelf, 2, img, from-1, data);
			
			img = shelfGetClientImage(shelf, from-2, &data);
			shelfSetImage(shelf, 1, img, from-2, data);
			
			img = shelfGetClientImage(shelf, from-3, &data);
			shelfSetImage(shelf, 0, img, from-3, data);
		}else*/ if (from > 1){
			int img = shelfGetClientImage(shelf, from-1, &data);
			shelfSetImage(shelf, 2, img, from-1, data);
			
			img = shelfGetClientImage(shelf, from-2, &data);
			shelfSetImage(shelf, 1, img, from-2, data);

		}else if (from == 1){
			int img = shelfGetClientImage(shelf, from-1, &data);
			shelfSetImage(shelf, 2, img, from-1, data);
		}
		
		for (int i = shelf->totalImg-4, vIdx = from; i < shelf->totalImg; i++, vIdx++){
			if (vIdx < 0){
				shelfSetImage(shelf, i, 0, -1, -1);
			}else{
				int img = shelfGetClientImage(shelf, vIdx, &data);
				shelfSetImage(shelf, i, img, vIdx, data);
			}
		}
	}else if (from > to){
		int data = 0;
		shelf->direction = 2;
		shelf->index = from-3;

		if (from < shelfGetClientImageTotal(shelf)-4){
			int img = shelfGetClientImage(shelf, from+1, &data);
			shelfSetImage(shelf, 4, img, from+1, data);
			
			img = shelfGetClientImage(shelf, from+2, &data);
			shelfSetImage(shelf, 5, img, from+2, data);
			
			img = shelfGetClientImage(shelf, from+3, &data);
			shelfSetImage(shelf, 6, img, from+3, data);
		}else if (from < shelfGetClientImageTotal(shelf)-3){
			int img = shelfGetClientImage(shelf, from+1, &data);
			shelfSetImage(shelf, 4, img, from+1, data);
			
			img = shelfGetClientImage(shelf, from+2, &data);
			shelfSetImage(shelf, 5, img, from+2, data);
			
		}else if (from == shelfGetClientImageTotal(shelf)-2){
			int img = shelfGetClientImage(shelf, from+1, &data);
			shelfSetImage(shelf, 4, img, from+1, data);
		}
		
		for (int i = 3, vIdx = from; i >= 0; i--, vIdx--){
			if (vIdx < 0){
				shelfSetImage(shelf, i, 0, -1, -1);
			}else{
				int img = shelfGetClientImage(shelf, vIdx, &data);
				shelfSetImage(shelf, i, img, vIdx, data);
			}
		}
	}else{
		int data = 0;
		shelf->direction = 0;
		shelf->index = from+3;

		for (int i = 0, vIdx = from; i < shelf->totalImg; i++, vIdx++){
			if (vIdx-3 < 0){
				shelfSetImage(shelf, i, 0, -1, -1);
			}else{
				int img = shelfGetClientImage(shelf, vIdx-3, &data);
				shelfSetImage(shelf, i, img, vIdx-3, data);
			}
		}
	}

	return shelf->direction;
}

int shelfAnimateNext (TSHELF *shelf)
{
	/*if (shelf->from < shelf->to)
		shelf->direction = 1;
	else if (shelf->from > shelf->to)
		shelf->direction = 2;*/
		
	
	if (shelf->direction){
		if (shelf->direction == 1){		// moving from right to left
			shelf->x -= shelf->xDelta;
			if (shelf->x <= -shelf->spacing){
				shelf->x = 0.0;
				shelfMoveShelfLeft(shelf);

				shelf->from++;
				if (++shelf->index > shelfGetClientImageTotal(shelf)-1){
					shelfSetImageLast(shelf, 0, -1);

					if (shelf->index > shelfGetClientImageTotal(shelf)+2){
						shelf->index = shelfGetClientImageTotal(shelf)-4;
						shelf->direction = 0;
					}
				}else{
					//printf(" < requesting index %i, %i\n", shelf->index, shelf->from);
					int data = 0;
					int img = shelfGetClientImage(shelf, shelf->index, &data);
					shelfSetImage(shelf, 6, img, shelf->index, data);
				}
			}
			shelf->ichanged = 1;

		}else if (shelf->direction == 2){ //  moving from left to right
			shelf->x += shelf->xDelta;
			if (shelf->x >= shelf->spacing){
				shelf->x = 0.0;
				shelfMoveShelfRight(shelf);

				shelf->from--;
				if (--shelf->index < 0){
					shelfSetImageFirst(shelf, 0, -1);

					if (shelf->index < -3){
						shelf->index = 3;
						shelf->direction = 0;
					}
				}else{
					//printf(" > requesting index %i, %i\n", shelf->index, shelf->from);
					
					// accquire an updated left most image
					int data = 0;
					int img = shelfGetClientImage(shelf, shelf->index-6, &data);
					shelfSetImage(shelf, 0, img, shelf->index-6, data);
				}
			}
			shelf->ichanged = 1;
		}
		if (shelf->from == shelf->to)
			shelf->direction = 0;
	}
	return shelf->from;
}

