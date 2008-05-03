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

#ifndef PLUGIN_GW_H
#define PLUGIN_GW_H

#include <plugin.h>
#include <mvp_refmem.h>
#include <gw.h>

typedef enum {
	GW_EVENT_EXPOSE =	0x01,
	GW_EVENT_KEYPRESS =	0x02,
	GW_EVENT_TIMER =	0x04,
	GW_EVENT_MOUSE =	0x08,
	GW_EVENT_FDINPUT =	0x10,
} gw_event_t;

typedef struct {
	gw_t *child;
} gw_container_t;

typedef struct {
	char *text;
	bool selectable;
	bool checked;
	bool hilited;
	gw_select_t select;
	gw_hilite_t hilite;
	void *key;
} gw_menu_item_t;

typedef struct {
	char *title;
	int n;
	gw_menu_item_t **items;
} gw_menu_t;

typedef struct {
	char *text;
} gw_text_t;

typedef struct {
	char *title;
	int nx;
	int ny;
	char **xlabels;
	char **ylabels;
	char **elements;
} gw_table_t;

typedef struct {
	char *path;
	int width;
	int height;
} gw_image_t;

typedef struct {
	int min;
	int max;
	int cur;
} gw_graph_t;

typedef struct {
	char *title;
	char *text;
} gw_dialog_t;

typedef struct {
	int width;
	int height;
} gw_surface_t;

typedef struct {
	char *path;
} gw_pointer_t;

typedef struct {
	char *text;
	char *command;
} gw_command_t;

typedef struct {
	int timeout;
} gw_screensaver_t;

struct gw_s {
	gw_type_t type;
	char *name;
	bool realized;
	bool updating;
	struct gw_s *next;
	struct gw_s *prev;
	struct gw_s *parent;
	struct gw_s *above;
	struct gw_s *below;
	gw_event_t event_mask;
	mvp_atomic_t generation;
	union {
		gw_container_t *container;
		gw_menu_t *menu;
		gw_text_t *text;
		gw_table_t *table;
		gw_graph_t *graph;
		gw_dialog_t *dialog;
		gw_pointer_t *pointer;
		gw_command_t *command;
		gw_screensaver_t *screensaver;
		gw_image_t *image;
	} data;
};

#endif /* PLUGIN_GW_H */
