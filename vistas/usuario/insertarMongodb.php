<?php
include "Conexion.php";
include "Crud.php";

$Crud = new Crud();
$datos = array( "id_usuario"=>$_POST['id_usuario'],
                "cedula"=>$_POST['cedula'],
                "nombres"=>$_POST['nombres'],
                "apellidos"=>$_POST['apellidos'],
                "celular"=>$_POST['celular'],
                "correo"=>$_POST['correo'],
                "direccion"=>$_POST['direccion'],
                "id_tipo_documento"=>$_POST['id_tipo_documento'],
                "id_perfil"=>$_POST['id_perfil'],
                "id_estado"=>$_POST['id_estado'],
                "login"=>$_POST['login'],
                "id_perfil"=>$_POST['id_perfil'],
                "id_perfil"=>$_POST['id_perfil'],
                "password"=>$_POST['password']
            );
            try {
            
            $respuesta =$Crud->insertarDatos($datos);
           
            if ($respuesta instanceof MongoDB\InsertOneResult && $respuesta->getInsertedId() !== null) {
                // Insertado con éxito
                header("location:usuarios.html");
            } else {
                // Error al insertar
                echo "Error al insertar datos.";
            }
        } catch (\Throwable $th) {
            echo "Error: " . $th->getMessage();
        }