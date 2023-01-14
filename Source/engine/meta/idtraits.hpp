

#ifndef META_ID_TRAITS_HPP
#define META_ID_TRAITS_HPP

#ifdef META_ID_TRAITS_STD_STRING

#include <string>

namespace Meta {

typedef std::size_t size_t;

namespace detail {

struct IdTraits {
    typedef std::string string_t;
    typedef string_t id_value_t;
    typedef const id_value_t& id_ref_t;
    typedef const id_value_t& id_return_t;

    static inline const char* cstr(id_ref_t r) { return r.c_str(); }
};

}  // namespace detail
}  // namespace Meta

#elif defined(META_ID_TRAITS_STRING_VIEW)

#include <string>

#include "engine/meta/string_view.hpp"

namespace Meta {

typedef std::size_t size_t;

namespace detail {

struct IdTraits {
    typedef std::string string_t;
    typedef string_t id_value_t;
    typedef string_view id_ref_t;
    typedef const id_value_t& id_return_t;

    static inline const char* cstr(id_ref_t r) { return r.data(); }
};

}  // namespace detail
}  // namespace Meta

#endif  // META_ID_TRAITS_STRING_VIEW

namespace Meta {

/// Meta string class. Needs to have traits of std::string.
typedef detail::IdTraits::string_t String;

/// Type used to hold identifier values.
typedef detail::IdTraits::id_value_t Id;

/// Type used to pass around references to the an identifier type.
typedef detail::IdTraits::id_ref_t IdRef;

/// Type used to return a identifier value.
typedef detail::IdTraits::id_return_t IdReturn;

namespace id {

static inline const char* c_str(IdRef r) { return Meta::detail::IdTraits::cstr(r); }

}  // namespace id

}  // namespace Meta

#endif  // META_ID_TRAITS_HPP
