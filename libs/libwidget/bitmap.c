/*
 *  Copyright (C) 2004, Jon Gettler
 *  http://mvpmc.sourceforge.net/
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

#ident "$Id$"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "mvp_widget.h"
#include "widget.h"

static void
expose(mvp_widget_t *widget)
{
	int x, y, i;
	GR_GC_ID gc[4];
	char *image;
	int count[4];
	int alpha;

	memset(count, 0, sizeof(count));

	image = widget->data.bitmap.image;

	for (y=0; y<widget->height; y++) {
		for (x=0; x<widget->width; x++) {
			count[(int)image[(y*widget->width)+x]]++;
		}
	}

	for (i=0; i<4; i++) {
		gc[i] = GrNewGC();
		GrSetGCBackground(gc[i], 0);
	}

	if (count[1] > count[0]) {
		GrSetGCForeground(gc[0], 0xffffffff);
		GrSetGCForeground(gc[1], 0);
		alpha = 1;
	} else {
		GrSetGCForeground(gc[0], 0);
		GrSetGCForeground(gc[1], 0xffffffff);
		alpha = 0;
	}
	GrSetGCForeground(gc[2], 0xff808080);
	GrSetGCForeground(gc[3], 0xff008080);

	for (y=0; y<widget->height; y++) {
		for (x=0; x<widget->width; x++) {
			if (alpha != image[(y*widget->width)+x])
				GrPoint(widget->wid,
					gc[(int)image[(y*widget->width)+x]],
					x, y);
		}
	}

	for (i=0; i<4; i++)
		GrDestroyGC(gc[i]);
}

static void
destroy(mvp_widget_t *widget)
{
	if (widget->data.bitmap.image)
		free(widget->data.bitmap.image);
}

mvp_widget_t*
mvpw_create_bitmap(mvp_widget_t *parent,
		   int x, int y, int w, int h,
		   uint32_t bg, uint32_t border_color, int border_size)
{
	mvp_widget_t *widget;

	widget = mvpw_create(parent, x, y, w, h, bg,
			     border_color, border_size);

	if (widget == NULL)
		return NULL;

	GrSelectEvents(widget->wid, widget->event_mask);

	widget->type = MVPW_BITMAP;
	widget->expose = expose;
	widget->destroy = destroy;

	memset(&widget->data, 0, sizeof(widget->data));

	return widget;
}

int
mvpw_set_bitmap(mvp_widget_t *widget, mvpw_bitmap_attr_t *bitmap)
{
	if ((widget->data.bitmap.image=
	     malloc(widget->width*widget->height)) == NULL)
		return -1;

	memcpy(widget->data.bitmap.image, bitmap->image,
	       widget->width*widget->height);

	return 0;
}
