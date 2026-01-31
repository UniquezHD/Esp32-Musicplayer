const fs = require('fs')

const getMP3Duration = require('get-mp3-duration')

const toRemove = [
    " (Official Music Video)",
    " (Lyric Video)",
    " (Official Video)"
]

let songDurationList = [];
let songList = [];
let songsCount;

fs.readdir(`./FilesToRename/`, (err,files) => {
    files.forEach((file , i) => {
        songsCount = i;

        let buffer = fs.readFileSync(`./FilesToRename/${file}`)
        songDurationList.push(getMP3Duration(buffer)); 

        if(i >= 9){
            if(file.includes("(Official Music Video)")){
                fs.copyFile(`./FilesToRename/${file}`, `RenamedFiles/0${i + 1} ${file.replace(" (Official Music Video)", "")}`, () => {})
                songList.push(`{"${file.replace(".mp3", "")}"}`)
            } else {
                fs.copyFile(`./FilesToRename/${file}`, `RenamedFiles/0${i + 1} ${file}`, () => {})
                songList.push(`{"${file.replace(".mp3", "")}"}`)
            }
        } else {
            if(file.includes("(Official Music Video)")){
                fs.copyFile(`./FilesToRename/${file}`, `RenamedFiles/00${i + 1} ${file.replace(" (Official Music Video)", "")}`, () => {})
                songList.push(`{"${file.replace(".mp3", "")}"}`)
            } else {
                fs.copyFile(`./FilesToRename/${file}`, `RenamedFiles/00${i + 1} ${file}`, () => {})
                songList.push(`{"${file.replace(".mp3", "")}"}`)
            }
        }
    });
    songsCount += 1
    console.log(`${songList.join(", ")}`)
    console.log('---------------------------------')
    console.log(`{${songDurationList.map(item => `{"${item}"}`).join(", ")}}`);
    console.log(songDurationList.length)
    console.log('Songs: ' + songsCount)
    console.log('Done')    
})