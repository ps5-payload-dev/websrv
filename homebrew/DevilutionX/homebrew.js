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
    const PAYLOAD = window.workingDir + '/devilutionx.elf';

    return {
        mainText: "DevilutionX",
        secondaryText: 'Diablo build for modern OSes',
        onclick: async () => {
	    return {
		path: PAYLOAD,
		args: ''
	    };
        },
	options: [
	    {
		text: "Force Shareware mode",
		onclick: async () => {
		    return {
			path: PAYLOAD,
			args: '--spawn'
		    };
		}
            },
	    {
		text: "Force Diablo mode",
		onclick: async () => {
		    return {
			path: PAYLOAD,
			args: '--diablo'
		    };
		}
	    },
	    {
		text: "Force Hellfire mode",
		onclick: async () => {
		    return {
			path: PAYLOAD,
			args: '--hellfire'
		    };
		}
	    }   
	]
    };
}
