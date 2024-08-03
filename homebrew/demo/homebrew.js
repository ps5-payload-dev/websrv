// must be called main
// main must return in 5 seconds, other stuff like onclicks dont have such restrictions
async function main() {
    let stateTest = "State is preserved";
    // must return an object like this
    return {
        // every field of this is optional
        mainText: "demo",
        secondaryText: window.workingDir, // injected, contains the path to the folder containing this js file
        imgPath: baseURL + "/fs/data/homebrew/demo/proxy-image.png", // the image in sce_sys/icon0.png wont show bc this overrides it
        onclick: async () => {
            // get rom infos here or you can create a function for it outside

            // one second delay to test the spinner
            await new Promise(resolve => setTimeout(resolve, 1000));

            // this is the format you need to pass to showCarousel
            let items = [
                {
                    mainText: "The Final Insect (Taurus 1993)",
                    imgPath: baseURL + "/fs/data/homebrew/demo/roms/1/img.jpg",
                    onclick: async () => {
                        await ApiClient.launchApp("/fs/data/homebrew/demo/demo.elf");
                        return true; // return true to indicate stuff has launched and show you can exit now text
                    },
                    options: [
                        {
                            text: "Enter args",
                            onclick: () => { alert(prompt("Enter args")); }
                        }
                    ]
                },
                {
                    mainText: stateTest,
                    secondaryText: "bottom text",
                    onclick: () => {
                        return false;
                    }
                },
                {
                    mainText: "Pick file...",
                    onclick: async () => { alert("This alert is coming from the extension, path:" + await pickFile(window.workingDir)); } // pickfile arg = initial path, optional
                }
            ];

            showCarousel(items);
        },
        options: [
            {
                text: "Error test",
                onclick: () => { throw new Error("Testing"); }
            },
            {
                text: "Option 2",
                onclick: () => { alert("Option 2"); }
            }
        ]

    };
}
