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
    const BASE_API_URL = "http://api.sr.se/api/v2/";
    const PAYLOAD_PATH = window.workingDir + '/ffplay.elf';

    async function getChannelProgramme(chid) {
	const params = new URLSearchParams({
	    "channelid": chid,
	    "format": "json"
	});

	try {
	    let response = await fetch(BASE_API_URL + "/scheduledepisodes/rightnow?" + params.toString());
	    if (response.ok) {
		let data = await response.json();
		return data.channel.currentscheduledepisode;
            }
        } catch (error) {
        }
    }

    async function getChannelList() {
	const params = new URLSearchParams({
	    "format": "json",
            "pagination": "false",
	    "audioquality": "hi"
	});
	let response = await fetch(BASE_API_URL + "/channels?" + params.toString());
	if (!response.ok) {
            return [];
        }

	let data = await response.json();

	return await Promise.all(data.channels.map(async (ch) => {
	    let img = ch.image;
	    let title = '';
	    let proginfo = await getChannelProgramme(ch.id);

	    if(proginfo != undefined) {
		title = proginfo.title;
		if(proginfo.socialimage) {
		    img = proginfo.socialimage;
		}
	    }

	    return {
		mainText: ch.name,
		secondaryText: title,
		imgPath: img,
		onclick: async () => {
		    return {
			path: PAYLOAD_PATH,
			args: ch.liveaudio.url
		    };
		}
	    };
	}));
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
