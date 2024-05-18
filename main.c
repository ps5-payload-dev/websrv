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

#ifdef __PROSPERO__
#include <sys/sysctl.h>
#include <sys/syscall.h>
#include <ps5/klog.h>
#endif

#include <microhttpd.h>

#include "fs.h"


#define PAGE_400                   \
  "<html>"                         \
    "<head>"                       \
      "<title>Bad request</title>" \
    "</head>"                      \
    "<body>Bad request</body>"     \
  "</html>"


static enum MHD_Result
ahc_echo(void *cls, struct MHD_Connection *conn,
	 const char *url, const char *method,
	 const char *version, const char *upload_data,
	 size_t *upload_data_size, void **ptr) {

  struct MHD_Response *resp;
  int ret = MHD_NO;
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

  if(!strncmp("/fs", url, 3)) {
    return fs_on_request(conn, url);
  }

  if((resp=MHD_create_response_from_buffer(strlen(PAGE_400), PAGE_400,
					   MHD_RESPMEM_PERSISTENT))) {
    ret = MHD_queue_response(conn, MHD_HTTP_BAD_REQUEST, resp);
    MHD_destroy_response(resp);
  }

  return ret;
}


#ifdef __PROSPERO__
/**
 * Fint the pid of a process with the given name.
 **/
static pid_t
find_pid(const char* name) {
  int mib[4] = {1, 14, 8, 0};
  pid_t mypid = getpid();
  pid_t pid = -1;
  size_t buf_size;
  uint8_t *buf;

  if(sysctl(mib, 4, 0, &buf_size, 0, 0)) {
    klog_perror("sysctl");
    return -1;
  }

  if(!(buf=malloc(buf_size))) {
    klog_perror("malloc");
    return -1;
  }

  if(sysctl(mib, 4, buf, &buf_size, 0, 0)) {
    klog_perror("sysctl");
    free(buf);
    return -1;
  }

  for(uint8_t *ptr=buf; ptr<(buf+buf_size);) {
    int ki_structsize = *(int*)ptr;
    pid_t ki_pid = *(pid_t*)&ptr[72];
    char *ki_tdname = (char*)&ptr[447];

    ptr += ki_structsize;
    if(!strcmp(name, ki_tdname) && ki_pid != mypid) {
      pid = ki_pid;
    }
  }

  free(buf);

  return pid;
}
#endif


int
main() {
  const uint16_t port = 8080;
  struct MHD_Daemon *d;

#ifdef __PROSPERO__
  pid_t pid;

  syscall(SYS_thr_set_name, -1, "websrv.elf");

  klog_printf("Web server was compiled at %s %s\n", __DATE__, __TIME__);

  while((pid=find_pid("websrv.elf")) > 0) {
    if(kill(pid, SIGKILL)) {
      klog_perror("kill");
      _exit(-1);
    }
    sleep(1);
  }

#endif

  if(!(d=MHD_start_daemon((MHD_USE_THREAD_PER_CONNECTION |
			   MHD_USE_INTERNAL_POLLING_THREAD |
			   MHD_USE_ERROR_LOG), port, 0, 0,
			  &ahc_echo, 0, MHD_OPTION_END))) {
    perror("MHD_start_daemon");
    return 1;
  }

  while(1) {
    sleep(3);
  }

  MHD_stop_daemon(d);

  return 0;
}
