'use strict';
var mongoose = require('mongoose'),Libro = mongoose.model('Libros');


exports.ListarTodosLosLibros = function (req, res) {
    Libro.find({})
      .then(libros => {
        res.json(libros);
      })
      .catch(err => {
        res.status(500).send(err);
      });
  };
  exports.crearLibro = function (req, res) {
    var newLibro = new Libro(req.body);
    newLibro.save()
      .then(libro => {
        res.json(libro);
      })
      .catch(err => {
        res.status(500).send(err);
      });
  };
  
  exports.leerLibro = function (req, res) {
    Libro.findById(req.params.libroId)
      .then(libro => {
        res.json(libro);
      })
      .catch(err => {
        res.status(500).send(err);
      });
  };
  
  exports.actualizarLibro = function (req, res) {
    Libro.findOneAndUpdate({ _id: req.params.libroId }, req.body, { new: true })
      .then(libro => {
        res.json(libro);
      })
      .catch(err => {
        res.status(500).send(err);
      });
  };
  
  exports.borrarLibro = function (req, res) {
    Libro.remove({ _id: req.params.libroId })
      .then(() => {
        res.json({ message: 'Libro Borrado Exitosamente' });
      })
      .catch(err => {
        res.status(500).send(err);
      });
  };
  