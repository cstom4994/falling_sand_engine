

#pragma once
#ifndef META_USEROBJECT_HPP
#define META_USEROBJECT_HPP

#include "engine/meta/args.hpp"
#include "engine/meta/classcast.hpp"
#include "engine/meta/objectholder.hpp"
#include "engine/meta/objecttraits.hpp"
#include "engine/meta/util.hpp"
#include "engine/meta/errors.hpp"

namespace Meta {

class Property;
class UserProperty;
class Value;
class Args;
class ParentObject;

/**
 * \brief Wrapper to manipulate user objects in the Meta system
 *
 * Meta::UserObject is an abstract representation of object instances, which can safely
 * be passed to and manipulated by all the entities in Meta.
 *
 * \note UserObjects are stored interally as objects (a copy) or references (an existing
 *       object). To be sure which you are constructing use UserObject::makeRef() or
 *       UserObject::makeCopy().
 *
 * \sa EnumObject
 */
class UserObject {
public:
    /**
     * \brief Construct a user object from a reference to an object
     *
     * This functions is equivalent to calling `UserObject(&object)`.
     *
     * \param object Instance to store in the user object
     * \return UserObject containing a reference to \a object
     */
    template <typename T>
    static UserObject makeRef(T& object);

    /**
     * \brief Construct a user object from a const reference to an object
     *
     * This functions is equivalent to calling `UserObject(&object)`.
     *
     * \param object Instance to store in the user object
     * \return UserObject containing a const reference to \a object
     */
    template <typename T>
    static UserObject makeRef(T* object);

    /**
     * \brief Construct a user object with a copy of an object
     *
     * This functions is *not* equivalent to calling UserObject(object).
     *
     * \param object Instance to store in the user object
     *
     * \return UserObject containing a copy of \a object
     */
    template <typename T>
    static UserObject makeCopy(const T& object);

    template <typename T>
    static UserObject makeOwned(T&& object);

    /**
     * \brief Default constructor
     *
     * Constructs an empty/invalid object
     */
    UserObject();

    /**
     * \brief Copy constructor
     *
     * \param other instance to copy
     */
    UserObject(const UserObject& other);

    /**
     * \brief Move constructor
     *
     * \param other instance to move
     */
    UserObject(UserObject&& other) noexcept;

    /**
     * \brief Construct the user object from an instance copy
     *
     * \param object Instance to store in the user object
     *
     * \sa makeRef(), makeCopy()
     */
    template <typename T>
    UserObject(const T& object);

    /**
     * \brief Construct the user object from an instance reference
     *
     * \param object Pointer to the object to reference in the user object
     *
     * \sa makeRef(), makeCopy()
     */
    template <typename T>
    UserObject(T* object);

    /**
     * \brief Copy assignment operator
     *
     * \param other User object to assign
     *
     * \return Reference to this
     */
    UserObject& operator=(const UserObject& other);

    /**
     * \brief Move assignment operator
     *
     * \param other User object to assign
     *
     * \return Reference to this
     */
    UserObject& operator=(UserObject&& other) noexcept;

    /**
     * \brief Retrieve the instance stored in the user object
     *
     * The template parameter T is the type to convert the instance to.
     * T must be compatible with the original type of the instance.
     *
     * \return Reference to the instance of the stored object
     *
     * \throw NullObject the stored object is invalid
     * \throw ClassNotFound T has not metaclass
     * \throw ClassUnrelated the type of the object is not compatible with T
     */
    template <typename T>
    typename detail::TypeTraits<T>::ReferenceType get() const;

    /**
     * \brief Retrieve the address of the stored object
     *
     * This function must be used with caution, as the returned address
     * may not be what you expect it to be!
     *
     * \return Pointer to the stored object
     */
    void* pointer() const;

    /**
     * \brief Get a const reference to the object data contained
     *
     * Returns a const reference to the contained object, of type T. The user is responsible for
     * ensuring that the type passed is correct. See ref() for non-const ref.
     *
     * \return A const reference to the contained object.
     */
    template <typename T>
    const T& cref() const;

    /**
     * \brief Get a non-const reference to the object data contained
     *
     * Returns a reference to the contained object, of type T. The user is responsible for
     * ensuring that the type passed is correct. See cref() for const ref.
     *
     * \return A reference to the contained object.
     */
    template <typename T>
    T& ref() const;

    /**
     * \brief Retrieve the metaclass of the stored instance
     *
     * \return Reference to the instance's metaclass
     *
     * \throw NullObject the stored object has no metaclass
     */
    const Class& getClass() const;

    /**
     * \brief Get the value of an object's property by name
     *
     * This function is defined for convenience, it is a shortcut
     * for `object.getClass().property(name).get(object);`
     *
     * \param property Name of the property to get
     *
     * \return Current value of the property
     *
     * \throw PropertyNotFound \a property is not a property of the object
     * \throw ForbiddenRead \a property is not readable
     */
    Value get(IdRef property) const;

    /**
     * \brief Get the value of an object's property by index
     *
     * This function is defined for convenience, it is a shortcut
     * for `object.getClass().property(index).get(object);`
     *
     * \param index Index of the property to get
     *
     * \return Current value of the property
     *
     * \throw OutOfRange index is invalid
     * \throw ForbiddenRead \a property is not readable
     */
    Value get(size_t index) const;

    /**
     * \brief Set the value of an object's property by name
     *
     * This function is defined for convenience, it is a shortcut
     * for `object.getClass().property(name).set(object, value);`
     *
     * \param property Name of the property to set
     * \param value Value to set
     *
     * \throw PropertyNotFound \a property is not a property of the object
     * \throw ForbiddenWrite \a property is not writable
     * \throw BadType \a value can't be converted to the property's type
     */
    void set(IdRef property, const Value& value) const;

    /**
     * \brief Set the value of an object's property by index
     *
     * This function is defined for convenience, it is a shortcut
     * for `object.getClass().property(index).set(object, value);`
     *
     * \param index Index of the property to set
     * \param value Value to set
     *
     * \throw OutOfRange index is invalid
     * \throw ForbiddenWrite \a property is not writable
     * \throw BadType \a value can't be converted to the property's type
     */
    void set(size_t index, const Value& value) const;

    /**
     * \brief Operator == to compare equality between two user objects
     *
     * Two user objects are equal if their metaclasses and pointers are both equal,
     * i.e. they point to the same object, not if the object *values* are the same.
     *
     * \param other User object to compare with this
     *
     * \return True if both user objects are the same, false otherwise
     */
    bool operator==(const UserObject& other) const;

    /**
     * \brief Operator != to compare inequality between two user objects
     *
     * \see operator ==
     */
    bool operator!=(const UserObject& other) const { return !(*this == other); }

    /**
     * \brief Operator < to compare two user objects
     *
     * \param other User object to compare with this
     *
     * \return True if this < other
     */
    bool operator<(const UserObject& other) const;

    /**
     * \brief Special UserObject instance representing an empty object
     */
    static const UserObject nothing;

private:
    friend class Property;

    // Assign a new value to a property of the object
    void set(const Property& property, const Value& value) const;

    UserObject(const Class* cls, detail::AbstractObjectHolder* h) : m_class(cls), m_holder(h) {}

    // Metaclass of the stored object
    const Class* m_class;

    // Optional abstract holder storing the object
    std::shared_ptr<detail::AbstractObjectHolder> m_holder;
};

}  // namespace Meta

namespace Meta {

template <typename T>
UserObject::UserObject(const T& object) : m_class(&classByType<T>()) {
    typedef detail::TypeTraits<const T> PropTraits;
    typedef detail::ObjectHolderByCopy<typename PropTraits::DataType> Holder;
    m_holder.reset(new Holder(PropTraits::getPointer(object)));
}

template <typename T>
UserObject::UserObject(T* object) : m_class(&classByType<T>()) {
    typedef detail::TypeTraits<T> PropTraits;
    static_assert(!PropTraits::isRef, "Cannot make reference to reference");

    typedef typename std::conditional<std::is_const<T>::value, detail::ObjectHolderByConstRef<typename PropTraits::DataType>, detail::ObjectHolderByRef<typename PropTraits::DataType>>::type Holder;
    m_holder.reset(new Holder(object));
}

template <typename T>
typename detail::TypeTraits<T>::ReferenceType UserObject::get() const {
    // Make sure that we have a valid internal object
    void* ptr = pointer();
    if (!ptr) META_ERROR(NullObject(m_class));

    // Get the metaclass of T (we use classByTypeSafe because it may not exist)
    const Class* targetClass = classByTypeSafe<T>();
    if (!targetClass) META_ERROR(ClassNotFound("unknown"));

    // Apply the proper offset to the pointer (solves multiple inheritance issues)
    ptr = classCast(ptr, *m_class, *targetClass);

    return detail::TypeTraits<T>::get(ptr);
}

template <typename T>
inline UserObject UserObject::makeRef(T& object) {
    typedef detail::TypeTraits<T> TypeTraits;
    static_assert(!TypeTraits::isRef, "Cannot make reference to reference");

    typedef typename std::conditional<std::is_const<T>::value, detail::ObjectHolderByConstRef<typename TypeTraits::DataType>, detail::ObjectHolderByRef<typename TypeTraits::DataType>>::type Holder;

    return UserObject(&classByObject(object), new Holder(TypeTraits::getPointer(object)));
}

template <typename T>
inline UserObject UserObject::makeRef(T* object) {
    return makeRef(*object);
}

template <typename T>
inline UserObject UserObject::makeCopy(const T& object) {
    typedef detail::TypeTraits<const T> PropTraits;
    typedef detail::ObjectHolderByCopy<typename PropTraits::DataType> Holder;
    return UserObject(&classByType<T>(), new Holder(PropTraits::getPointer(object)));
}

template <typename T>
inline UserObject UserObject::makeOwned(T&& object) {
    typedef detail::TypeTraits<const T> PropTraits;
    typedef detail::ObjectHolderByCopy<typename PropTraits::DataType> Holder;
    return UserObject(&classByType<T>(), new Holder(std::forward<T>(object)));
}

template <typename T>
inline T& UserObject::ref() const {
    return *reinterpret_cast<T*>(m_holder->object());
}

template <typename T>
inline const T& UserObject::cref() const {
    return *reinterpret_cast<T*>(m_holder->object());
}

}  // namespace Meta

#endif  // META_USEROBJECT_HPP
