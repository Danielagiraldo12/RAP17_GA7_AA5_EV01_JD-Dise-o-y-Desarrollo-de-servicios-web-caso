--TEST--
Decimal128: [basx263] Numbers with E
--DESCRIPTION--
Generated by scripts/convert-bson-corpus-tests.php

DO NOT EDIT THIS FILE
--FILE--
<?php

require_once __DIR__ . '/../utils/basic.inc';

$canonicalBson = hex2bin('18000000136400F104000000000000000000000000603000');
$canonicalExtJson = '{"d" : {"$numberDecimal" : "1.265E+19"}}';
$degenerateExtJson = '{"d" : {"$numberDecimal" : "0.1265E+20"}}';

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
18000000136400f104000000000000000000000000603000
{"d":{"$numberDecimal":"1.265E+19"}}
18000000136400f104000000000000000000000000603000
18000000136400f104000000000000000000000000603000
===DONE===