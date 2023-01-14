

#pragma once
#ifndef META_USERPROPERTY_HPP
#define META_USERPROPERTY_HPP

#include "engine/meta/property.hpp"

namespace Meta
{
class Class;

/**
 * \brief Specialized type of property for user types.
 */
class UserProperty : public Property
{
public:

    /**
     * \brief Construct the property from its description
     *
     * \param name Name of the property
     * \param propClass Eumeration the property is bound to
     */
    UserProperty(IdRef name, const Class& propClass);

    /**
     * \brief Destructor
     */
    virtual ~UserProperty();

    /**
     * \brief Get the owner class
     *
     * \return Class the property is bound to
     */
    const Class& getClass() const;

    /**
     * \brief Accept the visitation of a ClassVisitor
     *
     * \param visitor Visitor to accept
     */
    void accept(ClassVisitor& visitor) const override;

private:

    const Class* m_class; ///< Owner class of the property
};

} // namespace Meta

#endif // META_ENUMPROPERTY_HPP
