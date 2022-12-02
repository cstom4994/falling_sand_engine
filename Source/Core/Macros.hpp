// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_MACROS_HPP_
#define _METADOT_MACROS_HPP_

#if defined(__cplusplus)

#include <string>

#if defined(__cpp_char8_t)
template<typename T>
const char *u8Cpp20(T &&t) noexcept {
#pragma warning(disable : 26490)
    return reinterpret_cast<const char *>(t);
#pragma warning(default : 26490)
}
#define U8(x) u8Cpp20(u8##x)
#else
#define U8(x) u8##x
#endif

#if defined(NDEBUG) && !defined(_DEBUG)
#define METADOT_RELEASE 1
#else
#define METADOT_DEBUG 1
#endif

#define METADOT_CONCAT_IMPL(x, y) x##y
#define METADOT_CONCAT(x, y) METADOT_CONCAT_IMPL(x, y)

#define INVOKE_ONCE(...)                                                                           \
    static char METADOT_CONCAT(unused, __LINE__) = [&]() {                                         \
        __VA_ARGS__;                                                                               \
        return '\0';                                                                               \
    }();                                                                                           \
    (void) METADOT_CONCAT(unused, __LINE__)

#define BOOL_STRING(b) (bool(b) ? "true" : "false")

#define METADOT_STRING_IMPL(x) #x
#define METADOT_STRING(x) METADOT_STRING_IMPL(x)

#define METADOT_MAKE_INTERNAL_TAG(tag) ("!" tag)
#define METADOT_INTERNAL_TAG_SYMBOL '!'

#define METADOT_MAKE_MOVEONLY(class_name)                                                          \
    class_name() = default;                                                                        \
    class_name(const class_name &) = delete;                                                       \
    class_name &operator=(const class_name &) = delete;                                            \
    class_name(class_name &&) = default;                                                           \
    class_name &operator=(class_name &&) = default

#define GENERATE_METHOD_CHECK(NAME, ...)                                                           \
    namespace Meta {                                                                               \
        template<typename T>                                                                       \
        class has_method_##NAME {                                                                  \
            template<typename U>                                                                   \
            constexpr static auto check(int) -> decltype(std::declval<U>().__VA_ARGS__, bool()) {  \
                return true;                                                                       \
            }                                                                                      \
            template<typename>                                                                     \
            constexpr static bool check(...) {                                                     \
                return false;                                                                      \
            }                                                                                      \
                                                                                                   \
        public:                                                                                    \
            static constexpr bool value = check<T>(0);                                             \
        };                                                                                         \
    }

// VS2013 doesn't support alignof
#if defined(_MSC_VER) && _MSC_VER <= 1800
#define METADOT_ALIGNOF(x) __alignof(x)
#else
#define METADOT_ALIGNOF(x) alignof(x)
#endif

#define ARRAYCOUNT(X) (sizeof(X) / sizeof(*(X)))

// Preferably, and ironically, this macro should go unused.
#ifndef METADOT_UNUSED
#define METADOT_UNUSED(x) (void) sizeof(x)
#endif

#define METADOT_NODISCARD [[nodiscard]]

#define METADOT_GET_PIXEL(surface, x, y)                                                           \
    *((UInt32 *) ((UInt8 *) surface->pixels + ((y) *surface->pitch) + ((x) * sizeof(UInt32))))

#define METADOT_OPTMIZE_OFF __pragma(optimize("", off))
#define METADOT_OPTMIZE_ON __pragma(optimize("", on))
#define METADOT_DEBUGBREAK __debugbreak()

#define METADOT_INLINE inline

#ifndef static_inline
#ifdef _MSC_VER
#define static_inline static
#else
#define static_inline static METADOT_INLINE
#endif
#endif

#endif

// Platforms Macros
#if defined(_WIN32) || defined(_WINDOWS)
#define METADOT_PLATFORM_WINDOWS
#elif defined(__linux)
#define METADOT_PLATFORM_LINUX
#elif defined(__APPLE__)
#define METADOT_PLATFORM_APPLE
#if defined(__METADOT_ARCH_ARM)
#define METADOT_PLATFORM_APPLE_ARM
#elif defined(__METADOT_ARCH_x86)
#define METADOT_PLATFORM_APPLE_64
#endif
#endif

#if defined(_MSC_VER)
#define METADOT_COMPILER_MSVC
#elif defined(__clang__)
#define METADOT_COMPILER_CLANG
#elif defined(__GNUC__)
#define METADOT_COMPILER_GCC
#endif

#endif