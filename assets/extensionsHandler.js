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

const EXTENSION_SANDBOX_EXECUTE_ACTION = "EXTENSION_SANDBOX_EXECUTE_ACTION";

const EXTENSION_SANDBOX_EXECUTE_RESPONSE = "EXTENSION_SANDBOX_EXECUTE_RESPONSE";
// @ts-ignore (demo hb.js is in a diff context)
const EXTENSION_API_PARENT_SHOW_CAROUSEL = "EXTENSION_API_PARENT_SHOW_CAROUSEL";
// @ts-ignore (demo hb.js is in a diff context)
const EXTENSION_API_PARENT_FILE_BROWSER = "EXTENSION_API_PARENT_FILE_BROWSER";

const EXTENSION_MAIN_TIMEOUT_MS = 5000;

/**
 * @typedef {Object} ExtensionInfo
 * @property {string} uuid - reference the sandbox iframe by this id 
 * @property {CarouselItem} result
 */

/**
 * @typedef {Object} ExtensionSandboxExecResult
 * @property {string} uuid
 * @property {boolean?} result
 */


/**
 * @param {string} extensionPath
 * @returns {Promise<ExtensionInfo?>} extensionId and result of main(), null if main() timed out, if null sandbox is deleted
 */
async function loadJsExtension(extensionPath) {
    // load js as raw text
    let hbJs = await ApiClient.fsGetFileText(extensionPath);
    if (!hbJs.data) {
        // throw new Error("Failed to load extension: " + extensionPath);
        removeFromHomebrewList(extensionPath);
        await Globals.Router.handleHome();
        return null;
    }

    if (!Globals.hbApiJs) {
        let request2 = await fetch("homebrewApi.js");
        if (!request2.ok) {
            throw new Error("Failed to load extension api js");
        }
        Globals.hbApiJs = await request2.text();
    }

    if (!Globals.ApiClientJs) {
        let request3 = await fetch("apiClient.js");
        if (!request3.ok) {
            throw new Error("Failed to load api client js for extension");
        }
        Globals.ApiClientJs = await request3.text();
    }

    // create element to hold iframes if there isnt one already
    let sandbox = document.getElementById("js-extension-sandbox");

    if (!sandbox) {
        sandbox = document.createElement("div");
        sandbox.style.display = "none";
        sandbox.id = "js-extension-sandbox";
        document.body.appendChild(sandbox);
    }

    let extensionId = uuidv4();

    const iframeContent = `
        <!DOCTYPE html>
        <html lang="en">

        <head>
            <meta charset="UTF-8">
        </head>

        <body>
            <script>
                window.extensionId = "${extensionId}";
                window.workingDir = "${extensionPath.substring(0, extensionPath.lastIndexOf("/"))}";
                ${Globals.hbApiJs}
                ${Globals.ApiClientJs}
                ${hbJs.data}
                // register message listener to execute code here in the iframe sandbox
                window.addEventListener("message", async (event) => {
                    if (event.data.action !== "${EXTENSION_SANDBOX_EXECUTE_ACTION}") {
                        return;
                    }

                    let res = await window.Funcs[event.data.funcId]();
                    window.parent.postMessage({ extensionId: window.extensionId, action: "${EXTENSION_SANDBOX_EXECUTE_RESPONSE}", result: encodeURIComponent(JSON.stringify(res)), resultId: event.data.resultId }, "*");
                });
                window.addEventListener("error", (event) => { alert(event.error); })
                window.addEventListener("unhandledrejection", (event) => { alert(event.reason); });
                // call main and return result
                (async () => {
                    const result = await main();
                    window.parent.postMessage({ extensionId: "${extensionId}", result: encodeURIComponent(JSON.stringify(remapFunctionsToFunctionIds(result))) }, "*");
                })();
            </script>
        </body>

        </html>
    `;

    const blob = new Blob([iframeContent], { type: "text/html" });
    const url = URL.createObjectURL(blob);
    
    const iframe = document.createElement("iframe");
    iframe.id = extensionId;
    // @ts-ignore
    iframe.sandbox = "allow-scripts allow-modals";
    iframe.style.display = "none";

    let result = new Promise((resolve, reject) => {
        let timeout = null; 
        let handler = function(event) {
            if (event.data.extensionId === extensionId) {
                clearTimeout(timeout);
                window.removeEventListener("message", handler);
                resolve({ uuid: extensionId, result: remapFunctionIdsToSandboxedProxyCalls(JSON.parse(decodeURIComponent(event.data.result)),extensionId) });
            }
        }

        timeout = setTimeout(() => {
            window.removeEventListener("message", handler);
            sandbox.removeChild(iframe);
            resolve(null);
        }, EXTENSION_MAIN_TIMEOUT_MS);

        window.addEventListener("message", handler);
    });

    iframe.src = url;
    
    sandbox.appendChild(iframe);

    return result;
}

/**
 * @param {string} extensionId
 * @param {string} funcId
 * @returns {Promise<ExtensionSandboxExecResult>?}
 */
function executeInExtensionSandbox(extensionId, funcId) {
    const iframe = /** @type {HTMLIFrameElement} */ (document.getElementById(extensionId));
    if (!iframe || !iframe.contentWindow) {
        throw new Error("extension sandbox iframe not found");
    }

    let resultId = uuidv4();

    iframe.contentWindow.postMessage({ action: EXTENSION_SANDBOX_EXECUTE_ACTION, funcId: funcId, resultId: resultId }, "*");

    return new Promise((resolve, reject) => {
        window.addEventListener("message", function handler(event) {
            if (event.data.extensionId === extensionId && event.data.action === EXTENSION_SANDBOX_EXECUTE_RESPONSE && event.data.resultId === resultId) {
                window.removeEventListener("message", handler);
                let decodedResult = decodeURIComponent(event.data.result);
                if (decodedResult == "null" || decodedResult == "undefined") {
                    resolve({ uuid: extensionId, result: null });
                    return;
                }
                resolve({ uuid: extensionId, result: JSON.parse(decodedResult) });
            }
        });
    });
}

/** Reverse what remapFunctionsToFunctionIds of the hb api does */
function remapFunctionIdsToSandboxedProxyCalls(obj, extensionId) {
    if (typeof obj !== "object" || obj === null) {
        return obj;
    }

    // if array, iterate over its elements recursively
    if (Array.isArray(obj)) {
        return obj.map(item => remapFunctionIdsToSandboxedProxyCalls(item, extensionId));
    }

    return Object.keys(obj).reduce((acc, key) => {
        if (key === "onclick") {
            acc[key] = async () => {
                let res = await executeInExtensionSandbox(extensionId, obj[key]);
                if (res) {
                    return res.result;
                }
                return null;
            }
        } else if (typeof obj[key] === "object" && obj[key] !== null) {
            // Recursively serialize nested objects
            acc[key] = remapFunctionIdsToSandboxedProxyCalls(obj[key], extensionId);
        } else {
            acc[key] = obj[key];
        }
        return acc;
    }, {});
}


/**
 * Only call once at load
 */
function registerExtensionMessagesListener() {
    window.addEventListener("message", async function handler(event) {
        // check if message has an extension uuid and extension is loaded
        if (!event.data.extensionId || !this.document.getElementById(event.data.extensionId)) {
            return;
        }

        if (event.data.action === EXTENSION_API_PARENT_FILE_BROWSER) {
            let sandbox = /** @type {HTMLIFrameElement?} */ (document.getElementById(event.data.extensionId));
            if (!sandbox || !sandbox.contentWindow) {
                return;
            }
            let result = await Globals.Router.pickFile(decodeURIComponent(event.data.initialPath), decodeURIComponent(event.data.title), decodeURIComponent(event.data.allowNetworkLocations) === "true");

            sandbox.contentWindow.postMessage({ extensionId: event.data.extensionId, callback: event.data.callback, result: result }, "*");
        } else if (event.data.action === EXTENSION_API_PARENT_SHOW_CAROUSEL) {
            let temp_items = JSON.parse(decodeURIComponent(event.data.items));
            // these items should already be almost the same as what the carousel expects
            // the only difference is the the onclicks where replaced by function ids
            /**
             * @type {CarouselItem[]}
             */
            let items = remapFunctionIdsToSandboxedProxyCalls(temp_items, event.data.extensionId);
            await Globals.Router.handleCarousel(items);
        }
    });
}
