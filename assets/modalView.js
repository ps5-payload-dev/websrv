// @ts-check

/**
 * @typedef {Object} ModalItem
 * @property {string} text
 * @property {string} onclick
 */

/**
 * @param {ModalItem[]} items
 */
function renderModalOverlay(items) {
	let modalOverlay = document.getElementById('modal-overlay');
	if (modalOverlay) {
		modalOverlay.remove();
	}

	removeAllCursorSnapOverlays();

	const overlay = document.createElement('div');
	overlay.id = 'modal-overlay';

	const modalContent = document.createElement('div');
	modalContent.id = 'modal-content';

	for (let item of items) {
		const entry = document.createElement('a');
		entry.classList.add('list-entry');
		const textElement = document.createElement('p');
		textElement.classList.add('text-center');
		textElement.innerText = item.text;
		entry.appendChild(textElement);

		entry.onclick = () => {
			let onlick = new Function('return ' + item.onclick)();
			onlick();
		}
		entry.tabIndex = 1;
		modalContent.appendChild(entry);
	}

	overlay.appendChild(modalContent);
	document.body.appendChild(overlay);
}

function closeModal() {
	let overlay = document.getElementById('modal-overlay');
	if (!overlay) {
		return;
	}
	overlay.remove();
	generateCursorSnapOverlays();
	// TODO: animate
}
