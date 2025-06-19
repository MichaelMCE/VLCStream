
// https://github.com/MichaelMCE/VLCStream

// okio@users.sourceforge.net

//  Copyright (c) 2005-2025
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




static int currentGestureState = 3;



lDISPLAY hiddGetLibmylcdDID (THWD *hw)
{
	lDISPLAY did = lDriverNameToID(hw, "HidDisplay", LDRV_DISPLAY);
	return did;
}

static inline int hiddReportRotary (const TTOUCHINPUT *sbg, void *ptr)
{
	rotary_t enc;
	enc.buttonPressed = sbg->x;
	enc.positionDelta = sbg->y;
	enc.encoderId = sbg->z;

	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;
	if (vp->applState){
		if (renderLock(vp)){
	  		if (vp->applState){
	  			wakeup(vp);
				const double t0 = getTime(vp);
	  			page2Input(vp->pages, PAGE_IN_ROTARY, &enc, PAGE_IN_ROTARY_CHANGE);
	  			vp->gui.inputCallLength = getTime(vp) - t0;
	  		}
	  		renderUnlock(vp);
	  	}
	}
	renderSignalUpdate(vp);
	return 1;
}

static inline int hiddReportTouch (const TTOUCHINPUT *sbg, void *ptr)
{
	if (!sbg || !ptr) return 0;	// shouldn't ever happen
	
	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;
	if (!vp->applState) return 0;
	if (!vp->renderState) return 1;

	static int btnId = 1024;	// todo: put these somewhere

	TTOUCHCOORD pos;
	pos.x = sbg->x;
	pos.y = sbg->y;
	pos.z1 = sbg->z;
	pos.z2 = sbg->z;
	pos.time = getTime(vp);// sbg->time;
	pos.dt = sbg->dt;
	pos.count = sbg->ct;
	pos.id = sbg->id;
	pos.pen = (sbg->z != 1);
	pos.pressure = 100;
	
	int pressState = 0;

	if (currentGestureState == 3){						// was up
		pressState = 1;									// but is now down
		pos.pen = 0;
		pos.id = ++btnId;
		pos.dt = 120;

	}else if (currentGestureState == 1 && !pos.pen){	// was down
		pressState = 2;									// is still down, so must be a drag
		pos.id = btnId;
		
	}else if (currentGestureState == 1 && pos.pen){		// was down
		pressState = 3;									// but is now up
		pos.id = btnId;
		
	}else if (currentGestureState == 2 && !pos.pen){	// was down, draging
		pressState = 2;									// is still draging
		pos.id = btnId;
		
	}else if (currentGestureState == 2 && pos.pen){		// was down, draging
		pressState = 3;									// but is now up
		pos.id = btnId;
	}

	currentGestureState = pressState;	
	
	switch (pressState){
	  case 1:
		touchSimulate(&pos, TOUCH_VINPUT|0, ptr);
		break;
	  case 2:
		touchSimulate(&pos, TOUCH_VINPUT|1, ptr);
		break;
	  case 3:
		touchSimulate(&pos, TOUCH_VINPUT|3, ptr);
		break;
	}
	return 1;
}

static inline int hiddTouchCB (const TTOUCHINPUT *sbg, void *ptr)
{
	if (!sbg || !ptr) return 0;	// shouldn't ever happen
	
	TVLCPLAYER *vp = (TVLCPLAYER*)ptr;
	if (!vp->applState) return 0;
	if (!vp->renderState) return 1;

	if (sbg->type == 1)		// 1 == touch, 2 == rotary
		return hiddReportTouch(sbg, ptr);
	else if (sbg->type == 2)		// 1 == touch, 2 == rotary
		return hiddReportRotary(sbg, ptr);

	return 1;
}

int hiddTouchCBEnable (TVLCPLAYER *vp)
{
	lDISPLAY did = hiddGetLibmylcdDID(vp->ml->hw);
	if (did){
		if (lSetDisplayOption(vp->ml->hw, did, lOPT_HIDD_UDATAPTR, (intptr_t*)vp)){
			if (lSetDisplayOption(vp->ml->hw, did, lOPT_HIDD_TOUCHCB, (intptr_t*)hiddTouchCB)){
				return 1;
			}
		}
	}else{

	}
	return 0;
}
