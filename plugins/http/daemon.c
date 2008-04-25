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

#include <mvp_string.h>
#include <plugin.h>
#include <plugin/html.h>
#include <plugin/http.h>

#include "http_local.h"

#define NOT_FOUND \
	"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">" \
	"<html><head>" \
	"<title>404 Not Found</title>" \
	"</head><body>" \
	"<h1>Not Found</h1>" \
	"<p>The requested URL %s was not found on this server.</p>" \
	"<hr>" \
	"<address>mvpmc (%s) Server at %s Port %d</address>" \
	"</body></html>"

#define CSS_DEFAULT \
	"body { background: green; color: white; }\n" \
	"#root { background: black; color: white; }\n" \
	"#commands { background: white; color: red; }\n" \
	"a { text-decoration: none; }\n" \
	"a, a:link, a:visited, a:acted { color: white; }\n" \
	"a:hover { background: green; color: white; }\n" \
	".title { background: blue; color: white; }\n"

typedef struct {
	const char *state;
	const char *menu;
	const char *key;
	const char *page;
} get_data_t;

static struct MHD_Daemon *mhd;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static volatile int port;

#if defined(MVPMC_MG35)
static pthread_t thread;
#endif

static int
get_args(void *cls, enum MHD_ValueKind kind,
	 const char *key, const char *value)
{
	get_data_t *data = (get_data_t*)cls;

	if (strcmp(key, "state") == 0) {
		data->state = (void*)strtoul(value, NULL, 0);
	} else if (strcmp(key, "menu") == 0) {
		data->menu = (void*)strtoul(value, NULL, 0);
	} else if (strcmp(key, "page") == 0) {
		data->page = value;
	} else if (strcmp(key, "key") == 0) {
		data->key = (void*)strtoul(value, NULL, 0);
	} else {
		return MHD_NO;
	}

	return MHD_YES;
}

static int
text_response(struct MHD_Connection *connection, char *text, int must_free)
{
	struct MHD_Response *response;
	int ret;

	response = MHD_create_response_from_data(strlen(text), text,
						 must_free, MHD_NO);

	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);

	return ret;
}

static int
notfound(struct MHD_Connection *connection, const char *url)
{
	struct MHD_Response *response;
	char *str;
	int len, ret;
	char host[32];

	len = strlen(NOT_FOUND) + strlen(url) + 128;

	if ((str=malloc(len)) == NULL) {
		return -1;
	}

	gethostname(host, sizeof(host));
	len = snprintf(str, len, NOT_FOUND, url, PLATFORM, host, port);

	response = MHD_create_response_from_data(len, str, MHD_YES, MHD_NO);

	ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
	MHD_destroy_response(response);

	return ret;
}

static int
resp_callback(void *cls, size_t pos, char *buf, int max)
{
	plugin_html_resp_t *resp = (plugin_html_resp_t*)cls;
	int len, remain;
	int offset = 0;

	while ((resp->offset+resp->len) <= pos) {
		offset += resp->len;
		resp = resp->next;
	}

	remain = (resp->len - (pos - offset));
	len = (max > remain) ? remain : max;

	if (len > 0) {
		memcpy(buf, resp->data, len);
	}

	return len;
}

static void
resp_free(void *cls)
{
	plugin_html_resp_t *resp = (plugin_html_resp_t*)cls;
	plugin_html_resp_t *next;

	printf("%s(): %d\n", __FUNCTION__, __LINE__);

	while (resp) {
		next = resp->next;
		if (resp->data)
			free(resp->data);
		free(resp);
		resp = next;
	}
}

static int
send_response(struct MHD_Connection *connection, plugin_html_resp_t *resp)
{
	struct MHD_Response *response;
	int ret;
	int len = 0;
	plugin_html_resp_t *cur = resp;

	while (cur) {
		len += cur->len;
		cur = cur->next;
	}

	response = MHD_create_response_from_callback(len, 1024, resp_callback,
						     (void*)resp, resp_free);

	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);

	return ret;
}

static void
cmd_callback(void *arg)
{
	pthread_cond_t *cond = (pthread_cond_t*)arg;

	pthread_mutex_lock(&mutex);
	pthread_cond_broadcast(cond);
	pthread_mutex_unlock(&mutex);
}

static int
send_command(struct MHD_Connection *connection, get_data_t *data)
{
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	plugin_http_cmd_t *cmd;
	unsigned long addr;
	plugin_html_resp_t *resp;
	int ret;

	if ((cmd=(plugin_http_cmd_t*)malloc(sizeof(*cmd))) == NULL) {
		return MHD_NO;
	}

	cmd->menu = (gw_menu_t*)data->menu;
	cmd->key = (void*)data->key;
	cmd->data = (void*)&cond;
	cmd->callback = cmd_callback;

	printf("COMMAND: %p %p\n", data->menu, data->key);

	addr = (unsigned long)cmd;

	pthread_mutex_lock(&mutex);
	write(pipefds[1], &addr, sizeof(addr));
	pthread_cond_wait(&cond, &mutex);
	pthread_mutex_unlock(&mutex);

	printf("COMMAND complete\n");

	resp = html->generate();
	ret = send_response(connection, resp);

	return ret;
}

static int
callback(void *cls, struct MHD_Connection *connection,
	 const char *url, const char *method, const char *version,
	 const char *upload_data, unsigned int *upload_data_size, void **ptr)
{
	plugin_html_resp_t *resp = NULL;
	get_data_t data;
	int state = 0;
	int ret = MHD_NO;

	printf("%s(): %d\n", __FUNCTION__, __LINE__);

	memset(&data, 0, sizeof(data));
	data.page = url;

	MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND,
				  get_args, &data);

	printf("%s(): %d\n", __FUNCTION__, __LINE__);
	if (data.state) {
		printf("data.state %p\n", data.state);
		state = (int)data.state;
	}

	printf("%s(): %d\n", __FUNCTION__, __LINE__);
	if (strcmp(url, "/web.css") == 0) {
		ret = text_response(connection, CSS_DEFAULT, MHD_NO);
	} else if (strcmp(url, "/cmd.html") == 0) {
		ret = send_command(connection, &data);
	} else if ((strcmp(url, "/") == 0) ||
		   (strcmp(url, "/osd.html") == 0)) {
		if (!state || (state != html->get_state())) {
			printf("=== REGENERATE ===\n");
			resp = html->generate();
			ret = send_response(connection, resp);
		} else {
			ret = send_command(connection, &data);
		}
	} else {
		ret = notfound(connection, url);
	}

	if (resp && (ret == MHD_NO)) {
		resp_free((void*)resp);
	}

	return ret;
}

#if defined(MVPMC_MG35)
static void*
httpd_thread(void *arg)
{
	fd_set fdsr, fdsw, fdse;
	int max;

	mhd = MHD_start_daemon(0, port,
			       NULL, NULL, &callback, NULL, MHD_OPTION_END);

	while (1) {
		struct timeval tv;
		unsigned long long ms = 0;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		if (MHD_get_timeout(mhd, &ms) == MHD_YES) {
			tv.tv_sec = 0;
			tv.tv_usec = ms * 1000;
		}

		max = 0;
		FD_ZERO(&fdsr);
		MHD_get_fdset(mhd, &fdsr, &fdsw, &fdse, &max);
		select(max+1, &fdsr, NULL, NULL, &tv);
		MHD_run(mhd);
	}

	return NULL;
}
#endif /* MVPMC_MG35 */

int
httpd_start(int p)
{
	if (port != 0) {
		return -1;
	}

	if (p == 0) {
		port = HTTPD_PORT;
	} else {
		port = p;
	}

	printf("starting httpd daemon on port %d\n", port);

#if defined(MVPMC_MG35)
	pthread_create(&thread, NULL, httpd_thread, NULL);
#else
	mhd = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port,
			       NULL, NULL, &callback, NULL, MHD_OPTION_END);
#endif

	return 0;
}

int
httpd_stop(void)
{
#if defined(MVPMC_MG35)
	pthread_cancel(thread);
#else
	MHD_stop_daemon(mhd);
#endif

	port = 0;

	return 0;
}
