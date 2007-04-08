/*
 *  Copyright (C) 2004-2007, Jon Gettler
 *  http://www.mvpmc.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*! \mainpage MediaMVP Media Center
 *
 * The MediaMVP Media Center is an open source replacement for the Hauppauge
 * firmware that runs on the Hauppauge MediaMVP.
 *
 * \section projectweb Project website
 * http://www.mvpmc.org/
 *
 * \section repos Source repositories
 * http://git.mvpmc.org/
 *
 * \section libraries Libraries
 * \li \link mvp_av.h libav \endlink
 * \li \link cmyth.h libcmyth \endlink
 * \li \link mvp_demux.h libdemux \endlink
 * \li \link mvp_osd.h libosd \endlink
 * \li \link mvp_widget.h libwidget \endlink
 *
 * \section application Application Headers
 * \li \link mvpmc.h mvpmc.h \endlink
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/select.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <libgen.h>

#include "plugin.h"
#include "gw.h"

char *version = NULL;
char build_info[256];

static struct option opts[] = {
	{ "aspect", required_argument, 0, 'a' },
	{ "config", required_argument, 0, 'F' },
	{ "iee", required_argument, 0, 'd' },
	{ "mclient", required_argument, 0, 'c' },
	{ "mode", required_argument, 0, 'm' },
	{ "mythtv", required_argument, 0, 's' },
	{ "mythtv-debug", no_argument, 0, 'M' },
	{ "no-wss", no_argument, 0, 0 },
	{ "no-filebrowser", no_argument, 0, 0 },
	{ "no-reboot", no_argument, 0, 0 },
	{ "no-settings", no_argument, 0, 0 },
	{ "theme", required_argument, 0, 't' },
	{ "startup", required_argument, 0, 0},
	{ "web-port", required_argument, 0, 0},
	{ "version", no_argument, 0, 0},
	{ "vlc", required_argument, 0, 0},
	{ "vlc-vopts", required_argument, 0, 0},
	{ "vlc-aopts", required_argument, 0, 0},
	{ "vlc-vb", required_argument, 0, 0},
	{ "vlc-ab", required_argument, 0, 0},
	{ "use-mplayer", no_argument, 0, 0 },
	{ "emulate", required_argument, 0, 0},
	{ "rfb-mode", required_argument, 0, 0},
	{ "rtwin", required_argument, 0, 0},
	{ "fs-rtwin", required_argument, 0, 0},
	{ "flicker", no_argument, 0, 0},
	{ "mythtv-db", required_argument, 0, 'y'},
	{ "mythtv-username", required_argument, 0, 'u'},
	{ "mythtv-password", required_argument, 0, 'p'},
	{ "mythtv-database", required_argument, 0, 'T'},
	{ 0, 0, 0, 0 }
};

static gw_t *about;
static gw_t *menu;

static int
do_exit(gw_t *widget)
{
	printf("Bye!\n");
	exit(0);

	return 0;
}

static int
do_about(gw_t *widget)
{
	gw_map(about);
	gw_unmap(menu);
	gw_focus_set(about);

	return 0;
}

static int
do_key(gw_t *widget)
{
	if (widget == about) {
		gw_unmap(about);
		gw_map(menu);
		gw_focus_set(menu);
	}

	return 0;
}

static void*
gui_start(void *arg)
{
	gw_t *splash;
	gw_t *text;
	gw_t *root;

	root = gw_root();

	gw_focus_cb_set(do_key);

	if ((splash=gw_create(GW_TYPE_CONTAINER, root)) == NULL)
		goto err;
	if ((text=gw_create(GW_TYPE_TEXT, splash)) == NULL)
		goto err;
	if ((menu=gw_create(GW_TYPE_MENU, root)) == NULL)
		goto err;
	if ((about=gw_create(GW_TYPE_TEXT, root)) == NULL)
		goto err;

	gw_text_set(text, "http://www.mvpmc.org/");
	gw_text_set(about, "mvpmc (plug-in version) http://www.mvpmc.org/");

	gw_map(text);
	gw_map(splash);

	printf("splash screen created...\n");

	gw_map(root);

	gw_output();

	gw_menu_title_set(menu, "mvpmc");
	gw_menu_item_add(menu, "File Browser", NULL, NULL);
	gw_menu_item_add(menu, "About", do_about, NULL);
	gw_menu_item_add(menu, "Exit", do_exit, NULL);

	gw_unmap(splash);
	gw_map(menu);

	gw_focus_set(menu);

	gw_output();

	return NULL;

 err:
	return NULL;
}

/*
 * main()
 */
int
mvpmc_main(int argc, char **argv)
{
	extern char compile_time[], version_number[], build_user[];
	extern char git_revision[], git_diffs[];
	extern int plugin_setup(void);
	int c, i;
	int opt_index;
	char *saved_argv[32];

	/*
	 * Ensure the build info is easily found in any corefile.
	 */
	snprintf(build_info, sizeof(build_info),
		 "BUILD_INFO: '%s' '%s' '%s' '%s' '%s'\n",
		 version_number,
		 compile_time,
		 build_user,
		 git_revision,
		 git_diffs);

	if (version_number[0] != '\0') {
		version = version_number;
	} else if (git_revision[0] != '\0') {
		version = git_revision;
	}

	if (argc > 32) {
		fprintf(stderr, "too many arguments\n");

		exit(1);
	}
	for (i=0; i<argc; i++)
		saved_argv[i] = strdup(argv[i]);

	while ((c=getopt_long(argc, argv,
			      "a:b:C:c:d:D:f:F:hHi:m:Mo:r:R:s:S:y:t:u:p:T:",
			      opts, &opt_index)) != -1) {
		switch (c) {
		case 'h':
			exit(0);
			break;
		default:
			exit(1);
			break;
		}
	}

	tzset();

	/*
	 * setup the plug-in loader
	 */
	plugin_setup();

	if (gw_init(GW_DEV_OSD|GW_DEV_HTML) < 0) {
		fprintf(stderr, "failed to initialize libgw!\n");
		exit(1);
	}

	printf("gw initialized\n");

	gui_start(NULL);

	gw_loop(NULL);
    
	return 0;
}

#ifdef MVPMC_MEDIAMVP
static int
ticonfig_main(int argc, char **argv)
{
	return -1;
}

static int
vpdread_main(int argc, char **argv)
{
	return -1;
}
#endif /* MVPMC_MEDIAMVP */

int
main(int argc, char **argv)
{
	char *prog;

	prog = basename(argv[0]);

	if (strcmp(prog, "mvpmc") == 0) {
		return mvpmc_main(argc, argv);
#ifdef MVPMC_MEDIAMVP
	} else if (strcmp(prog, "ticonfig") == 0) {
		return ticonfig_main(argc, argv);
	} else if (strcmp(prog, "vpdread") == 0) {
		return vpdread_main(argc, argv);
#endif /* MVPMC_MEDIAMVP */
	}

	fprintf(stderr, "Unknown app: %s\n", prog);

	return -1;
}
