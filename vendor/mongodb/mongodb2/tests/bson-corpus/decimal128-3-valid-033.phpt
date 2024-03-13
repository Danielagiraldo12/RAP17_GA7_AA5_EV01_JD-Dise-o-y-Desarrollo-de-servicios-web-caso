--TEST--
Decimal128: [basx130] Numbers with E
--DESCRIPTION--
Generated by scripts/convert-bson-corpus-tests.php

DO NOT EDIT THIS FILE
--FILE--
<?php

require_once __DIR__ . '/../utils/basic.inc';

$canonicalBson = hex2bin('180000001364000000000000000000000000000000383000');
$canonicalExtJson = '{"d" : {"$numberDecimal" : "0.0000"}}';
$degenerateExtJson = '{"d" : {"$numberDecimal" : "0.000E-1"}}';

// Canonical BSON -> Native -> Canonical BSON
echo bin2hex(fromPHP(toPHP($canonicalBson))), "\n";

// Canonical BSON -> Canonical extJSON
echo json_canonicalize(toCanonicalExtendedJSON($canonicalBson)), "\n";

// Canonical extJSON -> Canonical BSON
echo bin2hex(fromJSON($canonicalExtJson)), "\n";

// Degenerate extJSON -> Canonical BSON
echo bin2hex(fromJSON($degenerateExtJson)), "\n";

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
180000001364000000000000000000000000000000383000
{"d":{"$numberDecimal":"0.0000"}}
180000001364000000000000000000000000000000383000
180000001364000000000000000000000000000000383000
===DONE===