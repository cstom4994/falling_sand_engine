

#ifndef META_ENUMGET_HPP
#define META_ENUMGET_HPP

#include <string>

#include "meta/enummanager.hpp"
#include "meta/error.hpp"
#include "meta/typeid.hpp"

namespace Meta {

/**
 * \relates Enum
 *
 * \brief Get the total number of existing metaenums
 *
 * \return Global metaenum count
 */
size_t enumCount();

/**
 * \relates Enum
 *
 * \brief Get a metaenum from its name
 *
 * \param name Name of the metaenum to retrieve (case sensitive)
 *
 * \return Reference to the requested metaenum
 *
 * \throw EnumNotFound name is not a valid metaenum name
 */
const Enum& enumByName(IdRef name);

/**
 * \relates Enum
 *
 * \brief Get a metaenum from a C++ object
 *
 * It is equivalent to calling enumByType<T>(index).
 *
 * \param value Value to get the metaenum of
 *
 * \return Reference to the metaenum bound to type T
 *
 * \throw EnumNotFound no metaenum has been declared for T
 */
template <typename T>
const Enum& enumByObject(T value);

/**
 * \relates Enum
 *
 * \brief Get a metaenum from its C++ type
 *
 * \return Reference to the metaenum bound to type T
 *
 * \throw EnumNotFound no metaenum has been declared for T
 */
template <typename T>
const Enum& enumByType();

/**
 * \relates Enum
 *
 * \brief Get a metaenum from its C++ type
 *
 * \return Pointer to the metaenum bound to type T, or null pointer if no metaenum has been declared
 */
template <typename T>
const Enum* enumByTypeSafe();

}  // namespace Meta

namespace Meta {

inline size_t enumCount() { return detail::EnumManager::instance().count(); }

inline const Enum& enumByName(IdRef name) { return detail::EnumManager::instance().getByName(name); }

template <typename T>
const Enum& enumByObject(T) {
    return detail::EnumManager::instance().getById(detail::getTypeId<T>());
}

template <typename T>
const Enum& enumByType() {
    return detail::EnumManager::instance().getById(detail::getTypeId<T>());
}

template <typename T>
const Enum* enumByTypeSafe() {
    return detail::EnumManager::instance().getByIdSafe(detail::calcTypeId<T>());
}

}  // namespace Meta

#endif  // META_ENUMGET_HPP
