async function makeRequest(path) {
    let response = await fetch(path);
    let data = await response.json();
    return data;
}

async function launchTitle(titleId) {
    await fetch('/launch?titleId=' + titleId);
}

async function getTitles() {
    return makeRequest('/fs/user/appmeta/?fmt=json');
}

async function renderTitleList() {
    var titles = await getTitles();
    
    titles.forEach(entry => {
	if(entry.name.length != 9) {
	    return;
	}

	const icon0 = document.createElement('img');
	icon0.src = '/fs/user/appmeta/' + entry.name + '/icon0.png';
	icon0.width = 300;
	icon0.height = 300;
	icon0.style.cursor = 'pointer';
	icon0.onclick = async function() {
	    launchTitle(entry.name);
	}
	document.body.appendChild(icon0);
    });
}
