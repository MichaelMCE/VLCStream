
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



#ifndef _PLAYER_H_
#define _PLAYER_H_




enum _btnPlyQueue
{
	BTN_PLYQUEUE_CLOSE,
	BTN_PLYQUEUE_STOP,
	BTN_PLYQUEUE_EQ,
	BTN_PLYQUEUE_PLAY,
	BTN_PLYQUEUE_TOTAL
};

typedef struct qitem {
	void *dataPtr;
	int track;
	int itemNumber;
	void *ccCtrl;
	
	struct {
		int large;
		int small;
		int art;
	}itemId;
}playlist_queue_item_t;


typedef struct qitems {
	TLISTITEM *items;
	TLISTITEM *last;
}playlist_queue_t;



typedef struct{
	TPAGE2COM *com;
	TCCBUTTONS *btns;
	TLABEL *base;
	playlist_queue_t playlistQueue;
	
	int playingTrk;
	int signalQueueRebuild;
	int trackTitleCharSpacing;
	int playlistQueueUID;
	int playlistQueueItemsAdded;
	
	int updateButtons;		// flag
		
	struct {
		int baseYDelta;
		int baseIsSliding;
		int baseIsPressed;
		int basePressedId;
		int baseIsHeld;
		double time0;
	}input;
}TPLYQUEUE;

int page_queueCb (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

void plyQueueNewTrackEvent (TPLYQUEUE *plyqueue, unsigned int uid, const int trackIdx);

#endif


