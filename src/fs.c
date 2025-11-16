/* Copyright (C) 2024 John Törnblom

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.  */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/user.h>

#include <microhttpd.h>


#include "fs.h"
#include "mime.h"
#include "websrv.h"


#ifndef PAGE_SIZE
#define PAGE_SIZE 0x4000
#endif


/**
 * File not found (404)
 **/
#define PAGE_404                      \
  "<html>"                            \
    "<head>"                          \
      "<title>File not found</title>" \
    "</head>"                         \
    "<body>File not found</body>"     \
  "</html>"


/**
 * State machine for rendering directory listing.
 **/
typedef struct dir_read_sm {
  const char *path;
  DIR* dir;
  int state;
  char* buf;
  dev_t st_dev;
} dir_read_sm_t;



/**
 * Read parts of a file on disk.
 **/
static ssize_t
file_read(void *cls, uint64_t pos, char *buf, size_t max) {
  FILE *file = cls;
  size_t len;

  if(fseek(file, pos, SEEK_SET)) {
    return MHD_CONTENT_READER_END_WITH_ERROR;
  }

  if(!(len=fread(buf, 1, max, file))) {
    if(ferror(file)) {
      return MHD_CONTENT_READER_END_WITH_ERROR;
    } else {
      return MHD_CONTENT_READER_END_OF_STREAM;
    }
  }

  return len;
}


/**
 * Close a file on disk.
 **/
static void
file_close(void *cls) {
  FILE *file = cls;

  if(file) {
    fclose(file);
  }
}


static char
stat_modechar(const struct stat* st, const dev_t parent_dev) {
  if(S_ISDIR(st->st_mode) && st->st_dev != parent_dev) return 'm'; // quick and dirty way to check if theres another device mounted (didnt want to make another syscall for every item)
  if(S_ISDIR(st->st_mode)) return 'd';
  if(S_ISBLK(st->st_mode)) return 'b';
  if(S_ISCHR(st->st_mode)) return 'c';
  if(S_ISLNK(st->st_mode)) return 'l';
  if(S_ISFIFO(st->st_mode)) return 'p';
  if(S_ISSOCK(st->st_mode)) return 's';
  return '-';
}


/**
 * Read the contents of a directory, and render it as json.
 **/
static ssize_t
dir_read_json(void *cls, uint64_t pos, char *buf, size_t max) {
  dir_read_sm_t* sm = cls;
  struct dirent *e;
  struct stat st;

  if(max < 512) {
    return 0;
  }

  if(sm->state == 0) {
    sm->state++;
    return snprintf(buf, max, "[{ \"name\": \".\", \"mode\": \"d\", \"mtime\": 0, \"size\": 0 }");
  }

  if(sm->state == 1) {
    if(!(e=readdir(sm->dir))) {
      sm->state++;
      return 0;
    }
    if(e->d_name[0] == '.') {
      return 0;
    }

    strncpy(sm->buf, sm->path, PATH_MAX);
    strncat(sm->buf, "/", PATH_MAX);
    strncat(sm->buf, e->d_name, PATH_MAX);


    if(stat(sm->buf, &st)) {
      return 0;
    }

    return snprintf(buf, max, ",{ \"name\": \"%s\", \"mode\": \"%c\", \"mtime\": %ld, \"size\": %ld }",
            e->d_name, stat_modechar(&st, sm->st_dev), st.st_mtim.tv_sec, st.st_size);
  }

  if(sm->state == 2) {
    sm->state++;
    return snprintf(buf, max, "]");
  }

  return MHD_CONTENT_READER_END_OF_STREAM;
}


/**
 * Read the contents of a directory, and render it as html.
 **/
static ssize_t
dir_read_html(void *cls, uint64_t pos, char *buf, size_t max) {
  dir_read_sm_t* sm = cls;
  struct dirent *e;

  if(max < 512) {
    return 0;
  }

  if(sm->state == 0) {
    sm->state++;
    return snprintf(buf, max,
		    "<!DOCTYPE html>"					\
		    "<html>"						\
		    "  <head>"						\
		    "    <title>Index of %s</title>"			\
		    "  </head>"						\
		    "  <body>"						\
		    "    <h1>Index of %s</h1>"				\
		    "    <ul>"						\
		    ,
		    sm->path, sm->path);
  }

  if(sm->state == 1) {
    if(!(e=readdir(sm->dir))) {
      sm->state++;
      return 0;
    }
    if(e->d_name[0] == '.') {
      return 0;
    }
    return snprintf(buf, max, "<li><a href=\"%s\">%s</a></li>",
		    e->d_name, e->d_name);
  }

  if(sm->state == 2) {
    sm->state++;
    return snprintf(buf, max, "</ul></body></html>");
  }

  return MHD_CONTENT_READER_END_OF_STREAM;
}


/**
 * Close a directory.
 **/
static void
dir_close(void *cls) {
  dir_read_sm_t* sm = cls;

  if(sm && sm->buf) {
    free(sm->buf);
    sm->buf = 0;
  }
  if(sm && sm->dir) {
    closedir(sm->dir);
    sm->dir = 0;
  }
}


/**
 * Respond to a file request.
 **/
static enum MHD_Result
file_request(struct MHD_Connection *conn, const char* path) {
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  const char* mime = 0;
  struct stat st;
  FILE *file = 0;

  if(!stat(path, &st)) {
    mime = mime_get_type(path);
    file = fopen(path, "rb");
  }

  if(!file) {
    if((resp=MHD_create_response_from_buffer(strlen(PAGE_404), PAGE_404,
					     MHD_RESPMEM_PERSISTENT))) {
      ret = websrv_queue_response(conn, MHD_HTTP_NOT_FOUND, resp);
      MHD_destroy_response(resp);
    }
    return ret;
  }

  if((resp=MHD_create_response_from_callback(st.st_size, 32 * PAGE_SIZE,
					     &file_read, file,
					     &file_close))) {
    if(mime) {
      MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, mime);
    }
    ret = websrv_queue_response (conn, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
    return ret;
  }

  fclose(file);

  return MHD_NO;
}


/**
 * Respond to a directory request.
 **/
static enum MHD_Result
dir_request(struct MHD_Connection *conn, const char* path) {
  MHD_ContentReaderCallback dir_read_cb;
  enum MHD_Result ret = MHD_NO;
  size_t len = strlen(path);
  struct MHD_Response *resp;
  char url[PATH_MAX];
  dir_read_sm_t sm;
  const char* mime;
  const char* fmt;
  DIR *dir;
  struct stat st;

  if(!len) {
    sprintf(url, "/fs/");
    if(!(resp=MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT))) {
      return MHD_NO;
    }

    MHD_add_response_header(resp, MHD_HTTP_HEADER_LOCATION, url);
    ret = websrv_queue_response(conn, MHD_HTTP_MOVED_PERMANENTLY, resp);
    MHD_destroy_response(resp);
    return ret;
  }

  if(!(dir=opendir(path))) {
    if((resp=MHD_create_response_from_buffer(strlen(PAGE_404), PAGE_404,
					     MHD_RESPMEM_PERSISTENT))) {
      ret = websrv_queue_response(conn, MHD_HTTP_NOT_FOUND, resp);
      MHD_destroy_response(resp);
    }
    return ret;
  }

  if (stat(path, &st)) {
    closedir(dir);
    return MHD_NO;
  }

  sm.dir = dir;
  sm.path = path;
  sm.state = 0;
  sm.buf = malloc(PATH_MAX+1);
  sm.st_dev = st.st_dev;

  fmt = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "fmt");
  if(fmt && !strcmp(fmt, "json")) {
    dir_read_cb = &dir_read_json;
    mime = "application/json";
  } else {
    dir_read_cb = &dir_read_html;
    mime = "text/html";
  }

  if((resp=MHD_create_response_from_callback(MHD_SIZE_UNKNOWN, 32*PAGE_SIZE,
					     dir_read_cb, &sm,
					     &dir_close))) {
    MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, mime);
    ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
    return ret;
  }

  closedir(dir);
  free(sm.buf);

  return MHD_NO;
}


enum MHD_Result
fs_request(struct MHD_Connection *conn, const char* url) {
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  const char* path = url+3;
  struct stat st;

  /* If the client requested "/fs" (no trailing slash) redirect to "/fs/"
   * so that relative links in the generated directory listing resolve
   * under /fs/ rather than the server root. */
  if(strcmp(url, "/fs") == 0) {
    if(!(resp=MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT))) {
      return MHD_NO;
    }

    MHD_add_response_header(resp, MHD_HTTP_HEADER_LOCATION, "/fs/");
    ret = websrv_queue_response(conn, MHD_HTTP_MOVED_PERMANENTLY, resp);
    MHD_destroy_response(resp);
    return ret;
  }

  if(!strlen(path)) {
    return dir_request(conn, "/");
  }

  if(!stat(path, &st)) {
    /* If this path is a directory but the URL doesn't end with '/'
     * redirect the client to the trailing-slash version so the browser
     * resolves relative links correctly (browsers treat a URL without
     * a trailing slash as a file and will resolve relative paths to the
     * parent directory). */
    if(S_ISDIR(st.st_mode)) {
      size_t url_len = strlen(url);
      if(url_len == 0 || url[url_len - 1] != '/') {
        char location[PATH_MAX];
        if(snprintf(location, PATH_MAX, "%s/", url) >= PATH_MAX) {
          /* shouldn't happen for reasonable URLs; fall back to no-op */
          return MHD_NO;
        }
        if(!(resp = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT))) {
          return MHD_NO;
        }
        MHD_add_response_header(resp, MHD_HTTP_HEADER_LOCATION, location);
        ret = websrv_queue_response(conn, MHD_HTTP_MOVED_PERMANENTLY, resp);
        MHD_destroy_response(resp);
        return ret;
      }

      return dir_request(conn, path);
    } else {
      return file_request(conn, path);
    }
  }

  if((resp=MHD_create_response_from_buffer(strlen(PAGE_404), PAGE_404,
					   MHD_RESPMEM_PERSISTENT))) {
    ret = websrv_queue_response(conn, MHD_HTTP_NOT_FOUND, resp);
    MHD_destroy_response(resp);
  }

  return ret;
}


uint8_t*
fs_readfile(const char* path, size_t* size) {
  uint8_t* buf;
  ssize_t len;
  FILE* file;

  if(!(file=fopen(path, "rb"))) {
    return 0;
  }

  if(fseek(file, 0, SEEK_END)) {
    return 0;
  }

  if((len=ftell(file)) < 0) {
    return 0;
  }

  if(fseek(file, 0, SEEK_SET)) {
    return 0;
  }

  if(!(buf=malloc(len))) {
    return 0;
  }

  if(fread(buf, 1, len, file) != len) {
    free(buf);
    return 0;
  }

  if(fclose(file)) {
    free(buf);
    return 0;
  }

  if(size) {
    *size = len;
  }

  return buf;
}


