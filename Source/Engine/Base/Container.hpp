// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _CONTAINER_HPP_
#define _CONTAINER_HPP_

#include <algorithm>

namespace Utils {

    // 包装常量表达式字符串的文字类类型。
    // 使用隐式转换来允许模板*seemingly*接受常量字符串。

    template<size_t N>
    struct StringLiteral
    {
        constexpr StringLiteral(const char (&str)[N]) {
            std::copy_n(str, N, value);
        }

        char value[N];
    };

    template<Utils::StringLiteral Key>
    struct KeyMatcher
    {
    };
}// namespace Utils


template<Utils::StringLiteral Key, typename T>
struct Field
{
    constexpr static Utils::StringLiteral key = Key;
    using type = T;

    type obj;

    type &r_(Utils::KeyMatcher<Key>) { return obj; }

    const type &c_(Utils::KeyMatcher<Key>) const { return obj; }
};

//
namespace Utils {
    template<typename T>
    constexpr bool is_field = false;

    template<Utils::StringLiteral Key, typename T>
    constexpr bool is_field<Field<Key, T>> = true;
}// namespace Utils


template<typename... Fields>
struct Container : Fields...
{
    static_assert((Utils::is_field<Fields> && ...), "Container must consist of Field.");

    Container() = default;

    explicit Container(typename Fields::type &&...args) : Fields(std::forward<typename Fields::type>(args))... {}

    template<Utils::StringLiteral Key>
    auto &r() { return r_(Utils::KeyMatcher<Key>{}); }

    template<Utils::StringLiteral Key>
    const auto &c() const { return c_(Utils::KeyMatcher<Key>{}); }

private:
    using Fields::r_...;
    using Fields::c_...;

    template<Utils::StringLiteral Key>
    auto &r_(Utils::KeyMatcher<Key>) { static_assert(sizeof(Key) != sizeof(Key), "invalid Key!"); }

    template<Utils::StringLiteral Key>
    const auto &c_(Utils::KeyMatcher<Key>) const { static_assert(sizeof(Key) != sizeof(Key), "invalid Key!"); }
};

#endif
