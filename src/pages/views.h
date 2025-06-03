
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



#ifndef _VIEWS_H_
#define _VIEWS_H_

#include "../common.h"

#include "page_filepane.h"
#include "page_exppanel.h"

#include "page_album.h"
#include "page_keyboard.h"
#include "page_search.h"

#include "page_videotransform.h"
#include "page_ctrloverlay.h"

#include "page_meta.h"
#include "page_mediastats.h"
#include "page_textoverlay.h"
#include "page_chapter.h"
#include "page_es.h"
#include "page_imgovr.h"
#include "page_exit.h"
#include "page_cfg.h"
#include "page_sub.h"

#include "page_eq.h"
#include "page_alarm.h"
#include "page_epg.h"
#include "page_plytree.h"
#include "page_plypanel.h"
#include "page_plypane.h"
#include "page_imgpane.h"
#include "page_clock.h"
#include "page_tetris.h"
#include "page_taskman.h"
#include "page_home.h"
#include "page_player.h"

#if ENABLE_GARMINTCX
#include "page_garmin.h"
#endif
#if ENABLE_ANTPLUS
#include "page_antplus.h"
#endif



#endif
