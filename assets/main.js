// @ts-check

async function renderHomePage() {
	await scanHomebrews();
	let items = getHomebrewList();

	// remove previous extension sandboxes
	let oldsandbox = document.getElementById('js-extension-sandbox');
	if (oldsandbox) {
		oldsandbox.remove();
	}
	Globals.Funcs = {};

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
			if (item.filename.endsWith('.js')) {
				newItem.asyncInfo = async () => {
					let temp = await loadJsExtension(item.path);
					if (!temp) {
						// TODO: Remove entry
						return { result: { mainText: "Error", secondaryText: "Extension main() timed out." } };
					}
					temp.result = remapFunctionIdsToSandboxedCalls(temp.result, temp.uuid);
					return temp;
				}
			} else {
				newItem.onclick = () => {
					return ApiClient.launchApp(item.path);
				}

				newItem.options = [
					{
						text: 'Remove from list',
						// same issue as below
						// onclick: () => { removeFromHomebrewList(item.path); Globals.Router.navigate("/"); }
						onclick: new Function(`
							return () => {
								removeFromHomebrewList("${item.path}");
								Globals.Router.navigate("/");
							}
						`)()
					},
					{
						text: 'Launch with args',
						// same issue as below
						// onclick: () => { let args = window.prompt('Enter args'); if (args) { ApiClient.launchApp(item.path, args); } }
						onclick: new Function(`
							return () => {
								let args = window.prompt('Enter args');
								if (args) { ApiClient.launchApp("${item.path}", args); }
							}
						`)()
					},
					{
						text: 'Launch with file',
						// TODO: i dont get whats wrong with this, the execpath inside wont get captured no matter what i tried
						// onclick: ((execPath) => {
						// 	console.log("execPath", execPath);
						// 	return (path) => {
						// 		let funcId = uuidv4();
						// 		console.log("execpath2", execPath);
						// 		Globals.Funcs[funcId] = (path) => {
						// 			ApiClient.launchApp(execPath, path);
						// 			delete Globals.Funcs[funcId];
						// 		}
						// 		Globals.Router.navigate('/browser?' + new URLSearchParams({
						// 			initialPath: execPath.substring(0, execPath.lastIndexOf('/') + 1),
						// 			a: BROWSER_CALL_FUNCTIONID_AT_END,
						// 			functionId: funcId,
						// 			execPath: execPath
						// 		}));
						// 	}
						// })(item.path.toString())
						onclick: new Function(`
							return () => { 
								let funcId = uuidv4();
								Globals.Funcs[funcId] = (path) => {
									ApiClient.launchApp("${item.path}", path);
									delete Globals.Funcs[funcId];
								}
								Globals.Router.navigate('/browser?' + new URLSearchParams({
									initialPath: "${item.path.substring(0, item.path.lastIndexOf('/') + 1)}",
									a: BROWSER_CALL_FUNCTIONID_AT_END,
									functionId: funcId,
									backCount: 2
								}));
							}
						`)()
						
					}
				];
			}

			acc.push(newItem);
			return acc;
		}, []);

	// add "Add more..." button
	carouselItems.push({
		mainText: '+',
		secondaryText: 'Add more...',
		onclick: () => {
			Globals.Router.navigate('/browser?' + new URLSearchParams(
				{
					initialPath: '/',
					a: BROWSER_REGISTER_HOMEBREW_ACTION
				}).toString()
			);
		}
	})

	await renderMainContentCarousel(carouselItems);
}

// @ts-ignore
document.onload = (async function () {
	window.addEventListener('error', (event) => {
		alert(event.error);
		// window.location.href = '/';
	})
	window.addEventListener('unhandledrejection', (event) => {
		alert(event.reason);
		// window.location.href = '/';
	});

	registerExtensionMessagesListener();

	// for home page carousel
	document.addEventListener('keydown', (event) => {
		// L1
		if (event.keyCode === 116) {
			// reset hb list
			if (!confirm('Reset homebrew list?')) {
				return;
			}
			localStorage.removeItem(LOCALSTORE_HOMEBREW_LIST_KEY);
			window.location.href = '/';
		}
		if (event.keyCode === 39 || event.keyCode === 37) {
			// return if modal is open
			if (document.getElementById('modal-overlay')) {
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
})();

window.addEventListener("load", () => {
	// function references are lost at refresh which causes issues at some routes
	// on initial load lets ignore the path and redirect to index
	if (window.location.hash != '') {
		window.location.href = '/';
		return;
	}

	Globals.Router
		.on("/browser", async (match) => {
			closeModal();

			switch (match.params.a) {
				case BROWSER_REGISTER_HOMEBREW_ACTION:
					browseHandleRegisterHomebrewAction(match.params.initialPath);
					break;

				case BROWSER_CALL_FUNCTIONID_AT_END:
					browseHandleCallFunctionAtEnd(match.params.initialPath, match.params.functionId, match.params.backCount ? match.params.backCount : 1 );
					break;
			}
		})
		.on("/modal", async (match) => {
			closeModal();
			await renderModalOverlay(remapFunctionIdsToLocalCalls(JSON.parse(decodeURIComponent(match.params.items))));
		})
		.on("/carousel", async (match) => {
			closeModal();
			await renderMainContentCarousel(remapFunctionIdsToLocalCalls(JSON.parse(decodeURIComponent(match.params.items))));
		})
		.on(async (match) => {
			closeModal();
			await renderHomePage();
		})
		.resolve();
});