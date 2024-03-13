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

// Verifica si se recibió un ID de usuario válido
if (isset($_GET['id']) && !empty($_GET['id'])) {
    // Sanitiza y obtiene el ID del usuario
    $userId = $_GET['id'];

    // Prepara y ejecuta la consulta SQL para obtener los datos del usuario con el ID proporcionado
    $sql = "SELECT * FROM usuario WHERE id_usuario = $userId";
    $result = $conn->query($sql);

    if ($result->num_rows > 0) {
        // Si se encontraron resultados, devuelve los datos del usuario como un objeto JSON
        $row = $result->fetch_assoc();
        echo json_encode($row);
    } else {
        // Si no se encontraron resultados, devuelve un mensaje de error
        echo json_encode(array("message" => "No se encontró ningún usuario con el ID proporcionado."));
    }
} else {
    // Si no se proporcionó un ID de usuario válido, devuelve un mensaje de error
    echo json_encode(array("message" => "Se requiere un ID de usuario válido."));
}

$conn->close();
?>
