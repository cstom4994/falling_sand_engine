

#pragma once
#ifndef META_DETAIL_PROPERTYFACTORY_HPP
#define META_DETAIL_PROPERTYFACTORY_HPP

#include "engine/meta/simpleproperty.hpp"

namespace Meta {
namespace detail {

/**
 * \brief Typed implementation of SimpleProperty
 *
 * SimplePropertyImpl is a template implementation of SimpleProperty, which is strongly typed
 * in order to keep track of the true underlying C++ types involved in the property.
 *
 * The template parameter A is an abstract helper to access the actual C++ property.
 *
 * \sa SimpleProperty
 */
template <typename A>
class SimplePropertyImpl : public SimpleProperty {
public:
    /**
     * \brief Construct the property from its accessors
     *
     * \param name Name of the property
     * \param accessor Object used to access the actual C++ property
     */
    SimplePropertyImpl(IdRef name, A accessor);

protected:
    /**
     * \see Property::isReadable
     */
    bool isReadable() const final;

    /**
     * \see Property::isWritable
     */
    bool isWritable() const final;

    /**
     * \see Property::getValue
     */
    Value getValue(const UserObject& object) const final;

    /**
     * \see Property::setValue
     */
    void setValue(const UserObject& object, const Value& value) const final;

private:
    A m_accessor;  // Accessor used to access the actual C++ property
};

}  // namespace detail
}  // namespace Meta

namespace Meta {
namespace detail {

template <typename A>
SimplePropertyImpl<A>::SimplePropertyImpl(IdRef name, A accessor) : SimpleProperty(name, mapType<typename A::DataType>()), m_accessor(accessor) {}

template <typename A>
Value SimplePropertyImpl<A>::getValue(const UserObject& object) const {
    return Value{m_accessor.m_interface.getter(object.get<typename A::ClassType>())};
}

template <typename A>
void SimplePropertyImpl<A>::setValue(const UserObject& object, const Value& value) const {
    if (!m_accessor.m_interface.setter(object.ref<typename A::ClassType>(), value.to<typename A::DataType>())) META_ERROR(ForbiddenWrite(name()));
}

template <typename A>
bool SimplePropertyImpl<A>::isReadable() const {
    return A::canRead;
}

template <typename A>
bool SimplePropertyImpl<A>::isWritable() const {
    return A::canWrite;
}

}  // namespace detail
}  // namespace Meta

#include "engine/meta/arraymapper.hpp"
#include "engine/meta/arrayproperty.hpp"
#include "engine/meta/impl/valueprovider.hpp"

namespace Meta {
namespace detail {

/**
 * \brief Typed implementation of ArrayProperty
 *
 * ArrayPropertyImpl is a template implementation of ArrayProperty, which is strongly typed
 * in order to keep track of the true underlying C++ types involved in the property.
 *
 * The template parameter A is an abstract helper to access the actual C++ property.
 *
 * This class uses the MetaExt::ArrayMapper template to implement its operations according
 * to the type of array.
 *
 * \sa ArrayProperty, MetaExt::ArrayMapper
 */
template <typename A>
class ArrayPropertyImpl final : public ArrayProperty {
public:
    /**
     * \brief Construct the property
     *
     * \param name Name of the property
     * \param accessor Object used to access the actual C++ property
     */
    ArrayPropertyImpl(IdRef name, A&& accessor);

protected:
    /**
     * \see ArrayProperty::getSize
     */
    size_t getSize(const UserObject& object) const final;

    /**
     * \see ArrayProperty::setSize
     */
    void setSize(const UserObject& object, size_t size) const final;

    /**
     * \see ArrayProperty::getElement
     */
    Value getElement(const UserObject& object, size_t index) const final;

    /**
     * \see ArrayProperty::setElement
     */
    void setElement(const UserObject& object, size_t index, const Value& value) const final;

    /**
     * \see ArrayProperty::insertElement
     */
    void insertElement(const UserObject& object, size_t before, const Value& value) const final;

    /**
     * \see ArrayProperty::removeElement
     */
    void removeElement(const UserObject& object, size_t index) const final;

private:
    typedef typename A::ExposedType ArrayType;
    typedef typename A::InterfaceType Mapper;
    typedef typename Mapper::ElementType ElementType;

    /*
     * \brief Retrieve a reference to the array
     * \param object Owner object
     * \return Reference to the underlying array
     */
    ArrayType& array(const UserObject& object) const;

    A m_accessor;  // Object used to access the actual C++ property
};

}  // namespace detail
}  // namespace Meta

namespace Meta {
namespace detail {

template <typename A>
ArrayPropertyImpl<A>::ArrayPropertyImpl(IdRef name, A&& accessor) : ArrayProperty(name, mapType<ElementType>(), Mapper::dynamic()), m_accessor(accessor) {}

template <typename A>
size_t ArrayPropertyImpl<A>::getSize(const UserObject& object) const {
    return Mapper::size(array(object));
}

template <typename A>
void ArrayPropertyImpl<A>::setSize(const UserObject& object, size_t size) const {
    size_t currentSize = getSize(object);
    if (size < currentSize) {
        while (size < currentSize) removeElement(object, --currentSize);
    } else if (size > currentSize) {
        ValueProvider<ElementType> provider;
        while (size > currentSize) insertElement(object, currentSize++, provider());
    }
}

template <typename A>
Value ArrayPropertyImpl<A>::getElement(const UserObject& object, size_t index) const {
    return Mapper::get(array(object), index);
}

template <typename A>
void ArrayPropertyImpl<A>::setElement(const UserObject& object, size_t index, const Value& value) const {
    Mapper::set(array(object), index, value.to<ElementType>());
}

template <typename A>
void ArrayPropertyImpl<A>::insertElement(const UserObject& object, size_t before, const Value& value) const {
    Mapper::insert(array(object), before, value.to<ElementType>());
}

template <typename A>
void ArrayPropertyImpl<A>::removeElement(const UserObject& object, size_t index) const {
    Mapper::remove(array(object), index);
}

template <typename A>
typename ArrayPropertyImpl<A>::ArrayType& ArrayPropertyImpl<A>::array(const UserObject& object) const {
    return m_accessor.m_interface.getter(object.get<typename A::ClassType>());
}

}  // namespace detail
}  // namespace Meta

#include "engine/meta/enumproperty.hpp"

namespace Meta {
namespace detail {

/**
 * \brief Typed implementation of EnumProperty
 *
 * EnumPropertyImpl is a template implementation of EnumProperty, which is strongly typed
 * in order to keep track of the true underlying C++ types involved in the property.
 *
 * The template parameter A is an abstract helper to access the actual C++ property.
 *
 * \sa EnumProperty
 */
template <typename A>
class EnumPropertyImpl : public EnumProperty {
public:
    /**
     * \brief Construct the property from its accessors
     *
     * \param name Name of the property
     * \param accessor Object used to access the actual C++ property
     */
    EnumPropertyImpl(IdRef name, A&& accessor);

protected:
    /**
     * \see Property::isReadable
     */
    bool isReadable() const final;

    /**
     * \see Property::isWritable
     */
    bool isWritable() const final;

    /**
     * \see Property::getValue
     */
    Value getValue(const UserObject& object) const final;

    /**
     * \see Property::setValue
     */
    void setValue(const UserObject& object, const Value& value) const final;

private:
    A m_accessor;
};

}  // namespace detail
}  // namespace Meta

namespace Meta {
namespace detail {

template <typename A>
EnumPropertyImpl<A>::EnumPropertyImpl(IdRef name, A&& accessor) : EnumProperty(name, enumByType<typename A::DataType>()), m_accessor(accessor) {}

template <typename A>
Value EnumPropertyImpl<A>::getValue(const UserObject& object) const {
    return m_accessor.m_interface.getter(object.get<typename A::ClassType>());
}

template <typename A>
void EnumPropertyImpl<A>::setValue(const UserObject& object, const Value& value) const {
    if (!m_accessor.m_interface.setter(object.get<typename A::ClassType>(), value)) META_ERROR(ForbiddenWrite(name()));
}

template <typename A>
bool EnumPropertyImpl<A>::isReadable() const {
    return A::canRead;
}

template <typename A>
bool EnumPropertyImpl<A>::isWritable() const {
    return A::canWrite;
}

}  // namespace detail
}  // namespace Meta

#include "engine/meta/userproperty.hpp"

namespace Meta {
namespace detail {

template <typename A>
class UserPropertyImpl : public UserProperty {
public:
    UserPropertyImpl(IdRef name, A&& accessor);

protected:
    bool isReadable() const final;
    bool isWritable() const final;

    Value getValue(const UserObject& object) const final;
    void setValue(const UserObject& object, const Value& value) const final;

private:
    A m_accessor;  // Object used to access the actual C++ property
};

}  // namespace detail
}  // namespace Meta

namespace Meta {
namespace detail {

template <typename A>
UserPropertyImpl<A>::UserPropertyImpl(IdRef name, A&& accessor) : UserProperty(name, classByType<typename A::DataType>()), m_accessor(accessor) {}

template <typename A>
Value UserPropertyImpl<A>::getValue(const UserObject& object) const {
    // We copy the returned object based on the return type of the getter: (copy) T, const T&, (ref) T&.
    return m_accessor.m_interface.getValue(object.get<typename A::ClassType>());
}

template <typename A>
void UserPropertyImpl<A>::setValue(const UserObject& object, const Value& value) const {
    if (!m_accessor.m_interface.setter(object.ref<typename A::ClassType>(), value)) META_ERROR(ForbiddenWrite(name()));
}

template <typename A>
bool UserPropertyImpl<A>::isReadable() const {
    return A::canRead;
}

template <typename A>
bool UserPropertyImpl<A>::isWritable() const {
    return A::canWrite;
}

}  // namespace detail
}  // namespace Meta

#include "engine/meta/impl/functiontraits.hpp"
#include "engine/meta/impl/typeid.hpp"

namespace Meta {
namespace detail {

// Bind to value.
template <class C, typename PropTraits>
class ValueBinder {
public:
    typedef C ClassType;
    typedef typename std::conditional<PropTraits::isWritable, typename PropTraits::AccessType&, typename PropTraits::AccessType>::type AccessType;
    typedef typename std::remove_reference<AccessType>::type SetType;

    using Binding = typename PropTraits::template Binding<ClassType, AccessType>;

    static_assert(!std::is_pointer<AccessType>::value, "Error: Pointers not handled here");

    ValueBinder(const Binding& b) : m_bound(b) {}

    AccessType getter(ClassType& c) const { return m_bound.access(c); }

    Value getValue(ClassType& c) const {
        if constexpr (PropTraits::isWritable)
            return UserObject::makeRef(getter(c));
        else
            return UserObject::makeCopy(getter(c));
    }

    bool setter(ClassType& c, SetType v) const {
        if constexpr (PropTraits::isWritable)
            return this->m_bound.access(c) = v, true;
        else
            return false;
    }

    bool setter(ClassType& c, Value const& value) const { return setter(c, value.to<SetType>()); }

protected:
    Binding m_bound;
};

template <class C, typename PropTraits>
class ValueBinder2 : public ValueBinder<C, PropTraits> {
    typedef ValueBinder<C, PropTraits> Base;

public:
    template <typename S>
    ValueBinder2(const typename Base::Binding& g, S s) : Base(g), m_set(s) {}

    bool setter(typename Base::ClassType& c, typename Base::SetType v) const { return m_set(c, v), true; }

    bool setter(typename Base::ClassType& c, Value const& value) const { return setter(c, value.to<typename Base::SetType>()); }

protected:
    std::function<void(typename Base::ClassType&, typename Base::AccessType)> m_set;
};

// Bind to internal reference getter.
template <class C, typename PropTraits>
class InternalRefBinder {
public:
    typedef C ClassType;
    typedef typename PropTraits::ExposedType AccessType;

    using Binding = typename PropTraits::template Binding<ClassType, AccessType>;

    static_assert(std::is_pointer<AccessType>::value, "Error: Only pointers handled here");

    InternalRefBinder(const Binding& b) : m_bound(b) {}

    AccessType getter(ClassType& c) const {
        if constexpr (std::is_const<AccessType>::value)
            return m_bound.access(c);
        else
            return m_bound.access(const_cast<typename std::remove_const<ClassType>::type&>(c));
    }

    Value getValue(ClassType& c) const { return UserObject::makeRef(getter(c)); }

    bool setter(ClassType&, AccessType) const { return false; }
    bool setter(ClassType&, Value const&) const { return false; }

protected:
    Binding m_bound;
};

// Internal reference getter & setter.
template <class C, typename PropTraits>
class InternalRefBinder2 : public InternalRefBinder<C, PropTraits> {
    typedef InternalRefBinder<C, PropTraits> Base;

public:
    template <typename S>
    InternalRefBinder2(const typename Base::Binding& g, S s) : Base(g), m_set(s) {}

    bool setter(typename Base::ClassType& c, typename Base::AccessType v) const { return m_set(c, v), true; }
    bool setter(typename Base::ClassType& c, Value const& value) const { return setter(c, value.to<typename Base::AccessType>()); }

protected:
    std::function<void(typename Base::ClassType&, typename Base::AccessType)> m_set;
};

/*
 *  Access traits for an exposed type T.
 *    - I.e. how we use an instance to access the bound property data using the correct interface.
 *  Traits:
 *    - ValueBinder & RefBinder : RO (const) or RW data.
 *    - Impl : which specialise property impl to use.
 */
template <typename PT, typename E = void>
struct AccessTraits {
    static constexpr PropertyAccessKind kind = PropertyAccessKind::Simple;

    template <class C>
    using ValueBinder = ValueBinder<C, PT>;

    template <class C>
    using ValueBinder2 = ValueBinder2<C, PT>;

    template <typename A>
    using Impl = SimplePropertyImpl<A>;
};

/*
 * Enums.
 */
template <typename PT>
struct AccessTraits<PT, typename std::enable_if<std::is_enum<typename PT::ExposedTraits::DereferencedType>::value>::type> {
    static constexpr PropertyAccessKind kind = PropertyAccessKind::Enum;

    template <class C>
    using ValueBinder = ValueBinder<C, PT>;

    template <class C>
    using ValueBinder2 = ValueBinder2<C, PT>;

    template <typename A>
    using Impl = EnumPropertyImpl<A>;
};

/*
 * Array types.
 */
template <typename PT>
struct AccessTraits<PT, typename std::enable_if<MetaExt::ArrayMapper<typename PT::ExposedTraits::DereferencedType>::isArray>::type> {
    static constexpr PropertyAccessKind kind = PropertyAccessKind::Container;

    typedef MetaExt::ArrayMapper<typename PT::ExposedTraits::DereferencedType> ArrayTraits;

    template <class C>
    class ValueBinder : public ArrayTraits {
    public:
        typedef typename PT::ExposedTraits::DereferencedType ArrayType;
        typedef C ClassType;
        typedef typename PT::AccessType& AccessType;

        using Binding = typename PT::template Binding<ClassType, AccessType>;

        ValueBinder(const Binding& a) : m_bound(a) {}

        AccessType getter(ClassType& c) const { return m_bound.access(c); }

        bool setter(ClassType& c, AccessType v) const { return this->m_bound.access(c) = v, true; }

    protected:
        Binding m_bound;
    };

    template <typename A>
    using Impl = ArrayPropertyImpl<A>;
};

/*
 * User objects.
 *  - I.e. Registered classes.
 *  - Enums also use registration so must differentiate.
 */
template <typename PT>
struct AccessTraits<PT, typename std::enable_if<hasStaticTypeDecl<typename PT::ExposedTraits::DereferencedType>() && !std::is_enum<typename PT::ExposedTraits::DereferencedType>::value>::type> {
    static constexpr PropertyAccessKind kind = PropertyAccessKind::User;

    template <class C>
    using ValueBinder = typename std::conditional<std::is_pointer<typename PT::ExposedType>::value, InternalRefBinder<C, PT>, ValueBinder<C, PT>>::type;

    template <class C>
    using ValueBinder2 = typename std::conditional<std::is_pointer<typename PT::ExposedType>::value, InternalRefBinder2<C, PT>, ValueBinder2<C, PT>>::type;

    template <typename A>
    using Impl = UserPropertyImpl<A>;
};

// Read-only accessor wrapper. Not RW, not a pointer.
template <class C, typename TRAITS>
class GetSet1 {
public:
    typedef TRAITS PropTraits;
    typedef C ClassType;
    typedef typename PropTraits::ExposedType ExposedType;
    typedef typename PropTraits::ExposedTraits TypeTraits;
    typedef typename PropTraits::DataType DataType;  // raw type or container
    static constexpr bool canRead = true;
    static constexpr bool canWrite = PropTraits::isWritable;

    typedef AccessTraits<PropTraits> Access;

    typedef typename Access::template ValueBinder<ClassType> InterfaceType;

    InterfaceType m_interface;

    GetSet1(typename PropTraits::BoundType getter) : m_interface(typename InterfaceType::Binding(getter)) {}
};

/*
 * Property accessor composed of 1 getter and 1 setter
 */
template <typename C, typename FUNCTRAITS>
class GetSet2 {
public:
    typedef FUNCTRAITS PropTraits;
    typedef C ClassType;
    typedef typename PropTraits::ExposedType ExposedType;
    typedef typename PropTraits::ExposedTraits TypeTraits;
    typedef typename PropTraits::DataType DataType;  // raw type
    static constexpr bool canRead = true;
    static constexpr bool canWrite = true;

    typedef AccessTraits<PropTraits> Access;

    typedef typename Access::template ValueBinder2<ClassType> InterfaceType;

    InterfaceType m_interface;

    template <typename F1, typename F2>
    GetSet2(F1 getter, F2 setter) : m_interface(typename InterfaceType::Binding(getter), setter) {}
};

/*
 * Property factory which instantiates the proper type of property from 1 accessor.
 */
template <typename C, typename T, typename E = void>
struct PropertyFactory1 {
    static constexpr PropertyKind kind = PropertyKind::Function;

    static Property* create(IdRef name, T accessor) {
        typedef GetSet1<C, FunctionTraits<T>> Accessor;  // read-only?

        typedef typename Accessor::Access::template Impl<Accessor> PropertyImpl;

        return new PropertyImpl(name, Accessor(accessor));
    }
};

template <typename C, typename T>
struct PropertyFactory1<C, T, typename std::enable_if<std::is_member_object_pointer<T>::value>::type> {
    static constexpr PropertyKind kind = PropertyKind::MemberObject;

    static Property* create(IdRef name, T accessor) {
        typedef GetSet1<C, MemberTraits<T>> Accessor;  // read-only?

        typedef typename Accessor::Access::template Impl<Accessor> PropertyImpl;

        return new PropertyImpl(name, Accessor(accessor));
    }
};

/*
 * Expose property with a getter and setter function.
 * Type of property is the return type of the getter.
 */
template <typename C, typename F1, typename F2, typename E = void>
struct PropertyFactory2 {
    static Property* create(IdRef name, F1 accessor1, F2 accessor2) {
        typedef GetSet2<C, FunctionTraits<F1>> Accessor;  // read-write wrapper

        typedef typename Accessor::Access::template Impl<Accessor> PropertyImpl;

        return new PropertyImpl(name, Accessor(accessor1, accessor2));
    }
};

}  // namespace detail
}  // namespace Meta

#endif  // META_DETAIL_PROPERTYFACTORY_HPP
