// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.

#pragma once


#define METADOT_NAME "MetaDot"
#define METADOT_VERSION_TEXT "0.0.2"
#define METADOT_VERSION_MAJOR 0
#define METADOT_VERSION_MINOR 0
#define METADOT_VERSION_BUILD 2
#define METADOT_VERSION Version(METADOT_VERSION_MAJOR, METADOT_VERSION_MINOR, METADOT_VERSION_BUILD)
#define METADOT_COMPANY "MetaDot"
#define METADOT_COPYRIGHT "Copyright (c) 2019-2022 KaoruXun. All rights reserved."

static const int VERSION_MAJOR = METADOT_VERSION_MAJOR;
static const int VERSION_MINOR = METADOT_VERSION_MINOR;
static const int VERSION_REV = METADOT_VERSION_BUILD;
static const char* VERSION = METADOT_VERSION_TEXT;
static const char* VERSION_COMPATIBILITY[] = { VERSION, "0.0.1", 0 };

namespace MetaEngine
{
    int meko_buildnum(void);
    const char* meko_buildos(void);
    const char* meko_buildarch(void);
    const char* meko_buildcommit(void);
}

#define BUILD_SERIES_NAME "local build"
#define BUILD_ID "buildid"
#define BUILD_COMMIT_ID "Unknown"
#define BUILD_DATETIME "Unknown"

#if defined(NDEBUG) && !defined(_DEBUG)
#define METADOT_RELEASE
#else 
#define METADOT_DEBUG
#endif

#define METADOT_CONCAT_IMPL(x, y) x##y
#define METADOT_CONCAT(x, y) METADOT_CONCAT_IMPL(x, y)

#define INVOKE_ONCE(...) static char METADOT_CONCAT(unused, __LINE__) = [&](){ __VA_ARGS__; return '\0'; }(); (void)METADOT_CONCAT(unused, __LINE__)

#define BOOL_STRING(b) (bool(b) ? "true" : "false")

#define METADOT_STRING_IMPL(x) #x
#define METADOT_STRING(x) METADOT_STRING_IMPL(x)

#define METADOT_MAKE_INTERNAL_TAG(tag) ("!" tag)
#define METADOT_INTERNAL_TAG_SYMBOL '!'

#define METADOT_MAKE_MOVEONLY(class_name) class_name() = default; class_name(const class_name&) = delete; class_name& operator=(const class_name&) = delete;\
        class_name(class_name&&) = default; class_name& operator=(class_name&&) = default

#define GENERATE_METHOD_CHECK(NAME, ...) namespace MetaEngine { template<typename T> class has_method_##NAME {\
    template<typename U> constexpr static auto check(int) -> decltype(std::declval<U>().__VA_ARGS__, bool()) { return true; }\
    template<typename> constexpr static bool check(...) { return false; } public:\
    static constexpr bool value = check<T>(0); }; }

// VS2013 doesn't support alignof
#if defined(_MSC_VER) && _MSC_VER <= 1800
#define METADOT_ALIGNOF(x) __alignof(x)
#else
#define METADOT_ALIGNOF(x) alignof(x)
#endif

#define ARRAYCOUNT(X) (sizeof(X) / sizeof(*(X)))

// Preferably, and ironically, this macro should go unused.
#ifndef METADOT_UNUSED
#	define METADOT_UNUSED(x) (void)sizeof(x)
#endif

#define METADOT_NODISCARD [[nodiscard]]

#define METADOT_GET_PIXEL(surface, x, y) *((Uint32*)( (Uint8*)surface->pixels + ((y) * surface->pitch) + ((x) * sizeof(Uint32))) )

#define METADOT_OPTMIZE_OFF __pragma(optimize("", off))
#define METADOT_OPTMIZE_ON __pragma(optimize("", on))
#define METADOT_DEBUGBREAK __debugbreak()