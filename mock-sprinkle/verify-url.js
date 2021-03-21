
const { basename } = require("path");

const { ethers } = require("ethers");

const { recover } = require("./mock");

if (process.argv.length !== 3) {
    console.log(`Usage: ${ basename(process.argv[1]) } URL`);
    process.exit(1);
}

const url = process.argv[2];

const hash = ethers.utils.id("sprinkles");

let signature = null;
let bareUrl = url.replace(/(signature=(0x[0-9a-f]*))/, (all, clump, sig) => {
    console.log(sig);
    signature = {
        r: ("0x" + sig.substring(2, 66)),
        s: ("0x" + sig.substring(66, 130)),
        recoveryParam: ((sig.substring(130, 132) === "00") ? 0: 1)
    };
});
console.log(signature);
if (signature == null) {
    console.log("No signature found.");
    process.exit(2);
}

const publicKey = recover(hash, signature);
console.log(publicKey);
