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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <microhttpd.h>

#include "asset.h"
#include "fs.h"
#include "mdns.h"
#include "smb.h"
#include "sys.h"
#include "version.h"
#include "websrv.h"


typedef struct post_data {
  char *key;
  uint8_t *val;
  size_t len;
  struct post_data *next;
} post_data_t;


typedef struct post_request {
  struct MHD_PostProcessor* pp;
  post_data_t* data;
} post_request_t;


static post_data_t*
post_data_get(post_data_t* data, const char* key) {
  if(!data) {
    return 0;
  }

  if(!strcmp(key, data->key)) {
    return data;
  }

  return post_data_get(data->next, key);
}


static const char*
post_data_val(post_data_t* data, const char* key) {
  data = post_data_get(data, key);
  return data ? (const char*)data->val : 0;
}


static enum MHD_Result
post_iterator(void *cls, enum MHD_ValueKind kind, const char *key,
               const char *filename, const char *mime, const char *encoding,
               const char *value, uint64_t off, size_t size) {
  post_request_t *req = cls;
  post_data_t *data = post_data_get(req->data, key);

  if(data) {
    data->val = realloc(data->val, off+size+1);
  } else {
    data = malloc(sizeof(post_data_t));
    data->next = req->data;
    data->key = strdup(key);
    data->val = malloc(off+size+1);
    data->len = 0;
    req->data = data;
  }

  memcpy(data->val+off, value, size);
  data->val[off+size] = 0;
  data->len += size;

  return MHD_YES;
}


enum MHD_Result
websrv_queue_response(struct MHD_Connection *conn, unsigned int status,
		      struct MHD_Response *resp) {
  MHD_add_response_header(resp, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN,
  			  "*");

  return MHD_queue_response(conn, status, resp);
}



/**
 * Respond to a version request.
 **/
static enum MHD_Result
version_request(struct MHD_Connection *conn) {
  size_t size = strlen(PAGE_VERSION);
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  void* data = PAGE_VERSION;

  if((resp=MHD_create_response_from_buffer(size, data,
					   MHD_RESPMEM_PERSISTENT))) {
    MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE,
                            "application/json");
    ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
  }

  return ret;
}


/**
 * Respond to a launch request.
 **/
static enum MHD_Result
launch_request(struct MHD_Connection *conn) {
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  const char* title_id;
  unsigned int status;
  const char *args;

  title_id = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "titleId");
  args = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "args");

  if(!title_id) {
    status = MHD_HTTP_BAD_REQUEST;
  } else if(sys_launch_title(title_id, args)) {
    status = MHD_HTTP_SERVICE_UNAVAILABLE;
  } else {
    status = MHD_HTTP_OK;
  }

  if((resp=MHD_create_response_from_buffer(0, "",
					   MHD_RESPMEM_PERSISTENT))) {
    ret = websrv_queue_response(conn, status, resp);
    MHD_destroy_response(resp);
  }

  return ret;
}


/**
 * Respond to a homebrew loading request.
 **/
static enum MHD_Result
hbldr_request(struct MHD_Connection *conn) {
  int (*sys_launch)(const char*, const char*, const char*, const char*) = 0;
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  const char* daemon;
  const char* path;
  const char *args;
  const char *pipe;
  const char *env;
  const char *cwd;
  int fd;

  path = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "path");
  args = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "args");
  env = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "env");
  pipe = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "pipe");
  cwd = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "cwd");
  daemon = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "daemon");

  if(daemon && strcmp(daemon, "0")) {
    sys_launch = sys_launch_daemon;
  } else {
    sys_launch = sys_launch_homebrew;
  }

  if(!path) {
    if((resp=MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT))) {
      ret = websrv_queue_response(conn, MHD_HTTP_BAD_REQUEST, resp);
      MHD_destroy_response(resp);
    }
  } else if((fd=sys_launch(cwd, path, args, env)) < 0) {
    if((resp=MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT))) {
      ret = websrv_queue_response(conn, MHD_HTTP_SERVICE_UNAVAILABLE, resp);
      MHD_destroy_response(resp);
    }
  } else if(pipe && strcmp(pipe, "0")) {
    if((resp=MHD_create_response_from_pipe(fd))) {
      MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "text/x-log; charset=utf-8");
      ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
      MHD_destroy_response(resp);
    }
  } else {
    close(fd);
    if((resp=MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT))) {
      ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
      MHD_destroy_response(resp);
    }
  }

  return ret;
}



/**
 * Respond to a ELF payload loading request.
 **/
static enum MHD_Result
elfldr_request(struct MHD_Connection *conn, post_data_t *data) {
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  post_data_t *elf;
  const char *args;
  const char *pipe;
  const char *env;
  const char *cwd;
  int fd;

  if(!(args=MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "args"))) {
    args = post_data_val(data, "args");
  }
  if(!(env=MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "env"))) {
    env = post_data_val(data, "env");
  }
  if(!(pipe=MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "pipe"))) {
    pipe = post_data_val(data, "pipe");
  }
  if(!(cwd=MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "cwd"))) {
    cwd = post_data_val(data, "cwd");
  }

  if(!(elf=post_data_get(data, "elf"))) {
    if((resp=MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT))) {
      ret = websrv_queue_response(conn, MHD_HTTP_BAD_REQUEST, resp);
      MHD_destroy_response(resp);
    }

  } else if((fd=sys_launch_payload(cwd, elf->val, elf->len, args, env)) < 0) {
    if((resp=MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT))) {
      ret = websrv_queue_response(conn, MHD_HTTP_SERVICE_UNAVAILABLE, resp);
      MHD_destroy_response(resp);
    }

  } else if(pipe && strcmp(pipe, "0")) {
    if((resp=MHD_create_response_from_pipe(fd))) {
      ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
      MHD_destroy_response(resp);
    }
  } else {
    close(fd);
    if((resp=MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT))) {
      ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
      MHD_destroy_response(resp);
    }
  }

  return ret;
}


/**
 *
 **/
static enum MHD_Result
websrv_on_request(void *cls, struct MHD_Connection *conn,
                  const char *url, const char *method,
                  const char *version, const char *upload_data,
                  size_t *upload_data_size, void **con_cls) {
  post_request_t *req = *con_cls;
  enum MHD_Result ret = MHD_NO;

  if(strcmp(method, MHD_HTTP_METHOD_GET) &&
     strcmp(method, MHD_HTTP_METHOD_POST) &&
     strcmp(method, MHD_HTTP_METHOD_HEAD)) {
    return MHD_NO;
  }

  if(!req) {
    req = *con_cls = malloc(sizeof(post_request_t));
    req->pp = MHD_create_post_processor(conn, 0x1000, &post_iterator, req);
    req->data = 0;
    return MHD_YES;
  }

  if(!strcmp(method, MHD_HTTP_METHOD_GET)) {
    if(!strcmp("/fs", url)) {
      return fs_request(conn, url);
    }
    if(!strncmp("/fs/", url, 4)) {
      return fs_request(conn, url);
    }
#ifdef __SCE__
    if(!strcmp("/mdns", url)) {
      return mdns_request(conn, url);
    }
    if(!strncmp("/smb", url, 4)) {
      return smb_request(conn, url);
    }
#endif
    if(!strcmp("/launch", url)) {
      return launch_request(conn);
    }
    if(!strcmp("/hbldr", url)) {
      return hbldr_request(conn);
    }
    if(!strcmp("/elfldr", url)) {
      return asset_request(conn, "/elfldr.html");
    }
    if(!strcmp("/version", url)) {
      return version_request(conn);
    }
    if(!strcmp("/", url) || !url[0]) {
      return asset_request(conn, "/index.html");
    }
    return asset_request(conn, url);
  }

  if(!strcmp(method, MHD_HTTP_METHOD_POST)) {
    if(*upload_data_size) {
      ret = MHD_post_process(req->pp, upload_data, *upload_data_size);
      *upload_data_size = 0;
      return ret;
    }
    if(!strcmp("/elfldr", url)) {
      return elfldr_request(conn, req->data);
    }
  }

  return MHD_NO;
}



static void
websrv_on_completed(void *cls, struct MHD_Connection *connection,
                    void **con_cls, enum MHD_RequestTerminationCode toe) {
  post_request_t *req = *con_cls;
  post_data_t *data;

  if(!req) {
    return;
  }

  while((data=req->data)) {
    req->data = data->next;
    free(data->key);
    free(data->val);
    free(data);
  }

  MHD_destroy_post_processor(req->pp);
  free(req);
}


int
websrv_listen(unsigned short port) {
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  struct MHD_Daemon *httpd;
  socklen_t addr_len;
  int connfd;
  int srvfd;

  signal(SIGPIPE, SIG_IGN);

  if((srvfd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }

  if(setsockopt(srvfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
    perror("setsockopt");
    close(srvfd);
    return -1;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  if(bind(srvfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
    perror("bind");
    close(srvfd);
    return -1;
  }

  if(listen(srvfd, 5) != 0) {
    perror("listen");
    close(srvfd);
    return -1;
  }

  if(!(httpd=MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION | MHD_USE_ITC |
			      MHD_USE_NO_LISTEN_SOCKET | MHD_USE_DEBUG |
			      MHD_USE_INTERNAL_POLLING_THREAD,
			      0, NULL, NULL, &websrv_on_request, NULL,
                              MHD_OPTION_NOTIFY_COMPLETED, &websrv_on_completed,
                              NULL, MHD_OPTION_END))) {
    perror("MHD_start_daemon");
    close(srvfd);
    return -1;
  }

  while(1) {
    addr_len = sizeof(client_addr);
    if((connfd=accept(srvfd, (struct sockaddr*)&client_addr, &addr_len)) < 0) {
      perror("accept");
      break;
    }

    if(MHD_add_connection(httpd, connfd, (struct sockaddr*)&client_addr,
			  addr_len) != MHD_YES) {
      perror("MHD_add_connection");
      break;
    }
  }

  MHD_stop_daemon(httpd);

  return close(srvfd);
}
