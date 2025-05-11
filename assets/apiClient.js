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


// this file also gets injected into extension sandbox

// @ts-check

class DirectoryListing {
    /**
     * @param {string} name
     * @param {string} mode
     * @param {number} [unixTimestamp]
     * @param {number} [sizeBytes]
    */
    constructor(name, mode, unixTimestamp = 0, sizeBytes = 0) {
        this.name = name;
        this.mode = mode;
        this.mtime = new Date(unixTimestamp * 1000);
        this.size = sizeBytes;
    }

    static isDir(instance) {
        return (instance.mode === "d" || instance.mode === "m" || instance.mode === "smb-share") && !ignoredFileNames.includes(instance.name);
    }

    isDir() {
        return DirectoryListing.isDir(this);
    }

    isFile() {
        return this.mode === "-" && !ignoredFileNames.includes(this.name);
    }

    getIcon() {
        if (this.mode === "m" || this.mode === "d") {
            return DIR_ICON;
        } else if (this.mode === "smb-share") {
            return SMB_SHARE_ICON;
        } else {
            return FILE_ICON;
        }
    }

    getHumanReadableMode() {
        // https://github.com/ps5-payload-dev/sdk/blob/17b1d592d4aa095f6cc52e33c8a26fd6f898d65c/include/freebsd/sys/stat.h#L237
        // if (this.mode === "m") {
        //     return "Device mountpoint";
        // } else if (this.mode === "d") {
        //     return "Directory";
        // } else 
        if (this.mode === "b") {
            return "Block device";
        } else if (this.mode === "c") {
            return "Char device";
        } else if (this.mode === "l") {
            return "Symbolic link";
        } else if (this.mode === "p") {
            return "Fifo or socket";
        } else if (this.mode === "s") {
            return "Socket";
        } else {
            return ""; // file, dir, device mount or unknown
        }
    }

    getHumanReadableSize() {
        if (this.size < 1024) {
            return this.size + " B";
        } else if (this.size < 1024 * 1024) {
            return (this.size / 1024).toFixed(2) + " KB";
        } else if (this.size < 1024 * 1024 * 1024) {
            return (this.size / (1024 * 1024)).toFixed(2) + " MB";
        } else {
            return (this.size / (1024 * 1024 * 1024)).toFixed(2) + " GB";
        }
    }
}

const HTTP_OK = 200;
const HTTP_ERROR_BAD_REQUEST = 400;
const HTTP_UNAUTHORIZED = 401;
const HTTP_FORBIDDEN = 403;
const HTTP_INTERNAL_SERVER_ERROR = 500;

const SMB_PORT = 445;
const SMB_PROTOCOL = "smb:";
const SMB_SCHEME_PREFIX = `${SMB_PROTOCOL}//`;

/**
 * @template T
 * @typedef {Object} APIResponse
 * @property {number} status
 * @property {T} data
 */
 

// @ts-ignore
const baseURL = window.location.origin == "null" ? "http://127.0.0.1:8080" : window.location.origin;
const ignoredFileNames = [".", ".."];
class ApiClient {
    /**
     * @param {string} path
     * @param {(string | string[])?} [args]
     * @param {(string | string[])?} [env]
     * @param {string?} [cwd]
     * @param {boolean} [daemon]
     * @returns {Promise<APIResponse<ReadableStream<Uint8Array>?>>}
     */
    static async launchApp(path, args = null, env = null, cwd = null, daemon = false) {
        let params = new URLSearchParams({
            "pipe": "1",
            "daemon": new Number(daemon).toString(),
            "path": path
        });

        if (typeof args === "string") {
            // @ts-ignore
            params.append("args", args.replace(/ /g, "\\ "));
        } else if (Array.isArray(args)) {
            // @ts-ignore
            params.append("args", args.map(arg => arg.replace(/ /g, "\\ ")).join(" "));
        }

        if (env != null) {
            // @ts-ignore
            params.append("env", Object.entries(env).map(([key, val]) =>
                `${key}=${val}`.replace(/ /g, "\\ ")
            ).join(" "));
        }

        if (cwd != null) {
            params.append("cwd", cwd);
        }

        let uri = baseURL + "/hbldr?" + params.toString();

        let response = await fetch(uri);
        if (!response.ok) {
            throw new Error("Failed to launch app, status code: " + response.status); // TODO: throw the error at the consumer
            return { status: response.status, data: null };
        }

        return {
            status: response.status,
            data: response.body
        };
    }

    /**
     * @param {string} path
     * @returns {Promise<APIResponse<DirectoryListing[]?>>}
     */
    static async fsListInternalDir(path) {
        if (!path.endsWith("/")) {
            path += "/";
        }

        let response = await fetch(baseURL + "/fs" + path + "?fmt=json");
        if (!response.ok) {
            return { status: response.status, data: null };
        }
        let data = await response.json();
        data = data.filter(entry => !ignoredFileNames.includes(entry.name));
        data.sort((x, y) => (DirectoryListing.isDir(x) == DirectoryListing.isDir(y)) ?
            x.name.localeCompare(y.name) :
            y.mode.localeCompare(x.mode));

        const resultData = data.map(entry => new DirectoryListing(entry.name, entry.mode, entry.mtime, entry.size));
        return { status: response.status, data: resultData };
    }

    /**
     * @param {string} networkShareUrl
     * @returns {string}
     */
    static getNetworkShareHttpProxyUrl(networkShareUrl) {
        const url = new URL(networkShareUrl);
        
        // for now only smb is supported
        if (url.protocol !== SMB_PROTOCOL) {
            throw new Error("Unsupported network share protocol: " + url.protocol);
        }
        const host = url.hostname;
        const user = url.username;
        const pass = url.password;
        const path = url.pathname;

        const protocolWithoutColon = url.protocol.replace(":", "");

        let result = `${baseURL}/${protocolWithoutColon}${path}?addr=${host}`;
        if (user) {
            result += `&user=${user}`;
        }
        if (pass) {
            result += `&pass=${pass}`;
        }

        return result;
    }

    /**
     * @param {string} path
     * @returns {Promise<APIResponse<DirectoryListing[]?>>}
     */
    static async fsListSmbDir(path) {
        if (!path.startsWith(SMB_SCHEME_PREFIX)) {
            return { status: HTTP_ERROR_BAD_REQUEST, data: null };
        }

        if (!path.endsWith("/")) {
            path += "/";
        }

        let url = ApiClient.getNetworkShareHttpProxyUrl(path);

        let response = await fetch(url);
        if (!response.ok) {
            return { status: response.status, data: null };
        }
        let data = await response.json();
        data = data.filter(entry => !ignoredFileNames.includes(entry.name));
        data.sort((x, y) => (DirectoryListing.isDir(x) == DirectoryListing.isDir(y)) ?
            x.name.localeCompare(y.name) :
            y.mode.localeCompare(x.mode));

        let resultData = data.map(entry => new DirectoryListing(entry.name, entry.mode, entry.mtime, entry.size));
        return { status: response.status, data: resultData };
    }

    
    /**
     * @param {string} path 
     * @returns {Promise<APIResponse<DirectoryListing[]?>>}
     */
    static async listSmbShares(path) {
        if (!path.startsWith(SMB_SCHEME_PREFIX)) {
            return { status: HTTP_ERROR_BAD_REQUEST, data: null };
        }

        if (path.endsWith("/")) {
            path = path.substring(0, path.length - 1);
        }

        let url = ApiClient.getNetworkShareHttpProxyUrl(path);

        let response = await fetch(url);
        if (!response.ok) {
            return { status: response.status, data: null };
        }
        let data = await response.json();
        data = data.filter(entry => !entry.name.endsWith("$"));
        let resultData = data.map(entry => new DirectoryListing(entry.name, "smb-share"));
        return { status: response.status, data: resultData };
    }


    /**
     * @param {string} path
     * @returns {Promise<APIResponse<DirectoryListing[]?>>}
     */
    static async fsListDir(path) {
        if (path.startsWith(SMB_SCHEME_PREFIX)) {
            const url = new URL(path);
            if (url.pathname === "" || url.pathname === "/") {
                return await ApiClient.listSmbShares(path);
            }

            return await ApiClient.fsListSmbDir(path);
        }

        return await ApiClient.fsListInternalDir(path);
    }

    /**
     * 
     * @param {string} path 
     * @returns {Promise<APIResponse<ReadableStream<Uint8Array>?>>}
     */
    static async fsGetFileStream(path) {
        if (path.endsWith("/") || !path.startsWith("/")) {
            return { status: HTTP_ERROR_BAD_REQUEST, data: null };
        }
        
        let response = await fetch(baseURL + "/fs" + path);
        if (!response.ok) {
            return { status: response.status, data: null };
        }
        
        return { status: response.status, data: response.body };
    }

    /**
     * 
     * @param {string} path 
     * @returns {Promise<APIResponse<string?>>}
     */
    static async fsGetFileText(path) {
        if (path.endsWith("/") || !path.startsWith("/")) {
            return { status: HTTP_ERROR_BAD_REQUEST, data: null };
        }

        let response = await fetch(baseURL + "/fs" + path);
        if (!response.ok) {
            return { status: response.status, data: null };
        }
        let resultData = await response.text();
        return { status: response.status, data: resultData };
    }

    /** @typedef {Object} MdnsLocation
     * @property {string} domain
     * @property {string} protocol
     * @property {string} hostname
     * @property {string} address
     * @property {number} port
     */

    /**
     * @returns {Promise<MdnsLocation[]>}
     */
    static async getMdnsDiscoveredLocations() {
        let response = await fetch(baseURL + "/mdns");
        if (!response.ok) {
            return [];
        }
        return await response.json();
    }

    static async getVersion() {
        try {
            const response = await fetch(baseURL + '/version');
            if (response.ok) {
                return await response.json();
            }
        } catch (error) { }
        return {
            api: 0,
            tag: '',
            date: '',
            time: ''
        };
    }
}
