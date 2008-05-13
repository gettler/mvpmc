/*
 *  Copyright (C) 2008, Jon Gettler
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

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>

#include <cmyth.h>
#include <mvp_refmem.h>

#define MAX_CONN	32
#define MAX_BSIZE	(256*1024*3)
#define MIN_BSIZE	(1024*2)

struct myth_conn {
	char *host;
	cmyth_conn_t control;
	cmyth_proglist_t list;
};

struct path_info {
	char *host;
	char *file;
};

struct file_info {
	cmyth_file_t file;
	off_t offset;
	struct path_info *info;
};

static struct myth_conn conn[MAX_CONN];

static FILE *F = NULL;

static int tcp_control = 4096;
static int tcp_program = 0;
static int port = 6543;

#define debug(fmt...) 					\
	{						\
		if (F != NULL) {			\
			fprintf(F, fmt);		\
			fflush(F);			\
		}					\
	}

static int
lookup_server(char *host)
{
	int i, j = -1;
	cmyth_conn_t control;

	debug("%s(): host '%s'\n", __FUNCTION__, host);

	for (i=0; i<MAX_CONN; i++) {
		if (conn[i].host && (strcmp(conn[i].host, host) == 0)) {
			break;
		}
		if (j < 0) {
			j = i;
		}
	}

	if (i == MAX_CONN) {
		if (j < 0) {
			debug("%s(): %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if ((control=cmyth_conn_connect_ctrl(host, port, 16*1024,
						     tcp_control)) == NULL) {
			debug("%s(): %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		conn[j].host = strdup(host);
		conn[j].control = control;
		conn[j].list = NULL;
		i = j;
	}

	control = conn[i].control;

	return i;
}

static int
lookup_path(const char *path, struct path_info *info)
{
	char *f;
	char *p = strdup(path);
	int ret = 0;

	if (strcmp(path, "/") == 0) {
		info->host = NULL;
		info->file = NULL;
		goto out;
	}

	if ((f=strchr(p+1, '/')) == NULL) {
		info->host = strdup(p+1);
		info->file = NULL;
		goto out;
	}

	*(f++) = '\0';

	if (strchr(f, '/') == NULL) {
		info->host = strdup(p+1);
		info->file = strdup(f);
	} else {
		ret = -1;
	}

	debug("%s(): host '%s' file '%s'\n", __FUNCTION__,
	      info->host, info->file);

out:
	if (p)
		free(p);

	return ret;
}

static void *myth_init(struct fuse_conn_info *conn)
{
	return 0;
}

static void myth_destroy(void *arg)
{
	int i;

	for (i=0; i<MAX_CONN; i++) {
		if (conn[i].list) {
			ref_release(conn[i].list);
		}
		if (conn[i].control) {
			ref_release(conn[i].control);
		}
	}
}

static int
do_open(cmyth_proginfo_t prog, struct fuse_file_info *fi)
{
	cmyth_conn_t c = NULL;
	cmyth_file_t f = NULL;
	char *host;

	if (F) {
		cmyth_dbg_all();
		stderr = F;
	}

	if ((host=cmyth_proginfo_host(prog)) == NULL) {
		return -1;
	}

	if ((c=cmyth_conn_connect_ctrl(host, port, 1024,
				       tcp_control)) == NULL) {
		return -1;
	}

	if ((f=cmyth_conn_connect_file(prog, c, MAX_BSIZE,
				       tcp_program)) == NULL) {
		return -1;
	}

	ref_release(host);
	ref_release(c);

	fi->fh = (uint64_t)(unsigned long)f;

	return 0;
}

static int myth_open(const char *path, struct fuse_file_info *fi)
{
	int i;
	struct path_info info;
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;

	debug("%s(): path '%s'\n", __FUNCTION__, path);

	memset(&info, 0, sizeof(info));
	if (lookup_path(path, &info) < 0) {
		return -ENOENT;
	}

	if (info.file == NULL) {
		return -ENOENT;
	}

	fi->fh = 0;

	if ((i=lookup_server(info.host)) < 0) {
		return -ENOENT;
	}

	control = conn[i].control;

	if (conn[i].list == NULL) {
		list = cmyth_proglist_get_all_recorded(control);
		conn[i].list = list;
	} else {
		list = conn[i].list;
	}
	count = cmyth_proglist_get_count(list);

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		char *pn;

		prog = cmyth_proglist_get_item(list, i);
		pn = cmyth_proginfo_pathname(prog);

		if (strcmp(pn+1, info.file) == 0) {
			if (do_open(prog, fi) < 0) {
				ref_release(pn);
				return -ENOENT;
			}
			ref_release(pn);
			return 0;
		}

		ref_release(pn);
	}

	return -ENOENT;
}

static int myth_release(const char *path, struct fuse_file_info *fi)
{
	cmyth_file_t f;

	debug("%s(): path '%s'\n", __FUNCTION__, path);

	if (fi->fh) {
		f = (cmyth_file_t)(unsigned long)(fi->fh);
		ref_release(f);
		fi->fh = 0;
	}

	return 0;
}

static int myth_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			off_t offset, struct fuse_file_info *fi)
{
	struct path_info info;
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;
	int i;

	debug("%s(): path '%s'\n", __FUNCTION__, path);

	memset(&info, 0, sizeof(info));
	if (lookup_path(path, &info) < 0) {
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	if (info.host == NULL) {
		return 0;
	}

	if ((i=lookup_server(info.host)) < 0) {
		return 0;
	}

	control = conn[i].control;

	if (conn[i].list == NULL) {
		list = cmyth_proglist_get_all_recorded(control);
		conn[i].list = list;
	} else {
		list = conn[i].list;
	}
	count = cmyth_proglist_get_count(list);

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		long long len;
		char *fn, *pn;
		struct stat st;

		prog = cmyth_proglist_get_item(list, i);
		pn = cmyth_proginfo_pathname(prog);
		len = cmyth_proginfo_length(prog);

		fn = pn+1;

		memset(&st, 0, sizeof(st));
		st.st_mode = S_IFREG | 0644;
		st.st_size = len;

		debug("%s(): file '%s' len %lld\n", __FUNCTION__, fn, len);
		filler(buf, fn, &st, 0);

		ref_release(prog);
		ref_release(pn);
	}

	return 0;
}

static int myth_getattr(const char *path, struct stat *stbuf)
{
	struct path_info info;
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;
	int i;

	debug("%s(): path '%s'\n", __FUNCTION__, path);

	memset(&info, 0, sizeof(info));
	if (lookup_path(path, &info) < 0) {
		return -ENOENT;
	}

        memset(stbuf, 0, sizeof(struct stat));
        if (info.host == NULL) {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
		return 0;
	}

	if (info.file == NULL) {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
		return 0;
	}

	if ((i=lookup_server(info.host)) < 0) {
		return -ENOENT;
	}


	control = conn[i].control;

	if (conn[i].list == NULL) {
		list = cmyth_proglist_get_all_recorded(control);
		conn[i].list = list;
	} else {
		list = conn[i].list;
	}

	stbuf->st_mode = S_IFREG | 0644;
	stbuf->st_nlink = 1;

	count = cmyth_proglist_get_count(list);

	debug("%s(): file '%s'\n", __FUNCTION__, info.file);

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		long long len;
		char *pn;

		prog = cmyth_proglist_get_item(list, i);
		pn = cmyth_proginfo_pathname(prog);

		if (strcmp(pn+1, info.file) == 0) {
			cmyth_timestamp_t ts;
			time_t t;
			len = cmyth_proginfo_length(prog);
			debug("%s(): file '%s' len %lld\n",
			      __FUNCTION__, pn+1, len);
			stbuf->st_size = len;
			ts = cmyth_proginfo_rec_end(prog);
			t = cmyth_timestamp_to_unixtime(ts);
			stbuf->st_atime = t;
			stbuf->st_mtime = t;
			stbuf->st_ctime = t;
			ref_release(prog);
			ref_release(pn);
			ref_release(ts);
			return 0;
		}
		ref_release(prog);
		ref_release(pn);
	}

	return -ENOENT;
}

static int myth_read(const char *path, char *buf, size_t size, off_t offset,
		     struct fuse_file_info *fi)
{
	cmyth_file_t f;
	int n, len, tot;

	debug("%s(): path '%s'\n", __FUNCTION__, path);

	if (fi->fh == 0) {
		return -ENOENT;
	}

	f = (cmyth_file_t)(unsigned long)(fi->fh);

	tot = 0;
	len = cmyth_file_request_block(f, size);
	while (tot < len) {
		n = cmyth_file_get_block(f, buf+tot, len-tot);
		if (n >= 0) {
			tot += n;
		}
		if (n < 0)
			break;
	}

	debug("%s(): size %d len %d ret %d\n", __FUNCTION__, size, len, tot);

	return tot;
}

static struct fuse_operations myth_oper = {
	.init = myth_init,
	.destroy = myth_destroy,
	.open = myth_open,
	.release = myth_release,
	.readdir = myth_readdir,
	.getattr = myth_getattr,
	.read = myth_read,
};

int
main(int argc, char **argv)
{
	F = fopen("debug.fuse", "w+");

	fuse_main(argc, argv, &myth_oper, NULL);

	return 0;
}
