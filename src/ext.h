
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


#ifndef _EXT_H_
#define _EXT_H_


#include "crc.h"



#define NAME_INSTANCEEVENT		"VLCSTREAM_INSTANCE_002"
#define NAME_WINMSG				"VLCSTREAM_MMCEMSGWINDOW"

#define WM_VLCSTREAMUSER		(WM_USER+2000)
#define WM_TOUCHIN				(WM_VLCSTREAMUSER+0)
#define WM_MCTRLNEXTTRACK		(WM_VLCSTREAMUSER+10)
#define WM_WAKEUP				(WM_VLCSTREAMUSER+20)
#define WM_ADDTRACKINIT			(WM_VLCSTREAMUSER+30)
#define WM_ADDTRACKCHAR			(WM_VLCSTREAMUSER+31)

#define WM_CDS_ADDTRACK			(WM_VLCSTREAMUSER+40)		// add track/path to root
#define WM_CDS_ADDTRACK_DSP		(WM_VLCSTREAMUSER+41)		// to whichever playlist is displayed in Album (shelf) view
#define WM_CDS_ADDTRACK_PLY		(WM_VLCSTREAMUSER+42)		// to queued (playing) playlist
#define WM_CDS_CMDCTRL			(WM_VLCSTREAMUSER+43)

#define WM_MWHEELFORWARD		(WM_VLCSTREAMUSER+50)
#define WM_MWHEELBACK			(WM_VLCSTREAMUSER+51)
#define WM_MWHEEL_FORWARD		(WM_MWHEELFORWARD)
#define WM_MWHEEL_BACK			(WM_MWHEELBACK)
#define WM_MWHEEL_LEFT			(WM_VLCSTREAMUSER+52)
#define WM_MWHEEL_RIGHT			(WM_VLCSTREAMUSER+53)
#define WM_MEDIA_CHANGE			(WM_VLCSTREAMUSER+60)
#define WM_VAUDIO_DEV_CHANGE	(WM_VLCSTREAMUSER+70)
#define WM_VAUDIO_VOL_CHANGE	(WM_VLCSTREAMUSER+71)
#define WM_HOOKPOINTER			(WM_VLCSTREAMUSER+80)
#define WM_EXTCMD				(WM_VLCSTREAMUSER+90)
#define WM_TRAY_HOOK			(WM_VLCSTREAMUSER+100)
#define WM_SHELL_HOOK			(WM_VLCSTREAMUSER+101)
#define WM_SYSTRAY				(WM_VLCSTREAMUSER+200)
#define WM_TRACKPLAYNOTIFY		(WM_VLCSTREAMUSER+201)
#define WM_TRACKTIMESTAMPNOTIFY	(WM_VLCSTREAMUSER+202)
#define WM_TRACKCHAPTERUPDATE	(WM_VLCSTREAMUSER+203)

#define WM_MM					(WM_VLCSTREAMUSER+2000)
#define WM_DD_MOUSEACTIVATE  	(WM_MM+WM_MOUSEACTIVATE)
#define WM_DD_MOUSEMOVE	    	(WM_MM+WM_MOUSEMOVE)
#define WM_DD_LBUTTONDOWN	   	(WM_MM+WM_LBUTTONDOWN)
#define WM_DD_LBUTTONUP	    	(WM_MM+WM_LBUTTONUP)
#define WM_DD_LBUTTONDBLCLK   	(WM_MM+WM_LBUTTONDBLCLK)
#define WM_DD_RBUTTONDOWN	   	(WM_MM+WM_RBUTTONDOWN)
#define WM_DD_RBUTTONUP	    	(WM_MM+WM_RBUTTONUP)
#define WM_DD_RBUTTONDBLCLK   	(WM_MM+WM_RBUTTONDBLCLK)
#define WM_DD_MBUTTONDOWN	   	(WM_MM+WM_MBUTTONDOWN)
#define WM_DD_MBUTTONUP	    	(WM_MM+WM_MBUTTONUP)
#define WM_DD_MBUTTONDBLCLK   	(WM_MM+WM_MBUTTONDBLCLK)
#define WM_DD_MOUSEWHEEL	   	(WM_MM+WM_MOUSEWHEEL)
#define WM_DD_MOUSEHWHEEL	   	(WM_MM+WM_MOUSEHWHEEL)
#define WM_DD_MOUSEHOVER	   	(WM_MM+WM_MOUSEHOVER)
#define WM_DD_MOUSELEAVE	   	(WM_MM+WM_MOUSELEAVE)
#define WM_DD_NCMOUSEHOVER		(WM_MM+WM_NCMOUSEHOVER)
#define WM_DD_NCMOUSELEAVE		(WM_MM+WM_NCMOUSELEAVE)
#define WM_DD_CHARDOWN			(WM_MM+WM_CHAR)
#define WM_DD_KEYDOWN			(WM_MM+WM_KEYDOWN)
#define WM_DD_CLOSE				(WM_MM+WM_CLOSE)


#define IPC_NONE		0
#define IPC_POSTMSG		1
#define IPC_CDS			2

enum _extcmd {
	CMD_HELP = 10,
	CMD_START = 100,
	CMD_SHUTDOWN,
	CMD_MEDIA_PLAY,
	CMD_MEDIA_PAUSE,
	CMD_MEDIA_PLAYPAUSE,
	CMD_MEDIA_STOP,
	CMD_MEDIA_NEXT_TRACK,
	CMD_MEDIA_PREV_TRACK,
	CMD_MEDIA_VOL_UP,
	CMD_MEDIA_VOL_DOWN,
	CMD_WIN_VOL_UP,
	CMD_WIN_VOL_DOWN,
	CMD_IDLE,
	CMD_WAKE,
	CMD_CLOCK,
	CMD_SNAPSHOT,
	CMD_RESYNC,
	CMD_PLAYLIST_GOTO,
	CMD_MEDIA_VOL,
	CMD_INPUT_COPY,				// TODO
	CMD_INPUT_PASTE,			// TODO
	CMD_MEDIA_RANDOM,
	CMD_MEDIA_TIME,
	CMD_MEDIA_POSITION,
	CMD_SBDK_PRESS,
	CMD_EQ_BAND_SET,
	CMD_EQ_PROFILE_SET,
	CMD_PLAYLIST_NAME,
	CMD_MEDIA_DVDNAV,
	CMD_MEDIA_CHAPTER,
	CMD_MEDIA_TITLE,
	CMD_BOT_MORBID,
	CMD_BOT_BOFH,
	CMD_BOT_DUBYA,
	CMD_BOT_FACTS,
	CMD_FLUSH,
	CMD_TASKBAR_TITLE_UPDATE,
	CMD_PLAYLIST_MVQ_UP,			// changed queued playlist up one level
	CMD_PLAYLIST_MVQ_LEFT,			// changed queued playlist to left of current, same level if possible
	CMD_PLAYLIST_MVQ_RIGHT,			// changed queued playlist to right of current, same level if possible
};



#ifdef PLAYER_NAME

int extCommandFunc (TVLCPLAYER *vp, const int op, const int varInt, const char *varStr1, const char *varStr2);
void extReceiveCdsCmd (TVLCPLAYER *vp, const int op, const char *var8);
void extReceiveCdsPath (TVLCPLAYER *vp, const int to, const unsigned int hash, const char *pathIn8, const int pathInLen);

#endif






#endif

