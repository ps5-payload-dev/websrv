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
	constructor(max) {
		this.max = max;
		this.current = 0;
		this.queue = [];
	}

	acquire() {
		if (this.current < this.max) {
			this.current++;
			return Promise.resolve();
		} else {
			return new Promise((resolve) => this.queue.push(resolve));
		}
	}

	release() {
		if (this.queue.length > 0) {
			const resolve = this.queue.shift();
			resolve();
		} else {
			this.current--;
		}
	}

	awaitCurrentQueue() {
		return Promise.all(this.queue);
	}
}
