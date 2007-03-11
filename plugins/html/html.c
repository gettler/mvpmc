/*
 *  Copyright (C) 2007, Jon Gettler
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

#include "plugin_gw.h"

unsigned long plugin_version = CURRENT_PLUGIN_VERSION;

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

static struct html_output_s {
	gw_type_t type;
	int (*output)(int, gw_t*, int);
} output[] = {
	{ GW_TYPE_CONTAINER, html_generate_container },
	{ GW_TYPE_MENU, html_generate_menu },
	{ 0, NULL },
};

static char *header = HTML_HEADER;
static char *footer = HTML_FOOTER;

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
html_li(int fd, char *label, char *text, int level)
{
	char *head1 = "<li><a class=\"";
	char *head2 = "\" href=\"";
	char *head3 = "\">";
	char *foot = "</a></li>\n";

	LEVEL();
	WRITE(fd, head1, strlen(head1));
	WRITE(fd, label, strlen(label));
	WRITE(fd, head2, strlen(head2));
	WRITE(fd, text, strlen(text));
	WRITE(fd, head3, strlen(head3));
	
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
			if (html_li(fd, "link",
				    data->items[i]->text, level+1) < 0) {
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

int
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
init_html(void)
{
	return 0;
}

static int
release_html(void)
{
	return 0;
}

PLUGIN_INIT(init_html);
PLUGIN_RELEASE(release_html);
