

#ifndef META_ENUMOBJECT_HPP
#define META_ENUMOBJECT_HPP

#include <string>

#include "meta/enumget.hpp"

namespace Meta {

/**
 * \brief Wrapper to manipulate enumerated values in the Meta system
 *
 * Meta::EnumObject is an abstract representation of enum values, and supports
 * conversions from strings and integers.
 *
 * \sa UserObject
 */
class EnumObject {
public:
    /**
     * \brief Construct the enum object from an enumerated value
     *
     * \param value Value to store in the enum object
     */
    template <typename T>
    EnumObject(T value, typename std::enable_if<std::is_enum<T>::value>::type* = 0);

    /**
     * \brief Get the value of the enum object
     *
     * \return Integer value of the enum object
     */
    long value() const;

    /**
     * \brief Get the value of the enum class object
     *
     * \return Enum typed value of the enum class object
     */
    template <typename E>
    E value() const;

    /**
     * \brief Get the name of the enum object
     *
     * \return String containing the name of the enum object
     */
    IdReturn name() const;

    /**
     * \brief Retrieve the metaenum of the stored enum object
     *
     * \return Reference to the object's metaenum
     */
    const Enum& getEnum() const;

    /**
     * \brief Operator == to compare equality between two enum objects
     *
     * Two enum objects are equal if their metaenums and values are both equal
     *
     * \param other Enum object to compare with this
     *
     * \return True if both enum objects are the same, false otherwise
     */
    bool operator==(const EnumObject& other) const;

    /**
     * \brief Operator < to compare two enum objects
     *
     * \param other Enum object to compare with this
     *
     * \return True if this < other
     */
    bool operator<(const EnumObject& other) const;

private:
    long m_value;        ///< Value
    const Enum* m_enum;  ///< Metaenum associated to the value
};

}  // namespace Meta

namespace Meta {

template <typename T>
inline EnumObject::EnumObject(T value, typename std::enable_if<std::is_enum<T>::value>::type*) : m_value(static_cast<long>(value)), m_enum(&enumByType<T>()) {}

template <typename E>
inline E EnumObject::value() const {
    return static_cast<E>(value());
}

}  // namespace Meta

#endif  // META_ENUMOBJECT_HPP
