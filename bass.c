
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

#if (ENABLE_BASS || !RELEASEBUILD)




extern int SHUTDOWN;
float audioLevel[2] = {0.0, 0.0};

#define blockWidth  12


void bass_render (TBASSLIB *bass, TFRAME *frame, const int x1, const int y1)
{
	if (!bass->visuals) return;

	TBASSVISUAL *vis = &bass->vis[0];

	for (int i = 0; i < 7; i++)
		my_memcpy(bass->visData[i], bass->visData[i+1], vis->bufferLen);
		
	int *visDataLive = my_malloc(vis->bufferLen);
	my_memcpy(visDataLive, bass->visData[7], vis->bufferLen);

	//int hPre = y1;
	

#if 0
	for (int x = 0; x < bass->vwidth; x += blockWidth+1){
		int acc = 0;
		for (int a = x; a < x+blockWidth; a++)
			acc += visDataLive[a>>1];
			
		acc /= blockWidth;
		lDrawRectangleFilled(frame, x1+x, y1, x1+x+blockWidth-1, y1 - acc, 220<<24 | 0x00FF00);
	}
#endif


#if 1
	int gap = 0;
	for (int x = 0; x < bass->vwidth; x += blockWidth){
		float acc = visDataLive[x>>1];
		
		for (int a = x+1; a < x+blockWidth-1; a++)
			acc = (acc + visDataLive[a/2])/2.0f;
			
		//acc /= blockWidth;
		float val = acc * (100.0f/(float)bass->vheight);
		int colour = RGB(255-(2.55f*val),255-(2.15f*val),2.55f*val);
		
		lDrawRectangleFilled(frame, x1+x+gap, y1, gap+x1+x+blockWidth-1, y1 - acc, 255<<24 | (colour&0xFFFFFF));
		gap++;
	}
#endif


#if 0
	for (int x = 1; x < bass->vwidth; x++){
		int val = 0;
		
		for (int i = 0; i < 8; i++)
			val += (bass->visData[i][x>>1] * (i+1));
			
		val /= 26;
		int h = y1 - val;
		lDrawLine(frame, x1+x-1, hPre, x1+x, h, 0xFFFF0000);
		lDrawLine(frame, x1+x-1, hPre-1, x1+x, h-1, 0xFFFF0000);
		hPre = h;
		
	}
#endif

#if 0
	for (int x = 0; x < bass->vwidth; x++){
		float val = visDataLive[x>>1] * (100.0f/(float)bass->vheight);
		int colour = RGB(255-(2.55f*val),255-(2.15f*val),2.55f*val);
		
		lDrawLine(frame, x+x1, y1, x+x1, y1-visDataLive[x>>1], 255<<24 | colour);	
	}
#endif


#if 0
	for (int x = 0; x < bass->vwidth; x++){
		float acc = visDataLive[x/2];
		
		for (int i = 3; i < 8; i++)
			acc = (acc + bass->visData[i][x/2])/2.0f;
		
		float val = acc * (100.0f/(float)bass->vheight);
		int colour = RGB(255-(2.55f*val),255-(2.15f*val),2.55f*val);

		lDrawLine(frame, x+x1, y1, x+x1, y1-visDataLive[x>>1], 255<<24 | colour);	
	}
#endif




#if 0
	hPre = y1;
	for (int x = 1; x < bass->vwidth; x++){
		int h = y1 - visDataLive[x>>1];
		lDrawLine(frame, x1+x-1, hPre, x1+x, h, 170<<24 | 0xFF0000);
		lDrawLine(frame, x1+x-1, hPre-1, x1+x, h-1, 170<<24 | 0x0FF0000);
		hPre = h;
	}
	
#endif

#if 0
	int gap = 0;
	for (int x = 0; x < bass->vwidth; x += blockWidth){
		float acc = visDataLive[x/2];
		
		for (int i = 0; i < 8; i++){
			for (int a = x+1; a < x+blockWidth-1; a++)
				acc = (acc + bass->visData[i][a/2])/2.0f;
		}
		for (int a = x+1; a < x+blockWidth-1; a++)
			acc = (acc + visDataLive[a/2])/2.0f;
				
		//acc /= (blockWidth*32);
				
		for (int a = x+1; a < x+blockWidth-1; a++){
			if (visDataLive[a/2] > acc)
				acc = visDataLive[a/2];
		}

		lDrawRectangleFilled(frame, gap+x1+x, (y1 - acc)-1, gap+x1+x+blockWidth-1, (y1 - acc)+1, 220<<24 | 0xFF000F);
		gap++;
	}
#endif

	//sbuiSetDKImage3(bass->vp, frame, 0, 200, x1, y1-bass->vheight-1, x1+bass->vwidth-1, y1);

	my_free(visDataLive);
}

#if 0
void bass_muteSet (TBASSLIB *bass, const int mute)
{
	if (bass->outEnabled)
		BASS_WASAPI_SetMute(BASS_WASAPI_CURVE_WINDOWS, mute&0x01);
}

int bass_muteGet (TBASSLIB *bass)
{
	if (bass->outEnabled)
		return BASS_WASAPI_GetMute(BASS_WASAPI_CURVE_WINDOWS);
	return -1;
}

void bass_volumeSet (TBASSLIB *bass, const float vol)
{
	if (bass->outEnabled)
		BASS_WASAPI_SetVolume(BASS_WASAPI_CURVE_WINDOWS, vol/100.0f);
}

float bass_volumeGet (TBASSLIB *bass)
{
	if (bass->outEnabled)
		return bass->volume = (BASS_WASAPI_GetVolume(BASS_WASAPI_CURVE_WINDOWS)*100.0f);
	else
		return 0.0;
}
#endif

static inline int bass_getDefaultIn ()
{
	BASS_WASAPI_DEVICEINFO di;

	for (int i = 0; BASS_WASAPI_GetDeviceInfo(i, &di); i++){
		if (di.flags&BASS_DEVICE_INPUT && di.flags&BASS_DEVICE_DEFAULT && di.flags&BASS_DEVICE_ENABLED)
			return i;
	}
	return -1;
}

static inline int bass_getDefaultOut ()
{
	BASS_WASAPI_DEVICEINFO di;
	memset(&di, 0, sizeof(BASS_WASAPI_DEVICEINFO));

	for (int i = 0; BASS_WASAPI_GetDeviceInfo(i, &di); i++){
		if ((di.flags&~BASS_DEVICE_INPUT) && di.flags&BASS_DEVICE_DEFAULT && di.flags&BASS_DEVICE_ENABLED)
			return i;
	}

	return -1;
}


__stdcall unsigned int sbRenderLevelFunc (void *ptr)
{
	const int tid = GetCurrentThreadId();
	printf("sbRenderLevelFunc START %i\n", tid);
	
	TBASSLIB *bass = (TBASSLIB*)ptr;
	lSleep(1000);
	
	TFRAME *img = lNewFrame(bass->vp->ml->hw, 120, 120, LFRM_BPP_32);
	for (int i = 1; i < 11; i++) sbuiSetDKImage(bass->vp, i, img);


	
	TFRAME *imgLeft = lNewFrame(bass->vp->ml->hw, 800, 64, LFRM_BPP_32);
	TFRAME *imgRight = lNewFrame(bass->vp->ml->hw, 800, 64, LFRM_BPP_32);
	
	while (WaitForSingleObject(bass->changeEventAudioLevel, INFINITE) == WAIT_OBJECT_0){
		if (SHUTDOWN || !bass->chan) break;
		
		//sbuiSetDKImage(bass->vp, 2, img);
		
		float left = audioLevel[0] * 0.9f;
		float vol = left * 1.0f;
		if (vol > 1.0f) vol = 1.0f;
		
		int col = RGB(255-(255.0f*vol),255-(200.0f*vol),255.0f*vol);
		memset(imgLeft->pixels, 0, imgLeft->frameSize);
		lDrawRectangleFilled(imgLeft, 0, 1, 800.0 * left, 62, col);
		sbuiSetDKImageEx(bass->vp, imgLeft, 0, 172, 0, 0, imgLeft->width-1, imgLeft->height-1);
		
		float right = audioLevel[1] * 0.9f;
		vol = right * 1.0f;
		if (vol > 1.0f) vol = 1.0f;
		
		col = RGB(255-(255.0f*vol),255-(200.0f*vol),255.0f*vol);
		memset(imgRight->pixels, 0, imgRight->frameSize);
		lDrawRectangleFilled(imgRight, 0, 1, 800.0 * right, 62, col);
		sbuiSetDKImageEx(bass->vp, imgRight, 0, 352, 0, 0, imgRight->width-1, imgRight->height-1);
		
	}
	
	lDeleteFrame(img);
	lDeleteFrame(imgLeft);
	lDeleteFrame(imgRight);

	
	//CloseHandle(bass->changeEventAudioLevel);
	
	printf("sbRenderLevelFunc EXIT %i\n", tid);
	_endthreadex(1);
	return 1;
}


#if 1
BOOL CALLBACK DuffRecording (HRECORD handle, const void *buffer, DWORD length, void *ptr)
{
	return FALSE;
	
	float *data = (float*)buffer;
	//printf("DuffRecording %p %i\n", data, (int)length);
	
	length /= sizeof(float);
	float maxValL = 0.0f;
	float maxValR = 0.0f;
	
	for (int i = 0; i < length; i+=2){
		if (*data > maxValL) maxValL = *data;
		if (*(++data) > maxValR) maxValR = *data;
	}

	audioLevel[0] = maxValL;
	audioLevel[1] = maxValR;
	
	TBASSLIB *bass = ptr;
	SetEvent(bass->changeEventAudioLevel);
	
	//printf("DuffRecording %f %i\n", maxVal, (int)length);
	return TRUE; // continue recording
}
#else
BOOL CALLBACK DuffRecording (HRECORD handle, const void *buffer, DWORD length, void *ptr)
{
	
	unsigned char *data = (unsigned char*)buffer;
	//printf("DuffRecording %p %i\n", data, (int)length);
	
	length /= sizeof(unsigned char);
	int maxValL = 0;
	int maxValR = 0;
	
	for (int i = 0; i < length; i+=2){
		int val = data[i];
		if (val > maxValL) maxValL = val;
	}

	for (int i = 1; i < length; i+=2){
		int val = data[i];
		if (val > maxValR) maxValR = val;
	}
	
	audioLevel[0] = maxValL;
	audioLevel[1] = maxValR;
	
	TBASSLIB *bass = ptr;
	SetEvent(bass->changeEventAudioLevel);
	
	//printf("DuffRecording %i %i\n", maxValL, maxValR);
	return TRUE; // continue recording
}

#endif

__stdcall unsigned int bassThreadInputFunc (void *ptr)
{
	TBASSLIB *bass = ptr;
	
	const int tid = GetCurrentThreadId();
	printf("bassThreadInputFunc start %i\n", tid);
	
	
/*	BASS_WASAPI_INFO info;
	BASS_WASAPI_GetInfo(&info);
	printf("flags %X\n", (int)info.initflags);
    printf("freq %i\n", (int)info.freq);
    printf("chans %i\n", (int)info.chans);
    printf("format %i\n", (int)info.format);
    printf("buflen %i\n", (int)info.buflen);
    printf("vmax %i\n", (int)info.volmax);
    printf("vmin %i\n", (int)info.volmin);
    printf("vstep %i\n", (int)info.volstep);
*/

	if (!BASS_RecordInit(-1)) {
		//printf("Can't initialize device\n");
		return -1;
	}
	
	if (!(bass->chan=BASS_RecordStart(48000/2, 2, MAKELONG(BASS_SAMPLE_FLOAT, 50), &DuffRecording, bass))){
		//printf("Can't start recording\n");
		return -1;
	}

	
	TBASSVISUAL *vis = &bass->vis[0];
	float fft[2048];	
	int y1 = 0;
	
	//BASS_WASAPI_Lock(1);
	while (!SHUTDOWN && bass->chan){
	  if (0){
		if (BASS_ChannelGetData(bass->chan, fft, BASS_DATA_FFT512) < 1)
			break;

		//printf("fft %f\n", fft[1]);
		//memset(vis->buffer, 0, vis->bufferLen);
			
		float y;
		float maxVal = 0.0f;
		
		for (int x = 0; x < vis->width; x++){
			float val = fft[x];
			if (val > maxVal) maxVal = val;
			
			//printf("val %f\n", val);
			if (val < 0.00010f)
				y = 0.0f;
			else
				y = sqrtf(val) * 2.5f * (const int)(vis->height-1);		// scale it (sqrt to make low values more visible)
				//y = fft[x+1]*10.0 * vis->height; // scale it (linearly)

			//printf("%f %i\n", sqrt(fftPoint), y);
			
			if (y > vis->height) y = vis->height;			// cap it
			vis->buffer[x] = (y+y1)/2;
			y1 = y;
		}
	
		//if (maxVal < 0.00010f) maxVal = 0.0f;
		//y = sqrtf(maxVal) * 2.0f;
		//y = maxVal * 5.0f;
		//printf("max fft %f\n", y);
	
		//for (int i = 0; i < 7; i++)
		//	my_memcpy(bass->visData[i], bass->visData[i+1], vis->bufferLen);
		my_memcpy(bass->visData[7], vis->buffer, vis->bufferLen);

		//printf("BASS_ChannelGetLevel %X\n", (unsigned int)BASS_ChannelGetLevel(bass->chan));

		if (!SHUTDOWN){
			if (!getIdle(bass->vp))
				lSleep(70);
			else
				lSleep(2000);
		}
	  }
	  lSleep(1000);
	}
	//BASS_WASAPI_Lock(0);
	
	//BASS_WASAPI_Stop(1);
	//BASS_WASAPI_Free();

	BASS_RecordFree();	
	
	printf("bassThreadInputFunc end %i\n", tid);
	_endthreadex(1);
	return 1;
}

/*
__stdcall unsigned int bassThreadVolChangeFunc (void *ptr)
{
	TBASSLIB *bass = ptr;
	
	bass->volumeChangeEvent = CreateEventW(NULL, 0, 0, L"LCDMiscVolumeChange");
	
	while(!SHUTDOWN){
		if (WaitForSingleObject(bass->volumeChangeEvent, INFINITE) == WAIT_OBJECT_0){
			if (!SHUTDOWN && !getIdle(bass->vp)){
				//if (pageGet(bass->vp) == PAGE_OVERLAY){
					//printf("volume %.2f\n", bass_volumeGet(bass));
					//overlayActivateOverlayResetTimer(bass->vp);
					overlaySetOverlay(bass->vp);
					setVolumeDisplay(bass->vp, bass_volumeGet(bass));
					renderSignalUpdate(bass->vp);
				//}
			}
		}
	}

	CloseHandle(bass->volumeChangeEvent);
	_endthreadex(1);
	return 1;
}*/

int bass_start (TBASSLIB *bass, const int visuals, const int vwidth, const int vheight, TVLCPLAYER *vp)
{

	int out = bass_getDefaultOut();
	bass->outEnabled = BASS_WASAPI_Init(out, 0, 2, BASS_WASAPI_AUTOFORMAT, 0.02, 0, NULL, NULL);
	bass->visuals = visuals;
	bass->vwidth = vwidth;
	bass->vheight = vheight;
	bass->vp = vp;

	

	if (0 && visuals){		
		for (int i = 0; i < 8; i++)
			bass->visData[i] = my_calloc(sizeof(int), vwidth);
	
	
		for (int i = 0; i < BASS_VISUAL_TOTAL; i++){
			bass->vis[i].bufferLen = sizeof(int) * vwidth;
			bass->vis[i].buffer = my_calloc(1, bass->vis[i].bufferLen);
			bass->vis[i].width = vwidth;
			bass->vis[i].height = vheight;
		}


		bass->hThreadInput = _beginthreadex(NULL, 0, bassThreadInputFunc, bass, CREATE_SUSPENDED, &bass->threadId);
		ResumeThread((HANDLE)bass->hThreadInput);

		bass->changeEventAudioLevel = CreateEvent(NULL, 0, 0, NULL);
		bass->hThreadAudioLevel = _beginthreadex(NULL, 0, sbRenderLevelFunc, bass, CREATE_SUSPENDED, &bass->threadIdAudioLevel);
		ResumeThread((HANDLE)bass->hThreadAudioLevel);
	}

	//bass->hThreadVolChange = _beginthreadex(NULL, 0, bassThreadVolChangeFunc, bass, CREATE_SUSPENDED, &bass->threadVolChangeId);
	//ResumeThread((HANDLE)bass->hThreadVolChange);

	return 1;
}

void bass_close (TBASSLIB *bass)
{
	/*
	for (int i = 0; i < 5; i++)
		SetEvent(bass->volumeChangeEvent);

	WaitForSingleObject((HANDLE)bass->hThreadVolChange, INFINITE);
	CloseHandle((HANDLE)bass->hThreadVolChange);
	*/
	
		
	if (bass->visuals){
		for (int i = 0; i < 5; i++)
			SetEvent(bass->changeEventAudioLevel);

		WaitForSingleObject((HANDLE)bass->hThreadAudioLevel, INFINITE);
		CloseHandle((HANDLE)bass->hThreadAudioLevel);
	
		WaitForSingleObject((HANDLE)bass->hThreadInput, INFINITE);
		CloseHandle((HANDLE)bass->hThreadInput);
	
		for (int i = 0; i < BASS_VISUAL_TOTAL; i++)
			my_free(bass->vis[i].buffer);
	
		for (int i = 0; i < 8; i++)
			my_free(bass->visData[i]);
	}

	if (bass->outEnabled){
		bass->inEnabled = 0;
		BASS_WASAPI_Free();
	}
}





#endif

