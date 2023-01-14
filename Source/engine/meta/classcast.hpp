

#pragma once
#ifndef META_CLASSCAST_HPP
#define META_CLASSCAST_HPP

#include "engine/meta/config.hpp"

namespace Meta
{
class Class;

/**
 * \brief Convert a pointer from a source metaclass to a related target metaclass
 *
 * \param pointer Source pointer to convert
 * \param sourceClass Source metaclass to convert from
 * \param targetClass Target metaclass to convert to
 *
 * \return Converted pointer, or 0 on failure
 *
 * \throw ClassUnrelated sourceClass is not a base nor a derived of targetClass
 */
void* classCast(void* pointer, const Class& sourceClass, const Class& targetClass);

} // namespace Meta

#endif // META_CLASSCAST_HPP
