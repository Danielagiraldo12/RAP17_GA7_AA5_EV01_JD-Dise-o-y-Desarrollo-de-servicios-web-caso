#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

#define MONGOC_ENABLE_DEBUG_ASSERTIONS 1

#if MONGOC_ENABLE_DEBUG_ASSERTIONS != 1
#  undef MONGOC_ENABLE_DEBUG_ASSERTIONS
#endif

#endif
