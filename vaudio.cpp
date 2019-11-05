

#define INITGUID     1
#define USE___UUIDOF 0

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wtypes.h>

#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include "common.h"




static HWND ghWnd;
static int ComCount = 0;

int InitCom ()
{
	if (!(ComCount++)){
		HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
		return (hr == S_OK || hr == S_FALSE);
	}
	return 1;
}

void KillCom ()
{
	if (ComCount){
		ComCount = 0;
		CoUninitialize();
	}
}

void UninitCom ()
{
	if (!--ComCount)
		CoUninitialize();
}

class CMMNotificationClient : public IMMNotificationClient, public IAudioEndpointVolumeCallback{
    LONG refs;
public:
	CMMNotificationClient() : refs(1){
	}
	~CMMNotificationClient(){}

	// IUnknown methods -- AddRef, Release, and QueryInterface

	ULONG STDMETHODCALLTYPE AddRef(){
		return InterlockedIncrement(&refs);
	}

	ULONG STDMETHODCALLTYPE Release (){
		LONG refs = InterlockedDecrement(&this->refs);
		if (!refs){
			//delete this;
		}
		return refs;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, VOID **ppvInterface){
		if (IID_IUnknown == riid){
			AddRef();
			*ppvInterface = (IMMNotificationClient*)this;
		}else if (IID_IMMNotificationClient == riid){
	 		AddRef();
			*ppvInterface = (IMMNotificationClient*)this;
		}else if (IID_IAudioEndpointVolumeCallback == riid){
			AddRef();
			*ppvInterface = (IAudioEndpointVolumeCallback*)this;
		}else{
			*ppvInterface = NULL;
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnNotify (AUDIO_VOLUME_NOTIFICATION_DATA *Notify){
		float vol = Notify->fMasterVolume;
		int mute = (int)Notify->bMuted;
		//GetVistaMasterVolume(&vol, &mute);
		PostMessage(ghWnd, WM_VAUDIO_VOL_CHANGE, vol*1000.0f, mute);
		return S_OK;
	}


	// Callback methods for device-event notifications.

	HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged (EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId){
		PostMessage(ghWnd, WM_VAUDIO_DEV_CHANGE, 0, 0);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceAdded (LPCWSTR pwstrDeviceId){
		PostMessage(ghWnd, WM_VAUDIO_DEV_CHANGE, 0, 0);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceRemoved (LPCWSTR pwstrDeviceId){
		PostMessage(ghWnd, WM_VAUDIO_DEV_CHANGE, 0, 0);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceStateChanged (LPCWSTR pwstrDeviceId, DWORD dwNewState){
		PostMessage(ghWnd, WM_VAUDIO_DEV_CHANGE, 0, 0);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnPropertyValueChanged (LPCWSTR pwstrDeviceId, const PROPERTYKEY key){
		PostMessage(ghWnd, WM_VAUDIO_DEV_CHANGE, 0, 0);
		return S_OK;
	}
};

static CMMNotificationClient *client = 0;
static IMMDeviceEnumerator *deviceEnumerator = 0;
static IMMDevice *defaultDevice = 0;
static IAudioEndpointVolume *endpointVolume = 0;

int SetupVistaVolume (HWND hWnd)
{
	HRESULT hr;
	static char com = 0;
	ghWnd = hWnd;

	if (!com){
		if (!InitCom()) return 0;
		com = 1;
	}

	if (!client)
		client = new CMMNotificationClient();

	if (!deviceEnumerator){
		hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, IID_IMMDeviceEnumerator, (LPVOID *)&deviceEnumerator);
		if (hr != S_OK){
			deviceEnumerator = 0;
			//delete client;
			client = 0;
			UninitCom();
			com = 0;
			return 0;
		}
		hr = deviceEnumerator->RegisterEndpointNotificationCallback(client);
		if (hr != S_OK){
			deviceEnumerator->Release();
			deviceEnumerator = 0;
			return 0;
		}
	}
	if (defaultDevice){
		defaultDevice->Release();
		defaultDevice = 0;
	}
	if (endpointVolume){
		endpointVolume->UnregisterControlChangeNotify(client);
		endpointVolume->Release();
		endpointVolume = 0;
	}
	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	if (hr != S_OK){
		defaultDevice = 0;
		return 0;
	}

	hr = defaultDevice->Activate(IID_IAudioEndpointVolume, CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
	if (hr != S_OK){
		endpointVolume = 0;
		return 0;
	}
	hr = endpointVolume->RegisterControlChangeNotify(client);
	if (hr != S_OK){
		endpointVolume->Release();
		endpointVolume = 0;
		return 0;
	}

	return 1;
}

void CleanupVistaVolume ()
{
	if (endpointVolume){
		endpointVolume->UnregisterControlChangeNotify(client);
		endpointVolume->Release();
		endpointVolume = 0;
	}
	if (defaultDevice){
		defaultDevice->Release();
		defaultDevice = 0;
	}
	if (deviceEnumerator){
		deviceEnumerator->UnregisterEndpointNotificationCallback(client);
		deviceEnumerator->Release();
		deviceEnumerator = 0;
	}
	if (client){
		client->Release();
		client = 0;
	}
}

void SetVistaMasterMute (const int mute)
{
	if (!endpointVolume)
		return;

	endpointVolume->SetMute(mute, 0);
}

void GetVistaMasterMute (int *mute)
{
	if (!endpointVolume)
		return;

	endpointVolume->GetMute(mute);
	//IAudioEndpointVolume_GetMute(endpointVolume, mute);
}

void SetVistaMasterVolume (const float vol)
{
	if (!endpointVolume)
		return;

	endpointVolume->SetMasterVolumeLevelScalar(vol, 0);
}

void GetVistaMasterVolume (float *vol, int *mute)
{
	if (!endpointVolume)
		return;

	if (vol) endpointVolume->GetMasterVolumeLevelScalar(vol);
	if (mute) endpointVolume->GetMute(mute);
}

int master_volumeGet ()
{
	float vol = 0.0f;
	GetVistaMasterVolume(&vol, NULL);
	return (int)(vol*100.0f);
}

void master_volumeSet (const int vol)
{
	SetVistaMasterVolume((float)vol/100.0f);
}

void master_muteSet (const int mute)
{
	SetVistaMasterMute(mute);
}

int master_muteGet ()
{
	int mute = 0;
	GetVistaMasterVolume(NULL, &mute);
	return mute;
}

