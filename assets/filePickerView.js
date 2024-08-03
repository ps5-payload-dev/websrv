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
 * @param {string} path 
 * @returns {Promise<{path: string, finished: boolean}>}
 */
async function renderBrowsePage(path, fadein = false, title = 'Select file...') {
    /** @type {Promise<{path: string, finished: boolean}>[]} */
    let result = [];

    let data = null;

    if (!path.endsWith("/")) {
        path += "/";
    }

    data = await ApiClient.fsListDir(path);

    if (!data) {
        throw new Error("Failed to load directory: " + path);
    }

    // reset content
    const content = /** @type {HTMLElement} */ (document.getElementById("content"));
    content.innerHTML = "";

    // create list div
    const list = document.createElement("div");
    list.classList.add("list");

    let titleElement = document.createElement("div");
    titleElement.classList.add("list-title");
    titleElement.innerText = title;
    list.appendChild(titleElement);

    // add back button if needed
    if (path !== "/") {
        let entryElement = document.createElement("a");
        entryElement.classList.add("list-entry");

        entryElement.tabIndex = 0;

        result.push(new Promise((resolve, reject) => {
            entryElement.onclick = () => {
                let backPath = path.substring(0, path.lastIndexOf("/"));
                backPath = backPath.substring(0, backPath.lastIndexOf("/") + 1);
                resolve({ path: backPath, finished: false });
            }
        }));


        const backIcon = document.createElement("div");
        backIcon.classList.add("list-entry-icon");
        backIcon.classList.add("back-icon");

        entryElement.appendChild(backIcon);

        const nameElement = document.createElement("p");
        nameElement.classList.add("list-entry-name");
        nameElement.innerText = "..";
        entryElement.appendChild(nameElement);

        list.appendChild(entryElement);
    }


    // add entries
    data.forEach(entry => {
        let entryElement = document.createElement("a");
        entryElement.classList.add("list-entry");
        if (entry.isDir()) {
            entryElement.classList.add("list-entry-directory");
            // add dir icon
            const dirIcon = document.createElement("div");
            dirIcon.classList.add("list-entry-icon");
            dirIcon.classList.add("dir-icon");

            entryElement.appendChild(dirIcon);
        } else {
            entryElement.classList.add("list-entry-file");
            // add file icon
            const fileIcon = document.createElement("div");
            fileIcon.classList.add("list-entry-icon");
            fileIcon.classList.add("file-icon");

            entryElement.appendChild(fileIcon);
        }

        // make the cursor snap
        entryElement.tabIndex = 0;

        const nameElement = document.createElement("p");
        nameElement.classList.add("list-entry-name");
        nameElement.innerText = entry.name;
        entryElement.appendChild(nameElement);

        result.push(new Promise((resolve, reject) => {
            entryElement.onclick = () => {
                if (entry.isDir()) {
                    resolve({ path: path + entry.name + "/", finished: false });
                } else {
                    resolve({ path: path + entry.name, finished: true });
                }
            }
        }));

        list.appendChild(entryElement);
    });

    if (fadein) {
        content.classList.add("fadein");
    }

    content.appendChild(list);

    return Promise.race(result);
}