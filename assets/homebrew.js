const HOMEBREW_FS = '/fs/data/homebrew/';

async function launchHomebrew(name) {
    const path = (HOMEBREW_FS + name + '/eboot.elf').substring(3);
    await fetch('/hbldr?path=' + path);
}

async function getHomebrewList() {
    let response = await fetch(HOMEBREW_FS + '?fmt=json');
    let data = await response.json();
    return data;
}

async function renderHomebrewList() {
    let homebrewList = await getHomebrewList();
    homebrewList.forEach(entry => {
	if(entry.name.startsWith(".")) {
	    return;
	}

	const icon0 = document.createElement('img');
	icon0.src = HOMEBREW_FS + entry.name + '/sce_sys/icon0.png';
	icon0.alt = entry.name;

	const link = document.createElement('a');
	link.href = '#';
	link.onclick = async function() {
	    const href = HOMEBREW_FS + entry.name + '/index.html';
	    let resp = await fetch(href);
	    if(resp.status == 200) {
		location.href = href;
	    } else {
		launchHomebrew(entry.name);
	    }
	}

	link.appendChild(icon0);
	document.body.appendChild(link);
    });
}
