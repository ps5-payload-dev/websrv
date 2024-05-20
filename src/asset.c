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

#include <stdlib.h>
#include <string.h>

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


typedef struct asset {
  const char   *path;
  void         *data;
  size_t        size;
  struct asset *next;
} asset_t;


static asset_t* g_asset_head = 0;


void
asset_register(const char* path, void* data, size_t size) {
  asset_t* a = calloc(1, sizeof(asset_t));

  a->path = path;
  a->data = data;
  a->size = size;
  a->next = g_asset_head;

  g_asset_head = a;
}


enum MHD_Result
asset_request(struct MHD_Connection *conn, const char* url) {
  int status = MHD_HTTP_NOT_FOUND;
  size_t size = strlen(PAGE_404);
  struct MHD_Response *resp;
  void* data = PAGE_404;
  int ret = MHD_NO;

  for(asset_t* a=g_asset_head; a!=0; a=a->next) {
    if(!strcmp(url, a->path)) {
      data = a->data;
      size = a->size;
      status = MHD_HTTP_OK;
      break;
    }
  }

  if((resp=MHD_create_response_from_buffer(size, data,
					   MHD_RESPMEM_PERSISTENT))) {
    ret = websrv_queue_response(conn, status, resp);
    MHD_destroy_response(resp);
  }

  return ret;
}
