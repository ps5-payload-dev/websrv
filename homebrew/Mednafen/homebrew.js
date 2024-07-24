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
    const ENVVARS = {MEDNAFEN_HOME: window.workingDir};

    const ROMDIR = window.workingDir + '/roms/';
    const ROMTYPES = ['cue', 'dsk', 'gb', 'gba', 'gbc', 'gen', 'gg', 'lnx',
		      'm3u', 'nes', 'ngp', 'pce', 'psx', 'sfc', 'smc', 'sms',
		      'vb', 'ws', 'wsc', 'zip'];

    function getRomType(filename) {
	const dotIndex = filename.lastIndexOf('.');
	if (dotIndex === -1) {
	    return '';
	}
	return filename.slice(dotIndex + 1);
    }

    function getRomName(filename) {
	const dotIndex = filename.lastIndexOf('.');
	if (dotIndex === -1) {
	    return filename;
	}

	return filename.slice(0, dotIndex);
    }

    function getPlatformName(romType) {
	switch(romType) {
        case 'dsk': return 'Apple ][';
        case 'lnx': return 'Atari Lynx';
        case 'gb': return 'Game Boy';
        case 'gba': return 'Game Boy Advance';
        case 'gbc': return 'Game Boy Color';
        case 'gen': return 'Sega MegaDrive';
        case 'gg': return 'Sega Game Gear';
        case 'nes': return 'Nintendo Entertainment System';
        case 'ngp': return 'Neo Geo Pocket';
        case 'pce': return 'PC Engine';
        case 'psx': return 'Sony Playstation';
        case 'sfc':
        case 'smc': return 'Super Nintendo';
        case 'sms': return 'Sega Master System';
        case 'vb': return 'Virtual Boy';
        case 'ws': return 'WonderSwan';
        case 'wsc': return 'WonderSwan Color';
        default: return '';
	}
    }

    async function checkApiVersion() {
	try {
	    const ver = await ApiClient.getVersion();
	    console.log(ver);
	    if (ver.api > 0) {
		return true;
	    }
	} catch(error) {
	    console.error(error);
	}

	alert('Incompatible web server');
	return false;
    }

    async function getRomList() {
        let listing = await ApiClient.fsListDir(ROMDIR);
	return listing.filter(entry =>
	    ROMTYPES.includes(getRomType(entry.name))).map(entry => {
		const romType = getRomType(entry.name);
		const romName = getRomName(entry.name);
		const platformName = getPlatformName(romType);

		return {
		    mainText: romName,
		    secondaryText: platformName,
		    imgPath: '/fs/' + ROMDIR + romName + '.jpg',
		    onclick: async() => {
			return {
			    path: PAYLOAD,
			    args: ROMDIR + entry.name,
			    env: ENVVARS
			}
		    }
		};
	    });
    }

    return {
        mainText: "Mednafen",
        secondaryText: 'Multi-system Emulator',
        onclick: async () => {
	    if(await checkApiVersion()) {
		let items = await getRomList();
		showCarousel(items);
	    }
        },
	options: [
	    {
		text: "Browse ROM...",
		onclick: async () => {
		    if(await checkApiVersion()) {
			const file = await pickFile(window.workingDir);
			if(!file) {
			    return;
			}
			return {
			    path: PAYLOAD,
			    args: file,
			    env: ENVVARS
			};
		    }
		}
	    }
	]
    };
}

