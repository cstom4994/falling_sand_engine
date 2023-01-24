

#ifndef META_CLASSBUILDER_HPP
#define META_CLASSBUILDER_HPP

#include <cassert>
#include <string>

#include "meta/class.hpp"
#include "meta/classget.hpp"
#include "meta/constructorimpl.hpp"
#include "meta/functionimpl.hpp"
#include "meta/pondertype.hpp"
#include "meta/propertyfactory.hpp"
#include "meta/type.hpp"
#include "meta/userdata.hpp"
#include "meta/uses.hpp"

namespace Meta {

/**
 * \brief Proxy class which fills a metaclass with its members
 *
 * This class is returned by Class::declare<T> in order construct a
 * new metaclass. It contains functions to declare and bind metaproperties,
 * metafunctions, base metaclasses, metaconstructors, etc. with many overloads
 * in order to accept as many types of binds as possible.
 *
 * ClassBuilder also contains functions to set attributes of metafunctions
 * and metaproperties.
 *
 * This class should never be explicitely instantiated, unless you
 * need to split the metaclass creation in multiple parts.
 */
template <typename T>
class ClassBuilder {
public:
    /**
     * \brief Construct the builder with a target metaclass to fill
     *
     * \param target Metaclass to build
     */
    ClassBuilder(Class& target);

    /**
     * \brief Declare a base metaclass
     *
     * The template parameter U is the C++ base class of T.
     *
     * This function makes the target metaclass inherit of all the metaproperties and
     * metafunctions of the given base metaclass.
     *
     * \note We *do not* support virtual inheritance fully here due to the associated problems
     *       with compiler specific class layouts. e.g. see Class::applyOffset.
     *
     * \return Reference to this, in order to chain other calls
     *
     * \throw ClassNotFound no metaclass is bound to U
     */
    template <typename U>
    ClassBuilder<T>& base();

    /**
     * \brief Declare a new property from a single accessor
     *
     * The accessor parameter can be a getter of any valid type, or a direct
     * pointer-to-member (which is considered both a getter and a setter)
     *
     * Example:
     *
     * \code
     * struct Point
     * {
     *     float x, y;
     *
     *     float length() const;
     * };
     *
     * Meta::Class::declare<Point>("Point")
     *     .property("x",      &Point::x)       // getter + setter
     *     .property("y",      &Point::y)       // getter + setter
     *     .property("length", &Point::length); // getter only
     * \endcode

     * \param name Name of the property (must be unique within the metaclass)
     * \param accessor Accessor to the C++ implementation of the property
     *
     * \return Reference to this, in order to chain other calls
     */
    template <typename F>
    ClassBuilder<T>& property(IdRef name, F accessor);

    /**
     * \brief Declare a new property from a pair of accessors
     *
     * The accessor1 and accessor2 parameters can be a pair of getter/setter, or
     * two getters which must be composed to form a single getter.
     * If F1 is a direct pointer-to-member, it is considered both a getter and a setter.
     *
     * Having two getters allows to expose a property which requires an extra level of
     * indirection to be accessed (for example, a property of a member of the class instead of
     * a property of the class itself).
     *
     * Example:
     *
     * \code
     * struct Point {float x, y;};
     *
     * class Entity
     * {
     * public:
     *     Point p;
     * };
     *
     * Meta::Class::declare<Entity>("Entity")
     *     .property("x", &Point::x, &Entity::p)  // will internally resolve to e.p.x
     *     .property("y", &Point::y, &Entity::p); // will internally resolve to e.p.y
     * \endcode
     *
     * \param name Name of the property (must be unique within the metaclass)
     * \param accessor1 First accessor to the C++ implementation of the property (getter)
     * \param accessor2 Second accessor to the C++ implementation of the property (setter or
     *        getter to compose)
     * \return Reference to this, in order to chain other calls
     */
    template <typename F1, typename F2>
    ClassBuilder<T>& property(IdRef name, F1 accessor1, F2 accessor2);

    /**
     * \brief Declare a new function from any bindable type
     *
     * The function parameter can be any valid type: a non-member function,
     * member function, const, non-const, lambda, etc. Polices can be applied to the
     * function to affect things like the way objects are returned. See \ref Meta::policy.
     *
     * \param name Name of the function (must be unique within the metaclass)
     * \param function C++ callable entity to bind to the function
     * \param policies Optional policies applied to function exposer
     * \return Reference to this, in order to chain other calls
     *
     * \sa property(), Meta::policy, \ref eg_page_shapes
     */
    template <typename F, typename... P>
    ClassBuilder<T>& function(IdRef name, F function, P... policies);

    /**
     * \brief Declare a constructor for the metaclass.
     *
     * Variable number of parameters can be passed.
     *
     * \return Reference to this, in order to chain other calls
     */
    template <typename... A>
    ClassBuilder<T>& constructor();

    /**
     * \brief Add properties and/or functions from an external source
     *
     * The purpose of this function is to allow the binding of classes
     * that already use a similar system of metaproperties and metafunctions,
     * with a direct mapping from external attributes to Meta ones.
     *
     * The mapping process must be done in a specific mapper class (see below), thus avoiding
     * to manually write the mapping for every class.
     *
     * The mapper class must accept a template parameter (which is the target C++ class)
     * and be compatible with the following interface:
     *
     * \code
     * template <typename T>
     * class MyClassMapper
     * {
     * public:
     *     MyClassMapper();
     *
     *     size_t propertyCount();
     *     Meta::Property* property(size_t index);
     *
     *     size_t functionCount();
     *     Meta::Function* function(size_t index);
     * };
     * \endcode
     *
     * Example of usage:
     *
     * \code
     * Meta::Class::declare<MyClass>("MyClass")
     *     .external<MyClassMapper>()
     *     ...
     * \endcode
     *
     * \return Reference to this, in order to chain other calls
     */
    template <template <typename> class U>
    ClassBuilder<T>& external();

    /**
     * \brief Add user data to the last declared member type
     *
     * \code
     * Meta::Class::declare<MyClass>("MyClass")
     *     .function("foo", &MyClass::foo)( Meta::UserData("user", 3) );
     * \endcode
     *
     * \return Reference to this, in order to chain other calls
     */
    template <typename... U>
    ClassBuilder<T>& operator()(U&&... uds) {
        const std::initializer_list<UserData> il = {uds...};
        for (UserData const& ud : il) userDataStore()->setValue(*m_currentType, ud.getName(), ud.getValue());
        return *this;
    }

private:
    ClassBuilder<T>& addProperty(Property* property);
    ClassBuilder<T>& addFunction(Function* function);

    Class* m_target;      // Target metaclass to fill
    Type* m_currentType;  // Last member type which has been declared
};

}  // namespace Meta

namespace Meta {

template <typename T>
ClassBuilder<T>::ClassBuilder(Class& target) : m_target(&target), m_currentType(&target) {}

template <typename T>  // class
template <typename U>  // base
ClassBuilder<T>& ClassBuilder<T>::base() {
    // Retrieve the base metaclass and its name
    const Class& baseClass = classByType<U>();
    IdReturn baseName = baseClass.name();

    // First make sure that the base class is not already a base of the current class
    for (Class::BaseInfo const& bi : m_target->m_bases) {
        if (bi.base->name() == baseName) META_ERROR(TypeAmbiguity(bi.base->name()));
    }

    // Compute the offset to apply for pointer conversions
    // - Note we do NOT support virtual inheritance here due to the associated problems
    //   with compiler specific class layouts.
    // - Use pointer dummy buffer here as some platforms seem to trap bad memory access even
    //   though not dereferencing the pointer.
    // - U : Base, T : Derived.
    char dummy[8];
    T* asDerived = reinterpret_cast<T*>(dummy);
    U* asBase = static_cast<U*>(asDerived);
    const int offset = static_cast<int>(reinterpret_cast<char*>(asBase) - reinterpret_cast<char*>(asDerived));

    // Add the base metaclass to the bases of the current class
    Class::BaseInfo baseInfos;
    baseInfos.base = &baseClass;
    baseInfos.offset = offset;
    m_target->m_bases.push_back(baseInfos);

    // Copy all properties of the base class into the current class
    for (auto&& it = baseClass.m_properties.begin(); it != baseClass.m_properties.end(); ++it) {
        m_target->m_properties.insert(it);
    }

    // Copy all functions of the base class into the current class
    for (auto&& it = baseClass.m_functions.begin(); it != baseClass.m_functions.end(); ++it) {
        m_target->m_functions.insert(it);
    }

    return *this;
}

template <typename T>
template <typename F>
ClassBuilder<T>& ClassBuilder<T>::property(IdRef name, F accessor) {
    return addProperty(detail::PropertyFactory1<T, F>::create(name, accessor));
}

template <typename T>
template <typename F1, typename F2>
ClassBuilder<T>& ClassBuilder<T>::property(IdRef name, F1 accessor1, F2 accessor2) {
    return addProperty(detail::PropertyFactory2<T, F1, F2>::create(name, accessor1, accessor2));
}

template <typename T>
template <typename F, typename... P>
ClassBuilder<T>& ClassBuilder<T>::function(IdRef name, F function, P... policies) {
    // Construct and add the metafunction
    return addFunction(detail::newFunction(name, function, policies...));
}

template <typename T>
template <typename... A>
ClassBuilder<T>& ClassBuilder<T>::constructor() {
    Constructor* constructor = new detail::ConstructorImpl<T, A...>();
    m_target->m_constructors.push_back(Class::ConstructorPtr(constructor));
    return *this;
}

template <typename T>
template <template <typename> class U>
ClassBuilder<T>& ClassBuilder<T>::external() {
    // Create an instance of the mapper
    U<T> mapper;

    // Retrieve the properties
    size_t propertyCount = mapper.propertyCount();
    for (size_t i = 0; i < propertyCount; ++i) addProperty(mapper.property(i));

    // Retrieve the functions
    size_t functionCount = mapper.functionCount();
    for (size_t i = 0; i < functionCount; ++i) addFunction(mapper.function(i));

    return *this;
}

template <typename T>
ClassBuilder<T>& ClassBuilder<T>::addProperty(Property* property) {
    // Retrieve the class' properties indexed by name
    Class::PropertyTable& properties = m_target->m_properties;

    // First remove any property that already exists with the same name
    properties.erase(property->name());

    // Insert the new property
    properties.insert(property->name(), Class::PropertyPtr(property));

    m_currentType = property;

    return *this;
}

template <typename T>
ClassBuilder<T>& ClassBuilder<T>::addFunction(Function* function) {
    // Retrieve the class' functions indexed by name
    Class::FunctionTable& functions = m_target->m_functions;

    // First remove any function that already exists with the same name
    functions.erase(function->name());

    // Insert the new function
    functions.insert(function->name(), Class::FunctionPtr(function));

    m_currentType = function;

    return *this;
}

}  // namespace Meta

#endif  // META_CLASSBUILDER_HPP
