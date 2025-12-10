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

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/stat.h>


int
sys_launch_title(const char* title_id, char* args) {
  printf("launch title: %s %s\n", title_id, args);
  return -1;
}


int
sys_launch_homebrew(const char* cwd, const char* path, const char* args,
		    const char* env) {
  char cmd[255];
  FILE *pf;

  if(!args) {
    args = "";
  }

  snprintf(cmd, sizeof(cmd), "%s %s", path, args);
  printf("launch homebrew: %s\n", cmd);

  if(!(pf=popen(cmd,"r"))) {
    perror("popen");
    return -1;
  }

  return fileno(pf);
}

int
sys_launch_daemon(const char* cwd, const char* path, const char* args,
		  const char* env) {
  char cmd[255];
  FILE *pf;

  if(!args) {
    args = "";
  }

  snprintf(cmd, sizeof(cmd), "%s %s", path, args);
  printf("launch daemon: %s\n", cmd);

  if(!(pf=popen(cmd,"r"))) {
    perror("popen");
    return -1;
  }

  return fileno(pf);
}


int
sys_launch_payload(const char* cwd, uint8_t* elf, size_t elf_size,
                   const char* args, const char* env) {
  char filename[] = "/tmp/elfXXXXXX";
  mktemp(filename);

  FILE *f = fopen(filename, "wx");
  fwrite(elf, elf_size, 1, f);
  fclose(f);

  chmod(filename, 0777);

  return sys_launch_homebrew(cwd, filename, args, env);
}

int
sys_get_cpu_temperature(int* temperature) {
  printf("get cpu temperature");
  return -1;
}

int
sys_get_soc_temperature(int index, int* temperature) {
  printf("get soc[%02d] temperature", index);
  return -1;
}
