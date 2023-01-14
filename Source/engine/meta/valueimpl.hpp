

#ifndef META_DETAIL_VALUEIMPL_HPP
#define META_DETAIL_VALUEIMPL_HPP

#include "engine/meta/type.hpp"
#include "engine/meta/valuemapper.hpp"

namespace Meta {
namespace detail {

/**
 * \brief Value visitor which converts the stored value to a type T
 */
template <typename T>
struct ConvertVisitor {
    typedef T result_type;

    template <typename U>
    T operator()(const U& value) const {
        // Dispatch to the proper ValueConverter
        return MetaExt::ValueMapper<T>::from(value);
    }

    // Optimization when source type is the same as requested type
    T operator()(const T& value) const { return value; }
    T operator()(T&& value) const { return std::move(value); }

    T operator()(NoType) const {
        // Error: trying to convert an empty value
        META_ERROR(BadType(ValueKind::None, mapType<T>()));
    }
};

/**
 * \brief Binary value visitor which compares two values using operator <
 */
struct LessThanVisitor {
    typedef bool result_type;

    template <typename T, typename U>
    bool operator()(const T&, const U&) const {
        // Different types : compare types identifiers
        return mapType<T>() < mapType<U>();
    }

    template <typename T>
    bool operator()(const T& v1, const T& v2) const {
        // Same types : compare values
        return v1 < v2;
    }

    bool operator()(NoType, NoType) const {
        // No type (empty values) : they're considered equal
        return false;
    }
};

/**
 * \brief Binary value visitor which compares two values using operator ==
 */
struct EqualVisitor {
    typedef bool result_type;

    template <typename T, typename U>
    bool operator()(const T&, const U&) const {
        // Different types : not equal
        return false;
    }

    template <typename T>
    bool operator()(const T& v1, const T& v2) const {
        // Same types : compare values
        return v1 == v2;
    }

    bool operator()(NoType, NoType) const {
        // No type (empty values) : they're considered equal
        return true;
    }
};

}  // namespace detail
}  // namespace Meta

#endif  // META_DETAIL_VALUEIMPL_HPP
