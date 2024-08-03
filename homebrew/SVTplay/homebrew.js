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
    const BASE_API_URL = "https://api.svt.se/";
    const BASE_IMG_URL = 'https://www.svtstatic.se/image/custom/400/';
    const PAYLOAD_PATH = window.workingDir + '/ffplay.elf';

    async function resolveStreamURL(channelId) {
	let response = await fetch(BASE_API_URL + 'video/' + channelId);
        if (!response.ok) {
            return '';
        }
        let data = await response.json();
	for (const vidref of data.videoReferences) {
	    if(vidref.format == 'hls') {
		return ['-fs', vidref.redirect];
	    }
	}

	return '';
    }

    async function getChannelList() {
	const params = new URLSearchParams({
	    "ua": "svtplaywebb-play-render-produnction-client",
            "operationName": "ChannelsQuery",
            "variables": "{}",
	    "extensions": JSON.stringify({
		"persistedQuery":{
		    "sha256Hash":"21da6ea41fa3a9300a7b51071f9dc91317955a7dcddc800ddce58fc708e7c634",
		    "version" : 1
		}
	    })
	});

	let response = await fetch(BASE_API_URL + "/contento/graphql?" + params.toString());
	if (!response.ok) {
            return [];
        }

	let data = await response.json();
	return data.data.channels.channels.map((ch) => ({
	    mainText: ch.name,
	    secondaryText: ch.running.name,
	    imgPath:BASE_IMG_URL + ch.running.image.id + '/' + ch.running.image.changed,
	    onclick: async() => {
		return {
		    path: PAYLOAD_PATH,
		    args: await resolveStreamURL(ch.id)
		};
	    }
	}));
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
