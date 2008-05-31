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

#define GW_DEV_OSD	0x0001
#define GW_DEV_HTTP	0x0002
#define GW_DEV_VFD	0x0004

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

typedef struct {
	char *name;
	gw_t *widget;
	gw_t *focus;
} gw_console_t;

#define ROOT_CONSOLE		"root"
#define VIDEO_CONSOLE		"video"
#define SCREENSAVER_CONSOLE	"screensaver"

typedef int (*gw_cmd_t)(gw_t*, int);
typedef int (*gw_select_t)(gw_t*, char*, void*);
typedef int (*gw_hilite_t)(gw_t*, char*, void*, bool);

extern int gw_init(void);
extern int gw_shutdown(void);
extern gw_t *gw_root(char *name);
extern int gw_device_add(unsigned int dev);
extern int gw_device_remove(unsigned int dev);

extern gw_t *gw_create_console(char *name);
extern int gw_set_console(char *name);
extern char *gw_get_console(void);
extern gw_console_t *gw_find_console(char *name);

extern gw_t* gw_create(gw_type_t type, gw_t *parent);

extern int gw_map(gw_t* widget);
extern int gw_unmap(gw_t* widget);
extern int gw_name_set(gw_t *widget, char *name);

extern int gw_focus_set(gw_t *widget);
extern int gw_focus_cb_set(gw_cmd_t input);

extern int gw_menu_title_set(gw_t *widget, char *title);
extern char* gw_menu_title_get(gw_t *widget);
extern int gw_menu_item_add(gw_t *widget, char *text, void *key,
			    gw_select_t select, gw_hilite_t hilite);
extern int gw_menu_input(gw_t *widget, int c);
extern int gw_menu_clear(gw_t *widget);

extern int gw_image_set(gw_t *widget, char *path);

extern int gw_text_set(gw_t *widget, char *text);

extern int gw_html_generate(int fd);

extern int gw_loop(struct timeval *to);
extern int gw_output(void);

extern int gw_ss_enable(void);
extern int gw_ss_disable(void);

#endif /* GW_H */
