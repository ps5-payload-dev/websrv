/**
 * throws exception if the server is down
 * @param {string} path 
 * @returns {object?} null if not success
 */
async function fetchJson(path) {
	let response = await fetch(baseURL + path);
	if (!response.ok) {
		return null;
	}
	let data = await response.json();
	return data;
}

// https://stackoverflow.com/questions/105034/how-do-i-create-a-guid-uuid
function uuidv4() {
	return "10000000-1000-4000-8000-100000000000".replace(/[018]/g, c =>
		(+c ^ crypto.getRandomValues(new Uint8Array(1))[0] & 15 >> +c / 4).toString(16)
	);
}

async function sleep(ms) {
	return new Promise(resolve => setTimeout(resolve, ms));
}

// this is kind of a memory leak, 
// if the user goes to home and then reenters the carousel created by the hb.js itll make new entries
// which makes sense but the old ones arent removed
function remapFunctionsToLocalFunctionIds(obj) {
    if (typeof obj !== 'object' || obj === null) {
        return obj;
    }

    // if array, iterate over its elements recursively
    if (Array.isArray(obj)) {
        return obj.map(item => remapFunctionsToLocalFunctionIds(item));
    }

    return Object.keys(obj).reduce((acc, key) => {
        if (typeof obj[key] === 'function') {
            let uuid = uuidv4();
            if (!Globals.Funcs) {
                Globals.Funcs = {};
            }
            Globals.Funcs[uuid] = obj[key];
            acc[key] = uuid;
        } else if (typeof obj[key] === 'object' && obj[key] !== null) {
            // Recursively serialize nested objects
            acc[key] = remapFunctionsToLocalFunctionIds(obj[key]);
        } else {
            acc[key] = obj[key];
        }
        return acc;
    }, {});
}

function remapFunctionIdsToLocalCalls(obj) {
    if (typeof obj !== 'object' || obj === null) {
        return obj;
    }

    // if array, iterate over its elements recursively
    if (Array.isArray(obj)) {
        return obj.map(item => remapFunctionIdsToLocalCalls(item));
    }

    return Object.keys(obj).reduce((acc, key) => {
        if (key === 'onclick') {
            let matchingFunc = Globals.Funcs[obj[key]];
            acc[key] = matchingFunc;
        } else if (typeof obj[key] === 'object' && obj[key] !== null) {
            // Recursively serialize nested objects
            acc[key] = remapFunctionIdsToLocalCalls(obj[key]);
        } else {
            acc[key] = obj[key];
        }
        return acc;
    }, {});
}