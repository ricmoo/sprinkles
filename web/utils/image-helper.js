const Jimp = require('jimp');
const ethers = require('ethers');

const provider = new ethers.providers.InfuraProvider();

const toRGB = async (imageUrl) => {
   const matrix = [];
   res = await Jimp.read(imageUrl)
               .then(image => image
                  .resize(240,240)
                  .scan(0, 0, image.bitmap.width, image.bitmap.height, (x, y, idx) => {
                        const red   = image.bitmap.data[idx + 0];
                        const green = image.bitmap.data[idx + 1];
                        const blue  = image.bitmap.data[idx + 2];
                        matrix.push(red);
                        matrix.push(green);
                        matrix.push(blue);
                  }));

   const rgbHex = ethers.utils.hexlify(matrix);

   return rgbHex;

}

const getImage = async (assetAddress, id) => {
    let contractAddress = assetAddress;
    if( !ethers.utils.isAddress(contractAddress) ) {
        contractAddress = await provider.resolveName(assetAddress);
    }
 
    const baseUrl=`https://api.opensea.io/api/v1/assets?asset_contract_address=${contractAddress}&limit=1`
    const url = id? `${baseUrl}&token_ids=${id}` : baseUrl;
 
    let res = await ethers.utils.fetchJson(url);
    const imageUrl = res.assets[0].image_url;
 
    if(!imageUrl) {
       throw new Error('Unable to fetch image')
    }
    
    return toRGB(imageUrl);
 }
 

 module.exports = {
    getImage,
    toRGB
 };