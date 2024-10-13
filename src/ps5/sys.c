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

#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/sysctl.h>
#include <sys/syscall.h>

#include "elfldr.h"
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
 * Decode an escaped argument.
 **/
static char*
args_decode(const char* s) {
  size_t length = strlen(s);
  char *arg = malloc(length+1);
  size_t off = 0;
  int escape = 0;

  for(size_t i=0; i<length; i++) {
    if(s[i] == '\\' && !escape) {
      escape = 1;
    } else {
      arg[off++] = s[i];
      escape = 0;
    }
  }

  arg[off] = 0;
  return arg;
}


static int
args_split(const char* args, char** argv, size_t size) {
  char* buf = strdup(args);
  size_t len = strlen(buf);
  int escape = 0;
  int argc = 0;

  memset(argv, 0, size*sizeof(char*));
  for(int i=0; i<len && argc<size; i++) {
    if(escape) {
      escape = 0;
      continue;
    }

    if(buf[i] == '\\') {
      escape = 1;
      continue;
    }

    if(buf[i] == ' ') {
      buf[i] = 0;
      continue;
    }

    if(buf[i] && !i) {
      argv[argc++] = buf+i;
      continue;
    }

    if(buf[i] && !buf[i-1]) {
      argv[argc++] = buf+i;
    }
  }

  for(int i=0; i<argc; i++) {
    argv[i] = args_decode(argv[i]);
  }

  free(buf);

  return argc;
}


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
sys_launch_homebrew(const char* cwd, const char* path, const char* args,
		    const char* env) {
  char* argv[255];
  char* envp[255];
  int optval = 1;
  int fds[2];
  pid_t pid;

  if(!cwd) {
    cwd = "/";
  }

  if(!args) {
    args = "";
  }

  if(!env) {
    env = "";
  }

  printf("launch homebrew: CWD=%s %s %s %s\n", cwd, env, path, args);

  if(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
    perror("socketpair");
    return 1;
  }

  if(setsockopt(fds[1], SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval)) < 0) {
    perror("setsockopt");
    close(fds[0]);
    close(fds[1]);
    return -11;
  }

  args_split(args, argv, 255);
  args_split(env, envp, 255);
  pid = hbldr_launch(cwd, path, fds[1], argv, envp);

  for(int i=0; argv[i]; i++) {
    free(argv[i]);
  }
  for(int i=0; envp[i]; i++) {
    free(envp[i]);
  }

  close(fds[1]);
  if(pid < 0) {
    close(fds[0]);
    return -1;
  }

  return fds[0];
}


int
sys_launch_payload(const char* cwd, uint8_t* elf, size_t elf_size,
                   const char* args, const char* env) {
  char* argv[255];
  char* envp[255];

  int fds[2];
  pid_t pid;

  if(!cwd) {
    cwd = "/";
  }

  if(!args) {
    args = "";
  }

  if(!env) {
    env = "";
  }

  printf("launch payload: CWD=%s %s %s\n", cwd, env, args);

  if(pipe(fds) == -1) {
    perror("pipe");
    return 1;
  }

  args_split(args, argv, 255);
  args_split(env, envp, 255);
  pid = elfldr_spawn(cwd, fds[1], elf, argv, envp);

  for(int i=0; argv[i]; i++) {
    free(argv[i]);
  }
  for(int i=0; envp[i]; i++) {
    free(envp[i]);
  }

  close(fds[1]);
  if(pid < 0) {
    close(fds[0]);
    return -1;
  }

  return fds[0];
}

int
sys_launch_title(const char* title_id, const char* args) {
  app_launch_ctx_t ctx = {0};
  char* argv[255];
  int argc = 0;
  int app_id;
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

  argc = args_split(args, argv, 255);
  if((err=sceSystemServiceLaunchApp(title_id, argv, &ctx)) < 0) {
    perror("sceSystemServiceLaunchApp");
  }

  for(int i=0; i<argc; i++) {
    free(argv[i]);
  }

  return err;
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
