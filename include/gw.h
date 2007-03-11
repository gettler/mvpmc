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

#ifndef GW_H
#define GW_H

#if !defined(__cplusplus) && !defined(HAVE_TYPE_BOOL)
#define HAVE_TYPE_BOOL
/**
 * Boolean type.
 */
typedef enum {
	false = 0,
	true = 1
} bool;
#endif /* !__cplusplus && !HAVE_TYPE_BOOL */

typedef struct gw_s gw_t;

typedef enum {
	/* GUI types */
	GW_TYPE_CONTAINER = 1,
	GW_TYPE_MENU,
	GW_TYPE_TEXT,
	GW_TYPE_TABLE,
	GW_TYPE_IMAGE,
	GW_TYPE_GRAPH,
	GW_TYPE_DIALOG,
	GW_TYPE_SURFACE,
	/* special types */
	GW_TYPE_POINTER = 1001,
	GW_TYPE_COMMAND,
	GW_TYPE_SCREENSAVER,
	GW_TYPE_BUSY,
} gw_type_t;

extern int gw_init(void);
extern int gw_shutdown(void);
extern gw_t *gw_root(void);

extern gw_t* gw_create(gw_type_t type, gw_t *parent);

extern int gw_realize(gw_t* widget);
extern int gw_name_set(gw_t *widget, char *name);

extern int gw_menu_title_set(gw_t *widget, char *title);
extern int gw_menu_item_add(gw_t *widget, char *text);

extern int gw_html_generate(int fd);

#endif /* GW_H */
