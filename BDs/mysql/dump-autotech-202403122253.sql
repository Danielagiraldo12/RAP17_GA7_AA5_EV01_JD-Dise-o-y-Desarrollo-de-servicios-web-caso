-- MySQL dump 10.13  Distrib 8.0.19, for Win64 (x86_64)
--
-- Host: localhost    Database: autotech
-- ------------------------------------------------------
-- Server version	5.5.5-10.4.17-MariaDB

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `estado`
--

DROP TABLE IF EXISTS `estado`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `estado` (
  `id_estado` int(11) NOT NULL AUTO_INCREMENT COMMENT 'identificador de el estado de usuario en BD',
  `descripcion_estado` varchar(50) COLLATE utf8_bin DEFAULT NULL COMMENT 'descripcion de el estado',
  `eliminado` int(11) DEFAULT 0 COMMENT 'si el registro es esliminado 1 si  esta eliminado  y 0 si el registro  no a sido eliminado',
  PRIMARY KEY (`id_estado`)
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='tabla para describir los estados del usuario\r\n';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `estado`
--

LOCK TABLES `estado` WRITE;
/*!40000 ALTER TABLE `estado` DISABLE KEYS */;
INSERT INTO `estado` VALUES (1,'ACTIVO',0),(2,'INACTIVO',0);
/*!40000 ALTER TABLE `estado` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `perfil`
--

DROP TABLE IF EXISTS `perfil`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `perfil` (
  `id_perfil` int(11) NOT NULL AUTO_INCREMENT COMMENT 'identificador del perfil en la BD',
  `descripcion_perfil` varchar(50) COLLATE utf8_bin NOT NULL COMMENT 'descripcion del perfil',
  `eliminado` int(11) DEFAULT 0 COMMENT 'si el registro es esliminado 1 si  esta eliminado  y 0 si el registro  no a sido eliminado',
  PRIMARY KEY (`id_perfil`)
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='tabla  de perfiles de los usuarios del sistema';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `perfil`
--

LOCK TABLES `perfil` WRITE;
/*!40000 ALTER TABLE `perfil` DISABLE KEYS */;
INSERT INTO `perfil` VALUES (1,'ADMINISTRADOR',0),(2,'MECANICO',0),(3,'ALMACEN',0),(4,'VENDEDOR',0);
/*!40000 ALTER TABLE `perfil` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `tipodocumento`
--

DROP TABLE IF EXISTS `tipodocumento`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `tipodocumento` (
  `id_documento` int(11) NOT NULL AUTO_INCREMENT COMMENT 'identificador del perfil en la BD',
  `descripcion` varchar(50) COLLATE utf8_bin NOT NULL COMMENT 'descripcion del perfil',
  `eliminado` int(11) DEFAULT 0 COMMENT 'si el registro es esliminado 1 si  esta eliminado  y 0 si el registro  no a sido eliminado',
  PRIMARY KEY (`id_documento`)
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='tabla  de tipo documentos\r\n de los usuarios del sistema';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `tipodocumento`
--

LOCK TABLES `tipodocumento` WRITE;
/*!40000 ALTER TABLE `tipodocumento` DISABLE KEYS */;
INSERT INTO `tipodocumento` VALUES (1,'CEDULA',0),(2,'NUI',0);
/*!40000 ALTER TABLE `tipodocumento` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `usuario`
--

DROP TABLE IF EXISTS `usuario`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `usuario` (
  `id_usuario` int(11) NOT NULL AUTO_INCREMENT COMMENT 'identificador del usuario en la BD',
  `cedula` int(11) NOT NULL COMMENT 'cedula del usuario',
  `nombres` varchar(60) COLLATE utf8_bin NOT NULL COMMENT 'nombre del usuario',
  `apellidos` varchar(200) COLLATE utf8_bin DEFAULT NULL,
  `celular` varchar(30) COLLATE utf8_bin NOT NULL COMMENT 'celular del usuario',
  `correo` varchar(300) COLLATE utf8_bin DEFAULT NULL,
  `direccion` varchar(200) COLLATE utf8_bin DEFAULT NULL,
  `id_tipo_documento` int(11) DEFAULT 1,
  `id_perfil` int(11) NOT NULL COMMENT 'identificador del perfil del usuario',
  `id_estado` int(11) NOT NULL DEFAULT 1 COMMENT 'identificador del estado del usuario',
  `login` varchar(60) COLLATE utf8_bin NOT NULL COMMENT 'login del usuario',
  `password` varchar(60) COLLATE utf8_bin NOT NULL COMMENT 'password del usuario',
  `eliminado` int(11) DEFAULT 0 COMMENT 'si el registro es esliminado 1 si  esta eliminado  y 0 si el registro  no a sido eliminado',
  PRIMARY KEY (`id_usuario`)
) ENGINE=InnoDB AUTO_INCREMENT=12 DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='tabla para el manejo de usuario en el sistema';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `usuario`
--

LOCK TABLES `usuario` WRITE;
/*!40000 ALTER TABLE `usuario` DISABLE KEYS */;
INSERT INTO `usuario` VALUES (4,1231231231,'ARMANDO','LOPEZ','1231231231','SDFSDFSD','SDFSDFSD',2,3,2,'admin@gmail.com','1234',0),(5,1053849619,'DANIELA','GIRALDO DUQUE','3137951224','GIRALDODANIELA1296@GMAIL.COM','CARRERA 19A # 5-33',1,3,1,'admin@gmail.com','1234566666',0),(8,1234567890,'SOFIA','RAMIREZ','1234567890','micoreo@correo','CALLE 10 CASA 8',1,1,1,'admin@gmail.com','admin@gmail.com',0),(10,1053849758,'MARGARITA','ZULETA','3128866597','MARGAZULE@GMAIL.COM','CARRERA 45 # 57-102',1,3,1,'margazule','2222255555',0),(11,1053789456,'DAVID','LOAIZA LOPEZ','3137598467','DALOLO@GMAIL.COM','CARRERA 45 # 57-102',1,4,1,'dalolo','2222333333',0);
/*!40000 ALTER TABLE `usuario` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping events for database 'autotech'
--

--
-- Dumping routines for database 'autotech'
--
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2024-03-12 22:53:26
