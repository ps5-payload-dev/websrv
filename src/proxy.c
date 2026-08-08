/* Copyright (C) 2026 John Törnblom

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

#include <stdio.h>
#include <strings.h>

#include "http.h"
#include "proxy.h"
#include "websrv.h"


/**
 * Size of the chunks pulled from the remote server.
 **/
#define PROXY_BLOCK_SIZE 0x4000


/**
 * Headers that must not be forwarded from the remote server to the client.
 *
 * Content-Length is managed by MHD, which chunks the response since the size
 * of the remote body is not known up front. Access-Control-* is added by
 * websrv_queue_response().
 **/
static const char* proxy_skip_headers[] = {
  "Connection",
  "Content-Length",
  "Keep-Alive",
  "Proxy-Authenticate",
  "Proxy-Authorization",
  "TE",
  "Trailer",
  "Transfer-Encoding",
  "Upgrade",
  0
};


/**
 * Headers forwarded from the client to the remote server.
 **/
static const char* proxy_keep_headers[] = {
  "Accept",
  "Accept-Encoding",
  "Accept-Language",
  "If-Modified-Since",
  "If-None-Match",
  "Range",
  0
};


/**
 * Copy a header sent by the remote server to the response of the client.
 **/
static void
proxy_copy_header(const char *name, const char *value, void *ctx) {
  struct MHD_Response *resp = ctx;

  for(int i=0; proxy_skip_headers[i]; i++) {
    if(!strcasecmp(name, proxy_skip_headers[i])) {
      return;
    }
  }

  if(!strncasecmp(name, "Access-Control-", 15)) {
    return;
  }

  MHD_add_response_header(resp, name, value);
}


/**
 * Pull more of the body from the remote server.
 **/
static ssize_t
proxy_reader(void *cls, uint64_t pos, char *buf, size_t max) {
  ssize_t len = http_response_read(cls, buf, max);

  if(len > 0) {
    return len;
  }

  if(len == 0) {
    return MHD_CONTENT_READER_END_OF_STREAM;
  }

  return MHD_CONTENT_READER_END_WITH_ERROR;
}


/**
 * Called by MHD once the client is done with the response.
 **/
static void
proxy_reader_free(void *cls) {
  http_response_free(cls);
}


/**
 * Respond to a proxy request.
 **/
enum MHD_Result
proxy_request(struct MHD_Connection *conn, const char *method) {
  char lines[6][512];
  const char *headers[7];
  enum MHD_Result ret = MHD_NO;
  http_response_t *remote;
  struct MHD_Response *resp;
  unsigned int status;
  const char *value;
  size_t nb_headers = 0;
  const char *url;

  url = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "url");

  if(!url) {
    if((resp=MHD_create_response_from_buffer(0, "",
					     MHD_RESPMEM_PERSISTENT))) {
      ret = websrv_queue_response(conn, MHD_HTTP_BAD_REQUEST, resp);
      MHD_destroy_response(resp);
    }
    return ret;
  }

  for(int i=0; proxy_keep_headers[i] && nb_headers < 6; i++) {
    if(!(value=MHD_lookup_connection_value(conn, MHD_HEADER_KIND,
                                           proxy_keep_headers[i]))) {
      continue;
    }
    snprintf(lines[nb_headers], sizeof(lines[0]), "%s: %s",
             proxy_keep_headers[i], value);
    headers[nb_headers] = lines[nb_headers];
    nb_headers++;
  }
  headers[nb_headers] = 0;

  if(!(remote=http_request(method, url, headers))) {
    if((resp=MHD_create_response_from_buffer(0, "",
					     MHD_RESPMEM_PERSISTENT))) {
      ret = websrv_queue_response(conn, MHD_HTTP_BAD_GATEWAY, resp);
      MHD_destroy_response(resp);
    }
    return ret;
  }

  if((status=(unsigned int)http_response_status(remote)) < 100) {
    status = MHD_HTTP_BAD_GATEWAY;
  }

  // MHD owns the remote response from here on, and releases it with
  // proxy_reader_free() once the client is done with it
  if(!(resp=MHD_create_response_from_callback(MHD_SIZE_UNKNOWN,
					      PROXY_BLOCK_SIZE, proxy_reader,
					      remote, proxy_reader_free))) {
    http_response_free(remote);
    return MHD_NO;
  }

  http_response_foreach_header(remote, proxy_copy_header, resp);

  ret = websrv_queue_response(conn, status, resp);
  MHD_destroy_response(resp);

  return ret;
}

