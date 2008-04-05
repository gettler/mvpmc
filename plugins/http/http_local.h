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

#ifndef HTTP_LOCAL_H
#define HTTP_LOCAL_H

typedef enum {
	HTTP_METHOD_GET = 1,
	HTTP_METHOD_HEAD,
	HTTP_METHOD_POST,
	HTTP_METHOD_PUT,
	HTTP_METHOD_DELETE,
	HTTP_METHOD_TRACE,
	HTTP_METHOD_CONNECT,
} http_method_t;

typedef struct {
	char *name;
	char *val;
} http_field_t;

typedef struct {
	char *url;
	char *data;
	http_field_t **req;
	http_field_t **resp;
	int fd;
	int error;
	int done;
	int code;
} http_req_t;

typedef struct {
	http_method_t method;
	char *path;
	int major;
	int minor;
	http_field_t **header;
} httpd_req_t;

extern int http_get(http_req_t *req, int block);

#define HTTP_PORT	8500

#endif /* HTTP_LOCAL_H */
