'use strict';
var mongoose = require('mongoose'), 
    Usuario = mongoose.model('Usuario'); // Asegúrate de que el nombre del modelo coincida con el definido en el archivo del modelo.

exports.ListarTodosLosUsuarios = function (req, res) {
    Usuario.find({})
      .then(usuario => { // Cambiado el nombre de la variable para evitar confusión con el modelo.
        res.json(usuario); // Pasar los datos de los usuarios encontrados como argumento.
      })
      .catch(err => {
        res.status(500).send(err);
      });
};
  exports.crearUsuario = function (req, res) {
    var newUsuario = new Usuario(req.body);
    newUsuario.save()
      .then(usuario => {
        res.json(Usuario);
      })
      .catch(err => {
        res.status(500).send(err);
      });
  };
  
  exports.leerUsuario = function (req, res) {
    Usuario.findById(req.params.usuarioId)
      .then(usuario => {
        res.json(usuario);
      })
      .catch(err => {
        res.status(500).send(err);
      });
  };

  
  exports.LogueoUsuario = function (req, res) {
    Usuario.findOne({ login: req.params.login, password: req.params.password })
        .then(usuario => {
            if (!usuario) {
                return res.status(200).json({ message: 'Usuario no encontrado desde el servicio' });
            }
            res.json(usuario);
        })
        .catch(err => {
            res.status(500).send(err);
        });
};
  
  exports.actualizarUsuario = function (req, res) {
    Usuario.findOneAndUpdate({ _id: req.params.usuarioId }, req.body, { new: true })
      .then(usuario => {
        res.json(usuario);
      })
      .catch(err => {
        res.status(500).send(err);
      });
  };
  
  exports.borrarUsuario = function (req, res) {
    Usuario.remove({ _id: req.params.usuarioId })
      .then(() => {
        res.json({ message: 'Usuario Borrado Exitosamente' });
      })
      .catch(err => {
        res.status(500).send(err);
      });
  };
  