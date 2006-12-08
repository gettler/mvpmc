/*
 *  Copyright (C) 2004-2006, Eric Lund, Jon Gettler
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

/**
 * \file cmyth_local.h
 * Local definitions which are internal to libcmyth.
 */

#ifndef __CMYTH_LOCAL_H
#define __CMYTH_LOCAL_H

#include <unistd.h>
#include <cmyth.h>
#include <time.h>
#include <pthread.h>
#include <mysql/mysql.h>

#define mutex __cmyth_mutex
extern pthread_mutex_t mutex;

/*
 * Some useful constants
 */
#define CMYTH_LONGLONG_LEN (sizeof("-18446744073709551616") - 1)
#define CMYTH_LONG_LEN (sizeof("-4294967296") - 1)
#define CMYTH_SHORT_LEN (sizeof("-65536") - 1)
#define CMYTH_BYTE_LEN (sizeof("-256") - 1)
#define CMYTH_TIMESTAMP_LEN (sizeof("YYYY-MM-DDTHH:MM:SS") - 1)
#define CMYTH_DATESTAMP_LEN (sizeof("YYYY-MM-DD") - 1)

/**
 * Atomically incremente a reference count variable.
 * \param ref address of atomic variable
 * \return incremented reference count
 */
typedef	unsigned cmyth_atomic_t;
static inline unsigned
__cmyth_atomic_increment(cmyth_atomic_t *ref)
{
	cmyth_atomic_t __val;
#if defined __i486__ || defined __i586__ || defined __i686__
	__asm__ __volatile__(
		"lock xaddl %0, (%1);"
		"     inc   %0;"
		: "=r" (__val)
		: "r" (ref), "0" (0x1)
		: "cc", "memory"
		);
#elif defined __i386__
	asm volatile (".byte 0xf0, 0x0f, 0xc1, 0x02" /*lock; xaddl %eax, (%edx) */
		      : "=a" (__val)
		      : "0" (1), "m" (*ref), "d" (ref)
		      : "memory");
	/* on the x86 __val is the pre-increment value, so normalize it. */
	++__val;
#elif defined __powerpc__
	asm volatile ("1:	lwarx   %0,0,%1\n"
		      "	addic.   %0,%0,1\n"
		      "	dcbt    %0,%1\n"
		      "	stwcx.  %0,0,%1\n"
		      "	bne-    1b\n"
		      "	isync\n"
		      : "=&r" (__val)
		      : "r" (ref)
		      : "cc", "memory");
#else
	/*
	 * Don't know how to atomic increment for a generic architecture
	 * so punt and just increment the value.
	 */
#warning unknown architecture, atomic increment is not...
	__val = ++(*ref);
#endif
	return __val;
}

/**
 * Atomically decrement a reference count variable.
 * \param ref address of atomic variable
 * \return incremented reference count
 */
static inline unsigned
__cmyth_atomic_decrement(cmyth_atomic_t *ref)
{
	cmyth_atomic_t __val;
#if defined __i486__ || defined __i586__ || defined __i686__
	__asm__ __volatile__(
		"lock xaddl %0, (%1);"
		"     inc   %0;"
		: "=r" (__val)
		: "r" (ref), "0" (0x1)
		: "cc", "memory"
		);
#elif defined __i386__
	asm volatile (".byte 0xf0, 0x0f, 0xc1, 0x02" /*lock; xaddl %eax, (%edx) */
		      : "=a" (__val)
		      : "0" (-1), "m" (*ref), "d" (ref)
		      : "memory");
	/* __val is the pre-decrement value, so normalize it */
	--__val;
#elif defined __powerpc__
	asm volatile ("1:	lwarx   %0,0,%1\n"
		      "	addic.   %0,%0,-1\n"
		      "	dcbt    %0,%1\n"
		      "	stwcx.  %0,0,%1\n"
		      "	bne-    1b\n"
		      "	isync\n"
		      : "=&r" (__val)
		      : "r" (ref)
		      : "cc", "memory");
#elif defined __sparcv9__
	cmyth_atomic_t __newval, __oldval = (*ref);
	do
		{
			__newval = __oldval - 1;
			__asm__ ("cas	[%4], %2, %0"
				 : "=r" (__oldval), "=m" (*ref)
				 : "r" (__oldval), "m" (*ref), "r"((ref)), "0" (__newval));
		}
	while (__newval != __oldval);
	/*  The value for __val is in '__oldval' */
	__val = __oldval;
#else
	/*
	 * Don't know how to atomic decrement for a generic architecture
	 * so punt and just decrement the value.
	 */
#warning unknown architecture, atomic deccrement is not...
	__val = --(*ref);
#endif
	return __val;
}
#define cmyth_atomic_inc __cmyth_atomic_inc
static inline int cmyth_atomic_inc(cmyth_atomic_t *a) {
	return __cmyth_atomic_increment(a);
};

#define cmyth_atomic_dec __cmyth_atomic_dec
static inline int cmyth_atomic_dec(cmyth_atomic_t *a) {
	return __cmyth_atomic_decrement(a);
};

#define cmyth_atomic_dec_and_test __cmyth_atomic_dec_and_test
static inline int cmyth_atomic_dec_and_test(cmyth_atomic_t *a) {
	return (__cmyth_atomic_decrement(a) == 0);
};

#define cmyth_atomic_set __cmyth_atomic_set
static inline void cmyth_atomic_set(cmyth_atomic_t *a, unsigned val) {
	*a = val;
};

/**
 * MythTV backend connection
 */
struct cmyth_conn {
	int		conn_fd;	/**< socket file descriptor */
	unsigned char	*conn_buf;	/**< connection buffer */
	int		conn_buflen;	/**< buffer size */
	int		conn_len;	/**< amount of data in buffer */
	int		conn_pos;	/**< current position in buffer */
	unsigned long	conn_version;	/**< protocol version */
	volatile int	conn_hang;	/**< is connection stuck? */
	int		conn_tcp_rcvbuf;/**< TCP receive buffer size */
};

/* Sergio: Added to support new livetv protocol */
struct cmyth_livetv_chain {
	char *chainid;
	int chain_ct;
	int chain_switch_on_create;
	int chain_current;
	void (*prog_update_callback)(cmyth_proginfo_t prog);
	cmyth_proginfo_t *progs;
	char **chain_urls;
	cmyth_file_t *chain_files; /* File pointers for the urls */
};

/* Sergio: Added to clean up database interaction */
struct cmyth_database {
	char * db_host;
	char * db_user;
	char * db_pass;
	char * db_name;
	MYSQL * mysql;
};	

/* Sergio: Added to clean up channel list handling */
struct cmyth_channel {
	long chanid;
	int channum;
	char chanstr[10];
	long cardids;/* A bit array of recorders/tuners supporting the channel */
	char *callsign;
	char *name;
};

struct cmyth_chanlist {
	cmyth_channel_t chanlist_list;
	int chanlist_sort_desc;
	int chanlist_count;
	int chanlist_alloc;
};

/* Sergio: Added to support the tvguide functionality */
struct cmyth_tvguide_progs {
	cmyth_program_t * progs;
	int count;
	int alloc;
};

struct cmyth_recorder {
	unsigned rec_have_stream;
	unsigned rec_id;
	char *rec_server;
	int rec_port;
	cmyth_ringbuf_t rec_ring;
	cmyth_conn_t rec_conn;
	/* Sergio: Added to support new livetv protocol */
	cmyth_livetv_chain_t rec_livetv_chain;
	cmyth_file_t rec_livetv_file;
	double rec_framerate;
};

/**
 * MythTV file connection
 */
struct cmyth_file {
	cmyth_conn_t file_data;		/**< backend connection */
	long file_id;			/**< file identifier */
	unsigned long long file_start;	/**< file start offest */
	unsigned long long file_length;	/**< file length */
	unsigned long long file_pos;	/**< current file position */
	cmyth_conn_t file_control;	/**< master backend connection */
};

struct cmyth_ringbuf {
	cmyth_conn_t conn_data;
	long file_id;
	char *ringbuf_url;
	unsigned long long ringbuf_size;
	unsigned long long file_length;
	unsigned long long file_pos;
	unsigned long long ringbuf_fill;
	char *ringbuf_hostname;
	int ringbuf_port;
};

struct cmyth_rec_num {
	char *recnum_host;
	unsigned short recnum_port;
	unsigned recnum_id;
};

struct cmyth_keyframe {
	unsigned long keyframe_number;
	unsigned long long keyframe_pos;
};

struct cmyth_posmap {
	unsigned posmap_count;
	struct cmyth_keyframe **posmap_list;
};

struct cmyth_freespace {
	unsigned long long freespace_total;
	unsigned long long freespace_used;
};

struct cmyth_timestamp {
	unsigned long timestamp_year;
	unsigned long timestamp_month;
	unsigned long timestamp_day;
	unsigned long timestamp_hour;
	unsigned long timestamp_minute;
	unsigned long timestamp_second;
	int timestamp_isdst;
};

struct cmyth_proginfo {
	char *proginfo_title;
	char *proginfo_subtitle;
	char *proginfo_description;
	char *proginfo_category;
	long proginfo_chanId;
	char *proginfo_chanstr;
	char *proginfo_chansign;
	char *proginfo_channame;  /* Deprecated in V8, simulated for compat. */
	char *proginfo_chanicon;  /* New in V8 */
	char *proginfo_url;
	long long proginfo_Length;
	cmyth_timestamp_t proginfo_start_ts;
	cmyth_timestamp_t proginfo_end_ts;
	unsigned long proginfo_conflicting; /* Deprecated in V8, always 0 */
	char *proginfo_unknown_0;   /* May be new 'conflicting' in V8 */
	unsigned long proginfo_recording;
	unsigned long proginfo_override;
	char *proginfo_hostname;
	long proginfo_source_id; /* ??? in V8 */
	long proginfo_card_id;   /* ??? in V8 */
	long proginfo_input_id;  /* ??? in V8 */
	char *proginfo_rec_priority;  /* ??? in V8 */
	long proginfo_rec_status; /* ??? in V8 */
	unsigned long proginfo_record_id;  /* ??? in V8 */
	unsigned long proginfo_rec_type;   /* ??? in V8 */
	unsigned long proginfo_rec_dups;   /* ??? in V8 */
	unsigned long proginfo_unknown_1;  /* new in V8 */
	cmyth_timestamp_t proginfo_rec_start_ts;
	cmyth_timestamp_t proginfo_rec_end_ts;
	unsigned long proginfo_repeat;   /* ??? in V8 */
	long proginfo_program_flags;
	char *proginfo_rec_profile;  /* new in V8 */
	char *proginfo_recgroup;    /* new in V8 */
	char *proginfo_chancommfree;    /* new in V8 */
	char *proginfo_chan_output_filters;    /* new in V8 */
	char *proginfo_seriesid;    /* new in V8 */
	char *proginfo_programid;    /* new in V12 */
	cmyth_timestamp_t proginfo_lastmodified;    /* new in V12 */
	char *proginfo_stars;    /* new in V12 */
	cmyth_timestamp_t proginfo_originalairdate;	/* new in V12 */
	char *proginfo_pathname;
	int proginfo_port;
        unsigned long proginfo_hasairdate;
	char *proginfo_host;
	unsigned long proginfo_version;
	char *proginfo_playgroup; /* new in v18 */
	char *proginfo_recpriority_2;  /* new in V25 */
	long proginfo_parentid; /* new in V31 */
	char *proginfo_storagegroup; /* new in v32 */
};

struct cmyth_proglist {
	cmyth_proginfo_t *proglist_list;
	long proglist_count;
};

/*
 * Private funtions in socket.c
 */
#define cmyth_send_message __cmyth_send_message
extern int cmyth_send_message(cmyth_conn_t conn, char *request);

#define cmyth_rcv_length __cmyth_rcv_length
extern int cmyth_rcv_length(cmyth_conn_t conn);

#define cmyth_rcv_string __cmyth_rcv_string
extern int cmyth_rcv_string(cmyth_conn_t conn,
			    int *err,
			    char *buf, int buflen,
			    int count);

#define cmyth_rcv_okay __cmyth_rcv_okay
extern int cmyth_rcv_okay(cmyth_conn_t conn, char *ok);

#define cmyth_rcv_version __cmyth_rcv_version
extern int cmyth_rcv_version(cmyth_conn_t conn, unsigned long *vers);

#define cmyth_rcv_byte __cmyth_rcv_byte
extern int cmyth_rcv_byte(cmyth_conn_t conn, int *err, char *buf, int count);

#define cmyth_rcv_short __cmyth_rcv_short
extern int cmyth_rcv_short(cmyth_conn_t conn, int *err, short *buf, int count);

#define cmyth_rcv_long __cmyth_rcv_long
extern int cmyth_rcv_long(cmyth_conn_t conn, int *err, long *buf, int count);
#define cmyth_rcv_u_long(c, e, b, n) cmyth_rcv_long(c, e, (long*)b, n)

#define cmyth_rcv_long_long __cmyth_rcv_long_long
extern int cmyth_rcv_long_long(cmyth_conn_t conn, int *err, long long *buf,
			       int count);
#define cmyth_rcv_u_long_long(c, e, b, n) cmyth_rcv_long_long(c, e, (long long*)b, n)

#define cmyth_rcv_ubyte __cmyth_rcv_ubyte
extern int cmyth_rcv_ubyte(cmyth_conn_t conn, int *err, unsigned char *buf,
			   int count);

#define cmyth_rcv_ushort __cmyth_rcv_ushort
extern int cmyth_rcv_ushort(cmyth_conn_t conn, int *err, unsigned short *buf,
			    int count);

#define cmyth_rcv_ulong __cmyth_rcv_ulong
extern int cmyth_rcv_ulong(cmyth_conn_t conn, int *err, unsigned long *buf,
			   int count);

#define cmyth_rcv_ulong_long __cmyth_rcv_ulong_long
extern int cmyth_rcv_ulong_long(cmyth_conn_t conn,
				int *err,
				unsigned long long *buf,
				int count);

#define cmyth_rcv_data __cmyth_rcv_data
extern int cmyth_rcv_data(cmyth_conn_t conn, int *err, unsigned char *buf,
			  int count);

#define cmyth_rcv_timestamp __cmyth_rcv_timestamp
extern int cmyth_rcv_timestamp(cmyth_conn_t conn, int *err,
			       cmyth_timestamp_t *ts_p,
			       int count);
#define cmyth_rcv_datetime __cmyth_rcv_datetime
extern int cmyth_rcv_datetime(cmyth_conn_t conn, int *err,
			      cmyth_timestamp_t *ts_p,
			      int count);

#define cmyth_rcv_proginfo __cmyth_rcv_proginfo
extern int cmyth_rcv_proginfo(cmyth_conn_t conn, int *err,
			      cmyth_proginfo_t buf,
			      int count);

#define cmyth_rcv_chaninfo __cmyth_rcv_chaninfo
extern int cmyth_rcv_chaninfo(cmyth_conn_t conn, int *err,
			      cmyth_proginfo_t buf,
			      int count);

#define cmyth_rcv_proglist __cmyth_rcv_proglist
extern int cmyth_rcv_proglist(cmyth_conn_t conn, int *err,
			      cmyth_proglist_t buf,
			      int count);

#define cmyth_rcv_keyframe __cmyth_rcv_keyframe
extern int cmyth_rcv_keyframe(cmyth_conn_t conn, int *err,
			      cmyth_keyframe_t buf,
			      int count);

#define cmyth_rcv_freespace __cmyth_rcv_freespace
extern int cmyth_rcv_freespace(cmyth_conn_t conn, int *err,
			       cmyth_freespace_t buf,
			       int count);

#define cmyth_rcv_recorder __cmyth_rcv_recorder
extern int cmyth_rcv_recorder(cmyth_conn_t conn, int *err,
			      cmyth_recorder_t buf,
			      int count);

#define cmyth_rcv_ringbuf __cmyth_rcv_ringbuf
extern int cmyth_rcv_ringbuf(cmyth_conn_t conn, int *err, cmyth_ringbuf_t buf,
			     int count);

/*
 * From proginfo.c
 */
#define cmyth_proginfo_string __cmyth_proginfo_string
extern char *cmyth_proginfo_string(cmyth_proginfo_t prog);

#define cmyth_chaninfo_string __cmyth_chaninfo_string
extern char *cmyth_chaninfo_string(cmyth_proginfo_t prog);

/*
 * From file.c
 */
#define cmyth_file_create __cmyth_file_create
extern cmyth_file_t cmyth_file_create(cmyth_conn_t control);

/*
 * From timestamp.c
 */
#define cmyth_timestamp_diff __cmyth_timestamp_diff
extern int cmyth_timestamp_diff(cmyth_timestamp_t, cmyth_timestamp_t);

/*
 * From mythtv_mysql.c
 */

extern MYSQL * cmyth_db_get_connection(cmyth_database_t db);


/*
 * From mysql_query.c
 */

typedef struct cmyth_mysql_query_s cmyth_mysql_query_t;

extern cmyth_mysql_query_t * cmyth_mysql_query_create(cmyth_database_t db, const char * query_string);

extern void cmyth_mysql_query_reset(cmyth_mysql_query_t *query);

extern int cmyth_mysql_query_param_long(cmyth_mysql_query_t * query,long param);

extern int cmyth_mysql_query_param_ulong(cmyth_mysql_query_t * query,unsigned long param);

extern int cmyth_mysql_query_param_int(cmyth_mysql_query_t * query,int param);

extern int cmyth_mysql_query_param_uint(cmyth_mysql_query_t * query,int param);

extern int cmyth_mysql_query_param_unixtime(cmyth_mysql_query_t * query, time_t param);

extern int cmyth_mysql_query_param_str(cmyth_mysql_query_t * query, const char *param);

extern char * cmyth_mysql_query_string(cmyth_mysql_query_t * query);

extern MYSQL_RES * cmyth_mysql_query_result(cmyth_mysql_query_t * query);



#endif /* __CMYTH_LOCAL_H */
