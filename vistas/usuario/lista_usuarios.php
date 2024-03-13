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

// Prepara y ejecuta la consulta SQL para obtener la lista de usuarios
//$sql = "SELECT *,  FROM usuario";
$sql = "SELECT u.*, p.descripcion_perfil, e.descripcion_estado   FROM usuario u
left join perfil p on u.id_perfil = p.id_perfil 
left join estado e on u.id_estado  = e.id_estado  ";
$result = $conn->query($sql);

// Verifica si se encontraron resultados
if ($result->num_rows > 0) {
    // Array para almacenar los usuarios
    $usuarios = array();

    // Recorre los resultados y agrega cada usuario al array
    while ($row = $result->fetch_assoc()) {
        $usuarios[] = $row;
    }

    // Devuelve la lista de usuarios como respuesta JSON
    echo json_encode($usuarios);
} else {
    echo "No se encontraron usuarios";
}

$conn->close();
?>
