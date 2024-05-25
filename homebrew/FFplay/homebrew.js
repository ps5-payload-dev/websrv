
async function main() {
    const PAYLOAD = window.workingDir + '/ffplay.elf';
    const MEDIADIR = window.workingDir + '/media/';
    
    return {
        mainText: "FFplay",
        secondaryText: 'FFmpeg Media Player',
        onclick: async () => {
	    let filepath = await pickFile(MEDIADIR);
            await ApiClient.launchApp(PAYLOAD, filepath);
	    return true;
        }
    };
}
