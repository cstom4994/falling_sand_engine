// Metadot builtin pfr is enhanced based on Boost.pfr modification
// Metadot code Copyright(c) 2022-2023, KaoruXun All rights reserved.
// Boost.pfr code by BoostOrg distributed under the Boost Software License, Version 1.0.
// https://github.com/boostorg/pfr

#ifndef ME_PFR_HPP
#define ME_PFR_HPP

#if defined(_MSC_VER)
#if _MSC_VER <= 1900
#error Boost.PFR library requires MSVC with c++17 support (Visual Studio 2017 or later).
#endif
#elif __cplusplus < 201402L
#error Boost.PFR library requires at least C++14.
#endif

#ifndef BOOST_PFR_USE_LOOPHOLE
#define BOOST_PFR_USE_LOOPHOLE 1
#endif

#ifndef BOOST_PFR_USE_CPP17
#ifdef __cpp_structured_bindings
#define BOOST_PFR_USE_CPP17 1
#elif defined(_MSC_VER)
#pragma message("PFR library supports MSVC compiler only with /std:c++latest or /std:c++17 flag. Assuming that you`ve used it. Define `BOOST_PFR_USE_CPP17` to 1 to suppress this warning.")
#define BOOST_PFR_USE_CPP17 1
#else
#define BOOST_PFR_USE_CPP17 0
#endif
#endif

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ME::cpp::pfr {
namespace detail {
namespace sequence_tuple {

template <std::size_t N, class T>
struct base_from_member {
    T value;
};

template <class I, class... Tail>
struct tuple_base;

template <std::size_t... I, class... Tail>
struct tuple_base<std::index_sequence<I...>, Tail...> : base_from_member<I, Tail>... {
    static constexpr std::size_t size_v = sizeof...(I);

    constexpr tuple_base() = default;
    constexpr tuple_base(tuple_base &&) = default;
    constexpr tuple_base(const tuple_base &) = default;

    constexpr tuple_base(Tail... v) noexcept : base_from_member<I, Tail>{v}... {}
};

template <>
struct tuple_base<std::index_sequence<>> {
    static constexpr std::size_t size_v = 0;
};

template <std::size_t N, class T>
constexpr T &get_impl(base_from_member<N, T> &t) noexcept {
    return t.value;
}

template <std::size_t N, class T>
constexpr const T &get_impl(const base_from_member<N, T> &t) noexcept {
    return t.value;
}

template <std::size_t N, class T>
constexpr volatile T &get_impl(volatile base_from_member<N, T> &t) noexcept {
    return t.value;
}

template <std::size_t N, class T>
constexpr const volatile T &get_impl(const volatile base_from_member<N, T> &t) noexcept {
    return t.value;
}

template <std::size_t N, class T>
constexpr T &&get_impl(base_from_member<N, T> &&t) noexcept {
    return std::forward<T>(t.value);
}

template <class... Values>
struct tuple : tuple_base<std::make_index_sequence<sizeof...(Values)>, Values...> {
    using tuple_base<std::make_index_sequence<sizeof...(Values)>, Values...>::tuple_base;
};

template <std::size_t N, class... T>
constexpr decltype(auto) get(tuple<T...> &t) noexcept {
    static_assert(N < tuple<T...>::size_v, "Tuple index out of bounds");
    return get_impl<N>(t);
}

template <std::size_t N, class... T>
constexpr decltype(auto) get(const tuple<T...> &t) noexcept {
    static_assert(N < tuple<T...>::size_v, "Tuple index out of bounds");
    return get_impl<N>(t);
}

template <std::size_t N, class... T>
constexpr decltype(auto) get(const volatile tuple<T...> &t) noexcept {
    static_assert(N < tuple<T...>::size_v, "Tuple index out of bounds");
    return get_impl<N>(t);
}

template <std::size_t N, class... T>
constexpr decltype(auto) get(volatile tuple<T...> &t) noexcept {
    static_assert(N < tuple<T...>::size_v, "Tuple index out of bounds");
    return get_impl<N>(t);
}

template <std::size_t N, class... T>
constexpr decltype(auto) get(tuple<T...> &&t) noexcept {
    static_assert(N < tuple<T...>::size_v, "Tuple index out of bounds");
    return get_impl<N>(std::move(t));
}

template <std::size_t I, class T>
using tuple_element = std::remove_reference<decltype(::ME::cpp::pfr::detail::sequence_tuple::get<I>(std::declval<T>()))>;

}  // namespace sequence_tuple

template <class T, std::size_t... I>
constexpr auto make_stdtuple_from_tietuple(const T &t, std::index_sequence<I...>) noexcept {
    return std::make_tuple(ME::cpp::pfr::detail::sequence_tuple::get<I>(t)...);
}

template <class T, std::size_t... I>
constexpr auto make_stdtiedtuple_from_tietuple(const T &t, std::index_sequence<I...>) noexcept {
    return std::tie(ME::cpp::pfr::detail::sequence_tuple::get<I>(t)...);
}

template <class T
#ifdef BOOST_PFR_DETAIL_STRICT_RVALUE_TESTING
          ,
          class = std::enable_if_t<std::is_rvalue_reference<T &&>::value>
#endif
          >
using rvalue_t = T &&;

template <std::size_t Index>
using size_t_ = std::integral_constant<std::size_t, Index>;

template <class T, class F, class I, class = decltype(std::declval<F>()(std::declval<T>(), I{}))>
void for_each_field_impl_apply(T &&v, F &&f, I i, long) {
    std::forward<F>(f)(std::forward<T>(v), i);
}

template <class T, class F, class I>
void for_each_field_impl_apply(T &&v, F &&f, I, int) {
    std::forward<F>(f)(std::forward<T>(v));
}

template <class T, class F, std::size_t... I>
void for_each_field_impl(T &t, F &&f, std::index_sequence<I...>, std::false_type) {
    const int v[] = {(for_each_field_impl_apply(sequence_tuple::get<I>(t), std::forward<F>(f), size_t_<I>{}, 1L), 0)...};
    (void)v;
}

template <class T, class F, std::size_t... I>
void for_each_field_impl(T &t, F &&f, std::index_sequence<I...>, std::true_type) {
    const int v[] = {(for_each_field_impl_apply(sequence_tuple::get<I>(std::move(t)), std::forward<F>(f), size_t_<I>{}, 1L), 0)...};
    (void)v;
}

template <std::size_t Index>
using size_t_ = std::integral_constant<std::size_t, Index>;

struct ubiq_constructor {
    std::size_t ignore;
    template <class Type>
    constexpr operator Type &() const noexcept;
};

template <class T>
struct ubiq_constructor_except {
    template <class Type>
    constexpr operator std::enable_if_t<!std::is_same<T, Type>::value, Type &>() const noexcept;
};

template <std::size_t N, class T>
struct is_single_field_and_aggregate_initializable : std::false_type {};
template <class T>
struct is_single_field_and_aggregate_initializable<1, T> : std::integral_constant<bool, !std::is_constructible<T, ubiq_constructor_except<T>>::value> {};

template <class T, std::size_t N>
struct is_aggregate_initializable_n {
    template <std::size_t... I>
    static constexpr bool is_not_constructible_n(std::index_sequence<I...>) noexcept {
        return !std::is_constructible<T, decltype(ubiq_constructor{I})...>::value || is_single_field_and_aggregate_initializable<N, T>::value;
    }

    static constexpr bool value = std::is_empty<T>::value || std::is_fundamental<T>::value || is_not_constructible_n(std::make_index_sequence<N>{});
};

template <class T, std::size_t... I>
constexpr auto enable_if_constructible_helper(std::index_sequence<I...>) noexcept -> typename std::add_pointer<decltype(T{ubiq_constructor{I}...})>::type;

template <class T, std::size_t N, class = decltype(enable_if_constructible_helper<T>(std::make_index_sequence<N>()))>
using enable_if_constructible_helper_t = std::size_t;

template <class T, std::size_t N>
constexpr std::size_t detect_fields_count(size_t_<N>, size_t_<N>, long) noexcept {
    return N;
}

template <class T, std::size_t Begin, std::size_t Middle>
constexpr std::size_t detect_fields_count(size_t_<Begin>, size_t_<Middle>, int) noexcept;

template <class T, std::size_t Begin, std::size_t Middle>
constexpr auto detect_fields_count(size_t_<Begin>, size_t_<Middle>, long) noexcept -> enable_if_constructible_helper_t<T, Middle> {
    using next_t = size_t_<Middle + (Middle - Begin + 1) / 2>;
    return detect_fields_count<T>(size_t_<Middle>{}, next_t{}, 1L);
}

template <class T, std::size_t Begin, std::size_t Middle>
constexpr std::size_t detect_fields_count(size_t_<Begin>, size_t_<Middle>, int) noexcept {
    using next_t = size_t_<(Begin + Middle) / 2>;
    return detect_fields_count<T>(size_t_<Begin>{}, next_t{}, 1L);
}

template <class T, std::size_t N>
constexpr auto detect_fields_count_greedy_remember(size_t_<N>, long) noexcept -> enable_if_constructible_helper_t<T, N> {
    return N;
}

template <class T, std::size_t N>
constexpr std::size_t detect_fields_count_greedy_remember(size_t_<N>, int) noexcept {
    return 0;
}

template <class T, std::size_t N>
constexpr std::size_t detect_fields_count_greedy(size_t_<N>, size_t_<N>) noexcept {
    return detect_fields_count_greedy_remember<T>(size_t_<N>{}, 1L);
}

template <class T, std::size_t Begin, std::size_t Last>
constexpr std::size_t detect_fields_count_greedy(size_t_<Begin>, size_t_<Last>) noexcept {
    constexpr std::size_t middle = Begin + (Last - Begin) / 2;
    constexpr std::size_t fields_count_big = detect_fields_count_greedy<T>(size_t_<middle + 1>{}, size_t_<Last>{});
    constexpr std::size_t fields_count_small = detect_fields_count_greedy<T>(size_t_<Begin>{}, size_t_ < fields_count_big ? Begin : middle > {});
    return fields_count_big ? fields_count_big : fields_count_small;
}

template <class T, std::size_t N>
constexpr auto detect_fields_count_dispatch(size_t_<N>, long) noexcept -> decltype(sizeof(T{})) {
    return detect_fields_count<T>(size_t_<0>{}, size_t_<N / 2 + 1>{}, 1L);
}

template <class T, std::size_t N>
constexpr std::size_t detect_fields_count_dispatch(size_t_<N>, int) noexcept {

    return detect_fields_count_greedy<T>(size_t_<0>{}, size_t_<N>{});
}

template <class T>
constexpr std::size_t fields_count() noexcept {
    using type = std::remove_cv_t<T>;

    static_assert(!std::is_reference<type>::value,
                  "Attempt to get fields count on a reference. This is not allowed "
                  "because that could hide an issue and different library users expect "
                  "different behavior in that case.");

    static_assert(std::is_copy_constructible<std::remove_all_extents_t<type>>::value, "Type and each field in the type must be copy constructible.");

    static_assert(!std::is_polymorphic<type>::value,
                  "Type must have no virtual function, because otherwise it is not "
                  "aggregate initializable.");

#ifdef __cpp_lib_is_aggregate
    static_assert(std::is_aggregate<type>::value || std::is_standard_layout<type>::value, "Type must be aggregate initializable.");
#endif

    constexpr std::size_t max_fields_count = (sizeof(type) * 8);
    constexpr std::size_t result = detect_fields_count_dispatch<type>(ME::cpp::pfr::detail::size_t_<max_fields_count>{}, 1L);

    static_assert(is_aggregate_initializable_n<type, result>::value,
                  "Types with user specified constructors (non-aggregate initializable "
                  "types) are not supported.");

    static_assert(result != 0 || std::is_empty<type>::value || std::is_fundamental<type>::value || std::is_reference<type>::value,
                  "Something went wrong. Please report this issue to the github along "
                  "with the structure you're reflecting.");

    return result;
}

}  // namespace detail

template <class T>
using tuple_size = detail::size_t_<::ME::cpp::pfr::detail::fields_count<T>()>;

template <class T>
constexpr std::size_t tuple_size_v = tuple_size<T>::value;

namespace detail {

template <class... Args>
constexpr auto make_tuple_of_references(Args &&...args) noexcept {
    return sequence_tuple::tuple<Args &...>{args...};
}

template <class T>
constexpr auto tie_as_tuple(T &, size_t_<0>) noexcept {
    return sequence_tuple::tuple<>{};
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<1>, std::enable_if_t<std::is_class<std::remove_cv_t<T>>::value> * = 0) noexcept {
    auto &[a] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<1>, std::enable_if_t<!std::is_class<std::remove_cv_t<T>>::value> * = 0) noexcept {
    return ::ME::cpp::pfr::detail::make_tuple_of_references(val);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<2>) noexcept {
    auto &[a, b] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<3>) noexcept {
    auto &[a, b, c] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<4>) noexcept {
    auto &[a, b, c, d] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<5>) noexcept {
    auto &[a, b, c, d, e] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<6>) noexcept {
    auto &[a, b, c, d, e, f] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<7>) noexcept {
    auto &[a, b, c, d, e, f, g] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<8>) noexcept {
    auto &[a, b, c, d, e, f, g, h] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<9>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<10>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<11>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<12>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<13>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<14>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<15>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<16>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<17>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<18>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<19>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<20>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<21>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<22>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<23>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<24>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<25>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<26>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<27>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<28>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<29>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<30>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<31>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<32>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<33>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<34>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<35>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<36>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<37>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<38>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<39>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<40>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<41>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<42>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<43>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<44>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<45>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<46>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<47>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z] = val;
    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y,
                                                            Z);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<48>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<49>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<50>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<51>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<52>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<53>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<54>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<55>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<56>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<57>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<58>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al] =
            val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<59>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al,
           am] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<60>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<61>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<62>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<63>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<64>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<65>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<66>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<67>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<68>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<69>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<70>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<71>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<72>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<73>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<74>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<75>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<76>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<77>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<78>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<79>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<80>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<81>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<82>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<83>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<84>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<85>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<86>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<87>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<88>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR, aS] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR, aS);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<89>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR, aS, aU] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR, aS, aU);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<90>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR, aS, aU, aV] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR, aS, aU, aV);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<91>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR, aS, aU, aV, aW] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR, aS, aU, aV, aW);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<92>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<93>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<94>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<95>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ, ba] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ, ba);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<96>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ, ba, bb] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ, ba, bb);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<97>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ, ba, bb, bc] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ, ba, bb, bc);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<98>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ, ba, bb, bc, bd] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ, ba, bb, bc, bd);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<99>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ, ba, bb, bc, bd, be] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ, ba, bb, bc, bd, be);
}

template <class T>
constexpr auto tie_as_tuple(T &val, size_t_<100>) noexcept {
    auto &[a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z, aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am,
           an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL, aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ, ba, bb, bc, bd, be, bf] = val;

    return ::ME::cpp::pfr::detail::make_tuple_of_references(a, b, c, d, e, f, g, h, j, k, l, m, n, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F, G, H, J, K, L, M, N, P, Q, R, S, U, V, W, X, Y, Z,
                                                            aa, ab, ac, ad, ae, af, ag, ah, aj, ak, al, am, an, ap, aq, ar, as, at, au, av, aw, ax, ay, az, aA, aB, aC, aD, aE, aF, aG, aH, aJ, aK, aL,
                                                            aM, aN, aP, aQ, aR, aS, aU, aV, aW, aX, aY, aZ, ba, bb, bc, bd, be, bf);
}

template <class T>
constexpr auto tie_as_tuple(T &val) noexcept {
    typedef size_t_<fields_count<T>()> fields_count_tag;
    return ME::cpp::pfr::detail::tie_as_tuple(val, fields_count_tag{});
}

#ifndef _MSC_VER

struct do_not_define_std_tuple_size_for_me {
    bool test1 = true;
};

template <class T>
constexpr bool do_structured_bindings_work() noexcept {
    T val{};
    const auto &[a] = val;

    return a;
}

static_assert(do_structured_bindings_work<do_not_define_std_tuple_size_for_me>(),
              "Your compiler can not handle C++17 structured bindings. Read the above "
              "comments for workarounds.");

#endif

template <class T, class F, std::size_t... I>
void for_each_field_dispatcher(T &t, F &&f, std::index_sequence<I...>) {
    std::forward<F>(f)(detail::tie_as_tuple(t));
}

}  // namespace detail

template <std::size_t I, class T>
constexpr decltype(auto) get(const T &val) noexcept {
    return detail::sequence_tuple::get<I>(detail::tie_as_tuple(val));
}

template <std::size_t I, class T>
constexpr decltype(auto) get(T &val) noexcept {
    return detail::sequence_tuple::get<I>(detail::tie_as_tuple(val));
}

template <std::size_t I, class T>
using tuple_element = detail::sequence_tuple::tuple_element<I, decltype(::ME::cpp::pfr::detail::tie_as_tuple(std::declval<T &>()))>;

template <std::size_t I, class T>
using tuple_element_t = typename tuple_element<I, T>::type;

template <class T>
constexpr auto structure_to_tuple(const T &val) noexcept {
    return detail::make_stdtuple_from_tietuple(detail::tie_as_tuple(val), std::make_index_sequence<tuple_size_v<T>>());
}

template <class T>
constexpr auto structure_tie(T &val) noexcept {
    return detail::make_stdtiedtuple_from_tietuple(detail::tie_as_tuple(val), std::make_index_sequence<tuple_size_v<T>>());
}

template <class T, class F>
void for_each_field(T &&value, F &&func) {
    constexpr std::size_t fields_count_val = ME::cpp::pfr::detail::fields_count<std::remove_reference_t<T>>();

    ::ME::cpp::pfr::detail::for_each_field_dispatcher(
            value,
            [f = std::forward<F>(func)](auto &&t) mutable {
                constexpr std::size_t fields_count_val_in_lambda = ME::cpp::pfr::detail::fields_count<std::remove_reference_t<T>>();

                ::ME::cpp::pfr::detail::for_each_field_impl(t, std::forward<F>(f), std::make_index_sequence<fields_count_val_in_lambda>{}, std::is_rvalue_reference<T &&>{});
            },
            std::make_index_sequence<fields_count_val>{});
}

}  // namespace ME::cpp::pfr

#endif