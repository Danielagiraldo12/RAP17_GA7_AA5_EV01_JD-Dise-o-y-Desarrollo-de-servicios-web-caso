/*
 * Copyright 2014-present MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <php.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "phongo_compat.h"
#include "php_phongo.h"

zend_class_entry* php_phongo_writeexception_ce;

/* {{{ proto MongoDB\Driver\WriteResult MongoDB\Driver\Exception\WriteException::getWriteResult()
   Returns the WriteResult from the failed write operation. */
static PHP_METHOD(WriteException, getWriteResult)
{
	zend_error_handling error_handling;
	zval*               writeresult;
	zval                rv;

	zend_replace_error_handling(EH_THROW, phongo_exception_from_phongo_domain(PHONGO_ERROR_INVALID_ARGUMENT), &error_handling);
	if (zend_parse_parameters_none() == FAILURE) {
		zend_restore_error_handling(&error_handling);
		return;
	}
	zend_restore_error_handling(&error_handling);

	writeresult = zend_read_property(php_phongo_writeexception_ce, PHONGO_COMPAT_OBJ_P(getThis()), ZEND_STRL("writeResult"), 0, &rv);

	RETURN_ZVAL(writeresult, 1, 0);
} /* }}} */

/* {{{ MongoDB\Driver\Exception\WriteException function entries */
ZEND_BEGIN_ARG_INFO_EX(ai_WriteException_void, 0, 0, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_phongo_writeexception_me[] = {
	/* clang-format off */
	PHP_ME(WriteException, getWriteResult, ai_WriteException_void, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
	/* clang-format on */
};
/* }}} */

void php_phongo_writeexception_init_ce(INIT_FUNC_ARGS) /* {{{ */
{
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "MongoDB\\Driver\\Exception", "WriteException", php_phongo_writeexception_me);
	php_phongo_writeexception_ce = zend_register_internal_class_ex(&ce, php_phongo_serverexception_ce);
	php_phongo_writeexception_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

	zend_declare_property_null(php_phongo_writeexception_ce, ZEND_STRL("writeResult"), ZEND_ACC_PROTECTED);
} /* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
