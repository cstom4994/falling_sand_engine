// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_CORE_H_
#define _METADOT_CORE_H_

#include <assert.h>
#include <stdint.h>

#include "core/macros.h"
#include "engine/log.h"

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
typedef int8_t I8;
typedef uint8_t U8;
typedef int16_t I16;
typedef uint16_t U16;
typedef int32_t I32;
typedef uint32_t U32;
typedef int64_t I64;
typedef uint64_t U64;
typedef float F32;
typedef double F64;

typedef unsigned char Byte;

typedef struct U16Point
{
    U16 x;
    U16 y;
} U16Point;

typedef struct Vector3
{
    float x;
    float y;
    float z;
} Vector3;

typedef struct Pixel
{
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
} Pixel;

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
