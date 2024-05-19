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

#pragma once

#include <unistd.h>

/**
 * Spawn a new process.
 **/
pid_t elfldr_spawn(int stdin_fd, int stdout_fd, int stderr_fd,
		   uint8_t *elf, char* argv[]);


/**
 * Execute an ELF inside the process with the given pid.
 **/
int elfldr_exec(int stdin_fd, int stdout_fd, int stderr_fd,
		pid_t pid, uint8_t* elf);
