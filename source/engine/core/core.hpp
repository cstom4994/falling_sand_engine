// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_CORE_HPP
#define ME_CORE_HPP

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
#include <iomanip>
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

#include "engine/core/basic_types.h"
#include "engine/core/macros.hpp"
#include "engine/utils/struct.hpp"

#define METADOT_INT8_MAX 0x7F
#define METADOT_UINT8_MAX 0xFF
#define METADOT_INT16_MAX 0x7FFF
#define METADOT_UINT16_MAX 0xFFFF
#define METADOT_INT32_MAX 0x7FFFFFFF
#define METADOT_UINT32_MAX 0xFFFFFFFF
#define METADOT_INT64_MAX 0x7FFFFFFFFFFFFFFF
#define METADOT_UINT64_MAX 0xFFFFFFFFFFFFFFFF

#define METADOT_OK 0
#define METADOT_FAILED 1

#ifdef ME_PLATFORM_WINDOWS
#define METADOT_CDECL __cdecl
#else
#define METADOT_CDECL
#endif

#define ME_UNUSED(x) (void)x
#define ME_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define ME_KB 1024
#define ME_MB (ME_KB * ME_KB)
#define ME_GB (ME_MB * ME_MB)
#define ME_SERIALIZE_CHECK(x)                          \
    do {                                               \
        if ((x) != SERIALIZE_SUCCESS) goto cute_error; \
    } while (0)
#define ME_STATIC_ASSERT(condition, error_message_string) static_assert(condition, error_message_string)
#define ME_STRINGIZE_INTERNAL(...) #__VA_ARGS__
#define ME_STRINGIZE(...) ME_STRINGIZE_INTERNAL(__VA_ARGS__)
#define ME_OFFSET_OF(T, member) ((size_t)((uintptr_t)(&(((T *)0)->member))))
#define ME_DEBUG_PRINTF(...)
#define ME_ALIGN_TRUNCATE(v, n) ((v) & ~((n)-1))
#define ME_ALIGN_FORWARD(v, n) ME_ALIGN_TRUNCATE((v) + (n)-1, (n))
#define ME_ALIGN_TRUNCATE_PTR(p, n) ((void *)ME_ALIGN_TRUNCATE((uintptr_t)(p), n))
#define ME_ALIGN_FORWARD_PTR(p, n) ((void *)ME_ALIGN_FORWARD((uintptr_t)(p), n))

#define METADOT_CALLABLE(func_name) [](auto... args) -> decltype(auto) { return func_name(std::move(args)...); }

#if defined(IGNORE_DEPRECATED_WARNING)
#define ME_DEPRECATED_
#elif defined(_MSC_VER)
#define ME_DEPRECATED_ __declspec(deprecated)
#elif defined(__GNUC__)
#define ME_DEPRECATED_ __attribute__((deprecated))
#else
#define ME_DEPRECATED_
#endif

#define ME_OVERRIDE_ override

namespace ME {

typedef struct Pixel {
    u8 b;
    u8 g;
    u8 r;
    u8 a;
} Pixel;

// ImGuiData Types
struct MarkdownData {
    std::string data;
};

// Not sure where to put this... Here is good I guess.
ME_INLINE uint64_t metadot_fnv1a(const void *data, int size) {
    const char *s = (const char *)data;
    uint64_t h = 14695981039346656037ULL;
    char c = 0;
    while (size--) {
        h = h ^ (uint64_t)(*s++);
        h = h * 1099511628211ULL;
    }
    return h;
}

// A portable and safe way to add byte offset to any pointer
// https://stackoverflow.com/questions/15934111/portable-and-safe-way-to-add-byte-offset-to-any-pointer
template <typename T>
inline void addOffset(std::ptrdiff_t offset, T *&ptr) {
    if (!ptr) return;
    ptr = (T *)((unsigned char *)ptr + offset);
}

template <typename T>
inline T *addOffsetR(std::ptrdiff_t offset, T *ptr) {
    if (!ptr) return nullptr;
    return (T *)((unsigned char *)ptr + offset);
}

}  // namespace ME

#endif
