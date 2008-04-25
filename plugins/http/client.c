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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>   
#include <sys/socket.h>   
#include <sys/signal.h>   
#include <fcntl.h>
#include <pthread.h>
#include <microhttpd.h>
#include <curl/curl.h>

#include <mvp_string.h>
#include <plugin.h>
#include <plugin/html.h>
#include <plugin/http.h>

#include "http_local.h"

typedef struct {
	char *data;
	int len;
} url_resp_t;

typedef struct {
	int (*callback)(void*, char*, int);
	void *data;
} url_cb_t;

static char agent[64];

static size_t
callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	url_resp_t *resp = (url_resp_t*)data;
	int len = size * nmemb;

	if ((resp->data=realloc(resp->data, resp->len+len)) != NULL) {
		memcpy(resp->data+resp->len, ptr, len);
		resp->len += len;
	}

	return len;
}

int
client_get_url(char *url, char **ret)
{
	CURL *handle;
	url_resp_t resp;

	if ((handle=curl_easy_init()) == NULL) {
		return -1;
	}

	memset(&resp, 0, sizeof(resp));

	curl_easy_setopt(handle, CURLOPT_USERAGENT, agent);
	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&resp);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, callback);

	curl_easy_perform(handle);

	curl_easy_cleanup(handle);

	*ret = resp.data;

	return resp.len;
}

static size_t
get_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
	url_cb_t *cb = (url_cb_t*)data;
	int len = size * nmemb;

	return cb->callback(cb->data, ptr, len);
}

int
client_get(char *url, int (*callback)(void*, char*, int), void *data)
{
	CURL *handle;
	url_cb_t cb;

	if ((handle=curl_easy_init()) == NULL) {
		return -1;
	}

	cb.callback = callback;
	cb.data = data;

	curl_easy_setopt(handle, CURLOPT_USERAGENT, agent);
	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&cb);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, get_cb);

	curl_easy_perform(handle);

	curl_easy_cleanup(handle);

	return 0;
}

int
client_init(void)
{
	snprintf(agent, sizeof(agent), "mvpmc (%s)", PLATFORM);

	curl_global_init(CURL_GLOBAL_ALL);

	return 0;
}

int
client_deinit(void)
{
	curl_global_cleanup();

	return 0;
}
