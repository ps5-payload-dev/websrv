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

async function renderHomePage() {
	await scanHomebrews();
	let items = getHomebrewList();

	// remove previous extension sandboxes
	let oldsandbox = document.getElementById("js-extension-sandbox");
	if (oldsandbox) {
		oldsandbox.remove();
	}

	let carouselItems = items.reduce(
		/**
		 * @param {Array} acc
		 * @param {*} item 
		 * @returns {CarouselItem[]} 
		 */
		(acc, item) => {
			let newItem = {
				mainText: item.filename,
				secondaryText: item.dir,
				imgPath: baseURL + "/fs" + item.dir + "/sce_sys/icon0.png"
			}
			if (item.filename.endsWith(".js")) {
				newItem.asyncInfo = async () => {
					let temp = await loadJsExtension(item.path);
					if (!temp) {
						// TODO: Remove entry
						return { result: { mainText: "Error", secondaryText: "Extension main() timed out." } };
					}

					// let removeFromListOption = {
					// 	text: "Remove from list",
					// 	onclick: async () => {
					// 		removeFromHomebrewList(item.path);
					// 		await Globals.Router.handleHome();
					// 	}
					// };

					// if (temp.result.options) {
					// 	temp.result.options.push(removeFromListOption);
					// } else {
					// 	temp.result.options = [removeFromListOption];
					// }

					return temp;
				}
			} else {
				newItem.onclick = () => {
					return { path: item.path };
				}

				newItem.options = [
					{
						text: "Remove from list",
						onclick: async () => {
							removeFromHomebrewList(item.path);
							await Globals.Router.handleHome();
						}
					},
					{
						text: "Launch with args",
						onclick: () => {
							let args = window.prompt("Enter args");
							if (args) {
								return { path: item.path, args: args };
							}
							return null;
						}
					},
					{
						text: "Launch with file",
						onclick: async () => {
							let result = await Globals.Router.pickFile(item.path.substring(0, item.path.lastIndexOf("/")));

							if (result) {
								return { path: item.path, args: result };
							}
						}
					}
				];
			}

			acc.push(newItem);
			return acc;
		}, []);

	// add "Add more..." button
	carouselItems.push({
		mainText: "+",
		secondaryText: "Add more...",
		onclick: async () => {
			let result = await Globals.Router.pickFile();

			if (result) {
				addToHomebrewStore(result);
				await Globals.Router.handleHome();
			}
		}
	})

	await renderMainContentCarousel(carouselItems);
}

/**
 * @param {ReadableStream<Uint8Array>?} [hbldrLogStream] 
 */
async function renderLaunchedAppView(hbldrLogStream = null) {
	const content = document.getElementById("content");
	if (!content) {
		return;
	}

	let canExitWithBackButton = Globals.Router.initialHistoryLength <= 1;

	const wrapper = document.createElement("div");
	wrapper.classList.add("launched-app-view");

	const text1 = document.createElement("p");
	text1.innerText = "App has launched in the background.";
	wrapper.appendChild(text1);

	const text2 = document.createElement("div");
	const text2_1 = document.createElement("span");
	text2_1.innerText = "You can close with window by pressing ";
	text2.appendChild(text2_1);

	const psIconDiv = document.createElement("div");
	psIconDiv.classList.add("ps-icon");
	text2.appendChild(psIconDiv);

	if (canExitWithBackButton) {
		const text2_2 = document.createElement("span");
		text2_2.innerText = " or ";
		text2.appendChild(text2_2);
		const circleIconDiv = document.createElement("div");
		circleIconDiv.classList.add("ps-circle-icon");
		text2.appendChild(circleIconDiv);
	}
	wrapper.appendChild(text2);

	if (hbldrLogStream) {
		const text3 = document.createElement("div");

		const text3_1 = document.createElement("span");
		text3_1.innerText = "Press ";
		text3.appendChild(text3_1);

		const text3_2 = document.createElement("div");
		text3_2.classList.add("ps-r2-icon");
		text3.appendChild(text3_2);

		const text3_3 = document.createElement("span");
		text3_3.innerText = " to view logs.";
		text3.appendChild(text3_3);

		wrapper.appendChild(text3);
	}

	const text4 = document.createElement("div");

	const text4_1 = document.createElement("span");
	text4_1.innerText = "Press ";
	text4.appendChild(text4_1);

	const text4_2 = document.createElement("div");
	text4_2.classList.add("ps-triangle-icon");
	text4.appendChild(text4_2);

	const text4_3 = document.createElement("span");
	text4_3.innerText = " to get back to menu.";
	text4.appendChild(text4_3);

	wrapper.appendChild(text4);

	const terminalWrapper = document.createElement("div");
	terminalWrapper.classList.add("terminal-wrapper");
	// terminalWrapper.style.display = "flex";
	terminalWrapper.style.display = "none";


	const terminal = document.createElement("div");
	terminal.classList.add("terminal");
	
	terminal.style.display = "block";
	terminal.style.alignItems = "unset";
	terminal.style.justifyContent = "unset";

	terminalWrapper.appendChild(terminal);

	wrapper.appendChild(terminalWrapper);

	content.appendChild(wrapper);

	if (hbldrLogStream) {
		await (async () => {
			const term = new Terminal({
				convertEol: true,
				altClickMovesCursor: false,
				disableStdin: true
			});

			try {
				const reader = hbldrLogStream.getReader();
				const decoder = new TextDecoder();

				term.open(terminal);
				while (true) {
					const { done, value } = await reader.read();
					if (done) {
						break;
					}
					let decodedValue = decoder.decode(value);
					// alert(decodedValue);
					term.write(decodedValue);
				}
			} catch (error) {

			}
		})();
	}
}

window.onload = async function () {
	window.addEventListener("error", (event) => {
		alert(event.error);
		// window.location.href = "/";
	})
	window.addEventListener("unhandledrejection", (event) => {
		alert(event.reason);
		// window.location.href = "/";
	});

	registerExtensionMessagesListener();

	// for home page carousel
	document.addEventListener("keydown", (event) => {
		// R2
		if (event.keyCode === 119) {
			// try to find termnial-wrapper
			const terminalWrappers = document.getElementsByClassName("terminal-wrapper");
			if (terminalWrappers.length === 0) {
				return;
			}

			const terminalWrapper = terminalWrappers[0];

			if (terminalWrapper.style.display === "none") {
				terminalWrapper.style.display = "flex";
			} else {
				terminalWrapper.style.display = "none";
			}
		}

		// L1
		if (event.keyCode === 116) {
			// reset hb list
			if (!confirm("Reset homebrew list?")) {
				return;
			}
			localStorage.removeItem(LOCALSTORE_HOMEBREW_LIST_KEY);
			window.location.href = "/";
		}

		// carousel left-right dpad
		if (event.keyCode === 39 || event.keyCode === 37) {
			// return if modal is open
			if (document.getElementById("modal-overlay")) {
				return;
			}

			let currentElementIndex = getCurrentSelectedEntryIndex();
			if (!currentElementIndex) {
				currentElementIndex = 0;
			}

			if (event.keyCode === 39) {
				// right
				currentElementIndex++;
			} else if (event.keyCode === 37) {
				// left
				currentElementIndex--;
			}
			smoothScrollToElementIndex(currentElementIndex, true);
		}

	});

	// this ctor renders the main page
	Globals.Router = new Router();
};

