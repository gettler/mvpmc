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

#include <mvp_string.h>
#include <plugin.h>
#include <plugin/html.h>
#include <plugin/http.h>

#include "http_local.h"

#define HTTPD_SERVER \
	"Server: mvpmc (%s)\r\n"

#if defined(MVPMC_MEDIAMVP)
#define PLATFORM	"MediaMVP"
#elif defined(MVPMC_NMT)
#define PLATFORM	"Networked Media Tank"
#elif defined(MVPMC_MG35)
#define PLATFORM	"Mediagate MG-35"
#elif defined(MVPMC_HOST)
#define PLATFORM	"host"
#else
#error unknown platform
#endif

#if defined(PLUGIN_SUPPORT)
unsigned long plugin_version = CURRENT_PLUGIN_VERSION;
#endif

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t thread, threadd;

#define NLIST	32

static volatile http_req_t *list[NLIST];

static int httpd_fd = -1;

static plugin_html_t *html;

static char *httpd_server;

static int pipefds[2];

static volatile int waiting;

#if defined(MVPMC_MG35)
static int http_generate(void);
static volatile int cleanup;
static pid_t httpd_pid;
#endif

static httpd_req_t*
resp_header(int fd)
{
	httpd_req_t *req = NULL;
	char buf[2048];
	char data[2048];
	char *p;
	int n, i;

	while (1) {
		char *p;

		if ((n=recv(fd, buf, sizeof(buf)-1, MSG_PEEK)) == 0) {
			usleep(1000);
			continue;
		}

		if (n < 0) {
			perror("recv()");
			goto err;
		}

		if (n > 0)
			buf[n] = '\0';

		if ((p=strstr(buf, "\r\n\r\n")) != NULL) {
			n = recv(fd, data, p-buf+4, 0);
			data[n] = '\0';
			break;
		} else {
			if (n == (sizeof(buf)-1)) {
				printf("ERROR: %s():%d\n", __FUNCTION__, __LINE__);
				goto err;
			}
			usleep(1000);
		}

		if (n < 0) {
			perror("recv()");
			printf("ERROR: %s():%d  n %d\n", __FUNCTION__, __LINE__, n);
			goto err;
		}

	}

	if ((req=(httpd_req_t*)malloc(sizeof(*req))) == NULL) {
		printf("ERROR: %s():%d\n", __FUNCTION__, __LINE__);
		goto err;
	}
	memset(req, 0, sizeof(*req));

	i = 0;
	p = data;

	while (1) {
		char *l, *s;

		l = p;
		if ((p=strchr(p, '\n')) == NULL)
			break;
		*(p++) = '\0';

		if (i == 0) {
			char method[16], path[128];
			int major, minor;
			if (sscanf(l, "%s %s HTTP/%d.%d", method, path,
				   &major, &minor) == 4) {
				if (strcmp(method, "GET") == 0) {
					req->method = HTTP_METHOD_GET;
				} else {
					printf("ERROR: %s():%d\n", __FUNCTION__, __LINE__);
					goto err;
				}

				req->path = strdup(path);
				req->major = major;
				req->minor = minor;

				printf("%s\n", method);
				printf("%s\n", path);
			}
		} else {
			if ((s=strchr(l, ':')) != NULL) {
				*(s++) = '\0';
				printf("%s:%s\n", l, s);
			}
		}

		i++;
	}

	return req;

 err:
	if (req)
		free(req);

	return NULL;
}

static void
build_field(http_req_t *req, char *buf, int n, char *name, char *val)
{
	int i = 0;

	if (req->req) {
		while (req->req[i]) {
			if (strcasecmp(name, req->req[i]->name) == 0) {
				snprintf(buf, n, "%s: %s\r\n",
					 req->req[i]->name, req->req[i]->val);
				return;
			}
			i++;
		}
	}

	snprintf(buf, n, "%s: %s\r\n", name, val);
}

static void
do_request(http_req_t *req)
{
	char name[256], path[256], buf[256];
	char *data, *line;
	int len = 0, rc, n;

	if ((data=malloc(2048)) == NULL) {
		goto err;
	}

	path[0] = '\0';
	rc = sscanf(req->url, "http://%[^:/]%s", name, path);

	if (rc == 0) {
		fprintf(stderr, "url error!\n");
		goto err;
	}

	if (path[0] == '\0') {
		strcpy(path, "/");
	}

	if (req->data) {
		len = strlen(req->data);
	}

	if (req->fd < 0) {
		struct sockaddr_in addr;
		struct hostent* host;
		int sock, rc;

		if ((sock=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("socket()");
			goto err;
		}
		addr.sin_family = AF_INET;    
		addr.sin_port = htons(80);           
		host = gethostbyname(name);
		memcpy ((char *) &addr.sin_addr,
			(char *) host->h_addr,
			host->h_length);
		rc = connect(sock, (struct sockaddr *)&addr, sizeof(addr));

		if (rc != 0) {
			perror("connect()");
			goto err;
		}

		req->fd = sock;
	}

	snprintf(buf, sizeof(buf), "GET %s HTTP/1.0\r\n", path);
	printf("%s", buf);
	if (send(req->fd, buf, strlen(buf), 0) != strlen(buf)) {
		goto err;
	}

	build_field(req, buf, sizeof(buf), "Connection", "close");
	printf("%s", buf);
	if (send(req->fd, buf, strlen(buf), 0) != strlen(buf)) {
		goto err;
	}

	build_field(req, buf, sizeof(buf), "User-Agent", "mvpmc");
	printf("%s", buf);
	if (send(req->fd, buf, strlen(buf), 0) != strlen(buf)) {
		goto err;
	}

	snprintf(buf, sizeof(buf), "Length: %d\r\n", len);
	printf("%s", buf);
	if (send(req->fd, buf, strlen(buf), 0) != strlen(buf)) {
		goto err;
	}

	snprintf(buf, sizeof(buf), "\r\n");
	printf("%s", buf);
	if (send(req->fd, buf, strlen(buf), 0) != strlen(buf)) {
		goto err;
	}

	while (1) {
		char *p;

		n = recv(req->fd, buf, 2048-1, MSG_PEEK);

		if ((p=strstr(buf, "\r\n\r\n")) != NULL) {
			n = recv(req->fd, buf, p-buf+4, 0);
			buf[n] = '\0';
			break;
		} else {
			if (n == (2048-1)) {
				goto err;
			}
			usleep(1000);
		}

		if (n <= 0) {
			goto err;
		}
	}

	if ((line=strchr(buf, '\n')) == NULL) {
		goto err;
	}
	line++;
	while (line) {
		char *p, *s;
		p = strchr(line, '\r');
		if (p) {
			*p = '\0';
			p += 2;
		}
		s = strchr(line, ':');
		if (s == NULL) {
			break;
		}
		*(s++) = '\0';
		while (*s == ' ') {
			s++;
		}
		printf("  %s: %s\n", line, s);
		line = p;
	}

	req->done = 1;

	free(data);

	return;

 err:
	fprintf(stderr, "%s(): error\n", __FUNCTION__);
	close(req->fd);
	req->error = 1;
	req->done = 1;

	if (data)
		free(data);

	return;
}

static void*
http_thread(void *arg)
{
	pthread_mutex_lock(&mutex);
	while (1) {
		pthread_cond_wait(&cond, &mutex);
		if (list[0] != NULL) {
			do_request((http_req_t*)list[0]);
		}
		pthread_cond_broadcast(&cond);
	}

	return NULL;
}

int
http_get(http_req_t *req, int block)
{
	int i;
	int rc = -1;

	pthread_mutex_lock(&mutex);
	pthread_cond_broadcast(&cond);

	req->done = 0;

	for (i=0; i<NLIST; i++) {
		if (list[i] == NULL) {
			list[i] = req;
			rc = 0;
			break;
		}
	}

	if ((block) && (rc == 0)) {
		while (!req->done) {
			pthread_cond_wait(&cond, &mutex);
		}

		if (req->fd >= 0) {
			rc = 0;
		}
	}

	list[i] = NULL;

	pthread_mutex_unlock(&mutex);

	return rc;
}

static void
httpd_header(int fd, int code)
{
	char line[128];
	char *text;

	switch (code) {
	case 200:
		text = "OK";
		break;
	case 404:
		text = "Not Found";
		break;
	default:
		return;
	}

	snprintf(line, sizeof(line), "HTTP/1.0 %d %s\r\n", code, text);
	write(fd, line, strlen(line));

	write(fd, httpd_server, strlen(httpd_server));

	write(fd, "\r\n\r\n", 4);
}

static unsigned int
get_state(char *url)
{
	char *p;
	int ret = 0;

	if ((p=strstr(url, "&state=")) != NULL) {
		p += 7;
		ret = strtoul(p, NULL, 0);
	}

	return ret;
}

static unsigned int
get_menu(char *url)
{
	char *p;
	int ret = 0;

	if ((p=strstr(url, "?menu=")) != NULL) {
		p += 6;
		ret = strtoul(p, NULL, 0);
	}

	return ret;
}

static unsigned int
get_key(char *url)
{
	char *p;
	int ret = 0;

	if ((p=strstr(url, "&key=")) != NULL) {
		p += 5;
		ret = strtoul(p, NULL, 0);
	}

	return ret;
}

static char*
get_page(char *url)
{
	char *u, *p;

	if ((u=strdup(url)) == NULL) {
		return NULL;
	}

	if ((p=strchr(u, '?')) != NULL) {
		*p = '\0';
	}

	return u;
}

static int
send_command(int fd, unsigned int menu, unsigned int key, unsigned int state)
{
	plugin_http_cmd_t *cmd;
	unsigned int addr;

	if ((cmd=(plugin_http_cmd_t*)malloc(sizeof(*cmd))) == NULL) {
		return -1;
	}

	cmd->menu = (gw_menu_t*)menu;
	cmd->key = (void*)key;
	cmd->fd = fd;

	printf("COMMAND at 0x%.8x: 0x%.8x 0x%.8x %d\n", state, menu, key, fd);

	addr = (unsigned int)cmd;

	write(pipefds[1], &addr, sizeof(addr));

	return 0;
}

static void*
httpd_thread(void *arg)
{
	int reuse = 1;
	struct sockaddr_in addr;
	fd_set fds;
	char *header;

#if defined(MVPMC_MG35)
	httpd_pid = getpid();
#endif

	header = HTTPD_SERVER;
	if ((httpd_server=malloc(strlen(header)+128)) == NULL) {
		goto err;
	}
	sprintf(httpd_server, header, PLATFORM);

	if ((httpd_fd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		goto err;
	}
	setsockopt(httpd_fd, SOL_SOCKET, SO_REUSEADDR,
		   (const void *)&reuse , sizeof(int));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((unsigned short)HTTP_PORT);

	if (bind(httpd_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind()");
		goto err;
	}

	if (listen(httpd_fd, 5) < 0) {
		perror("listen()");
		goto err;
	}

	while (1) {
		socklen_t len;

#if defined(MVPMC_MG35)
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
#define TIMEOUT &tv

		if (cleanup) {
			cleanup = 0;
			http_generate();
		}
#else
#define TIMEOUT NULL
#endif

		FD_ZERO(&fds);
		FD_SET(httpd_fd, &fds);

		if (select(httpd_fd+1, &fds, NULL, NULL, TIMEOUT) > 0) {
			struct sockaddr_in in_addr;
			int fd;
			httpd_req_t *req;

			if (FD_ISSET(httpd_fd, &fds)) {
				if ((fd=accept(httpd_fd,
					       (struct sockaddr *)&in_addr,
					       &len)) < 0) {
					perror("accept()");
					continue;
				}
				printf("received connection...\n");
				if ((req=resp_header(fd)) != NULL) {
					unsigned int state;
					unsigned int menu;
					unsigned int key;
					char *page;

					state = get_state(req->path);
					menu = get_menu(req->path);
					key = get_key(req->path);
					page = get_page(req->path);

					if (page == NULL) {
						httpd_header(fd, 404);
						html->notfound(fd, req->path,
							       HTTP_PORT);
					} else if (state &&
						   (state != html->get_state())) {
						printf("=== REGENERATE ===\n");
						httpd_header(fd, 200);
						html->generate(fd);
					} else if ((strcmp(page, "/osd.html") == 0) &&
						   (state == html->get_state())) {
						printf("== FOLLOW LINK ==\n");
						if (send_command(fd, menu,
								 key,
								 state) == 0) {
							waiting = fd;
							fd = -1;
						} else {
							httpd_header(fd, 404);
							html->notfound(fd,
								       req->path,
								       HTTP_PORT);
						}
					} else if ((strcmp(page, "/") == 0) ||
						   (strcmp(page, "/osd.html") == 0)) {
						printf("=== REGENERATE ===\n");
						httpd_header(fd, 200);
						html->generate(fd);
					} else if (strcmp(req->path,
							  "/web.css") == 0) {
						httpd_header(fd, 200);
						html->css(fd);
					} else {
						httpd_header(fd, 404);
						html->notfound(fd, req->path,
							       HTTP_PORT);

					}

					if (page)
						free(page);
				}
				close(fd);
			}
		}
	}

 err:
	return NULL;
}

static int
http_generate(void)
{
#if defined(MVPMC_MG35)
	if (httpd_pid != getpid()) {
		cleanup = 1;
		return 0;
	}
#endif

	if (waiting) {
		int fd = waiting;
		waiting = 0;
		httpd_header(fd, 200);
		html->generate(fd);
		close(fd);
	}

	return 0;
}

static int
http_update_widget(gw_t *widget)
{
	html->update_widget(widget);

	return 0;
}

static int
http_input_fd(void)
{
	return pipefds[0];
}

static plugin_http_t http = {
	.generate = http_generate,
	.update_widget = http_update_widget,
	.input_fd = http_input_fd,
};

static void*
init_http(void)
{
	pthread_attr_t attr;

	pipe(pipefds);

	if ((html=plugin_load("html")) == NULL) {
		return NULL;
	}

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1024*64);

	pthread_create(&thread, &attr, http_thread, NULL);
	pthread_create(&threadd, &attr, httpd_thread, NULL);

	printf("HTTP plug-in registered!\n");

	return (void*)&http;
}

static int
release_http(void)
{
	if (pthread_kill(thread, SIGTERM) == 0) {
		pthread_join(thread, NULL);
	}

	if (pthread_kill(threadd, SIGTERM) == 0) {
		pthread_join(threadd, NULL);
	}

	close(httpd_fd);
	httpd_fd = -1;

	close(pipefds[0]);
	close(pipefds[1]);

	plugin_unload("html");

	printf("HTTP plug-in deregistered!\n");

	return 0;
}

PLUGIN_INIT(init_http);
PLUGIN_RELEASE(release_http);
