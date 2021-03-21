// To deploy use:
// /home/ricmoo> npm run build
// /home/ricmoo> ethers --network ropsten --account PATH_TO_WALLET run deploy.js

const fs = require("fs");

/*
const ethers = require("ethers");
const provider = new ethers.providers.InfuraProvider("ropsten");
const accounts = [ new ethers.Wallet(ethers.utils.id("MooMoo"), provider) ];
console.log(accounts[0].address);
*/

const tokens = [
    {
        hash: "0xfd5be5c0ed82501be7c470827402ea9980d6a986a05fb025f5089c8b9a651ace",
        addr: "0x8ba1f109551bD432803012645Ac136ddd64DBA72" // ricmoo.firefly.eth
    },
    {
        hash: "0xd5bd1af126c31c14d7a32f0974fa736282159e311024d6323bdbb96f7941fcfb",
        addr: "0x8ba1f109551bD432803012645Ac136ddd64DBA72" // ricmoo.firefly.eth
    },
    {
        hash: "0xd7d51c164923d260f46da51418ee33f3ed2fc5d91a31ec6774104df48064bfdd",
        addr: "0x8ba1f109551bD432803012645Ac136ddd64DBA72" // ricmoo.firefly.eth
    },
    {
        hash: "0x8bfdaaa52fe9de7c6ed32841251ba61ab38e6db04d4e9774828d4cf2093ce250",
        addr: "0x23936429FC179dA0e1300644fB3B489c736D562F" // yuetloo.eth
    },
    {
        hash: "0x9bc8bb43f89a73c1a6b82ed4b038cbb685f4a199bedc77bc9b59285b1d92c97f",
        addr: "0x23936429FC179dA0e1300644fB3B489c736D562F" // yuetloo.eth
    },
    {
        hash: "0x6424ede1f84f0965185a7dac24640fc77d1a95a08f646bdfb9209228902ba842",
        addr: "0x23936429FC179dA0e1300644fB3B489c736D562F" // yuetloo.eth
    },
];

(async function() {
    const filename = "artifacts/build-info/output.json";
    const json = JSON.parse(fs.readFileSync(filename).toString());
    const data = json.output.contracts["contracts/Sprinkles.sol"].Sprinkles;
    console.log(data.abi);

    const factory = ethers.ContractFactory.fromSolidity(data, accounts[0]);
    console.log(factory);

    console.log("Deploying...");
    const contract = await factory.deploy();
    console.log("Contract Address:", contract.address);
    const tx = await contract.deployTransaction;
    const receipt = await tx.wait();
    console.log("Deployed!");

    for (let i = 0; i < tokens.length; i++) {
        const token = tokens[i];
        console.log(`Adding Token #${ i + 1 }: ${ token.hash.substring(0, 12)} to ${ token.addr }`);
        const addTx = await contract.mint(token.addr, token.hash, {
            nonce: tx.nonce + i + 1
        });
        const receipt = await addTx.wait();
        console.log("Added:", addTx.hash);
    }

    console.log(`Please point sprinkles.eth to ${ contract.address }`);
})();
