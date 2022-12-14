// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_CORE_H_
#define _METADOT_CORE_H_

#include <assert.h>
#include <stdint.h>

#include "Core/Macros.h"
#include "Engine/Logging.h"

#define METADOT_INT8_MAX 0x7F
#define METADOT_UINT8_MAX 0xFF
#define METADOT_INT16_MAX 0x7FFF
#define METADOT_UINT16_MAX 0xFFFF
#define METADOT_INT32_MAX 0x7FFFFFFF
#define METADOT_UINT32_MAX 0xFFFFFFFF
#define METADOT_INT64_MAX 0x7FFFFFFFFFFFFFFF
#define METADOT_UINT64_MAX 0xFFFFFFFFFFFFFFFF

#define METADOT_OK 0
#define METADOT_FAILED -1

typedef int R_bool;
#define R_false 0
#define R_true 1
#define R_null NULL

// Num types
typedef int8_t Int8;
typedef uint8_t UInt8;
typedef int16_t Int16;
typedef uint16_t UInt16;
typedef int32_t Int32;
typedef uint32_t UInt32;
typedef int64_t Int64;
typedef uint64_t UInt64;
typedef float Float32;
typedef double Float64;

typedef unsigned char Byte;

//--------------------------------------------------------------------------------------------------------------------------------//
// LOGGING FUNCTIONS

#if defined(METADOT_DEBUG)
#define METADOT_BUG(...) log_debug(__VA_ARGS__)
#else
#define METADOT_BUG(...)
#endif
#define METADOT_TRACE(...) log_trace(__VA_ARGS__)
#define METADOT_INFO(...) log_info(__VA_ARGS__)
#define METADOT_WARN(...) log_warn(__VA_ARGS__)
#define METADOT_ERROR(...) log_error(__VA_ARGS__)
#define METADOT_LOG_SCOPE_FUNCTION(_c)
#define METADOT_LOG_SCOPE_F(...)

#define METADOT_ASSERT(x, _c) assert(x)
#define METADOT_ASSERT_E(x) assert(x)

#endif