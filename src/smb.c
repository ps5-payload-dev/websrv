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

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <smb2/smb2.h>
#include <smb2/libsmb2.h>
#include <smb2/libsmb2-raw.h>

#include "smb.h"
#include "websrv.h"


/**
 * Bad Request (400)
 **/
#define PAGE_400                          \
  "<html>"                                \
  "  <head>"                              \
  "    <title>Bad request</title>"        \
  "  </head>"                             \
  "  <body>Bad request</body>"            \
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
 * Arguments used by the shares callback function.
 **/
typedef struct smb_request_shares_args {
  struct MHD_Connection *conn;
  enum MHD_Result result;
  int finished;
} smb_request_shares_args_t;


/**
 * Arguments used by dir callback functions.
 **/
typedef struct smb_request_dir_args {
  struct smb2_context *smb2;
  struct smb2dir* dir;
  char* path;
  int state;
} smb_request_dir_args_t;


/**
 * Arguments used by file callback functions.
 **/
typedef struct smb_request_file_args {
  struct smb2_context *smb2;
  struct smb2fh* file;
  size_t start;
} smb_request_file_args_t;


/**
 * Callback function used to transmit smb file data to a http request.
 **/
static ssize_t
smb_request_file_read_cb(void *ctx, uint64_t pos, char *buf, size_t max) {
  smb_request_file_args_t* args = (smb_request_file_args_t*)ctx;
  int ret;

  if(smb2_lseek(args->smb2, args->file, args->start+pos, SEEK_SET, 0) < 0) {
    return MHD_CONTENT_READER_END_WITH_ERROR;
  }

  ret = smb2_read(args->smb2, args->file, (uint8_t*)buf, max);
  if(ret < 0) {
    return MHD_CONTENT_READER_END_WITH_ERROR;
  } else if(ret == 0){
    return MHD_CONTENT_READER_END_OF_STREAM;
  } else {
    return ret;
  }
}


/**
 * Callback function used to close an smb file that has been transmitted
 * via a http request successfully.
 **/
static void
smb_request_file_close_cb(void *ctx) {
  smb_request_file_args_t* args = (smb_request_file_args_t*)ctx;

  smb2_close(args->smb2, args->file);
  smb2_destroy_context(args->smb2);
  free(args);
}


/**
 * Callback function used to transmit smb dir data to a http request.
 **/
static ssize_t
smb_request_dir_read_cb(void *ctx, uint64_t pos, char *buf, size_t max) {
  smb_request_dir_args_t* args = (smb_request_dir_args_t*)ctx;
  struct smb2dirent* ent;
  struct smb2_stat_64 st;
  char path[PATH_MAX];
  char mode = '-';

  if(args->state == 0) {
    args->state++;
    return snprintf(buf, max, "[\n  {\"name\":\".\",\"mode\":\"d\"}");
  }

  if(args->state == 2) {
    args->state++;
    return snprintf(buf, max, "\n]\n");
  }

  if(args->state == 3) {
    return MHD_CONTENT_READER_END_OF_STREAM;
  }

  if(!(ent=smb2_readdir(args->smb2, args->dir))) {
    args->state++;
    return 0;
  }

  if(!strcmp(".", ent->name)) {
    return 0;
  }

  if(args->path[0]) {
    snprintf(path, PATH_MAX, "%s/%s", args->path, ent->name);
  } else {
    snprintf(path, PATH_MAX, "%s", ent->name);
  }

  if(smb2_stat(args->smb2, path, &st) < 0) {
    return 0;
  }

  switch(st.smb2_type) {
  case SMB2_TYPE_DIRECTORY:
    mode = 'd';
    break;
  case SMB2_TYPE_FILE:
    mode = '-';
    break;
  case SMB2_TYPE_LINK:
    mode = 'l';
    break;
  default:
    mode = '-';
    break;
  }

  return snprintf(buf, max, ",\n  {\"name\":\"%s\",\"mode\":\"%c\"}",
                  ent->name, mode);
}


/**
 * Callback function used to close an smb dir that has been transmitted
 * via a http request successfully.
 **/
static void
smb_request_dir_close_cb(void *ctx) {
  smb_request_dir_args_t* args = (smb_request_dir_args_t*)ctx;

  smb2_closedir(args->smb2, args->dir);
  smb2_destroy_context(args->smb2);
  free(args->path);
  free(args);
}


/**
 * Respond to a http request with an internal error.
 **/
static enum MHD_Result
smb_response_error(struct MHD_Connection *conn, struct smb2_context *smb2) {
  int nterror = smb2_get_nterror(smb2);
  int err = nterror_to_errno(nterror);
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  int http_error;
  char* buf;

  switch(err) {
  case ECONNREFUSED:
    buf = strdup(strerror(err));
    http_error = MHD_HTTP_UNAUTHORIZED;
    break;

  case EACCES:
    buf = strdup(strerror(err));
    http_error = MHD_HTTP_FORBIDDEN;
    break;

  case EINVAL:
    buf = strdup(strerror(err));
    http_error = MHD_HTTP_BAD_REQUEST;
    break;

  case ENOENT:
    buf = strdup(strerror(err));
    http_error = MHD_HTTP_NOT_FOUND;
    break;

  case 0:
    buf = strdup(smb2_get_error(smb2));
    http_error = MHD_HTTP_INTERNAL_SERVER_ERROR;
    break;

  default:
    buf = strdup(strerror(err));
    http_error = MHD_HTTP_INTERNAL_SERVER_ERROR;
    break;
  }

  if(!buf) {
    if((resp=MHD_create_response_from_buffer(strlen(PAGE_500), PAGE_500,
                                             MHD_RESPMEM_PERSISTENT))) {
      MHD_add_response_header(resp, "Content-Type", "text/html");
      ret = websrv_queue_response(conn, MHD_HTTP_INTERNAL_SERVER_ERROR, resp);
      MHD_destroy_response(resp);
    }
    return ret;
  }

  if((resp=MHD_create_response_from_buffer(strlen(buf), buf,
                                           MHD_RESPMEM_MUST_FREE))) {
    MHD_add_response_header(resp, "Content-Type", "text/plain");
    ret = websrv_queue_response(conn, http_error, resp);
    MHD_destroy_response(resp);
  }

  return ret;
}


/**
 * Create a response for a dir request.
 **/
static struct MHD_Response*
smb_create_dir_response(struct smb2_context *smb2, struct smb2dir* dir,
                        const char* path) {
  smb_request_dir_args_t *args;
  struct MHD_Response *resp;

  if(!(args=malloc(sizeof(smb_request_dir_args_t)))) {
    return 0;
  }

  if(!path) {
    path = "";
  }
  args->smb2  = smb2;
  args->dir   = dir;
  args->path  = strdup(path);
  args->state = 0;

  resp = MHD_create_response_from_callback(MHD_SIZE_UNKNOWN, 32*0x1000,
                                           smb_request_dir_read_cb,
                                           args,
                                           smb_request_dir_close_cb);
  if(resp) {
    MHD_add_response_header(resp, "Content-Type", "application/json");
  } else {
    free(args->path);
    free(args);
  }

  return resp;
}


/**
 * Create a response for a file request.
 **/
static struct MHD_Response*
smb_create_file_response(struct smb2_context *smb2, struct smb2fh* file,
                         const char* range, size_t size) {
  smb_request_file_args_t *args;
  struct MHD_Response *resp;
  size_t start = 0;
  size_t end = size - 1;
  char buf[100];

  if(range && !strncmp("bytes=", range, 6)) {
    sscanf(range+6, "%lu-%lu", &start, &end);
    if(end <= 0 || end >= size) {
      end = size - 1;
    }
    if(start > end) {
      start = 0;
    }
  }

  if(smb2_lseek(smb2, file, start, SEEK_SET, 0) < 0) {
    return 0;
  }

  if(!(args=malloc(sizeof(smb_request_file_args_t)))) {
    return 0;
  }

  args->smb2 = smb2;
  args->file = file;
  args->start = start;

  resp = MHD_create_response_from_callback(end-start+1, 32 * 1024,
                                           smb_request_file_read_cb,
                                           args,
                                           smb_request_file_close_cb);
  if(!resp) {
    free(args);
  }

  MHD_add_response_header(resp, MHD_HTTP_HEADER_ACCEPT_RANGES, "bytes");

  if(range) {
    snprintf(buf, sizeof(buf), "bytes %zd-%zd/%zu", start, end, size);
    MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_RANGE, buf);

    snprintf(buf, sizeof(buf), "%zu", end-start+1);
    MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_LENGTH, buf);
  }

  return resp;
}


/**
 * Respond to a http request of a remote smb path.
 **/
static enum MHD_Result
smb_request_path(struct MHD_Connection *conn, const char* user,
                 const char* pass, const char* uri) {
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp = 0;
  struct smb2_context *smb2 = 0;
  struct smb2_url *url = 0;
  struct smb2fh* file = 0;
  struct smb2dir* dir = 0;
  struct smb2_stat_64 st;
  const char* range;

  if(!(smb2=smb2_init_context())) {
    perror("smb2_init_context"); // TODO: output posix error to http response.
    return ret;
  }

  if(!(url=smb2_parse_url(smb2, uri))) {
    ret = smb_response_error(conn, smb2);
    smb2_destroy_context(smb2);
    return ret;
  }

  smb2_set_user(smb2, user);
  smb2_set_password(smb2, pass);
  smb2_set_security_mode(smb2, SMB2_NEGOTIATE_SIGNING_ENABLED);

  if(smb2_connect_share(smb2, url->server, url->share, user) < 0) {
    if(pass && *pass == 0) {
      ret = smb_request_path(conn, user, 0, uri);
    } else {
      ret = smb_response_error(conn, smb2);
    }
    smb2_destroy_url(url);
    smb2_destroy_context(smb2);
    return ret;
  }

  if(smb2_stat(smb2, url->path, &st) < 0) {
    ret = smb_response_error(conn, smb2);
    smb2_destroy_context(smb2);
    smb2_destroy_url(url);
    return ret;
  }

  range = MHD_lookup_connection_value(conn, MHD_HEADER_KIND,
                                      MHD_HTTP_HEADER_RANGE);
  if(range && strncmp("bytes=", range, 6)) {
    range = 0;
  }

  switch(st.smb2_type) {
  case SMB2_TYPE_DIRECTORY:
    if(!(dir=smb2_opendir(smb2, url->path))) {
      ret = smb_response_error(conn, smb2);
      smb2_destroy_context(smb2);
      smb2_destroy_url(url);
      return ret;
    }
    resp = smb_create_dir_response(smb2, dir, url->path);
    break;

  case SMB2_TYPE_FILE:
    if(!(file=smb2_open(smb2, url->path, O_RDONLY))) {
      ret = smb_response_error(conn, smb2);
      smb2_destroy_context(smb2);
      smb2_destroy_url(url);
      return ret;
    }
    resp = smb_create_file_response(smb2, file, range, st.smb2_size);
    break;

  case SMB2_TYPE_LINK:
  default:
    break;
  }

  smb2_destroy_url(url);

  if(resp) {
    if(range) {
      ret = websrv_queue_response(conn, MHD_HTTP_PARTIAL_CONTENT, resp);
    } else {
      ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
    }
    MHD_destroy_response(resp);
    return ret;
  }

  if(file) {
    smb2_close(smb2, file);
  }
  if(dir) {
    smb2_closedir(smb2, dir);
  }

  smb2_destroy_context(smb2);

  return ret;
}


/**
 * Callback function used to transmit smb shares listings to a http request.
 **/
static void
smb_request_shares_cb(struct smb2_context *smb2, int status, void *data,
                      void *ctx) {
  smb_request_shares_args_t* args = (smb_request_shares_args_t*)ctx;
  struct srvsvc_NetrShareEnum_rep *rep = data;
  struct MHD_Response *resp;
  const char* share;
  size_t size;
  char *buf;
  char *ptr;

  if(status) {
    args->result = smb_response_error(args->conn, smb2);
    args->finished = 1;
    return;
  }

  size = rep->ses.ShareInfo.Level0.EntriesRead * (PATH_MAX + 20);
  if(!(buf=ptr=malloc(size))) {
    perror("malloc"); // TODO: output posix error to http response.
    args->result = MHD_NO;
    args->finished = 1;
    return;
  }

  ptr += sprintf(buf, "[\n");
  for(int i=0; i<rep->ses.ShareInfo.Level0.EntriesRead; i++) {
    share = rep->ses.ShareInfo.Level0.Buffer->share_info_0[i].netname.utf8;
    if(i) {
      ptr += sprintf(ptr, ",\n");
    }
    ptr += sprintf(ptr, "  {\"name\":\"%s\",\"mode\":\"d\"}", share);
  }
  ptr += sprintf(ptr, "\n]\n");

  smb2_free_data(smb2, rep);

  size = ptr-buf;
  if((resp=MHD_create_response_from_buffer(size, buf,
					   MHD_RESPMEM_MUST_FREE))) {
    MHD_add_response_header(resp, "Content-Type", "application/json");
    args->result = websrv_queue_response(args->conn, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
  } else {
    args->result = MHD_NO;
  }
  args->finished = 1;
}


/**
 * Respond to a http request of a remote smb shares listing.
 **/
static enum MHD_Result
smb_request_shares(struct MHD_Connection *conn, const char* user,
                   const char* pass, const char* uri) {
  smb_request_shares_args_t args = {conn, MHD_NO, 0};
  enum MHD_Result ret = MHD_NO;
  struct smb2_context *smb2;
  struct smb2_url *url;
  struct pollfd pfd;

  if(!(smb2=smb2_init_context())) {
    perror("smb2_init_context"); // TODO: output posix error to http response.
    return ret;
  }

  smb2_set_user(smb2, user);
  smb2_set_password(smb2, pass);
  smb2_set_security_mode(smb2, SMB2_NEGOTIATE_SIGNING_ENABLED);

  if(!(url=smb2_parse_url(smb2, uri))) {
    ret = smb_response_error(conn, smb2);
    smb2_destroy_context(smb2);
    return ret;
  }

  if(smb2_connect_share(smb2, url->server, "IPC$", user)) {
    if(pass && *pass == 0) {
      ret = smb_request_shares(conn, user, 0, uri);
    } else {
      ret = smb_response_error(conn, smb2);
    }
    smb2_destroy_url(url);
    smb2_destroy_context(smb2);
    return ret;
  }

  smb2_destroy_url(url);

  if(smb2_share_enum_async(smb2, SHARE_INFO_0,
                           smb_request_shares_cb, &args)) {
    ret = smb_response_error(conn, smb2);
    smb2_destroy_context(smb2);
    return ret;
  }

  while(!args.finished) {
    pfd.fd = smb2_get_fd(smb2);
    pfd.events = smb2_which_events(smb2);

    if(poll(&pfd, 1, 1000) < 0) {
      args.result = MHD_NO;
      perror("poll"); // TODO: output posix error to http response.
      break;
    }
    if(pfd.revents == 0) {
      continue;
    }
    if(smb2_service(smb2, pfd.revents) < 0) {
      args.result = smb_response_error(conn, smb2);
      args.finished = 1;
    }
  }

  smb2_destroy_context(smb2);

  return args.result;
}


/**
 * Respond to a http request of a remote smb resource.
 **/
enum MHD_Result
smb_request(struct MHD_Connection *conn, const char* url) {
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  const char* user;
  const char* pass;
  const char* addr;
  const char* port;
  const char* path;
  char uri[PATH_MAX];

  user = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "user");
  pass = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "pass");
  addr = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "addr");
  port = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "port");
  path = url+4;

  if(!addr) {
    if((resp=MHD_create_response_from_buffer(strlen(PAGE_400), PAGE_400,
                                             MHD_RESPMEM_PERSISTENT))) {
      MHD_add_response_header(resp, "Content-Type", "text/html");
      ret = websrv_queue_response(conn, MHD_HTTP_BAD_REQUEST, resp);
      MHD_destroy_response(resp);
    }
    return ret;
  }
  if(!user) {
    user = "";
  }
  if(!pass) {
    pass = "";
  }
  if(!port) {
    port = "445";
  }

  if(!path[0]) {
    snprintf(uri, PATH_MAX, "smb://%s:%s/%s", addr, port, path);
    return smb_request_shares(conn, user, pass, uri);
  } else {
    snprintf(uri, PATH_MAX, "smb://%s:%s%s", addr, port, path);
    return smb_request_path(conn, user, pass, uri);
  }

  return ret;
}

