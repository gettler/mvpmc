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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <plugin/gw.h>
#include <plugin/html.h>

#if defined(PLUGIN_SUPPORT)
unsigned long plugin_version = CURRENT_PLUGIN_VERSION;
#endif

#define HTML_HEADER \
	"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" " \
	"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n" \
	"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">\n" \
	"<head>\n" \
	"<meta http-equiv=\"content-type\" " \
	"content=\"text/html; charset=iso-8859-1\" />\n" \
	"<link rel=\"icon\" href=\"http://mvpmc.org/favicon.ico\" />\n" \
	"<title>MediaMVP Media Center</title>\n" \
	"<link rel=\"stylesheet\" type=\"text/css\" " \
	"title=\"web\" href=\"web.css\" />\n" \
	"</head>\n\n" \
	"<body>\n"

#define HTML_FOOTER \
	"</body>\n" \
	"</html>\n"

#define CSS_DEFAULT \
	"body { background: black; color: white; }\n" \
	"a { text-decoration: none; }\n" \
	"a, a:link, a:visited, a:acted { color: white; }\n" \
	"a:hover { background: green; color: white; }\n" \
	".title { background: blue; color: white; }\n"

#define NOT_FOUND_HEADER \
	"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">" \
	"<html><head>" \
	"<title>404 Not Found</title>" \
	"</head><body>" \
	"<h1>Not Found</h1>"

#define NOT_FOUND_PATH \
	"<p>The requested URL %s was not found on this server.</p>" \
	"<hr>"

#define NOT_FOUND_TAIL \
	"<address>mvpmc (%s) Server at %s Port %d</address>" \
	"</body></html>"

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

#define WRITE(fd,buf,len) \
	if (full_write(fd, buf, len) < 0) { \
		goto err; \
	}

#define LEVEL() \
	{ \
		int i; \
		for (i=0; i<level; i++) \
			WRITE(fd, " ", 1); \
	}

static int html_generate_container(int fd, gw_t *widget, int level);
static int html_generate_menu(int fd, gw_t *widget, int level);
static int html_generate_text(int fd, gw_t *widget, int level);

static struct html_output_s {
	gw_type_t type;
	int (*output)(int, gw_t*, int);
} output[] = {
	{ GW_TYPE_CONTAINER, html_generate_container },
	{ GW_TYPE_MENU, html_generate_menu },
	{ GW_TYPE_TEXT, html_generate_text },
	{ 0, NULL },
};

static char *header = HTML_HEADER;
static char *footer = HTML_FOOTER;
static char *css = CSS_DEFAULT;
static char *nf_header = NOT_FOUND_HEADER;

static unsigned int state;

static int
full_write(int fd, char *buf, int len)
{
	int n, tot = 0;

	while (tot < len) {
		n = write(fd, buf+tot, len-tot);
		if ((n < 0) && (errno != EINTR))
			break;
		if (n == 0)
			break;
		tot += n;
	}

	if (tot == len) {
		return 0;
	} else {
		return -1;
	}
}

static int
output_widget(int fd, gw_t *widget, int level)
{
	int i = 0;

	while (output[i].output) {
		if (output[i].type == widget->type) {
			if (widget->realized) {
				return output[i].output(fd, widget, level+1);
			} else {
				return 0;
			}
		}
		i++;
	}

	return -1;
}

static int
html_paragraph(int fd, char *label, char *text, int level)
{
	char *head1 = "<p class=\"";
	char *head2 = "\">\n";
	char *foot = "</p>\n";

	LEVEL();
	WRITE(fd, head1, strlen(head1));
	WRITE(fd, label, strlen(label));
	WRITE(fd, head2, strlen(head2));
	
	LEVEL();
	WRITE(fd, text, strlen(text));
	WRITE(fd, "\n", 1);
	
	LEVEL();
	WRITE(fd, foot, strlen(foot));

	return 0;

 err:
	return -1;
}

static int
html_li(int fd, char *label, char *text, int level, gw_menu_t *gw, void *key)
{
	char *head1 = "<li><a class=\"";
	char *head2 = "\" href=\"/osd.html?";
	char *head3 = "menu=";
	char *head4 = "&key=";
	char *head5 = "&state=";
	char *head6 = "\">";
	char *foot = "</a></li>\n";
	char str[16];

	LEVEL();
	WRITE(fd, head1, strlen(head1));
	WRITE(fd, label, strlen(label));
	WRITE(fd, head2, strlen(head2));
	WRITE(fd, head3, strlen(head3));
	snprintf(str, sizeof(str), "0x%.8x", (unsigned int)gw);
	WRITE(fd, str, strlen(str));
	WRITE(fd, head4, strlen(head4));
	snprintf(str, sizeof(str), "0x%.8x", (unsigned int)key);
	WRITE(fd, str, strlen(str));
	WRITE(fd, head5, strlen(head5));
	snprintf(str, sizeof(str), "0x%.8x", state);
	WRITE(fd, str, strlen(str));
	WRITE(fd, head6, strlen(head6));
	
	if (text) {
		WRITE(fd, text, strlen(text));
	}
	
	WRITE(fd, foot, strlen(foot));

	return 0;

 err:
	return -1;
}

static int
html_generate_container(int fd, gw_t *widget, int level)
{
	char *head1 = "<div id=\"";
	char *head2 = "\">\n";
	char *foot = "</div>\n";
	gw_t *next;

	LEVEL();
	WRITE(fd, head1, strlen(head1));
	if (widget->name) {
		WRITE(fd, widget->name, strlen(widget->name));
	} else {
		char *def = "container";
		WRITE(fd, def, strlen(def));
	}
	WRITE(fd, head2, strlen(head2));
	
	next = widget->data.container->child;
	while (next) {
		if (output_widget(fd, next, level) < 0) {
			goto err;
		}
		next = next->below;
	}
	
	LEVEL();
	WRITE(fd, foot, strlen(foot));

	return 0;

 err:
	return -1;
}

static int
html_generate_menu(int fd, gw_t *widget, int level)
{
	char *head1 = "<div id=\"";
	char *head2 = "\">\n";
	char *foot = "</div>\n";
	gw_menu_t *data;
	int i;

	LEVEL();
	WRITE(fd, head1, strlen(head1));
	if (widget->name) {
		WRITE(fd, widget->name, strlen(widget->name));
	} else {
		char *def = "menu";
		WRITE(fd, def, strlen(def));
	}
	WRITE(fd, head2, strlen(head2));

	data = widget->data.menu;

	if (data->title) {
		if (html_paragraph(fd, "title", data->title, level+1) < 0) {
			goto err;
		}
	}

	level++;
	if (data->n > 0) {
		char *head = "<p class=\"body\"><ul>\n";
		char *foot = "</ul></p>\n";

		LEVEL();
		WRITE(fd, head, strlen(head));

		for (i=0; i<data->n; i++) {
			void *key = data->items[i]->key;
			if (html_li(fd, "link", data->items[i]->text,
				    level+1, data, key) < 0) {
				goto err;
			}
		}

		LEVEL();
		WRITE(fd, foot, strlen(foot));
	}
	level--;
	
	LEVEL();
	WRITE(fd, foot, strlen(foot));

	return 0;

 err:
	return -1;
}

static int
html_generate_text(int fd, gw_t *widget, int level)
{
	char *head1 = "<div id=\"";
	char *head2 = "\"><p>\n";
	char *foot = "</p></div>\n";
	char *text = NULL;

	LEVEL();
	WRITE(fd, head1, strlen(head1));
	if (widget->name) {
		WRITE(fd, widget->name, strlen(widget->name));
	} else {
		char *def = "text";
		WRITE(fd, def, strlen(def));
	}
	
	if (widget->data.text)
		text = widget->data.text->text;

	WRITE(fd, head2, strlen(head2));
	if (text)
		WRITE(fd, text, strlen(text));
	
	LEVEL();
	WRITE(fd, foot, strlen(foot));

	return 0;

 err:
	return -1;
}

static int
html_generate(int fd)
{
	gw_t *root;

	if ((root=gw_root()) == NULL) {
		return -1;
	}

	if (full_write(fd, header, strlen(header)) < 0) {
		return -1;
	}

	if (html_generate_container(fd, root, 1) < 0) {
		return -1;
	}

	if (full_write(fd, footer, strlen(footer)) < 0) {
		return -1;
	}

	return 0;
}

static int
html_default_css(int fd)
{
	if (full_write(fd, css, strlen(css)) < 0) {
		return -1;
	}

	return 0;
}

static int
html_notfound(int fd, char *path, int port)
{
	char *str = NULL;
	int ret = -1;
	int len = 8 * 1024;
	char host[32];

	gethostname(host, sizeof(host));

	if ((str=malloc(len)) == NULL) {
		goto err;
	}

	if (full_write(fd, nf_header, strlen(nf_header)) < 0) {
		goto err;
	}

	snprintf(str, len, NOT_FOUND_PATH, path);
	if (full_write(fd, str, strlen(str)) < 0) {
		goto err;
	}

	snprintf(str, len, NOT_FOUND_TAIL, PLATFORM, host, port);
	if (full_write(fd, str, strlen(str)) < 0) {
		goto err;
	}

	ret = 0;

 err:
	if (str)
		free(str);

	return ret;
}

static int
html_update_widget(gw_t *widget)
{
	state++;

	if (state == 0)
		state++;

	printf("STATE: 0x%.8x\n", state);

	return 0;
}

static unsigned int
html_get_state(void)
{
	printf("CURRENT STATE: 0x%.8x\n", state);

	return state;
}

static plugin_html_t html = {
	.generate = html_generate,
	.css = html_default_css,
	.notfound = html_notfound,
	.update_widget = html_update_widget,
	.get_state = html_get_state,
};

static void*
init_html(void)
{
	printf("HTML plug-in registered!\n");

	state = rand();

	return (void*)&html;
}

static int
release_html(void)
{
	return 0;
}

PLUGIN_INIT(init_html);
PLUGIN_RELEASE(release_html);
