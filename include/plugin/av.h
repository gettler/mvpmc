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

#ifndef PLUGIN_AV_H
#define PLUGIN_AV_H

#include <gw.h>
#include <plugin/gw.h>

typedef enum {
	AV_EVENT_STOPPED,
	AV_EVENT_PLAYING,
	AV_EVENT_PAUSED,
	AV_EVENT_COMPLETE,
} av_event_t;

typedef struct plugin_av_s plugin_av_t;

typedef int (*av_cb_t)(plugin_av_t*, av_event_t);

struct plugin_av_s {
	int (*play_file)(char*, av_cb_t*);
	int (*play_dvd)(char*, av_cb_t*);
	int (*play_url)(char*, av_cb_t*);
	int (*play_list)(char**, av_cb_t*);
	int (*stop)(void);
	av_event_t (*get_event)(void);
};

#endif /* PLUGIN_AV_H */
