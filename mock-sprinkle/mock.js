const ethers = require("ethers");

const EC = require('elliptic').ec;
const ec = new EC('p256');

/*
const msgHash = [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1 ];
const signature = key.sign(msgHash);
console.log("SIG", signature);

console.log("VERIFY", key.verify(msgHash, signature));

const pub = key.getPublic(true, "hex");
//const pub = pubPoint.encode('hex');
console.log("PUB", pub);

//key2 = ec.keyFromPublic(pub, 'hex');
//console.log("FromPub", key2);
const rec = ec.recoverPubKey(msgHash, { r: signature.r, s: signature.s }, signature.recoveryParam);
console.log("REC", rec.encode("hex", true));
*/

module.exports = {
    generate: function() {
        const key = ec.genKeyPair();
        const pubKey = ("0x" + key.getPublic(false, "hex"));
        return {
            public: pubKey,
            pubkeyHash: ethers.utils.keccak256(pubKey),
            private: (ethers.utils.hexZeroPad("0x" + key.priv.toString(16)))
        };
    },
    sign: function(privateKey, digest) {
        const key = ec.keyFromPrivate(ethers.utils.arrayify(privateKey));
        const sig = key.sign(ethers.utils.arrayify(digest), { canonical: true });
        return ethers.utils.hexlify(ethers.utils.concat([
            ethers.utils.hexZeroPad("0x" + sig.r.toString(16), 32),
            ethers.utils.hexZeroPad("0x" + sig.s.toString(16), 32),
            ((sig.recoveryParam === 0) ? "0x00": "0x01")
        ]));
    },
    recover: function(digest, signature) {
        const rec = ec.recoverPubKey(ethers.utils.arrayify(digest), {
            r: ethers.utils.arrayify(signature.r),
            s: ethers.utils.arrayify(signature.s)
        }, signature.recoveryParam);
        const pubKey = "0x" + rec.encode("hex", false);
        return {
            public: pubKey,
            pubkeyHash: ethers.utils.keccak256(pubKey)
        };
    }
};
