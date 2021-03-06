const createError = require('http-errors');
const express = require('express');
const router = express.Router();

const { MongoClient } = require("mongodb");

const dbName = 'sprinklesdb'
const collection = 'sprinkles'

const validate = (req, res, next ) => {
    if( !req.body.sprinkle ) {
        next(new createError.BadRequest('Missing sprinkle data'))
        return;
    }

    const {
        sprinkleId,
        publicKeyHash,
        owner,
        tokenId,
        tokenContractAddress
    } = req.body.sprinkle;


    if( !sprinkleId ) {
        next(new createError.BadRequest('Invalid sprinkleId'))
        return;
    }
    
    if( !publicKeyHash ) {
        next(new createError.BadRequest('Invalid publicKeyHash'))
        return;
    }
    
    if( !tokenId ) {
        next(new createError.BadRequest('Invalid tokenId'))
        return;
    }

    if( !tokenContractAddress ) {
        next(new createError.BadRequest('Invalid tokenContractAddress'))
        return;
    }

    req.sprinkle = {         
        sprinkleId,
        publicKeyHash,
        owner,
        tokenId,
        tokenContractAddress
    }
    next();

}

const handleSave = async (req, res, next) => {
    let error;
    let result;

    const client = new MongoClient(process.env.MONGODB_URL, { useUnifiedTopology: true });
    try {
        await client.connect();
        const database = client.db(dbName);
        const sprinkles = database.collection(collection);
    
        // insert if no record found
        const options = { upsert: true };    
        const filter = { sprinkleId: req.sprinkle.sprinkleId };

        // set new config for a sprinkle
        const updateDoc = {
          $set: req.sprinkle
        };
    
        result = await sprinkles.updateOne(filter, updateDoc, options);
        
        console.log(
          `${result.matchedCount} document(s) matched the filter, ` + 
          `updated ${result.modifiedCount} document(s) ` + 
          `upsertedCount ${result.upsertedCount} documents(s)`,
        );
       
    } catch ( err ) {
        error = err;
    } finally {
        await client.close();

        if( error ) {
            next(error);
        } else {
            res.send({result: 'ok'});
        }
    }
 }


 const handleGet = async (req, res, next) => {
     const { sprinkleId } = req.params;

     let error;
     let sprinkle = {};
 
     const client = new MongoClient(process.env.MONGODB_URL, { useUnifiedTopology: true });
     try {
        await client.connect();
        const database = client.db(dbName);
        const sprinkles = database.collection(collection);
     
        const query = { sprinkleId }; 
        sprinkle = await sprinkles.findOne(query);
        if( !sprinkle ) {
            sprinkle = {};
        }
     } catch ( err ) {
         error = err;
     } finally {
         await client.close();
 
         if( error ) {
            next(error);
         } else {
            res.send(sprinkle || {});
         }
     }
 }
 


 /* Save a sprinkle config */
 router.post('/', validate, handleSave);
 router.get('/:sprinkleId', handleGet);
 
 module.exports = router;
 