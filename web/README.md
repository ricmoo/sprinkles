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
