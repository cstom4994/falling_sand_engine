

//! \file classget.hpp

#pragma once
#ifndef META_CLASSGET_HPP
#define META_CLASSGET_HPP

#include "engine/meta/config.hpp"
#include "engine/meta/classmanager.hpp"
#include "engine/meta/typeid.hpp"
#include "engine/meta/util.hpp"
#include "engine/meta/error.hpp"
#include <string>

namespace Meta {

/**
 * \brief Get the total number of existing metaclasses
 *
 * \relates Class
 *
 * \return Global metaclass count
 */
size_t classCount();

/**
 * \brief Get an iterator that can be used to iterate over all registered classes
 *
 * \relates Class
 * \snippet inspect.cpp classIterator
 *
 * \return ClassView
 */
detail::ClassManager::ClassView classes();

/**
 * \brief Get a metaclass from its name
 *
 * \note Automated registration does not occur when using this lookup call (since we don't
 *       have the object type). Use META_TYPE() registration if you use this.
 *
 * \relates Class
 *
 * \param name Name of the metaclass to retrieve (case sensitive)
 *
 * \return Reference to the requested metaclass
 *
 * \throw ClassNotFound name is not a valid metaclass name
 */
const Class& classByName(IdRef name);

/**
 * \brief Get a metaclass from a C++ object
 *
 * \relates Class
 *
 * \param object object to get the metaclass of
 *
 * \return Reference to the metaclass bound to type T
 *
 * \throw ClassNotFound no metaclass has been declared for T or any of its bases
 */
template <typename T>
const Class& classByObject(const T& object);

/**
 * \brief Get a metaclass from its C++ type
 *
 * \relates Class
 *
 * \return Reference to the metaclass bound to type T
 *
 * \throw ClassNotFound no metaclass has been declared for T
 */
template <typename T>
const Class& classByType();

/**
 * \brief Get a metaclass from its C++ type
 *
 * \relates Class
 *
 * \return Pointer to the metaclass bound to type T, or null pointer if no metaclass has
 *         been declared
 */
template <typename T>
const Class* classByTypeSafe();

}  // namespace Meta

namespace Meta {

inline size_t classCount() { return detail::ClassManager::instance().count(); }

inline detail::ClassManager::ClassView classes() { return detail::ClassManager::instance().getClasses(); }

inline const Class& classByName(IdRef name) {
    // Note: detail::typeName() not used here so no automated registration.
    return detail::ClassManager::instance().getByName(name);
}

template <typename T>
const Class& classByObject(const T& object) {
    return detail::ClassManager::instance().getById(detail::getTypeId(object));
}

template <typename T>
const Class& classByType() {
    return detail::ClassManager::instance().getById(detail::getTypeId<T>());
}

template <typename T>
const Class* classByTypeSafe() {
    return detail::ClassManager::instance().getByIdSafe(detail::calcTypeId<typename detail::DataType<T>::Type>());
}

}  // namespace Meta

#endif  // META_CLASSGET_HPP
