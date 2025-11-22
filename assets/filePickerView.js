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

/** @typedef {Object} BrowsePageCategoryItemSecondaryButton
 * @property {string} icon
 * @property {function} onclick
 */

/** @typedef {Object} BrowsePageCategoryItem
 * @property {string} primaryText
 * @property {string?} [secondaryText]
 * @property {string?} [endTextPrimary]
 * @property {string?} [endTextSecondary]
 * @property {string} icon
 * @property {function} onclick
 * @property {BrowsePageCategoryItemSecondaryButton[]?} [secondaryButtons]
 */

/** @typedef {Object} BrowsePageCategory
 * @property {string?} name
 * @property {BrowsePageCategoryItem[]} items
 */

/**
 * @param {BrowsePageCategory[]} items
 * @returns {Promise<{path: string, finished: boolean}>}
 */
async function renderBrowsePage_internal(items, fadein = false, fadeout = false, title) {
    /** @type {Promise<{path: string, finished: boolean}>[]} */
    let result = [];

    // reset content
    const content = /** @type {HTMLElement} */ (document.getElementById("content"));

    content.classList.remove("fadein");
    content.classList.remove("fadeout");
    if (fadeout) {
        content.offsetHeight; // reflow
        content.classList.add("fadeout");
        await new Promise((resolve) => {
            content.addEventListener("animationend", resolve, { once: true });
        });
        content.classList.remove("fadeout");
    }

    content.innerHTML = "";

    // create list div
    const list = document.createElement("div");
    list.classList.add("list");

    let titleElement = document.createElement("p");
    titleElement.classList.add("list-title");
    titleElement.innerText = title;
    list.appendChild(titleElement);

    for (let ci = 0; ci < items.length; ci++) {
        let category = items[ci];
        let categoryElement = document.createElement("div");
        categoryElement.classList.add("list-category");
        list.appendChild(categoryElement);

        if (category.name) {
            let categoryNameElement = document.createElement("p");
            categoryNameElement.classList.add("list-category-name");
            categoryNameElement.innerText = category.name;
            categoryElement.appendChild(categoryNameElement);
        }

        for (let item of category.items) {
            let entryRow = document.createElement("div");
            entryRow.classList.add("list-entry");

            let entryElement = document.createElement("a");
            entryElement.classList.add("list-entry-btn");
            entryElement.classList.add("list-entry-main-button");
            entryRow.appendChild(entryElement);

            const iconElement = createSvgUseElement(item.icon);
            iconElement.classList.add("list-entry-icon");
            entryElement.appendChild(iconElement);

            // make the cursor snap
            entryElement.tabIndex = 0;

            const primaryInfoColumn = document.createElement("div");
            primaryInfoColumn.classList.add("list-entry-column");
            entryElement.appendChild(primaryInfoColumn);

            const nameElement = document.createElement("p");
            nameElement.classList.add("list-entry-name");
            nameElement.innerText = item.primaryText;
            primaryInfoColumn.appendChild(nameElement);

            if (item.secondaryText) {
                const secondaryTextElement = document.createElement("p");
                secondaryTextElement.classList.add("list-entry-secondary-text");
                secondaryTextElement.innerText = item.secondaryText;
                primaryInfoColumn.appendChild(secondaryTextElement);
            }

            if (item.endTextPrimary || item.endTextSecondary) {
                const endColumn = document.createElement("div");
                endColumn.classList.add("list-entry-column-end");
                entryElement.appendChild(endColumn);

                if (item.endTextPrimary) {
                    const sizeElement = document.createElement("p");
                    sizeElement.classList.add("list-entry-secondary-text");
                    sizeElement.innerText = item.endTextPrimary;
                    endColumn.appendChild(sizeElement);
                }

                if (item.endTextSecondary) {
                    const dateElement = document.createElement("p");
                    dateElement.classList.add("list-entry-secondary-text");
                    dateElement.innerText = item.endTextSecondary;
                    endColumn.appendChild(dateElement);
                }
            }

            result.push(new Promise((resolve, reject) => {
                entryElement.onclick = () => {
                    resolve(item.onclick());
                }
            }));


            if (item.secondaryButtons) {
                for (let i = 0; i < item.secondaryButtons.length; i++) {
                    const secondaryButton = document.createElement("a");
                    secondaryButton.classList.add("list-entry-secondary-button");
                    secondaryButton.classList.add("list-entry-btn");
                    secondaryButton.tabIndex = 0;

                    const iconElement = createSvgUseElement(item.secondaryButtons[i].icon);
                    iconElement.classList.add("list-entry-icon");
                    secondaryButton.appendChild(iconElement);

                    if (item.secondaryButtons[i].onclick) {
                        result.push(new Promise((resolve, reject) => {
                            secondaryButton.onclick = () => {
                                // @ts-ignore
                                resolve(item.secondaryButtons[i].onclick());
                            }
                        }));
                    }

                    entryRow.appendChild(secondaryButton);
                }

            }

            list.appendChild(entryRow);
        }

        // if theres another category, add a divider
        if (ci < items.length - 1) {
            const separator = document.createElement("div");
            separator.classList.add("list-category-separator");
            list.appendChild(separator);
        }

    }


    if (fadein) {
        content.offsetHeight;
        content.classList.add("fadein");
    }

    content.appendChild(list);

    return Promise.race(result);
}

/**
 * 
 * @param {string} path 
 * @returns {string}
 */
function getBackPath(path, rootPath = "") {
    if (path === "") {
        return "";
    }

    if (path.startsWith("/")) { // local path
        if (path.endsWith("/")) { // strip trailing slash
            path = path.substring(0, path.length - 1);
        }

        let lastSlashIndex = path.lastIndexOf("/"); // strip actual last part
        if (lastSlashIndex !== -1) {
            path = path.substring(0, lastSlashIndex + 1);
        }

        if (rootPath.length > path.length) { // we must have went back farther than the root, so return to ""
            path = "";
        }
        return path;
    }

    let rootPathUrlPathname = "";
    try {
        const url = new URL(rootPath);
        rootPathUrlPathname = url.pathname;
    } catch (error) { }

    try {
        const url = new URL(path);
        if (url.pathname === "/") {
            return "";
        }

        if (url.pathname.endsWith("/")) { // strip trailing slash
            url.pathname = url.pathname.substring(0, url.pathname.length - 1);
        }

        if (url.pathname.indexOf("/") !== -1) { // strip actual last part
            url.pathname = url.pathname.substring(0, url.pathname.lastIndexOf("/") + 1);
        }

        if (rootPathUrlPathname.length > url.pathname.length) { // we must have went back farther than the root, so return to ""
            return "";
        }

        return url.toString();
    } catch (e) { }

    return "";
}

function getNextPath(parentPath, entry, isDir) {
    parentPath = addTrailingSlashIfNeeded(parentPath);

    let result = parentPath + entry;
    if (isDir) {
        result = addTrailingSlashIfNeeded(result);
    }

    if (result.startsWith("/")) { // local path, were done
        return result;
    }

    try {
        // try to inject credentials from saved network locations if there isnt one set already
        const url = new URL(result);
        if (url.username) {
            return result;
        }

        // if no port, set to default
        if (url.protocol === SMB_PROTOCOL && !url.port) {
            url.port = SMB_PORT.toString();
        }

        const savedNetworkLocations = getSavedNetworkLocations();
        for (let i = 0; i < savedNetworkLocations.length; i++) {
            try {
                let savedLocation = new URL(savedNetworkLocations[i]);

                if (savedLocation.protocol !== url.protocol ||
                    savedLocation.hostname !== url.hostname) {
                    continue;
                }

                if (savedLocation.protocol === SMB_PROTOCOL && !savedLocation.port) {
                    savedLocation.port = SMB_PORT.toString();
                }

                // make sure ports match, if they didnt specify before, we set it to default (just for smb for now)
                if (savedLocation.port !== url.port) {
                    continue;
                }

                // we have the same host and protocol, for smb we need to check the share name too
                if (savedLocation.protocol === SMB_PROTOCOL && !url.pathname.startsWith(savedLocation.pathname)) {
                    continue;
                }

                // ok, we have a match, inject the credentials
                url.username = savedLocation.username;
                url.password = savedLocation.password;
                break;
            } catch (e) {
                // invalid url, skip
                continue;
            }
        }

        return url.toString();

    } catch (e) { }

    return result;
}

async function renderBrowsePageForPath(path, rootPath, fadein = false, fadeout = false, title = 'Select file...', pathType = 'file') {
    let data = await ApiClient.fsListDir(path);
    if ((data.status === HTTP_UNAUTHORIZED || data.status === HTTP_FORBIDDEN) && path.startsWith(SMB_SCHEME_PREFIX)) {
        // credentials are injected into the URL in getNextPath so if we reach this theres no saved entry for this (or there are multiple and the first one is now invalid but thats an edge case so meh)
        const shouldAuth = confirm("This share requires authentication. Do you want to enter credentials?");
        if (!shouldAuth) {
            return { path: getBackPath(path, rootPath), finished: false };
        }
        const username = prompt("Enter username");
        const password = prompt("Enter password");
        let requestedUrl = new URL(path);
        requestedUrl.username = username || "";
        requestedUrl.password = password || "";
        const authenticatedUrl = requestedUrl.toString();
        data = await ApiClient.fsListDir(authenticatedUrl);
        if (data.status !== HTTP_OK) {
            alert("Failed to authenticate to share: " + redactUrlPassword(path) + "\nError code: " + data.status);
            return { path: getBackPath(path, rootPath), finished: false };
        }

        path = authenticatedUrl;

        const shouldSave = confirm("Successfully authenticated. Do you want to save these credentials?");
        if (shouldSave) {
            addToSavedNetworkLocations(authenticatedUrl);
        }

        // we re-fetched the data and added the credentials to the path, go on like nothing happened
    }
    if (!data.data) {
        alert("Failed to load directory: " + redactUrlPassword(path) + "\nError code: " + data.status);
        return { path: "", finished: false };
    }

    /** @type {BrowsePageCategoryItem[]} */
    let items = new Array();

    if (pathType == 'dir') {
        items.push({
            primaryText: "[Select this directory]",
            secondaryText: "",
            endTextPrimary: "",
            endTextSecondary: "",
            icon: FOLDER_CHECK_ICON,
            onclick: () => {
                return { path: path, finished: true };
            }
        });
    }

    items.push({
        primaryText: "..",
        secondaryText: "",
        endTextPrimary: "",
        endTextSecondary: "",
        icon: BACK_ICON,
        onclick: () => {
            return { path: getBackPath(path, rootPath), finished: false };
        }
    });

    for (let i = 0; i < data.data.length; i++) {
        let dirListing = data.data[i];

        // if select dir hide files
        if (pathType == 'dir' && !dirListing.isDir()) {
            continue;
        }

        /** @type {BrowsePageCategoryItem} */
        items.push({
            primaryText: dirListing.name,
            secondaryText: dirListing.getHumanReadableMode(),
            endTextPrimary: dirListing.isDir() ? "" : dirListing.getHumanReadableSize(),
            endTextSecondary: dirListing.isDir() ? "" : dirListing.mtime.toLocaleString(),
            icon: dirListing.getIcon(),
            onclick: () => {
                if (dirListing.isDir()) {
                    return { path: getNextPath(path, dirListing.name, true), finished: false };
                } else {
                    return { path: getNextPath(path, dirListing.name, false), finished: true };
                }
            }
        });
    }

    /** @type {BrowsePageCategory[]} */
    let categories = [
        {
            name: redactUrlPassword(path),
            items: items
        }
    ];

    return renderBrowsePage_internal(categories, fadein, fadeout, title);
}

async function renderStorageDevicePicker(fadein = false, fadeout = false, title = 'Select Storage Device...', allowNetworkLocations = false, pathType = 'file') {
    /** @type {BrowsePageCategory[]} */
    let categories = [];

    let localStorageCategory = {
        name: "Local Storage",
        items: [
            {
                primaryText: "Console Storage",
                secondaryText: "/",
                icon: HDD_ICON,
                onclick: () => { return { path: "/", finished: pathType == 'dev' }; }
            }
        ]
    };

    const mnt = await ApiClient.fsListDir("/mnt/");
    if (!mnt.data) {
        throw new Error("Failed to list /mnt"); // /mnt should always exist 
    }
    // get items with mode m with prefix "ext" or "usb"
    for (let i = 0; i < mnt.data.length; i++) {
        let dirListing = mnt.data[i];
        if (dirListing.mode !== "m") {
            continue;
        }

        const isUsb = dirListing.name.startsWith("usb");
        const isExt = dirListing.name.startsWith("ext");

        if (!isUsb && !isExt) {
            continue;
        }

        const num = parseInt(dirListing.name.substring(3));
        if (isNaN(num)) {
            continue;
        }

        const icon = isUsb ? USB_ICON : HDD_ICON;
        let name = (isUsb ? "USB Storage " : "PS Formatted Storage ") + (num + 1).toString();
        if(dirListing.name == "ext0") {
	    name = 'USB Extended Storage';
	} else if(dirListing.name == "ext1") {
	    name = 'M.2 SSD Storage';
	}

        localStorageCategory.items.push({
            primaryText: name,
            secondaryText: `/mnt/${dirListing.name}`,
            icon: icon,
            onclick: () => { return { path: `/mnt/${dirListing.name}/`, finished: pathType == 'dev' }; }
        });
    }

    categories.push(localStorageCategory);

    if (allowNetworkLocations) {
        /** @type {BrowsePageCategory} */
        let networkLocationsCategory = {
            name: "Network Locations",
            items: []
        };

        const savedNetworkLocations = getSavedNetworkLocations();
        for (let i = 0; i < savedNetworkLocations.length; i++) {
            const location = savedNetworkLocations[i];
            networkLocationsCategory.items.push({
                primaryText: getFriendlySmbShareName(location), // TODO: make getFriendlySmbShareName generic
                secondaryText: redactUrlPassword(location),
                icon: SMB_SHARE_ICON,
                onclick: () => {
                    return { path: getNextPath(location, "", true), finished: pathType == 'dev' };
                },
                secondaryButtons: [
                    {
                        icon: EDIT_ICON,
                        onclick: () => {
                            const DEFAULT_TEXT = location;
                            const promptResult = prompt("Edit url:", DEFAULT_TEXT);
                            if (promptResult && promptResult !== DEFAULT_TEXT) {
                                removeFromSavedNetworkLocations(location);
                                addToSavedNetworkLocations(promptResult);
                                alert("Updated url has been saved.");
                            }
                            return { path: "", finished: false };  // refresh
                        }
                    },
                    {
                        icon: DELETE_ICON,
                        onclick: () => {
                            if (confirm(`Are you sure you want to remove this share?\n${redactUrlPassword(location)}`)) {
                                removeFromSavedNetworkLocations(location);
                            }
                            return { path: "", finished: false };  // refresh
                        }
                    }
                ]
            });
        }

        const discoveredLocations = await ApiClient.getMdnsDiscoveredLocations();
        for (let i = 0; i < discoveredLocations.length; i++) {
            const location = discoveredLocations[i];
            networkLocationsCategory.items.push({
                primaryText: `${location.hostname} (${location.address})`,
                secondaryText: protocolToFriendlyName(location.protocol),
                endTextPrimary: "mdns discovered",
                icon: LAN_ICON,
                onclick: () => {
                    return { path: getNextPath(`${SMB_SCHEME_PREFIX}${location.address}:${location.port}`, "", true), finished: false };
                }
            });
        }

        const addNetworkLocationButton = {
            primaryText: "Add custom share",
            icon: ADD_ICON,
            onclick: () => {
                const DEFAULT_TEXT = SMB_SCHEME_PREFIX;
                const promptResult = prompt("Enter the path to the share:\nE.g.: `smb://username:password@192.168.1.2/share1`", DEFAULT_TEXT);
                // for some .. reason the back button acts as if the user pressed OK
                // so lets double check the user actually want this
                if (promptResult && promptResult !== DEFAULT_TEXT &&
                    confirm(`Are you sure you want to add this share?\n${promptResult}`)) {
                    addToSavedNetworkLocations(promptResult);
                }

                return { path: "", finished: false };  // refresh
            }
        };
        networkLocationsCategory.items.push(addNetworkLocationButton);
        categories.push(networkLocationsCategory);
    }


    return renderBrowsePage_internal(categories, fadein, fadeout, title);
}
