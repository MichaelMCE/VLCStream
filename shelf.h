
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



#ifndef _SHELF_H_
#define _SHELF_H_


#define SHELF_IMGTYPE_NOART		0
#define SHELF_IMGTYPE_ARTWORK	1


typedef struct {
	int imgId;
	
	double modifier;
	double zOrder;
	TLPOINTEX pos;		// location of rendered image within target frame
	int imgSrcIndex;
	int data;			// per item callback data
}TSHELFIMG;

typedef struct {
	TSHELFIMG *simg;	// a shelf image
	int *sortlist;		// list of sorted ([shelf image(s)) indexes by Z depth, ready for drawing
	
	int totalImg;		// on display images per shelf (odd numbers work best)
	int index;			// current image ops on this, usually set to center of shelf
	int direction;		// scroll direction (1:left to right, 2:right to left, 0:don't scroll)
	int	ichanged;		// flag raised when image changed has occurred
	
	int from;			// shuffle from
	int to;				// shuffle to
	
	double sigma;
	double rho;
    double expMultiplier;
    double expMember;
    
	double spacing;		// distance between images
    double x;			// -1 = far left, 0.0 = center, 1.0 = far right
    double y;			// unused
    double xDelta;		// scroll x by this amount
    
    double imgOpacityMult;
	double imgScaleMult;
    
    int clientImageTotal;	// number of images client has available [ready to grab]
    
    int 	(*getClientImage_cb) (void *ptr, const int index, int *data);
    void	(*clientFreeImage_cb) (void *fptr, const int index, const int data);
	void	 *fptr;			// user data supplied with clientFreeImage_cb
    void	 *uptr;			// user pointer for image callbacks
    int		(*clientRender_cb) (void *ptr, const int idx, TFRAME *src, TFRAME *des, const int x, const int y, const int width, const int height, double modifier);
    void	 *uRenderPtr;	// user data for rendering callback
    
    TARTMANAGER *am;
    int imgNoArtId;
}TSHELF;



TSHELF *shelfNew (const int totalImg, const double sigma, const double spacing, const double xDelta, const double rho, const double expMemMult, const double imgScaleMult, const double imgOpacityMult);
void shelfDelete(TSHELF *shelf);
int shelfSetState (TSHELF *shelf, const int from, const int to, const double x);
void shelfSetClientImageLookupCB (TSHELF *shelf, void *uptr, int (*cb) (void *ptr, const int index, int *data));
void shelfSetClientFreeImageCB (TSHELF *shelf, void (*cb) (void *fptr, const int index, const int data), void *fptr);
void shelfSetClientRenderCB (TSHELF *shelf, int (*renderCB)(void *ptr, const int idx, TFRAME *src, TFRAME *des, const int x, const int y, const int width, const int height, double modifier), void *uRednerPtr);
void shelfSetClientImageTotal (TSHELF *shelf, const int imgTotal);
int shelfGetClientImageTotal (TSHELF *shelf);
int shelfRender (TSHELF *shelf, TFRAME *frame, const int shelfIndex);

double shelfSetAnimationStepRate (TSHELF *shelf, double xDelta);
int shelfCalcZDepth (TSHELF *shelf);
void shelfSortZDepth (TSHELF *shelf);
int shelfAnimateNext (TSHELF *shelf);

int shelfGetClientImage (TSHELF *shelf, const int index, int *data);

void shelfFlush (TSHELF *shelf);

void shelfSetSigma (TSHELF *shelf, const double sigma);

#endif
