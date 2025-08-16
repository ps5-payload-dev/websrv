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
#include <signal.h>

#include "mdns.h"
#include "websrv.h"


int
main(int argc, char** argv) {
  const uint16_t port = 8080;

  puts(".-------------------------------------------------------------------.");
  puts("|                   _                                       _    __ |");
  puts("|__      __   ___  | |__    ___   _ __  __   __       ___  | |  / _||");
  puts("|\\ \\ /\\ / /  / _ \\ | '_ \\  / __| | '__| \\ \\ / /      / _ \\ | | | |_ |");
  puts("| \\ V  V /  |  __/ | |_) | \\__ \\ | |     \\ V /   _  |  __/ | | |  _||");
  puts("|  \\_/\\_/    \\___| |_.__/  |___/ |_|      \\_/   (_)  \\___| |_| |_|  |");
  puts("|                                                                   |");
  printf("| %-30s   Copyright (C) 2025 John Törnblom |\n", VERSION_TAG);
  puts("'-------------------------------------------------------------------'");

  signal(SIGPIPE, SIG_IGN);
  signal(SIGCHLD, SIG_IGN);

  while(1) {
#ifdef __SCE__
    mdns_discovery_start();
#endif
    websrv_listen(port);
    sleep(3);
  }

  return 0;
}

