

#ifndef META_ENUMPROPERTY_HPP
#define META_ENUMPROPERTY_HPP

#include "meta/property.hpp"

namespace Meta {

class Enum;

/**
 * \brief Specialized type of property for enums
 *
 */
class EnumProperty : public Property {
public:
    /**
     * \brief Construct the property from its description
     *
     * \param name Name of the property
     * \param propEnum Eumeration the property is bound to
     */
    EnumProperty(IdRef name, const Enum& propEnum);

    /**
     * \brief Destructor
     */
    virtual ~EnumProperty();

    /**
     * \brief Get the owner enum
     *
     * \return Enum the property is bound to
     */
    const Enum& getEnum() const;

    /**
     * \brief Accept the visitation of a ClassVisitor
     *
     * \param visitor Visitor to accept
     */
    void accept(ClassVisitor& visitor) const override;

private:
    const Enum* m_enum;  // Owner enum of the property
};

}  // namespace Meta

#endif  // META_ENUMPROPERTY_HPP
