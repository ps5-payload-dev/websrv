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
	const wrapper = document.createElement("div");
	wrapper.classList.add("launched-app-view");
	content.appendChild(wrapper);

	const msgWrapper = document.createElement("div");
	wrapper.appendChild(msgWrapper);

	const msgIngress = document.createElement("span");
	msgIngress.innerText = "App is running in the background, press ";
	msgWrapper.appendChild(msgIngress);

	const btnCircle = document.createElement("span");
	btnCircle.classList.add("ps-circle-icon");
	msgWrapper.appendChild(btnCircle);

	const textCircle = document.createElement("span");
	textCircle.innerText = " to close this dialog, or ";
	msgWrapper.appendChild(textCircle);

	const btnTriangle = document.createElement("span");
	btnTriangle.classList.add("ps-triangle-icon");
	msgWrapper.appendChild(btnTriangle);

	const textTriangle = document.createElement("span");
	textTriangle.innerText = " to go back to the launcher.";
	msgWrapper.appendChild(textTriangle);

	if (hbldrLogStream) {
		const terminal = document.createElement("div");
		terminal.classList.add("terminal");

		terminal.style.display = "block";
		terminal.style.alignItems = "unset";
		terminal.style.justifyContent = "unset";
		terminal.style.borderWidth = "2px";
		terminal.style.padding = "5px";
		wrapper.appendChild(terminal);

		await (async () => {
			const term = new Terminal({
				convertEol: true,
				altClickMovesCursor: false,
				disableStdin: true,
				fontSize: 18,
				cols: 132,
				rows: 26
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

	const ver = await ApiClient.getVersion();
	if (ver.api > 0) {
		const verstr = ` ${ver.tag} (compiled at ${ver.date} ${ver.time})`;
		document.title += verstr;
	} else {
		document.title += ' (unknown version)';
	}

	registerExtensionMessagesListener();

	// for home page carousel
	document.addEventListener("keydown", (event) => {
		// L2
		if (event.keyCode === 118) {
			// reset hb list
			if (!confirm("Reset homebrew list?")) {
				return;
			}
			localStorage.removeItem(LOCALSTORE_HOMEBREW_LIST_KEY);
			window.location.href = "/";
		}

		// L1 - go to first element in carousel
		if (event.keyCode === 116) {
			smoothScrollToElementIndex(0, true); // doesnt do anything if the current view isnt a carouselView
			event.preventDefault();
		}

		// R1 - go to last element in carousel
		if (event.keyCode === 117) {
			let carouselInfo = getCurrentCarouselSelectionInfo();
			if (!carouselInfo) {
				// current view not a carousel
				return;
			}

			smoothScrollToElementIndex(carouselInfo.totalEntries - 1, true);
			event.preventDefault();
		}

		// carousel left-right dpad
		if (event.keyCode === 39 || event.keyCode === 37) {
			// return if modal is open
			if (document.getElementById("modal-overlay")) {
				return;
			}

			let carouselInfo = getCurrentCarouselSelectionInfo();

			if (!carouselInfo) {
				// current view not a carousel
				return;
			}

			let currentElementIndex = carouselInfo.selectedIndex;
			if (!currentElementIndex) {
				currentElementIndex = 0;
			}

			if (event.keyCode === 39) {
				// right
				// if the current element is the last element, go to the first element
				if (currentElementIndex === carouselInfo.totalEntries - 1 && 
					(Date.now() - Globals.lastCarouselLeftRightKeyDownTimestamp) > 200) { // 200ms so holding down the key doesnt keep scrolling
					currentElementIndex = 0;
				} else {
					currentElementIndex++;
				}
			} else if (event.keyCode === 37) {
				// left
				// if the current element is the first element, go to the last element
				if (currentElementIndex === 0 && 
					(Date.now() - Globals.lastCarouselLeftRightKeyDownTimestamp) > 200) { // 200ms so holding down the key doesnt keep scrolling
					currentElementIndex = carouselInfo.totalEntries - 1;
				} else {
					currentElementIndex--;
				}
			}

			Globals.lastCarouselLeftRightKeyDownTimestamp = Date.now();
			smoothScrollToElementIndex(currentElementIndex, true); // this checks if new index is out of bounds
		}
	});
	// this ctor renders the main page
	Globals.Router = new Router();
};

