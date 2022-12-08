// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_CORE_HPP_
#define _METADOT_CORE_HPP_

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <istream>
#include <iterator>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Core/Macros.hpp"
#include "Engine/Logging.hpp"
#include "Libs/VisitStruct.hpp"

#define METADOT_INT8_MAX 0x7F
#define METADOT_UINT8_MAX 0xFF
#define METADOT_INT16_MAX 0x7FFF
#define METADOT_UINT16_MAX 0xFFFF
#define METADOT_INT32_MAX 0x7FFFFFFF
#define METADOT_UINT32_MAX 0xFFFFFFFF
#define METADOT_INT64_MAX 0x7FFFFFFFFFFFFFFF
#define METADOT_UINT64_MAX 0xFFFFFFFFFFFFFFFF

#define METADOT_STRUCT VISITABLE_STRUCT

#define METADOT_CALLABLE(func_name)                                                                \
    [](auto... args) -> decltype(auto) { return func_name(std::move(args)...); }

#define METADOT_OK 0
#define METADOT_FAILED -1

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

template<typename T, size_t Size>
class SizeChecker {
    static_assert(sizeof(T) == Size, "Check the size of integral types");
};

template class SizeChecker<Int64, 8>;

template class SizeChecker<Int32, 4>;

template class SizeChecker<Int16, 2>;

template class SizeChecker<Int8, 1>;

template class SizeChecker<UInt64, 8>;

template class SizeChecker<UInt32, 4>;

template class SizeChecker<UInt16, 2>;

template class SizeChecker<UInt8, 1>;

namespace MetaEngine {

    static inline UInt16 swapuint16(UInt16 x) { return (x >> 8) | (x << 8); }

    static inline UInt32 swapuint32(UInt32 x) {
        return ((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8) | ((x & 0x00FF0000) >> 8) |
               ((x & 0xFF000000) >> 24);
    }

    static inline UInt64 swapuint64(UInt64 x) {
        return ((x << 56) & 0xFF00000000000000ULL) | ((x << 40) & 0x00FF000000000000ULL) |
               ((x << 24) & 0x0000FF0000000000ULL) | ((x << 8) & 0x000000FF00000000ULL) |
               ((x >> 8) & 0x00000000FF000000ULL) | ((x >> 24) & 0x0000000000FF0000ULL) |
               ((x >> 40) & 0x000000000000FF00ULL) | ((x >> 56) & 0x00000000000000FFULL);
    }

    // Function types

    template<typename T>
    using MetaFunction = std::function<T>;
    using VoidFunction = std::function<void(void)>;
    using AnyEventCallback = std::function<bool(void *backendEvent)>;

}// namespace MetaEngine

#include <fmt/format.h>
#include <fmt/printf.h>

namespace MetaEngine {
    template<typename S, typename... Args, typename Char = fmt::char_t<S>>
    inline std::basic_string<Char> Format(const S &formatStr, Args &&...args) {
        return fmt::format(formatStr, std::forward<Args>(args)...);
    }
}// namespace MetaEngine

// ImGuiData Types

struct MarkdownData
{
    std::string data;
};
METADOT_STRUCT(MarkdownData, data);

#if defined(IGNORE_DEPRECATED_WARNING)
#define _METADOT_DEPRECATED_
#elif defined(_MSC_VER)
#define _METADOT_DEPRECATED_ __declspec(deprecated)
#elif defined(__GNUC__)
#define _METADOT_DEPRECATED_ __attribute__((deprecated))
#else
#define _METADOT_DEPRECATED_
#endif

#define _METADOT_OVERRIDE_ override

//--------------------------------------------------------------------------------------------------------------------------------//
// MEMORY FUNCTIONS USED BY LIBRARY

#ifndef METAENGINE_MALLOC
#define METAENGINE_MALLOC(s) METADOT_GC_ALLOC((s))
#endif

#ifndef METAENGINE_FREE
#define METAENGINE_FREE(p) METADOT_GC_DEALLOC((p))
#endif

#ifndef METAENGINE_REALLOC
#define METAENGINE_REALLOC(p, s) METADOT_GC_REALLOC(p, s)
#endif

//--------------------------------------------------------------------------------------------------------------------------------//
// LOGGING FUNCTIONS

#define METADOT_BUG(...) DLOG_F(1, __VA_ARGS__)
#define METADOT_TRACE(...) LOG_F(2, __VA_ARGS__)
#define METADOT_INFO(...) LOG_F(INFO, __VA_ARGS__)
#define METADOT_WARN(...) LOG_F(WARNING, __VA_ARGS__)
#define METADOT_ERROR(...) LOG_F(ERROR, __VA_ARGS__)
#define METADOT_LOG_SCOPE_FUNCTION(_c) LOG_SCOPE_FUNCTION(_c)
#define METADOT_LOG_SCOPE_F(...) LOG_SCOPE_F(__VA_ARGS__)

namespace MetaEngine {

    template<class T>
    using Ref = std::shared_ptr<T>;

    template<class _Ty, class... _Types>
    Ref<_Ty> MakeRef(_Types &&..._Args) {
        // make a shared_ptr
        return std::make_shared<_Ty>(std::forward<_Types>(_Args)...);
    }

}// namespace MetaEngine

#endif// !_CORE_H