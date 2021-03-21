const ethers = require("ethers");

const EC = require('elliptic').ec;
const ec = new EC('p256');

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
