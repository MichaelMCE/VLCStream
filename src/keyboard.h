
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



#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_



typedef struct{
	TPAGE2COM *com;
	
	TKEYPAD *kp;
	TCCBUTTONS *btns;
}TKEYBOARD;


int page_vkbCallback (void *pageStruct, const int msg, int64_t dataInt1, int64_t dataInt2, void *dataPtr, void *opaquePtr);

TKEYPAD *keyboardGetKeypad (void *pageStruct);





#if 0

keypad.position.x
keypad.position.y
keypad.firstPad: 2		// set pad 2 as the primary pad
keypad.editbox.position.x
keypad.editbox.position.y
keypad.editbox.width
keypad.editbox.height
keypad.editbox.font
keypad.editbox.caret: caret char. 




key types:
	1	standard key with a unicode value.
	2	pad switch - go to a pad (eg; digits)
	3	pad toggle - toggle between two pads (eg; shift)
	4	pad cycle - cycle through given list of pads
	5	enter - input completed, send string to control and close keypad
	6	return - as above but leave control open
	7	backspace - delete whatever is left of the caret

examples:
			1, unicode, image path
keypad.key.1: 1, 32, space_32.png
keypad.key.2: 1, 65, A.png
keypad.key.3: 1, 66, B.png
keypad.key.4: 1, 67, C.png

			2, pad ids, image_path
keypad.key.5: 2, 3, digits.png					// switch to pad 3 when pressed

			3, pad ids<|>image_path
keypad.key.6: 3, <1,2>,shift.png				// go to and/or switch between pad 1 and 2

			4, pad ids<|>image_path
keypad.key.7: 4, <1,2,3,4,5>,padcycle.png		// cycle through all of the pads

			5, image_path
keypad.key.8: 5, enter_png

			6, image_path
keypad.key.9: 6, enter_png

			7, image_path
keypad.key.10: 7, del_png



keypad.pad.1: key1,<position>,key2,<position>etc..
keypad.pad.2: key33,<position>,key34,<position>etc..

example:
keypad.pad.1: 1,<15,145>,2,<85,145>,3,<135,145>
keypad.pad.2: 33,<15,145>,34,<85,145>,35,<135,145>




#endif


#endif


