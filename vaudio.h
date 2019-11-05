

#ifdef __cplusplus
extern "C" {
#endif

int InitCom ();
void UninitCom ();
int SetupVistaVolume (HWND hWnd);
void CleanupVistaVolume ();


void SetVistaMasterVolume (const float vol);
void GetVistaMasterVolume (float *vol, int *mute);
void GetVistaMasterMute (int *mute);
void SetVistaMasterMute (const int muteState);


void master_volumeSet (const int vol);
int master_volumeGet ();
void master_muteSet (const int mute);
int master_muteGet ();






#ifdef __cplusplus
}
#endif
