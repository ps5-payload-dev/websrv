// @ts-check

const BROWSER_REGISTER_HOMEBREW_ACTION = "BROWSER_REGISTER_HOMEBREW_ACTION";
const BROWSER_CALL_FUNCTIONID_AT_END = "BROWSER_CALL_FUNCTIONID_AT_END";

/**
 * @param {string} path 
 * @returns {Promise<{path: string, finished: boolean}>}
 */
async function renderBrowsePage(path) {
    /** @type {Promise<{path: string, finished: boolean}>[]} */
    let result = [];

    let data = null;

    if (!path.endsWith('/')) {
        path += '/';
    }

    data = await ApiClient.fsListDir(path);

    if (!data) {
        throw new Error('Failed to load directory: ' + path);
    }

    // group by type
    const directories = data.filter(entry => entry.mode === 'd');
    const files = data.filter(entry => entry.mode === '-');
    // sort by name
    directories.sort((a, b) => a.name.localeCompare(b.name));
    files.sort((a, b) => a.name.localeCompare(b.name));
    // merge
    data = directories.concat(files);

    // reset content
    const content = /** @type {HTMLElement} */ (document.getElementById('content'));
    content.innerHTML = '';

    // create list div
    const list = document.createElement('div');
    list.classList.add('list');

    // add back button if needed
    if (path !== '/') {
        let entryElement = document.createElement('a');
        entryElement.classList.add('list-entry');

        entryElement.tabIndex = 0;

        result.push(new Promise((resolve, reject) => {
            entryElement.onclick = () => {
                let backPath = path.substring(0, path.lastIndexOf('/'));
                backPath = backPath.substring(0, backPath.lastIndexOf('/') + 1);
                resolve({ path: backPath, finished: false });
            }
        }));


        const backIcon = document.createElement('div');
        backIcon.classList.add('list-entry-icon');
        backIcon.classList.add('back-icon');

        entryElement.appendChild(backIcon);

        const nameElement = document.createElement('p');
        nameElement.classList.add('list-entry-name');
        nameElement.innerText = '..';
        entryElement.appendChild(nameElement);

        list.appendChild(entryElement);
    }


    // add entries
    data.forEach(entry => {
        let entryElement = document.createElement('a');
        entryElement.classList.add('list-entry');
        if (entry.mode === 'd') {
            entryElement.classList.add('list-entry-directory');
            // add dir icon
            const dirIcon = document.createElement('div');
            dirIcon.classList.add('list-entry-icon');
            dirIcon.classList.add('dir-icon');

            entryElement.appendChild(dirIcon);
        } else if (entry.mode === '-') {
            entryElement.classList.add('list-entry-file');
            // add file icon
            const fileIcon = document.createElement('div');
            fileIcon.classList.add('list-entry-icon');
            fileIcon.classList.add('file-icon');

            entryElement.appendChild(fileIcon);
        }

        // make the cursor snap
        entryElement.tabIndex = 0;

        const nameElement = document.createElement('p');
        nameElement.classList.add('list-entry-name');
        nameElement.innerText = entry.name;
        entryElement.appendChild(nameElement);

        result.push(new Promise((resolve, reject) => {
            entryElement.onclick = () => {
                if (entry.mode === 'd') {
                    resolve({ path: path + entry.name + '/', finished: false });
                } else if (entry.mode === '-') {
                    resolve({ path: path + entry.name, finished: true });
                }
            }
        }));

        list.appendChild(entryElement);
    });

    content.appendChild(list);

    return Promise.race(result);
}

/**
 * @param {string} initialPath 
 * @returns {Promise<string>}
 */
async function pickFile(initialPath = "/") {
    let result = await renderBrowsePage(initialPath);
    while (!result.finished) {
        result = await renderBrowsePage(result.path);
    }
    return result.path;
}

/**
 * @param {string} initialPath 
 * @returns {Promise}
 */
async function browseHandleRegisterHomebrewAction(initialPath = "/") {
    let path = await pickFile(initialPath);
    addToHomebrewStore(path);
    window.history.back();
}

/**
 * @param {string} initialPath 
 * @param {string} functionId
 * @returns {Promise}
 */
async function browseHandleCallFunctionAtEnd(initialPath = "/", functionId, backCount = 1) {
    let func = Globals.Funcs[functionId];
    if (!func) {
        return
    }
    let path = await pickFile(initialPath);
    func(path);
    
    window.history.go(-backCount);
}