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
    const ENV = {TMPDIR: '/user/temp'};
    const MENU_ITEMS = [
	{
	    mainText: 'Doom',
	    secondaryText: 'A fast-paced sci-fi shooter',
	    imgPath: '/fs/' + CWD + '/doom.png',
	    onclick: async () => {
		return {
		    path: CWD + '/crispy-doom.elf',
		    args: ['-iwad', CWD + '/DOOM1.WAD'],
		    cwd: CWD,
		    env: ENV
		}
	    },
	    options: [{
		text: "Play",
		onclick: () => {
		    return {
			path: CWD + '/crispy-doom.elf',
			args: ['-iwad', CWD + '/DOOM1.WAD'],
			cwd: CWD,
			env: ENV
		    };
		}
	    },{
		text: "Configure",
		onclick: () => {
		    return {
			path: CWD + '/crispy-doom-setup.elf',
			cwd: CWD,
			env: ENV
		    };
		}
	    }]
	}, {
	    mainText: 'Heretic',
	    secondaryText: 'A dark fantasy shooter',
	    imgPath: '/fs/' + CWD + '/heretic.png',
	    onclick: async () => {
		return {
		    path: CWD + '/crispy-heretic.elf',
		    args: ['-iwad', CWD + '/HERETIC1.WAD'],
		    cwd: CWD,
		    env: ENV
		}
	    },
	    options: [{
		text: "Play",
		onclick: () => {
		    return {
			path: CWD + '/crispy-heretic.elf',
			args: ['-iwad', CWD + '/HERETIC.WAD'],
			cwd: CWD,
			env: ENV
		    };
		}
	    },{
		text: "Configure",
		onclick: () => {
		    return {
			path: CWD + '/crispy-heretic-setup.elf',
			cwd: CWD,
			env: ENV
		    };
		}
	    }]
	}, {
	    mainText: 'Hexen',
	    secondaryText: 'A fantasy action game',
	    imgPath: '/fs/' + CWD + '/hexen.png',
	    onclick: async () => {
		return {
		    path: CWD + '/crispy-hexen.elf',
		    args: ['-iwad', CWD + '/HEXEN.WAD'],
		    cwd: CWD,
		    env: ENV
		}
	    },
	    options: [{
		text: "Play",
		onclick: () => {
		    return {
			path: CWD + '/crispy-hexen.elf',
			args: ['-iwad', CWD + '/HEXEN.WAD'],
			cwd: CWD,
			env: ENV
		    };
		}
	    },{
		text: "Configure",
		onclick: () => {
		    return {
			path: CWD + '/crispy-hexen-setup.elf',
			cwd: CWD,
			env: ENV
		    };
		}
	    }]
	}, {
	    mainText: 'Strife',
	    secondaryText: 'A story-driven shooter',
	    imgPath: '/fs/' + CWD + '/strife.png',
	    onclick: async () => {
		return {
		    path: CWD + '/crispy-strife.elf',
		    args: ['-iwad', CWD + '/STRIFE.WAD'],
		    cwd: CWD,
		    env: ENV
		}
	    },
	    options: [{
		text: "Play",
		onclick: () => {
		    return {
			path: CWD + '/crispy-strife.elf',
			args: ['-iwad', CWD + '/STRIFE.WAD'],
			cwd: CWD,
			env: ENV
		    };
		}
	    },{
		text: "Configure",
		onclick: () => {
		    return {
			path: CWD + '/crispy-strife-setup.elf',
			cwd: CWD,
			env: ENV
		    };
		}
	    }]
	}
    ];

    return {
        mainText: "Crispy Doom",
        secondaryText: 'Source port of Doom engine',
        onclick: async () => {
            showCarousel(MENU_ITEMS);
        }
    };
}
