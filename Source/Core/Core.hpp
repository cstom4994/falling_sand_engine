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

#include "Core/Core.h"
#include "Core/Macros.h"
#include "Libs/VisitStruct.hpp"

#define METADOT_STRUCT VISITABLE_STRUCT

#define METADOT_CALLABLE(func_name)                                                                \
    [](auto... args) -> decltype(auto) { return func_name(std::move(args)...); }

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

namespace MetaEngine {

    class ArgBase {
    public:
        ArgBase() {}
        virtual ~ArgBase() {}
        virtual void Format(std::ostringstream &ss, const std::string &fmt) = 0;
    };

    template<class T>
    class Arg : public ArgBase {
    public:
        Arg(T arg) : m_arg(arg) {}
        virtual ~Arg() {}
        virtual void Format(std::ostringstream &ss, const std::string &fmt) { ss << m_arg; }

    private:
        T m_arg;
    };

    class ArgArray : public std::vector<ArgBase *> {
    public:
        ArgArray() {}
        ~ArgArray() {
            std::for_each(begin(), end(), [](ArgBase *p) { delete p; });
        }
    };

    static void FormatItem(std::ostringstream &ss, const std::string &item, const ArgArray &args) {
        int index = 0;
        int alignment = 0;
        std::string fmt;

        char *endptr = nullptr;
        index = strtol(&item[0], &endptr, 10);
        if (index < 0 || index >= args.size()) { return; }

        if (*endptr == ',') {
            alignment = strtol(endptr + 1, &endptr, 10);
            if (alignment > 0) {
                ss << std::right << std::setw(alignment);
            } else if (alignment < 0) {
                ss << std::left << std::setw(-alignment);
            }
        }

        if (*endptr == ':') { fmt = endptr + 1; }

        args[index]->Format(ss, fmt);

        return;
    }

    template<class T>
    static void Transfer(ArgArray &argArray, T t) {
        argArray.push_back(new Arg<T>(t));
    }

    template<class T, typename... Args>
    static void Transfer(ArgArray &argArray, T t, Args &&...args) {
        Transfer(argArray, t);
        Transfer(argArray, args...);
    }

    template<typename... Args>
    std::string Format(const std::string &format, Args &&...args) {
        if (sizeof...(args) == 0) { return format; }

        ArgArray argArray;
        Transfer(argArray, args...);
        size_t start = 0;
        size_t pos = 0;
        std::ostringstream ss;
        while (true) {
            pos = format.find('{', start);
            if (pos == std::string::npos) {
                ss << format.substr(start);
                break;
            }

            ss << format.substr(start, pos - start);
            if (format[pos + 1] == '{') {
                ss << '{';
                start = pos + 2;
                continue;
            }

            start = pos + 1;
            pos = format.find('}', start);
            if (pos == std::string::npos) {
                ss << format.substr(start - 1);
                break;
            }

            FormatItem(ss, format.substr(start, pos - start), argArray);
            start = pos + 1;
        }

        return ss.str();
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

namespace MetaEngine {

    template<class T>
    using Ref = std::shared_ptr<T>;

    template<class _Ty, class... _Types>
    Ref<_Ty> MakeRef(_Types &&..._Args) {
        // make a shared_ptr
        return std::make_shared<_Ty>(std::forward<_Types>(_Args)...);
    }

}// namespace MetaEngine

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

#endif// !_CORE_H
