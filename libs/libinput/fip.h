/*
 *  Copyright (C) 2007, Jon Gettler
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef FIP_H
#define FIP_H

typedef struct {
	unsigned int code;
	char *name;
} fip_key_t;

#define KEY_FIP_UP	0xed12807f
#define KEY_FIP_DOWN	0xec13807f
#define KEY_FIP_LEFT	0xeb14807f
#define KEY_FIP_RIGHT	0xe916807f
#define KEY_FIP_ENTER	0xea15807f
#define KEY_FIP_POWER	0xff00807f
#define KEY_FIP_PLAY	0xb649807f
#define KEY_FIP_STOP	0xb748807f

#define KEY_IR_POWER	0xff007f80
#define KEY_IR_MENU	0xa7587f80
#define KEY_IR_TITLE	0xe8177f80
#define KEY_IR_SETUP	0xee117f80
#define KEY_IR_ANGLE	0xa6597f80
#define KEY_IR_REPEAT	0xa8577f80
#define KEY_IR_ABREPEAT	0xf40b7f80
#define KEY_IR_SLOW	0xb24d7f80
#define KEY_IR_FILEINFO	0xf6097f80
#define KEY_IR_TIMESRCH	0xa55a7f80
#define KEY_IR_BCONT	0xf30c7f80
#define KEY_IR_MEDIA	0xa35c7f80
#define KEY_IR_UP	0xed127f80
#define KEY_IR_DOWN	0xec137f80
#define KEY_IR_LEFT	0xeb147f80
#define KEY_IR_RIGHT	0xe9167f80
#define KEY_IR_ENTER	0xea157f80
#define KEY_IR_MAINPAGE	0xe41b7f80
#define KEY_IR_PLAY	0xb6497f80
#define KEY_IR_STOP	0xb7487f80
#define KEY_IR_REWIND	0xef107f80
#define KEY_IR_FF	0xb14e7f80
#define KEY_IR_PREV	0xe01f7f80
#define KEY_IR_NEXT	0xbc437f80
#define KEY_IR_VIDEO	0xf9067f80
#define KEY_IR_AUDIO	0xf8077f80
#define KEY_IR_SUBTITLE	0xfd027f80
#define KEY_IR_SCRSIZE	0xfc037f80
#define KEY_IR_ONE	0xe31c7f80
#define KEY_IR_TWO	0xe21d7f80
#define KEY_IR_THREE	0xe11e7f80
#define KEY_IR_FOUR	0xbf407f80
#define KEY_IR_FIVE	0xbe417f80
#define KEY_IR_SIX	0xbd427f80
#define KEY_IR_SEVEN	0xbb447f80
#define KEY_IR_EIGHT	0xba457f80
#define KEY_IR_NINE	0xb9467f80
#define KEY_IR_ZERO	0xb8477f80
#define KEY_IR_VOLUP	0xb54a7f80
#define KEY_IR_VOLDOWN	0xf7087f80
#define KEY_IR_MUTE	0xa9567f80
#define KEY_IR_CANCEL	0xfa057f80

#endif /* FIP_H */
