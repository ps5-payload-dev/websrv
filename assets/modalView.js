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


// @ts-check

/**
 * @typedef {Object} ModalItem
 * @property {string} text
 * @property {function():any} onclick
 */

/**
 * Shouldnt be interacted with directly, use router methods
 * @param {ModalItem[]} items
 */
function renderModalOverlay(items) {
	const content = document.getElementById("content");
	if (!content) {
		throw new Error("content not found");
	}
	const oldOverlays = content.querySelectorAll("[data-modal-overlay]");
	for (let overlay of oldOverlays) {
		overlay.remove();
	}

	removeAllCursorSnapOverlays();

	const overlay = document.createElement("div");
	overlay.setAttribute("data-modal-overlay", "");
	overlay.classList.add("modal-overlay");

	const modalContent = document.createElement("div");
	modalContent.classList.add("modal-content");

	for (let item of items) {
		const entry = document.createElement("a");
		entry.classList.add("list-entry");
		entry.style.transition = "transform 0.3s ease";
		entry.style.position = "relative";
		const textElement = document.createElement("p");
		textElement.classList.add("text-center");
		textElement.innerText = item.text;
		entry.appendChild(textElement);

		// entry.onclick = item.onclick;
		entry.onclick = async () => {

			entry.classList.add("loading-overlay");
			entry.classList.add("loading");
			await sleep(0);

			let onclickResult = await item.onclick();

			let res = onclickResult;
			let logStream = null;

			if (res && res.path) {
				logStream = await ApiClient.launchApp(res.path, res.args, res.env, res.cwd, res.daemon);
				res = logStream != null;
			}

			if (res == true) {
				entry.style.transform = "scale(2)";
				setTimeout(() => {
					entry.style.removeProperty("transform");
				}, 300);
				Globals.Router.handleLaunchedAppView(logStream);
			}
			
			entry.classList.remove("loading-overlay");
			entry.classList.remove("loading");
		}
		entry.tabIndex = 1;
		modalContent.appendChild(entry);
	}

	overlay.appendChild(modalContent);
	content.appendChild(overlay);
}

function closeModal() {
	const content = document.getElementById("content");
	if (!content) {
		throw new Error("content not found");
	}
	const oldOverlays = content.querySelectorAll("[data-modal-overlay]");
	for (let overlay of oldOverlays) {
		overlay.remove();
	}
	generateCursorSnapOverlays();
}
