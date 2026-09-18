/* Copyright (C) 2026 John Törnblom

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

async function main() {
    const WORKDIR = window.workingDir;
    const PAYLOAD = WORKDIR + '/thextech-smbx.elf';
    const ENV = {HOME: window.workingDir,
                 LD_LIBRARY_PATH: window.workingDir};

    return {
        mainText: "Super Mario Bros. X",
        secondaryText: 'Fan-made Mario game engine',
        onclick: async() => {
            return {
                path: PAYLOAD,
                cwd: WORKDIR,
                env: ENV
            };
        },
        options: [{
            text: "Speedrun Mode 1 (Native)",
            onclick: () => {
                return {
                    path: PAYLOAD,
                    cwd: WORKDIR,
                    env: ENV,
                    args: '--speed-run-mode 1'
                };
            }
        }, {
            text: "Speedrun Mode 2 (SMBX2)",
            onclick: () => {
                return {
                    path: PAYLOAD,
                    cwd: WORKDIR,
                    env: ENV,
                    args: '--speed-run-mode 2'
                };
            }
        }, {
            text: "Speedrun Mode 3 (Strict SMBX 1.3)",
            onclick: () => {
                return {
                    path: PAYLOAD,
                    cwd: WORKDIR,
                    env: ENV,
                    args: '--speed-run-mode 3'
                };
            }
        }, {
            text: "SMBX2 compatibiltiy mode",
            onclick: () => {
                return {
                    path: PAYLOAD,
                    cwd: WORKDIR,
                    env: ENV,
                    args: '--compat-level smbx2'
                };
            }
        }, {
            text: "SMBX 1.3 strict compatibiltiy mode",
            onclick: () => {
                return {
                    path: PAYLOAD,
                    cwd: WORKDIR,
                    env: ENV,
                    args: '--compat-level smbx13'
                };
            }
        }]
    };
}
