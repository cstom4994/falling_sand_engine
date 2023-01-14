

#ifndef META_CLASSVISITOR_HPP
#define META_CLASSVISITOR_HPP

#include "engine/meta/config.hpp"

namespace Meta {

class Property;
class SimpleProperty;
class ArrayProperty;
class EnumProperty;
class UserProperty;
class Function;

/**
 * \brief Base class for writing custom Class visitors
 *
 * A ClassVisitor, when applied to a Class, will be notified for each member property
 * and function of the target class. To receive this notification for a specific type T
 * of property or function, you have to override the \c visit(T) function.
 *
 * The benefit of such visitation is that you directly get the actual type of the
 * property or function, and can handle it safely without having to use dangerous downcasts.
 *
 * Another benefit of this method is that you can easily filter the properties and functions
 * according to their C++ type: if you don't want to handle a specific type of property or
 * function, just don't override the corresponding \c visit function.
 *
 * Here an example of a visitor which prints the contents of a class:
 * \code
 * class MyClassVisitor : public Meta::ClassVisitor
 * {
 *     void visit(const Meta::SimpleProperty& property)
 *     {
 *         std::cout << "Simple property: " << property.name() << std::endl;
 *     }
 *
 *     void visit(const Meta::ArrayProperty& property)
 *     {
 *         std::cout << "Array property: " << property.name() << " - "
 *                   << "dynamic:" << property.dynamic() << std::endl;
 *     }
 *
 *     void visit(const Meta::EnumProperty& property)
 *     {
 *         std::cout << "Enum property: " << property.name() << " - "
 *                   << "owner enum:" << property.getEnum().name() << std::endl;
 *     }
 *
 *     void visit(const Meta::UserProperty& property)
 *     {
 *         std::cout << "User property: " << property.name() << " - "
 *                   << "owner class:" << property.getClass().name() << std::endl;
 *     }
 *
 *     void visit(const Meta::Function& function)
 *     {
 *         std::cout << "Function: " << function.name() << " - "
 *                   << "number of parameters:" << function.paramCount() << std::endl;
 *     }
 * };
 *
 * const Meta::Class& metaclass = Meta::classByName("MyClass");
 * MyClassVisitor visitor;
 * metaclass.visit(visitor);
 * \endcode
 */
class ClassVisitor {
public:
    /**
     * \brief Destructor
     */
    virtual ~ClassVisitor();

    /**
     * \brief Visit any kind of property
     *
     * This function is a generic callback which can be used
     * to receive all property types which are not handled with
     * their derived type.
     *
     * \param property Property which is being visited
     */
    virtual void visit(const Property& property);

    /**
     * \brief Visit a simple property
     *
     * \param property Property which is being visited
     */
    virtual void visit(const SimpleProperty& property);

    /**
     * \brief Visit an array property
     *
     * \param property Property which is being visited
     */
    virtual void visit(const ArrayProperty& property);

    /**
     * \brief Visit an enum property
     *
     * \param property Property which is being visited
     */
    virtual void visit(const EnumProperty& property);

    /**
     * \brief Visit a user property
     *
     * \param property Property which is being visited
     */
    virtual void visit(const UserProperty& property);

    /**
     * \brief Visit a function
     *
     * \param function Function which is being visited
     */
    virtual void visit(const Function& function);

protected:
    /**
     * \brief Default constructor
     */
    ClassVisitor();
};

}  // namespace Meta

#endif  // META_VALUEVISITOR_HPP
