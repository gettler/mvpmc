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

#define WRITE(resp,buf,len) \
	if (write_resp(resp, buf, len) < 0) { \
		goto err; \
	}

#define LEVEL() \
	{ \
		int i; \
		for (i=0; i<level; i++) \
			WRITE(resp, " ", 1); \
	}

static int html_generate_container(plugin_html_resp_t*, gw_t*, int);
static int html_generate_menu(plugin_html_resp_t*, gw_t*, int);
static int html_generate_text(plugin_html_resp_t*, gw_t*, int);
static int html_generate_image(plugin_html_resp_t*, gw_t*, int);

static struct html_output_s {
	gw_type_t type;
	int (*output)(plugin_html_resp_t*, gw_t*, int);
} output[] = {
	{ GW_TYPE_CONTAINER, html_generate_container },
	{ GW_TYPE_MENU, html_generate_menu },
	{ GW_TYPE_TEXT, html_generate_text },
	{ GW_TYPE_IMAGE, html_generate_image },
	{ 0, NULL },
};

static char *header = HTML_HEADER;
static char *footer = HTML_FOOTER;

static unsigned int state;

static int
write_resp_guts(plugin_html_resp_t *resp, char *buf, int len)
{
	plugin_html_resp_t *new;
	plugin_html_resp_t *cur;

	if (resp->data == NULL) {
		new = resp;
		memcpy(resp->data, buf, len);
		resp->len = len;
	} else {
		cur = resp;
		while (cur->next) {
			cur = cur->next;
		}

		if (len < (sizeof(cur->data)-cur->len)) {
			memcpy(cur->data+cur->len, buf, len);
			cur->len += len;
		} else {
			if ((new=malloc(sizeof(*new))) == NULL) {
				return -1;
			}
			memcpy(new->data, buf, len);
			new->len = len;
			new->next = NULL;

			cur->next = new;
			new->offset = cur->offset + cur->len;
		}
	}

	return 0;
}

static int
write_resp(plugin_html_resp_t *resp, char *buf, int len)
{
	int n, total = 0, r = len;

	while (r > 0) {
		n = (r < sizeof(resp->data)) ? r : sizeof(resp->data);
		if (write_resp_guts(resp, buf+total, n) < 0) {
			return -1;
		}
		total += n;
		r -= n;
	}

	return 0;
}

static int
output_widget(plugin_html_resp_t *resp, gw_t *widget, int level)
{
	int i = 0;

	while (output[i].output) {
		if (output[i].type == widget->type) {
			if (widget->realized) {
				return output[i].output(resp, widget, level+1);
			} else {
				return 0;
			}
		}
		i++;
	}

	return -1;
}

static int
html_paragraph(plugin_html_resp_t *resp, char *label, char *text, int level)
{
	char *head1 = "<p class=\"";
	char *head2 = "\">\n";
	char *foot = "</p>\n";

	LEVEL();
	WRITE(resp, head1, strlen(head1));
	WRITE(resp, label, strlen(label));
	WRITE(resp, head2, strlen(head2));
	
	LEVEL();
	WRITE(resp, text, strlen(text));
	WRITE(resp, "\n", 1);
	
	LEVEL();
	WRITE(resp, foot, strlen(foot));

	return 0;

 err:
	return -1;
}

static int
html_li(plugin_html_resp_t *resp, char *label, char *text, int level, gw_menu_t *gw, void *key)
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
	WRITE(resp, head1, strlen(head1));
	WRITE(resp, label, strlen(label));
	WRITE(resp, head2, strlen(head2));
	WRITE(resp, head3, strlen(head3));
	snprintf(str, sizeof(str), "0x%.8lx", (unsigned long)gw);
	WRITE(resp, str, strlen(str));
	WRITE(resp, head4, strlen(head4));
	snprintf(str, sizeof(str), "0x%.8lx", (unsigned long)key);
	WRITE(resp, str, strlen(str));
	WRITE(resp, head5, strlen(head5));
	snprintf(str, sizeof(str), "0x%.8x", state);
	WRITE(resp, str, strlen(str));
	WRITE(resp, head6, strlen(head6));
	
	if (text) {
		WRITE(resp, text, strlen(text));
	}
	
	WRITE(resp, foot, strlen(foot));

	return 0;

 err:
	return -1;
}

static int
html_generate_container(plugin_html_resp_t *resp, gw_t *widget, int level)
{
	char *head1 = "<div id=\"";
	char *head2 = "\">\n";
	char *foot = "</div>\n";
	gw_t *next;

	LEVEL();
	WRITE(resp, head1, strlen(head1));
	if (widget->name) {
		WRITE(resp, widget->name, strlen(widget->name));
	} else {
		char *def = "container";
		WRITE(resp, def, strlen(def));
	}
	WRITE(resp, head2, strlen(head2));
	
	next = widget->data.container->child;
	while (next) {
		if (output_widget(resp, next, level) < 0) {
			goto err;
		}
		next = next->below;
	}
	
	LEVEL();
	WRITE(resp, foot, strlen(foot));

	return 0;

 err:
	return -1;
}

static int
html_generate_menu(plugin_html_resp_t *resp, gw_t *widget, int level)
{
	char *head1 = "<div id=\"";
	char *head2 = "\">\n";
	char *foot = "</div>\n";
	gw_menu_t *data;
	int i;

	LEVEL();
	WRITE(resp, head1, strlen(head1));
	if (widget->name) {
		WRITE(resp, widget->name, strlen(widget->name));
	} else {
		char *def = "menu";
		WRITE(resp, def, strlen(def));
	}
	WRITE(resp, head2, strlen(head2));

	data = widget->data.menu;

	if (data->title) {
		if (html_paragraph(resp, "title", data->title, level+1) < 0) {
			goto err;
		}
	}

	level++;
	if (data->n > 0) {
		char *head = "<p class=\"body\"><ul>\n";
		char *foot = "</ul></p>\n";

		LEVEL();
		WRITE(resp, head, strlen(head));

		for (i=0; i<data->n; i++) {
			void *key = data->items[i]->key;
			if (html_li(resp, "link", data->items[i]->text,
				    level+1, data, key) < 0) {
				goto err;
			}
		}

		LEVEL();
		WRITE(resp, foot, strlen(foot));
	}
	level--;
	
	LEVEL();
	WRITE(resp, foot, strlen(foot));

	return 0;

 err:
	return -1;
}

static int
html_generate_text(plugin_html_resp_t *resp, gw_t *widget, int level)
{
	char *head1 = "<div id=\"";
	char *head2 = "\"><p>\n";
	char *foot = "</p></div>\n";
	char *text = NULL;

	LEVEL();
	WRITE(resp, head1, strlen(head1));
	if (widget->name) {
		WRITE(resp, widget->name, strlen(widget->name));
	} else {
		char *def = "text";
		WRITE(resp, def, strlen(def));
	}
	
	if (widget->data.text)
		text = widget->data.text->text;

	WRITE(resp, head2, strlen(head2));
	if (text)
		WRITE(resp, text, strlen(text));
	
	LEVEL();
	WRITE(resp, foot, strlen(foot));

	return 0;

 err:
	return -1;
}

static int
html_generate_image(plugin_html_resp_t *resp, gw_t *widget, int level)
{
	char *head1 = "<div id=\"";
	char *head2 = "\"><p>\n";
	char *img1 = "<img src=\"";
	char *img2 = "\">";
	char *foot = "</p></div>\n";
	char *path = widget->data.image->path;

	LEVEL();
	WRITE(resp, head1, strlen(head1));
	if (widget->name) {
		WRITE(resp, widget->name, strlen(widget->name));
	} else {
		char *def = "image";
		WRITE(resp, def, strlen(def));
	}
	WRITE(resp, head2, strlen(head2));

	WRITE(resp, img1, strlen(img1));
	WRITE(resp, path, strlen(path));
	WRITE(resp, img2, strlen(img2));

	LEVEL();
	WRITE(resp, foot, strlen(foot));

	return 0;

 err:
	return -1;
}

int
html_generate_commands(plugin_html_resp_t *resp)
{
	int i;
	char *head = "<div id=\"commands\"><ul>\n";
	char *cmd1 = "<li><a href=\"cmd.html?cmd=";
	char *cmd2 = "\">";
	char *cmd3 = "</a></li>\n";
	char *foot = "</ul></div>\n";
	char *cmd[] = {
		"return",
	};

	WRITE(resp, head, strlen(head));

	for (i=0; i<sizeof(cmd)/sizeof(cmd[0]); i++) {
		WRITE(resp, cmd1, strlen(cmd1));
		WRITE(resp, cmd[i], strlen(cmd[i]));
		WRITE(resp, cmd2, strlen(cmd2));
		WRITE(resp, cmd[i], strlen(cmd[i]));
		WRITE(resp, cmd3, strlen(cmd3));
	}

	WRITE(resp, foot, strlen(foot));

	return 0;

 err:
	return -1;
}

static plugin_html_resp_t*
html_generate(void)
{
	gw_t *root;
	plugin_html_resp_t *resp;

	if ((resp=malloc(sizeof(*resp))) == NULL) {
		return NULL;
	}
	memset(resp, 0, sizeof(*resp));

	if ((root=gw_root()) == NULL) {
		goto err;
	}

	if (write_resp(resp, header, strlen(header)) < 0) {
		goto err;
	}

	if (html_generate_container(resp, root, 1) < 0) {
		goto err;
	}

	if (html_generate_commands(resp) < 0) {
		goto err;
	}

	if (write_resp(resp, footer, strlen(footer)) < 0) {
		goto err;
	}

	return resp;

 err:
	if (resp)
		free(resp);

	return NULL;
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
