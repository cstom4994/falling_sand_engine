

#ifndef _ME_MACROS_HPP
#define _ME_MACROS_HPP

#if defined(NDEBUG) && !defined(_DEBUG)
#define ME_RELEASE
#else
#define ME_DEBUG
#endif

#define ME_INLINE inline

#ifndef static_inline
#ifdef _MSC_VER
#define static_inline static
#else
#define static_inline static ME_INLINE
#endif
#endif

#ifdef _MSC_VER
#define ME_NOINLINE(...) __declspec(noinline) __VA_ARGS__
#else
#define ME_NOINLINE(...) __VA_ARGS__ __attribute__((noinline))
#endif

#if defined(__GNUC__) || defined(__clang__)
#define ME_FORCE_INLINE __attribute__((always_inline)) inline
#ifdef __cplusplus
#define ME_RESTRICT __restrict
#else
#define ME_RESTRICT restrict
#endif  // __cplusplus
#elif defined(_MSC_VER)
#define ME_FORCE_INLINE __forceinline
#define ME_RESTRICT __restrict
#else
#define ME_FORCE_INLINE inline
#ifdef __cplusplus
#define ME_RESTRICT
#else
#define ME_RESTRICT restrict
#endif  // __cplusplus
#endif

// VS2013 doesn't support alignof
#if defined(_MSC_VER) && _MSC_VER <= 1800
#define ME_ALIGNOF(x) __alignof(x)
#else
#define ME_ALIGNOF(x) alignof(x)
#endif

#define ARRAYCOUNT(X) (sizeof(X) / sizeof(*(X)))
#define ME_UNUSED(x) (void)sizeof(x)
#define ME_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ME_CONCAT_IMPL(x, y) x##y
#define ME_CONCAT(x, y) ME_CONCAT_IMPL(x, y)

#define INVOKE_ONCE(...)                              \
    static char ME_CONCAT(unused, __LINE__) = [&]() { \
        __VA_ARGS__;                                  \
        return '\0';                                  \
    }();                                              \
    (void)ME_CONCAT(unused, __LINE__)

#define BOOL_STRING(b) (bool(b) ? "true" : "false")

#define ME_STRING_IMPL(x) #x
#define ME_STRING(x) ME_STRING_IMPL(x)
#define ME_VARIABLE_NAME(x) ME_STRING_IMPL(x)

#define ME_MAKE_INTERNAL_TAG(tag) ("!" tag)
#define ME_INTERNAL_TAG_SYMBOL '!'

#ifdef __GNUC__
#define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK(__Declaration__) __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#endif

#define ME_NODISCARD [[nodiscard]]

#define ME_OPTMIZE_OFF __pragma(optimize("", off))
#define ME_OPTMIZE_ON __pragma(optimize("", on))
#define ME_DEBUGBREAK __debugbreak()

#if defined(IGNORE_DEPRECATED_WARNING)
#define ME_DEPRECATED
#elif defined(_MSC_VER)
#define ME_DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__)
#define ME_DEPRECATED __attribute__((deprecated))
#else
#define ME_DEPRECATED
#endif

// Platforms Macros
#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__)
#define ME_PLATFORM_WINDOWS

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#elif defined(__linux__) || defined(linux)
#define ME_PLATFORM_LINUX
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__) || defined(ME_PLATFORM_WINDOWS)
#define ME_PLATFORM_WIN32
#else
#define ME_PLATFORM_POSIX (ME_PLATFORM_LINUX || ME_PLATFORM_APPLE || 0)
#endif

#if defined(_MSC_VER)
#define ME_COMPILER_MSVC
#define __func__ __FUNCTION__
#elif defined(__clang__)
#define ME_COMPILER_CLANG
#elif defined(__GNUC__)
#define ME_COMPILER_GCC
#endif

#if __cplusplus >= 201103L
#define ME_THREADLOCAL thread_local
#elif __STDC_VERSION_ >= 201112L
#define ME_THREADLOCAL _Thread_local
#elif defined(ME_COMPILER_GCC) || defined(ME_COMPILER_CLANG)
#define ME_THREADLOCAL __thread
#elif defined(ME_COMPILER_MSVC)
#define ME_THREADLOCAL __declspec(thread)
#endif

// Vista and later only. This helps MingW builds.
#ifdef ME_PLATFORM_WINDOWS
#include <sdkddkver.h>
#ifdef _WIN32_WINNT
#if _WIN32_WINNT < 0x0600
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#endif
#endif

#ifdef ME_PLATFORM_WINDOWS
#define METADOT_CDECL __cdecl
#else
#define METADOT_CDECL
#endif

#define ME_LITTLE_ENDIAN 1

#define BIT(x) (1 << x)

#define ME_BIND_EVENT_FN(fn) [=](auto &&...args) -> decltype(auto) { return fn(std::forward<decltype(args)>(args)...); }

#define ME_MAKE_MOVEONLY(class_name)                    \
    class_name(const class_name &) = delete;            \
    class_name &operator=(const class_name &) = delete; \
    class_name(class_name &&) = default;                \
    class_name &operator=(class_name &&) = default

#define GENERATE_METHOD_CHECK(NAME, ...)                                                      \
    namespace ME::meta {                                                                      \
    template <typename T>                                                                     \
    class has_method_##NAME {                                                                 \
        template <typename U>                                                                 \
        constexpr static auto check(int) -> decltype(std::declval<U>().__VA_ARGS__, bool()) { \
            return true;                                                                      \
        }                                                                                     \
        template <typename>                                                                   \
        constexpr static bool check(...) {                                                    \
            return false;                                                                     \
        }                                                                                     \
                                                                                              \
    public:                                                                                   \
        static constexpr bool value = check<T>(0);                                            \
    };                                                                                        \
    }

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

#define ME_INT8_MAX 0x7F
#define ME_UINT8_MAX 0xFF
#define ME_INT16_MAX 0x7FFF
#define ME_UINT16_MAX 0xFFFF
#define ME_INT32_MAX 0x7FFFFFFF
#define ME_UINT32_MAX 0xFFFFFFFF
#define ME_INT64_MAX 0x7FFFFFFFFFFFFFFF
#define ME_UINT64_MAX 0xFFFFFFFFFFFFFFFF

#define METADOT_OK 0
#define METADOT_FAILED -1

#ifdef ME_PLATFORM_WINDOWS
#define ME_ASSERT SDL_assert
#else
#define ME_ASSERT assert
#endif

#define ME_PRIVATE(_result_type) static _result_type
#define ME_PUBLIC(_result_type) _result_type

#define DD_ALIGNED_BUFFER(name) alignas(16) static const std::uint8_t name[]

#if defined(__cplusplus)
#include <string>
#if defined(__cpp_char8_t)
template <typename T>
const char *u8Cpp20(T &&t) noexcept {
#pragma warning(disable : 26490)
    return reinterpret_cast<const char *>(t);
#pragma warning(default : 26490)
}
#define CC(x) u8Cpp20(u8##x)
#else
#define CC(x) u8##x
#endif
#else
#define CC(x) x
#endif

#endif