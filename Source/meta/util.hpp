

#ifndef META_UTIL_HPP
#define META_UTIL_HPP

#include <memory>
#include <type_traits>

#include "meta/config.hpp"
#include "meta/type.hpp"

namespace Meta {
namespace detail {

//------------------------------------------------------------------------------

template <bool C, typename T, typename F>
struct if_c {
    typedef T type;
};

template <typename T, typename F>
struct if_c<false, T, F> {
    typedef F type;
};

//------------------------------------------------------------------------------

class bad_conversion : public std::exception {};

template <typename T, typename F, typename O = void>
struct convert_impl {
    T operator()(const F& from) {
        const T result = static_cast<T>(from);
        return result;
    }
};

template <typename F>
Id to_str(F from) {
    return std::to_string(from);
}

template <typename F>
struct convert_impl<Id, F> {
    Id operator()(const F& from) { return detail::to_str(from); }
};

template <>
struct convert_impl<Id, bool> {
    Id operator()(const bool& from) {
        static const Id t("1"), f("0");
        return from ? t : f;
    }
};

bool conv(const String& from, bool& to);
bool conv(const String& from, char& to);
bool conv(const String& from, unsigned char& to);
bool conv(const String& from, short& to);
bool conv(const String& from, unsigned short& to);
bool conv(const String& from, int& to);
bool conv(const String& from, unsigned int& to);
bool conv(const String& from, long& to);
bool conv(const String& from, unsigned long& to);
bool conv(const String& from, long long& to);
bool conv(const String& from, unsigned long long& to);
bool conv(const String& from, float& to);
bool conv(const String& from, double& to);

template <typename T>
struct convert_impl<T, Id, typename std::enable_if<(std::is_integral<T>::value || std::is_floating_point<T>::value) && !std::is_const<T>::value && !std::is_reference<T>::value>::type> {
    T operator()(const String& from) {
        T result;
        if (!conv(from, result)) throw detail::bad_conversion();
        return result;
    }
};

template <typename T, typename F>
T convert(const F& from) {
    return convert_impl<T, F>()(from);
}

//------------------------------------------------------------------------------
// index_sequence
// From: http://stackoverflow.com/a/32223343/3233
// A pre C++14 version supplied here. MSVC chokes on this but has its own version.
//
#ifdef _MSC_VER
#define META__SEQNS std
#else
#define META__SEQNS ::Meta::detail

template <size_t... Ints>
struct index_sequence {
    using type = index_sequence;
    using value_type = size_t;
    static constexpr size_t size() { return sizeof...(Ints); }
};

template <class Sequence1, class Sequence2>
struct _merge_and_renumber;

template <size_t... I1, size_t... I2>
struct _merge_and_renumber<index_sequence<I1...>, index_sequence<I2...> > : index_sequence<I1..., (sizeof...(I1) + I2)...> {};

template <size_t N>
struct make_index_sequence : _merge_and_renumber<typename make_index_sequence<N / 2>::type, typename make_index_sequence<N - N / 2>::type> {};

template <>
struct make_index_sequence<0> : index_sequence<> {};
template <>
struct make_index_sequence<1> : index_sequence<0> {};

#endif  // index_sequence

//------------------------------------------------------------------------------
// make_unique supplied for compilers missing it (e.g. clang 5.0 on Travis Linux).
// source: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3656.htm

template <class T>
struct _Unique_if {
    typedef std::unique_ptr<T> _Single_object;
};

template <class T>
struct _Unique_if<T[]> {
    typedef std::unique_ptr<T[]> _Unknown_bound;
};

template <class T, size_t N>
struct _Unique_if<T[N]> {
    typedef void _Known_bound;
};

template <class T, class... Args>
typename _Unique_if<T>::_Single_object make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T>
typename _Unique_if<T>::_Unknown_bound make_unique(size_t n) {
    typedef typename std::remove_extent<T>::type U;
    return std::unique_ptr<T>(new U[n]());
}

template <class T, class... Args>
typename _Unique_if<T>::_Known_bound make_unique(Args&&...) = delete;

//------------------------------------------------------------------------------
// Return true if all args true. Useful for variadic template expansions.
static inline bool allTrue() { return true; }
static inline bool allTrue(bool a0) { return a0; }
static inline bool allTrue(bool a0, bool a1) { return a0 & a1; }
static inline bool allTrue(bool a0, bool a1, bool a2) { return a0 & a1 & a2; }
static inline bool allTrue(bool a0, bool a1, bool a2, bool a3) { return a0 & a1 & a2 & a3; }
static inline bool allTrue(bool a0, bool a1, bool a2, bool a3, bool a4) { return a0 & a1 & a2 & a3 & a4; }
static inline bool allTrue(bool a0, bool a1, bool a2, bool a3, bool a4, bool a5) { return a0 & a1 & a2 & a3 & a4 & a5; }
static inline bool allTrue(bool a0, bool a1, bool a2, bool a3, bool a4, bool a5, bool a6) { return a0 & a1 & a2 & a3 & a4 & a5 & a6; }
static inline bool allTrue(bool a0, bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7) { return a0 & a1 & a2 & a3 & a4 & a5 & a6 & a7; }

// Get value type enum as string description.
const char* valueKindAsString(ValueKind t);

}  // namespace detail
}  // namespace Meta

#endif  // META_UTIL_HPP
