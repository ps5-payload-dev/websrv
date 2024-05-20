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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/sysctl.h>
#include <sys/syscall.h>

#include "hbldr.h"
#include "pt.h"
#include "sys.h"
#include "websrv.h"


typedef struct app_launch_ctx {
  uint32_t structsize;
  uint32_t user_id;
  uint32_t app_opt;
  uint64_t crash_report;
  uint32_t check_flag;
} app_launch_ctx_t;


int  sceUserServiceInitialize(void*);
int  sceUserServiceGetForegroundUser(uint32_t *user_id);
void sceUserServiceTerminate(void);

int sceSystemServiceGetAppIdOfRunningBigApp(void);
int sceSystemServiceKillApp(int app_id, int how, int reason, int core_dump);
int sceSystemServiceLaunchApp(const char* title_id, char** argv,
			      app_launch_ctx_t* ctx);


/**
 * Fint the pid of a process with the given name.
 **/
static pid_t
ps5_find_pid(const char* name) {
  int mib[4] = {1, 14, 8, 0};
  pid_t mypid = getpid();
  pid_t pid = -1;
  size_t buf_size;
  uint8_t *buf;

  if(sysctl(mib, 4, 0, &buf_size, 0, 0)) {
    perror("sysctl");
    return -1;
  }

  if(!(buf=malloc(buf_size))) {
    perror("malloc");
    return -1;
  }

  if(sysctl(mib, 4, buf, &buf_size, 0, 0)) {
    perror("sysctl");
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


int
sys_launch_homebrew(const char* path, const char* args) {
  char* argv[255];
  int fds[2];
  char* buf;
  pid_t pid;

  if(!args) {
    args = "";
  }

  printf("launch homebrew: %s %s\n", path, args);

  if(pipe(fds)) {
    perror("pipe");
    return -1;
  }

  buf = strdup(args);
  websrv_split_args(buf, argv, 255);

  if((pid=hbldr_launch(path, argv, fds[1])) < 0) {
    free(buf);
    close(fds[0]);
    close(fds[1]);
    return -1;
  }

  free(buf);
  close(fds[1]);

  return fds[0];
}


int
sys_launch_title(const char* title_id, const char* args) {
  app_launch_ctx_t ctx = {0};
  char* argv[255];
  int app_id;
  char* buf;
  int err;

  if(!args) {
    args = "";
  }

  printf("launch title: %s %s\n", title_id, args);

  if((err=sceUserServiceGetForegroundUser(&ctx.user_id))) {
    perror("sceUserServiceGetForegroundUser");
    return err;
  }

  if((app_id=sceSystemServiceGetAppIdOfRunningBigApp()) > 0) {
    if((err=sceSystemServiceKillApp(app_id, -1, 0, 0))) {
      perror("sceSystemServiceKillApp");
      return err;
    }
  }

  buf = strdup(args);
  websrv_split_args(buf, argv, 255);
  if((err=sceSystemServiceLaunchApp(title_id, argv, &ctx)) < 0) {
    perror("sceSystemServiceLaunchApp");
    free(buf);
    return err;
  }

  free(buf);

  return 0;
}




__attribute__((constructor)) static void
ps5_init(void) {
  pid_t pid;
  int err;

  if((err=sceUserServiceInitialize(0))) {
    perror("sceUserServiceInitialize");
    exit(err);
  }

  syscall(SYS_thr_set_name, -1, "websrv.elf");

  while((pid=ps5_find_pid("websrv.elf")) > 0) {
    if((err=kill(pid, SIGKILL))) {
      perror("kill");
      exit(err);
    }
    sleep(1);
  }
}


__attribute__((destructor)) static void
ps5_fini(void) {
  sceUserServiceTerminate();
}
