'use strict';
var mongoose = require('mongoose');
var Schema = mongoose.Schema;
var usuarioSchema = new Schema({
  id_usuario: {
    type: Number,
    required: 'El identificador del usuario es obligatorio'
  },
  cedula: {
    type: String,
    required: 'La cédula del usuario es obligatoria'
  },
  nombres: {
    type: String,
    required: 'El nombre del usuario es obligatorio'
  },
  apellidos: {
    type: String,
    required: 'Los apellidos del usuario son obligatorios'
  },
  celular: {
    type: String,
    required: 'El número de celular del usuario es obligatorio'
  },
  correo: {
    type: String,
    required: 'El correo electrónico del usuario es obligatorio'
  },
  direccion: {
    type: String,
    required: 'La dirección del usuario es obligatoria'
  },
  id_tipo_documento: {
    type: Number,
    required: 'El tipo de documento del usuario es obligatorio'
  },
  id_perfil: {
    type: Number,
    required: 'El perfil del usuario es obligatorio'
  },
  id_estado: {
    type: Number,
    required: 'El estado del usuario es obligatorio'
  },
  login: {
    type: String,
    required: 'El nombre de usuario es obligatorio'
  },
  password: {
    type: String,
    required: 'La contraseña del usuario es obligatoria'
  },
  eliminado: {
    type: Number,
    default: 0
  },
  Created_date: {
    type: Date,
    default: Date.now
  }
});

module.exports = mongoose.model('Usuario', usuarioSchema);