// libmylcd - http://mylcd.sourceforge.net/
// An LCD framebuffer library
// Michael McElligott
// okio@users.sourceforge.net
//
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


// all alarms are processed and fired via render thread [only]
// input events which impact alarm state(s) should be redirected to render thread


//#define ALARM_INITIALTIME		"10:25"


typedef struct {
	int yRenderOffset;
	char *str;
	int colFore;
	int colBack;
	int colOutline;
}tstrcol;

static const tstrcol strEnabled[] = {
	{0, "Disabled", 0xCA3A3A, 0x7FFF80, COL_WHITE},
	{0, "Enabled", COL_WHITE, 0x7FFF80, COL_BLUE_SEA_TINT}
};

static const tstrcol strPeriod[] = {
	{-6,"Once",  COL_CYAN, 0x7FFF80, COL_WHITE},
	{0, "Daily", COL_CYAN, 0x7FFF80, COL_WHITE},
	{0, "Weekly",COL_CYAN, 0x7FFF80, COL_WHITE}
};



static inline int alarmTimeToString (time64_t t64, char *buffer, const size_t bufferLen)
{
	*buffer = 0;

	int minutes = (t64/60) % 60;
	int hours = (int)((int)t64/60/60.0f);
	if (t64 && t64%60 == 0 && !hours) minutes = (int)t64/60;

	//printf("alarmTimeToString %i, %i %i\n", (int)t64, hours, minutes);
	int ret = _snprintf(buffer, bufferLen+1, "%.02i:%02i", hours, minutes);
	//printf("alarmTimeToString #%s# %.02i:%02i\n", buffer, hours, minutes);
	return ret;
}


static inline TALARMTIMER *alarmIdToAlm (TALARM *alarm, const int id)
{
	for (int i = 0; i < ALARM_MAX; i++){
		if (alarm->alm[i] && alarm->alm[i]->id == id)
			return alarm->alm[i];
	}
	return NULL;
}

static inline int alarmGetWhen (TALARM *alarm, const int id)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (alm)
		return alm->trigger.when;

	return 0;
}

const char *alarmCfgModeToPeriod (const int mode)
{
	if (mode == ALARM_FIRE_SINGLESHOT)
		return strPeriod[ALARM_FIRE_SINGLESHOT].str;
	else if (mode == ALARM_FIRE_DAILY)
		return strPeriod[ALARM_FIRE_DAILY].str;
	else if (mode == ALARM_FIRE_WEEKLY)
		return strPeriod[ALARM_FIRE_WEEKLY].str;
	else
		return strPeriod[ALARM_FIRE_DEFAULT].str;
}

int alarmCfgGetWhen (TALARM *alarm)
{
	return alarmGetWhen(alarm, alarm->active.aid);
}

static inline int alarmCfgPeriodToMode (const char *period)
{
	if (stristr(period, strPeriod[ALARM_FIRE_SINGLESHOT].str))
		return ALARM_FIRE_SINGLESHOT;
	else if (stristr(period, strPeriod[ALARM_FIRE_DAILY].str))
		return ALARM_FIRE_DAILY;
	else if (stristr(period, strPeriod[ALARM_FIRE_WEEKLY].str))
		return ALARM_FIRE_WEEKLY;
	return ALARM_FIRE_DEFAULT;
}

const char *alarmCfgGetPeriodStr (TALARM *alarm)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, alarm->active.aid);
	if (alm){
		//printf("alarmCfgGetPeriodStr: %i '%s'\n", alarm->active.aid, alarmCfgModeToPeriod(alm->trigger.period));
		return alarmCfgModeToPeriod(alm->trigger.period);
	}
		
	return alarmCfgModeToPeriod(ALARM_FIRE_DEFAULT);
}

static inline void alarmDrawDateDay (TFRAME *frame, const struct tm *tdate, int x, int y, const int font)
{
	char buffer[32];
	strftime(buffer, sizeof(buffer)-1, "  %A", tdate);
	
	int blurOp = LTR_BLUR5;
	int flags = PF_IGNOREFORMATTING;

	TLPRINTR rt;
	memset(&rt, 0, sizeof(TLPRINTR));

	if (x == -1){
		x = 0;
		flags |= PF_LEFTJUSTIFY|PF_CLIPWRAP|PF_FORCEAUTOWIDTH;
	}

	lSetForegroundColour(frame->hw, COL_BLACK);
	lSetRenderEffect(frame->hw, blurOp);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, COL_PURPLE_GLOW);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_RADIUS, 6);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_SETTOP, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_ALPHA, 700);
	

	rt.sx = x;
	rt.ex = x;
	rt.bx2 = frame->width/2;
	rt.sy = y;
	lPrintEx(frame, &rt, font, flags, LPRT_CPY, buffer);

	blurOp = LTR_BLUR4;
	lSetRenderEffect(frame->hw, blurOp);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, COL_WHITE);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_RADIUS, 3);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_SETTOP, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_ALPHA, 850);

	rt.sx = x;
	rt.ex = x;
	rt.bx2 = frame->width/2;
	rt.sy = y;
	lPrintEx(frame, &rt, font, flags, LPRT_CPY, buffer);
}

static inline void alarmDrawDateMonth (TFRAME *frame, const struct tm *tdate, int x, int y, const int font)
{
	char buffer[32];
	strftime(buffer, sizeof(buffer)-1, "%d/%m/%y\t", tdate);
	
	int blurOp = LTR_BLUR5;
	int flags = PF_IGNOREFORMATTING;

	TLPRINTR rt;
	memset(&rt, 0, sizeof(TLPRINTR));

	if (x == -1){
		x = 0;
		flags |= PF_RIGHTJUSTIFY|PF_CLIPWRAP|PF_FORCEAUTOWIDTH;
	}

	lSetForegroundColour(frame->hw, COL_BLACK);
	lSetRenderEffect(frame->hw, blurOp);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, COL_PURPLE_GLOW);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_RADIUS, 6);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_SETTOP, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_ALPHA, 700);
	

	rt.sx = x;
	rt.ex = x;
	rt.bx1 = frame->width/2;
	rt.sy = y;
	lPrintEx(frame, &rt, font, flags, LPRT_CPY, buffer);

	blurOp = LTR_BLUR4;
	lSetRenderEffect(frame->hw, blurOp);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, COL_WHITE);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_RADIUS, 3);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_SETTOP, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_ALPHA, 850);

	rt.sx = x;
	rt.ex = x;
	rt.bx1 = frame->width/2;
	rt.sy = y;
	lPrintEx(frame, &rt, font, flags, LPRT_CPY, buffer);
}

static inline void alarmDrawDateTime (TFRAME *frame, const struct tm *tdate, int x, int y, const int font)
{
	char buffer[32];
	_snprintf(buffer, 15, "%.2i:%.2i:%.2i", tdate->tm_hour, tdate->tm_min, tdate->tm_sec);
	//printf("alarmDrawDateTime #%s#\n", buffer);


	int blurOp = LTR_BLUR5;
	int flags = PF_IGNOREFORMATTING;

	TLPRINTR rt;
	memset(&rt, 0, sizeof(TLPRINTR));

	if (x == -1){
		x = abs(x);
		flags |= PF_MIDDLEJUSTIFY|PF_CLIPWRAP;
	}

	lSetForegroundColour(frame->hw, COL_BLACK);
	lSetRenderEffect(frame->hw, blurOp);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, COL_BLUE);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_RADIUS, 5);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_SETTOP, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_ALPHA, 800);

	rt.sx = x;
	rt.ex = x;
	//rt.bx2 = frame->width/2;
	rt.sy = y;
	lPrintEx(frame, &rt, font, flags, LPRT_CPY, buffer);

	blurOp = LTR_BLUR4;
	lSetRenderEffect(frame->hw, blurOp);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, COL_WHITE);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_RADIUS, 4);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_SETTOP, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_ALPHA, 900);

	rt.sx = x;
	rt.ex = x;
	//rt.bx2 = frame->width/2;
	rt.sy = y;
	lPrintEx(frame, &rt, font, flags, LPRT_CPY, buffer);
}


static inline void alarm_action_startTask (TALARM *alarm, const int id, alarm_starttask *act)
{
	if (act->path){
		wchar_t *path = converttow(act->path);
		if (path){
			act->pid = processCreateW(path);
			my_free(path);
		}
	}
}

static inline void alarm_action_endTask (TALARM *alarm, const int id, alarm_endtask *act)
{
	int pid = 0;

	if (act->module){
		wchar_t *path = converttow(act->module);
		if (path){
			pid = processGetPidW(path);
			my_free(path);
		}
	}
	if (!pid && act->pid)
		pid = act->pid;
				
	if (pid){
		processKillWindow(pid);
		lSleep(1);
		processKillOpenThread(pid);
		lSleep(1);
		processKillRemoteThread(pid);
	}
}

static inline void alarm_action_beep (TALARM *alarm, const int id, alarm_beep *act)
{
	for (int n = 0; n < act->repeatN+1; n++){
		for (int i = 0; i < act->cfgTotal && act->config[i].pitch; i++){
			Beep(act->config[i].pitch, act->config[i].duration);
			if (act->config[i].pause) lSleep(act->config[i].pause);
		}
	}
}

static inline void alarm_action_plystart (TALARM *alarm, const int id, alarm_plystart *act)
{
	TVLCPLAYER *vp = alarm->com->vp;
	//TTIMERPLAYTRACK *tpt = &vp->gui.playtrack;
	
	TTIMERPLAYTRACK _tpt;
	TTIMERPLAYTRACK *tpt = &_tpt;
	
	tpt->uid = act->uid;
	tpt->track = act->track-1;
	if (tpt->track < 0) tpt->track = 0;

	if (act->title)
		tpt->uid = playlistManagerGetUIDByName(vp->plm, act->title);
	
	if (!tpt->uid && act->uid)
		tpt->uid = act->uid;
	
	if (playlistManagerIsValidUID(vp->plm, tpt->uid)){
		if (getPlayState(vp)){
			trackStop(vp);
			unloadMedia(vp, vp->vlc);
		}

		if (act->volume > 100) act->volume = 100;
		else if (act->volume < 0) act->volume = 0;

		//timerSet(vp, TIMER_PLAYTRACK, 0);
		startPlaylistTrackUID(vp, tpt->uid, tpt->track);
		setVolume(vp, act->volume, VOLUME_APP);
	}
}

static inline void alarm_action_plystop (TALARM *alarm, const int id, alarm_plystop *act)
{
	TVLCPLAYER *vp = alarm->com->vp;
	
	if (getPlayState(vp)){
		trackStop(vp);
		unloadMedia(vp, vp->vlc);
	}
	if (act->goIdle)
		timerSet(vp, TIMER_SETIDLEA, 200);
}

static inline void alarm_action_flash (TALARM *alarm, const int id, alarm_flash *act)
{
	// TODO
}

static inline void alarm_action_callback (TALARM *alarm, const int id, alarm_callback *act)
{
	if (act->cb)
		act->cb(id, act->dataInt1, act->dataPtr1, act->dataPtr2);
}
	
static inline void alarm_action_msg (TALARM *alarm, const int id, alarm_msg *act)
{
	TVLCPLAYER *vp = alarm->com->vp;
	
	if (act->pageId > 0)
		pageSendMessage(vp->pages, act->pageId, PAGE_MSG_ALARM_FIRED, id, act->dataInt2, act->dataPtr);
	else
		pageDispatchMessage(vp->pages, PAGE_MSG_ALARM_FIRED, id, act->dataInt2, act->dataPtr);
}


static inline void alarm_action_ignore (TALARM *alarm, const int id, alarm_ignore *act)
{
	// do nothing..
}

static inline void alarm_action_exit (TALARM *alarm, const int id, alarm_exit *act)
{
	timerSet(alarm->com->vp, TIMER_SHUTDOWN, 0);
}

static inline void *alarmAlloc ()
{
	return my_calloc(1, sizeof(TALARMTIMER));
}

static inline void alarmFree (void *ptr)
{
	my_free(ptr);
}

static inline TALARMTIMER *alarmNew ()
{
	TALARMTIMER *alarm = alarmAlloc();
	if (alarm){
		alarm->state = ALARM_UNSET;
		strcpy(alarm->timeStr, "unset");
		alarm->action = ALARM_ACT_IGNORE;
		alarm->wakeOnIdle = 1;
		alarm->trigger.time64 = 0;
		alarm->trigger.period = ALARM_FIRE_DAILY;
	}
	return alarm;
}

static inline void alarmDestroy (TALARMTIMER *alarm)
{
	if (alarm->starttask.path)
		my_free(alarm->starttask.path);
	if (alarm->plystart.title)
		my_free(alarm->plystart.title);
	if (alarm->endtask.module)
		my_free(alarm->endtask.module);
	alarmFree(alarm);
}

static inline int alarmAdd (TALARM *alarm, TALARMTIMER *alm)
{
	for (int i = 0; i < ALARM_MAX; i++){
		if (!alarm->alm[i]){
			alarm->alm[i] = alm;
			alm->id = ++alarm->idSrc;
			return alm->id;
		}
	}
	return 0;
}

static inline void alarmUiSetEnabledState (TALARM *alarm, const int state)
{
	TLABELSTR *lblstr = alarm->ui.btns[ALARM_BTN_ENABLE];
	if (!lblstr) return;

	//printf("alarmUiSetEnabledState %i\n", state);

	labelRenderColourSet(lblstr->label, lblstr->strId, strEnabled[state].colFore, strEnabled[state].colBack, strEnabled[state].colOutline);
	labelStringSet(lblstr->label, lblstr->strId, strEnabled[state].str);
	ccSetUserDataInt(lblstr->label, state);
}

static inline void alarmUiSetPeriodState (TALARM *alarm, const int state)
{
	TLABELSTR *lblstr = alarm->ui.btns[ALARM_BTN_DAILY];
	if (!lblstr) return;

	labelItemPositionSet(lblstr->label, lblstr->strId, 0, strPeriod[state].yRenderOffset);
	labelRenderColourSet(lblstr->label, lblstr->strId, strPeriod[state].colFore, strPeriod[state].colBack, strPeriod[state].colOutline);
	labelStringSet(lblstr->label, lblstr->strId, strPeriod[state].str);
	ccSetUserDataInt(lblstr->label, state);
	buttonsSetState(alarm->ui.cfg.btns[BTN_ALM_DAYS], (state == ALARM_FIRE_WEEKLY));
}

static inline void alarmUiSetAlarmTime (TALARM *alarm, const char *tStr)
{
	//printf("alarmUiSetAlarmTime '%s'\n", tStr);
	
	int slen = strlen(tStr);
	if (slen < 4 || slen > 5) return;	// min str = 1:23, ideal is nn:nn

	char str[4] = {0};
	char timeStr[16];
	
	// reformat str to nn:nn
	if (tStr[1] == ':'){
		timeStr[0] = '0';
		strcpy(&timeStr[1], tStr);
	}else if (tStr[2] == ':'){
		strcpy(timeStr, tStr);
	}else{
		return;
	}

	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
		TLABELSTR *lblstr = alarm->ui.time[i];
		if (lblstr){
			str[0] = timeStr[i];
			labelStrUpdate(lblstr, str);
			labelItemDataSet(lblstr->label, lblstr->strId, str[0]);
		}
	}
}

// is the default UI alarm enabled or nor
int alarmUiGetStatus (TALARM *alarm)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, alarm->active.aid);
	if (alm)
		return (alm->state == ALARM_ENABLED);
	else
		return 0;
}

int alarmUiGetAlarmTime (TALARM *alarm, char *buffer)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, alarm->active.aid);
	if (alm){
		strncpy(buffer, alm->timeStr, ALARM_MAXTIMEDIGITS);
		buffer[ALARM_MAXTIMEDIGITS] = 0;
		return 1;
	}
	return 0;
}

static inline int alarmEnableAlarm (TALARM *alarm, const int id)
{
	//printf("alarmEnableAlarm %i\n", id);
	
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (alm){
		if (!(alm->state == ALARM_FIRED && alm->trigger.period == ALARM_FIRE_SINGLESHOT)){
			alm->state = ALARM_ENABLED;
			//printf("alarmEnableAlarm %i set\n", id);
			return 1;
		}else{
			//printf("alarmEnableAlarm %i not ready\n", id);
		}
	}else{
		//printf("alarmEnableAlarm %i invalid\n", id);
	}
	
	//printf("alarmEnableAlarm %i not set\n", id);
	return 0;
}

static inline void alarmDisableAlarm (TALARM *alarm, const int id)
{
	//printf("alarmDisableAlarm %i\n", id);
	
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (alm) alm->state = ALARM_DISABLED;
}

static inline void alarmEnable (TALARM *alarm, const int id)
{
	if (alarmEnableAlarm(alarm, id)){
		if (id == alarm->active.aid)	// only modify if its the ui alarm
			alarmUiSetEnabledState(alarm, 1);
	}else{
		if (id == alarm->active.aid)
			alarmUiSetEnabledState(alarm, 0);
	}
}

static inline void alarmDisable (TALARM *alarm, const int id)
{
	alarmDisableAlarm(alarm, id);
	if (id == alarm->active.aid)
		alarmUiSetEnabledState(alarm, 0);
}

static inline void alarmEnableRepeat (TALARM *alarm, const int id)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (alm) alm->keepAlive = 1;
}

static inline void alarmDisableRepeat (TALARM *alarm, const int id)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (alm) alm->keepAlive = 0;
}

// the time at 00:00 today
static inline time64_t getDayStartTime ()
{
	struct tm *tdate = getTimeReal(NULL);
	tdate->tm_hour = 0;
	tdate->tm_min = 0;
	tdate->tm_sec = 0;
	return _mktime64(tdate);
}

static inline time64_t alarmGetTime (TALARM *alarm, const int id)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (alm)
		return alm->trigger.time64;

	return 0;
}

static inline int alarmGetPeriod (TALARM *alarm, const int id)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (alm)
		return alm->trigger.period;

	return 0;
}

static inline int alarmSetWhen (TALARM *alarm, const int id, const int when)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (alm){
		alm->trigger.when = when;
		return 1;
	}
	return 0;
}

static inline int alarmSetPeriod (TALARM *alarm, const int id, const int period)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (alm){
		alm->trigger.period = period;
		if (id == alarm->active.aid){
			alarmUiSetPeriodState(alarm, period);
		}
		return 1;
	}
	return 0;
}

static inline int alarmSetTime (TALARM *alarm, const int id, const time64_t t64)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (alm){
		alm->trigger.time64 = t64;
		alarmTimeToString(t64, alm->timeStr, ALARM_MAXTIMEDIGITS);
		if (id == alarm->active.aid)
			alarmUiSetAlarmTime(alarm, alm->timeStr);
		//printf("alarmSetTime %I64d '%s'\n", t64, alm->timeStr);
		return 1;
	}
	return 0;
}

static inline void alarmActionClear (TALARM *alarm, const int id)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (alm) alm->action = 0;
}

// set this as a concurrent action
static inline int alarmActionAdd (TALARM *alarm, const int id, const int action, void *desc)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	//printf("alarmActionAdd: %p %i %i\n", alm, id, action);
	if (!alm) return 0;
	
/*	
	printf("%i\n", sizeof(alarm_starttask));
	printf("%i\n", sizeof(alarm_endtask));
	printf("%i\n", sizeof(alarm_beep));
	printf("%i\n", sizeof(alarm_plystart));
	printf("%i\n", sizeof(alarm_plystop));
	printf("%i\n", sizeof(alarm_flash));
	printf("%i\n", sizeof(alarm_callback));
	printf("%i\n", sizeof(alarm_msg));
	printf("%i\n", sizeof(alarm_exit));
*/
	
	switch (action){
	  case ALARM_ACT_IGNORE:	break;
	  case ALARM_ACT_STARTTASK:	my_memcpy(&alm->starttask,	desc, sizeof(alarm_starttask)); break;
	  case ALARM_ACT_ENDTASK:	my_memcpy(&alm->endtask,	desc, sizeof(alarm_endtask)); break;
	  case ALARM_ACT_BEEP:		my_memcpy(&alm->beep,		desc, sizeof(alarm_beep)); break;
	  case ALARM_ACT_PLYSTART:	my_memcpy(&alm->plystart,	desc, sizeof(alarm_plystart)); break;
	  case ALARM_ACT_PLYSTOP:	my_memcpy(&alm->plystop,	desc, sizeof(alarm_plystop)); break;
	  case ALARM_ACT_FLASH:		my_memcpy(&alm->flash,		desc, sizeof(alarm_flash)); break;
	  case ALARM_ACT_CB:		my_memcpy(&alm->callback,	desc, sizeof(alarm_callback)); break;
	  case ALARM_ACT_MSG:		my_memcpy(&alm->msg,		desc, sizeof(alarm_msg)); break;
	  case ALARM_ACT_EXIT:		my_memcpy(&alm->exit,		desc, sizeof(alarm_exit)); break;
	  default: return 0;
	};
	
	alm->action |= action;
	return alm->action;
}

// set this as the only action
static inline int alarmActionSet (TALARM *alarm, const int id, const int action, void *desc)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (!alm) return 0;
	
	if (alarmActionAdd(alarm, id, action, desc)&action)
		return alm->action = action;
	else
		return 0;
}

int alarmActionGet (TALARM *alarm, const int id)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (alm)
		return alm->action;
	else
		return 0;
}

int alarmActionGetDesc (TALARM *alarm, const int id, const int action, void *desc)
{
	TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	if (!alm) return 0;
	
	//printf("alarmActionGetDesc %X\n", action);
	
	switch (action){
	  case ALARM_ACT_IGNORE:	break;
	  case ALARM_ACT_STARTTASK:	my_memcpy(desc, &alm->starttask, sizeof(alarm_starttask)); break;
	  case ALARM_ACT_ENDTASK:	my_memcpy(desc, &alm->endtask, sizeof(alarm_endtask)); break;
	  case ALARM_ACT_BEEP:		my_memcpy(desc, &alm->beep, sizeof(alarm_beep)); break;
	  case ALARM_ACT_PLYSTART:	my_memcpy(desc, &alm->plystart, sizeof(alarm_plystart)); break;
	  case ALARM_ACT_PLYSTOP:	my_memcpy(desc, &alm->plystop, sizeof(alarm_plystop)); break;
	  case ALARM_ACT_FLASH:		my_memcpy(desc, &alm->flash, sizeof(alarm_flash)); break;
	  case ALARM_ACT_CB:		my_memcpy(desc, &alm->callback, sizeof(alarm_callback)); break;
	  case ALARM_ACT_MSG:		my_memcpy(desc, &alm->msg, sizeof(alarm_msg)); break;
	  case ALARM_ACT_EXIT:		my_memcpy(desc, &alm->exit, sizeof(alarm_exit)); break;
	  default: return 0;
	};
	return 1;
}

void alarmRemove (TALARM *alarm, const int id)
{
	for (int i = 0; i < ALARM_MAX; i++){
		if (alarm->alm[i] && alarm->alm[i]->id == id){
			alarmDestroy(alarm->alm[i]);
			alarm->alm[i] = NULL;
			return;
		}
	}
}

int alarmCreate (TALARM *alarm, const int period, const time64_t t64, const int when)
{
	TALARMTIMER *alm = alarmNew();
	if (alm){
		alm->trigger.time64 = /*getDayStartTime() +*/ t64;
		alm->trigger.period = period;
		alm->trigger.when = when;
		alarmTimeToString(t64*60, alm->timeStr, ALARM_MAXTIMEDIGITS);
		//printf("alarmCreate: %I64d '%s'\n", t64, alm->timeStr);
		if (alarmAdd(alarm, alm))
			return alm->id;
		else
			alarmDestroy(alm);
	}
	return 0;
}

static inline int alarmFire (TALARM *alarm, TALARMTIMER *alm)
{
	//TALARMTIMER *alm = alarmIdToAlm(alarm, id);
	//if (!alm) return 0;

	int ret = 0;
	const int action = alm->action&ALARM_ACT_MASK;
	
	//printf("alarmFire %X\n", action);
	
	// more than alarm event may be active
	if (action&ALARM_ACT_IGNORE){		// don't act upon alarm, do nothing
		alarm_action_ignore(alarm, alm->id, &alm->ignore);
		return 1;
	}
	if (action&ALARM_ACT_EXIT){
		alarm_action_exit(alarm, alm->id, &alm->exit);
		return 1;
	}
	if (action&ALARM_ACT_STARTTASK){
		alarm_action_startTask(alarm, alm->id, &alm->starttask);
		ret++;
	}
	if (action&ALARM_ACT_ENDTASK){
		alarm_action_endTask(alarm, alm->id, &alm->endtask);
		ret++;
	}
	if (action&ALARM_ACT_BEEP){
		alarm_action_beep(alarm, alm->id, &alm->beep);
		ret++;
	}
	if (action&ALARM_ACT_PLYSTART){
		alarm_action_plystart(alarm, alm->id, &alm->plystart);
		ret++;
	}
	if (action&ALARM_ACT_PLYSTOP){
		alarm_action_plystop(alarm, alm->id, &alm->plystop);
		ret++;
	}
	if (action&ALARM_ACT_FLASH){
		alarm_action_flash(alarm, alm->id, &alm->flash);
		ret++;
	}
	if (action&ALARM_ACT_CB){
		alarm_action_callback(alarm, alm->id, &alm->callback);
		ret++;
	}
	if (action&ALARM_ACT_MSG){
		alarm_action_msg(alarm, alm->id, &alm->msg);
		ret++;
	}

	return ret;
}

int alarmCheckAlarm (TALARM *alarm, const time64_t t0, TALARMTIMER *alm)
{
	//const double dt = _difftime64(getDayStartTime() + alm->trigger.time64, t0);
	//printf("alarmCheckAlarm: %I64d %I64d %.0f\n", t0, alm->trigger.time64, dt);
	
	if (alm->trigger.period == ALARM_FIRE_WEEKLY){
		struct tm *date = getTimeReal(NULL);
		if (!(alm->trigger.when & (1<<date->tm_wday)))
			return 0;
	}

	const double dt = _difftime64(getDayStartTime() + alm->trigger.time64, t0);
	if (dt <= ALARM_FIREWINDOW_HIGH && dt > ALARM_FIREWINDOW_LOW){
		//printf("timer %i firing\n", alm->id);
		
		if (alm->wakeOnIdle){
			if (getIdle(alarm->com->vp)){
				wakeup(alarm->com->vp);
				sbuiWoken(alarm->com->vp);	// shouldn't be here
			}
		}
		int fired = alarmFire(alarm, alm);
		if (fired) alm->trigger.tFired = getTickCount();
		
		if (!alm->keepAlive)
			alarmRemove(alarm, alm->id);
		//else if (alm->trigger.period == ALARM_FIRE_SINGLESHOT)
		//	alm->state = ALARM_FIRED;
		else
			alm->state = ALARM_FIRED;
			//alm->state = ALARM_ENABLED;
		return fired > 0;
	}
	return 0;
}

int alarmCheckAlarms (TALARM *alarm)
{
	struct tm *tdate = getTimeReal(NULL);
	const time64_t t0 = _mktime64(tdate);
	int enabledAlmCt = 0;
	
	for (int i = 0; i < ALARM_MAX; i++){
		TALARMTIMER *alm = alarm->alm[i];
		if (!alm) continue;
		
		if (alm->state == ALARM_ENABLED){
			alarmCheckAlarm(alarm, t0, alm);
			enabledAlmCt++;
		}else if (alm->state == ALARM_FIRED){							// reenable any repeatable alarms after previous firing
			if (alm->trigger.period == ALARM_FIRE_DAILY){				// but not too soon, don't want a double trigger
				if (getTickCount() - alm->trigger.tFired > ALARM_REENABLE_WINDOW){
					alarmEnable(alarm, alm->id);
				}
			}else if (alm->trigger.period == ALARM_FIRE_SINGLESHOT){	// allow singleshots to be reactivated
				if (getTickCount() - alm->trigger.tFired > ALARM_REENABLE_WINDOW){
					alarmDisable(alarm, alm->id);
				}
			}
		}
	}

	return enabledAlmCt;
}

// TIMER_ALARM
void timer_alarm (TVLCPLAYER *vp)
{
	//printf("TIMER_ALARM\n");
	
	if (alarmCheckAlarms(pageGetPtr(vp,PAGE_ALARM)))
		timerSet(vp, TIMER_ALARM, ALARM_PERIOD);
	else
		timerSet(vp, TIMER_ALARM, ALARM_PERIOD+10000);
		
	//TALARM *alarm = pageGetPtr(vp, PAGE_ALARM);
	//TALARMTIMER *alm = alarmIdToAlm(alarm, alarm->active.aid);
	//printf("TIMER_ALARM %i\n", alm->state);
	

}

static inline int alarmTimeGetWidth (TALARM *alarm, int *widths)
{
	int w = 0;
	
	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
		TLABELSTR *lblstr = alarm->ui.time[i];
		labelStringGetMetrics(lblstr->label, lblstr->strId, NULL, NULL, widths, NULL);
		w += *widths++;
	}
	return w;
}

static inline void alarmTimeUpdateMetrics (TALARM *alarm, int fw, int gap, const int y)
{
	int charWidths[8];
	const int aWidth = alarmTimeGetWidth(alarm, charWidths);

	if (fw > 0)
		alarm->ui.frameWidth = fw;
	else
		fw = alarm->ui.frameWidth;

	if (gap >= 0)
		alarm->ui.charSpace = gap;
	else if (gap < 0)
		gap = alarm->ui.charSpace;

	int x = ((fw - (aWidth+(gap*4))) / 2) + 3;
	if (x < 0) x = 0;

	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
		ccSetMetrics(alarm->ui.time[i]->label, x, y, charWidths[i], -1);
		x += charWidths[i] + gap;
	}
}

static inline void alarmTimeEnable (TALARM *alarm)
{
	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
		TLABELSTR *lblstr = alarm->ui.time[i];
		if (lblstr) labelStrEnable(lblstr);
	}
}

static inline void alarmTimeDisable (TALARM *alarm)
{
	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
		TLABELSTR *lblstr = alarm->ui.time[i];
		if (lblstr) labelStrDisable(lblstr);
	}
}

static inline void alarmTimeInputEnable (TALARM *alarm)
{
	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
		TLABELSTR *lblstr = alarm->ui.time[i];
		if (lblstr && i != 2) ccInputEnable(lblstr->label);	// 2 = ':'
	}
}

static inline void alarmTimeInputDisable (TALARM *alarm)
{
	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
		TLABELSTR *lblstr = alarm->ui.time[i];
		if (lblstr) ccInputDisable(lblstr->label);
	}
}

static inline int alarmTimeStrToInt (const char *tstr)
{
	return stringToTime(tstr, strlen(tstr)) * 60;
}

static inline char *alarmTimeAlarmToStr (TALARM *alarm, char *tstr)
{
	*tstr = 0;
	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
		TLABELSTR *lblstr = alarm->ui.time[i];
		tstr[i] = labelItemDataGet(lblstr->label, lblstr->strId);
	}
	
	return tstr;
}

static inline int alarmTimeAlarmToInt (TALARM *alarm)
{
	char tstr[ALARM_MAXTIMEDIGITS+3] = {0};
	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
		TLABELSTR *lblstr = alarm->ui.time[i];
		tstr[i] = labelItemDataGet(lblstr->label, lblstr->strId);
	}
	return alarmTimeStrToInt(tstr) / 60;
}

static inline void alarmTimeSetColour (TALARM *alarm, const unsigned int colI, const double colD)
{
	if (colI){
	  	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
			TLABELSTR *lblstr  = alarm->ui.time[i];
			labelRenderColourSet(lblstr->label, lblstr->strId, 0xFFFFFF, colI, 240<<24 | COL_BLUE_SEA_TINT);
	  	}
	}else{
	  	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
			TLABELSTR *lblstr  = alarm->ui.time[i];
			labelRenderColourSet(lblstr->label, lblstr->strId, 0xD0E0E0, RGB(255-(2.55*colD),255-(2.00*colD),2.55*colD), 240<<24 | COL_BLUE_SEA_TINT);
	  	}
	}
	
	char tstr[ALARM_MAXTIMEDIGITS+3];
  	alarmTimeAlarmToStr(alarm, tstr);
  	alarmUiSetAlarmTime(alarm, tstr);
}

static inline int alarmTimeGetDigitIdxChar (TALARM *alarm, const int idx)
{
	TLABELSTR *lblstr = alarm->ui.time[idx];
	return labelItemDataGet(lblstr->label, lblstr->strId);
}


static inline int64_t alarmLblstr_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//if (msg != CC_MSG_RENDER)
	//	printf("almLblstr_cb, id:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, msg, (int)data1, (int)data2, dataPtr);
		
	TLABEL *label = (TLABEL*)object;
	if (msg == LABEL_MSG_TEXT_SELECTED_PRESS){
		TALARM *alarm = ccGetUserData(label);
		int idx =  ccGetUserDataInt(label)&0x0000FF;
		TLABELSTR *lblstr = alarm->ui.time[idx];
		int c = labelItemDataGet(lblstr->label, lblstr->strId);
		//printf("%c\n", c);
		
		c++;
		if (idx == 0){
			if (c > '2') c = '0';
		}else if (idx == 1){
			if (alarmTimeGetDigitIdxChar(alarm,0) > '1'){
				if (c > '3') c = '0';
			}else{
				if (c > '9') c = '0';
			}
		}else if (idx == 3){
			if (c > '5') c = '0';
		}else{
			if (c > '9') c = '0';
		}
		labelItemDataSet(lblstr->label, lblstr->strId, c);
		
		char buffer[ALARM_MAXTIMEDIGITS+3] = {0};
		char *tstr = buffer;
		TALARMTIMER *alm = alarmIdToAlm(alarm, alarm->ui.aid);
		if (alm) tstr = alm->timeStr;
		
		for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
			TLABELSTR *lblstr = alarm->ui.time[i];
			tstr[i] = labelItemDataGet(lblstr->label, lblstr->strId);
		}

		alarmUiSetAlarmTime(alarm, tstr);
		alarm->ui.strTimeInt = alarmTimeAlarmToInt(alarm);
		//printf("alarm->ui.strTimeInt %i\n", alarm->ui.strTimeInt);

		if (alarm->ui.strTimeInt > ALARM_MAXTIME){
			if (alarm->active.state){
				alarmTimeSetColour(alarm, 0, 80.0);
				//printf("alarmTimeSetColour red\n");
			}
			alarm->active.state = 0;
		}else if (alarm->ui.strTimeInt <= ALARM_MAXTIME){
			if (!alarm->active.state){
				alarmTimeSetColour(alarm, 240<<24 | COL_GREEN_TINT, 0.0);
				//printf("alarmTimeSetColour green\n");
			}
			alarm->active.state = 1;
		}

		lblstr = alarm->ui.btns[ALARM_BTN_ENABLE];
		if (!alarm->active.state)
			labelStrDisable(lblstr);
		else
			labelStrEnable(lblstr);

		alarmDisable(alarm, alarm->active.aid);
		alarmTimeUpdateMetrics(alarm, -1, -1, -1);
	}
	
	return 1;
}

static inline void alarmUiUpdateDaysState (TALARM *alarm, const int when)
{
	for (int i = 0; i < BTN_ALM_DAY_TOTAL; i++){
		TCCBUTTON *btn = buttonsButtonGet(alarm->ui.cfg.btns[BTN_ALM_DAYS], i);
		buttonFaceActiveSet(btn, !(when&(1<<i)));
	}
}

static inline void alarmCfgApplyConfigSetup (TALARM *alarm)
{

	TVLCPLAYER *vp = alarm->com->vp;
	
	char *period = NULL;
	settingsGet(vp, "alarm.period", &period);
	if (period){
		alarmSetPeriod(alarm, alarm->active.aid, alarmCfgPeriodToMode(period));
		my_free(period);
	}
	
	int when = 0;
	str_list *strList = NULL;
	settingsGet(vp, "alarm.weekly.", &strList);
	if (strList){
		for (int i = 0; i < strList->total; i++){
			char *day = strList->strings[i];

			for (int d = 0; d < 7; d++){
				if (stristr(day, clkDayToName(d))){
					//printf("%i %i: '%s' '%s'\n", i, d, day, clkDayToName(d));
					when |= (1<<d);
					break;
				}
			}
		}
	
		cfg_configStrListFree(strList);
		my_free(strList);
	}

	alarmSetWhen(alarm, alarm->active.aid, when);
	
	char tStr[8];
	time64_t t64 = 0;
	char *value = NULL;
	
	settingsGet(vp, "alarm.time", &value);
	if (!value){
		t64 = alarmTimeStrToInt(ALARM_INITIALTIME);
		strcpy(tStr, ALARM_INITIALTIME);
	}else{
		t64 = alarmTimeStrToInt(value);
		if (t64 < 0 || t64 > (ALARM_MAXTIME*60)){
			t64 = alarmTimeStrToInt(ALARM_INITIALTIME);
			strcpy(tStr, ALARM_INITIALTIME);
		}else{
			strcpy(tStr, value);
		}
		my_free(value);
	}

	alarmSetTime(alarm, alarm->active.aid, t64);

	int status = 0;
	settingsGet(vp, "alarm.enabled", &status);
	//printf("status %i\n", status);
	if (!status)
		alarmDisable(alarm, alarm->active.aid);
	else
		alarmEnable(alarm, alarm->active.aid);
	
}

static inline void alarmCfgApplyConfigAction (TALARM *alarm)
{
	TVLCPLAYER *vp = alarm->com->vp;
	
	alarm_plystart media = {0};

	settingsGet(vp, "alarm.action.playtrack.title", &media.title);
	settingsGet(vp, "alarm.action.playtrack.uid", &media.uid);
	settingsGet(vp, "alarm.action.playtrack.track", &media.track);
	settingsGet(vp, "alarm.action.playtrack.volume", &media.volume);

	if (media.volume > 100) media.volume = 100;	
	//printf("'%s' %X %i %i\n", media.title, (int)media.uid, media.track, media.volume);
	
	alarmActionClear(alarm, alarm->active.aid);
	alarmActionAdd(alarm, alarm->active.aid, ALARM_ACT_PLYSTART, &media);
}

static inline int page_alarmRenderInit (TALARM *alarm, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	const int fw = frame->width;

	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){		// 12:45
		TLABELSTR *lblstr = my_calloc(1, sizeof(TLABELSTR));
		if (!lblstr) return 0;

		lblstr->label = ccCreate(vp->cc, PAGE_ALARM, CC_LABEL, alarmLblstr_cb, &lblstr->ccId, 132, 265);
		lblstr->strId = labelTextCreate(lblstr->label, "0", PF_LEFTJUSTIFY, ALARM_SETTIME_FONT, -6, 0);
		alarm->ui.time[i] = lblstr;
		labelRenderBlurRadiusSet(lblstr->label, lblstr->strId, 8);
		labelRenderFilterSet(lblstr->label, lblstr->strId, 1);
		int hover = LABEL_RENDER_HOVER;
		if (i == 2) hover = 0;	// don't enable hovering for non adjustable char ':'
		labelRenderFlagsSet(lblstr->label, LABEL_RENDER_TEXT|hover);
		ccSetUserData(lblstr->label, alarm);
		ccSetUserDataInt(lblstr->label, i&0xFF);
		labelItemDataSet(lblstr->label, lblstr->strId, 0);		// will hold this labels digit
		ccInputDisable(lblstr->label);
	}

	alarmCfgApplyConfigSetup(alarm);
	alarmTimeSetColour(alarm, 240<<24 | COL_GREEN_TINT, 0.0);
	alarmTimeUpdateMetrics(alarm, fw, ALARM_CHARSPACE, ALARM_VERTICALPOS);
	alarmTimeInputEnable(alarm);
	alarmCfgApplyConfigAction(alarm);

	timerSet(vp, TIMER_ALARM, 2500);

#if 0
	TALARMTIMER *alm = alarmIdToAlm(alarm, alarm->active.aid);
	printf("alm: %I64d %i %i '%s'\n", alm->trigger.time64, alm->trigger.period, alm->trigger.when, alm->timeStr);
#endif
	return 1;
}

static inline int page_alarmRenderBegin (TALARM *alarm, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, TFRAME *frame, void *opaquePtr)
{
	alarmTimeUpdateMetrics(alarm, -1, -1, -1);
	alarmUiUpdateDaysState(alarm, alarmGetWhen(alarm, alarm->active.aid));
	alarmTimeEnable(alarm);
	alarmTimeInputEnable(alarm);
	return 1;
}

static inline int page_alarmRenderEnd (TALARM *alarm, TVLCPLAYER *vp, int64_t time0, int64_t zDepth, void *opaquePtr)
{
	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
		TLABELSTR *lblstr = alarm->ui.time[i];
		if (lblstr) ccInputDisable(lblstr->label);
	}

	lSetRenderEffect(vp->ml->hw, LTR_DEFAULT);
	
	return 1;
}
	
static inline int page_alarmRender (TALARM *alarm, TVLCPLAYER *vp, TFRAME *frame)
{

//double t0 = getTime(vp);
	const struct tm *tdate = getTimeReal(NULL);

	alarmDrawDateDay(frame, tdate, -1, 20, ALARM_DAY_FONT);
	alarmDrawDateMonth(frame, tdate, -1, 20, ALARM_DATE_FONT);
	alarmDrawDateTime(frame, tdate, -1, 0, ALARM_TIME_FONT);

	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++)
		labelStrRender(alarm->ui.time[i], frame);

	for (int i = 0; i < ALARM_BTN_TOTAL; i++)
		labelStrRender(alarm->ui.btns[i], frame);

	if (alarmGetPeriod(alarm, alarm->active.aid) == ALARM_FIRE_WEEKLY)
		buttonsRenderAll(alarm->ui.cfg.btns[BTN_ALM_DAYS], frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);

//printf("render %f\n", getTime(vp)-t0);

	return 1;
}

static inline int64_t ccAlmBtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//if (msg != CC_MSG_RENDER)
	//	printf("ccAlmBtn_cb, id:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, msg, (int)data1, (int)data2, dataPtr);

	TCCBUTTON *btn = (TCCBUTTON*)object;
	
	if (msg == BUTTON_MSG_SELECTED_PRESS){
		TCCBUTTONS *btns = ccGetUserData(btn);
		if (btns) btns->t0 = getTickCount();
		
		TALARM *alarm = pageGetPtr(btn->cc->vp, PAGE_ALARM);
		const int aid = alarm->active.aid;
		int when = alarmGetWhen(alarm, aid);
		
		int day = 1 << ccGetUserDataInt(btn);
		if (when & day)
			when &= ~day;
		else
			when |= day;
		
		alarmSetWhen(alarm, aid, when);
		//printf("when %X %X\n", when, day);

		buttonFaceActiveSet(btn, !(when&day));
	}
	return 1;	
}

static inline int64_t alarmLblBtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//if (msg != CC_MSG_RENDER)
	//	printf("alarmLblBtn_cb, id:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, msg, (int)data1, (int)data2, dataPtr);
		
	TLABEL *label = (TLABEL*)object;
	TALARM *alarm = ccGetUserData(label);
			
	if (msg == LABEL_MSG_BASE_SELECTED_PRESS){
		if (label->id == alarm->ui.btns[ALARM_BTN_ENABLE]->ccId){
			int btnState = ccGetUserDataInt(label)^1;
			ccSetUserDataInt(label, btnState);
			//printf("btnState %i %i\n", btnState, alarm->active.state);

			if (btnState){
				alarmEnable(alarm, alarm->active.aid);
				alarmSetTime(alarm, alarm->active.aid, alarmTimeAlarmToInt(alarm)*60);
				//printf("alarmSetTime %i\n", alarmTimeAlarmToInt(alarm));
				
			}else{
				alarmDisable(alarm, alarm->active.aid);
			}
		}else if (label->id == alarm->ui.btns[ALARM_BTN_DAILY]->ccId){
			int btnState = ccGetUserDataInt(label) + 1;
			if (btnState > 2) btnState = 0;
			//ccSetUserDataInt(label, btnState);
			
			alarmUiSetPeriodState(alarm, btnState);
			
			if (btnState == ALARM_FIRE_DAILY){
				alarmSetPeriod(alarm, alarm->active.aid, ALARM_FIRE_DAILY);
				alarmEnableRepeat(alarm, alarm->active.aid);
				
			}else if (btnState == ALARM_FIRE_SINGLESHOT){
				alarmSetPeriod(alarm, alarm->active.aid, ALARM_FIRE_SINGLESHOT);
				//alarmDisableRepeat(alarm, alarm->active.aid);
				alarmEnableRepeat(alarm, alarm->active.aid);
				
			}else if (btnState == ALARM_FIRE_WEEKLY){
				alarmSetPeriod(alarm, alarm->active.aid, ALARM_FIRE_WEEKLY);
				alarmEnableRepeat(alarm, alarm->active.aid);
			}
		}
	}
	
	return 1;
}

static inline int page_alarmStartup (TALARM *alarm, TVLCPLAYER *vp, const int fw, const int fh)
{
	alarm->idSrc = ALARM_BASEID;
	alarm->ui.aid = 0;
	alarm->active.state = 1;
	alarm->active.aid = alarmCreate(alarm, ALARM_FIRE_SINGLESHOT, alarmTimeStrToInt(ALARM_INITIALTIME), ALARM_DAY_SUN);
	alarm_msg amsg = {0, 99, NULL};
	alarmActionSet(alarm, alarm->active.aid, ALARM_ACT_MSG, &amsg);
	alarmEnableRepeat(alarm, alarm->active.aid);
	alarmDisable(alarm, alarm->active.aid);

		
	for (int i = 0; i < ALARM_BTN_TOTAL; i++){
		TLABELSTR *lblstr = my_calloc(1, sizeof(TLABELSTR));
		if (!lblstr) return 0;
		
		TLABEL *label = ccCreate(vp->cc, PAGE_ALARM, CC_LABEL, alarmLblBtn_cb, &lblstr->ccId, 32, 23);
		lblstr->label = label;
		alarm->ui.btns[i] = lblstr;
		
		ccSetUserData(label, alarm);
		ccSetUserDataInt(label, 0);
		lblstr->strId = labelTextCreate(label, "Enabled", 0, ALARM_STATUS_FONT, 0, 0);
		labelRenderFlagsSet(label, LABEL_RENDER_TEXT|LABEL_RENDER_HOVER);
		labelRenderBlurRadiusSet(label, lblstr->strId, 3);
		labelRenderFilterSet(label, lblstr->strId, 2);
		labelItemDataSet(label, lblstr->strId, 0);

		int width = 0, height = 0;
		labelStringGetMetrics(label, lblstr->strId, NULL, NULL, &width, &height);
				
		if (i == ALARM_BTN_ENABLE){
			alarmUiSetEnabledState(alarm, 0);
			ccSetMetrics(label, 20, fh - 90, width, height+6);
		}else if (i == ALARM_BTN_DAILY){
			alarmUiSetPeriodState(alarm, 0);
			ccSetMetrics(label, 216, fh - 88, width, height+6);
		}

		labelStrEnable(lblstr);
	}

	TCCBUTTONS *days = buttonsCreate(vp->cc, PAGE_ALARM, BTN_ALM_DAY_TOTAL, ccAlmBtn_cb);
	alarm->ui.cfg.btns[BTN_ALM_DAYS] = days;
	
	buttonsCreateButton(days, L"alarm/sun.png", L"alarm/sunalt.png", BTN_ALM_DAY_SUN, 0, 1, 0, 0);
	buttonsCreateButton(days, L"alarm/mon.png", L"alarm/monalt.png", BTN_ALM_DAY_MON, 0, 1, 0, 0);
	buttonsCreateButton(days, L"alarm/tue.png", L"alarm/tuealt.png", BTN_ALM_DAY_TUE, 0, 1, 0, 0);
	buttonsCreateButton(days, L"alarm/wed.png", L"alarm/wedalt.png", BTN_ALM_DAY_WED, 0, 1, 0, 0);
	buttonsCreateButton(days, L"alarm/thr.png", L"alarm/thralt.png", BTN_ALM_DAY_THR, 0, 1, 0, 0);
	buttonsCreateButton(days, L"alarm/fri.png", L"alarm/frialt.png", BTN_ALM_DAY_FRI, 0, 1, 0, 0);
	buttonsCreateButton(days, L"alarm/sat.png", L"alarm/satalt.png", BTN_ALM_DAY_SAT, 0, 1, 0, 0);

	
	int x = 390;
	int y = fh - 114;
	int pitch = buttonsWidthGet(days, BTN_ALM_DAY_SUN) + 10;
	buttonsPosSet(days, BTN_ALM_DAY_SUN, x, y);
	buttonsPosSet(days, BTN_ALM_DAY_MON, x+=pitch, y);
	buttonsPosSet(days, BTN_ALM_DAY_TUE, x+=pitch, y);
	buttonsPosSet(days, BTN_ALM_DAY_WED, x+=pitch, y);
	x = 390 + 46;
	y += buttonsHeightGet(days, BTN_ALM_DAY_SUN) + 10;
	buttonsPosSet(days, BTN_ALM_DAY_THR, x, y);
	buttonsPosSet(days, BTN_ALM_DAY_FRI, x+=pitch, y);
	buttonsPosSet(days, BTN_ALM_DAY_SAT, x+=pitch, y);

	alarmUiSetPeriodState(alarm, alarmGetPeriod(alarm, alarm->active.aid));
	return 1;
}

static inline int page_alarmInitalize (TALARM *alarm, TVLCPLAYER *vp, const int fw, const int fh)
{
	//printf("page_alarmInitalize\n");
	setPageAccessed(vp, PAGE_ALARM);
	return 1;
}

static inline int page_alarmShutdown (TALARM *alarm, TVLCPLAYER *vp)
{
	//resetPageAccessed(vp, PAGE_ALARM);
	
	for (int i = 0; i < ALARM_MAX; i++){
		if (alarm->alm[i])
			alarmDestroy(alarm->alm[i]);
	}
	
	for (int i = 0; i < ALARM_MAXTIMEDIGITS; i++){
		TLABELSTR *lblstr = alarm->ui.time[i];
		if (lblstr) labelStrDelete(lblstr);
	}
	
	for (int i = 0; i < ALARM_BTN_TOTAL; i++)
		labelStrDelete(alarm->ui.btns[i]);
		
	buttonsDeleteAll(alarm->ui.cfg.btns[BTN_ALM_DAYS]);
	//buttonsDeleteAll(alarm->ui.cfg.btns[BTN_ALM_MISC]);
	return 1;
}

static inline int page_alarmInput (TALARM *alarm, TVLCPLAYER *vp, const int msg, const int flags, TTOUCHCOORD *pos)
{
	switch(msg){
	  //case PAGE_IN_TOUCH_SLIDE:
	  case PAGE_IN_TOUCH_DOWN:
	  	if (pos->y < 80){
	  		page2SetPrevious(alarm);
	  	}
	  	break;
	  //case PAGE_IN_TOUCH_UP:
	  //case PAGE_IN_WHEEL_FORWARD:
	  //case PAGE_IN_WHEEL_BACK:
	  //	break;
	  //case PAGE_IN_WHEEL_LEFT:
	  case PAGE_IN_WHEEL_RIGHT:
	  	page2SetPrevious(alarm);
	  	break;
	}
	
	return 1;
}

int page_alarmCb (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TALARM *alarm = (TALARM*)pageStruct;
	
	//if (msg != PAGE_CTL_RENDER)
	//	printf("# page_alarmCb: %p %i %I64d %I64d %p %p\n", alarm, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_alarmRender(alarm, alarm->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_INPUT){
		return page_alarmInput(alarm, alarm->com->vp, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_MSG_ALARM_FIRED){
		//alarmEnable(alarm, dataInt1/*alarm->active.aid*/);
		
	}else if (msg == PAGE_CTL_RENDER_START){
		return page_alarmRenderBegin(alarm, alarm->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_END){
		return page_alarmRenderEnd(alarm, alarm->com->vp, dataInt1, dataInt2, opaquePtr);
		
	}else if (msg == PAGE_CTL_RENDER_INIT){
		return page_alarmRenderInit(alarm, alarm->com->vp, dataInt1, dataInt2, dataPtr, opaquePtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_alarmStartup(alarm, alarm->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_alarmInitalize(alarm, alarm->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_alarmShutdown(alarm, alarm->com->vp);
		
	}
	
	return 1;
}

