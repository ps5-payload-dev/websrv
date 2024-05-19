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

#include "websrv.h"


enum MHD_Result
websrv_queue_response(struct MHD_Connection *conn, unsigned int status,
		      struct MHD_Response *resp) {

  MHD_add_response_header(resp, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN,
			  "*");
  return MHD_queue_response(conn, status, resp);
}


void
websrv_split_args(char* args, char** argv, size_t size) {
  size_t len = strlen(args);

  for(int i=0, j=0; i<len && j<size; i++) {
    if(args[i] == ' ') {
      args[i] = 0;
      continue;
    }

    if(args[i] && !i) {
      argv[j++] = args+i;
      continue;
    }

    if(args[i] && !args[i-1]) {
      argv[j++] = args+i;
    }
  }
}
