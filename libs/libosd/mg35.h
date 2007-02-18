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

#ifndef OSD_MG35_H
#define OSD_MG35_H

#define IOCTL_RM_INIT	0x3e1

typedef struct {
	unsigned char *addr;
	int width;
	int height;
	int depth;
} rm_info_t;

typedef struct {
	unsigned long unknown;
	short width;
	short height;
	unsigned long palette[256];
	unsigned char data[0];
} rm_overlay_t;

typedef struct {
	int colors;
	unsigned long *palette;
	unsigned char *data;
} overlay_data_t;

#define overlay_create	fb_create
#define fb_create	__osd_overlay_create

extern osd_surface_t* overlay_create(int w, int h, unsigned long color);
extern int overlay_init(void);

#endif /* OSD_MG35_H */
