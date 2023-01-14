

#ifndef META_PROPERTY_HPP
#define META_PROPERTY_HPP

#include "engine/meta/value.hpp"

namespace Meta {

class ClassVisitor;

/**
 * \brief Abstract representation of a property
 *
 * Properties are members of metaclasses. Their main purpose is to be get and/or set;
 * They also provide detailed informations about their type.
 *
 * \sa SimpleProperty, ArrayProperty, EnumProperty, ObjectProperty
 */
class Property : public Type {
    META__NON_COPYABLE(Property);

public:
    /**
     * \brief Destructor
     */
    virtual ~Property();

    /**
     * \brief Get the name of the property
     *
     * \return Name of the property
     */
    IdReturn name() const;

    /**
     * \brief Get the type of the property
     *
     * \return Type of the property
     */
    ValueKind kind() const;

    /**
     * \brief Check if the property can be read
     *
     * \return True if the property can be read, false otherwise
     */
    virtual bool isReadable() const;

    /**
     * \brief Check if the property can be written
     *
     * \return True if the property can be written, false otherwise
     */
    virtual bool isWritable() const;

    /**
     * \brief Get the current value of the property for a given object
     *
     * \param object Object
     *
     * \return Value of the property
     *
     * \throw NullObject object is invalid
     * \throw ForbiddenRead property is not readable
     */
    Value get(const UserObject& object) const;

    /**
     * \brief Set the current value of the property for a given object
     *
     * \param object Object
     * \param value New value to assign to the property
     *
     * \throw NullObject \a object is invalid
     * \throw ForbiddenWrite property is not writable
     * \throw BadType \a value can't be converted to the property's type
     */
    void set(const UserObject& object, const Value& value) const;

    /**
     * \brief Accept the visitation of a ClassVisitor
     *
     * \param visitor Visitor to accept
     */
    virtual void accept(ClassVisitor& visitor) const;

protected:
    template <typename T>
    friend class ClassBuilder;
    friend class UserObject;

    /**
     * \brief Construct the property from its description
     *
     * \param name Name of the property
     * \param type Type of the property
     */
    Property(IdRef name, ValueKind type);

    /**
     * \brief Do the actual reading of the value
     *
     * This function is a pure virtual which has to be implemented in derived classes.
     *
     * \param object Object
     *
     * \return Value of the property
     */
    virtual Value getValue(const UserObject& object) const = 0;

    /**
     * \brief Do the actual writing of the value
     *
     * This function is a pure virtual which has to be implemented in derived classes.
     *
     * \param object Object
     * \param value New value to assign to the property
     */
    virtual void setValue(const UserObject& object, const Value& value) const = 0;

private:
    Id m_name;         // Name of the property
    ValueKind m_type;  // Type of the property
};

}  // namespace Meta

#endif  // META_PROPERTY_HPP
