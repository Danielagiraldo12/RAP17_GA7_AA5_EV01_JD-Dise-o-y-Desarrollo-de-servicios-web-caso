'use strict';
module.exports = function (app) {
    var libro = require('../controllers/LibroController');
        // libros Routes
      app.route('/libros')
      .get(libro.ListarTodosLosLibros)
      .post(libro.crearLibro);  
      app.route('/libros/:libroId')
      .get(libro.leerLibro)
      .put(libro.actualizarLibro)
      .delete(libro.borrarLibro);
  };
