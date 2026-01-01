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
    const WORKDIR = window.workingDir;
    const PAYLOAD = WORKDIR + '/transmission-daemon.elf';
    const ARGS = ["transmission-daemon", "--foreground"];
    const ENV = {HOME: WORKDIR,
		 XDG_CONFIG_HOME: WORKDIR,
		 TRANSMISSION_HOME: WORKDIR,
		 TRANSMISSION_WEB_HOME: WORKDIR + "/public_html"};

    async function isRunning() {
	const webIfUrl = window.location.origin == "null" ? "http://127.0.0.1:9091" : window.location.origin.replace(":8080", ":9091");
	let response = await fetch(webIfUrl);
	return response.ok;
    }

    return {
        mainText: "Transmission",
        secondaryText: 'A Fast, Easy and Free Bittorrent Client',
        onclick: async () => {
	    if(await isRunning()) {
		alert("Transmission is already running");
		return null;
	    }
	    return {
		path: PAYLOAD,
		cwd: WORKDIR,
		args: ARGS,
		env: ENV,
		daemon: true
            }
	}
    };
}
