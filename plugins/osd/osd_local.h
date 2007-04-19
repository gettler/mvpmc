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

#ifndef OSD_LOCAL_H
#define OSD_LOCAL_H

#include <mvp_osd.h>
#include <plugin_gw.h>

#define CSS_DEFAULT	"/usr/share/mvpmc/osd.css"

#define osd_css_load	__osd_css_load
#define osd_css_arrange	__osd_css_arrange
#define wlist		__osd_wlist
#define hlist		__osd_hlist
#define list_add	__osd_list_add
#define list_remove	__osd_list_remove
#define draw		__osd_draw
#define drawable	__osd_drawable
#define screen_width	__osd_screen_width
#define screen_height	__osd_screen_height
#define lookup		__osd_lookup
#define css_init	__osd_css_init

typedef struct osd_widget_s {
	struct osd_widget_s *next;
	struct osd_widget_s *child;
	struct osd_widget_s *parent;/* list this widget is on */
	gw_t *gw;
	mvp_atomic_t generation;
	bool dirty;
	bool visible;			/* visible from CSS point of view */
	osd_surface_t *surface;
	int x;
	int y;
	int width;
	int height;
	unsigned int bg;
} osd_widget_t;

extern osd_widget_t *wlist;
extern osd_widget_t *hlist;

extern int css_init(void);

extern int osd_css_load(char *path);
extern int osd_css_arrange(gw_t *gw);

extern int list_add(osd_widget_t **head, gw_t *gw, osd_widget_t *widget);
extern osd_widget_t *list_remove(osd_widget_t **head, gw_t *gw);

extern int draw(osd_widget_t *widget);
extern int draw_init(void);

extern int screen_width;
extern int screen_height;

extern osd_widget_t* lookup(gw_t *widget);

#endif /* OSD_LOCAL_H */
