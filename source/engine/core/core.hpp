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
