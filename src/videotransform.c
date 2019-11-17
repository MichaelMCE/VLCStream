
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




static inline int64_t tfCcObject_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	TCCOBJECT *obj = (TCCOBJECT*)object;
	if (msg == CC_MSG_RENDER) return 1;
	
	
	if (obj->type == CC_SLIDER_HORIZONTAL){
		TSLIDER *slider = (TSLIDER*)object;
		TVLCPLAYER *vp = (TVLCPLAYER*)obj->cc->vp;
		
		if (slider->id == ccGetId(vp, CCID_SLIDER_VIDEOROTATE)){
			switch (msg){
			  case SLIDER_MSG_VALSET:
		  	  case SLIDER_MSG_VALCHANGED:{
		  	  	double val = sliderGetValueFloat(slider);
		  	  	vp->vlc->rotateAngle = -((val * 360.0) - 180.0);
		  	  	//printf("rotateAngle: %f %f\n", val, vp->vlc->rotateAngle);
		  	  	
		  	    break;
		  	  }
		  }
		}else if (slider->id == ccGetId(vp, CCID_SLIDER_VIDEOSCALE)){
			switch (msg){
			  case SLIDER_MSG_VALSET:
		  	  case SLIDER_MSG_VALCHANGED:{
		  	  	double val = sliderGetValueFloat(slider);
		  	  	//printf("scale %f\n", val);

		  	  	if (val < 0.5){
		  	  		val *= 2.0;
		  	  	}else{
		  	  		val *= 2.0;
		  	  		val *= val;
		  	  	}

		  	  	if (val < 0.03) val = 0.03;
		  	  	/*double v;
		  	  	if (val >= 1.0)
		  	  		v = sqrtf(val) / 2.0;
		  	  	else
		  	  		v = val / 2.0;
		  	  	printf("scale %f %f %f %f\n", sliderGetValueFloat(slider), val, sqrtf(val), v);*/
		  	  	vp->vlc->scaleFactor = val;
		  	    break;
		  	  }
		  }
		}else if (slider->id == ccGetId(vp, CCID_SLIDER_VIDEOBLUR)){
			switch (msg){
			  case SLIDER_MSG_VALSET:
		  	  case SLIDER_MSG_VALCHANGED:{
		  	  	int val = sliderGetValue(slider);
		  	  	vp->vlc->blurRadius = val;
		  	  	//printf("blurRadius %i\n", val);
		  	    break;
		  	  }
		  }
		}else if (slider->id == ccGetId(vp, CCID_SLIDER_PIXELIZE)){
			switch (msg){
			  case SLIDER_MSG_VALSET:
		  	  case SLIDER_MSG_VALCHANGED:{
		  	  	int val = sliderGetValue(slider);
		  	  	if (val > 0)
		  	  		vp->vlc->pixelize = val+1;
		  	  	else
		  	  		vp->vlc->pixelize = 0;
		  	  		
		  	  	//printf("  %i\n", val);
		  	    break;
		  	  }
		  }
		}else if (slider->id == ccGetId(vp, CCID_SLIDER_BRIGHTNESS)){
			switch (msg){
			  case SLIDER_MSG_VALSET:
		  	  case SLIDER_MSG_VALCHANGED:{
		  	  	double value = sliderGetValueFloat(slider) * 2.0;
		  	  	vlc_setAdjustFloat(vp->vlc, libvlc_adjust_Brightness, value);
				vlc_setAdjustInt(vp->vlc, libvlc_adjust_Enable, value != 1.0);
				vp->vlc->brightness = value;
				//printf("  %f %f\n", value, sliderGetValueFloat(slider));
		  	    break;
		  	  }
		  }
		}else if (slider->id == ccGetId(vp, CCID_SLIDER_CONTRAST)){
			switch (msg){
			  case SLIDER_MSG_VALSET:
		  	  case SLIDER_MSG_VALCHANGED:{
		  	  	double value = sliderGetValueFloat(slider) * 4.0;
		  	  	vlc_setAdjustFloat(vp->vlc, libvlc_adjust_Contrast, value);
				vlc_setAdjustInt(vp->vlc, libvlc_adjust_Enable, value != 1.0);
		  	  	vp->vlc->contrast = value;
		  	  	//printf("  %f %f\n", value, sliderGetValueFloat(slider));
		  	    break;
		  	  }
		  }
		}else if (slider->id == ccGetId(vp, CCID_SLIDER_SATURATION)){
			switch (msg){
			  case SLIDER_MSG_VALSET:
		  	  case SLIDER_MSG_VALCHANGED:{
		  	  	double value = sliderGetValueFloat(slider) * 4.0;
		  	  	vlc_setAdjustFloat(vp->vlc, libvlc_adjust_Saturation, value);
				vlc_setAdjustInt(vp->vlc, libvlc_adjust_Enable, value != 1.0);
				vp->vlc->saturation = value;
		  	    break;
		  	  }
		  }
		}else if (slider->id == ccGetId(vp, CCID_SLIDER_GAMMA)){
			switch (msg){
			  case SLIDER_MSG_VALSET:
		  	  case SLIDER_MSG_VALCHANGED:{
		  	  	double value = sliderGetValueFloat(slider) * 4.0;
		  	  	vlc_setAdjustFloat(vp->vlc, libvlc_adjust_Gamma, value);
				vlc_setAdjustInt(vp->vlc, libvlc_adjust_Enable, value != 1.0);
				
		  	  	//int val = sliderGetValue(slider);
		  	  	vp->vlc->gamma = value;
		  	  	//printf("  %i\n", val);
		  	    break;
		  	  }
			}
		}
	}
	return 1;
}

static inline TSLIDER *tfGetSlider (TTRANSFORM *tf, const int sliderIdx)
{
	return tf->str[sliderIdx].slider;
}

void tfSetRotate (TTRANSFORM *tf, double val)
{
	//printf("tfSetRotate %f\n", val);
	TSLIDER *slider = tfGetSlider(tf, SLIDER_ROTATE);

	val = -val;
	if (val > 180.0) val = 180.0;
	else if (val < -180.0) val = -180.0;

	sliderSetValueFloat(slider, (1.0/360.0)*(val+180.0));
	ccSendMessage(slider, SLIDER_MSG_VALCHANGED, 0, 0, NULL);
}

void tfSetScale (TTRANSFORM *tf, double val)
{
	//printf("tfSetScale %f\n", val);
	TSLIDER *slider = tfGetSlider(tf, SLIDER_SCALE);

	if (val < 0.03) val = 0.03;
	else if (val > 4.0) val = 4.0;

	double v;
	if (val >= 1.0)
		v = sqrtf(val) / 2.0;
	else
		v = val / 2.0;

	sliderSetValueFloat(slider, v);
	ccSendMessage(slider, SLIDER_MSG_VALCHANGED, 0, 0, NULL);
}

void tfSetBlur (TTRANSFORM *tf, int val)
{
	TSLIDER *slider = tfGetSlider(tf, SLIDER_BLUR);

	int64_t max;
	sliderGetRange(slider, NULL, &max);
	if (val < 0) val = 0;
	else if (val > max) val = max;

	sliderSetValue(slider, val);
	ccSendMessage(slider, SLIDER_MSG_VALCHANGED, 0, 0, NULL);
}

void tfSetPixelize (TTRANSFORM *tf, int val)
{
	TSLIDER *slider = tfGetSlider(tf, SLIDER_PIXELIZE);

	int64_t max;
	sliderGetRange(slider, NULL, &max);
	if (--val < 0) val = 0;
	else if (val > max) val = max;
	
	sliderSetValue(slider, val);
	ccSendMessage(slider, SLIDER_MSG_VALCHANGED, 0, 0, NULL);
}

void tfSetBrightness (TTRANSFORM *tf, double val)
{
	//printf("tfSetBrightness %f\n", val);
	TSLIDER *slider = tfGetSlider(tf, SLIDER_BRIGHTNESS);

	if (val < 0.0) val = 0.0;
	else if (val > 2.0) val = 2.0;
	
	sliderSetValueFloat(slider, val/2.0);
	ccSendMessage(slider, SLIDER_MSG_VALCHANGED, 0, 0, NULL);
}

void tfSetContrast (TTRANSFORM *tf, double val)
{
	//printf("tfSetContrast %f\n", val);
	TSLIDER *slider = tfGetSlider(tf, SLIDER_CONTRAST);

	if (val < 0.0) val = 0.0;
	else if (val > 4.0) val = 4.0;
	
	sliderSetValueFloat(slider, val/4.0);
	ccSendMessage(slider, SLIDER_MSG_VALCHANGED, 0, 0, NULL);
}

void tfSetSaturation (TTRANSFORM *tf, double val)
{
	//printf("tfSetSaturation %f\n", val);
	TSLIDER *slider = tfGetSlider(tf, SLIDER_SATURATION);

	if (val < 0.0) val = 0.0;
	else if (val > 4.0) val = 4.0;
	
	sliderSetValueFloat(slider, val/4.0);
	ccSendMessage(slider, SLIDER_MSG_VALCHANGED, 0, 0, NULL);
}

void tfSetGamma (TTRANSFORM *tf, double val)
{
	//printf("tfSetGamma %f\n", val);
	TSLIDER *slider = tfGetSlider(tf, SLIDER_GAMMA);

	if (val < 0.0) val = 0.0;
	else if (val > 4.0) val = 4.0;
	
	sliderSetValueFloat(slider, val/4.0);
	ccSendMessage(slider, SLIDER_MSG_VALCHANGED, 0, 0, NULL);
}

void tfReset (TVLCPLAYER *vp, TTRANSFORM *tf)
{
	sliderSetValue(tfGetSlider(tf, SLIDER_ROTATE), 500);
	ccSendMessage(tfGetSlider(tf, SLIDER_ROTATE), SLIDER_MSG_VALCHANGED, 0, 0, NULL);
	  	
	sliderSetValue(tfGetSlider(tf, SLIDER_SCALE), 500);
	ccSendMessage(tfGetSlider(tf, SLIDER_SCALE), SLIDER_MSG_VALCHANGED, 0, 0, NULL);
	  	
	sliderSetValue(tfGetSlider(tf, SLIDER_BLUR), 0);
	ccSendMessage(tfGetSlider(tf, SLIDER_BLUR), SLIDER_MSG_VALCHANGED, 0, 0, NULL);
	  	
	sliderSetValue(tfGetSlider(tf, SLIDER_PIXELIZE), 0);
	ccSendMessage(tfGetSlider(tf, SLIDER_PIXELIZE), SLIDER_MSG_VALCHANGED, 0, 0, NULL);
	  	
	sliderSetValue(tfGetSlider(tf, SLIDER_BRIGHTNESS), 0);
	ccSendMessage(tfGetSlider(tf, SLIDER_BRIGHTNESS), SLIDER_MSG_VALCHANGED, 0, 0, NULL);
	  	
	sliderSetValue(tfGetSlider(tf, SLIDER_CONTRAST), 2500);
	ccSendMessage(tfGetSlider(tf, SLIDER_CONTRAST), SLIDER_MSG_VALCHANGED, 0, 0, NULL);
	
	sliderSetValue(tfGetSlider(tf, SLIDER_SATURATION), 2500);
	ccSendMessage(tfGetSlider(tf, SLIDER_SATURATION), SLIDER_MSG_VALCHANGED, 0, 0, NULL);
	  	
	sliderSetValue(tfGetSlider(tf, SLIDER_GAMMA), 2500);
	ccSendMessage(tfGetSlider(tf, SLIDER_GAMMA), SLIDER_MSG_VALCHANGED, 0, 0, NULL);
	  	
	vlc_setAdjustInt(vp->vlc, libvlc_adjust_Enable, 0);	// disable filter
	
	vp->vlc->blurRadius = 0;
	vp->vlc->pixelize = 0;
	vp->vlc->rotateAngle = 0.0;
	vp->vlc->scaleFactor = 1.0;
	vp->vlc->brightness = 1.0;
	vp->vlc->contrast = 1.0;
	vp->vlc->saturation = 1.0;
	vp->vlc->gamma = 1.0;
	
	//vp->vlc->rotateOp = ROTATE_BILINEAR;
	//vp->vlc->scaleOp = SCALE_BILINEAR;
}

static inline int tfButtonPress (TTRANSFORM *tf, TCCBUTTON *btn, const int id, const TTOUCHCOORD *pos)
{
	TVLCPLAYER *vp = btn->cc->vp;
	tf->btns->t0 = getTickCount();
	
	switch (id){
	  case TF_RESET:
		tfReset(vp, tf);
	  	break;
	  	
	  case TF_CLOSE:
	  	page2SetPrevious(tf);
	  	break;
	}

	return 0;
}
	
static inline int64_t ccbtn_cb (const void *object, const int msg, const int64_t data1, const int64_t data2, void *dataPtr)
{
	if (msg == CC_MSG_RENDER || msg == CC_MSG_INPUT) return 1;

	TCCBUTTON *btn = (TCCBUTTON*)object;
	//const int id = (int)data2;

	if (msg == BUTTON_MSG_SELECTED_PRESS)
		return tfButtonPress(pageGetPtr(btn->cc->vp, ccGetOwner(btn)), btn, ccGetUserDataInt(btn), dataPtr);
	return 1;
}

static inline int page_tfRender (TTRANSFORM *tf, TVLCPLAYER *vp, TFRAME *frame)
{
	buttonsRenderAll(tf->btns, frame, BUTTONS_RENDER_HOVER|BUTTONS_RENDER_ANIMATE);

	lSetForegroundColour(frame->hw, 0xFFFFFFFF);
	lSetBackgroundColour(frame->hw, 0x00000000);
	outlineTextEnable(frame->hw, 160<<24 | COL_BLUE_SEA_TINT);
	
	TFRAME *value[SLIDER_TF_TOTAL];
	if (vp->vlc->rotateAngle == 0.0)
		value[SLIDER_ROTATE] = lNewString(frame->hw, frame->bpp, PF_IGNOREFORMATTING, VTRANSFORM_FONT, "0.0");
	else
		value[SLIDER_ROTATE] = lNewString(frame->hw, frame->bpp, 0, VTRANSFORM_FONT, "%.1f", -vp->vlc->rotateAngle);
	value[SLIDER_SCALE] = lNewString(frame->hw, frame->bpp, 0, VTRANSFORM_FONT, "%.3f", vp->vlc->scaleFactor);
	value[SLIDER_BLUR] = lNewString(frame->hw, frame->bpp, 0, VTRANSFORM_FONT, "%i", vp->vlc->blurRadius);
	value[SLIDER_PIXELIZE] = lNewString(frame->hw, frame->bpp, 0, VTRANSFORM_FONT, "%i", vp->vlc->pixelize);
	value[SLIDER_BRIGHTNESS] = lNewString(frame->hw, frame->bpp, 0, VTRANSFORM_FONT, "%.2f", vp->vlc->brightness);
	value[SLIDER_CONTRAST] = lNewString(frame->hw, frame->bpp, 0, VTRANSFORM_FONT, "%.2f", vp->vlc->contrast);
	value[SLIDER_SATURATION] = lNewString(frame->hw, frame->bpp, 0, VTRANSFORM_FONT, "%.2f", vp->vlc->saturation);
	value[SLIDER_GAMMA] = lNewString(frame->hw, frame->bpp, 0, VTRANSFORM_FONT, "%.2f", vp->vlc->gamma);

	outlineTextDisable(frame->hw);
		
	for (int i = 0; i < SLIDER_TF_TOTAL; i++){
		ccRender(tf->str[i].slider, frame);
		drawShadowedImage(tf->str[i].frame, frame, tf->str[i].x, tf->str[i].y, 0, 1, 2, 1);
		int x = (ccGetPositionX(tf->str[i].slider) + ccGetWidth(tf->str[i].slider) - value[i]->width) - 16;
		int y = ccGetPositionY(tf->str[i].slider) - 24;
		drawShadowedImage(value[i], frame, x, y, 0, 1, 2, 1);
		lDeleteFrame(value[i]);
	}

	//renderRotateArrow(vp, tf, frame);
	return 1;
}

static inline int page_tfInput (TTRANSFORM *tf, const int msg, const int flags, void *ptr)
{
	switch(msg){
	  case PAGE_IN_WHEEL_FORWARD:
	  case PAGE_IN_WHEEL_BACK:{
	  	const int x = (flags>>16)&0xFFFF;
	  	const int y = flags&0xFFFF;
	  	
	 	for (int i = 0; i < SLIDER_TF_TOTAL; i++){
	 		TSLIDER *slider = tf->str[i].slider;

			if (ccPositionIsOverlapped(slider, x, y)){
				int64_t max = 1;
				sliderGetRange(slider, NULL, &max);
				int64_t diff = max * 0.01;
				if (diff < 1) diff = 1;

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
	}

	return 0;
}

static inline int page_tfStartup (TTRANSFORM *tf, TVLCPLAYER *vp, const int fw, const int fh)
{

	tf->btns = buttonsCreate(vp->cc, PAGE_TRANSFORM, TF_TOTAL, ccbtn_cb);

	TCCBUTTON *btn = buttonsCreateButton(tf->btns, L"transform/reset.png", NULL, TF_RESET, 1, 1, 0, 0);
	int x = fw-1 - ccGetWidth(btn);
	int y = (fh/2) - ccGetHeight(btn) - 60;
	ccSetPosition(btn, x, y);

	btn = buttonsCreateButton(tf->btns, L"common/back_right92.png", NULL, TF_CLOSE, 1, 0, 0, 0);
	x = fw-1 - ccGetWidth(btn);
	y = (fh/2) - 4;
	ccSetPosition(btn, x, y);
	

#if 0
	imageCacheAddImage(vp->imgc, L"transform/point.png", SKINFILEBPP, &vp->gui.image[IMGC_ROTATEPOINT]);
	TFRAME *point = imageCacheGetImage(vp->imgc, vp->gui.image[IMGC_ROTATEPOINT]);
	tf->working = lNewFrame(frame->hw, fw, fh, frame->bpp);
	const int xc = (tf->working->width/2)-(point->width/2);
	const int yc = (tf->working->height/2)-(point->height/2);
	lRotate(point, tf->working, xc, yc, -vp->vlc->rotateAngle);
#endif

	x = fw * 0.02;
	y = fh * 0.135;
	const int height = 48;	
	const int width = fw * 0.39;
	const int vspace = fh * (1.0/4.0);

	TSLIDER *slider = ccCreate(vp->cc, PAGE_TRANSFORM, CC_SLIDER_HORIZONTAL, tfCcObject_cb, &vp->gui.ccIds[CCID_SLIDER_VIDEOROTATE], width, height);
	sliderFaceSet(slider, SLIDER_FACE_LEFT, L"transform/slider_h_thick_left.png");
	sliderFaceSet(slider, SLIDER_FACE_RIGHT, L"transform/slider_h_thick_right.png");
	sliderFaceSet(slider, SLIDER_FACE_MID, L"transform/slider_h_thick_mid.png");
	sliderFaceSet(slider, SLIDER_FACE_TIP, L"transform/slider_h_thick_tip.png");
	ccSetMetrics(slider, x, y, width, height);
	sliderSetRange(slider, 0, 1000);
	sliderSetValue(slider, 500);
	tf->str[SLIDER_ROTATE].slider = slider;
	ccEnable(slider);


	slider = ccCreate(vp->cc, PAGE_TRANSFORM, CC_SLIDER_HORIZONTAL, tfCcObject_cb, &vp->gui.ccIds[CCID_SLIDER_VIDEOSCALE], 0, 0);
	sliderFaceSet(slider, SLIDER_FACE_LEFT, L"transform/slider_h_thick_left.png");
	sliderFaceSet(slider, SLIDER_FACE_RIGHT, L"transform/slider_h_thick_right.png");
	sliderFaceSet(slider, SLIDER_FACE_MID, L"transform/slider_h_thick_mid.png");
	sliderFaceSet(slider, SLIDER_FACE_TIP, L"transform/slider_h_thick_tip.png");
	ccSetMetrics(slider, x, y+vspace, width, height);
	sliderSetRange(slider, 0, 1000);
	sliderSetValue(slider, 500);
	tf->str[SLIDER_SCALE].slider = slider;
	ccEnable(slider);
	
	slider = ccCreate(vp->cc, PAGE_TRANSFORM, CC_SLIDER_HORIZONTAL, tfCcObject_cb, &vp->gui.ccIds[CCID_SLIDER_BRIGHTNESS], 0, 0);
	sliderFaceSet(slider, SLIDER_FACE_LEFT, L"transform/slider_h_thick_left.png");
	sliderFaceSet(slider, SLIDER_FACE_RIGHT, L"transform/slider_h_thick_right.png");
	sliderFaceSet(slider, SLIDER_FACE_MID, L"transform/slider_h_thick_mid.png");
	sliderFaceSet(slider, SLIDER_FACE_TIP, L"transform/slider_h_thick_tip.png");
	ccSetMetrics(slider, x, y+(vspace*2.0), width, height);
	sliderSetRange(slider, -255, 255);
	sliderSetValue(slider, 0);
	tf->str[SLIDER_BRIGHTNESS].slider = slider;
	ccEnable(slider);
	
	slider = ccCreate(vp->cc, PAGE_TRANSFORM, CC_SLIDER_HORIZONTAL, tfCcObject_cb, &vp->gui.ccIds[CCID_SLIDER_CONTRAST], 0, 0);
	sliderFaceSet(slider, SLIDER_FACE_LEFT, L"transform/slider_h_thick_left.png");
	sliderFaceSet(slider, SLIDER_FACE_RIGHT, L"transform/slider_h_thick_right.png");
	sliderFaceSet(slider, SLIDER_FACE_MID, L"transform/slider_h_thick_mid.png");
	sliderFaceSet(slider, SLIDER_FACE_TIP, L"transform/slider_h_thick_tip.png");
	ccSetMetrics(slider, x, y+(vspace*3.0), width, height);
	sliderSetRange(slider, 0, 10000);
	sliderSetValue(slider, 2500);
	tf->str[SLIDER_CONTRAST].slider = slider;
	ccEnable(slider);
	
	x += ((fw - x)/2) - 16;
		
	slider = ccCreate(vp->cc, PAGE_TRANSFORM, CC_SLIDER_HORIZONTAL, tfCcObject_cb, &vp->gui.ccIds[CCID_SLIDER_VIDEOBLUR], 0, 0);
	sliderFaceSet(slider, SLIDER_FACE_LEFT, L"transform/slider_h_thick_left.png");
	sliderFaceSet(slider, SLIDER_FACE_RIGHT, L"transform/slider_h_thick_right.png");
	sliderFaceSet(slider, SLIDER_FACE_MID, L"transform/slider_h_thick_mid.png");
	sliderFaceSet(slider, SLIDER_FACE_TIP, L"transform/slider_h_thick_tip.png");
	ccSetMetrics(slider, x, y, width, height);
	sliderSetRange(slider, 0, 64);
	sliderSetValue(slider, 0);
	tf->str[SLIDER_BLUR].slider = slider;
	ccEnable(slider);
	
	slider = ccCreate(vp->cc, PAGE_TRANSFORM, CC_SLIDER_HORIZONTAL, tfCcObject_cb, &vp->gui.ccIds[CCID_SLIDER_PIXELIZE], 0, 0);
	sliderFaceSet(slider, SLIDER_FACE_LEFT, L"transform/slider_h_thick_left.png");
	sliderFaceSet(slider, SLIDER_FACE_RIGHT, L"transform/slider_h_thick_right.png");
	sliderFaceSet(slider, SLIDER_FACE_MID, L"transform/slider_h_thick_mid.png");
	sliderFaceSet(slider, SLIDER_FACE_TIP, L"transform/slider_h_thick_tip.png");
	ccSetMetrics(slider, x, y+vspace, width, height);
	sliderSetRange(slider, 0, 15);
	sliderSetValue(slider, 0);
	tf->str[SLIDER_PIXELIZE].slider = slider;
	ccEnable(slider);

	slider = ccCreate(vp->cc, PAGE_TRANSFORM, CC_SLIDER_HORIZONTAL, tfCcObject_cb, &vp->gui.ccIds[CCID_SLIDER_SATURATION], 0, 0);
	sliderFaceSet(slider, SLIDER_FACE_LEFT, L"transform/slider_h_thick_left.png");
	sliderFaceSet(slider, SLIDER_FACE_RIGHT, L"transform/slider_h_thick_right.png");
	sliderFaceSet(slider, SLIDER_FACE_MID, L"transform/slider_h_thick_mid.png");
	sliderFaceSet(slider, SLIDER_FACE_TIP, L"transform/slider_h_thick_tip.png");
	ccSetMetrics(slider, x, y+(vspace*2.0), width, height);
	sliderSetRange(slider, 0, 10000);
	sliderSetValue(slider, 2500);
	tf->str[SLIDER_SATURATION].slider = slider;
	ccEnable(slider);
	
	slider = ccCreate(vp->cc, PAGE_TRANSFORM, CC_SLIDER_HORIZONTAL, tfCcObject_cb, &vp->gui.ccIds[CCID_SLIDER_GAMMA], 0, 0);
	sliderFaceSet(slider, SLIDER_FACE_LEFT, L"transform/slider_h_thick_left.png");
	sliderFaceSet(slider, SLIDER_FACE_RIGHT, L"transform/slider_h_thick_right.png");
	sliderFaceSet(slider, SLIDER_FACE_MID, L"transform/slider_h_thick_mid.png");
	sliderFaceSet(slider, SLIDER_FACE_TIP, L"transform/slider_h_thick_tip.png");
	ccSetMetrics(slider, x, y+(vspace*3.0), width, height);
	sliderSetRange(slider, 0, 10000);
	sliderSetValue(slider, 2500);
	tf->str[SLIDER_GAMMA].slider = slider;
	ccEnable(slider);

	//vp->vlc->rotateOp = ROTATE_BILINEAR;
	//vp->vlc->scaleOp = SCALE_BILINEAR;
	//tfReset(vp, tf);
	
	settingsGet(vp, "video.filter.rotate", &vp->vlc->rotateAngle);
	settingsGet(vp, "video.filter.scale", &vp->vlc->scaleFactor);
	settingsGet(vp, "video.filter.blur",	&vp->vlc->blurRadius);
	settingsGet(vp, "video.filter.pixelize", &vp->vlc->pixelize);
	settingsGet(vp, "video.filter.brightness", &vp->vlc->brightness);
	settingsGet(vp, "video.filter.contrast", &vp->vlc->contrast);
	settingsGet(vp, "video.filter.saturation", &vp->vlc->saturation);
	settingsGet(vp, "video.filter.gamma", &vp->vlc->gamma);
	settingsGet(vp, "video.filter.rotateOp", &vp->vlc->rotateOp);
	settingsGet(vp, "video.filter.scaleOp", &vp->vlc->scaleOp);
	if (vp->vlc->rotateOp < ROTATE_BILINEAR || vp->vlc->rotateOp > ROTATE_NEIGHBOUR)
		vp->vlc->rotateOp = ROTATE_BILINEAR;
	if (vp->vlc->scaleOp < SCALE_BILINEAR || vp->vlc->scaleOp > SCALE_NEIGHBOUR)
		vp->vlc->scaleOp = SCALE_BILINEAR;

	tfSetRotate(tf, vp->vlc->rotateAngle);
	tfSetScale(tf, vp->vlc->scaleFactor);
	tfSetBlur(tf, vp->vlc->blurRadius);
	tfSetPixelize(tf, vp->vlc->pixelize);
	tfSetBrightness(tf, vp->vlc->brightness);
	tfSetContrast(tf, vp->vlc->contrast);
	tfSetGamma(tf, vp->vlc->gamma);
	tfSetSaturation(tf, vp->vlc->saturation);
	
	
	THWD *hw = getFrontBuffer(vp)->hw;
	const int bpp = getFrontBuffer(vp)->bpp;

	lSetForegroundColour(hw, 0xFFFFFFFF);
	lSetBackgroundColour(hw, 0x00000000);
	outlineTextEnable(hw, 160<<24 | COL_BLUE_SEA_TINT);
	const int font = VTRANSFORM_FONT;
	
	tf->str[SLIDER_ROTATE].frame = lNewString(hw, bpp, PF_IGNOREFORMATTING, font, "Rotate");
	tf->str[SLIDER_SCALE].frame = lNewString(hw, bpp, PF_IGNOREFORMATTING, font, "Scale");
	tf->str[SLIDER_BLUR].frame = lNewString(hw, bpp, PF_IGNOREFORMATTING, font, "Blur");
	tf->str[SLIDER_PIXELIZE].frame = lNewString(hw, bpp, PF_IGNOREFORMATTING, font, "Pixelize");
	tf->str[SLIDER_BRIGHTNESS].frame = lNewString(hw, bpp, PF_IGNOREFORMATTING, font, "Brightness");
	tf->str[SLIDER_CONTRAST].frame = lNewString(hw, bpp, PF_IGNOREFORMATTING, font, "Contrast");
	tf->str[SLIDER_SATURATION].frame = lNewString(hw, bpp, PF_IGNOREFORMATTING, font, "Saturation");
	tf->str[SLIDER_GAMMA].frame = lNewString(hw, bpp, PF_IGNOREFORMATTING, font, "Gamma");
	
	outlineTextDisable(hw);
		
	for (int i = 0; i < SLIDER_TF_TOTAL; i++){
		tf->str[i].x = ccGetPositionX(tf->str[i].slider) + 16;
		tf->str[i].y = ccGetPositionY(tf->str[i].slider) - (height+8);
	}
	return 1;
}

static inline int page_tfInitalize (TTRANSFORM *tf, TVLCPLAYER *vp, const int width, const int height)
{
	setPageAccessed(vp, PAGE_TRANSFORM);
	return 1;
}

static inline int page_tfShutdown (TTRANSFORM *tf, TVLCPLAYER *vp)
{
	buttonsDeleteAll(tf->btns);
	
	//lDeleteFrame(tf->working);
	for (int i = 0; i < SLIDER_TF_TOTAL; i++){
		ccDelete(tf->str[i].slider);
		lDeleteFrame(tf->str[i].frame);
	}
	return 1;
}

int page_tfCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr)
{
	TTRANSFORM *tf = (TTRANSFORM*)pageStruct;
	
	// if (msg != PAGE_CTL_RENDER)
		// printf("# page_tfCallback: %p %i %I64d %I64d %p %p\n", pageStruct, msg, dataInt1, dataInt2, dataPtr, opaquePtr);
	
	if (msg == PAGE_CTL_RENDER){
		return page_tfRender(tf, tf->com->vp, dataPtr);

	}else if (msg == PAGE_CTL_RENDER_START){

	}else if (msg == PAGE_CTL_RENDER_END){
		
	}else if (msg == PAGE_CTL_INPUT){
		return page_tfInput(tf, dataInt1, dataInt2, dataPtr);
		
	}else if (msg == PAGE_CTL_STARTUP){
		return page_tfStartup(tf, tf->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_INITIALIZE){
		return page_tfInitalize(tf, tf->com->vp, dataInt1, dataInt2);
		
	}else if (msg == PAGE_CTL_SHUTDOWN){
		return page_tfShutdown(tf, tf->com->vp);
		
	}
	
	return 1;
}


