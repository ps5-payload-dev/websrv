/* Copyright (C) 2025 John Törnblom

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
 * Internal Server Error (500)
 **/
#define PAGE_500                                 \
  "<html>"                                       \
  "  <head>"                                     \
  "    <title>Internal server error</title>"     \
  "  </head>"                                    \
  "  <body>Internal server error</body>"         \
  "</html>"


/**
 * State machine for rendering directory listings.
 **/
typedef struct dir_read_sm {
  enum {
    DIR_READ_HEAD,
    DIR_READ_BODY,
    DIR_READ_TAIL,
    DIR_READ_NULL,
  } state;
  struct {
    char path[PATH_MAX];
    DIR* dir;
    dev_t dev;
  } props;
} dir_read_sm_t;



/**
 * Obtain the character encoding for a file mode.
 **/
static char
modechar(const struct stat* st, const dev_t parent_dev) {
  if(S_ISDIR(st->st_mode) && st->st_dev != parent_dev) {
    // quick and dirty way to check if theres another device mounted
    return 'm';
  }
  if(S_ISDIR(st->st_mode)) {
    return 'd';
  }
  if(S_ISBLK(st->st_mode)) {
    return 'b';
  }
  if(S_ISCHR(st->st_mode)) {
    return 'c';
  }
  if(S_ISLNK(st->st_mode)) {
    return 'l';
  }
  if(S_ISFIFO(st->st_mode)) {
    return 'p';
  }
  if(S_ISSOCK(st->st_mode)) {
    return 's';
  }
  return '-';
}


/**
 * Remove superfluous forward slashes from the given path.
 **/
static void
normalize_path(char *path) {
  char *src = path;
  char *dst = path;
  size_t len;

  while(*src) {
    *dst = *src;
    if(*src == '/') {
      while(src[1] == '/') {
	src++;
      }
    }
    dst++;
    src++;
  }

  *dst = 0;

  len = dst - path;
  if(len > 1 && path[len-1] == '/') {
    path[len-1] = 0;
  }
}


/**
 * Read the contents of a directory, and render it as JSON.
 *
 * Implemented as a state machine:
 *
 *  ↦ [DIR_READ_HEAD] → [DIR_READ_BODY] → [DIR_READ_TAIL] → [DIR_READ_NULL]
 *                             ↺                                   ↺
 **/
static ssize_t
dir_read_json(void *cls, uint64_t pos, char *buf, size_t max) {
  dir_read_sm_t* sm = (dir_read_sm_t*)cls;
  struct dirent *entry;
  struct stat st;

  if(max < 512) {
    return 0;
  }

  switch(sm->state) {
  case DIR_READ_HEAD:
    sm->state = DIR_READ_BODY;
    return snprintf(buf, max,
		    "[{"			\
		    "\"name\": \".\","		\
		    "\"mode\": \"d\","		\
		    "\"mtime\": 0,"		\
		    "\"size\": 0"		\
		    "}");

  case DIR_READ_BODY:
    if(!(entry=readdir(sm->props.dir))) {
      sm->state = DIR_READ_TAIL;
      return 0;
    }
    if(!strcmp(entry->d_name, ".") ||
       !strcmp(entry->d_name, "..")) {
      return 0;
    }

    if(fstatat(dirfd(sm->props.dir), entry->d_name, &st, 0)) {
      return 0;
    }

    return snprintf(buf, max,
		    ",{"\
		    "\"name\": \"%s\","\
		    "\"mode\": \"%c\","\
		    "\"mtime\": %ld,"\
		    "\"size\": %ld"\
		    "}",
		    entry->d_name, modechar(&st, sm->props.dev),
		    st.st_mtim.tv_sec, st.st_size);

  case DIR_READ_TAIL:
    sm->state = DIR_READ_NULL;
    return snprintf(buf, max, "]");

  case DIR_READ_NULL:
  default:
    return MHD_CONTENT_READER_END_OF_STREAM;
  }
}


/**
 * Read the contents of a directory, and render it as HTML.
 *
 * Implemented as a state machine:
 *
 *  ↦ [DIR_READ_HEAD] → [DIR_READ_BODY] → [DIR_READ_TAIL] → [DIR_READ_NULL]
 *                             ↺                                   ↺
 **/
static ssize_t
dir_read_html(void *cls, uint64_t pos, char *buf, size_t max) {
  dir_read_sm_t* sm = (dir_read_sm_t*)cls;
  struct dirent *entry;
  size_t len = strlen(sm->props.path);

  if(max < 512) {
    return 0;
  }

  switch(sm->state) {
  case DIR_READ_HEAD:
    sm->state = DIR_READ_BODY;
    return snprintf(buf, max,
		    "<!DOCTYPE html>"					\
		    "<html>"						\
		    "  <head>"						\
		    "    <title>Index of %s</title>"			\
		    "  </head>"						\
		    "  <body>"						\
		    "    <h1>Index of %s</h1>"				\
		    "    <ul>"						\
		    , sm->props.path, sm->props.path);

  case DIR_READ_BODY:
    if(!(entry=readdir(sm->props.dir))) {
      sm->state = DIR_READ_TAIL;
      return 0;
    }
    if(!strcmp(entry->d_name, ".") ||
       !strcmp(entry->d_name, "..")) {
      return 0;
    }

    return snprintf(buf, max, "<li><a href=\"/fs%s%s%s\">%s</a></li>",
		    sm->props.path, (sm->props.path[len - 1] == '/') ? "" : "/",
            entry->d_name, entry->d_name);

  case DIR_READ_TAIL:
    sm->state = DIR_READ_NULL;
    return snprintf(buf, max, "</ul></body></html>");

  case DIR_READ_NULL:
  default:
    return MHD_CONTENT_READER_END_OF_STREAM;
  }
}


/**
 * Close a directory.
 **/
static void
dir_close(void *cls) {
  dir_read_sm_t* sm = (dir_read_sm_t*)cls;
  closedir(sm->props.dir);
  free(sm);
}


/**
 * Respond to a directory listing request.
 **/
static enum MHD_Result
dir_request(struct MHD_Connection *conn, const char* path) {
  MHD_ContentReaderCallback dir_read_cb = &dir_read_html;
  const char* mime = "text/html";
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  dir_read_sm_t* sm;
  const char* fmt;
  struct stat st;
  DIR *dir = 0;

  fmt = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "fmt");
  if(fmt && !strcmp(fmt, "json")) {
    dir_read_cb = &dir_read_json;
    mime = "application/json";
  }

  if(!stat(path, &st)) {
    dir = opendir(path);
  }

  if(!dir) {
    if((resp=MHD_create_response_from_buffer(strlen(PAGE_404), PAGE_404,
					     MHD_RESPMEM_PERSISTENT))) {
      ret = websrv_queue_response(conn, MHD_HTTP_NOT_FOUND, resp);
      MHD_destroy_response(resp);
    }
    return ret;
  }

  if(!(sm=calloc(1, sizeof(dir_read_sm_t)))) {
    closedir(dir);
    if((resp=MHD_create_response_from_buffer(strlen(PAGE_500), PAGE_500,
                                             MHD_RESPMEM_PERSISTENT))) {
      MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html");
      ret = websrv_queue_response(conn, MHD_HTTP_INTERNAL_SERVER_ERROR, resp);
      MHD_destroy_response(resp);
    }
    return ret;
  }

  sm->state = DIR_READ_HEAD;
  sm->props.dir = dir;
  sm->props.dev = st.st_dev;
  strncpy(sm->props.path, path, sizeof(sm->props.path));
  normalize_path(sm->props.path);

  if((resp=MHD_create_response_from_callback(MHD_SIZE_UNKNOWN, 32 * PAGE_SIZE,
					     dir_read_cb, sm,
					     &dir_close))) {
    MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, mime);
    ret = websrv_queue_response (conn, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
    return ret;
  }

  closedir(dir);
  free(sm);

  return MHD_NO;
}


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
  fclose(file);
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


enum MHD_Result
fs_request(struct MHD_Connection *conn, const char* url) {
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  const char* path = url+3;
  struct stat st = {0};

  if(!strlen(path)) {
    return dir_request(conn, "/");
  }

  if(stat(path, &st)) {
    if((resp=MHD_create_response_from_buffer(strlen(PAGE_404), PAGE_404,
					     MHD_RESPMEM_PERSISTENT))) {
      ret = websrv_queue_response(conn, MHD_HTTP_NOT_FOUND, resp);
      MHD_destroy_response(resp);
    }
    return ret;
  }

  if(S_ISDIR(st.st_mode)) {
    return dir_request(conn, path);
  } else {
    return file_request(conn, path);
  }
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
