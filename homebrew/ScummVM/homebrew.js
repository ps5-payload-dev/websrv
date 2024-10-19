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
    const PAYLOAD = window.workingDir + '/scummvm.elf';
    const CWD = window.workingDir + '/data';
    const ARGS = ['--fullscreen',
                  '--config=/data/homebrew/ScummVM/scummvm.ini',
                  '--screenshotpath=/data/homebrew/ScummVM/screenshots',
                  '--savepath=/data/homebrew/ScummVM/saves',
                  '--themepath=/data/homebrew/ScummVM/themes',
                  '--iconspath=/data/homebrew/ScummVM/icons',
                  '--extrapath=/data/homebrew/ScummVM/extras'];
    return {
        mainText: "ScummVM",
        secondaryText: 'Script Creation Utility for Maniac Mansion VM',
	onclick: async () => {
	    return {
		path: PAYLOAD,
                cwd: CWD,
                args: ARGS
	    };
        }
    };
}
