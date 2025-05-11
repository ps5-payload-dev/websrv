/* Copyright (C) 2024 idlesauce

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


/**
 * throws exception if the server is down
 * @param {string} path 
 * @returns {object?} null if not success
 */
async function fetchJson(path) {
	let response = await fetch(baseURL + path);
	if (!response.ok) {
		return null;
	}
	let data = await response.json();
	return data;
}

// https://stackoverflow.com/questions/105034/how-do-i-create-a-guid-uuid
function uuidv4() {
	return "10000000-1000-4000-8000-100000000000".replace(/[018]/g, c =>
		(+c ^ crypto.getRandomValues(new Uint8Array(1))[0] & 15 >> +c / 4).toString(16)
	);
}

async function sleep(ms) {
	return new Promise(resolve => setTimeout(resolve, ms));
}

class Semaphore {
    /**
     * @param {number} [maxConcurrency]
     */
    constructor(maxConcurrency = 1) {
        /** @type {{resolve: () => void, promise: Promise<void>}[]} */
        this.queue = [];
        this.maxConcurrency = maxConcurrency;
    }

    acquire() {
        let resolver;
        const promise = new Promise((resolve) => {
            resolver = resolve;
        });

        this.queue.push({ resolve: resolver, promise: promise });

        if (this.queue.length <= this.maxConcurrency) {
            return Promise.resolve(); 
        }

        return promise;
    }

    release() {
        if (this.queue.length === 0) {
            return;
        }

        this.queue.shift().resolve();
    }


    awaitCurrentQueue() {
        const promises = this.queue.map(item => item.promise);
        return Promise.all(promises);
    }
}

function redactUrlPassword(url) {
    try {
        var urlObj = new URL(url);
    } catch (e) {
        return url; // invalid URL
    }

    if (urlObj.username && urlObj.password) {
        urlObj.password = "*";
    }
    return urlObj.toString();
}

function getFriendlySmbShareName(location) {
    // sharename (host)
    const url = new URL(location);
    const host = url.hostname;
    let sharename = url.pathname;
    if (sharename.startsWith("/")) {
        sharename = sharename.substring(1);
    }
    sharename = sharename.split("/")[0];
    return `${sharename} (${host})`;
}

function strCountChar(str, char) {
    let count = 0;
    for (let i = 0; i < str.length; i++) {
        if (str[i] === char) {
            count++;
        }
    }
    return count;
}

function addTrailingSlashIfNeeded(path) {
    if (path.endsWith("/")) {
        return path;
    }
    return path + "/";
}

function createSvgUseElement(href) {
    const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    const use = document.createElementNS("http://www.w3.org/2000/svg", "use");
    if (!href.startsWith("#")) {
        href = "#" + href;
    }
    use.setAttributeNS("http://www.w3.org/1999/xlink", "xlink:href", href);
    use.setAttribute("href", href);
    svg.appendChild(use);

    return svg;
}

function protocolToFriendlyName(protocol) {
    // trim : from the end
    if (protocol.endsWith(":")) {
        protocol = protocol.substring(0, protocol.length - 1);
    }

    // trim everything after .
    const dotIndex = protocol.indexOf(".");
    if (dotIndex !== -1) {
        protocol = protocol.substring(0, dotIndex);
    }

    switch (protocol) {
        case "smb":
        case "_smb":
            return "SMB Share";
        default:
            return protocol.toUpperCase();
    }
}