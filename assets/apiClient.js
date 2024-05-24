// this file also gets injected into extension sandbox

// @ts-check

class DirectoryListing {
    /**
     * @param {string} name
     * @param {string} mode
    */
   constructor(name, mode) {
       this.name = name;
       this.mode = mode;
    }
    
    isDir() {
        return this.mode === 'd' && !ignoredFileNames.includes(this.name);
    }
    
    isFile() {
        return this.mode === '-' && !ignoredFileNames.includes(this.name);
    }
}

// @ts-ignore
const baseURL = window.location.origin == 'null' ? 'http://127.0.0.1:8080' : window.location.origin;
const ignoredFileNames = ['.', '..'];
class ApiClient {
    /**
     * @param {string} path
     * @param {(string | string[])?} [args]
     * @returns {Promise<boolean>} only returns true, throws error if not success code
     */
    static async launchApp(path, args = null) {
        let params = new URLSearchParams({
            'path': path
        });

        if (typeof args === 'string') {
            params.append('args', args.replaceAll(" ", "\\ "));
        } else if (Array.isArray(args)) {
            params.append('args', args.map(arg => arg.replaceAll(" ", "\\ ")).join(' '));
        }

        let uri = baseURL + '/hbldr?' + params.toString();

        let response = await fetch(uri);
        if (response.status !== 200) {
            throw new Error('Failed to launch app, status code: ' + response.status);
        }

        return true;
    }

    /**
     * @param {string} path
     * @returns {Promise<DirectoryListing[]?>}
     */
    static async fsListDir(path) {
        if (!path.endsWith('/')) {
            path += '/';
        }

        let response = await fetch(baseURL + "/fs" + path + "?fmt=json");
        if (!response.ok) {
            return null;
        }
        let data = await response.json();
        return data.filter(entry => !ignoredFileNames.includes(entry.name)).map(entry => new DirectoryListing(entry.name, entry.mode));
    }

    /**
     * 
     * @param {string} path 
     * @returns {Promise<ReadableStream<Uint8Array>?>}
     */
    static async fsGetFileStream(path) {
        if (path.endsWith('/') || !path.startsWith('/')) {
            return null;
        }

        let response = await fetch(baseURL + "/fs" + path);
        if (!response.ok) {
            return null;
        }
        return response.body;
    }

    /**
     * 
     * @param {string} path 
     * @returns {Promise<string?>}
     */
    static async fsGetFileText(path) {
        if (path.endsWith('/') || !path.startsWith('/')) {
            return null;
        }

        let response = await fetch(baseURL + "/fs" + path);
        if (!response.ok) {
            return null;
        }
        return await response.text();
    }
}
