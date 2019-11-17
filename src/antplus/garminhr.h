
// anthrm - http://mylcd.sourceforge.net/
// An LCD framebuffer and text rendering API
// Michael McElligott
// okio@users.sourceforge.net

//  Copyright (c) 2005-2011  Michael McElligott
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

#ifndef _GARMINHR_H_
#define _GARMINHR_H_

#if ENABLE_ANTPLUS

#include "../common.h"
#include "libantplus.h"







#define KEY_SIZE		8



enum _akeys {
	KEY_ANTSPORT,
	KEY_SUUNTO,
	KEY_GARMIN,
	KEY_ANTPLUS,
	KEY_TOTAL
};

typedef struct {
	TFRAME **image;
	int x;
	int y;
}TBMPDIGIT;

typedef struct {
	TBMPDIGIT digit[3];
	int width;
}TBMPVALUE;

typedef struct {
	TBMPVALUE bmp[256];
	int built;
}TBMPLOOKUP;

typedef struct{
	int channel;
	//int keyIdx;
	int RFFreq;
	int channelPeriod;
	int searchTimeout;
	int searchWaveform;
	
	int channelType;
	int networkNumber;
	int deviceNumber;
	int deviceType;
	int transType;

	void *lock;		// ready to update hr
	void *unlock;	// update complete
	
	struct {
		int index;
		unsigned char key[8];
	}dongle;
}TDCONFIG;

typedef struct{
	int channelStatus;
	int chanIdOnce;
	int running;
	int openCt;
}TCONNSTATES;

#define HRBMP_BUFFERLENGTH (800)		// should be as wide as the display

typedef struct{
	unsigned char bpm[HRBMP_BUFFERLENGTH];
	unsigned char currentBpm;
	int currentSequence;
	int previousSequence;
	
	int low;	// lowest thus far
	int high;	// maximum read thus far
	int average;
	int mode;
	uint64_t time0;	// time when last pulse was read
}THRBUFFER;

typedef struct{
	int scidDeviceNumber;
	int scidDeviceType;
	int scidTransType;
}TDEVICET;

typedef struct{
	TLIBANTPLUS *ant;
	TDCONFIG	*dcfg;
	TDEVICET	*dev;
	THRBUFFER	*rate;
	TCONNSTATES	*cstates;
	TIMAGES		*images;
	TBMPLOOKUP  digitsTable;
	TVLCPLAYER  *vp;
	
	void (*pulseCB) (const THRBUFFER *pulse, void *userPtr);
	void *pulsePtr;

	int keyset;
	
	uintptr_t hMsgThread;
	unsigned int msgThreadID;
	unsigned int msgThreadState;
	
	int dfound;		// is stick found and connected
	int hfound; 	// has hrm been found and synced
	
	uint64_t lastDSearchTime;
	uint64_t lastHSearchTime;
}THR;



int messageEventCb (const int chan, const int msgId, const unsigned char *payLoad, const size_t dataLength, void *uPtr);


#endif

#endif

