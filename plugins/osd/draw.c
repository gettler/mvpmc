/*
 *  Copyright (C) 2007-2008, Jon Gettler
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

#include "osd_local.h"

static osd_surface_t *drawable;

int
draw_init(void)
{
	unsigned int bg = OSD_COLOR_BLACK;
	osd_surface_t *s;

	if ((s=osd_create_surface(screen_width, screen_height,
				  bg, OSD_GFX)) == NULL) {
		return -1;
	}

	if (osd_display_surface(s) < 0) {
		return -1;
	}

	drawable = s;

	return 0;
}

int
draw(osd_widget_t *widget)
{
	int y;
	int i;
	char *str;
	unsigned long c;
	int w = 0, h = 0;
	int cur = 0, vis = 0;

	if (!widget->visible) {
		return 0;
	}

	osd_get_surface_size(drawable, &w, &h);

	switch (widget->gw->type) {
	case GW_TYPE_TEXT:
		printf("Draw text!\n");
		osd_draw_text(drawable, widget->x, widget->y,
			      widget->gw->data.text->text,
			      OSD_COLOR_WHITE,
			      OSD_COLOR_BLACK,
			      OSD_COLOR_BLACK, NULL);
		break;
	case GW_TYPE_MENU:
		printf("Draw menu!\n");
		y = widget->x;
		str = widget->gw->data.menu->title;
		osd_draw_text(drawable, widget->x, y,
			      str,
			      OSD_COLOR_WHITE,
			      OSD_COLOR_BLUE,
			      OSD_COLOR_BLACK, NULL);
		i = 0;
		while (i < widget->gw->data.menu->n) {
			if (widget->gw->data.menu->items[i]->hilited) {
				cur = i;
			}
			i++;
		}
		vis = h / osd_font_height(NULL);
		printf("Menu items: %d total %d visible [cur %d]\n",
		       i, vis, cur);
		if (vis < widget->gw->data.menu->n) {
			i = cur;
		} else {
			i = 0;
		}
		while ((i < widget->gw->data.menu->n) && (y < h)) {
			y += osd_font_height(NULL);
			str = widget->gw->data.menu->items[i]->text;
			printf("Draw menu item '%s'!\n", str);
			if (widget->gw->data.menu->items[i]->hilited) {
				c = OSD_COLOR_GREEN;
			} else {
				c = OSD_COLOR_BLACK;
			}
			osd_draw_text(drawable, widget->x, y,
				      str,
				      OSD_COLOR_WHITE,
				      c,
				      OSD_COLOR_BLACK, NULL);
			i++;
		}
		break;
	case GW_TYPE_CONTAINER:
		printf("Draw container!\n");
		osd_fill_rect(drawable, widget->x, widget->y,
			      widget->width, widget->height,
			      OSD_COLOR_BLACK);
		break;
	default:
		break;
	}

	return 0;
}
