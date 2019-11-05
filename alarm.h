
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

#ifndef _ALARM_H_
#define _ALARM_H_

#ifndef _TIME64_T_DEFINED
#define _TIME64_T_DEFINED
  typedef __int64 __time64_t;
#endif

#define time64_t __time64_t



#define ALARM_UNSET				0
#define ALARM_ENABLED			1
#define ALARM_DISABLED			2
#define ALARM_FIRED				3

#define ALARM_FIREWINDOW_LOW	(-(10.0))
#define ALARM_FIREWINDOW_HIGH	(10.0)		// fire alarm only when within this time window, in seconds
#define ALARM_REENABLE_WINDOW	(5*60*1000)	// prevent double triggering of alarm. must be greated than sum of _LOW and _HIGH
#define ALARM_MAX				16
#define ALARM_BASEID			100
#define ALARM_PERIOD			5000		// time between 'ready to fire' service check
#define ALARM_MAXTIME			1439		// 1439min is 23:59
#define ALARM_MAXTIMEDIGITS		5
#define ALARM_CHARSPACE			12			// added space between rendered digits, in pixels
#define	ALARM_VERTICALPOS		90			// alarm is centre justified, this will place it on the vertical


#define ALARM_ACT_INVALID		0x0000
#define ALARM_ACT_STARTTASK		0x0001		// run an external application
#define ALARM_ACT_ENDTASK		0x0002		// kill a running task via pid or its module path
#define ALARM_ACT_BEEP			0x0004		// sound the internal beeper
#define ALARM_ACT_PLYSTART		0x0008		// start media playback. provide either playlist title or playlist UID and track # (first track is 1)
#define ALARM_ACT_PLYSTOP		0x0010		// halt playback if started, otherwise do nothing
#define ALARM_ACT_FLASH			0x0020		// flash the display (alternate between 2 colours)
#define ALARM_ACT_CB			0x0040		// function callback
#define ALARM_ACT_MSG			0x0080		// send alarm fired message to all/specified page(s)
#define ALARM_ACT_IGNORE		0x0100		// allow alarm to run its course but don't fire (useful for testing)
#define ALARM_ACT_EXIT			0x0200		// shutdown application
#define ALARM_ACT_MASK			0xFFFF

#define ALARM_FIRE_SINGLESHOT	0
#define ALARM_FIRE_DAILY		1
#define ALARM_FIRE_WEEKLY		2
#define ALARM_FIRE_DEFAULT		ALARM_FIRE_SINGLESHOT

#define ALARM_DAY_SUN			0x01
#define ALARM_DAY_MON			0x02
#define ALARM_DAY_TUE			0x04
#define ALARM_DAY_WED			0x08
#define ALARM_DAY_THR			0x10
#define ALARM_DAY_FRI			0x20
#define ALARM_DAY_SAT			0x40


#define ALARM_BTN_ENABLE		0			// enabled/disabled
#define ALARM_BTN_DAILY			1			// daily/weekly
#define ALARM_BTN_TOTAL			2



enum _btnsAlarm
{
	BTN_ALM_DAYS,
	BTN_ALM_TOTAL
};

enum _btnAlarm
{
	BTN_ALM_DAY_SUN,
	BTN_ALM_DAY_MON,
	BTN_ALM_DAY_TUE,
	BTN_ALM_DAY_WED,
	BTN_ALM_DAY_THR,
	BTN_ALM_DAY_FRI,
	BTN_ALM_DAY_SAT,
	BTN_ALM_DAY_TOTAL
};




typedef void (*TAlarm_cb_t) (int alarmId, int64_t dataInt64, void *dataPtr1, void *dataPtr2);

typedef struct {
	char *path;			// utf8 path
	int pid;			// process id
}alarm_starttask;

typedef struct {
	char *module;		// kill all processes with this module path
	int pid;			// endtask only this process
}alarm_endtask;

typedef struct {
	struct {
		int pause;		// ms pause before sounding beeper
		int pitch;
		int duration;
	}config[16];

	int cfgTotal;		// total above
	int repeatN;		// repeat N times
}alarm_beep;

typedef struct {
	char *title;		// if set, search for playlist by title
	uint64_t uid;		// otherwise use its id ref
	int track;			// first track is 1
	int volume;			// play media at this volume then revert to original. -1: don't modify volume
	//int mode;			// repeat this track, stop on finish or continue to next
}alarm_plystart;

typedef struct {
	int goIdle;			// if set will enter idle mode immediatly 
}alarm_plystop;

typedef struct {
	int colours[2];
	int period;			// ms time each colour is displayed for
	int length;			// flash for this length of time (ms)
}alarm_flash;

typedef struct {
	int pageId;			// id:send msg (PAGE_MSG_ALARM_FIRED) to this page only, otherwise 0: send to all activated pages
	
	int64_t dataInt2;	// alarm id is sent over dataInt1
	void *dataPtr;
}alarm_msg;

typedef struct {
	TAlarm_cb_t cb;
	
	int64_t dataInt1;
	void *dataPtr1;
	void *dataPtr2;
}alarm_callback;

typedef struct {
	int stub;
}alarm_ignore;
	
typedef struct {
	int stub;
}alarm_exit;

typedef struct {
	int state;				// unset, enabled, disabled. alarm is unset until set, 
							// at which point it is then either enabled (ready to fire) or disabled (do not fire)
							// when fired, state will be set to fired
	int id;
	char timeStr[ALARM_MAXTIMEDIGITS+1];		// for visual reference
	int action;				// what to do when fired (ALARM_ACT_)
	int keepAlive;			// retain alarm instance after firing, otherwise its auto-deleted
	int wakeOnIdle;			// when triggering and is idle, leave idle if set
	
	struct{
		int period;			// when singleshot/daily/weekly 
		time64_t time64;	// alarm set for this local time
		int when;			// ALARM_DAY_XXX: 0x01=Sun, 0x02=Mon, 0x04=Tue, etc..)
		time64_t tFired;	// time when triggered
	}trigger;
	
	alarm_ignore	ignore;
	alarm_starttask	starttask;
	alarm_endtask	endtask;
	alarm_beep		beep;
	alarm_plystart	plystart;
	alarm_plystop	plystop;
	alarm_flash		flash;
	alarm_callback	callback;
	alarm_msg		msg;
	alarm_exit		exit;
}TALARMTIMER;

typedef struct {
	TPAGE2COM *com;
	int idSrc;

	TALARMTIMER	*alm[ALARM_MAX];

	struct {				// deals with alarm rendering only
		struct {
			TCCBUTTONS *btns[BTN_ALM_TOTAL];	// weekly (days of week)
		}cfg;
		
		TLABELSTR *time[ALARM_MAXTIMEDIGITS];
		int charSpace;
		int frameWidth;		// digit display area
		int strTimeInt;		// time according to UI string/display time, in minutes
		int aid;			// alarm id this time is being set/edited for
		int posY;			// alarm is centre justified, this will place it on the vertical
		
		TLABELSTR *btns[ALARM_BTN_TOTAL];	// enabled/disable and period
	}ui;
	
	
	struct {				// describes currently active alarm. there shall only be one
		int state;			// 1:alarm time is valid, ready to use. 0:disabled or incomplete
		int aid;
	}active;
}TALARM;


int page_alarmCb (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

int alarmCreate (TALARM *alarm, const int period, const time64_t t64, const int when);
void alarmRemove (TALARM *alarm, const int id);

// is the default UI alarm enabled or nor
int alarmUiGetStatus (TALARM *alarm);
int alarmUiGetAlarmTime (TALARM *alarm, char *buffer);

//int alarmActionSet (TALARM *alarm, const int id, const int action, void *alarm_act);

//int alarmEnable (TALARM *alarm, const int id);
//void alarmDisable (TALARM *alarm, const int id);


void timer_alarm (TVLCPLAYER *vp);

const char *alarmCfgModeToPeriod (const int mode);
const char *alarmCfgGetPeriodStr (TALARM *alarm);
int alarmCfgGetWhen (TALARM *alarm);

int alarmActionGetDesc (TALARM *alarm, const int id, const int action, void *desc);
int alarmActionGet (TALARM *alarm, const int id);

#endif

