

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include "ext.h"
#include "reg.h"




typedef struct {
	char *str;
	int op;
	int ipcType;		// 0:invalid, 1:PostMessage, 2:CDS
}tcmd;

static const tcmd cmds[] = {
		{"start",			CMD_START,					IPC_NONE},				// launch/run vlcstream
		{"help,list",		CMD_HELP,					IPC_NONE},
		{"quit",			CMD_SHUTDOWN,				IPC_POSTMSG},
		{"play",			CMD_MEDIA_PLAY,				IPC_CDS},			// var = "playlist uid:track". eg; 41D3:2 (first track is 1, 2'nd is 2, etc..)
		{"pause",			CMD_MEDIA_PAUSE,			IPC_POSTMSG},
		{"playpause,pp",	CMD_MEDIA_PLAYPAUSE,		IPC_POSTMSG},		// play toggle (play when paused else pause if playing)
		{"stop",			CMD_MEDIA_STOP,				IPC_POSTMSG},
		{"next",			CMD_MEDIA_NEXT_TRACK,		IPC_POSTMSG},
		{"back,prev",		CMD_MEDIA_PREV_TRACK,		IPC_POSTMSG},
		{"up",				CMD_MEDIA_VOL_UP,			IPC_POSTMSG},		// step up
		{"down",			CMD_MEDIA_VOL_DOWN,			IPC_POSTMSG},		// step down
		{"random",			CMD_MEDIA_RANDOM,			IPC_CDS},
		{"wup",				CMD_WIN_VOL_UP,				IPC_POSTMSG},
		{"wdown,wdn",		CMD_WIN_VOL_DOWN,			IPC_POSTMSG},
		{"idle",			CMD_IDLE,					IPC_POSTMSG},
		{"wake",			CMD_WAKE,					IPC_POSTMSG},
		{"clock,clk",		CMD_CLOCK,					IPC_POSTMSG},
		{"snapshot,ss",		CMD_SNAPSHOT,				IPC_POSTMSG},
		{"resync",			CMD_RESYNC,					IPC_POSTMSG},
		{"goto",			CMD_PLAYLIST_GOTO,			IPC_CDS},			// if possible, display this playlist (uid)
		{"volume,vol",		CMD_MEDIA_VOL,				IPC_CDS},			// set playback volume to n percent
		{"time",			CMD_MEDIA_TIME,				IPC_CDS},			// set playback position by time (hr:min:sec). eg; 1:43 (1 min 43 sec)
		{"position,pos",	CMD_MEDIA_POSITION,			IPC_CDS},			// set playback position by percent. eg; 85.4
		{"sbdk,dk",			CMD_SBDK_PRESS,				IPC_CDS},			// simulate a switchblade dynamic key press (1 through 10)
		{"eqband",			CMD_EQ_BAND_SET,			IPC_CDS},			// set eq band gain. eg: 4:2.6 for 600hz gain of 2.6, eg; 0:12 set Preamp (0:) gain to 12.0
		{"eqprofile",		CMD_EQ_PROFILE_SET,			IPC_CDS},			// select profile by name. eg: classical
		{"setname,rename",	CMD_PLAYLIST_NAME,			IPC_CDS},			// set a title eg; "setname uid:track". eg; 41D3:2 (first track is 1, 2'nd is 2, etc..)
		{"dvd,nav",			CMD_MEDIA_DVDNAV,			IPC_CDS},			// navigate a dvd menu. (up, down, left, right and go/start/play/enter)
		{"chapter,chap",	CMD_MEDIA_CHAPTER,			IPC_CDS},
		{"title",			CMD_MEDIA_TITLE,			IPC_CDS},
		{"morbid",			CMD_BOT_MORBID,				IPC_POSTMSG},
		{"bofh",			CMD_BOT_BOFH,				IPC_POSTMSG},
		{"dubya",			CMD_BOT_DUBYA,				IPC_POSTMSG},
		{"fact",			CMD_BOT_FACTS,				IPC_POSTMSG},
		{"copy",			CMD_INPUT_COPY,				IPC_POSTMSG},		// (TODO) copy keypad text to windows clipboard
		{"paste",			CMD_INPUT_PASTE,			IPC_POSTMSG},		// (TODO) copy clipboard text, if any, to keypad buffer
		{"flush",			CMD_FLUSH,					IPC_POSTMSG},
		{"tbupdate",		CMD_TASKBAR_TITLE_UPDATE,	IPC_POSTMSG},
		{"plymvqup",		CMD_PLAYLIST_MVQ_UP,		IPC_POSTMSG},		// changed queued playlist up one level (its' parent)
		{"plymvqleft",		CMD_PLAYLIST_MVQ_LEFT,		IPC_POSTMSG},		// change to left of current
		{"plymvqright",		CMD_PLAYLIST_MVQ_RIGHT, 	IPC_POSTMSG}		// change to right of current
};


static inline int startProgram (wchar_t *path)
{
	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));
	
	STARTUPINFOW si;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	
	return CreateProcessW(NULL, path, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
}

static inline wchar_t *getInstallPath (wchar_t *buffer, const size_t blen)
{
	HKEY hKey = 0;
	DWORD type = REG_SZ;
	wchar_t *ret = NULL;
	
	if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, LIBMYLCDVLCSTREAM, 0, KEY_READ, &hKey))){
		if ((ERROR_SUCCESS == RegQueryValueExW(hKey, L"Path", 0, &type, (LPBYTE)buffer, (PDWORD)&blen)))
			ret = buffer;
		RegCloseKey(hKey);
	}	
	return ret;
}

static inline HANDLE getWindowHandle ()
{
	return FindWindowA(NAME_WINMSG, NULL);
}

static inline void startVLCStream ()
{
	wchar_t path[MAX_PATH+1];
	
	if (getInstallPath(path, sizeof(path)-1))
		startProgram(path);
}

static inline unsigned int getHash (const char *str)
{
	return generateHash(str, strlen(str)+1);
}

static inline void cds_send (HWND hWin, const int msg, const void *varStr)
{
	COPYDATASTRUCT cds;
	cds.dwData = WM_CDS_CMDCTRL;
	cds.cbData = msg;
	cds.lpData = (void*)varStr;
	unsigned int hash = 0;
	
	if (cds.lpData)
		hash = getHash(cds.lpData);
	SendMessage(hWin, WM_COPYDATA, (WPARAM)hash, (LPARAM)&cds);
}

static inline void sendCmd (HANDLE hWnd, const int op, const int var)
{
	PostMessage(hWnd, WM_EXTCMD, op, var);
}

static inline void dumpCmds ()
{
	printf("Commands available:\n");
	
	const int tCmds = sizeof(cmds)/sizeof(tcmd);
	for (int i = 0; i < tCmds; i++)
		printf("%s\n",cmds[i].str);
	fflush(stdout);
}		
	
static inline void initCmd (const int ipcType, const int op, const char *var)
{
	if (ipcType == IPC_NONE){
		if (op == CMD_START)
			startVLCStream();
		else if (op == CMD_HELP)
			dumpCmds();

	}else if (ipcType == IPC_CDS){
		HANDLE hWnd = getWindowHandle();
		if (hWnd)
			cds_send(hWnd, op, var);
			
	}else if (ipcType == IPC_POSTMSG){
		HANDLE hWnd = getWindowHandle();
		if (hWnd)
			sendCmd(hWnd, op, 0);
	}
}

int main (const int argc, const char *argv[])
{

	if (argc < 2){
		HANDLE hWnd = getWindowHandle();
		if (hWnd)
			PostMessage(hWnd, WM_HOOKPOINTER, 0, 0);
	}else{
		for (int c = 1; c < argc; c++){
			int found = 0;
			const char *cmd = argv[c];
			//printf("cmd: %i #%s#\n", c, cmd);
			
			const int tCmds = sizeof(cmds)/sizeof(tcmd);
        	
			for (int i = 0; i < tCmds && !found; i++){
				char *str = strdup(cmds[i].str);
				if (!str) return 0;
				
				char *rcmd = strtok(str, " ,");
				while(rcmd && !found){
					//printf("%i #%s#\n", i, rcmd);
        	
					if (!stricmp(cmd, rcmd)){
						if (argc == 2){
							initCmd(cmds[i].ipcType, cmds[i].op, NULL);
						}else{
							char var[8192] = {0};
							strcat(var, argv[2]);
							
							for (int i = 3; i < argc; i++){
								strcat(var, "<|>");
								strcat(var, argv[i]);
							}
							initCmd(cmds[i].ipcType, cmds[i].op, var);
						}
        	
						//printf("command sent: %s\n", str);
						free(str);
						found = 1;
					}
					rcmd = strtok(NULL, " ,");
				}
				free(str);
			}
			//if (!found)
			//	printf("unknown command '%s'\n", cmd);
		}
	}

	return 0;		
}
