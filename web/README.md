# Sprinkles Web

## Overview
The web component of Sprinkles which has a UI and a API component

### UI
This is where owner of a sprinkle go to setup which asset to display on the sprinkle
 - login page to get the owner address
 - sprinkles dashboard to list all sprinkles owned by the account
 - assets page to list all assets owned by the account 
 - save asset to database
 
### API
Provides RGB image data to sprinkles
 - sprinkles calls the api to get image data to display on the screen


## Quick Setup
Deploy to heroku
 -  download [heroku cli](https://devcenter.heroku.com/articles/heroku-cli)
 -  clone this repo
 -  heroku create
 -  git push heroku master

## Try it out
- web
  - [https://powerful-stream-18222.herokuapp.com](https://powerful-stream-18222.herokuapp.com)
- api
  - [https://powerful-stream-18222.herokuapp.com/image?contract_address=0x31385d3520bced94f77aae104b406994d8f2168c&token_id=7197](https://powerful-stream-18222.herokuapp.com/image?contract_address=0x31385d3520bced94f77aae104b406994d8f2168c&token_id=7197)
  - [https://powerful-stream-18222.herokuapp.com/image?contract_address=0x06012c8cf97bead5deae237070f9587f8e7a266d&token_id=2](https://powerful-stream-18222.herokuapp.com/image?contract_address=0x06012c8cf97bead5deae237070f9587f8e7a266d&token_id=2)
