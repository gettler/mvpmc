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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "plugin.h"
#include "plugin_osd.h"

#include "osd_local.h"
#include "css.h"

static int
center(int width, int height, int *x, int *y)
{
	*x = (screen_width - width) / 2;
	*y = (screen_height - height) / 2;

	return 0;
}

int
osd_css_load(char *path)
{
	return -1;
}

static int
locate_widget(osd_widget_t *widget)
{
	int w, h, x, y;

	if (!widget->visible) {
		return -1;
	}

	switch (widget->gw->type) {
	case GW_TYPE_TEXT:
		w = osd_font_width(NULL, widget->gw->data.text->text);
		h = osd_font_height(NULL);
		center(w, h, &x, &y);
		widget->x = x;
		widget->y = y;
		break;
	case GW_TYPE_CONTAINER:
		widget->x = 0;
		widget->y = 0;
		widget->width = screen_width;
		widget->height = screen_height;
		break;
	case GW_TYPE_MENU:
		widget->x = 50;
		widget->y = 50;
		break;
	default:
		return -1;
	}

	return 0;
}

int
osd_css_arrange(gw_t *gw)
{
	int count = 0;
	osd_widget_t *w;

	w = lookup(gw);

	if (locate_widget(w) == 0) {
		count++;

		w = w->child;

		while (w) {
			count += osd_css_arrange(w->gw);
			w = w->next;
		}
	}

	printf("CSS: %d visible widgets\n", count);

	return count;
}
