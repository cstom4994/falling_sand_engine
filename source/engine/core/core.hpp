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
#include "engine/utils/struct.hpp"
#include "engine/core/macros.hpp"

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

typedef struct Pixel {
    u8 b;
    u8 g;
    u8 r;
    u8 a;
} Pixel;

#pragma region endian_h

// MetaDot endian
// https://github.com/mikepb/endian.h

#if (defined(_WIN16) || defined(_WIN32) || defined(_WIN64)) && !defined(__WINDOWS__)

#define __WINDOWS__

#endif

#if defined(__linux__) || defined(__CYGWIN__)

#include <endian.h>

#elif defined(__APPLE__)

#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#define __BYTE_ORDER BYTE_ORDER
#define __BIG_ENDIAN BIG_ENDIAN
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#define __PDP_ENDIAN PDP_ENDIAN

#elif defined(__OpenBSD__)

#include <sys/endian.h>

#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)

#include <sys/endian.h>

#define be16toh(x) betoh16(x)
#define le16toh(x) letoh16(x)

#define be32toh(x) betoh32(x)
#define le32toh(x) letoh32(x)

#define be64toh(x) betoh64(x)
#define le64toh(x) letoh64(x)

#elif defined(__WINDOWS__)

#include <winsock2.h>

#if BYTE_ORDER == LITTLE_ENDIAN

#define htobe16(x) htons(x)
#define htole16(x) (x)
#define be16toh(x) ntohs(x)
#define le16toh(x) (x)

#define htobe32(x) htonl(x)
#define htole32(x) (x)
#define be32toh(x) ntohl(x)
#define le32toh(x) (x)

#define htobe64(x) htonll(x)
#define htole64(x) (x)
#define be64toh(x) ntohll(x)
#define le64toh(x) (x)

#elif BYTE_ORDER == BIG_ENDIAN

/* that would be xbox 360 */
#define htobe16(x) (x)
#define htole16(x) __builtin_bswap16(x)
#define be16toh(x) (x)
#define le16toh(x) __builtin_bswap16(x)

#define htobe32(x) (x)
#define htole32(x) __builtin_bswap32(x)
#define be32toh(x) (x)
#define le32toh(x) __builtin_bswap32(x)

#define htobe64(x) (x)
#define htole64(x) __builtin_bswap64(x)
#define be64toh(x) (x)
#define le64toh(x) __builtin_bswap64(x)

#else

#error byte order not supported

#endif

#define __BYTE_ORDER BYTE_ORDER
#define __BIG_ENDIAN BIG_ENDIAN
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#define __PDP_ENDIAN PDP_ENDIAN

#else

#error platform not supported

#endif

#pragma endregion endian_h

#pragma region engine_framework

#include <stdint.h>

#ifndef NOMINMAX
#define NOMINMAX WINDOWS_IS_ANNOYING_AINT_IT
#endif

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

#ifdef ME_PLATFORM_WINDOWS

#if !defined(_INITIALIZER_LIST_) && !defined(_INITIALIZER_LIST) && !defined(_LIBCPP_INITIALIZER_LIST)
#define _INITIALIZER_LIST_        // MSVC
#define _INITIALIZER_LIST         // GCC
#define _LIBCPP_INITIALIZER_LIST  // Clang
// Will probably need to add more here for more compilers later.

namespace std {
template <typename T>
class initializer_list {
public:
    using value_type = T;
    using reference = const T &;
    using const_reference = const T &;
    using size_type = size_t;

    using iterator = const T *;
    using const_iterator = const T *;

    constexpr initializer_list() noexcept : m_first(0), m_last(0) {}

    constexpr initializer_list(const T *first, const T *last) noexcept : m_first(first), m_last(last) {}

    constexpr const T *begin() const noexcept { return m_first; }
    constexpr const T *end() const noexcept { return m_last; }
    constexpr size_t size() const noexcept { return (size_t)(m_last - m_first); }

private:
    const T *m_first;
    const T *m_last;
};

template <class T>
constexpr const T *begin(initializer_list<T> list) noexcept {
    return list.begin();
}
template <class T>
constexpr const T *end(initializer_list<T> list) noexcept {
    return list.end();
}
}  // namespace std

#endif

#else  // ME_PLATFORM_WINDOWS

#include <initializer_list>

#endif  // ME_PLATFORM_WINDOWS

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

#pragma endregion engine_framework

#define METADOT_CALLABLE(func_name) [](auto... args) -> decltype(auto) { return func_name(std::move(args)...); }

// ImGuiData Types

struct MarkdownData {
    std::string data;
};
METADOT_STRUCT(MarkdownData, data);

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

#define MAKE_ENUM_FLAGS(TEnum, TBase)                                               \
    enum class TEnum;                                                               \
    inline TEnum operator~(TEnum a) {                                               \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        return static_cast<TEnum>(~static_cast<TUnder>(a));                         \
    }                                                                               \
    inline TEnum operator|(TEnum a, TEnum b) {                                      \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        return static_cast<TEnum>(static_cast<TUnder>(a) | static_cast<TUnder>(b)); \
    }                                                                               \
    inline TEnum operator&(TEnum a, TEnum b) {                                      \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        return static_cast<TEnum>(static_cast<TUnder>(a) & static_cast<TUnder>(b)); \
    }                                                                               \
    inline TEnum operator^(TEnum a, TEnum b) {                                      \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        return static_cast<TEnum>(static_cast<TUnder>(a) ^ static_cast<TUnder>(b)); \
    }                                                                               \
    inline TEnum &operator|=(TEnum &a, TEnum b) {                                   \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        a = static_cast<TEnum>(static_cast<TUnder>(a) | static_cast<TUnder>(b));    \
        return a;                                                                   \
    }                                                                               \
    inline TEnum &operator&=(TEnum &a, TEnum b) {                                   \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        a = static_cast<TEnum>(static_cast<TUnder>(a) & static_cast<TUnder>(b));    \
        return a;                                                                   \
    }                                                                               \
    inline TEnum &operator^=(TEnum &a, TEnum b) {                                   \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        a = static_cast<TEnum>(static_cast<TUnder>(a) ^ static_cast<TUnder>(b));    \
        return a;                                                                   \
    }                                                                               \
    enum class TEnum : TBase

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

#endif
