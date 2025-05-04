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
 * Arguments used by the callback function smb_request_shares_cb().
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
  const char* path;
  int state;
} smb_dir_read_args_t;


/**
 * Arguments used by file callback functions.
 **/
typedef struct smb_file_read_args {
  struct smb2_context *smb2;
  struct smb2fh* file;
} smb_file_read_args_t;


/**
 * Callback function used to transmit smb file data to a http request.
 **/
static ssize_t
smb_request_file_read_cb(void *ctx, uint64_t pos, char *buf, size_t max) {
  smb_file_read_args_t* args = (smb_file_read_args_t*)ctx;
  int ret = smb2_read(args->smb2, args->file, (uint8_t*)buf, max);
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
  smb_file_read_args_t* args = (smb_file_read_args_t*)ctx;
  smb2_close(args->smb2, args->file);
  smb2_disconnect_share(args->smb2);
  smb2_destroy_context(args->smb2);
  free(args);
}


/**
 * Callback function used to transmit smb dir data to a http request.
 **/
static ssize_t
smb_request_dir_read_cb(void *ctx, uint64_t pos, char *buf, size_t max) {
  smb_dir_read_args_t* args = (smb_dir_read_args_t*)ctx;
  struct smb2dirent* ent;
  struct smb2_stat_64 st;
  char path[PATH_MAX];
  char mode = '-';

  if(args->state == 0) {
    args->state++;
    return snprintf(buf, max, "[{\"name\":\".\",\"mode\":\"d\"}");
  }

  if(args->state == 2) {
    args->state++;
    return snprintf(buf, max, "]");
  }

  if(args->state == 3) {
    return MHD_CONTENT_READER_END_OF_STREAM;
  }

  if(!(ent=smb2_readdir(args->smb2, args->dir))) {
    args->state++;
    return 0;
  }

  snprintf(path, PATH_MAX, "%s/%s", args->path, ent->name);
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

  return snprintf(buf, max, ",{\"name\":\"%s\",\"mode\":\"%c\"}",
                  ent->name, mode);
}


/**
 * Callback function used to close an smb dir that has been transmitted
 * via a http request successfully.
 **/
static void
smb_request_dir_close_cb(void *ctx) {
  smb_dir_read_args_t* args = (smb_dir_read_args_t*)ctx;
  smb2_closedir(args->smb2, args->dir);
  smb2_disconnect_share(args->smb2);
  smb2_destroy_context(args->smb2);
}


/**
 * Respond to a http request with an internal error.
 **/
static enum MHD_Result
smb2_response_error(struct MHD_Connection *conn, struct smb2_context *smb2) {
  const char* smb2_error = smb2_get_error(smb2);
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;

  if((resp=MHD_create_response_from_buffer(strlen(smb2_error), (void*)smb2_error,
                                           MHD_RESPMEM_PERSISTENT))) {
    ret = websrv_queue_response(conn, MHD_HTTP_INTERNAL_SERVER_ERROR, resp);
    MHD_destroy_response(resp);
  }
  return ret;
}


/**
 * Respond to a http request of a remote smb file.
 **/
static enum MHD_Result
smb_request_file(struct MHD_Connection *conn, struct smb2_context *smb2,
                const char* path) {
  smb_file_read_args_t* args;
  struct MHD_Response *resp;
  struct smb2_stat_64 st;
  enum MHD_Result ret;

  if(smb2_stat(smb2, path, &st) < 0) {
    return smb2_response_error(conn, smb2);
  }

  if(!(args=malloc(sizeof(smb_file_read_args_t)))) {
    perror("malloc"); // TODO: output posix error to http response.
    return MHD_NO;
  }

  args->smb2 = smb2;
  if(!(args->file=smb2_open(smb2, path, O_RDONLY))) {
    return smb2_response_error(conn, smb2);
  }

  if((resp=MHD_create_response_from_callback(st.smb2_size, 32*0x1000,
					     smb_request_file_read_cb, args,
					     smb_request_file_close_cb))) {
    ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
    return ret;
  }

  smb2_close(smb2, args->file);
  smb2_disconnect_share(smb2);
  smb2_destroy_context(smb2);
  free(args);

  return MHD_NO;
}


/**
 * Resapond to a http request of a remote smb dir.
 **/
static enum MHD_Result
smb_request_dir(struct MHD_Connection *conn, struct smb2_context *smb2,
                const char* path) {
  smb_dir_read_args_t* args;
  struct MHD_Response *resp;
  enum MHD_Result ret;

  if(!(args=malloc(sizeof(smb_dir_read_args_t)))) {
    perror("malloc"); // TODO: output posix error to http response.
    return MHD_NO;
  }

  args->smb2 = smb2;
  args->path = path;
  args->state = 0;
  if(!(args->dir=smb2_opendir(smb2, path))) {
    return smb2_response_error(conn, smb2);
  }

  if((resp=MHD_create_response_from_callback(MHD_SIZE_UNKNOWN, 32*0x1000,
					     smb_request_dir_read_cb, args,
					     smb_request_dir_close_cb))) {
    MHD_add_response_header(resp, "Content-Type", "application/json");
    ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
    return ret;
  }

  smb2_closedir(smb2, args->dir);
  smb2_disconnect_share(smb2);
  smb2_destroy_context(smb2);
  free(args);

  return MHD_NO;
}


/**
 * Respond to a http request of a remote smb path.
 **/
static enum MHD_Result
smb_request_path(struct MHD_Connection *conn, const char* uri) {
  enum MHD_Result ret = MHD_NO;
  struct smb2_context *smb2;
  struct smb2_stat_64 st;
  struct smb2_url *url;

  if(!(smb2=smb2_init_context())) {
    return smb2_response_error(conn, smb2);
  }

  if(!(url=smb2_parse_url(smb2, uri))) {
    return smb2_response_error(conn, smb2);
  }

  smb2_set_security_mode(smb2, SMB2_NEGOTIATE_SIGNING_ENABLED);
  if(smb2_connect_share(smb2, url->server, url->share, 0) < 0) {
    ret = smb2_response_error(conn, smb2);
  } else if(smb2_stat(smb2, url->path, &st) < 0) {
    ret = smb2_response_error(conn, smb2);
  } else {
    switch(st.smb2_type) {
    case SMB2_TYPE_DIRECTORY:
      ret = smb_request_dir(conn, smb2, url->path);
      break;
    case SMB2_TYPE_FILE:
      ret = smb_request_file(conn, smb2, url->path);
      break;
    case SMB2_TYPE_LINK:
    default:
      //TODO: unknown file type
      break;
    }
  }

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
    args->result = smb2_response_error(args->conn, smb2);
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
smb_request_shares(struct MHD_Connection *conn, const char* uri) {
  smb_request_shares_args_t args = {conn, MHD_NO, 0};
  struct smb2_context *smb2;
  struct smb2_url *url;
  struct pollfd pfd;

  if(!(smb2=smb2_init_context())) {
    return smb2_response_error(conn, smb2);
  }

  if(!(url=smb2_parse_url(smb2, uri))) {
    return smb2_response_error(conn, smb2);
  }

  smb2_set_security_mode(smb2, SMB2_NEGOTIATE_SIGNING_ENABLED);
  if(smb2_connect_share(smb2, url->server, "IPC$", 0) < 0) {
    args.result = smb2_response_error(conn, smb2);
  } else if(smb2_share_enum_async(smb2, SHARE_INFO_0,
                                  smb_request_shares_cb, &args)) {
    args.result = smb2_response_error(conn, smb2);
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
      args.result = smb2_response_error(conn, smb2);
      break;
    }
  }

  smb2_destroy_url(url);
  smb2_disconnect_share(smb2);
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
  //const char* username;
  //const char* password;
  const char* addr;
  const char* port;
  const char* path;
  char uri[PATH_MAX];

  // TODO: smb auth
  addr = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "addr");
  port = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "port");
  path = url+4;

  if(!addr || !port) {
    if((resp=MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT))) {
      ret = websrv_queue_response(conn, MHD_HTTP_BAD_REQUEST, resp);
      MHD_destroy_response(resp);
    }
    return ret;
  }

  if(!path[0]) {
    snprintf(uri, PATH_MAX, "smb://%s:%s/%s", addr, port, path);
    return smb_request_shares(conn, uri);
  } else {
    snprintf(uri, PATH_MAX, "smb://%s:%s%s", addr, port, path);
    return smb_request_path(conn, uri);
  }

  return ret;
}

