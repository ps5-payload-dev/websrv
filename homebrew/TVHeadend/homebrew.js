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

// TODO: make BASE_URL configurable via UI?

async function main() {
    const BASE_URL = "http://192.168.1.2:9981/"; // CHANGE ME
    const PAYLOAD_PATH = window.workingDir + '/ffplay.elf';

    async function getChannelProgramme(chid) {
        const params = new URLSearchParams({
            "mode": "now",
            "channel": chid
        });
        let response = await fetch(BASE_URL + "api/epg/events/grid?" + params.toString());
        if (!response.ok) {
            return '';
        }
        let data = await response.json();
        if(!data.totalCount) {
            return '';
        }
        return data.entries[0].title;
    }

    async function getChannelList() {
	const params = new URLSearchParams({
	    "sort": "number",
	    "filter": '[{"type":"boolean","value":true,"field":"enabled"}]'
	});
	let response = await fetch(BASE_URL + "/api/channel/grid?" + params.toString());
	if (!response.ok) {
            return [];
        }

	let data = await response.json();

	return await Promise.all(data.entries.map(async (ch) => {
	    let proginfo = await getChannelProgramme(ch.uuid);
            var icon = ch.icon_public_url;

            if(!icon) {
                icon = 'default-icon.png';
            }
            if(icon.startsWith('imagecache')) {
                icon = BASE_URL + icon;
            }
	    return {
		mainText: ch.name,
		secondaryText: proginfo,
		imgPath: icon,
		onclick: async () => {
	            return {
			path: PAYLOAD_PATH,
			args: [BASE_URL + '/play/stream/channel/' + ch.uuid,
                              '-vf', 'yadif=1:-1', '-sn']
		    };
		}
	    };
	}));
    }

    return {
        mainText: "TVHeadend",
        secondaryText: 'TV streaming server for Linux',
        onclick: async () => {
	    let items = await getChannelList();
            showCarousel(items);
        }
    };
}
