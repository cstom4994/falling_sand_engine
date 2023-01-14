

#pragma once
#ifndef META_CONFIG_HPP
#define META_CONFIG_HPP

// Get <version> file, if implemented (P0754R2).
#if (defined(__APPLE__) && __clang_major__ >= 10) || (!defined(__APPLE__) && ((defined(__clang__) && __clang_major__ >= 8) || (defined(__GNUC__) && __GNUC___ >= 9))) || \
        (defined(_MSC_VER) && _MSC_VER >= 1922)
#include <version>  // C++ version & features
#endif

// Check we have C++17 support. Report issues to Github project.
#if defined(_MSC_VER)
// Earlier MSVC compilers lack features or have C++17 bugs.
static_assert(_MSC_VER >= 1911, "MSVC 2017 required");
// We disable some annoying warnings of VC++
#pragma warning(disable : 4275)  // non dll-interface class 'X' used as base for dll-interface class 'Y'
#pragma warning(disable : 4251)  // class 'X' needs to have dll-interface to be used by clients of class 'Y'
#include <ostream>               // In future MSVC, <string> doesn't transitively <ostream>, ponder will
                                 // compile failed with error C2027 and C2065, so add <ostream>.
#endif

// Debug build config?
#define META_DEBUG (!defined(NDEBUG))

// If user doesn't define traits use the default:
#ifndef META_ID_TRAITS_USER
// # define META_ID_TRAITS_STD_STRING      // Use std::string and const std::string&
#define META_ID_TRAITS_STRING_VIEW  // Use std::string and Meta::string_view
#endif                              // META_ID_TRAITS_USER

#include <cassert>

#include "engine/meta/idtraits.hpp"

#define META__NON_COPYABLE(CLS) \
    CLS(CLS const&) = delete;   \
    CLS& operator=(CLS const&) = delete

#define META__UNUSED(VAR) ((void)&(VAR))

#endif  // META_CONFIG_HPP
