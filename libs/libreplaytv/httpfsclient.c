/*
 * Copyright (C) 2004 John Honeycutt
 * Copyright (C) 2002 John Todd Larason <jtl@molehill.org>
 *
 * Parts based on ReplayPC 0.3 by Matthew T. Linehan and others
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "sleep.h"
#include "crypt.h"
#include "rtvlib.h"
#include "httpfsclient.h"

#define ARGBUFSIZE 2048
static int make_httpfs_url(char *dst, size_t size, const rtv_device_info_t *device, 
                           const char *command, va_list args) 
{
    char * d, * tag, * value, *argp;
    int argno = 0;
    size_t l, argl;
    char argbuf[ARGBUFSIZE];          /* XXX bad, should fix this */

    (void) size;

    l = strlen(device->ipaddr) + strlen(command) + strlen("http:///httpfs-?");
    if (l >= size) {
        RTV_ERRLOG("make_httpfs_url: address + command too long for buffer\n");
        return -1;
    }
    
    d = dst;
    d += sprintf(d, "http://%s/httpfs-%s?", device->ipaddr, command);

    argp = argbuf;
    argl = 0;
    while ((tag = va_arg(args, char *)) != NULL) {
        value = va_arg(args, char *);
        if (value == NULL) {
            continue;
        }
        if (argno)
            argl++;
        argl += strlen(tag)+1+strlen(value);
        if (argl >= sizeof(argbuf)) {
            RTV_ERRLOG("make_httpfs_url: with arg %s, argbuf too small\n", tag);
            return -1;
        }
        if (argno)
            *argp++ = '&';
        argp += sprintf(argp, "%s=%s", tag, value);
        argno++;
    }
    RTV_DBGLOG(RTVLOG_CMD, "%s: argno=%d argbuf: %s\n", __FUNCTION__,argno, argbuf);
    
    if (device->version.major >= 5 ||
        (device->version.major == 4 && device->version.minor >= 3)) {
        unsigned char ctext[ARGBUFSIZE+32] ;
        u32 ctextlen;
        unsigned int i;

        if (l + strlen("__Q_=") + 2*argl + 32 >= size) {
            RTV_ERRLOG("make_httpfs_url: encrypted args too large for buffer\n");
            return -1;
        }

        strcpy(d, "__Q_=");
        d += strlen(d);
        RTV_DBGLOG(RTVLOG_CMD, "%s: d = %s\n argb = %s\n", __FUNCTION__, dst, argbuf);
        rtv_encrypt(argbuf, argl, ctext, sizeof ctext, &ctextlen, 1);
        for (i = 0; i < ctextlen; i++)
            d += sprintf(d, "%02x", ctext[i]);
    } else {
        if (l + argl >= size) {
            RTV_ERRLOG("make_httpfs_url: args too large for buffer\n");
            return -1;
        }
        strcpy(d, argbuf);
    }
    
    return 0;
}

static int add_httpfs_headers(struct hc * hc)
{
    hc_add_req_header(hc, "Authorization",   "Basic Uk5TQmFzaWM6QTd4KjgtUXQ=" );
    hc_add_req_header(hc, "User-Agent",      "Replay-HTTPFS/1");
    hc_add_req_header(hc, "Accept-Encoding", "text/plain");

    return 0;
}

static struct hc * make_request(const rtv_device_info_t *device, const char * command,
                                va_list ap)
{
    struct hc * hc = NULL;
    char url[URLSIZE];
    int http_status;

    RTV_DBGLOG(RTVLOG_CMD, "%s: ip_addr=%s cmd=%s\n", __FUNCTION__, device->ipaddr, command);    
    if (make_httpfs_url(url, sizeof url, device, command, ap) < 0)
        goto exit;

    hc = hc_start_request(url);
    if (!hc) {
        RTV_ERRLOG("make_request(): hc_start_request(): %d=>%s\n", errno, strerror(errno)); 
        goto exit;
    } 

    if (add_httpfs_headers(hc)) {
        goto exit;
    }

    hc_send_request(hc, NULL);

    http_status = hc_get_status(hc);
    if (http_status/100 != 2) {
        RTV_ERRLOG("http status %d\n", http_status);
        goto exit;
    }

    return hc;
    
exit:
    if (hc)
        hc_free(hc);
    return NULL;
}

    
static unsigned long hfs_do_simple(char **presult, const rtv_device_info_t *device, const char * command, ...)
{
    va_list       ap;
    struct hc *   hc;
    char *        tmp, * e;
    unsigned long rtv_status;
    
    RTV_DBGLOG(RTVLOG_CMD, "%s: ip_addr=%s cmd=%s\n", __FUNCTION__, device->ipaddr, command);    
    va_start(ap, command);
    hc = make_request(device, command, ap);
    va_end(ap);

    if (!hc)
        return -1;

    tmp = hc_read_all(hc);
    hc_free(hc);

    e = strchr(tmp, '\n');
    if (e) {
        *presult = strdup(e+1);
        rtv_status = strtoul(tmp, NULL, 10);
        free(tmp);
        return rtv_status;
    } else if (hc_get_status(hc) == 204) {
        *presult = NULL;
        free(tmp);
        return 0;
    } else {
        RTV_ERRLOG("end of httpfs status line not found\n");
        return -1;
    }
}

struct hfs_data
{
    void (*fn)(unsigned char *, size_t, void *);
    void * v;
    unsigned long status;
    u16 msec_delay;
    u8 firsttime;
};

/* XXX should this free the buf, or make a new one on the first block
 * and let our caller free them if it wants? */
static void hfs_callback(unsigned char * buf, size_t len, void * vd)
{
    struct hfs_data * data = vd;
    unsigned char * buf_data_start;
    
    if (data->firsttime) {
        unsigned char * e;

        data->firsttime = 0;

        /* First line: error code */
        e = strchr(buf, '\n');
        if (e)
            *e = '\0';
        data->status = strtoul(buf, NULL, 16);

        e++;
        len -= (e - buf);
        buf_data_start = e;
    } else {
        buf_data_start = buf;
    }

    data->fn(buf_data_start, len, data->v);
    free(buf);

    if (data->msec_delay)
        rtv_sleep(data->msec_delay);
}

static unsigned long hfs_do_chunked(void (*fn)(unsigned char *, size_t, void *),
                                    void                    *v,
                                    const rtv_device_info_t *device,
                                   __u16                    msec_delay,
                                   rtv_mergechunks_t        mergechunks,
                                   const char              *command,
                                   ...)
{
    struct hfs_data data;
    struct hc *   hc;
    va_list ap;
    
    RTV_DBGLOG(RTVLOG_CMD, "%s: ip_addr=%s cmd=%s\n", __FUNCTION__, device->ipaddr, command);    
    RTV_DBGLOG(RTVLOG_CMD, "%s: cback=%p cback_data=%p delay=%u\n", __FUNCTION__, fn, v, msec_delay);    
    va_start(ap, command);
    hc = make_request(device, command, ap);
    va_end(ap);

    if (!hc)
        return -1;

    memset(&data, 0, sizeof data);
    data.fn         = fn;
    data.v          = v;
    data.firsttime  = 1;
    data.msec_delay = msec_delay;
    
    hc_read_pieces(hc, hfs_callback, &data, mergechunks);
    hc_free(hc);

    return data.status;
}

unsigned long hfs_do_post_simple(char **presult, const rtv_device_info_t *device,
                                 int (*fn)(unsigned char *, size_t, void *),
                                 void *v,
                                 unsigned long size,
                                 const char *command, ...)
{
    char          buf[URLSIZE];
    va_list       ap;
    struct hc *   hc;
    char *        tmp, * e;
    int           http_status;
    unsigned long rtv_status;
    
    va_start(ap, command);
    if (make_httpfs_url(buf, sizeof buf, device, command, ap) < 0)
        return -1;
    va_end(ap);

    RTV_DBGLOG(RTVLOG_CMD, "%s: ip_addr=%s cmd=%s\n", __FUNCTION__, device->ipaddr, command);    
    hc = hc_start_request(buf);
    if (!hc) {
        RTV_ERRLOG("hfs_do_simple(): hc_start_request(): %d=>%s", errno, strerror(errno));
        return -1;
    } 
    sprintf(buf, "%lu", size);
    if (add_httpfs_headers(hc) != 0)
        return -1;
    
    hc_add_req_header(hc, "Content-Length",  buf);
    
    hc_post_request(hc, fn, v);

    http_status = hc_get_status(hc);
    if (http_status/100 != 2) {
        RTV_ERRLOG("http status %d\n", http_status);
        hc_free(hc);
        return http_status;
    }
    
    tmp = hc_read_all(hc);
    hc_free(hc);

    e = strchr(tmp, '\n');
    if (e) {
        *presult = strdup(e+1);
        rtv_status = strtoul(tmp, NULL, 10);
        free(tmp);
        return rtv_status;
    } else if (http_status == 204) {
        *presult = NULL;
        free(tmp);
        return 0;
    } else {
        RTV_ERRLOG("end of httpfs status line not found\n");
        return -1;
    }
}

static int vstrcmp(const void * v1, const void * v2)
{
   return (strcmp(*(char **)v1, *(char **)v2));
}

//+***********************************************************************************
//                         PUBLIC FUNCTIONS
//+***********************************************************************************

// allocates and returns rtv_fs_volume_t  struct, update volinfo** 
// Returns 0 for success
//
int rtv_get_volinfo( const rtv_device_info_t  *device, const char *name, rtv_fs_volume_t **volinfo )
{
   char             *data;
   unsigned long     status;
   rtv_fs_volume_t  *rec;
   int               num_lines, x;
   char            **lines;

   status = hfs_do_simple(&data, device,
                          "volinfo",
                          "name", name,
                          NULL);
   if (status != 0) {
      RTV_ERRLOG("%s:  hfs_do_simple returned %ld\n", __FUNCTION__, status);
      free(data);
      *volinfo = NULL;
      return status;
   }
   rec = *volinfo = malloc(sizeof(rtv_fs_volume_t));
   memset(rec, 0, sizeof(rtv_fs_volume_t));
   rec->name = malloc(strlen(name)+1);
   strcpy(rec->name, name);

   num_lines = split_lines(data, &lines);

   for (x = 0; x < num_lines; x++) {
      if (strncmp(lines[x], "cap=", 4) == 0) {
         sscanf(lines[x]+4, "%"U64F"d", &(rec->size));
      } else if (strncmp(lines[x], "inuse=", 6) == 0) {
         sscanf(lines[x]+6, "%"U64F"d", &(rec->used));
      }
      else {
         RTV_WARNLOG("%s: unknown response line: %s\n", __FUNCTION__, lines[x]);
      }
   }
   rec->size_k = rec->size / 1024;
   rec->used_k = rec->used / 1024;
   free(lines);
   free(data);
   return(0);;
}


void rtv_free_volinfo(rtv_fs_volume_t **volinfo) 
{
   rtv_fs_volume_t  *rec;

   if (  volinfo == NULL ) return;
   if ( *volinfo == NULL ) return;

   rec = *volinfo;
   if ( rec->name != NULL ) free(rec->name);
   free(*volinfo);
   *volinfo = NULL;
}


void rtv_print_volinfo(const rtv_fs_volume_t *volinfo) 
{
   if (  volinfo == NULL ) {
      RTV_PRT("volinfo object is NULL!\n");
      return;
   }

   RTV_PRT("RTV Disk Volume: %s\n", volinfo->name);
   RTV_PRT("                 size: %"U64F"d %lu(MB)\n", volinfo->size, volinfo->size_k / 1024);
   RTV_PRT("                 used: %"U64F"d %lu(MB)\n", volinfo->used, volinfo->used_k / 1024);

}


// updates  pre-allocated rtv_fs_file_t *fileinfo struct.
// Returns 0 for success
//
int rtv_get_file_info( const rtv_device_info_t  *device, const char *name,  rtv_fs_file_t *fileinfo )
{
   char           *data     = NULL;
   char            type     = RTV_FS_UNKNOWN;
   u64             filetime = 0;
   u64             size     = 0;
   unsigned long   status;
   unsigned int    num_lines, x;
   char          **lines;

   memset(fileinfo, 0, sizeof(rtv_fs_file_t));
   status = hfs_do_simple(&data, device,
                          "fstat",
                          "name", name,
                          NULL);
   if (status != 0) {
      RTV_ERRLOG("%s: hfs_do_simple fstat failed: %ld\n", __FUNCTION__, status);
      if ( data != NULL) free(data);
      return(status);
   }

   num_lines = split_lines(data, &lines);
   for (x = 0; x < num_lines; x++) {
      if (strncmp(lines[x], "type=", 5) == 0) {
         type = lines[x][5];
      } else if (strncmp(lines[x], "size=", 5) == 0) {
         sscanf(lines[x]+5, "%"U64F"d", &size);
      } else if (strncmp(lines[x], "ctime=", 6) == 0) {
         sscanf(lines[x]+6, "%"U64F"d", &filetime);
      } else if (strncmp(lines[x], "perm=", 5) == 0) {
         //nothing
      }
      else {
         RTV_WARNLOG("%s: unknown response line: %s\n", __FUNCTION__, lines[x]);
      }
   }

   fileinfo->name = malloc(strlen(name)+1);
   strcpy(fileinfo->name, name);
   fileinfo->type     = type;
   fileinfo->time     = filetime;
   fileinfo->time_str = rtv_format_time(filetime + 3000); //add 3 seconds to time
   if ( type == 'f' ) {
      fileinfo->size     = size;
      fileinfo->size_k   = size / 1024;
   }

   //rtv_print_file_info(fileinfo);
   free(lines);
   free(data);
   return(0);
}


void rtv_free_file_info(rtv_fs_file_t *fileinfo) 
{
   if (  fileinfo == NULL ) return;

   if ( fileinfo->name     != NULL ) free(fileinfo->name);
   if ( fileinfo->time_str != NULL ) free(fileinfo->time_str);
   memset(fileinfo, 0, sizeof(rtv_fs_file_t));
}


void rtv_print_file_info(const rtv_fs_file_t *fileinfo) 
{
   if (  fileinfo == NULL ) {
      RTV_PRT("fileinfo object is NULL!\n");
      return;
   }

   RTV_PRT("RTV File Status: %s\n", fileinfo->name);
   RTV_PRT("                 type: %c\n", fileinfo->type);
   if ( fileinfo->type == 'f' ) {
      RTV_PRT("                 size: %"U64F"d %lu(MB)\n", fileinfo->size, fileinfo->size_k / 1024);
      RTV_PRT("                 time: %"U64F"d\n", fileinfo->time);
      RTV_PRT("                       %s\n", fileinfo->time_str);
   }
}


// allocates and returns rtv_fs_filelist_t struct. updates filelist** 
// Returns 0 for success
//
int rtv_get_filelist( const rtv_device_info_t  *device, const char *name, int details, rtv_fs_filelist_t **filelist )
{
   char               *data;
   unsigned long       status;
   rtv_fs_filelist_t  *rec;
   unsigned int        num_lines, x;
   char              **lines;
   rtv_fs_file_t       fileinfo;


   // Get file status of the path passed in.
   //
   status = rtv_get_file_info(device, name, &fileinfo);
   if (status != 0) {
      RTV_ERRLOG("%s:  rtv_get_file_info returned %ld\n", __FUNCTION__, status);
      free(data);
      *filelist = NULL;
      return status;
   }
   
   // The path is a single file being listed.
   // 
   if ( fileinfo.type == 'f' ) {
      rec = *filelist = malloc(sizeof(rtv_fs_filelist_t));
      memset(rec, 0, sizeof(rtv_fs_filelist_t));
      rec->files = malloc(sizeof(rtv_fs_file_t));
      memcpy(rec->files, &fileinfo, sizeof(rtv_fs_file_t));
      rec->pathname = malloc(strlen(name)+1);
      strcpy(rec->pathname, name);
      rec->num_files = 1;
      return(0);
   }

   // The path is a directory
   //
   status = hfs_do_simple(&data, device,
                          "ls",
                          "name", name,
                          NULL);
   if (status != 0) {
      RTV_ERRLOG("%s:  hfs_do_simple returned %ld\n", __FUNCTION__, status);
      free(data);
      *filelist = NULL;
      return status;
   }

   num_lines = split_lines(data, &lines);
   qsort(lines, num_lines, sizeof(char *), vstrcmp);

   rec = *filelist = malloc(sizeof(rtv_fs_filelist_t));
   memset(rec, 0, sizeof(rtv_fs_filelist_t));
   rec->pathname = malloc(strlen(name)+1);
   strcpy(rec->pathname, name);
   rec->num_files = num_lines;  
   rec->files = malloc(sizeof(rtv_fs_file_t) * num_lines);
   memset(rec->files, 0, sizeof(rtv_fs_file_t) * num_lines);
   //printf("*** struct: %p filesbase: %p: numfiles: %d\n", rec, rec->files, num_lines);

   if ( details == 0 ) {
      // Just list the file names
      //
      for (x = 0; x < num_lines; x++) {
         rec->files[x].name = malloc(strlen(lines[x]) + 1);
         strcpy(rec->files[x].name, lines[x]); 
      }
   }
   else {
      // Get all the file info
      //
      for (x = 0; x < num_lines; x++) {
         char pathfile[256];
         snprintf(pathfile, 255, "%s/%s", rec->pathname, lines[x]);
         status = rtv_get_file_info( device, pathfile,  &(rec->files[x]));
         if ( status != 0 ) {
            RTV_ERRLOG("%s: rtv_get_file_info returned: %ld\n", __FUNCTION__, status);
            break;
         }
      }
   }
   
   free(lines);
   free(data);
   return(0);
}


void rtv_free_file_list( rtv_fs_filelist_t **filelist ) 
{
   rtv_fs_filelist_t  *rec;
   unsigned int        x;

   if (  filelist == NULL ) return;
   if ( *filelist == NULL ) return;

   rec = *filelist;
   
   for (x=0; x < rec->num_files; x++) {
      rtv_free_file_info(&(rec->files[x]));
   }

   if ( rec->pathname != NULL ) free(rec->pathname);
   free(*filelist);
   *filelist = NULL;
}


void rtv_print_file_list(const rtv_fs_filelist_t *filelist, int detailed) 
{
   unsigned int x;

   if (  filelist == NULL ) {
      RTV_PRT("filelist object is NULL!\n");
      return;
   }

   RTV_PRT("RTV File listing\n");
   RTV_PRT("PATH: %s       Number of files: %u\n", filelist->pathname, filelist->num_files);
   RTV_PRT("------------------------------------------\n");
   for (x=0; x < filelist->num_files; x++) {
      printf("%-25s", filelist->files[x].name);
      if (detailed) {
         printf("   %19s    type=%c  size: %11"U64F"d %7lu(MB)", 
                filelist->files[x].time_str, filelist->files[x].type, filelist->files[x].size, filelist->files[x].size_k / 1024 );
      }
      printf("\n");
   }

}


// read a file from the RTV.
// Callback data returned in 128KB chunks
// Returns 0 for success
//
__u32  rtv_read_file( const rtv_device_info_t *device, 
                      const char              *filename, 
                      __u64                    pos,        //fileposition
                      __u64                    size,       //amount of file to read ( 0 reads all of file )
                      unsigned int             ms_delay,   //mS delay between reads
                      void                     (*callback_fxn)(unsigned char *, size_t, void *),
                      void                    *callback_data                                     )
{
   __u32 status;
   char  pos_str[256];
   char  size_str[256];
   
   snprintf(pos_str, 255, "%"U64F"d", pos);

   if ( size != 0 ) {
      snprintf(size_str, 255, "%"U64F"d", size);
      status = hfs_do_chunked(callback_fxn, callback_data, device, ms_delay, RTV_MERGECHUNKS_4,
                              "readfile",
                              "pos",  pos_str,
                              "size", size_str,
                              "name", filename,
                              NULL);
   }
   else {
      status = hfs_do_chunked(callback_fxn, callback_data, device, ms_delay, RTV_MERGECHUNKS_4,
                              "readfile",
                              "pos",  pos_str,
                              "name", filename,
                              NULL);
   }
   return(status);
} 