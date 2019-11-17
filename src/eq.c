
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





//#if (LIBVLC_VERSION_MAJOR >= 2 && LIBVLC_VERSION_MINOR >= 1)


// 2000 = -20db, 1000 = 0.0db, 0 = +20db
int eqAmpToSlider (const double amp)
{
	return 2000 - ((amp * 50.0)+1000.0);
}

// scale is from -20.0db to +20.0db with 0.0 (default) having no effect
double eqSliderToAmp (const double sliderValue)
{
	return (sliderValue - 0.50) * -40.0;
}

int eqApply (TEQ *eq, TVLCCONFIG *vlc, const int force)
{
	if (vlc && vlc->mp && eq->eqObj){
		//double t1 = getTime(eq->vp);
		//printf("%f\n", t1 - eq->t0);
		//if (force || t1 - eq->t0 > 5.0){
			int ret = vlc_equalizerApply(vlc->mp, eq->eqObj);
			//if (ret == 1) eq->t0 = t1;
			return ret;
		//}
	}
	return 0;
}

double eqBandGet (TEQ *eq, const int band)
{
	double amp = 0.0;
	if (eq->bands[band].slider)
		amp = sliderGetValueFloat(eq->bands[band].slider);
	return eqSliderToAmp(amp);
}

int eqBandSet (TEQ *eq, const int band, const double amp)
{
	if (eq->bands[band].slider){
		int ret = sliderSetValue(eq->bands[band].slider, eqAmpToSlider(amp));
		if (!band)
			vlc_equalizerPreampSet(eq->eqObj, amp);
		else
			vlc_equalizerAmpSetByIndex(eq->eqObj, amp, eq->bands[band].index);
		return ret;
	}
	return 0;
}

int eqBandGetTotal ()
{
	return vlc_equalizerBandsGetCount();
}


// is identical to eqPresetGetTotal();
int eqProfileGetTotal ()
{
	return vlc_equalizerPresetGetTotal();
}

int eqPresetGetTotal (TEQ *eq)
{
	return eq->tPresets;
}

int64_t eqobject_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;

	//printf("eqobject_cb. id:%i, objType:%i, msg:%i, data1:%I64d, data2:%I64d\n", obj->id, obj->type, msg, data1, data2);


	if (obj->type == CC_SLIDER_VERTICAL){
		TSLIDER *slider = (TSLIDER*)object;
		TVLCPLAYER *vp = (TVLCPLAYER*)obj->cc->vp;

		switch (msg){
		  case SLIDER_MSG_VALSET:
		  case SLIDER_MSG_VALCHANGED:{
			TEQ *eq = (TEQ*)ccGetUserData(slider);
			if (eq){
				int index = ccGetUserDataInt(slider);	// band
				double value = eqSliderToAmp(sliderGetValueFloat(slider));
				//printf("eqobject_cb %i: value = %f, %i\n", index, value, eq->touched);

				if (index == -1)
					vlc_equalizerPreampSet(eq->eqObj, value);
				else
					vlc_equalizerAmpSetByIndex(eq->eqObj, value, index);

				eqApply(eq, getConfig(vp), 1);

				if (eq->touched++ > 10 && index >= 0){
					//eq->touched = 1;
					TCCBUTTON *btn = buttonsButtonGet(eq->btns, EQ_PRESET);
					buttonFaceTextUpdate(btn, BUTTON_PRI, "Custom");
				}
			}
		  }
		}
	}
	return 1;
}

int eqBuild (TEQ *eq, const int presetIdx)
{
	
	unsigned int pct = vlc_equalizerPresetGetTotal();
	if (presetIdx >= pct) return 0;

	if (presetIdx >= 0){
		vlc_equalizerRelease(eq->eqObj);
		eq->eqObj = vlc_equalizerNewFromPreset(presetIdx);
	}
	if (!eq->eqObj) return 0;

	const int width = 16;
	const char *name = vlc_equalizerPresetGetName(presetIdx);
	strncpy(eq->name, name, EQPRESET_NAME_LEN);

	eq->tPresets = pct;
	eq->tBands = 1 + vlc_equalizerBandsGetCount();	// preamp + band total in this preset

	for (int i = 0; i < eq->tBands; i++){
		TSLIDER *slider;
		if (!eq->bands[i].slider){
			slider = ccCreate(eq->com->vp->cc, PAGE_EQ, CC_SLIDER_VERTICAL, eqobject_cb, &eq->bands[i].ccId, 0, 0);
			sliderFaceSet(slider, SLIDER_FACE_TOP, L"cc/slider_v_thick_top.png");
			sliderFaceSet(slider, SLIDER_FACE_BTM, L"cc/slider_v_thick_btm.png");
			sliderFaceSet(slider, SLIDER_FACE_MID, L"cc/slider_v_thick_mid.png");
			sliderFaceSet(slider, SLIDER_FACE_TIP, L"eq/slider_v_thick_tip.png");
			sliderFacesApply(slider);
			ccSetUserData(slider, eq);
			ccSetMetrics(slider, -1, -1, width, 0);
			sliderSetRange(slider, 0, 2000);

			eq->bands[i].slider = slider;
		}else{
			slider = eq->bands[i].slider;
		}

		double amp;
		if (!i){	// first slider is always the preamp
			eq->bands[i].index = -1;		// -1 = preamp slider
			eq->bands[i].frequency = 0;
			strcpy(eq->bands[i].name, "Preamp");
			amp = vlc_equalizerPreampGet(eq->eqObj);

		}else{
			eq->bands[i].index = i-1;
			eq->bands[i].frequency = vlc_equalizerBandsGetFrequency(eq->bands[i].index);
			_itoa((int)eq->bands[i].frequency, eq->bands[i].name, 10);
			//printf(" %i: '%s' \t %.1f\n", i, eq->bands[i].name, eq->bands[i].frequency);			
			
			amp = vlc_equalizerAmpGetByIndex(eq->eqObj, eq->bands[i].index);
		}
		eq->bands[i].stringHash = getHash(eq->bands[i].name) ^ (EQ_SLIDER_FONT * (i+2));
		
		ccSetUserDataInt(slider, eq->bands[i].index);
		int value = eqAmpToSlider(amp);
		sliderSetValue(slider, value);
		ccEnable(slider);
		

	}

	eq->touched = 0;
	TCCBUTTON *btn = buttonsButtonGet(eq->btns, EQ_PRESET);
	buttonFaceTextUpdate(btn, BUTTON_PRI, eq->name);

	return eq->tBands;
}

const char *eqGetProfileName (const int index)
{
	return libvlc_audio_equalizer_get_preset_name(index);
}

int eqApplyPreset (TEQ *eq, const int preset)
{
	//printf("eqApplyPreset %i\n", preset);
	
	if (preset >= 0 && preset < eqPresetGetTotal(eq)){
		eq->preset = preset;
		eqBuild(eq, eq->preset);
		return eqApply(eq, eq->com->vp->vlc, 1);
	}
	return 0;
}

static inline int eqButtonPress (TEQ *eq, TCCBUTTON *btn, const int id, const TTOUCHCOORD *pos)
{
	TVLCPLAYER *vp = btn->cc->vp;
	eq->btns->t0 = getTickCount();


	switch (id){
	  case EQ_PRESET:
		if (++eq->preset >= eqPresetGetTotal(eq))
			eq->preset = 0;
		// then pass to 'eq_reset' (below)
		ALLOW_FALLTHROUGH;
	  case EQ_RESET:
		eqApplyPreset(eq, eq->preset);
		eq->touched = 11;
		renderSignalUpdate(vp);
		break;

	  case EQ_BACK:
		page2SetPrevious(eq);
		break;
	}
	return 1;
}

static inline int64_t ccbtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;

	//TCCOBJECT *obj = (TCCOBJECT*)object;
	//printf("ccbtn_cb, id:%i, objType:%i, msg:%i, data1:%i, data2:%i, ptr:%p\n", obj->id, obj->type, msg, (int)data1, (int)data2, dataPtr);

	TCCBUTTON *btn = (TCCBUTTON*)object;
	//const int id = (int)data2;

	if (msg == BUTTON_MSG_SELECTED_PRESS)
		return eqButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	return 1;
}

static inline int page_eqRender (TEQ *eq, TVLCPLAYER *vp, TFRAME *frame)
{
	const int blurOp = LTR_BLUR5;
	lSetRenderEffect(frame->hw, blurOp);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_COLOUR, 0x000000);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_RADIUS, 3);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_SETTOP, 1);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_X, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_Y, 0);
	lSetFilterAttribute(frame->hw, blurOp, LTRA_BLUR_ALPHA, 1000);
	lSetForegroundColour(frame->hw, 255<<24|COL_WHITE);


	for (int i = 0; i < eq->tBands; i++){
		TSLIDER *slider = eq->bands[i].slider;
		ccRender(slider, frame);

		TMETRICS metrics = {3, 3, 0, 0};
		metrics.x = 3;
		metrics.y = 3;
		metrics.width = 0;
		metrics.height = 0;

		TLPOINTEX rt;
		sliderGetSliderRect(slider, &rt);
		int sliderY = ccGetPositionY(slider);


		// draw band label
		TFRAME *str = strcFindString(vp->strc, eq->bands[i].stringHash);
		if (!str){
			str = lNewStringEx(frame->hw, &metrics, frame->bpp, 0, EQ_SLIDER_FONT, eq->bands[i].name);
			strcAddString(vp->strc, str, eq->bands[i].stringHash);
		}
		if (str){
			int w = (rt.x2-rt.x1)+1;
			int x = rt.x1 + ((w>>1) - (str->width>>1));
			int y = sliderY + ccGetHeight(slider);
			drawImage(str, frame, x, y, str->width-1, str->height-1);
		}

		metrics.x = 3;
		metrics.y = 3;
		metrics.width = 0;
		metrics.height = 0;

		//draw band value
		double value = eqBandGet(eq, i);
		if (value == 0.0)
			str = lNewStringEx(frame->hw, &metrics, frame->bpp, 0, EQ_SLIDER_FONT, "%i.0", (int)value);	// remove the - (minus) from "-0.0"
		else
			str = lNewStringEx(frame->hw, &metrics, frame->bpp, 0, EQ_SLIDER_FONT, "%.1f", value);
		if (str){
			int w = (rt.x2-rt.x1)+1;
			int x = rt.x1 + ((w>>1) - (str->width>>1));
			int y = sliderY - str->height;

			drawImage(str, frame, x, y, str->width-1, str->height-1);
			lDeleteFrame(str);
		}
	}

	lSetRenderEffect(frame->hw, LTR_DEFAULT);
	buttonsRenderAll(eq->btns, frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);
	return 1;
}

static inline int page_eqInput (TEQ *eq, TVLCPLAYER *vp, const int msg, const int flags, void *ptr)
{
	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
	  case PAGE_IN_WHEEL_BACK:{
	  	const int x = (flags>>16)&0xFFFF;
	  	const int y = flags&0xFFFF;

	  	if (ccPositionIsOverlapped(buttonsButtonGet(eq->btns,EQ_PRESET), x, y)){
	  		if (msg == PAGE_IN_WHEEL_BACK){
	  			if (++eq->preset >= eqPresetGetTotal(eq))
					eq->preset = 0;
			}else{
				if (--eq->preset < 0)
					eq->preset = eqPresetGetTotal(eq)-1;
		  	}
		  	eqApplyPreset(eq, eq->preset);
			eq->touched = 11;
			renderSignalUpdate(vp);
	  	}else{
			for (int i = 0; i < eq->tBands; i++){
				TSLIDER *slider = eq->bands[i].slider;
				if (ccPositionIsOverlapped(slider, x, y)){
					int64_t max = 1;
					sliderGetRange(slider, NULL, &max);
					int64_t diff = max * 0.01;
					int64_t val = sliderGetValue(slider);

					if (msg == PAGE_IN_WHEEL_FORWARD)
						val -= diff;
					else
						val += diff;
					sliderSetValue(slider, val);
					return 1;
				}
			}
		}
		break;
	  }
	};
	return 0;
}

static inline int page_eqStartup (TEQ *eq, TVLCPLAYER *vp, const int fw, const int fh)
{
	eq->vp = vp;
	eq->tBands = 0;
	eq->name[0] = 0;
	eq->eqObj = NULL;
	eq->touched = 0;
	eq->preset = 0;
	eq->t0 = 0.0;

	for (int i = 0; i < EQBANDS_MAX; i++)
		memset(&eq->bands[i], 0, sizeof(TEQBAND));


	int x = 4;
	int y = 4;

	eq->btns = buttonsCreate(vp->cc, PAGE_EQ, EQ_TOTAL, ccbtn_cb);
	TCCBUTTON *btn = buttonsCreateButton(eq->btns, L"eq/reset.png", NULL, EQ_RESET, 1, 1, x, y);

	btn = buttonsCreateButton(eq->btns, L"common/back_right92.png", NULL, EQ_BACK, 1, 0, x, y);
	x = (fw - ccGetWidth(btn)) - x;
	ccSetPosition(btn, x, y);

	btn = buttonsCreateButton(eq->btns, L"eq/preset.png", NULL, EQ_PRESET, 1, 0, x, y);
	x = (fw - ccGetWidth(btn))/2;
	ccSetPosition(btn, x, y);
	buttonFaceTextSet(btn, BUTTON_PRI, "-", PF_MIDDLEJUSTIFY, EQ_PRESET_FONT, 0, 2);
	buttonFaceTextColourSet(btn, BUTTON_PRI, 255<<24 | COL_WHITE, 255<<24 | COL_BLACK, 177<<24 | COL_BLUE_SEA_TINT);


	eqBuild(eq, eq->preset);

	x = 14; y = 94;
	int gap = 2;
	int s = ((fw - (x*2)) / eq->tBands) + gap;
	int width = 24;
	int height = fh - y - 32;

	for (int i = 0; i < eq->tBands; i++){
		if (!i)
			ccSetMetrics(eq->bands[i].slider, x + (s*i), y, width, height);
		else
			ccSetMetrics(eq->bands[i].slider, x + (s*i) + gap, y, width, height);

		int value = sliderGetValue(eq->bands[i].slider);
		sliderSetValue(eq->bands[i].slider, value);
	}

	return 1;
}

static inline int page_eqInitalize (TEQ *eq, TVLCPLAYER *vp, const int fw, const int fh)
{
	setPageAccessed(vp, PAGE_EQ);
	
	eq->touched = 0;
	return 1;
}

static inline int page_eqShutdown (TEQ *eq, TVLCPLAYER *vp)
{
	vlc_equalizerRelease(eq->eqObj);
	buttonsDeleteAll(eq->btns);

	for (int i = 0; i < EQBANDS_MAX; i++){
		if (eq->bands[i].slider)
			ccDelete(eq->bands[i].slider);
	}
	return 1;
}

int page_eqCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TEQ *eq = (TEQ*)pageStruct;

	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_eqCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);

	if (msg == PAGE_CTL_RENDER){
		return page_eqRender(eq, eq->com->vp, dataPtr);

	//}else if (msg == PAGE_CTL_RENDER_START){

	//}else if (msg == PAGE_CTL_RENDER_END){

	}else if (msg == PAGE_CTL_INPUT){
		return page_eqInput(eq, eq->com->vp, dataInt1, dataInt2, dataPtr);

	}else if (msg == PAGE_CTL_STARTUP){
		return page_eqStartup(eq, eq->com->vp, dataInt1, dataInt2);

	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_eqInitalize(eq, eq->com->vp, dataInt1, dataInt2);

	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_eqShutdown(eq, eq->com->vp);

	}

	return 1;
}

//#endif



