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
    const WORKDIR = window.workingDir;
    const PAYLOAD = WORKDIR + '/eduke32.elf';

    return {
        mainText: "EDuke32",
        secondaryText: 'An advanced Duke Nukem 3D source port',
	onclick: async () => {
	    return {
		path: PAYLOAD,
                cwd: WORKDIR
	    };
        }
    };
}
