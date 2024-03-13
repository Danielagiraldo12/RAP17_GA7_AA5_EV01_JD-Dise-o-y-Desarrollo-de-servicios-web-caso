var express = require('express'),
    app = express(),
    port = process.env.PORT || 3000,
    mongoose = require('mongoose'),
    bodyParser = require('body-parser');

// Importar modelos
var Libro = require('./api/models/LibroModel'),
    Usuario = require('./api/models/UsuarioModel');

// mongoose instance connection url connection
mongoose.Promise = global.Promise;
mongoose.connect('mongodb://localhost/autotech');

app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());

// Importar rutas
var libroRoutes = require('./api/routes/LibroRoutes');
var UsuarioRoutes = require('./api/routes/UsuarioRoutes');

// Registrar las rutas
libroRoutes(app);
UsuarioRoutes(app);

// Iniciar el servidor
app.listen(port);

console.log('Servidor para RESTful API de Libros iniciada en puerto 3000: ' + port);
