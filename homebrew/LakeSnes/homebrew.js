
async function main() {
    const PAYLOAD = window.workingDir + '/lakesnes.elf';
    const ROMSDIR = window.workingDir + '/roms/';
    const ROMTYPES = ['smc', 'sfc']
    
    async function launchRom(filename) {
        await ApiClient.launchApp(PAYLOAD, ROMSDIR + filename);
    }

    async function getRomList() {
        let listing = await ApiClient.fsListDir(ROMSDIR);
	return listing.filter(entry =>
	    ROMTYPES.includes(entry.name.slice(-3))).map(entry => {
		const name = entry.name.slice(0, -4);
		return {
		    mainText: name,
		    imgPath: '/fs/' + ROMSDIR + name + ".jpg",
		    onclick: async() => {
			launchRom(entry.name);
			history.back();
			return true;
		    }
		};
	    });
    }
    return {
        mainText: "LakeSnes",
        secondaryText: 'Super Nintendo Emulator',
        onclick: async () => {
	    let items = await getRomList();
            showCarousel(items);
        }
    };
}
