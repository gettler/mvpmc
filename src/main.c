/*
 *  Copyright (C) 2004, Jon Gettler
 *  http://mvpmc.sourceforge.net/
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

#ident "$Id$"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <mvp_widget.h>
#include <mvp_av.h>
#include <mvp_demux.h>
#include <mvp_osd.h>

#include "mvpmc.h"

#include "a52dec/a52.h"
#include "a52dec/mm_accel.h"

char *mythtv_server = NULL;
char *replaytv_server = NULL;

int fontid;
extern demux_handle_t *handle;

char *mythtv_recdir = NULL;

char *saved_argv[32];
int saved_argc = 0;

extern a52_state_t *a52_state;

extern void *ac3_start(void*);

static pthread_t ac3_thread;

/*
 * print_help() - print command line arguments
 *
 * Arguments:
 *	prog	- program name
 *
 * Returns:
 *	nothing
 */
static void
print_help(char *prog)
{
	printf("Usage: %s <options>\n", prog);

	printf("\t-a aspect \taspect ratio (4:3 or 16:9)\n");
	printf("\t-f font   \tfont file\n");
	printf("\t-h        \tprint this help\n");
	printf("\t-m mode   \toutput mode (ntsc or pal)\n");
	printf("\t-M        \tMythTV protocol debugging output\n");
	printf("\t-o output \toutput device (composite or svideo)\n");
	printf("\t-s server \tmythtv server IP address\n");
	printf("\t-r path   \tpath to NFS mounted mythtv recordings\n");
	printf("\t-R server \treplaytv server IP address\n");
}

/*
 * main()
 */
int
main(int argc, char **argv)
{
	int c, i;
	char *font = NULL;
	int mode = -1, output = -1, aspect = -1;
	int width, height;
	uint32_t accel = MM_ACCEL_DJBFFT;

	if (argc > 32) {
		fprintf(stderr, "too many arguments\n");
		exit(1);
	}

	for (i=0; i<argc; i++)
		saved_argv[i] = strdup(argv[i]);

	while ((c=getopt(argc, argv, "a:f:hm:Mo:r:R:s:")) != -1) {
		switch (c) {
		case 'a':
			if (strcmp(optarg, "4:3") == 0) {
				aspect = 0;
			} else if (strcmp(optarg, "16:9") == 0) {
				aspect = 1;
			} else {
				fprintf(stderr, "unknown aspect ratio '%s'\n",
					optarg);
				print_help(argv[0]);
				exit(1);
			}
			break;
		case 'h':
			print_help(argv[0]);
			exit(0);
			break;
		case 'f':
			font = strdup(optarg);
			break;
		case 'm':
			if (strcasecmp(optarg, "pal") == 0) {
				mode = AV_MODE_PAL;
			} else if (strcasecmp(optarg, "ntsc") == 0) {
				mode = AV_MODE_NTSC;
			} else {
				fprintf(stderr, "unknown mode '%s'\n", optarg);
				print_help(argv[0]);
				exit(1);
			}
			break;
		case 'M':
			mythtv_debug = 1;
			break;
		case 'o':
			if (strcasecmp(optarg, "svideo") == 0) {
				output = AV_OUTPUT_SVIDEO;
			} else if (strcasecmp(optarg, "composite") == 0) {
				output = AV_OUTPUT_COMPOSITE;
			} else {
				fprintf(stderr, "unknown output '%s'\n",
					optarg);
				print_help(argv[0]);
				exit(1);
			}
			break;
		case 'r':
			mythtv_recdir = strdup(optarg);
			break;
		case 'R':
			replaytv_server = strdup(optarg);
			break;
		case 's':
			mythtv_server = strdup(optarg);
			break;
		default:
			print_help(argv[0]);
			exit(1);
			break;
		}
	}

	if (font)
		fontid = mvpw_load_font(font);

	if (av_init() < 0) {
		fprintf(stderr, "failed to initialize av hardware!\n");
		exit(1);
	}

	if (mode != -1)
		av_set_mode(mode);

	if (av_get_mode() == AV_MODE_PAL) {
		printf("PAL mode, 720x576\n");
		width = 720;
		height = 576;
	} else {
		printf("NTSC mode, 720x480\n");
		width = 720;
		height = 480;
	}
	osd_set_surface_size(width, height);

	if (mw_init(mythtv_server, replaytv_server) < 0) {
		fprintf(stderr, "failed to initialize microwindows!\n");
		exit(1);
	}

	fd_audio = av_audio_fd();
	fd_video = av_video_fd();
	av_attach_fb();
	av_play();

	a52_state = a52_init (accel);

	if (aspect != -1)
		av_set_aspect(aspect);

	if (output != -1)
		av_set_output(output);

	if ((handle=demux_init(1024*1024*4)) == NULL) {
		fprintf(stderr, "failed to initialize demuxer\n");
		exit(1);
	}
	demux_set_display_size(handle, width, height);

	if (mythtv_server)
		mythtv_init(mythtv_server, -1);

	pthread_create(&ac3_thread, NULL, ac3_start, NULL);

	if (gui_init(mythtv_server, replaytv_server) < 0) {
		fprintf(stderr, "failed to initialize gui!\n");
		exit(1);
	}

	/*
	 * XXX: moving to uClibc seems to have exposed a problem...
	 *
	 * It appears that uClibc must make more use of malloc than glibc
	 * (ie, uClibc probably frees memory when not needed).  This is
	 * exposing a problem with memory exhaustion under linux where
	 * mvpmc needs memory, and it will hang until linux decides to
	 * flush some of its NFS pages.
	 *
	 * BTW, the heart of this problem is the linux buffer cache.  Since
	 * all file systems use the buffer cache, and linux assumes that
	 * the buffer cache is flushable, all hell breaks loose when you
	 * have a ramdisk.  Essentially, Linux is waiting for something
	 * that will never happen (the ramdisk to get flushed to backing
	 * store).
	 *
	 * So, force the stack and the heap to grow, and leave a hole
	 * in the heap for future calls to malloc().
	 */
	{
#define HOLE_SIZE (1024*1024*1)
#define PAGE_SIZE 4096
		int i, n = 0;
		char *ptr[HOLE_SIZE/PAGE_SIZE];
		char *last = NULL, *guard;
		char stack[1024*512];

		for (i=0; i<HOLE_SIZE/PAGE_SIZE; i++) {
			if ((ptr[i]=malloc(PAGE_SIZE)) != NULL) {
				*(ptr[i]) = 0;
				n++;
				last = ptr[i];
			}
		}

		for (i=1; i<1024; i++) {
			guard = malloc(1024*i);
			if ((unsigned int)guard < (unsigned int)last) {
				if (guard)
					free(guard);
			} else {
				break;
			}
		}
		if (guard)
			*guard = 0;

		memset(stack, 0, sizeof(stack));

		for (i=0; i<HOLE_SIZE/PAGE_SIZE; i++) {
			if (ptr[i] != NULL)
				free(ptr[i]);
		}

		if (guard)
			printf("Created hole in heap of size %d bytes\n",
			       n*PAGE_SIZE);
		else
			printf("Failed to create hole in heap\n");
	}

	mvpw_set_idle(NULL);
	mvpw_event_loop();

	return 0;
}

void
re_exec(void)
{
	int i, dt;

	dt = getdtablesize();

	for (i=3; i<dt; i++)
		close(i);

	printf("calling exec()...\n");

	pthread_kill_other_threads_np();
	execv(saved_argv[0], saved_argv);

	exit(1);
}