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
    const BASE_API_URL = "https://api.svt.se/video/";
    const BASE_LOGO_URL = '/fs/' + window.workingDir + '/logos/';
    const PAYLOAD_PATH = window.workingDir + '/ffplay.elf';

    async function resolveStreamURL(channelId) {
	let response = await fetch(BASE_API_URL + channelId);
        if (!response.ok) {
	    // throw Exception();
            return '';
        }
        let data = await response.json();
	for (const vidref of data.videoReferences) {
	    if(vidref.format == 'hls') {
		return ['-fs', vidref.redirect];
	    }
	}
	// throw Exception();
	return '';
    }

    async function getChannelList() {
	return [
	    {
		mainText: '',
		imgPath: BASE_LOGO_URL + 'SVT1.png',
		onclick: async() => {
		    return {
			path: PAYLOAD_PATH,
			args: await resolveStreamURL('ch-svt1')
		    };
		}
	    },
	    {
		mainText: '',
		imgPath: BASE_LOGO_URL + 'SVT2.png',
		onclick: async() => {
		    return {
			path: PAYLOAD_PATH,
			args: await resolveStreamURL('ch-svt2')
		    };
		}
	    },
	    {
		mainText: '',
		imgPath: BASE_LOGO_URL + 'SVT24.png',
		onclick: async() => {
		    return {
			path: PAYLOAD_PATH,
			args: await resolveStreamURL('ch-svt24')
		    };
		}
	    },
	    {
		mainText: '',
		imgPath: BASE_LOGO_URL + 'Kunskapskanalen.png',
		onclick: async() => {
		    return {
			path: PAYLOAD_PATH,
			args: await resolveStreamURL('ch-kunskapskanalen')
		    };
		}
	    },
	    {
		mainText: '',
		imgPath: BASE_LOGO_URL + 'Barnkanalen.png',
		onclick: async() => {
		    return {
			path: PAYLOAD_PATH,
			args: await resolveStreamURL('ch-barnkanalen')
		    };
		}
	    }
	];
    }

    return {
        mainText: "SVT Play",
        secondaryText: 'Live TV from Swedish public service',
        onclick: async () => {
	    let items = await getChannelList();
            showCarousel(items);
        }
    };
}
