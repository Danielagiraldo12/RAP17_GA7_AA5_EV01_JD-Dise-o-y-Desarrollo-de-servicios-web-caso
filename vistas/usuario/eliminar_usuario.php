<?php
// Establece la conexión a la base de datos
$servername = "localhost";
$username = "root";
$password = "";
$dbname = "autotech";

$conn = new mysqli($servername, $username, $password, $dbname);

// Verifica la conexión
if ($conn->connect_error) {
    die("Conexión fallida: " . $conn->connect_error);
}

// Recibe el ID de usuario a eliminar
$idUsuario = $_GET['id'];

// Ejecuta la consulta SQL para eliminar el usuario
$sql = "DELETE FROM usuario WHERE id_usuario = $idUsuario";

if ($conn->query($sql) === TRUE) {
    // Si la consulta se ejecuta con éxito, devuelve un mensaje de éxito
    echo json_encode(['mensaje' => 'Usuario eliminado con éxito']);
} else {
    // Si ocurre un error, devuelve un mensaje de error
    echo json_encode(['mensaje' => 'Error al eliminar el usuario: ' . $conn->error]);
}

// Cierra la conexión a la base de datos
$conn->close();
?>
