/* Copyright (C) 2025 Sunny Qeen
   Copyright (C) 2024 John Törnblom

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

#include "hwmonitor.h"
#include "mime.h"
#include "websrv.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_BUFFER_SIZE_MAX 4096
#define SOC_SENSOR_MAX 64

int sys_get_cpu_temperature(int* temperature);
int sys_get_soc_temperature(int index, int* temperature);

enum MHD_Result hwmonitor_request(struct MHD_Connection *conn)
{
  const char* mime = "text/html";
  const char* fmt;
  int size = 0;
  bool json = false;
  int cpu_temperature = 0, soc_temperature[SOC_SENSOR_MAX] = {0}, soc_index;
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  char data[DATA_BUFFER_SIZE_MAX];
  const char* html_header =
        "<!DOCTYPE html>\r\n"
        "<html>\r\n"
        "<meta http-equiv=\"refresh\" content=\"2\">\r\n"
        "<head>\r\n"
        "<title>HWMonitor</title>\r\n"
        "</head>\r\n"
        "<body>\r\n"
        "<h1>HWMonitor</h1>\r\n";

  const char* html_footer =
        "</body>\r\n"
        "</html>\r\n";

  fmt = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "fmt");
  if(fmt && !strcmp(fmt, "json")) {
    mime = "application/json";
    json = true;
  }

  sys_get_cpu_temperature(&cpu_temperature);
  for(soc_index = 0; soc_index < SOC_SENSOR_MAX; soc_index++) {
    if(sys_get_soc_temperature(soc_index, soc_temperature + soc_index)) {
      break;
    }
  }

  if(json) {
    int n = snprintf(data, DATA_BUFFER_SIZE_MAX, "{");
    size += n;

    if(cpu_temperature > 0) {
      n = snprintf(data + size, DATA_BUFFER_SIZE_MAX - size, "\"cpu_temperature\":%d,", cpu_temperature);
      size += n;
    }

    if(soc_index > 0) {
      n = snprintf(data + size, DATA_BUFFER_SIZE_MAX - size, "\"soc_temperature\":[");
      size += n;
      for(int i = 0; i < soc_index; i++) {
        n = snprintf(data + size, DATA_BUFFER_SIZE_MAX - size, "%d,", soc_temperature[i]);
        size += n;
      }

      size--;
      n = snprintf(data + size, DATA_BUFFER_SIZE_MAX - size, "],");
      size += n;
    }

    if(size > 1) {
      size--;
    }
    n = snprintf(data + size, DATA_BUFFER_SIZE_MAX - size, "}");
    size += n;
  } else {
    int n = snprintf(data, DATA_BUFFER_SIZE_MAX, "%s", html_header);
    size += n;

    if(cpu_temperature > 0) {
      n = snprintf(data + size, DATA_BUFFER_SIZE_MAX - size, "<p>CPU Temperature: %d&deg;C %d&deg;F</p>\r\n", cpu_temperature, cpu_temperature * 18 / 10 + 32);
      size += n;
    }

    if(soc_index > 0) {
      n = snprintf(data + size, DATA_BUFFER_SIZE_MAX - size, "<p>SOC Temperature:</p>\r\n");
      size += n;
      for(int i = 0; i < soc_index; i++) {
        n = snprintf(data + size, DATA_BUFFER_SIZE_MAX - size, "<p>&emsp;[S%02d]: %d&deg;C %d&deg;F</p>\r\n", i + 1, soc_temperature[i], soc_temperature[i] * 18 / 10 + 32);
        size += n;
      }
    }

    n = snprintf(data + size, DATA_BUFFER_SIZE_MAX - size, "%s", html_footer);
    size += n;
  }

  if((resp=MHD_create_response_from_buffer(size, data,
                       MHD_RESPMEM_PERSISTENT))) {
    MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE,
                            mime);
    ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
  }

  return ret;
}