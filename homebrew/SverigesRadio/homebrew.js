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
    const BASE_STREAM_URL = 'http://http-live.sr.se/'
    const BASE_LOGO_URL = '/fs/' + window.workingDir + '/logos/';
    const PAYLOAD_PATH = window.workingDir + '/ffplay.elf';

    async function getChannelList() {
	return [
	    {
		mainText: '',
		imgPath: BASE_LOGO_URL + 'P1.png',
		onclick: async() => {
		    return {
			path: PAYLOAD_PATH,
			args: BASE_STREAM_URL + 'p1-aac-192'
		    };
		}
	    },
	    {
		mainText: '',
		imgPath: BASE_LOGO_URL + 'P2.png',
		onclick: async() => {
		    return {
			path: PAYLOAD_PATH,
			args: BASE_STREAM_URL + 'p2musik-aac-320'
		    };
		}
	    },
	    {
		mainText: '',
		imgPath: '/fs/' + window.workingDir + '/logos/P3.png',
		onclick: async() => {
		    return {
			path: PAYLOAD_PATH,
			args: BASE_STREAM_URL + 'p3-aac-192'
		    };
		}
	    },
	    {
		mainText: '',
		imgPath: '/fs/' + window.workingDir + '/logos/P4.png',
		onclick: async() => {
		    return {
			path: PAYLOAD_PATH,
			args: BASE_STREAM_URL + 'p4ostergotland-aac-192'
		    };
		}
	    }
	];
    }

    return {
        mainText: "Sveriges Radio",
        secondaryText: 'Live Radio from Swedish public service',
        onclick: async () => {
	    let items = await getChannelList();
            showCarousel(items);
        }
    };
}
