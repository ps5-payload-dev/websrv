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
#include <string.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/user.h>

#include <magic.h>
#include <microhttpd.h>

#include "fs.h"
#include "websrv.h"


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
stat_modeat(const char* folder, const char* filename) {
  char path[PATH_MAX];
  struct stat st;

  snprintf(path, PATH_MAX, "%s/%s", folder, filename);

  if(stat(path, &st)) return '-';

  if(S_ISDIR(st.st_mode)) return 'd';
  if(S_ISBLK(st.st_mode)) return 'b';
  if(S_ISCHR(st.st_mode)) return 'c';
  if(S_ISLNK(st.st_mode)) return 'l';
  if(S_ISFIFO(st.st_mode)) return 'p';
  if(S_ISSOCK(st.st_mode)) return 's';

  return '-';
}


/**
 * Read the contents of a directory, and render it as json.
 **/
static ssize_t
dir_read_json(void *cls, uint64_t pos, char *buf, size_t max) {
  dir_read_sm_t* sm = cls;
  struct dirent *e;

  if(max < 512) {
    return 0;
  }

  if(sm->state == 0) {
    sm->state++;
    return snprintf(buf, max, "[{ \"name\": \".\", \"mode\": \"d\"}");
  }

  if(sm->state == 1) {
    if(!(e=readdir(sm->dir))) {
      sm->state++;
      return 0;
    }
    if(e->d_name[0] == '.') {
      return 0;
    }

    return snprintf(buf, max, ",{ \"name\": \"%s\", \"mode\": \"%c\"}",
		    e->d_name, stat_modeat(sm->path, e->d_name));
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

  if(sm && sm->dir) {
    closedir(sm->dir);
  }
}


/**
 * Respond to a file request.
 **/
static enum MHD_Result
file_request(struct MHD_Connection *conn, const char* path) {
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  const char* mime;
  struct stat st;
  FILE *file = 0;
  magic_t m;

  if(!stat(path, &st)) {
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
    if((m=magic_open(MAGIC_MIME))) {
      if(!magic_load(m, NULL)) {
	if((mime=magic_file(m, path))) {
	  MHD_add_response_header(resp, "Content-Type", mime);
	}
      }
      magic_close(m);
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

  if(!len || path[len-1] != '/') {
    sprintf(url, "/fs%s/", path);
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

  sm.dir = dir;
  sm.path = path;
  sm.state = 0;

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
    MHD_add_response_header(resp, "Content-Type", mime);
    ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
    return ret;
  }

  closedir(dir);

  return MHD_NO;
}


enum MHD_Result
fs_request(struct MHD_Connection *conn, const char* url) {
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  const char* path = url+3;
  struct stat st;

  if(!strlen(path)) {
    return dir_request(conn, "/");
  }

  if(!stat(path, &st)) {
    if(S_ISREG(st.st_mode)) {
      return file_request(conn, path);
    } else {
      return dir_request(conn, path);
    }
  }

  if((resp=MHD_create_response_from_buffer(strlen(PAGE_404), PAGE_404,
					   MHD_RESPMEM_PERSISTENT))) {
    ret = websrv_queue_response(conn, MHD_HTTP_NOT_FOUND, resp);
    MHD_destroy_response(resp);
  }

  return ret;
}

