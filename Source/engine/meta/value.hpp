

#pragma once
#ifndef META_VALUE_HPP
#define META_VALUE_HPP

#include "engine/meta/valueimpl.hpp"
#include "engine/meta/variant.hpp"
#include "engine/meta/enumobject.hpp"
#include "engine/meta/type.hpp"
#include "engine/meta/userobject.hpp"
#include "engine/meta/valuemapper.hpp"
#include <iosfwd>
#include <string>

namespace Meta {

/**
 * \brief Variant class which is used to wrap values in the Meta system
 *
 * The Value class can store any type of variable, and supports conversion
 * to any type compatible with the stored type.
 *
 * \code
 * Meta::Value v1 = true;
 * Meta::Value v2 = 10;
 * Meta::Value v3 = "24.5";
 * Meta::Value v4 = myObject;
 *
 * bool        b = v1; // b == true
 * String      s = v2; // s == "10"
 * float       f = v3; // f == 24.5
 * MyObject    o = v4; // o == myObject
 * \endcode
 *
 * It also supports unary and binary visitation for type-safe processing
 * depending on the stored type.
 *
 * \remark The set of supported types can be extended by specializing the
 * MetaExt::ValueMapper template.
 *
 * \sa ValueVisitor, MetaExt::ValueMapper
 */
class Value {
public:
    /**
     * \brief Default constructor, constructs a null value
     */
    Value();

    /**
     * \brief Construct the value from a variable of type T
     *
     * \param val Value to store
     */
    template <typename T>
    Value(const T& val);

    /**
     * \brief Copy constructor
     *
     * \param other Value to copy
     */
    Value(const Value& other);

    /**
     * \brief Move constructor
     *
     * \param other Value to move
     */
    Value(Value&& other);

    /**
     * \brief Assignment operator
     *
     * \param other Value to assign to this
     */
    void operator=(const Value& other);

    /**
     * \brief Return the Meta runtime kind of the value
     *
     * \return Type of the value
     */
    ValueKind kind() const;

    /**
     * \brief Convert the value to the type T
     *
     * Convert the value contained to the type provided. An exception is throw if the target
     * type is not compatible. The value returned will be a copy. See ref() and cref() for
     * referencing the internal data.
     *
     * \return Value converted to T
     *
     * \throw BadType the stored value is not convertible to T
     */
    template <typename T>
    T to() const;

    /**
     * \brief Get a reference to the value data contained
     *
     * Get a reference to the contained value of type T. The user is responsible for ensuring
     * that the type passed is correct. See cref() for a non-const reference, or to() to
     * convert the value.
     *
     * \return A non-const reference to the contained data.
     */
    template <typename T>
    T& ref();

    /**
     * \brief Get a const reference to the value data contained
     *
     * Get a const reference to the contained value of type T. The user is responsible for
     * ensuring that the type passed is correct. See ref() for a const reference, or to() to
     * convert the value.
     *
     * \return A const reference to the contained data.
     */
    template <typename T>
    const T& cref() const;

    /**
     * \brief Check if the stored value can be converted to a type T
     *
     * If this function returns true, then calling to<T>() or operator T() will succeed.
     *
     * \return True if conversion is possible, false otherwise
     */
    template <typename T>
    bool isCompatible() const;

    /**
     * \brief Visit the value with a unary visitor
     *
     * Using this function allows to dispatch an operation depending on the stored type.
     *
     * \param visitor Visitor to apply (must inherit from ValueVisitor<type_to_return>)
     *
     * \return Value returned by the visitor
     */
    template <typename T>
    typename T::result_type visit(T visitor) const;

    /**
     * \brief Visit the value and another one with a binary visitor
     *
     * Using this function allows to dispatch a binary operation depending on the stored type
     * of both values.
     *
     * \param visitor Visitor to apply (must inherit from ValueVisitor<type_to_return>)
     * \param other Other value to visit
     *
     * \return Value returned by the visitor
     */
    template <typename T>
    typename T::result_type visit(T visitor, const Value& other) const;

    /**
     * \brief Operator == to compare equality between two values
     *
     * Two values are equal if their Meta type and value are equal.
     * It uses the == operator of the stored type.
     *
     * \param other Value to compare with this
     *
     * \return True if both values are the same, false otherwise
     */
    bool operator==(const Value& other) const;

    /**
     * \brief Operator != to compare equality between two values
     *
     * \see operator==
     *
     * \return True if both values are not the same, false otherwise
     */
    bool operator!=(const Value& other) const { return !(*this == other); }

    /**
     * \brief Operator < to compare two values
     *
     * \param other Value to compare with this
     *
     * \return True if this < other
     */
    bool operator<(const Value& other) const;

    /**
     * \brief Operator > to compare two values
     *
     * \param other Value to compare with this
     *
     * \return True if this > other
     */
    bool operator>(const Value& other) const { return !(*this < other || *this == other); }

    /**
     * \brief Operator <= to compare two values
     *
     * \param other Value to compare with this
     *
     * \return True if this <= other
     */
    bool operator<=(const Value& other) const { return (*this < other || *this == other); }

    /**
     * \brief Operator >= to compare two values
     *
     * \param other Value to compare with this
     *
     * \return True if this >= other
     */
    bool operator>=(const Value& other) const { return !(*this < other); }

    /**
     * \brief Special Value instance representing an empty value
     */
    static const Value nothing;

private:
    typedef mapbox::util::variant<NoType, bool, long, double, Meta::String, EnumObject, UserObject, detail::ValueRef> Variant;

    Variant m_value;   // Stored value
    ValueKind m_type;  // Meta type of the value
};

/**
 * \brief Overload of operator >> to extract a Meta::Value from a standard stream
 *
 * \param stream Source input stream
 * \param value Value to fill
 *
 * \return Reference to the input stream
 */
std::istream& operator>>(std::istream& stream, Value& value);

/**
 * \brief Overload of operator << to print a Meta::Value into a standard stream
 *
 * \param stream Target output stream
 * \param value Value to print
 *
 * \return Reference to the output stream
 */
std::ostream& operator<<(std::ostream& stream, const Value& value);

}  // namespace Meta

namespace Meta {
namespace detail {

// Is T a user type.
template <typename T>
struct IsUserType {
    typedef typename detail::DataType<T>::Type DataType;
    static constexpr bool value = std::is_class<DataType>::value && !std::is_same<DataType, Value>::value && !std::is_same<DataType, UserObject>::value &&
                                  !std::is_same<DataType, detail::ValueRef>::value && !std::is_same<DataType, Meta::String>::value;
};

// Decide whether the UserObject holder should be ref (true) or copy (false).
template <typename T>
struct IsUserObjRef {
    static constexpr bool value = std::is_pointer<T>::value || std::is_reference<T>::value;
};

// Convert Meta::Value to type
template <typename T, typename E = void>
struct ValueTo {
    static T convert(const Value& value) { return value.visit(ConvertVisitor<T>()); }
};

// Don't need to convert, we're returning a Value
template <>
struct ValueTo<Value> {
    static Value convert(const Value& value) { return value; }
    static Value convert(Value&& value) { return std::move(value); }
};

// Convert Values to pointers for basic types
template <typename T>
struct ValueTo<T*, typename std::enable_if<!hasStaticTypeDecl<T>()>::type> {
    static T* convert(const Value& value) { return value.to<detail::ValueRef>().getRef<T>(); }
};

// Convert Values to references for basic types
// template <typename T>
// struct ValueTo<T&, typename std::enable_if<!hasStaticTypeDecl<T>()>::type>
//{
//    static T convert(const Value& value)
//    {
//        return *static_cast<T*>(value.to<UserObject>().pointer());
//    }
//};

}  // namespace detail

template <typename T>
Value::Value(const T& val)
    : m_value(MetaExt::ValueMapper<T>::to(val)),
      m_type(MetaExt::ValueMapper<T>::kind)  // mapType<T> NOT used so get same kind as to()
{}

template <typename T>
T Value::to() const {
    try {
        return detail::ValueTo<T>::convert(*this);
    } catch (detail::bad_conversion&) {
        META_ERROR(BadType(kind(), mapType<T>()));
    }
}

template <typename T>
T& Value::ref() {
    try {
        return m_value.get<T>();
    } catch (detail::bad_conversion&) {
        META_ERROR(BadType(kind(), mapType<T>()));
    }
}

template <typename T>
const T& Value::cref() const {
    try {
        return m_value.get<T>();
    } catch (detail::bad_conversion&) {
        META_ERROR(BadType(kind(), mapType<T>()));
    }
}

template <typename T>
bool Value::isCompatible() const {
    try {
        to<T>();
        return true;
    } catch (std::exception&) {
        return false;
    }
}

template <typename T>
typename T::result_type Value::visit(T visitor) const {
    return mapbox::util::apply_visitor(visitor, m_value);
}

template <typename T>
typename T::result_type Value::visit(T visitor, const Value& other) const {
    return mapbox::util::apply_visitor(visitor, m_value, other.m_value);
}

}  // namespace Meta

#endif  // META_VALUE_HPP
