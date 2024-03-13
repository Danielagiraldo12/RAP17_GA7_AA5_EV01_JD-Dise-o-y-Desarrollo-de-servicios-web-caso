'use strict';
module.exports = function (app) {
    var usuario = require('../controllers/UsuarioController');
        // libros Routes
      app.route('/usuario')
      .get(usuario.ListarTodosLosUsuarios)
      .post(usuario.crearUsuario);
      
      app.route('/usuario/:usuarioId')
      .get(usuario.leerUsuario)      
      .put(usuario.actualizarUsuario)
      .delete(usuario.borrarUsuario);

      // Ruta para la función LogueoUsuario
    app.route('/usuario/login/:login/:password')
    .get(usuario.LogueoUsuario);
  };