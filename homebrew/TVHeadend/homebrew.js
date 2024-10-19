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

    async function getChannelListByTag(tagId) {
        const params = new URLSearchParams({
            "sort": "number",
            "filter": '[{"type":"boolean","value":true,"field":"enabled"}]',
            "limit": "500" //set to high number as default is 50
        });
        let response = await fetch(BASE_URL + "api/channel/grid?" + params.toString());
        if (!response.ok) {
            return [];
        }

        let data = await response.json();
        const filteredChannels = data.entries.filter(ch => ch.tags.includes(tagId)); // Filter channels to only include those with the selected tag

        return await Promise.all(filteredChannels.map(async (ch) => {
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

    async function getDvrList() {     
        const params = new URLSearchParams({
            "sort": "start",
            "dir": "desc"
        });
        let response = await fetch(BASE_URL + "api/dvr/entry/grid_finished?" + params.toString());
        if (!response.ok) {
            return [];
        }

        let data = await response.json();

        return await Promise.all(data.entries.map(async (dvr) => {
            var icon = dvr.channel_icon;

            if(!icon) {
                icon = 'default-icon.png';
            }
            if(icon.startsWith('imagecache')) {
                icon = BASE_URL + icon;
            }
            return {
                mainText: dvr.channelname,
                secondaryText: dvr.disp_title, 
                imgPath: icon,
                onclick: async () => {
                    return {
                        path: PAYLOAD_PATH,
                        args: [BASE_URL + '/play/dvrfile/' + dvr.uuid,
                              '-vf', 'yadif=1:-1', '-sn']
                    };
                }
            };
        }));
    }

    async function getSectionList() {
        const params = new URLSearchParams({
            "sort": "index"
        });
        let response = await fetch(BASE_URL + "api/channeltag/grid?" + params.toString());
        if (!response.ok) {
            return [];
        }

        let data = await response.json();
        let sections = data.entries.map(tag => ({
            mainText: tag.name,
            onclick: async () => {
                let items = await getChannelListByTag(tag.uuid);
                showCarousel(items); // Display the available channel sections
            }
        }));

        let dvrItems = await getDvrList(); 
        if(dvrItems.length > 0) {
            sections.push({
                mainText: 'DVR',
                onclick: async () => {
                    showCarousel(dvrItems); // Display completed DVR entries
                }
            });
        }

        return sections;
    }

    return {
        mainText: "TVHeadend",
        secondaryText: 'TV streaming server for Linux',
        onclick: async () => {
            let items = await getSectionList(); //available channel sections + DVR section (if any completed recordings exist)
            showCarousel(items);
        }
    };
}
