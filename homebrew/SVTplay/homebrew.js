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
    const API_URL = "https://api.svt.se/video/";

    async function launchChannel(channelId) {
	let response = await fetch(API_URL + channelId);
        if (!response.ok) {
            return false;
        }
        let data = await response.json();
	for (const vidref of data.videoReferences) {
	    if(vidref.format == 'hls') {
		await ApiClient.launchApp(PAYLOAD, vidref.redirect);
		return true;
	    }
	}
	return false;
    }

    async function getChannelList() {
	return [
	    {
		mainText: '',
		imgPath: '/fs/' + window.workingDir + '/logos/SVT1.png',
		onclick: async() => {
		    let res = await launchChannel('ch-svt1');
		    history.back();
		    return res;
		}
	    },
	    {
		mainText: '',
		imgPath: '/fs/' + window.workingDir + '/logos/SVT2.png',
		onclick: async() => {
		    let res = await launchChannel('ch-svt2');
		    history.back();
		    return res;
		}
	    },
	    {
		mainText: '',
		imgPath: '/fs/' + window.workingDir + '/logos/SVT24.png',
		onclick: async() => {
		    let res = await launchChannel('ch-svt24');
		    history.back();
		    return res;
		}
	    },
	    {
		mainText: '',
		imgPath: '/fs/' + window.workingDir + '/logos/Kunskapskanalen.png',
		onclick: async() => {
		    let res = await launchChannel('ch-kunskapskanalen');
		    history.back();
		    return res;
		}
	    },
	    {
		mainText: '',
		imgPath: '/fs/' + window.workingDir + '/logos/Barnkanalen.png',
		onclick: async() => {
		    let res = await launchChannel('ch-barnkanalen');
		    history.back();
		    return res;
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
