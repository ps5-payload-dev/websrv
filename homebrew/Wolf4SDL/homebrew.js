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
    const ENV = {HOME: CWD};

    return {
        mainText: "Wolf4SDL",
        secondaryText: 'Source port for Wolfenstein 3D',
	imgPath: '/fs/' + CWD + '/coverart.jpg',
	onclick: async () => {
	    return {
		path: CWD + '/wolf3d-shareware.elf',
		cwd: CWD,
		env: ENV,
		args: []
	    };
        },
	options: [{
	    text: "Play reail version",
	    onclick: () => {
		return {
		    path: CWD + '/wolf3d.elf',
		    cwd: CWD,
		    env: ENV,
		    args: []
		};
	    }
	}, {
	    text: "Play shareware version",
	    onclick: () => {
		return {
		    path: CWD + '/wolf3d-shareware.elf',
		    cwd: CWD,
		    env: ENV,
		    args: []
		};
	    }
	}]
    };
}
