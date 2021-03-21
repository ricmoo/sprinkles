const createError = require('http-errors');
const express = require('express');
const router = express.Router();

const Jimp = require('jimp');
const ethers = require('ethers');

const provider = new ethers.providers.InfuraProvider();

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
   console.log({ contractAddress, id, imageUrl });

   return rgbHex;
}


const validate = async (req, res, next) => {
   let { contract_address: contractAddress, token_id: tokenId } = req.query;
   if( !contractAddress || !tokenId ) {
      next(new createError.BadRequest('missing query params contract_address or token_id'));
   }

   if( !ethers.utils.isAddress(contractAddress) ) {
      if( contractAddress ) {
         try {
            const resolvedAddress = await provider.resolveName(contractAddress);
            contractAdress = resolvedAddress;
         } catch (err) {
            next(new createError.BadRequest(`Invalid contract address: ${err}`));
         }
      }
   }

   req.contractAddress = contractAddress;
   req.tokenId = tokenId;
   next();
}

const handleGet = async (req, res, next) => {
   try {
      const image = await getImage(req.contractAddress, req.tokenId);
      res.send(image);
   } catch ( err ) {
      next(err);
   }
}

/* GET image */
router.get('/', validate, handleGet);

module.exports = router;
