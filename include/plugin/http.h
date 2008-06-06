/*
 *  Copyright (C) 2008, Jon Gettler
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

#ifndef PLUGIN_HTTP_H
#define PLUGIN_HTTP_H

#include <gw.h>
#include <input.h>
#include <plugin/gw.h>

typedef struct {
	int (*generate)(void);
	int (*update_widget)(gw_t*);
	int (*input_fd)(void);
	int (*get)(char*, int (*)(void*, char*, int), void*);
	int (*startd)(int);
	int (*stopd)(void);
} plugin_http_t;

typedef struct {
	gw_menu_t *menu;
	void *key;
	void *data;
	input_cmd_t input;
	void (*callback)(void*);
} plugin_http_cmd_t;

#endif /* PLUGIN_HTTP_H */
