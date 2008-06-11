/*
 *  Copyright (C) 2004-2008, Jon Gettler
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
#include <microhttpd.h>

#include <plugin.h>
#include <gw.h>
#include <plugin/av.h>
#include <plugin/app.h>

char *version = NULL;
char build_info[256];

char *mclient_server = NULL;
char *mythtv_server = NULL;
char *mythtv_recdir = NULL;

static gw_t *root;

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

plugin_av_t *av;
plugin_app_t *myth;

static int do_key(gw_t *widget, int key);

#if defined(MVPMC_MEDIAMVP) || defined(MVPMC_NMT)
#include <sys/reboot.h>
#include <linux/reboot.h>
static int
do_reboot(gw_t *widget, char *text, void *key)
{
	sync();

	/*
	 * Do an orderly shutdown, if possible.
	 */
#if defined(MVPMC_MEDIAMVP)
	system("/sbin/reboot");
	sleep(1);
#endif

	reboot(LINUX_REBOOT_CMD_RESTART);

	return 0;
}
#endif /* MVPMC_MEDIAMVP || MVPMC_NMT */

#if defined(MVPMC_MG35)
static int
do_poweroff(gw_t *widget, char *text, void *key)
{
	extern int av_poweroff(void);

	sync();

	av_poweroff();

	while (1) {
		pause();
	}

	return 0;
}
#endif /* MVPMC_MG35 */

#if defined(MVPMC_NMT)
static int
do_gaya(gw_t *widget, char *text, void *key)
{
	char path[] = "/bin/gaya";
	pid_t parent = getpid();

	sync();

	switch (fork()) {
	case 0:
		while (kill(parent, 0) != -1) {
			usleep(1000);
		}
		unsetenv("LD_LIBRARY_PATH");
		unsetenv("PATH");
		execl(path, path, NULL);
		exit(-1);
		break;
	case -1:
		perror("fork()");
		break;
	default:
		exit(0);
		break;
	}

	return -1;
}
#endif /* MVPMC_NMT */

static int
do_exit(gw_t *widget, char *text, void *key)
{
	printf("Bye!  Pid %d exiting...\n", getpid());
#if defined(MVPMC_MG35)
	extern void pthread_deinit(void);
	pthread_deinit();
#endif
	exit(0);

	return 0;
}

static int
do_about(gw_t *widget, char *text, void *key)
{
	gw_map(about);
	gw_unmap(menu);
	gw_focus_set(about);

	return 0;
}

static int
do_fb(gw_t *widget, char *text, void *key)
{
	extern void fb_display(void);

	gw_unmap(menu);

	fb_display();

	return 0;
}

#if defined(MVPMC_NMT)
static int
do_ss(gw_t *widget, char *text, void *key)
{
	char url[1024];

	snprintf(url, sizeof(url), "http://%s:9000/stream.mp3",
		 mclient_server);

	return av->play_url(url, NULL);
}
#endif

static void
redisplay(void)
{
	gw_map(menu);
	gw_focus_set(menu);
	gw_focus_cb_set(do_key);
}

static int
do_myth(gw_t *widget, char *text, void *key)
{
	gw_unmap(widget);

	myth->enter(redisplay);

	return 0;
}

static int
do_key(gw_t *widget, int key)
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
	extern int fb_init(gw_t *root);
	gw_t *splash;
	gw_t *text;
#if defined(MVPMC_NMT)
	char *gaya = getenv("EXIT_TO_GAYA");
#endif

#if defined(MVPMC_NMT)
	/*
	 * If mvpmc is started by gaya, set the hard drive spindown time
	 * to 2 minutes, since gaya doesn't seem to be setting this.
	 */
	if (gaya) {
		system("hdparm -S 24 /dev/hda");
	}
#endif

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

	fb_init(root);

	gw_menu_title_set(menu, "mvpmc");
	gw_menu_item_add(menu, "File Browser", (void*)0, do_fb, NULL);
#if defined(MVPMC_NMT)
	if (mclient_server) {
		gw_menu_item_add(menu, "SlimServer", (void*)5, do_ss, NULL);
	}
#endif
	if (myth) {
		gw_menu_item_add(menu, "MythTV", (void*)6, do_myth, NULL);
	}
	gw_menu_item_add(menu, "About", (void*)1, do_about, NULL);
#if defined(MVPMC_NMT)
	if (gaya) {
		gw_menu_item_add(menu, "Exit", (void*)2, do_exit, NULL);
		gw_menu_item_add(menu, "Exit to Gaya", (void*)4, do_gaya, NULL);
	} else {
		gw_menu_item_add(menu, "Exit", (void*)2, do_exit, NULL);
		gw_menu_item_add(menu, "Start Gaya", (void*)4, do_gaya, NULL);
	}
#else
	gw_menu_item_add(menu, "Exit", (void*)2, do_exit, NULL);
#endif
#if defined(MVPMC_MEDIAMVP) || defined(MVPMC_NMT)
	gw_menu_item_add(menu, "Reboot", (void*)3, do_reboot, NULL);
#endif
#if defined(MVPMC_MG35)
	gw_menu_item_add(menu, "Power Off", (void*)3, do_poweroff, NULL);
#endif

	gw_unmap(splash);
	gw_map(menu);

	gw_focus_set(menu);

	gw_output();

	return NULL;

 err:
	return NULL;
}

void
main_display(void)
{
	gw_map(menu);

	gw_focus_set(menu);
	gw_focus_cb_set(do_key);

	gw_output();
}

static void
early_init(void)
{
	extern char compile_time[], version_number[];
	extern char build_user[], build_target[];
	extern char git_revision[], git_diffs[];
	char *root;

	/*
	 * Ensure the build info is easily found in any corefile.
	 */
	snprintf(build_info, sizeof(build_info),
		 "BUILD_INFO: '%s' '%s' '%s' '%s' '%s' '%s'\n",
		 version_number,
		 compile_time,
		 build_user,
		 build_target,
		 git_revision,
		 git_diffs);

	if (version_number[0] != '\0') {
		version = version_number;
	} else if (git_revision[0] != '\0') {
		version = git_revision;
	}

	/*
	 * Ensure MVPMC_ROOT is set to the top of the build filesystem.
	 */
	if ((root=getenv("MVPMC_ROOT")) == NULL) {
		char path[512], buf[512];
		char *bin, *p;

		snprintf(buf, sizeof(buf), "/proc/%d/exe", getpid());

		if (readlink(buf, path, sizeof(path)) > 0) {
			if ((bin=dirname(path)) != NULL) {
				snprintf(buf, sizeof(buf), "%s/../", bin);
				if ((p=strdup(buf)) != NULL) {
					setenv("MVPMC_ROOT", p, 1);
				}
			}
		}
	}
}

void
usage(char *path)
{
	char *prog = basename(path);

	printf("Usage: %s [options]\n", prog);
	printf("\t-c server \tslimdevices musicClient server IP address\n");
	printf("\t-h        \tprint this help\n");
	printf("\t-s server \tmythtv server IP address\n");
	printf("\t-r path   \tpath to NFS mounted mythtv recordings\n");
}

/*
 * main()
 */
int
mvpmc_main(int argc, char **argv)
{
	extern int do_plugin_setup(void);
	int c, i;
	int opt_index;
	char *saved_argv[32];

	early_init();

	if (argc > 32) {
		fprintf(stderr, "too many arguments\n");

		exit(1);
	}

	for (i=0; i<argc; i++) {
		saved_argv[i] = strdup(argv[i]);
	}

	while ((c=getopt_long(argc, argv,
			      "a:b:C:c:d:D:f:F:hHi:m:Mo:r:R:s:S:y:t:u:p:T:",
			      opts, &opt_index)) != -1) {
		switch (c) {
		case 'c':
			mclient_server = strdup(optarg);
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
			break;
		case 'r':
			mythtv_recdir = strdup(optarg);
			break;
		case 's':
			mythtv_server = strdup(optarg);
			break;
		case '?':
			exit(1);
			break;
		default:
			printf("Ignoring option: '%c'\n", c);
			break;
		}
	}

	tzset();
	srand(getpid());

	/*
	 * setup the plug-in loader
	 */
	if (do_plugin_setup() < 0) {
		fprintf(stderr, "failed to perform plug-in setup!\n");
		exit(1);
	}

	if (gw_init() < 0) {
		fprintf(stderr, "failed to initialize libgw!\n");
		exit(1);
	}

	if (gw_device_add(GW_DEV_OSD) < 0) {
		fprintf(stderr, "failed to initialize OSD!\n");
#if defined(MVPMC_NMT)
		/* directfb causes a segfault if exit() is used... */
		_exit(1);
#else
		exit(1);
#endif
	}

	if (gw_device_add(GW_DEV_HTTP) < 0) {
		fprintf(stderr, "failed to initialize HTTP!\n");
		exit(1);
	}

	printf("gw initialized\n");

	root = gw_create_console(ROOT_CONSOLE);

	if (gw_set_console(ROOT_CONSOLE) != 0) {
		fprintf(stderr, "root console not found!\n");
		exit(1);
	}

	printf("root console created\n");

	if (mythtv_server != NULL) { 
		if ((myth=plugin_load("myth")) == NULL) {
			fprintf(stderr, "mythtv plugin not found!\n");
		} else {
			myth->option("server", mythtv_server);
		}
	}

	if (myth && mythtv_recdir) {
		myth->option("recdir", mythtv_recdir);
	}

	gui_start(NULL);

	if ((av=plugin_load("av")) == NULL) {
		fprintf(stderr, "failed to initialize Audio/Video!\n");
		exit(1);
	}

	gw_loop(NULL);
    
#if defined(MVPMC_NMT)
	/*
	 * XXX: Here's the deal about this...
	 *
	 *    libmicrohttpd works on the NMT platform, but the shared library
	 *    build fails with binutils 2.17 (ld seg faults).  It will build
	 *    successfully with binutils 2.18, but mvpmc will fail to reload
	 *    the OSD plugin after running mono if built with binutils 2.18.
	 *
	 *    So...  this is here to force libmicrohttpd into the mvpmc binary,
	 *    so that the http plugin can run.
	 *
	 *    It will stay until I figure out a better solution...
	 */
	MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, 0,
			 NULL, NULL, NULL, NULL, MHD_OPTION_END);
	MHD_get_connection_values(NULL, MHD_GET_ARGUMENT_KIND, NULL, NULL);
	MHD_create_response_from_data(0, NULL, 0, MHD_NO);
	MHD_queue_response(NULL, MHD_HTTP_OK, NULL);
	MHD_destroy_response(NULL);
	MHD_create_response_from_callback(0, 0, NULL, NULL, NULL);
#endif /* MVPMC_NMT */

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

#if defined(MVPMC_MG35)
	extern int pthread_init(char*);
	printf("%s(): start pid %d...\n", __FUNCTION__, getpid());
	pthread_init(argv[0]);
#endif

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
