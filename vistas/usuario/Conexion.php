<?php 
    require_once $_SERVER['DOCUMENT_ROOT'] . "/Autotech/vendor/autoload.php";

    class Conexion {
        public static function conectar() {
           try {
                /*$servidor = "127.0.0.1";
                $puerto = "27017";
                $usuario = "";
                $password = "";
                
                $cadenaConexion = "mongodb://localhost:27017/" . 
                                    $usuario . ":" . 
                                    $password . "@". 
                                    $servidor .":". 
                                    $puerto ."/". 
                                    $BD;*/

                $cadenaConexion = "mongodb://localhost:27017/";
                $BD = "autotech";                
                $cliente = new MongoDB\Client($cadenaConexion);
                return $cliente->selectDatabase($BD);
           } catch (\Throwable $th) {
            echo $th->getMessage();
               return $th->getMessage();
           }
        }
    }

?>