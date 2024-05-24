
async function main() {
    const PAYLOAD = window.workingDir + '/devilutionx.elf';

    return {
        mainText: "DevilutionX",
        secondaryText: 'Diablo build for modern operating systems',
        onclick: async () => {
	    await ApiClient.launchApp(PAYLOAD);
	    return true;
        },
	options: [
	    {
		text: "Force Shareware mode",
		onclick: async () => {
		    await ApiClient.launchApp(PAYLOAD, "--spawn");
		    history.back();
		    return true;
		}
            },
	    {
		text: "Force Diablo mode",
		onclick: async () => {
		    await ApiClient.launchApp(PAYLOAD, "--diablo");
		    history.back();
		    return true;
		}
	    },
	    {
		text: "Force Hellfire mode",
		onclick: async () => {
		    await ApiClient.launchApp(PAYLOAD, "--hellfire");
		    history.back();
		    return true;
		}
	    }   
	]
    };
}
