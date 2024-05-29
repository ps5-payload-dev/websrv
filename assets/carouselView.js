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
 * @typedef {Object} CarouselItemSecondaryActions
 * @property {string} text
 * @property {function():(boolean|void|Promise<boolean>|Promise<void>)} onclick - return true to display you can exit now
 */

/**
 * @typedef {Object} CarouselItem
 * @property {string} mainText
 * @property {(function():(boolean|void|Promise<boolean>|Promise<void>))?} onclick - return true to display you can exit now
 * @property {(function():Promise<ExtensionInfo>)} [asyncInfo]
 * @property {string} [secondaryText]
 * @property {string} [imgPath]
 * @property {CarouselItemSecondaryActions[]} [options]
 */


/**
 * Shouldnt be interacted with directly, use router methods
 * @param {CarouselItem[]} items
 */
async function renderMainContentCarousel(items, fadeout = true) {
    // reset content
    const content = /** @type {HTMLElement?} */ (document.getElementById("content"));
    if (!content) {
        // window.location.href = "/";
        debugger;
        return;
    }

    if (content.children.length > 0) {
        if (fadeout) {
            let elementsWithLoading = content.children[0].querySelectorAll(".loading");
            for (let loading of elementsWithLoading) {
                // specifically not removing the overlay to make it look less jarring
                loading.classList.remove("loading");
            }

            for (let i = 0; i < content.children.length; i++) {
                content.children[i].classList.add("fadeout");
            }
            await sleep(250);
        }

        content.innerHTML = "";
    }

    // create carousel div
    const carousel = document.createElement("div");
    carousel.classList.add("carousel");
    carousel.classList.add("fadein");
    content.appendChild(carousel);

    let asyncInfoPromisesList = [];

    // add entries
    for (let i = 0; i < items.length; i++) {
        const item = items[i];
        let ii = i;

        const entryElement = document.createElement("div");
        entryElement.classList.add("entry-wrapper");

        if (i == 0) {
            entryElement.classList.add("selected");
        }

        const entryElementMainButton = document.createElement("div");
        entryElementMainButton.classList.add("entry");
        entryElementMainButton.classList.add("entry-main");

        const entryElementImg = document.createElement("img");
        entryElementImg.classList.add("entry-img");
        entryElementImg.style.display = "none";
        entryElementImg.onerror = () => {
            entryElementImg.style.display = "none";
        }
        entryElementMainButton.appendChild(entryElementImg);


        const entryElementMainText = document.createElement("p");
        entryElementMainText.classList.add("entry-name");
        entryElementMainText.innerText = item.asyncInfo ? "" : item.mainText;
        entryElementMainButton.appendChild(entryElementMainText);
        entryElement.appendChild(entryElementMainButton);

        if (item.asyncInfo) {
            entryElementMainButton.classList.add("loading-overlay");
            entryElementMainButton.classList.add("loading");
        }

        carousel.appendChild(entryElement);

        asyncInfoPromisesList.push(async () => {
            let asyncInfo = null;
            if (item.asyncInfo) {
                asyncInfo = await item.asyncInfo();
                entryElementMainButton.classList.remove("loading-overlay");
                entryElementMainButton.classList.remove("loading");
            }

            if (asyncInfo && asyncInfo.result.mainText) {
                entryElementMainText.innerText = asyncInfo.result.mainText;
            }

            if ((asyncInfo && asyncInfo.result.secondaryText) || item.secondaryText) {
                const entryElementSecondaryText = document.createElement("p");
                entryElementSecondaryText.classList.add("entry-secondary");
                entryElementSecondaryText.classList.add("text-secondary");
                entryElementSecondaryText.innerText = ((asyncInfo && asyncInfo.result.secondaryText) ? asyncInfo.result.secondaryText : item.secondaryText) || "";
                entryElementMainButton.appendChild(entryElementSecondaryText);
            }

            if (asyncInfo && !asyncInfo.uuid) {
                // TODO: this is temporary maybe, idk if its better to show an error on the entry or remove the entry altogether

                // return if uuid is null which only happens on a timeout
                // so image and onclicks arent set
                return;
            }

            if ((asyncInfo && asyncInfo.result.imgPath) || item.imgPath) {
                entryElementImg.style.display = "block";
                let imgPath = ((asyncInfo && asyncInfo.result.imgPath) ? asyncInfo.result.imgPath : item.imgPath) || "";
                // workaround for svgs since the server doesnt return mime
                if (imgPath.trimEnd().endsWith(".svg")) {
                    try {
                        let imgBytes = await (await fetch(imgPath)).arrayBuffer();
                        let imgBase64 = btoa(String.fromCharCode.apply(null, new Uint8Array(imgBytes)));
                        imgPath = "data:image/svg+xml;base64," + imgBase64;
                    }
                    catch (e) {
                        // nothing
                    }
                }

                entryElementImg.src = imgPath;
            }

            if ((asyncInfo && asyncInfo.result.options) || item.options) {
                const entryElementMoreOptionsButton = document.createElement("div");
                entryElementMoreOptionsButton.classList.add("entry");
                entryElementMoreOptionsButton.classList.add("entry-more-button");
                entryElementMoreOptionsButton.innerHTML = "&bull;&bull;&bull;";

                entryElementMoreOptionsButton.onclick = async () => {
                    if (!entryElement.classList.contains("selected")) {
                        smoothScrollToElementIndex(ii, true);
                        return;
                    }
                    let options = null;
                    if (asyncInfo && asyncInfo.result.options) {
                        options = asyncInfo.result.options;
                    }
                    else if (item.options) {
                        options = item.options;
                    }
                    // @ts-ignore
                    await Globals.Router.handleModal(options);
                }

                entryElement.appendChild(entryElementMoreOptionsButton);
            }


            entryElementMainButton.onclick = async () => {
                if (!entryElement.classList.contains("selected")) {
                    smoothScrollToElementIndex(ii, true);
                    return;
                }

                entryElementMainButton.classList.add("loading-overlay");
                entryElementMainButton.classList.add("loading");
                await sleep(0); // TODO: Didnt test if its needed here, but i think if the asyncInfo.result.onclick isnt async we might need this to trigger a rerender, or ideally something better
                let onclickResult = null;
                if (asyncInfo && asyncInfo.result.onclick) {
                    onclickResult = await asyncInfo.result.onclick();
                }
                else if (item.onclick) {
                    onclickResult = await item.onclick();
                }
                else {
                    // TODO: invalid item, remove
                }

                let res = onclickResult;
                let logStream = null;

                if (res && res.path) {
                    logStream = await ApiClient.launchApp(res.path, res.args);
                    res = logStream != null;
                }

                if (res == true) {
                    entryElement.style.transform = "scale(2)";
                    setTimeout(() => {
                        entryElement.style.removeProperty("transform");
                    }, 300);
                    Globals.Router.handleLaunchedAppView(logStream);
                }

                entryElementMainButton.classList.remove("loading-overlay");
                entryElementMainButton.classList.remove("loading");
            }

        });

    }

    smoothScrollToElementIndex(0, false);

    // await all and reset cursor snap overlays every time a task completes
    await Promise.all(asyncInfoPromisesList.map(asyncInfoPromise => asyncInfoPromise().then(() => {
        if (document.getElementById("modal-content")) {
            return;
        }
        generateCursorSnapOverlays();
    })));
}


function removeAllCursorSnapOverlays() {
    const content = document.getElementById("content");
    if (!content) {
        return;
    }
    const overlays = content.getElementsByClassName("home-cursor-snap-overlay");
    while (overlays.length > 0) {
        overlays[0].remove();
    }
}

function generateCursorSnapOverlays(entryWrapperIndex = -1) {
    // create overlay for cursor snap
    // remove existing overlays
    removeAllCursorSnapOverlays();
    let content = document.getElementById("content");
    if (!content) {
        return;
    }
    if (entryWrapperIndex === -1) {
        // set to index of entry-wrapper with selected class
        let wrappers = content.getElementsByClassName("entry-wrapper");
        for (let i = 0; i < wrappers.length; i++) {
            if (wrappers[i].classList.contains("selected")) {
                entryWrapperIndex = i;
                break;
            }
        }
    }

    const entry = content.getElementsByClassName("entry-wrapper")[entryWrapperIndex];

    if (!entry) {
        return;
    }

    // for each entry in entry-wrapper make a cursor snap overlay
    for (let i = 0; i < entry.children.length; i++) {
        const entryChild = entry.children[i];
        const overlay = document.createElement("a");
        overlay.classList.add("home-cursor-snap-overlay");
        overlay.style.position = "fixed";

        // calculate the offset from the top of the viewport

        const entryChildRect = entryChild.getBoundingClientRect();
        let entryChildBottom = entryChildRect.top + entryChildRect.height;
        let entryChildHeight = entryChildRect.height;
        let topOffset = 0;

        // this method has not added the selected class yet so if theres a selected class on the wrapper
        // thats because the element was initialized with it, in that case the element is already 1.2x scaled
        // but otherwise the bounding box is the 1.0x size here bc the animation has not run yet
        // even if we add the selected class the bounding box scales with the animation so its useless until its finished
        if (!entry.classList.contains("selected")) {
            // we gotta calculate the transforms
            const transformScale = 1.2;
            const parentRect = entry.getBoundingClientRect();
            const parentCenter = parentRect.top + parentRect.height / 2;
            const entryTopRelativeToParentCenter = entryChildRect.top - parentCenter;
            const entryBottomRelativeToParentCenter = entryChildRect.bottom - parentCenter;
            const entryTransformedTop = parentCenter + (entryTopRelativeToParentCenter * transformScale);
            const entryTransformedBottom = parentCenter + (entryBottomRelativeToParentCenter * transformScale);
            const entryTransformedHeight = entryTransformedBottom - entryTransformedTop;
            entryChildHeight = entryTransformedHeight;
            entryChildBottom = entryTransformedBottom;
        }

        if (entryChild.classList.contains("entry")) {
            // main big button
            // snap cursor to the lower 3/8 of the button
            topOffset = entryChildBottom - entryChildHeight * (2 / 5);
        } else {
            // snap to center
            topOffset = entryChildBottom - (entryChildHeight / 2);
        }

        overlay.style.top = topOffset + "px";
        overlay.style.left = "0";
        overlay.style.right = "0";
        overlay.style.height = "1px";
        // allow clicks through
        overlay.style.pointerEvents = "none";
        // snap cursor
        overlay.tabIndex = 0;
        content.appendChild(overlay);
    }
}


/**
 * Only for home page carousel, also sets selected class and creates the cursor snap points
 */
function smoothScrollToElementIndex(index, smooth = true) {
    const content = document.getElementById("content");
    if (!content) {
        return;
    }
    const carousel = content.getElementsByClassName("carousel")[0];
    if (!carousel){
        return;
    }
    const entries = carousel.getElementsByClassName("entry-wrapper");

    if (index < 0 || index >= entries.length) {
        return;
    }

    const entry = entries[index];

    generateCursorSnapOverlays(index);

    for (let t_entry of entries) {
        t_entry.classList.remove("selected");
    }

    entry.classList.add("selected");

    // webkit doesnt do smooth scrolling and smoothscroll.js cant center the item so we have to do it manually
    const itemRect = entry.getBoundingClientRect();
    const containerRect = content.getBoundingClientRect();
    const targetScrollLeft = itemRect.left - containerRect.left + content.scrollLeft - (containerRect.width - itemRect.width) / 2;

    if (targetScrollLeft < 1 && targetScrollLeft > -1) {
        return;
    }

    if (smooth) {
        carousel.scrollBy({ left: targetScrollLeft, behavior: "smooth" });
        carousel.classList.add("scrolling");
        if (Globals.removeScrollingClassFromCarouselTimeoutId) {
            clearTimeout(Globals.removeScrollingClassFromCarouselTimeoutId);
        }
        Globals.removeScrollingClassFromCarouselTimeoutId = setTimeout(() => {
            carousel.classList.remove("scrolling");
        }, 468); // 468 the constant length of the scrolling with smoothscroll.js
    } else {
        carousel.scrollLeft = targetScrollLeft;
    }
}

/**
 * @returns {number | null}
 */
function getCurrentSelectedEntryIndex() {
    const content = /** @type {HTMLElement} */ (document.getElementById("content"));
    const carousel = content.getElementsByClassName("carousel")[0];
    if (!carousel) {
        console.error("this shouldnt happen");
        return null;
    }

    const entries = carousel.getElementsByClassName("entry-wrapper");
    let currentElementIndex = Array.from(entries).findIndex(entry => entry.classList.contains("selected"));
    return currentElementIndex;
}
