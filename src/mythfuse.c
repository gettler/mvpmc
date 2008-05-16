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
#define MAX_FILES	32
#define MAX_BSIZE	(256*1024*3)
#define MIN_BSIZE	(1024*2)

struct myth_conn {
	char *host;
	cmyth_conn_t control;
	cmyth_proglist_t list;
};

struct path_info {
	char *host;
	char *dir;
	char *file;
};

struct file_info {
	cmyth_file_t file;
	off_t offset;
	struct path_info *info;
	char *buf;
	size_t n;
	off_t start;
};

struct dir_cb {
	char *name;
	int (*readdir)(struct path_info*, void*, fuse_fill_dir_t,
		       off_t, struct fuse_file_info*);
	int (*getattr)(struct path_info*, struct stat*);
	int (*open)(int, struct path_info*, struct fuse_file_info *fi);
};

static struct myth_conn conn[MAX_CONN];

static struct file_info files[MAX_FILES];

static FILE *F = NULL;

static int tcp_control = 4096;
static int tcp_program = 128*1024;
static int port = 6543;

static int rd_files(struct path_info*, void*, fuse_fill_dir_t, off_t,
		    struct fuse_file_info*);
static int ga_files(struct path_info*, struct stat*);
static int o_files(int, struct path_info*, struct fuse_file_info *fi);
static int rd_titles(struct path_info*, void*, fuse_fill_dir_t, off_t,
		     struct fuse_file_info*);
static int ga_titles(struct path_info*, struct stat*);
static int o_titles(int, struct path_info*, struct fuse_file_info *fi);

static struct dir_cb dircb[] = {
	{ "files", rd_files, ga_files, o_files },
	{ "titles", rd_titles, ga_titles, o_titles },
	{ NULL, NULL },
};

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
	char *f, *p;
	char *tmp = strdup(path);
	int ret = 0;
	char *parts[8];
	int i = 0;

	memset(info, 0, sizeof(info));

	if (strcmp(path, "/") == 0) {
		goto out;
	}

	memset(parts, 0, sizeof(parts));

	p = tmp;
	do {
		if ((f=strchr(p+1, '/')) != NULL) {
			*f = '\0';
			parts[i++] = strdup(p+1);
			p = f;
		}
	} while ((f != NULL) && (i < 8));

	debug("%s(): found %d parts\n", __FUNCTION__, i);

	if (i == 8) {
		ret = -ENOENT;
		goto out;
	}

	parts[i] = strdup(p+1);

	if ((i >= 2) && 
	    ((strcmp(parts[1], "files") != 0) &&
	     (strcmp(parts[1], "titles") != 0))) {
		ret = -ENOENT;
		goto out;
	}

	info->host = parts[0];
	info->dir = parts[1];
	info->file = parts[2];

	debug("%s(): host '%s' file '%s'\n", __FUNCTION__,
	      info->host, info->file);

out:
	if (tmp)
		free(tmp);

	debug("%s(): ret %d\n", __FUNCTION__, ret);

	return ret;
}

static void *myth_init(struct fuse_conn_info *conn)
{
	return 0;
}

static void myth_destroy(void *arg)
{
#if 0
	int i;

	for (i=0; i<MAX_CONN; i++) {
		if (conn[i].list) {
			ref_release(conn[i].list);
		}
		if (conn[i].control) {
			ref_release(conn[i].control);
		}
	}
#endif
}

static int
do_open(cmyth_proginfo_t prog, struct fuse_file_info *fi, int i)
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

	if ((c=cmyth_conn_connect_ctrl(host, port, 16*1024,
				       tcp_control)) == NULL) {
		return -1;
	}

	if ((f=cmyth_conn_connect_file(prog, c, MAX_BSIZE,
				       tcp_program)) == NULL) {
		return -1;
	}

	ref_release(host);
	ref_release(c);

	files[i].file = f;
	files[i].buf = malloc(MAX_BSIZE);
	files[i].start = 0;
	files[i].n = 0;

	fi->fh = i;

	return 0;
}

static int o_files(int f, struct path_info *info, struct fuse_file_info *fi)
{
	int i;
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;

	if ((i=lookup_server(info->host)) < 0) {
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

		if (strcmp(pn+1, info->file) == 0) {
			if (do_open(prog, fi, f) < 0) {
				ref_release(pn);
				ref_release(prog);
				return -ENOENT;
			}
			ref_release(pn);
			ref_release(prog);
			return 0;
		}

		ref_release(pn);
		ref_release(prog);
	}

	return -ENOENT;
}

static int o_titles(int f, struct path_info *info, struct fuse_file_info *fi)
{
	int i;
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;

	if ((i=lookup_server(info->host)) < 0) {
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
		char tmp[512];
		char *t, *s;

		prog = cmyth_proglist_get_item(list, i);
		t = cmyth_proginfo_title(prog);
		s = cmyth_proginfo_subtitle(prog);

		snprintf(tmp, sizeof(tmp), "%s - %s.mpg", t, s);

		if (strcmp(tmp, info->file) == 0) {
			if (do_open(prog, fi, f) < 0) {
				ref_release(t);
				ref_release(s);
				ref_release(prog);
				return -ENOENT;
			}
			ref_release(t);
			ref_release(s);
			ref_release(prog);
			return 0;
		}

		ref_release(t);
		ref_release(s);
		ref_release(prog);
	}

	return -ENOENT;
}

static int myth_open(const char *path, struct fuse_file_info *fi)
{
	int i, f;
	struct path_info info;

	debug("%s(): path '%s'\n", __FUNCTION__, path);

	memset(&info, 0, sizeof(info));
	if (lookup_path(path, &info) < 0) {
		return -ENOENT;
	}

	if (info.file == NULL) {
		return -ENOENT;
	}

	for (f=0; f<MAX_FILES; f++) {
		if (files[f].file == NULL) {
			break;
		}
	}
	if (f == MAX_FILES) {
		return -ENFILE;
	}

	fi->fh = -1;

	i = 0;
	while (dircb[i].name) {
		if (strcmp(info.dir, dircb[i].name) == 0) {
			dircb[i].open(f, &info, fi);
			return 0;
		}
		i++;
	}

	return -ENOENT;
}

static int myth_release(const char *path, struct fuse_file_info *fi)
{
	cmyth_file_t f;
	int i = fi->fh;

	debug("%s(): path '%s'\n", __FUNCTION__, path);

	if (fi->fh >= 0) {
		f = files[i].file;
		ref_release(f);
		if (files[i].buf) {
			free(files[i].buf);
		}
		memset(files+i, 0, sizeof(struct file_info));
		fi->fh = -1;
	}

	return 0;
}

static int
rd_files(struct path_info *info, void *buf, fuse_fill_dir_t filler,
	 off_t offset, struct fuse_file_info *fi)
{
	int i;
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;

	if ((i=lookup_server(info->host)) < 0) {
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
		st.st_mode = S_IFREG | 0444;
		st.st_size = len;

		debug("%s(): file '%s' len %lld\n", __FUNCTION__, fn, len);
		filler(buf, fn, &st, 0);

		ref_release(prog);
		ref_release(pn);
	}

	return 0;
}

static int
rd_titles(struct path_info *info, void *buf, fuse_fill_dir_t filler,
	  off_t offset, struct fuse_file_info *fi)
{
	int i;
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;

	if ((i=lookup_server(info->host)) < 0) {
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
		char *fn, *pn, *t, *s;
		struct stat st;
		char tmp[512];

		prog = cmyth_proglist_get_item(list, i);
		pn = cmyth_proginfo_pathname(prog);
		t = cmyth_proginfo_title(prog);
		s = cmyth_proginfo_subtitle(prog);
		len = cmyth_proginfo_length(prog);

		snprintf(tmp, sizeof(tmp), "%s - %s.mpg", t, s);

		fn = pn+1;

		memset(&st, 0, sizeof(st));
		st.st_mode = S_IFREG | 0444;
		st.st_size = len;

		debug("%s(): file '%s' len %lld\n", __FUNCTION__, fn, len);
		filler(buf, tmp, &st, 0);

		ref_release(prog);
		ref_release(pn);
		ref_release(t);
		ref_release(s);
	}

	return 0;
}

static int myth_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			off_t offset, struct fuse_file_info *fi)
{
	struct path_info info;
	int i;

	debug("%s(): path '%s'\n", __FUNCTION__, path);

	memset(&info, 0, sizeof(info));
	if (lookup_path(path, &info) < 0) {
		return -ENOENT;
	}


	if (info.host == NULL) {
		for (i=0; i<MAX_CONN; i++) {
			if (conn[i].host) {
				filler(buf, conn[i].host, NULL, 0);
			}
		}

		goto finish;
	}

	if (info.dir == NULL) {
		i = 0;
		while (dircb[i].name) {
			filler(buf, dircb[i].name, NULL, 0);
			i++;
		}
		goto finish;
	}

	i = 0;
	while (dircb[i].name) {
		if (strcmp(info.dir, dircb[i].name) == 0) {
			dircb[i].readdir(&info, buf, filler, offset, fi);
			goto finish;
		}
		i++;
	}

	return -ENOENT;

finish:
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	return 0;
}

static int ga_files(struct path_info *info, struct stat *stbuf)
{
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;
	int i;

	if ((i=lookup_server(info->host)) < 0) {
		return -ENOENT;
	}

	control = conn[i].control;

	if (conn[i].list == NULL) {
		list = cmyth_proglist_get_all_recorded(control);
		conn[i].list = list;
	} else {
		list = conn[i].list;
	}

	stbuf->st_mode = S_IFREG | 0444;
	stbuf->st_nlink = 1;

	count = cmyth_proglist_get_count(list);

	debug("%s(): file '%s'\n", __FUNCTION__, info->file);

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		long long len;
		char *pn;

		prog = cmyth_proglist_get_item(list, i);
		pn = cmyth_proginfo_pathname(prog);

		if (strcmp(pn+1, info->file) == 0) {
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

static int ga_titles(struct path_info *info, struct stat *stbuf)
{
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;
	int i;

	if ((i=lookup_server(info->host)) < 0) {
		return -ENOENT;
	}

	control = conn[i].control;

	if (conn[i].list == NULL) {
		list = cmyth_proglist_get_all_recorded(control);
		conn[i].list = list;
	} else {
		list = conn[i].list;
	}

	stbuf->st_mode = S_IFREG | 0444;
	stbuf->st_nlink = 1;

	count = cmyth_proglist_get_count(list);

	debug("%s(): file '%s'\n", __FUNCTION__, info->file);

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		long long len;
		char *title, *s;
		char tmp[512];

		prog = cmyth_proglist_get_item(list, i);
		title = cmyth_proginfo_title(prog);
		s = cmyth_proginfo_subtitle(prog);

		snprintf(tmp, sizeof(tmp), "%s - %s.mpg", title, s);

		if (strcmp(tmp, info->file) == 0) {
			cmyth_timestamp_t ts;
			time_t t;
			len = cmyth_proginfo_length(prog);
			debug("%s(): file '%s' len %lld\n",
			      __FUNCTION__, tmp, len);
			stbuf->st_size = len;
			ts = cmyth_proginfo_rec_end(prog);
			t = cmyth_timestamp_to_unixtime(ts);
			stbuf->st_atime = t;
			stbuf->st_mtime = t;
			stbuf->st_ctime = t;
			ref_release(prog);
			ref_release(ts);
			ref_release(title);
			ref_release(s);
			return 0;
		}
		ref_release(prog);
		ref_release(title);
		ref_release(s);
	}

	return -ENOENT;
}

static int myth_getattr(const char *path, struct stat *stbuf)
{
	struct path_info info;
	int i;

	debug("%s(): path '%s'\n", __FUNCTION__, path);

	memset(&info, 0, sizeof(info));
	if (lookup_path(path, &info) < 0) {
		return -ENOENT;
	}

        memset(stbuf, 0, sizeof(struct stat));
        if (info.host == NULL) {
                stbuf->st_mode = S_IFDIR | 0555;
                stbuf->st_nlink = 2;
		return 0;
	}

	if (info.dir == NULL) {
                stbuf->st_mode = S_IFDIR | 0555;
                stbuf->st_nlink = 2;
		return 0;
	}

	i = 0;
	while (dircb[i].name) {
		if (strcmp(info.dir, dircb[i].name) == 0) {
			break;
		}
		i++;
	}
	if (dircb[i].name == NULL) {
		return -ENOENT;
	}

	if (info.file == NULL) {
                stbuf->st_mode = S_IFDIR | 0555;
                stbuf->st_nlink = 2;
		return 0;
	}

	i = 0;
	while (dircb[i].name) {
		if (strcmp(info.dir, dircb[i].name) == 0) {
			return dircb[i].getattr(&info, stbuf);
		}
		i++;
	}

	return -ENOENT;
}

static int
do_seek(int i, off_t offset, int whence)
{
	off_t pos;

	pos = cmyth_file_seek(files[i].file, offset, whence);

	return pos;
}

static int
fill_buffer(int i, char *buf, size_t size)
{
	int tot, len, n;

	tot = 0;
	len = cmyth_file_request_block(files[i].file, size);
	while (tot < len) {
		n = cmyth_file_get_block(files[i].file, buf+tot, len-tot);
		if (n >= 0) {
			tot += n;
		}
		if (n < 0)
			break;
	}

	return tot;
}

static int myth_read(const char *path, char *buf, size_t size, off_t offset,
		     struct fuse_file_info *fi)
{
	int tot;

	debug("%s(): path '%s' size %d\n", __FUNCTION__, path, size);

	if (fi->fh < 0) {
		return -ENOENT;
	}

	if (files[fi->fh].offset != offset) {
		if (do_seek(fi->fh, offset, SEEK_SET) < 0) {
			return -EINVAL;
		}
	}

	tot = 0;
	while (size > 0) {
		int len = (size > MAX_BSIZE) ? MAX_BSIZE : size;
		if ((len=fill_buffer(fi->fh, buf+tot, len)) <= 0)
			break;
		size -= len;
		tot += len;
	}

	files[fi->fh].offset = offset + tot;

	debug("%s(): read %d bytes at %lld\n", __FUNCTION__, tot, offset);

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
	if (argc != 2) {
		printf("Usage: mythfuse <mountpoint>\n");
		exit(1);
	}

//	F = fopen("debug.fuse", "w+");

	fuse_main(argc, argv, &myth_oper, NULL);

	return 0;
}
