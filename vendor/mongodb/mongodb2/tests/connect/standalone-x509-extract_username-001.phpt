--TEST--
Connect to MongoDB with SSL and X509 auth and username retrieved from cert
--XFAIL--
parse_url() tests must be reimplemented (PHPC-1177)
--SKIPIF--
<?php require __DIR__ . "/../utils/basic-skipif.inc"; ?>
<?php skip_if_not_libmongoc_ssl(['OpenSSL', 'Secure Transport', 'Secure Channel']); ?>
<?php skip_if_not_ssl(); ?>
<?php skip_if_no_ssl_dir(); ?>
<?php skip_if_not_auth_mechanism('MONGODB-X509'); ?>
--FILE--
<?php
require_once __DIR__ . "/../utils/basic.inc";

$driverOptions = [
    // libmongoc does not allow the hostname to be overridden as "server"
    'allow_invalid_hostname' => true,
    'weak_cert_validation' => false,
    'ca_file' => SSL_DIR . '/ca.pem',
    'pem_file' => SSL_DIR . '/client.pem',
];

$uriOptions = ['authMechanism' => 'MONGODB-X509', 'ssl' => true];

$parsed = parse_url(URI);
$uri = sprintf('mongodb://%s:%d', $parsed['host'], $parsed['port']);

$manager = create_test_manager($uri, $uriOptions, $driverOptions);
$cursor = $manager->executeCommand(DATABASE_NAME, new MongoDB\Driver\Command(['ping' => 1]));
var_dump($cursor->toArray()[0]);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
object(stdClass)#%d (%d) {
  ["ok"]=>
  float(1)
}
===DONE===
