
// anthrm - http://mylcd.sourceforge.net/
// An LCD framebuffer and text rendering API
// Michael McElligott
// okio@users.sourceforge.net

//  Copyright (c) 2005-2011  Michael McElligott
//
//  This program is my_free software; you can redistribute it and/or
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
#include <process.h>


#if ENABLE_ANTPLUS


#include "anthrm.h"
#include "garminhr.h"


static const int minBpm = 20;		// i've just died
static const int maxBpm = 240;		// i'm probably about to fall over..


#ifdef drawImage
#undef drawImage
#endif
#define drawImage(s,d,x,y) copyArea(s,d,x,y,0,0,s->width-1,d->height-1)




// when trying each of the ant+ keys
// each key is tried CONNECTION_TIMEOUT times before moving on to the next key
// then repeated CONNECTION_RETRIES times
#define CONNECTION_TIMEOUT	(2)		// per key retry on failure
#define CONNECTION_RETRIES	(5)		// all key retry
#define LIBUSB_DISCONNECT	(-5)
#define LIBUSB_TIMEOUT		(-116)

#define ANT_CONN_TIMEOUT1	 5000
#define ANT_CONN_TIMEOUT2	10000
#define ANT_CONN_TIMEOUT3	15000


static const char *msg[3] = {"read timeout", "device not found", "read error"};


void hrmStatsClear (THRBUFFER *rate)
{
	for (int i = 0; i < HRBMP_BUFFERLENGTH; i++)
		rate->bpm[i] = 0;

	rate->currentBpm = 0;
	rate->low = 255;
	rate->high = 0;
	rate->mode = 0;
	rate->average = 0;
	rate->time0 = 0;
}

static inline void hrmStatsCopy (THRBUFFER *src, THRBUFFER *des)
{
	my_memcpy(des, src, sizeof(*des));
	des->time0 = 0;
}

void hrmStatsSet (THR *hr, THRBUFFER *src)
{
	hrmStatsCopy(src, hr->rate);
}

void hrmStatsGet (THR *hr, THRBUFFER *des)
{
	hrmStatsCopy(hr->rate, des);
}

void hrmStatsReset (THR *hr)
{
	hrmStatsClear(hr->rate);
}

static inline int appRunState (THR *hr)
{
	return hr->vp->applState != 0;
}

static inline int getRunState (THR *hr)
{
	return hr->cstates->running;
}

static inline void setRunState (THR *hr, const int state)
{
	hr->cstates->running = state;
}

static inline TFRAME *imageGet (TIMAGES *img, const int imgIdx)
{
	return img->list[imgIdx];
}

static inline TFRAME **imageGetAddr (const TIMAGES *img, const int imgIdx)
{
	return (TFRAME**)&img->list[imgIdx];
}

static inline int antOpen (THR *hr)
{
	int status = libantplus_Open(hr->ant, 0);
	if (status > 0)
		hr->cstates->openCt = 1;
	return status;
}

static inline void antClose (THR *hr)
{
	if (hr->cstates->openCt)
		libantplus_Close(hr->ant);
	hr->cstates->openCt = 0;
}

static inline int antGetOpenCt (THR *hr)
{
	return hr->cstates->openCt;
}

static inline void antCloseChannel (THR *hr)
{
	if (hr->cstates->channelStatus && antGetOpenCt(hr)){
		libantplus_CloseChannel(hr->ant, hr->dcfg->channel);
		//hr->cstates->channelStatus = 0;
	}
}

static inline THR *new_HR (const int vid, const int pid)
{
	THR *hr = my_calloc(1, sizeof(THR));
	if (hr){
		hr->dcfg = my_calloc(1, sizeof(TDCONFIG));
		if (hr->dcfg){
			hr->cstates = my_calloc(1, sizeof(TCONNSTATES));
			if (hr->cstates){
				hr->rate = my_calloc(1, sizeof(THRBUFFER));
				if (hr->rate){
					hr->dev = my_calloc(1, sizeof(TDEVICET));
					if (hr->dev){
						hr->ant = libantplus_Init(vid, pid);
						if (hr->ant){
							setRunState(hr, 1);
							return hr;
						}
						my_free(hr->dev);
					}
					my_free(hr->rate);
				}
				my_free(hr->cstates);
			}
			my_free(hr->dcfg);
		}
		my_free(hr);
	}

	return NULL;
}

static inline void delete_HR (THR *hr)
{
	if (hr){
		my_free(hr->ant);
		my_free(hr->dcfg);
		my_free(hr->cstates);
		my_free(hr->dev);
		my_free(hr->rate);
		my_free(hr);
	}
}

static inline void setKey (THR *hr, const int keyIdx)
{
	//hr->dcfg->keyIdx = keyIdx;
	hr->cstates->chanIdOnce = 0;
	hr->cstates->channelStatus = 0;
	hr->rate->low = 255;
	hr->rate->high = 0;
	hr->rate->time0 = 0;
	libantplus_ResetSystem(hr->ant);
}

static inline void drawSignal (THR *hr, TFRAME *frame, const int64_t t1, const unsigned int x, const unsigned int y)
{
	const int64_t dt = t1 - hr->rate->time0;
	if (dt <= ANT_CONN_TIMEOUT3){
		TFRAME *sig = NULL;
		//if (dt <= ANT_CONN_TIMEOUT1)
			sig = imageGet(hr->images, IMG_SIGNAL);
		//else if (dt <= ANT_CONN_TIMEOUT2)
		//	sig = imageGet(hr->images, IMG_SIGNAL2);
		//else/* if (dt <= 15000)*/
		//	sig = imageGet(hr->images, IMG_SIGNAL3);

		drawImage(sig, frame, frame->width-1 - sig->width, y);
	}
}

static inline void calcDigit (TBMPVALUE *bmp, const TIMAGES *images, const int value, const int space, const int frameWidth, const int frameHeight)
{
	TFRAME **d1 = NULL;
	TFRAME **d2 = NULL;
	TFRAME **d3 = NULL;

	int x1 = 0;
	int x2 = 0;
	int x3 = 0;
	int y1 = 0;
	int y2 = 0;
	int y3 = 0;


	if (value >= 100 && value <= 255){
		d1 = imageGetAddr(images, value / 100);
		d2 = imageGetAddr(images, (value / 10) % 10);
		d3 = imageGetAddr(images, value % 10);

    	bmp->width = (*d1)->width + space+ (*d2)->width + space + (*d3)->width;
    	x1 = (frameWidth - bmp->width) / 2;
    	x2 = x1 + (*d1)->width + space;
    	x3 = x2 + (*d2)->width + space;

    	y1 = (frameHeight - (*d1)->height)/2;
    	y2 = (frameHeight - (*d2)->height)/2;
    	y3 = (frameHeight - (*d3)->height)/2;

	}else if (value >= 10 && value <= 99){
		d1 = imageGetAddr(images, value / 10);
		d2 = imageGetAddr(images, value % 10);

    	bmp->width = (*d1)->width + space + (*d2)->width;
    	x1 = (frameWidth - bmp->width) / 2;
    	x2 = x1 + (*d1)->width + space;

    	y1 = (frameHeight - (*d1)->height)/2;
    	y2 = (frameHeight - (*d2)->height)/2;

	}else{
		d1 = imageGetAddr(images, value);

		bmp->width = (*d1)->width;
		x1 = (frameWidth - bmp->width)/2;
		y1 = (frameHeight - (*d1)->height)/2;
	}

	bmp->digit[0].image = d1;
	bmp->digit[0].x = x1;
	bmp->digit[0].y = y1;
	bmp->digit[1].image = d2;
	bmp->digit[1].x = x2;
	bmp->digit[1].y = y2;
	bmp->digit[2].image = d3;
	bmp->digit[2].x = x3;
	bmp->digit[2].y = y3;
}

static inline void buildDigitPositionTable (TBMPLOOKUP *digits, TIMAGES *images, const int width, const int height)
{
	for (int i = 0; i < 256; i++)
		calcDigit(&digits->bmp[i], images, i, -12, width, height);
	digits->built = 1;
}

static inline void drawPulse (THR *hr, TBMPLOOKUP *digits, TFRAME *frame, const int value)
{
	TBMPVALUE *bmp = &digits->bmp[value&0xFF];

	TFRAME *img = *(bmp->digit[0].image);
	drawImage(img, frame, bmp->digit[0].x, bmp->digit[0].y);

	if (bmp->digit[1].image){
		img = *(bmp->digit[1].image);
		drawImage(img, frame, bmp->digit[1].x, bmp->digit[1].y);
	}

	if (bmp->digit[2].image){
		img = *(bmp->digit[2].image);
		drawImage(img, frame, bmp->digit[2].x, bmp->digit[2].y);
	}
}

static inline int getAve (unsigned char *bpm, const int len)
{
	int ct = 0;
	int sum = 0;

	for (int i = (HRBMP_BUFFERLENGTH-len-1); i < HRBMP_BUFFERLENGTH; i++){
		if (bpm[i] >= minBpm && bpm[i] <= maxBpm){
			sum += bpm[i];
			ct++;
		}
	}
	if (ct)
		return sum/(float)ct;
	else
		return 0;
}

static inline int getMode (unsigned char *bpm, const int len)
{
	unsigned char hist[HRBMP_BUFFERLENGTH];
	memset(hist, 0, HRBMP_BUFFERLENGTH);

	for (int i = (HRBMP_BUFFERLENGTH-len-1); i < HRBMP_BUFFERLENGTH; i++)
		hist[bpm[i]]++;

	int mode = 0;
	int most = 1;

	for (int i = minBpm; i < HRBMP_BUFFERLENGTH && i <= maxBpm; i++){
		if (hist[i] > most){
			most = hist[i];
			mode = i;
		}
	}

	return mode;
}

static inline void drawHeading (THR *hr, TFRAME *frame, THRBUFFER *rate)
{
	static unsigned int alt;

	lSetBackgroundColour(frame->hw, 0x00FFFFFF);
	lSetForegroundColour(frame->hw, 0xFF000000);
	lSetRenderEffect(frame->hw, LTR_OUTLINE2);
	lSetFilterAttribute(frame->hw, LTR_OUTLINE2, 0, 220<<24 | 0xFFFFFF);
	const int font = ANT_HEADING_FONT;

	if (++alt&0x02){	// no need to recalculate every pass that isn't going to have changed by much
		rate->average = getAve((unsigned char*)rate->bpm, frame->width);
		rate->mode = getMode((unsigned char*)rate->bpm, frame->width);
	}

	TFRAME *min = lNewString(frame->hw, frame->bpp, 0, font, "  Min: %i", rate->low);
	TFRAME *ave = lNewString(frame->hw, frame->bpp, 0, font, "Ave: %i\t\t\tMode: %i", rate->average, rate->mode);
	TFRAME *max = lNewString(frame->hw, frame->bpp, 0, font, "Max: %i  ", rate->high);

	drawImage(min, frame, 0, 0);
	drawImage(ave, frame, (frame->width - ave->width)/2, 0);
	drawImage(max, frame, frame->width - max->width, 0);

	lDeleteFrame(min);
	lDeleteFrame(ave);
	lDeleteFrame(max);

	lSetRenderEffect(frame->hw, LTR_DEFAULT);
}

static inline int hrmGetDeviceCount (void *ptr)
{
	THR *hr = (THR*)ptr;
	if (hr && hr->ant)
		return libantplus_GetDeviceCount(hr->ant->vid, hr->ant->pid);
	else
		return libantplus_GetDeviceCount(ANTSTICK_VID, ANTSTICK_PID);
}

static inline int doHRMSearch (THR *hr, TFRAME *frame, const int attempts, const int timeout)
{

	if (!hr->keyset){
		hr->keyset = 1;
		setKey(hr, 0);	// this activates the search which is then handled & processed via the other thread
	}

	if (appRunState(hr)) setRunState(hr, 0);

	int found = hr->dev->scidDeviceNumber;
	if (found){
		setRunState(hr, 1);
		dbprintf(hr->vp, "Found HRM %i", hr->dev->scidDeviceNumber);

	}
	return found;
}

// block until the ant+ usb dongle is found
static inline int waitForDeviceConnect (THR *hr, TFRAME *frame, int deviceIdx)
{
	int found = 0;

	//do{
		dbprintf(hr->vp, "searching for Ant+ USB device %i (0x%.4X/0x%.4X)", deviceIdx, hr->ant->vid, hr->ant->pid);

		if (deviceIdx >= 0){
			found = libantplus_Discover(hr->ant, deviceIdx);
		}else{
			for (int i = 0; i < 8; i++){
				found = libantplus_Discover(hr->ant, i);
				if (found){
					deviceIdx = i;
					break;
				}
			}
		}

		if (found)
			dbprintf(hr->vp, "Ant+ USB device found. Index %i", deviceIdx);

		if (appRunState(hr)) setRunState(hr, 0);
		//if (!found && getRunState(hr))
		//	lSleep(100);
	//}while(!found && getRunState(hr));

	if (found) setRunState(hr, 1);

	return found;
}

static inline unsigned int __stdcall hrmMsgThread (void *ptr)
{
	//printf("hrmMsgThread start\n");

	THR *hr = ptr;
	hr->msgThreadState = 1;

	while(hr->msgThreadState){
		int readOK = libantplus_HandleMessages(hr->ant);		// blocking read
		if (appRunState(hr)){
			setRunState(hr, 0);
			//break;
		}

		if (readOK == LIBUSB_DISCONNECT && getRunState(hr)){
			//printf("@@resetting\n");
			libantplus_ResetSystem(hr->ant);
			antCloseChannel(hr);
			antClose(hr);
			hr->dev->scidDeviceNumber = 0;
			hr->hfound = 0;
			hr->dfound = 0;
			readOK = 1;
			lSleep(100);

			// if we're here then contact between HRM strap and skin could be poor
			// (strap could be poorly fitted or not moist enough)
		}else if (readOK < 0){
			const int dcount = hrmGetDeviceCount(hr);	// check if dongle has been removed
			if (dcount < 1){		// if so, set the Appl back in to search mode
				hr->dfound = 0;		// signal absent device
				hr->hfound = 0;		// signal strap is no longer reachable
			}

			if (pageGet(hr->vp) == PAGE_ANTPLUS && hr->dfound){
				char *errormsg;

				if (readOK == LIBUSB_TIMEOUT)
					errormsg = (char*)msg[0];
				else if (readOK == LIBUSB_DISCONNECT)
					errormsg = (char*)msg[1];
				else
					errormsg = (char*)msg[2];

				dbprintf(hr->vp, "Ant+ %s (%i, %i). Retrying...", errormsg, readOK, dcount);
				lSleep(100);
			}else{
				lSleep(1000);
			}
		}
		lSleep(10);
	}

	hr->msgThreadState = 0;
	//printf("hrmMsgThread exit\n");

	_endthreadex(1);
	return 1;
}

int hrmRender (THR *hr, TFRAME *frame)
{
	if (!hr->ant) return 0;

	drawHeading(hr, frame, hr->rate);

	uint64_t t1 = getTickCount();
	if (!hr->dfound){
		if (t1 - hr->lastDSearchTime >= 1000){
			hr->lastDSearchTime = t1;

			hr->dfound = waitForDeviceConnect(hr, frame, hr->dcfg->dongle.index);
			if (hr->dfound && getRunState(hr)){
				antClose(hr);
				hr->dfound = antOpen(hr);
				//printf("dfound %i\n", hr->dfound);
				if (!hr->dfound){
					dbprintf(hr->vp, "Device %i unavailable", hr->dcfg->dongle.index);
					return -3;
				}else{
					dbprintf(hr->vp, "Device %i acquired", hr->dcfg->dongle.index);
					if (hr->ant->strings[0][0])
						dbprintf(hr->vp, " %s ", hr->ant->strings[0]);
					if (hr->ant->strings[1][0] && hr->ant->strings[2][0])
						dbprintf(hr->vp, " %s - %s ", hr->ant->strings[1], hr->ant->strings[2]);
					else if (hr->ant->strings[1][0])
						dbprintf(hr->vp, " %s ", hr->ant->strings[1]);
				}
			}else{
				//lSleep(50);
				return -1;
			}
		}else{
			return -2;
		}
		hr->hfound = 0;
	}

	if (!hr->hfound){
		if (t1 - hr->lastHSearchTime >= 1000){
			hr->lastHSearchTime = t1;
			hr->hfound = doHRMSearch(hr, frame, CONNECTION_RETRIES, CONNECTION_TIMEOUT);
			//printf("hfound %i %I64d\n", hr->hfound, hr->lastHSearchTime);
			if (hr->hfound == -1) hr->dfound = 0;
		}
	}

	if (hr->hfound > 0){
		drawPulse(hr, &hr->digitsTable, frame, hr->rate->currentBpm);
		drawSignal(hr, frame, t1, 0, 128);
		return 1;
	}
	return 0;
}

THR *hrmStart (const int vid, const int pid, const int deviceIndex, const unsigned char *deviceKey, const int sensorId, void *vpPtr, void *cb, void *userPtr, TIMAGES *imagelist)
{
	THR *hr = new_HR(vid, pid);
	if (hr == NULL) return NULL;

	hr->vp = vpPtr;
	hr->pulseCB = cb;
	hr->pulsePtr = userPtr;
	hr->images = imagelist;
	buildDigitPositionTable(&hr->digitsTable, hr->images, getFrontBuffer(hr->vp)->width, getFrontBuffer(hr->vp)->height);

	hr->dcfg->deviceNumber = sensorId&0x7FFF;		// 0, search for this device. 0 = any		/* 26434 */
	hr->dcfg->deviceType = 0x78;	// 1
	hr->dcfg->transType = 0;		// 5
	hr->dcfg->channelType = 0;
	hr->dcfg->networkNumber = 1;
	hr->dcfg->channel = 0;
	hr->dcfg->channelPeriod = 8070;
	hr->dcfg->RFFreq = /*0x32*/ 0x39 /*66*/;
	hr->dcfg->searchTimeout = 0xFF;
	hr->dcfg->searchWaveform = 0x53;
	hr->dcfg->dongle.index = deviceIndex;
	my_memcpy(hr->dcfg->dongle.key, deviceKey, sizeof(hr->dcfg->dongle.key));

	hr->keyset = 0;
	libantplus_SetEventFunction(hr->ant, EVENTI_MESSAGE, messageEventCb, hr);

	hr->hMsgThread = _beginthreadex(NULL, THREADSTACKSIZE, hrmMsgThread, hr, 0, &hr->msgThreadID);

	return hr;
}

int hrmShutdown (THR *hr)
{
	//printf("hrmShutdown\n");

	hr->msgThreadID = 0;
	hr->msgThreadState = 0;
	//libantplus_HandleMessages(hr->ant);
	antCloseChannel(hr);
	antClose(hr);

	WaitForSingleObject((HANDLE)hr->hMsgThread, INFINITE);
	CloseHandle((HANDLE)hr->hMsgThread);

	delete_HR(hr);
	return 1;
}




#endif

