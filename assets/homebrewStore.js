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
const HOMEBREW_AUTO_SCAN_PATHS = ["/data/homebrew/", "/mnt/usb0/homebrew/",
				  "/mnt/usb1/homebrew/", "/mnt/usb2/homebrew/",
				  "/mnt/usb3/homebrew/", "/mnt/usb4/homebrew/",
				  "/mnt/usb5/homebrew/", "/mnt/usb6/homebrew/",
				  "/mnt/ext0/homebrew/", "/mnt/ext1/homebrew/"];
// order of this matters, if both homebrew.js and eboot.elf exist, homebrew.js will be used
const HOMEBREW_AUTO_SCAN_ALLOWED_EXEC_NAMES = [ "homebrew.js", "eboot.elf" ];

const LOCALSTORE_HOMEBREW_LIST_KEY = "LOCALSTORE_HOMEBREW_LIST";

/**
 * @typedef {Object} HomebrewEntry
 * @property {string} path
 * @property {string} dir
 * @property {string} filename
 */

/**
 * If item already exists it puts the item at the start
 * @param {string} path - Full absolute path with filename 
 */
function addToHomebrewStore(path) {
	let hbList = [];
    let temp_hblistStr = localStorage.getItem(LOCALSTORE_HOMEBREW_LIST_KEY);
    if (temp_hblistStr) {
        hbList = JSON.parse(temp_hblistStr);
        // if theres already an entry with the same path, remove it
        hbList = hbList.filter(entry => entry !== path);
    }

    // Add to start
	hbList.unshift(
		path
	);

	localStorage.setItem(LOCALSTORE_HOMEBREW_LIST_KEY, JSON.stringify(hbList));
}

/**
 * @param {string} path - Full absolute path with filename
 */
function removeFromHomebrewList(path) {
    let hbList = [];

    let temp_hblistStr = localStorage.getItem(LOCALSTORE_HOMEBREW_LIST_KEY);
    if (!temp_hblistStr) {
        return;
    }

    hbList = JSON.parse(temp_hblistStr);

    // if theres already an entry with the same path, remove it
    hbList = hbList.filter(entry => entry !== path);

    localStorage.setItem(LOCALSTORE_HOMEBREW_LIST_KEY, JSON.stringify(hbList));
}

/**
 * @returns {HomebrewEntry[]}
 */
function getHomebrewList() {
    let temp_hblistStr = localStorage.getItem(LOCALSTORE_HOMEBREW_LIST_KEY);
    if (!temp_hblistStr) {
        return [];
    }

    return JSON.parse(temp_hblistStr).reduce((acc, entry) => {
        acc.push({
            path: entry,
            dir: entry.substring(0, entry.lastIndexOf("/")),
            filename: entry.substring(entry.lastIndexOf("/") + 1)
        });
        return acc;
    }, []).sort((x, y) => {
	return x.dir.localeCompare(y.dir);
    });
}

// TODO: this is currently unused
async function removeNonexistentHomebrews() {
    let hbs = getHomebrewList();

	// group by path to avoid making a request for every entry in case theres multiple in the same folder
	let pathGroups = {};

	hbs.forEach(entry => {
		if (!pathGroups[entry.dir]) {
			pathGroups[entry.dir] = [];
		}
		pathGroups[entry.dir].push(entry);
	});

	// loop though groups
	Object.keys(pathGroups).forEach(async (key) => {
		let json = await ApiClient.fsListDir(key);

		pathGroups[key].forEach(entry => {
			if (json === null) {
				removeFromHomebrewList(entry.path);
				return;
			}

			json.forEach(jsonDirent => {
				if (jsonDirent.name === entry.filename) {
					return;
				}
			});

			removeFromHomebrewList(entry.path);
		});
	});
}

async function scanHomebrews() {
    for (let path of HOMEBREW_AUTO_SCAN_PATHS) {
        let autoScanParentDirEntryList = await ApiClient.fsListDir(path);
        if (autoScanParentDirEntryList === null) {
            continue;
        }

        for (let entry of autoScanParentDirEntryList) {
            let hbDirEntries = await ApiClient.fsListDir(path + entry.name);
            if (hbDirEntries === null) {
                continue;
            }

            let foundExecutableName = null;
            for (let hbDirEntry of hbDirEntries) {
                for (let allowedName of HOMEBREW_AUTO_SCAN_ALLOWED_EXEC_NAMES) {
                    if (hbDirEntry.name.toLowerCase() === allowedName.toLowerCase()) {
                        foundExecutableName = allowedName;
                        break;
                    }
                }

                if (foundExecutableName) {
                    break;
                }
            }

            if (foundExecutableName) {
                addToHomebrewStore(path + entry.name + "/" + foundExecutableName);
            }
        }
    }
}
