--TEST--
Decimal128: [decq012] derivative canonical plain strings
--DESCRIPTION--
Generated by scripts/convert-bson-corpus-tests.php

DO NOT EDIT THIS FILE
--FILE--
<?php

require_once __DIR__ . '/../utils/basic.inc';

$canonicalBson = hex2bin('18000000136400EE0200000000000000000000000038B000');
$canonicalExtJson = '{"d" : {"$numberDecimal" : "-0.0750"}}';

// Canonical BSON -> Native -> Canonical BSON
echo bin2hex(fromPHP(toPHP($canonicalBson))), "\n";

// Canonical BSON -> Canonical extJSON
echo json_canonicalize(toCanonicalExtendedJSON($canonicalBson)), "\n";

// Canonical extJSON -> Canonical BSON
echo bin2hex(fromJSON($canonicalExtJson)), "\n";

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
18000000136400ee0200000000000000000000000038b000
{"d":{"$numberDecimal":"-0.0750"}}
18000000136400ee0200000000000000000000000038b000
===DONE===