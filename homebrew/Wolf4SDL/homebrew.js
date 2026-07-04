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
    const CWD = window.workingDir;

    return {
        mainText: "Wolf4SDL",
        secondaryText: 'Source port for Wolfenstein 3D',
        onclick: async () => {
            // Assemble payloads path, so it's not cluttered with other stuff confusing the end user.
            const payloads_path = CWD + '/' + 'payloads';
            
            // Select elf.
            let payload_full_path = await pickFile(payloads_path, 'Select ELF...', true);
            // 2 sanity checks.
            if(!payload_full_path) {
                return;
            }

            if(!payload_full_path.startsWith('/')) {
                payload_full_path = ApiClient.getNetworkShareHttpProxyUrl(payload_full_path);
            }

            // Extract the basename of payload at / for detection.
            const payload_basename = payload_full_path.split('/').pop();

            let game_basename = '';

            if (payload_basename === 'wolf3d-registered.elf') {
                game_basename = 'registered'; 
            } else if (payload_basename === 'wolf3d-shareware.elf') {
                game_basename = 'shareware';
            }

            // Assemble full path to be used for saves and configs, in a self contained manner.
            const game_full_path = window.workingDir + '/' + game_basename;

            const ARGS = [ '--configdir', `${game_full_path}`, 
                '--datadir', `${game_full_path}`,
            ];

            return {
            path: payload_full_path,
                    cwd: CWD,
                    args: ARGS,
            };
        },
    };
}
