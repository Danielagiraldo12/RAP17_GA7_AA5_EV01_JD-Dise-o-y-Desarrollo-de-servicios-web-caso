'use strict';
var mongoose = require('mongoose');
var Schema = mongoose.Schema;
var LibroSchema = new Schema({
  title: {
    type: String,
    required: 'Titulos del Libro'
  },  isbn: {
    type: String,
    required: 'Identificador del Libro'
  },
  Created_date: {
    type: Date,
    default: Date.now
  },
});

module.exports = mongoose.model('Libros', LibroSchema);