const fs = require('fs')

fs.readdir(`./FilesToClean/`, (err, files) => {
    files.forEach((file, i) => {
        fs.copyFile(`./FilesToClean/${file}`, `./FilesToRename/${file.slice(4)}`, () => {})
        console.log(file + " " + `./FilesToRename/${file.slice(4)}`)
    })
})
