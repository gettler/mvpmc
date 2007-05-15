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
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "mvp_atomic.h"

#define PT_THREAD_MAX	128
#define PT_MSG_MAX	16
#define PT_SHARED_ENV	"PT_NOMMU_DATA"

typedef enum {
	PT_CMD_CREATE = 1,
	PT_CMD_EXIT,
} pt_cmd_t;

typedef struct {
	pid_t pid;
	void *(*start_routine)(void*);
	void *arg;
	int status;
	int running;
} pt_info_t;

typedef struct {
	int cmd;
	void *(*start_routine)(void*);
	void *arg;
} pt_msg_t;

typedef struct {
	pid_t manager;
	pt_info_t list[PT_THREAD_MAX];
	pt_msg_t msg[PT_MSG_MAX];
	mvp_atomic_t lock;
	char *path;
} pt_shared_t;

typedef pid_t pthread_t;
typedef int pthread_attr_t;

static volatile pt_shared_t *pt_shared;
static int initialized = 0;

static void
pthread_do_exit(void)
{
}

static void
sig_manager(int sig)
{
	printf("manager signal\n");

	if (sig == SIGCHLD) {
		int status, i = 0;
		pid_t p;

		while (i < PT_THREAD_MAX) {
			p = waitpid(pt_shared->list[i].pid, &status, WNOHANG);
			if (p == pt_shared->list[i].pid) {
				pt_shared->list[i].running = 0;
				pt_shared->list[i].status = status;
				printf("reaped thread %d\n", p);
			}
			i++;
		}
	}
}

static void
sig_thread(int sig)
{
}

static void
create_thread(int m)
{
	int i = 0;

	pt_shared->msg[m].cmd = 0;

	while ((i < PT_THREAD_MAX) && (pt_shared->list[i].pid != 0)) {
		i++;
	}

	printf("Create new thread %d...\n", i);

	if (vfork() == 0) {
		pt_shared->list[i].pid = getpid();
		pt_shared->list[i].start_routine = pt_shared->msg[m].start_routine;
		pt_shared->list[i].arg = pt_shared->msg[m].arg;
		pt_shared->list[i].running = 1;
		execl(pt_shared->path, pt_shared->path, NULL);
		exit(-1);
	}
}

static void*
pthread_manager(void)
{
	printf("pthread manager started\n");

	signal(SIGUSR2, sig_manager);
	signal(SIGCHLD, sig_manager);

	while (1) {
		int i = 0, n = 0;

		while (i < PT_MSG_MAX) {
			switch (pt_shared->msg[i].cmd) {
			case PT_CMD_CREATE:
				create_thread(i);
				break;
			default:
				break;
			}
			i++;
		}

		if (pt_shared->list[0].running) {
			if (kill(pt_shared->list[0].pid, 0) < 0) {
				pt_shared->list[0].running = 0;
			}
		}

		for (i=0; i<PT_THREAD_MAX; i++) {
			if (pt_shared->list[i].running) {
				n++;
			}
		}

		if (n == 0) {
			printf("All threads have exited...\n");
			exit(0);
		}

		printf("%d threads remain...\n", n);

		sleep(1);
	}

	return NULL;
}

static int
queue_create(pthread_t *thread, void *(*start_routine)(void*), void *arg)
{
	int i = 0;

	while (i < PT_MSG_MAX) {
		if (pt_shared->msg[i].cmd == 0) {
			pt_shared->msg[i].cmd = PT_CMD_CREATE;
			pt_shared->msg[i].start_routine = start_routine;
			pt_shared->msg[i].arg = arg;
			kill(pt_shared->manager, SIGUSR2);
			break;
		}
		i++;
	}

	return 0;
}

static void*
pthread_child(void)
{
	int i = 0;
	pid_t me = getpid();
	void *rc;

	atexit(pthread_do_exit);

	printf("start new thread...pid %d\n", getpid());

	while (!mvp_atomic_dec_and_test(&pt_shared->lock)) {
		mvp_atomic_inc(&pt_shared->lock);
		usleep(10000);
	}

	while (pt_shared->list[i].pid != me) {
		i++;
	}

	printf("%s(): i %d\n", __FUNCTION__, i);

	mvp_atomic_inc(&pt_shared->lock);

	printf("thread calling start routine %p\n",
	       pt_shared->list[i].start_routine);
	rc = pt_shared->list[i].start_routine(pt_shared->list[i].arg);

	return rc;
}

int
pthread_init(char *app)
{
	char *child = getenv(PT_SHARED_ENV);
	char *buf;

	if (child) {
		void *rc;

		pt_shared = (pt_shared_t*)strtoul(child, NULL, 0);

		printf("pthread manger is pid %d, shared %p, child '%s'\n",
		       pt_shared->manager, pt_shared, child);

		if (pt_shared->manager == getpid()) {
			rc = pthread_manager();
		} else {
			rc = pthread_child();
			printf("child thread exiting...\n");
		}

		exit((int)rc);
	}

	signal(SIGUSR2, sig_thread);

	if (initialized) {
		return -1;
	}

	if (access(app, X_OK) != 0) {
		return -1;
	}

	pt_shared = malloc(sizeof(pt_shared_t));
	memset((void*)pt_shared, 0, sizeof(pt_shared_t));

	pt_shared->path = strdup(app);
	printf("child path '%s'\n", pt_shared->path);

	mvp_atomic_set(&pt_shared->lock, 1);

	buf = malloc(16);
	snprintf(buf, 16, "%p", pt_shared);
	setenv(PT_SHARED_ENV, buf, 1);

	initialized = 1;

	printf("shared is %p, '%s'\n", pt_shared, buf);

	return 0;
}

int
pthread_create(pthread_t *thread, pthread_attr_t *attr,
	       void *(*start_routine)(void*), void *arg)
{
	if (pt_shared->manager == 0) {
		pid_t child;

		printf("no pthread manager found!\n");

		pt_shared->list[0].pid = getpid();
		pt_shared->list[0].running = 1;

		if ((child=vfork()) == 0) {
			printf("exec '%s' pid %d\n", pt_shared->path, getpid());
			pt_shared->manager = getpid();
			execl(pt_shared->path, pt_shared->path, NULL);
			exit(-1);
		}
	}

#if 0
	sleep(1);
#endif
	queue_create(thread, start_routine, arg);
	
	return 0;
}

int
pthread_kill(pthread_t thread, int signo)
{
	return 0;
}
