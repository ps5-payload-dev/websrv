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
    const PAYLOAD = window.workingDir + '/ffplay.elf';
    const MEDIADIR = window.workingDir + '/media/';

    return {
        mainText: "FFplay",
        secondaryText: 'FFmpeg Media Player',
        onclick: async () => {
	    const file = await pickFile(MEDIADIR);
	    if(!file) {
		return;
	    }
	    return {
		path: PAYLOAD,
		args: ['-fs', file]
	    };
        },
	options: [
	    {
		text: "Select video + subtitles",
		onclick: async () => {
		    const file = await pickFile(MEDIADIR, 'Select video...');
		    if(!file) {
			return;
		    }

		    const folder = file.substring(0, file.lastIndexOf('/'));
		    const subs = await pickFile(folder, 'Select subtitles...');
		    if(!subs) {
			return {
			    path: PAYLOAD,
			    args: ['-fs', file]
			};
		    }
		    return {
			path: PAYLOAD,
			args: ['-fs', '-vf', 'subtitles=' + subs, file]
		    };
		}
            }
	]
    };
}
