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


// the code here is what gets injected before the extension.js
// these are things the extension can use to interact with the frontend

const EXTENSION_API_PARENT_SHOW_CAROUSEL = "EXTENSION_API_PARENT_SHOW_CAROUSEL";
const EXTENSION_API_PARENT_FILE_BROWSER = "EXTENSION_API_PARENT_FILE_BROWSER";

// https://stackoverflow.com/questions/105034/how-do-i-create-a-guid-uuid
function uuidv4() {
    return "10000000-1000-4000-8000-100000000000".replace(/[018]/g, c =>
        (+c ^ crypto.getRandomValues(new Uint8Array(1))[0] & 15 >> +c / 4).toString(16)
    );
}

function remapFunctionsToFunctionIds(obj) {
    if (typeof obj !== "object" || obj === null) {
        return obj;
    }

    // if array, iterate over its elements recursively
    if (Array.isArray(obj)) {
        return obj.map(item => remapFunctionsToFunctionIds(item));
    }

    return Object.keys(obj).reduce((acc, key) => {
        // return a uuid instead of the function and save the the function here in the sandbox so its state is saved and we can easily call it
        if (typeof obj[key] === "function") {
            let uuid = uuidv4();
            if (!window.Funcs) {
                window.Funcs = {};
            }
            window.Funcs[uuid] = obj[key];
            acc[key] = uuid;
        } else if (typeof obj[key] === "object" && obj[key] !== null) {
            // Recursively serialize nested objects
            acc[key] = remapFunctionsToFunctionIds(obj[key]);
        } else {
            acc[key] = obj[key];
        }
        return acc;
    }, {});
}

// public functions below, apiClient.js also will be injected

function showCarousel(items) {
    window.parent.postMessage({ extensionId: window.extensionId, action: EXTENSION_API_PARENT_SHOW_CAROUSEL, items: encodeURIComponent(JSON.stringify(remapFunctionsToFunctionIds(items))) }, "*");
}

function pickFile(initialPath, title = 'Select file...') {
    const uuid = uuidv4();
    window.parent.postMessage({ extensionId: window.extensionId, action: EXTENSION_API_PARENT_FILE_BROWSER, callback: uuid, initialPath: encodeURIComponent(initialPath), title:  encodeURIComponent(title)}, "*");

    // wait for response
    return new Promise((resolve, reject) => {
        window.addEventListener("message", function handler(event) {
            if (event.data.callback !== uuid) {
                return;
            }
            
            window.removeEventListener("message", handler);
            resolve(event.data.result);
        });
    });
}
