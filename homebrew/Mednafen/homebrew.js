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
    const PAYLOAD = window.workingDir + '/mednafen.elf';
    const ROMDIR = window.workingDir + '/roms/';
    const ROMTYPES = ['nes', 'smc', 'sfc', 'gba', 'psx', 'cue', 'zip']
    
    async function getRomList() {
        let listing = await ApiClient.fsListDir(ROMDIR);
	return listing.filter(entry =>
	    ROMTYPES.includes(entry.name.slice(-3))).map(entry => {
		const name = entry.name.slice(0, -4);
		return {
		    mainText: name,
		    imgPath: '/fs/' + ROMDIR + name + '.jpg',
		    onclick: async() => {
			return {
			    path: PAYLOAD,
			    args: ROMDIR + entry.name
			}
		    }
		};
	    });
    }
    return {
        mainText: "Mednafen",
        secondaryText: 'Multi-system Emulator',
        onclick: async () => {
	    let items = await getRomList();
            showCarousel(items);
        }
    };
}
