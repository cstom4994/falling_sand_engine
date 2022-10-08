// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.

#pragma once

#include <cstdio>
#include <cstring>
#include <cstddef>
#include <stdexcept>
#include <iostream>
#include <cassert>

#include "Engine/Macros.hpp"
#include "Engine/render/renderer_gpu.h"

#include <box2d/box2d.h>


namespace Meko
{
    namespace utils
    {
        struct exception : public std::exception
        {
            const char* m_what;
            exception(const char* what) : m_what(what) {}
            const char* what() const throw() { return m_what; }
        };
    }
}

inline void _better_assert(const char* condition, const char* message, const char* fileline) {
    std::cout << "[" << fileline << "] "
        << "Assertion `" << condition << "` failed.\n"
        << message << std::endl;
    std::abort();
}

inline void _better_assert(const char* condition,
    const std::string& message,
    const char* fileline) {
    _better_assert(condition, message.c_str(), fileline);
}

#ifdef NDEBUG
#define METADOT_ASSERT(condition, message) static_cast<void>(0)
#define METADOT_ASSERT_E(condition) static_cast<void>(0)
#else
#define METADOT_ASSERT(condition, message) \
    static_cast<bool>(condition)          \
        ? static_cast<void>(0)            \
        : _better_assert(#condition, message, __FILE__ ":" METADOT_STRING(__LINE__))
#define METADOT_ASSERT_E(condition) METADOT_ASSERT(condition, "unknown")
#endif

#define METADOT_THROW(msg)                                                                     \
    {                                                                                       \
        console.error("throw exception: ", msg, "\t\t at ", __FILE__, ":", __LINE__, "\n"); \
        throw Meko::utils::exception(msg);                                                  \
    }






















#ifndef DBG_MACRO_DBG_H
#define DBG_MACRO_DBG_H

#ifndef DBG_MACRO_NO_WARNING
#pragma message("WARNING: the 'dbg.h' header is included in your code base")
#endif  // DBG_MACRO_NO_WARNING

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <ios>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>


#define DBG_MACRO_CXX_STANDARD 17

#if DBG_MACRO_CXX_STANDARD >= 17
#include <optional>
#include <variant>
#endif

namespace dbg {

    struct time {};

    namespace pretty_function {

        // Compiler-agnostic version of __PRETTY_FUNCTION__ and constants to
        // extract the template argument in `type_name_impl`

#if defined(__clang__)
#define DBG_MACRO_PRETTY_FUNCTION __PRETTY_FUNCTION__
        static constexpr size_t PREFIX_LENGTH =
            sizeof("const char *dbg::type_name_impl() [T = ") - 1;
        static constexpr size_t SUFFIX_LENGTH = sizeof("]") - 1;
#elif defined(__GNUC__) && !defined(__clang__)
#define DBG_MACRO_PRETTY_FUNCTION __PRETTY_FUNCTION__
        static constexpr size_t PREFIX_LENGTH =
            sizeof("const char* dbg::type_name_impl() [with T = ") - 1;
        static constexpr size_t SUFFIX_LENGTH = sizeof("]") - 1;
#elif defined(_MSC_VER)
#define DBG_MACRO_PRETTY_FUNCTION __FUNCSIG__
        static constexpr size_t PREFIX_LENGTH =
            sizeof("const char *__cdecl dbg::type_name_impl<") - 1;
        static constexpr size_t SUFFIX_LENGTH = sizeof(">(void)") - 1;
#else
#error "This compiler is currently not supported by dbg_macro."
#endif

    }  // namespace pretty_function

    // Formatting helpers

    template <typename T>
    struct print_formatted {
        static_assert(std::is_integral<T>::value,
            "Only integral types are supported.");

        print_formatted(T value, int numeric_base)
            : inner(value), base(numeric_base) {}

        operator T() const { return inner; }

        const char* prefix() const {
            switch (base) {
            case 8:
                return "0o";
            case 16:
                return "0x";
            case 2:
                return "0b";
            default:
                return "";
            }
        }

        T inner;
        int base;
    };

    template <typename T>
    print_formatted<T> hex(T value) {
        return print_formatted<T>{value, 16};
    }

    template <typename T>
    print_formatted<T> oct(T value) {
        return print_formatted<T>{value, 8};
    }

    template <typename T>
    print_formatted<T> bin(T value) {
        return print_formatted<T>{value, 2};
    }

    // Implementation of 'type_name<T>()'

    template <typename T>
    const char* type_name_impl() {
        return DBG_MACRO_PRETTY_FUNCTION;
    }

    template <typename T>
    struct type_tag {};

    template <int&... ExplicitArgumentBarrier, typename T>
    std::string get_type_name(type_tag<T>) {
        namespace pf = pretty_function;

        std::string type = type_name_impl<T>();
        return type.substr(pf::PREFIX_LENGTH,
            type.size() - pf::PREFIX_LENGTH - pf::SUFFIX_LENGTH);
    }

    template <typename T>
    std::string type_name() {
        if (std::is_volatile<T>::value) {
            if (std::is_pointer<T>::value) {
                return type_name<typename std::remove_volatile<T>::type>() + " volatile";
            }
            else {
                return "volatile " + type_name<typename std::remove_volatile<T>::type>();
            }
        }
        if (std::is_const<T>::value) {
            if (std::is_pointer<T>::value) {
                return type_name<typename std::remove_const<T>::type>() + " const";
            }
            else {
                return "const " + type_name<typename std::remove_const<T>::type>();
            }
        }
        if (std::is_pointer<T>::value) {
            return type_name<typename std::remove_pointer<T>::type>() + "*";
        }
        if (std::is_lvalue_reference<T>::value) {
            return type_name<typename std::remove_reference<T>::type>() + "&";
        }
        if (std::is_rvalue_reference<T>::value) {
            return type_name<typename std::remove_reference<T>::type>() + "&&";
        }
        return get_type_name(type_tag<T>{});
    }

    inline std::string get_type_name(type_tag<short>) {
        return "short";
    }

    inline std::string get_type_name(type_tag<unsigned short>) {
        return "unsigned short";
    }

    inline std::string get_type_name(type_tag<long>) {
        return "long";
    }

    inline std::string get_type_name(type_tag<unsigned long>) {
        return "unsigned long";
    }

    inline std::string get_type_name(type_tag<std::string>) {
        return "std::string";
    }

    template <typename T>
    std::string get_type_name(type_tag<std::vector<T, std::allocator<T>>>) {
        return "std::vector<" + type_name<T>() + ">";
    }

    template <typename T1, typename T2>
    std::string get_type_name(type_tag<std::pair<T1, T2>>) {
        return "std::pair<" + type_name<T1>() + ", " + type_name<T2>() + ">";
    }

    template <typename... T>
    std::string type_list_to_string() {
        std::string result;
        auto unused = { (result += type_name<T>() + ", ", 0)..., 0 };
        static_cast<void>(unused);

#if DBG_MACRO_CXX_STANDARD >= 17
        if constexpr (sizeof...(T) > 0) {
#else
        if (sizeof...(T) > 0) {
#endif
            result.pop_back();
            result.pop_back();
        }
        return result;
    }

    template <typename... T>
    std::string get_type_name(type_tag<std::tuple<T...>>) {
        return "std::tuple<" + type_list_to_string<T...>() + ">";
    }

    template <typename T>
    inline std::string get_type_name(type_tag<print_formatted<T>>) {
        return type_name<T>();
    }

    // Implementation of 'is_detected' to specialize for container-like types

    namespace detail_detector {

        struct nonesuch {
            nonesuch() = delete;
            ~nonesuch() = delete;
            nonesuch(nonesuch const&) = delete;
            void operator=(nonesuch const&) = delete;
        };

        template <typename...>
        using void_t = void;

        template <class Default,
            class AlwaysVoid,
            template <class...>
        class Op,
            class... Args>
        struct detector {
            using value_t = std::false_type;
            using type = Default;
        };

        template <class Default, template <class...> class Op, class... Args>
        struct detector<Default, void_t<Op<Args...>>, Op, Args...> {
            using value_t = std::true_type;
            using type = Op<Args...>;
        };

    }  // namespace detail_detector

    template <template <class...> class Op, class... Args>
    using is_detected = typename detail_detector::
        detector<detail_detector::nonesuch, void, Op, Args...>::value_t;

    namespace detail {

        namespace {
            using std::begin;
            using std::end;
#if DBG_MACRO_CXX_STANDARD < 17
            template <typename T>
            constexpr auto size(const T& c) -> decltype(c.size()) {
                return c.size();
            }
            template <typename T, std::size_t N>
            constexpr std::size_t size(const T(&)[N]) {
                return N;
            }
#else
            using std::size;
#endif
        }  // namespace

        template <typename T>
        using detect_begin_t = decltype(detail::begin(std::declval<T>()));

        template <typename T>
        using detect_end_t = decltype(detail::end(std::declval<T>()));

        template <typename T>
        using detect_size_t = decltype(detail::size(std::declval<T>()));

        template <typename T>
        struct is_container {
            static constexpr bool value =
                is_detected<detect_begin_t, T>::value &&
                is_detected<detect_end_t, T>::value &&
                is_detected<detect_size_t, T>::value &&
                !std::is_same<std::string,
                typename std::remove_cv<
                typename std::remove_reference<T>::type>::type>::value;
        };

        template <typename T>
        using ostream_operator_t =
            decltype(std::declval<std::ostream&>() << std::declval<T>());

        template <typename T>
        struct has_ostream_operator : is_detected<ostream_operator_t, T> {};

    }  // namespace detail

    // Helper to dbg(…)-print types
    template <typename T>
    struct print_type {};

    template <typename T>
    print_type<T> type() {
        return print_type<T>{};
    }

    // Forward declarations of "pretty_print"

    template <typename T>
    inline void pretty_print(std::ostream & stream, const T & value, std::true_type);

    template <typename T>
    inline void pretty_print(std::ostream&, const T&, std::false_type);

    template <typename T>
    inline typename std::enable_if<!detail::is_container<const T&>::value &&
        !std::is_enum<T>::value,
        bool>::type
        pretty_print(std::ostream & stream, const T & value);

    inline bool pretty_print(std::ostream & stream, const bool& value);

    inline bool pretty_print(std::ostream & stream, const char& value);

    template <typename P>
    inline bool pretty_print(std::ostream & stream, P* const& value);

    template <typename T, typename Deleter>
    inline bool pretty_print(std::ostream & stream,
        std::unique_ptr<T, Deleter>&value);

    template <typename T>
    inline bool pretty_print(std::ostream & stream, std::shared_ptr<T>&value);

    template <size_t N>
    inline bool pretty_print(std::ostream & stream, const char(&value)[N]);

    template <>
    inline bool pretty_print(std::ostream & stream, const char* const& value);

    template <typename... Ts>
    inline bool pretty_print(std::ostream & stream, const std::tuple<Ts...>&value);

    template <>
    inline bool pretty_print(std::ostream & stream, const std::tuple<>&);

    template <>
    inline bool pretty_print(std::ostream & stream, const time&);

    template <typename T>
    inline bool pretty_print(std::ostream & stream,
        const print_formatted<T>&value);

    template <typename T>
    inline bool pretty_print(std::ostream & stream, const print_type<T>&);

    template <typename Enum>
    inline typename std::enable_if<std::is_enum<Enum>::value, bool>::type
        pretty_print(std::ostream & stream, Enum const& value);

    inline bool pretty_print(std::ostream & stream, const std::string & value);

#if DBG_MACRO_CXX_STANDARD >= 17

    inline bool pretty_print(std::ostream & stream, const std::string_view & value);

#endif

    template <typename T1, typename T2>
    inline bool pretty_print(std::ostream & stream, const std::pair<T1, T2>&value);

#if DBG_MACRO_CXX_STANDARD >= 17

    template <typename T>
    inline bool pretty_print(std::ostream & stream, const std::optional<T>&value);

    template <typename... Ts>
    inline bool pretty_print(std::ostream & stream,
        const std::variant<Ts...>&value);

#endif

    template <typename Container>
    inline typename std::enable_if<detail::is_container<const Container&>::value,
        bool>::type
        pretty_print(std::ostream & stream, const Container & value);

    // Specializations of "pretty_print"

    template <typename T>
    inline void pretty_print(std::ostream & stream, const T & value, std::true_type) {
        stream << value;
    }

    template <typename T>
    inline void pretty_print(std::ostream&, const T&, std::false_type) {
        static_assert(detail::has_ostream_operator<const T&>::value,
            "Type does not support the << ostream operator");
    }

    template <typename T>
    inline typename std::enable_if<!detail::is_container<const T&>::value &&
        !std::is_enum<T>::value,
        bool>::type
        pretty_print(std::ostream & stream, const T & value) {
        pretty_print(stream, value,
            typename detail::has_ostream_operator<const T&>::type{});
        return true;
    }

    inline bool pretty_print(std::ostream & stream, const bool& value) {
        stream << std::boolalpha << value;
        return true;
    }

    inline bool pretty_print(std::ostream & stream, const char& value) {
        const bool printable = value >= 0x20 && value <= 0x7E;

        if (printable) {
            stream << "'" << value << "'";
        }
        else {
            stream << "'\\x" << std::setw(2) << std::setfill('0') << std::hex
                << std::uppercase << (0xFF & value) << "'";
        }
        return true;
    }

    template <typename P>
    inline bool pretty_print(std::ostream & stream, P* const& value) {
        if (value == nullptr) {
            stream << "nullptr";
        }
        else {
            stream << value;
        }
        return true;
    }

    template <typename T, typename Deleter>
    inline bool pretty_print(std::ostream & stream,
        std::unique_ptr<T, Deleter>&value) {
        pretty_print(stream, value.get());
        return true;
    }

    template <typename T>
    inline bool pretty_print(std::ostream & stream, std::shared_ptr<T>&value) {
        pretty_print(stream, value.get());
        stream << " (use_count = " << value.use_count() << ")";

        return true;
    }

    template <size_t N>
    inline bool pretty_print(std::ostream & stream, const char(&value)[N]) {
        stream << value;
        return false;
    }

    template <>
    inline bool pretty_print(std::ostream & stream, const char* const& value) {
        stream << '"' << value << '"';
        return true;
    }

    template <size_t Idx>
    struct pretty_print_tuple {
        template <typename... Ts>
        static void print(std::ostream& stream, const std::tuple<Ts...>& tuple) {
            pretty_print_tuple<Idx - 1>::print(stream, tuple);
            stream << ", ";
            pretty_print(stream, std::get<Idx>(tuple));
        }
    };

    template <>
    struct pretty_print_tuple<0> {
        template <typename... Ts>
        static void print(std::ostream& stream, const std::tuple<Ts...>& tuple) {
            pretty_print(stream, std::get<0>(tuple));
        }
    };

    template <typename... Ts>
    inline bool pretty_print(std::ostream & stream, const std::tuple<Ts...>&value) {
        stream << "{";
        pretty_print_tuple<sizeof...(Ts) - 1>::print(stream, value);
        stream << "}";

        return true;
    }

    template <>
    inline bool pretty_print(std::ostream & stream, const std::tuple<>&) {
        stream << "{}";

        return true;
    }

    template <>
    inline bool pretty_print(std::ostream & stream, const time&) {
        using namespace std::chrono;

        const auto now = system_clock::now();
        const auto us =
            duration_cast<microseconds>(now.time_since_epoch()).count() % 1000000;
        const auto hms = system_clock::to_time_t(now);
#if _MSC_VER >= 1600
        struct tm t;
        localtime_s(&t, &hms);
        const std::tm* tm = &t;
#else
        const std::tm* tm = std::localtime(&hms);
#endif
        stream << "current time = " << std::put_time(tm, "%H:%M:%S") << '.'
            << std::setw(6) << std::setfill('0') << us;
        return false;
    }

    // Converts decimal integer to binary string
    template <typename T>
    std::string decimalToBinary(T n) {
        const size_t length = 8 * sizeof(T);
        std::string toRet;
        toRet.resize(length);

        for (size_t i = 0; i < length; ++i) {
            const auto bit_at_index_i = static_cast<char>((n >> i) & 1);
            toRet[length - 1 - i] = bit_at_index_i + '0';
        }

        return toRet;
    }

    template <typename T>
    inline bool pretty_print(std::ostream & stream,
        const print_formatted<T>&value) {
        if (value.inner < 0) {
            stream << "-";
        }
        stream << value.prefix();

        // Print using setbase
        if (value.base != 2) {
            stream << std::setw(sizeof(T)) << std::setfill('0')
                << std::setbase(value.base) << std::uppercase;

            if (value.inner >= 0) {
                // The '+' sign makes sure that a uint_8 is printed as a number
                stream << +value.inner;
            }
            else {
                using unsigned_type = typename std::make_unsigned<T>::type;
                stream << +(static_cast<unsigned_type>(-(value.inner + 1)) + 1);
            }
        }
        else {
            // Print for binary
            if (value.inner >= 0) {
                stream << decimalToBinary(value.inner);
            }
            else {
                using unsigned_type = typename std::make_unsigned<T>::type;
                stream << decimalToBinary<unsigned_type>(
                    static_cast<unsigned_type>(-(value.inner + 1)) + 1);
            }
        }

        return true;
    }

    template <typename T>
    inline bool pretty_print(std::ostream & stream, const print_type<T>&) {
        stream << type_name<T>();

        stream << " [sizeof: " << sizeof(T) << " byte, ";

        stream << "trivial: ";
        if (std::is_trivial<T>::value) {
            stream << "yes";
        }
        else {
            stream << "no";
        }

        stream << ", standard layout: ";
        if (std::is_standard_layout<T>::value) {
            stream << "yes";
        }
        else {
            stream << "no";
        }
        stream << "]";

        return false;
    }

    template <typename Enum>
    inline typename std::enable_if<std::is_enum<Enum>::value, bool>::type
        pretty_print(std::ostream & stream, Enum const& value) {
        using UnderlyingType = typename std::underlying_type<Enum>::type;
        stream << static_cast<UnderlyingType>(value);

        return true;
    }

    inline bool pretty_print(std::ostream & stream, const std::string & value) {
        stream << '"' << value << '"';
        return true;
    }

#if DBG_MACRO_CXX_STANDARD >= 17

    inline bool pretty_print(std::ostream & stream, const std::string_view & value) {
        stream << '"' << std::string(value) << '"';
        return true;
    }

#endif

    template <typename T1, typename T2>
    inline bool pretty_print(std::ostream & stream, const std::pair<T1, T2>&value) {
        stream << "{";
        pretty_print(stream, value.first);
        stream << ", ";
        pretty_print(stream, value.second);
        stream << "}";
        return true;
    }

#if DBG_MACRO_CXX_STANDARD >= 17

    template <typename T>
    inline bool pretty_print(std::ostream & stream, const std::optional<T>&value) {
        if (value) {
            stream << '{';
            pretty_print(stream, *value);
            stream << '}';
        }
        else {
            stream << "nullopt";
        }

        return true;
    }

    template <typename... Ts>
    inline bool pretty_print(std::ostream & stream,
        const std::variant<Ts...>&value) {
        stream << "{";
        std::visit([&stream](auto&& arg) { pretty_print(stream, arg); }, value);
        stream << "}";

        return true;
    }

#endif

    template <typename Container>
    inline typename std::enable_if<detail::is_container<const Container&>::value,
        bool>::type
        pretty_print(std::ostream & stream, const Container & value) {
        stream << "{";
        const size_t size = detail::size(value);
        const size_t n = std::min(size_t{ 10 }, size);
        size_t i = 0;
        using std::begin;
        using std::end;
        for (auto it = begin(value); it != end(value) && i < n; ++it, ++i) {
            pretty_print(stream, *it);
            if (i != n - 1) {
                stream << ", ";
            }
        }

        if (size > n) {
            stream << ", ...";
            stream << " size:" << size;
        }

        stream << "}";
        return true;
    }

    template <typename T, typename... U>
    struct last {
        using type = typename last<U...>::type;
    };

    template <typename T>
    struct last<T> {
        using type = T;
    };

    template <typename... T>
    using last_t = typename last<T...>::type;

    class DebugOutput {
    public:
        // Helper alias to avoid obscure type `const char* const*` in signature.
        using expr_t = const char*;

        DebugOutput(const char* filepath, int line, const char* function_name)
        {
            std::string path = filepath;
            const std::size_t path_length = path.length();
            if (path_length > MAX_PATH_LENGTH) {
                path = ".." + path.substr(path_length - MAX_PATH_LENGTH, MAX_PATH_LENGTH);
            }
            std::stringstream ss;
            ss << "[" << path << ":" << line << " ("
                << function_name << ")] ";
            m_location = ss.str();
        }

        template <typename... T>
        auto print(std::initializer_list<expr_t> exprs,
            std::initializer_list<std::string> types,
            T&&... values) -> last_t<T...> {
            if (exprs.size() != sizeof...(values)) {
                std::cout
                    << m_location
                    << "The number of arguments mismatch, please check unprotected comma"
                    << std::endl;
            }
            return print_impl(exprs.begin(), types.begin(), std::forward<T>(values)...);
        }

    private:
        template <typename T>
        T&& print_impl(const expr_t* expr, const std::string* type, T&& value) {
            const T& ref = value;
            std::stringstream stream_value;
            const bool print_expr_and_type = pretty_print(stream_value, ref);

            std::stringstream output;
            output << m_location;
            if (print_expr_and_type) {
                output << *expr << " = ";
            }
            output << stream_value.str();
            if (print_expr_and_type) {
                output << " (" << *type << ")";
            }
            output << std::endl;
            std::cout << output.str();

            return std::forward<T>(value);
        }

        template <typename T, typename... U>
        auto print_impl(const expr_t* exprs,
            const std::string* types,
            T&& value,
            U&&... rest) -> last_t<T, U...> {
            print_impl(exprs, types, std::forward<T>(value));
            return print_impl(exprs + 1, types + 1, std::forward<U>(rest)...);
        }

        std::string m_location;

        static constexpr std::size_t MAX_PATH_LENGTH = 20;
    };

    // Identity function to suppress "-Wunused-value" warnings in DBG_MACRO_DISABLE
    // mode
    template <typename T>
    T&& identity(T && t) {
        return std::forward<T>(t);
    }

    template <typename T, typename... U>
    auto identity(T&&, U&&... u) -> last_t<U...> {
        return identity(std::forward<U>(u)...);
    }

}  // namespace dbg

#ifndef DBG_MACRO_DISABLE

// Force expanding argument with commas for MSVC, ref:
// https://stackoverflow.com/questions/35210637/macro-expansion-argument-with-commas
// Note that "args" should be a tuple with parentheses, such as "(e1, e2, ...)".
#define DBG_IDENTITY(x) x
#define DBG_CALL(fn, args) DBG_IDENTITY(fn args)

#define DBG_CAT_IMPL(_1, _2) _1##_2
#define DBG_CAT(_1, _2) DBG_CAT_IMPL(_1, _2)

#define DBG_16TH_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, \
                      _14, _15, _16, ...)                                     \
  _16
#define DBG_16TH(args) DBG_CALL(DBG_16TH_IMPL, args)
#define DBG_NARG(...) \
  DBG_16TH((__VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

// DBG_VARIADIC_CALL(fn, data, e1, e2, ...) => fn_N(data, (e1, e2, ...))
#define DBG_VARIADIC_CALL(fn, data, ...) \
  DBG_CAT(fn##_, DBG_NARG(__VA_ARGS__))(data, (__VA_ARGS__))

// (e1, e2, e3, ...) => e1
#define DBG_HEAD_IMPL(_1, ...) _1
#define DBG_HEAD(args) DBG_CALL(DBG_HEAD_IMPL, args)

// (e1, e2, e3, ...) => (e2, e3, ...)
#define DBG_TAIL_IMPL(_1, ...) (__VA_ARGS__)
#define DBG_TAIL(args) DBG_CALL(DBG_TAIL_IMPL, args)

#define DBG_MAP_1(fn, args) DBG_CALL(fn, args)
#define DBG_MAP_2(fn, args) fn(DBG_HEAD(args)), DBG_MAP_1(fn, DBG_TAIL(args))
#define DBG_MAP_3(fn, args) fn(DBG_HEAD(args)), DBG_MAP_2(fn, DBG_TAIL(args))
#define DBG_MAP_4(fn, args) fn(DBG_HEAD(args)), DBG_MAP_3(fn, DBG_TAIL(args))
#define DBG_MAP_5(fn, args) fn(DBG_HEAD(args)), DBG_MAP_4(fn, DBG_TAIL(args))
#define DBG_MAP_6(fn, args) fn(DBG_HEAD(args)), DBG_MAP_5(fn, DBG_TAIL(args))
#define DBG_MAP_7(fn, args) fn(DBG_HEAD(args)), DBG_MAP_6(fn, DBG_TAIL(args))
#define DBG_MAP_8(fn, args) fn(DBG_HEAD(args)), DBG_MAP_7(fn, DBG_TAIL(args))
#define DBG_MAP_9(fn, args) fn(DBG_HEAD(args)), DBG_MAP_8(fn, DBG_TAIL(args))
#define DBG_MAP_10(fn, args) fn(DBG_HEAD(args)), DBG_MAP_9(fn, DBG_TAIL(args))
#define DBG_MAP_11(fn, args) fn(DBG_HEAD(args)), DBG_MAP_10(fn, DBG_TAIL(args))
#define DBG_MAP_12(fn, args) fn(DBG_HEAD(args)), DBG_MAP_11(fn, DBG_TAIL(args))
#define DBG_MAP_13(fn, args) fn(DBG_HEAD(args)), DBG_MAP_12(fn, DBG_TAIL(args))
#define DBG_MAP_14(fn, args) fn(DBG_HEAD(args)), DBG_MAP_13(fn, DBG_TAIL(args))
#define DBG_MAP_15(fn, args) fn(DBG_HEAD(args)), DBG_MAP_14(fn, DBG_TAIL(args))
#define DBG_MAP_16(fn, args) fn(DBG_HEAD(args)), DBG_MAP_15(fn, DBG_TAIL(args))

// DBG_MAP(fn, e1, e2, e3, ...) => fn(e1), fn(e2), fn(e3), ...
#define DBG_MAP(fn, ...) DBG_VARIADIC_CALL(DBG_MAP, fn, __VA_ARGS__)

#define DBG_STRINGIFY_IMPL(x) #x
#define DBG_STRINGIFY(x) DBG_STRINGIFY_IMPL(x)

#define DBG_TYPE_NAME(x) dbg::type_name<decltype(x)>()

#define METADOT_DBG(...)                                    \
  dbg::DebugOutput(__FILE__, __LINE__, __func__)    \
      .print({DBG_MAP(DBG_STRINGIFY, __VA_ARGS__)}, \
             {DBG_MAP(DBG_TYPE_NAME, __VA_ARGS__)}, __VA_ARGS__)
#else
#define dbg(...) dbg::identity(__VA_ARGS__)
#endif  // DBG_MACRO_DISABLE

#endif  // DBG_MACRO_DBG_H
























class b2DebugDraw_impl : public b2Draw {
public:
    METAENGINE_Render_Target* target;
    float xOfs = 0;
    float yOfs = 0;
    float scale = 1;

    b2DebugDraw_impl(METAENGINE_Render_Target* target);
    ~b2DebugDraw_impl();

    void Create();
    void Destroy();

    b2Vec2 transform(const b2Vec2& pt);

    SDL_Color convertColor(const b2Color& color);

    void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);

    void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);

    void DrawCircle(const b2Vec2& center, float radius, const b2Color& color);

    void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color);

    void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);

    void DrawTransform(const b2Transform& xf);

    void DrawPoint(const b2Vec2& p, float size, const b2Color& color);

    void DrawString(int x, int y, const char* string, ...);

    void DrawString(const b2Vec2& p, const char* string, ...);

    void DrawAABB(b2AABB* aabb, const b2Color& color);

};


