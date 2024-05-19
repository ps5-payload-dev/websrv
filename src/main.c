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

#include <microhttpd.h>

#include "asset.h"
#include "fs.h"
#include "sys.h"


/**
 * Respond to a launch request.
 **/
static enum MHD_Result
launch_request(struct MHD_Connection *conn, const char* url) {
  struct MHD_Response *resp;
  const char* title_id;
  char *args[] = {0};
  int ret = MHD_NO;
  const char* page;
  int status;

  title_id = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "titleId");
  if(!title_id) {
    status = MHD_HTTP_BAD_REQUEST;
    page = "";

  } else if(sys_launch_title(title_id, args)) {
    status = MHD_HTTP_SERVICE_UNAVAILABLE;
    page = "";
  } else {
    status = MHD_HTTP_OK;
    page = "";
  }

  if((resp=MHD_create_response_from_buffer(strlen(page), (void*)page,
					   MHD_RESPMEM_PERSISTENT))) {
    ret = MHD_queue_response(conn, status, resp);
    MHD_destroy_response(resp);
  }

  return ret;
}


/**
 *
 **/
static enum MHD_Result
ahc_echo(void *cls, struct MHD_Connection *conn,
	 const char *url, const char *method,
	 const char *version, const char *upload_data,
	 size_t *upload_data_size, void **ptr) {
  static int aptr;

  if(strcmp(method, MHD_HTTP_METHOD_GET)) {
    return MHD_NO;
  }

  // never respond on first call
  if(&aptr != *ptr) {
    *ptr = &aptr;
    return MHD_YES;
  }

  // reset when done
  *ptr = NULL;

  if(!strcmp("/fs", url)) {
    return fs_request(conn, url);
  }
  if(!strncmp("/fs/", url, 4)) {
    return fs_request(conn, url);
  }

  if(!strcmp("/launch", url)) {
    return launch_request(conn, url);
  }

  if(!strcmp("/", url) || !url[0]) {
    return asset_request(conn, "/index.html");
  }

  return asset_request(conn, url);
}


int
main() {
  const uint16_t port = 8080;
  struct MHD_Daemon *d;

  printf("Web server was compiled at %s %s\n", __DATE__, __TIME__);

  if(!(d=MHD_start_daemon((MHD_USE_THREAD_PER_CONNECTION |
			   MHD_USE_INTERNAL_POLLING_THREAD |
			   MHD_USE_ERROR_LOG), port, 0, 0,
			  &ahc_echo, 0, MHD_OPTION_END))) {
    perror("MHD_start_daemon");
    return 1;
  }

  while(1) {
    sleep(1);
  }

  MHD_stop_daemon(d);

  return 0;
}

