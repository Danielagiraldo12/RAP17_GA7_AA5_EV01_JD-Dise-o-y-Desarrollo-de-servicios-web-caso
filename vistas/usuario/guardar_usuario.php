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

// Recibe los datos del formulario
$id_usuario = $_POST['id_usuario'];
$cedula = $_POST['cedula'];
$nombres = $_POST['nombres'];
$apellidos = $_POST['apellidos'];
$celular = $_POST['celular'];
$correo = $_POST['correo'];
$direccion = $_POST['direccion'];
$id_tipo_documento = $_POST['id_tipo_documento'];
$id_perfil = $_POST['id_perfil'];
$id_estado = $_POST['id_estado'];
$login = $_POST['login'];
$password = $_POST['password'];

// Prepara y ejecuta la consulta SQL para insertar los datos en la tabla
if($id_usuario==""){
    //insertamos el usuario
    $sql = "INSERT INTO usuario (cedula, nombres, apellidos, celular , correo, direccion, id_tipo_documento, id_perfil, id_estado, login, password) VALUES ('$cedula', '$nombres', '$apellidos', '$celular', '$correo', '$direccion', $id_tipo_documento,$id_perfil,$id_estado,'$login', '$password')";

}else{
    //actualizamos el usuario
    $sql = " UPDATE usuario SET cedula='$cedula', nombres='$nombres', apellidos='$apellidos', celular='$celular', correo='$correo', direccion='$direccion', id_tipo_documento=$id_tipo_documento, id_perfil=$id_perfil, id_estado=$id_estado, login='$login', password='$password' WHERE id_usuario=$id_usuario";
}



if ($conn->query($sql) === TRUE) {
    echo "Registro de usuario exitoso";
} else {
    echo "Error: " . $sql . "<br>" . $conn->error;
}

$conn->close();
?>
