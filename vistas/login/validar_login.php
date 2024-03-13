<?php

// Verifica si se enviaron los datos del formulario
if ($_SERVER["REQUEST_METHOD"] == "POST") {
    // Verifica si se recibieron el correo y la contraseña
    if (isset($_POST['email']) && isset($_POST['password'])) {
        // Sanitiza los datos recibidos del formulario
        $correo = $_POST['email'];
        $pass = $_POST['password'];


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

       

        // Prepara y ejecuta la consulta SQL para obtener los datos del usuario con el ID proporcionado
        /*$sql = "select * from usuario where login='$correo' and password ='$pass'";
        $result = $conn->query($sql);
        if ($result->num_rows > 0) {
            header("Location: ../../vistas/inicio/inicio.html");
            exit();
        } else {
            // Si no se encontraron resultados, devuelve un mensaje de error
            echo "No se encontro ningun usuario En BD.";
        }
        exit();*/
        
        try {


            $url="http://localhost:3000/usuario/login/".$correo."/".$pass."";
            $llamado=file_get_contents($url, true);            
            $datos = json_decode($llamado);
            
            //echo $datos;
             //if ($result->num_rows > 0) {
                if(isset($datos->_id) && !is_null($datos->_id)){ 
                    // Si se encontraron resultados, devuelve los datos del usuario como un objeto JSON
                    header("Location: ../../vistas/inicio/inicio.html");
                    exit();
                } else {
                    // Si no se encontraron resultados, devuelve un mensaje de error
                    echo $datos->message;
                }
        } catch (Exception $e) {
            echo "Error: " . $e->getMessage();
        }

           
    } else {
        // Si no se proporcionó un ID de usuario válido, devuelve un mensaje de error
        echo json_encode(array("message" => "Se requiere un ID de usuario válido."));
    }

    $conn->close();

        
        // Realiza la lógica de validación contra la base de datos aquí
        // Por ejemplo, puedes utilizar consultas SQL para buscar el usuario en la tabla de usuarios

        // Si la validación es exitosa, redirige al usuario a la página de inicio
       
    }

?>
