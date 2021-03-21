const createError = require('http-errors');
const express = require('express');
const path = require('path');
const exphbs = require('express-handlebars');


const indexRouter = require('./routes/index');
const imageRouter = require('./routes/image');
const sprinklesRouter = require('./routes/sprinkles');

const app = express();

// view engine setup
app.engine('handlebars', exphbs());
app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'handlebars');

app.use(express.json());
app.use(express.urlencoded({ extended: false }));
app.use(express.static(path.join(__dirname, 'public')));

app.use('/', indexRouter);
app.use('/image', imageRouter);
app.use('/sprinkles', sprinklesRouter);

// catch 404 and forward to error handler
app.use(function(req, res, next) {
   next(createError(404));
});

// error handler
app.use(function(err, req, res, next) {
  console.log('got error', err, err.status);
  // set locals, only providing error in development
  res.locals.message = err.message;
  res.locals.error = err;

  // render the error page
  
  res.status(err.status || 500);
  res.render('error', err);
});

module.exports = app;
