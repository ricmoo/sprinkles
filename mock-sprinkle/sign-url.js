
const { basename } = require("path");

const { ethers } = require("ethers");

const { sign } = require("./mock");


if (process.argv.length !== 4) {
    console.log(`Usage: ${ basename(process.argv[1]) } PRIVATE_KEY URL`);
    process.exit(1);
}

const privateKey = process.argv[2];
const url = process.argv[3];

// @TODO: in the future this will be a challenge/response timestamp
const hash = ethers.utils.id("sprinkles");


const signature = sign(privateKey, hash);

console.log(url + ((url.indexOf("?") == -1) ? "?": "&") + "signature=" + signature);
