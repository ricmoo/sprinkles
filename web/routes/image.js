const createError = require('http-errors');
const { getImage, toRGB } = require('../utils/image-helper');
const { recover } = require('../utils/signature-helper');
const express = require('express');
const router = express.Router();

const ethers = require('ethers');
const { MongoClient } = require("mongodb");

const dbName = 'sprinklesdb'
const collection = 'sprinkles'
const missingUrl = 'https://powerful-stream-18222.herokuapp.com/assets/sprinkles.png'


const validate = async (req, res, next) => {

   if( req.query.signature ) {
      next();
      return;
   }

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

const fetchSprinkle = async (publicKeys) => {
    
   let sprinkle;
   const client = new MongoClient(process.env.MONGODB_URL, { useUnifiedTopology: true });
   try {
      await client.connect();
      const database = client.db(dbName);
      const sprinkles = database.collection(collection);
   
      const query = { publicKeyHash: { $in: publicKeys } };
      sprinkle = await sprinkles.findOne(query);
   } 
   finally {
       await client.close();
   }

   return sprinkle;
}

const computePubKeys = (hash, signature) => {
   const keys = [0, 1].map(recoveryParam => {
       let sig = {
           r: ("0x" + signature.substring(2, 66)),
           s: ("0x" + signature.substring(66, 130)),
           recoveryParam
       };

       return recover(hash, sig).pubkeyHash;
   });

   return keys;
}

const getImageBySignature = async (signature ) => {
   const hash = ethers.utils.id("sprinkles");
   const keys = computePubKeys(hash, signature);

   console.log('keys', keys)
   const sprinkle = await fetchSprinkle(keys);

   let image;
   if( sprinkle ) {
      image = await getImage(sprinkle.tokenContractAddress, sprinkle.tokenId);
   } else {
      console.log('no image....')
      image = await toRGB(missingUrl);
   }
   return(image);

}


const handleGet = async (req, res, next) => {
   try {
      let image;

      if( req.query.signature ) {
         image = await getImageBySignature(req.query.signature);
      } else {
         image = await getImage(req.contractAddress, req.tokenId);
      }
      res.send(image);
   } catch ( err ) {
      next(err);
   }
}

/* GET image */
router.get('/', validate, handleGet);

module.exports = router;
