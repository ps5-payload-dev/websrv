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


async function main() {
    const CWD = window.workingDir;

    return {
        mainText: "Crispy Doom",
        secondaryText: 'Source port for Doom engine games',
        onclick: async () => {
            // Assemble payloads path, so it's not cluttered with other stuff confusing the end user.
            const payloads_path = CWD + '/' + 'payloads';
            
            // Select elf, either game elf or setup elf for game. Let because this can be changed technically by sanity check for network filepaths.
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

            if (payload_basename === 'crispy-doom.elf' || payload_basename === 'crispy-doom-setup.elf') {
                game_basename = 'doom'; 
            } else if (payload_basename === 'crispy-heretic.elf' || payload_basename === 'crispy-heretic-setup.elf') {
                game_basename = 'heretic'; 
            } else if (payload_basename === 'crispy-hexen.elf' || payload_basename === 'crispy-hexen-setup.elf') {
                game_basename = 'hexen'; 
            } else if (payload_basename === 'crispy-strife.elf' || payload_basename === 'crispy-strife-setup.elf') {
                game_basename = 'strife'; 
            }

            // Assemble full path to be used for saves, configs, WADs, in a self contained manner.
            const game_full_path = window.workingDir + '/' + game_basename;

            // Look in game specific dir for WAD selection. Using let for same reason that payload_full_path is usng let.
            let wad = await pickFile(game_full_path, 'Select WAD...', true);
            // Sanity checks.
            if(!wad) {
                return;
            }

            if(!wad.startsWith('/')) {
                wad = ApiClient.getNetworkShareHttpProxyUrl(wad);
            }

            // OK, good time to assemble ARGS and TMPDIR here now that we know everything neccesary.
            const ARGS = [ '-config', `${game_full_path}/${game_basename}.cfg`, 
                '-extraconfig', `${game_full_path}/crispy-${game_basename}.cfg`,
                '-savedir', game_full_path, 
                '-iwad', wad ];
            /*
            By default it expects /tmp to exist for it to extract then play MIDI. It also supports specifying other audio sources like FLAC but we need this working by default, so we change this to our own tmp inside the game folder makes the most sense to me.
            
            Lucky for us, Chocolate Doom introduced custom TMPDIR envar in v3.1.1, and that was already the base for sometime before Crispy Doom 7.1 was released. See below:
            
            * https://github.com/fabiangreffrath/crispy-doom/commit/410d96855b5df5410ff591a90efeafa889119224 - commit moving base up to v3.1.1 of Chocolate Doom.
            
            * https://github.com/fabiangreffrath/crispy-doom/commits/crispy-doom-7.1/ - commits up v7.1 release tag.
            
            */

            const ENV = { TMPDIR: game_full_path + '/tmp' };
            // Set CWD to game_full_path to make autoload populate there.
            return {
            path: payload_full_path,
                    cwd: game_full_path,
                    args: ARGS,
                    env: ENV
            };
        },
    };
}
