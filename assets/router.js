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
 * these paths are only used for internally, navigating to these manually wont resolve them, 
 * since most of these require non serializable parameters (function execution context) 
 * @private
*/
const RouterRoutes = [
    {
        path: "/",
        keepInHistory: true,
        replaceSamePath: true,
        forceReplaceCurrentState: true
    },
    {
        path: "/modal",
        keepInHistory: false
    },
    {
        path: "/filePicker",
        keepInHistory: false
    },
    {
        path: "/carousel",
        keepInHistory: true
    },
    {
        path: "/launchedAppView",
        keepInHistory: true,
        replaceSamePath: true,
        forceReplaceCurrentState: true
    },
];

class Router {
    /**
     * @typedef {Object} State
     * @property {string} path
     * @property {string} dataRouterId
     */

    /**
     * @typedef {Object} Route
     * @property {string} path
     * @property {function():any} handler
     * @property {boolean} keepInHistory
     */

    constructor() {
        /** @type {Semaphore} */
        this.navigationLock = new Semaphore(1);

        /** @type {boolean} */
        this.disablePopHandler = false;

        // save history length on first load (which in first load can only contain backward history)
        if (!sessionStorage.getItem("initialHistoryLength")) {
            sessionStorage.setItem("initialHistoryLength", window.history.length.toString());
        }
        this.initialHistoryLength = parseInt(/** @type {string} */(sessionStorage.getItem("initialHistoryLength")));

        // if the user reloads with history from old session, delete it
        // for it to be reliable first we need to get rid of the forwards history
        window.history.pushState({}, "", `#${uuidv4()}`);

        // the popstate after back fires twice if we do history.go(-) even though im removing the handler before it?
        // i guess removing the event listener is async?
        let handledInitialBack = false;

        // sadly history.back doesnt update instantly on webkit...
        window.addEventListener("popstate", function ctorPart2() {
            if (handledInitialBack) {
                window.removeEventListener("popstate", ctorPart2);
                return;
            }
            handledInitialBack = true;

            window.removeEventListener("popstate", ctorPart2);

            // we know there must be exactly one forward history, so subtract that
            if (this.initialHistoryLength != (window.history.length - 1)) {
                let goBackCount = -((window.history.length - 1) - this.initialHistoryLength);
                window.history.go(goBackCount);
            }

            window.addEventListener("popstate", this.handlePopState.bind(this));
            this.handleHome();
        }.bind(this));

        window.history.back();
    }

    async pushOrReplaceState(newPath) {
        await this.navigationLock.acquire();

        try {
            const oldPathItem = RouterRoutes.find((r) => r.path === this.getPath());
            const newPathItem = RouterRoutes.find((r) => r.path === newPath);

            let shouldReplace = oldPathItem ? !oldPathItem.keepInHistory : false;
            let shouldReplaceSamePath = oldPathItem ? oldPathItem.replaceSamePath : false;
            if (shouldReplaceSamePath && newPath === this.getPath()) {
                shouldReplace = true;
            }

            if (newPathItem && newPathItem.forceReplaceCurrentState) {
                shouldReplace = true;
            }

            /** @type {State} */
            const state = {
                path: newPath,
                dataRouterId: uuidv4()
            };

            // add hash if not root
            if (newPath !== "/") {
                newPath = `#${newPath}`;
            }

            const content = document.getElementById("content");
            if (content) {
                content.classList.add("fadeout");
                await sleep(250);
            }

            if (content && !shouldReplace) {
                // hide current content
                content.classList.remove("fadein");
                content.removeAttribute("id");
                content.style.display = "none";
                content.classList.remove("fadeout");
            } else if (content && shouldReplace) {
                // delete current content
                content.remove();
            }

            // create new content with data-router-id
            const newContent = document.createElement("div");
            newContent.id = "content"
            newContent.setAttribute("data-router-id", state.dataRouterId);
            newContent.setAttribute("data-router-path", encodeURIComponent(state.path));
            newContent.style.display = "block";
            document.body.appendChild(newContent);


            if (shouldReplace) {
                history.replaceState(state, "", newPath);
            } else {
                history.pushState(state, "", newPath);
            }

            closeModal();
        } finally {
            await this.navigationLock.release();
        }

    }

    async handlePopState(event) {
        if (this.disablePopHandler) {
            return;
        }

        await this.navigationLock.acquire();

        try {
            window.dispatchEvent(new CustomEvent("popstateHandlerEntered"));
            /** @type {State} */
            const state = event.state;

            if (!state) {
                await this.handleFallbackHome();
                return;
            }

            // delete current content div since we dont care about forwards history
            const currentContent = document.getElementById("content");
            if (currentContent) {
                currentContent.classList.add("fadeout");
                await sleep(250);
                // keep a copy of index
                if (currentContent.getAttribute("data-router-path") === encodeURIComponent("/")) {
                    currentContent.removeAttribute("id");
                    currentContent.style.display = "none";
                    currentContent.classList.remove("fadeout");
                }
                else {
                    currentContent.remove();
                }
            }

            // restore saved content div
            /** @type {HTMLElement?} */
            const restoredContent = document.querySelector(`[data-router-id="${state.dataRouterId}"]`);
            if (!restoredContent) {
                await this.handleFallbackHome();
                return;
            }

            restoredContent.id = "content";
            restoredContent.classList.add("fadein");
            setTimeout(() => {
                restoredContent.classList.remove("fadein");
            }, 250);
            restoredContent.style.display = "block";

            closeModal();
        } finally {
            await this.navigationLock.release();
        }
    }

    getNavigateAwayPromise() {
        return new Promise((resolve) => {
            function handler(event) {
                window.removeEventListener("popstate", handler);
                window.removeEventListener("hashchange", handler);
                resolve(null);
            }

            window.addEventListener("popstate", handler);
            window.addEventListener("hashchange", handler);
        });
    }

    async handleHome() {
        await this.pushOrReplaceState("/");
        await renderHomePage();
    }

    async handleFallbackHome() {
        let homeContent = /** @type {HTMLElement?} */ (document.querySelector(`[data-router-path="${encodeURIComponent("/")}"]`));
        // delete current content
        const currentContent = document.getElementById("content");
        if (currentContent && (!homeContent || homeContent.id !== "content")) {
            // currentContent.classList.add("fadeout");
            // await sleep(250);
            currentContent.remove();
        }

        let contentUuid = uuidv4();

        if (!homeContent) {
            homeContent = document.createElement("div");
            homeContent.id = "content";
            homeContent.setAttribute("data-router-path", encodeURIComponent("/"));
            homeContent.setAttribute("data-router-id", contentUuid);
            homeContent.style.display = "block";
            document.body.appendChild(homeContent);

            renderHomePage(); // fire and forget
        }
        else {
            homeContent.id = "content";
            homeContent.style.display = "block";
            contentUuid = /** @type {string} */ (homeContent.getAttribute("data-router-id"));
        }

        history.replaceState({ path: "/", dataRouterId: contentUuid }, "", `/`);
    }

    /**
     * @param {CarouselItem[]} items
     */
    async handleCarousel(items) {
        await this.pushOrReplaceState("/carousel");
        await renderMainContentCarousel(items, true);
    }

    /**
     * @param {ModalItem[]} items
     */
    async handleModal(items) {
        await this.pushOrReplaceState("/modal");
        renderModalOverlay(items);
    }

    /**
     * Replaces the initial page so you can exit with the back button
     * so you cant exit this, reload page to close this view
     * @param {ReadableStream?} [logStream] 
     */
    async handleLaunchedAppView(logStream = null) {
        // TODO: theres a workaround for the back button to close the window even if there was history before this site
        // you can listen to keycode 27, and navigate back as far as possible there

        this.disablePopHandler = true;
        // remove forward history
        window.history.pushState({}, "", `#${uuidv4()}`);

        await new Promise(async (resolve) => {
            function handler(event) {
                window.removeEventListener("popstate", handler);
                resolve(null);
            }
            window.addEventListener("popstate", handler);

            window.history.back();
        });

        if (this.initialHistoryLength != (window.history.length - 1)) {
            let goBackCount = -((window.history.length - 1) - this.initialHistoryLength);
            window.history.go(goBackCount);
        }

        await this.pushOrReplaceState("/launchedAppView");
        await renderLaunchedAppView(logStream);

        // this.disablePopHandler = false;
    }

    createBlockingBackPressPromise() {
        async function nonLazyHistoryBack() { // without this stuff breaks, history.back() is lazy
            const backDone = new Promise((resolve) => {
                function handler(event) {
                    window.removeEventListener("popstate", handler);
                    resolve(null);
                }
                window.addEventListener("popstate", handler);
            });
            window.history.back();
            await backDone;
        }

        this.disablePopHandler = true;
        window.history.pushState({}, "", `#${uuidv4()}`); // dummy
        let handler = null;
        let resolveFn = null;
        let hasCleanedUp = false;
        const promise = new Promise((resolve) => {
            resolveFn = resolve;
            handler = async (event) => {
                window.removeEventListener("popstate", handler);
                this.disablePopHandler = false;
                hasCleanedUp = true;
                resolve(null);
            };
            window.addEventListener("popstate", handler);
        });

        const cancel = async () => {
            if (hasCleanedUp) {
                return;
            }
            window.removeEventListener("popstate", handler);
            await nonLazyHistoryBack();
            this.disablePopHandler = false;
            hasCleanedUp = true;
            resolveFn(null);
        }
        return { promise, cancel };
    }


    /**
     * @param {string} initialPath 
     * @returns {Promise<string?>}
     */
    async pickPath(initialPath = "", title = "Select file...", allowNetworkLocations = false, pathType = 'file') {
        await this.pushOrReplaceState("/filePicker");

        /** @type {{path: string, finished: boolean}?} */
        let lastPath = { path: initialPath, finished: false };
        let isFirstRender = true; // dont run fadeout the first time, pushOrReplaceState already did it
        let rootPath = "";
        do {
            let backButtonPressPromise = this.createBlockingBackPressPromise();
            // if path is empty then render the device selector
            let newPath = null;
            if (lastPath.path === "") {
                newPath = await Promise.race([renderStorageDevicePicker(true, !isFirstRender, title, allowNetworkLocations, pathType), backButtonPressPromise.promise]);
                rootPath = newPath ? newPath.path : "";
            } else {
                newPath = await Promise.race([renderBrowsePageForPath(lastPath.path, rootPath, true, !isFirstRender, title, pathType), backButtonPressPromise.promise]);
            }

            await backButtonPressPromise.cancel();

            if (newPath === null) { // user pressed back
                if (lastPath.path === "") { // exit
                    newPath = null;
                } else { // try to go back
                    newPath = { path: getBackPath(lastPath.path, rootPath), finished: false };
                }
            }

            lastPath = newPath;

            isFirstRender = false;
        } while (lastPath !== null && !lastPath.finished);

        await this.back();

        return lastPath ? lastPath.path : null;
    }

    /**
     * @param {string} initialPath 
     * @returns {Promise<string?>}
     */
    async pickFile(initialPath = "", title = "Select file...", allowNetworkLocations = false) {
        return this.pickPath(initialPath, title, allowNetworkLocations, 'file');
    }

    /**
     * @param {string} initialPath 
     * @returns {Promise<string?>}
     */
    async pickDirectory(initialPath = "", title = "Select directory...", allowNetworkLocations = false) {
        return this.pickPath(initialPath, title, allowNetworkLocations, 'dir');
    }

    /**
     * @param {string} initialPath 
     * @returns {Promise<string?>}
     */
    async pickDevice(title = "Select device...", allowNetworkLocations = false) {
        return this.pickPath("", title, allowNetworkLocations, 'dev');
    }

    getPath() {
        const hash = window.location.hash;
        if (!hash) {
            return "/";
        }

        return hash.slice(1);
    }

    async back() {
        // history.back doesnt update the history.length and window.location.href instantly
        // plus we await the popstate handler for the animations to finish

        await new Promise(async (resolve) => {
            function handler(event) {
                window.removeEventListener("popstateHandlerEntered", handler);
                // window.removeEventListener("hashchange", handler);
                resolve(null);
            }
            window.addEventListener("popstateHandlerEntered", handler);
            // window.addEventListener("hashchange", handler);

            window.history.back();
        });

        await this.navigationLock.awaitCurrentQueue();
    }

}
