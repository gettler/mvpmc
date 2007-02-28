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

#ifndef IR_H
#define IR_H

typedef struct {
	unsigned int code;
	char *name;
} ir_key_t;

#define KEY_IR_OLD_POWER	0x003d0000
#define KEY_IR_OLD_GO		0x003b0000
#define KEY_IR_OLD_ONE		0x00010000
#define KEY_IR_OLD_TWO		0x00020000
#define KEY_IR_OLD_THREE	0x00030000
#define KEY_IR_OLD_FOUR		0x00040000
#define KEY_IR_OLD_FIVE		0x00050000
#define KEY_IR_OLD_SIX		0x00060000
#define KEY_IR_OLD_SEVEN	0x00070000
#define KEY_IR_OLD_EIGHT	0x00080000
#define KEY_IR_OLD_NINE		0x00090000
#define KEY_IR_OLD_ZERO		0x00000000
#define KEY_IR_OLD_BACK		0x001f0000
#define KEY_IR_OLD_MENU		0x000d0000
#define KEY_IR_OLD_RED		0x000b0000
#define KEY_IR_OLD_YELLOW	0x00380000	
#define KEY_IR_OLD_GREEN	0x002e0000
#define KEY_IR_OLD_BLUE		0x00290000
#define KEY_IR_OLD_UP		0x00200000
#define KEY_IR_OLD_DOWN		0x00210000
#define KEY_IR_OLD_LEFT		0x00110000
#define KEY_IR_OLD_RIGHT	0x00100000	
#define KEY_IR_OLD_OK		0x00250000
#define KEY_IR_OLD_MUTE		0x000f0000
#define KEY_IR_OLD_BLANK	0x000c0000	
#define KEY_IR_OLD_FULL		0x003c0000
#define KEY_IR_OLD_REWIND	0x00320000
#define KEY_IR_OLD_PLAY		0x00350000
#define KEY_IR_OLD_FFWD		0x00340000
#define KEY_IR_OLD_REC		0x00370000
#define KEY_IR_OLD_STOP		0x00360000
#define KEY_IR_OLD_PAUSE	0x00300000
#define KEY_IR_OLD_REPLAY	0x00240000
#define KEY_IR_OLD_SKIP		0x001e0000

#define KEY_IR_NEW_POWER	0x013d0000
#define KEY_IR_NEW_GO		0x013b0000
#define KEY_IR_NEW_TV		0x011c0000
#define KEY_IR_NEW_VIDEOS	0x01180000	
#define KEY_IR_NEW_MUSIC	0x01190000	
#define KEY_IR_NEW_PICTURES	0x011a0000	
#define KEY_IR_NEW_GUIDE	0x011b0000	
#define KEY_IR_NEW_RADIO	0x010c0000	
#define KEY_IR_NEW_UP		0x01140000
#define KEY_IR_NEW_DOWN		0x01150000
#define KEY_IR_NEW_LEFT		0x01160000
#define KEY_IR_NEW_RIGHT	0x01170000	
#define KEY_IR_NEW_OK		0x01250000
#define KEY_IR_NEW_BACK		0x011f0000
#define KEY_IR_NEW_MENU		0x010d0000
#define KEY_IR_NEW_VOLUP	0x01100000	
#define KEY_IR_NEW_VOLDOWN	0x01110000	
#define KEY_IR_NEW_CHANUP	0x01200000
#define KEY_IR_NEW_CHANDOWN	0x01210000
#define KEY_IR_NEW_PREVCHAN	0x01120000
#define KEY_IR_NEW_MUTE		0x010f0000
#define KEY_IR_NEW_REC		0x01370000
#define KEY_IR_NEW_STOP		0x01360000
#define KEY_IR_NEW_REPLAY	0x01320000	
#define KEY_IR_NEW_SKIP		0x01340000
#define KEY_IR_NEW_REWIND	0x01240000	
#define KEY_IR_NEW_FFWD		0x011e0000
#define KEY_IR_NEW_PLAY		0x01350000
#define KEY_IR_NEW_PAUSE	0x01300000	
#define KEY_IR_NEW_ONE		0x01010000
#define KEY_IR_NEW_TWO		0x01020000
#define KEY_IR_NEW_THREE	0x01030000
#define KEY_IR_NEW_FOUR		0x01040000
#define KEY_IR_NEW_FIVE		0x01050000
#define KEY_IR_NEW_SIX		0x01060000
#define KEY_IR_NEW_SEVEN	0x01070000
#define KEY_IR_NEW_EIGHT	0x01080000
#define KEY_IR_NEW_NINE		0x01090000
#define KEY_IR_NEW_ZERO		0x01000000
#define KEY_IR_NEW_TEXT		0x010a0000
#define KEY_IR_NEW_SUBTITLE	0x010e0000	
#define KEY_IR_NEW_RED		0x010b0000
#define KEY_IR_NEW_GREEN	0x012e0000
#define KEY_IR_NEW_YELLOW	0x01380000	
#define KEY_IR_NEW_BLUE		0x01290000

#endif /* FIP_H */
