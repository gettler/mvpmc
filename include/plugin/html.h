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

#ifndef PLUGIN_HTML_H
#define PLUGIN_HTML_H

#include <gw.h>
#include <plugin.h>

#define HTML_RESP_SIZE	1024

typedef struct plugin_html_data_s {
	struct plugin_html_data_s *next;
	int offset;
	int len;
	int used;
	char data[HTML_RESP_SIZE];
} plugin_html_resp_t;

typedef struct {
	plugin_html_resp_t *(*generate)(void);
	int (*css)(int);
	int (*notfound)(int, char*, int);
	int (*update_widget)(gw_t*);
	unsigned int (*get_state)(void);
} plugin_html_t;

typedef int (*plugin_html_cb_t)(void *handle, char *buf, int len);

#endif /* PLUGIN_HTML_H */
