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

#ifndef GW_LOCAL_H
#define GW_LOCAL_H

#include <plugin/gw.h>
#include <plugin/osd.h>
#include <plugin/http.h>
#include <plugin/screensaver.h>

#define SS_TIMEOUT	(1*60)

#define root		__gw_root
#define commands	__gw_commands
#define osd		__gw_osd
#define http		__gw_http
#define ss		__gw_ss
#define update		__gw_update
#define focus_input	__gw_focus_input
#define pipefds		__gw_pipefds

extern gw_t *root;
extern plugin_osd_t *osd;
extern plugin_http_t *http;
extern plugin_screensaver_t *ss;
extern gw_cmd_t focus_input;

extern int update(gw_t *widget);

extern int pipefds[2];

#endif /* GW_LOCAL_H */
