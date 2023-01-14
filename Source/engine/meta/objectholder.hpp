

#ifndef META_DETAIL_OBJECTHOLDER_HPP
#define META_DETAIL_OBJECTHOLDER_HPP

#include "engine/meta/classcast.hpp"
#include "engine/meta/classget.hpp"

namespace Meta {
namespace detail {

/**
 * \brief Abstract base class for object holders
 *
 * This class is meant to be used by UserObject.
 * @todo Use an optimized memory pool if there are too many allocations of holders
 */
class AbstractObjectHolder {
public:
    /**
     * \brief Destructor
     */
    virtual ~AbstractObjectHolder();

    /**
     * \brief Return a typeless pointer to the stored object
     *
     * \return Pointer to the object
     */
    virtual void* object() = 0;

    /**
     * \brief Return a holder which is able to modify its stored object
     *
     * The holder can return itself it it already meets the requirement,
     * otherwise it may return a new holder storing a copy of its object.
     *
     * \return Holder storing a writable object
     */
    virtual AbstractObjectHolder* getWritable() = 0;

protected:
    AbstractObjectHolder();
};

/**
 * \brief Typed specialization of AbstractObjectHolder for storage by const reference
 */
template <typename T>
class ObjectHolderByConstRef final : public AbstractObjectHolder {
public:
    /**
     * \brief Construct the holder from a const object
     *
     * \param object Pointer to the object to store
     */
    ObjectHolderByConstRef(const T* object);

    /**
     * \brief Return a typeless pointer to the stored object
     *
     * \return Pointer to the object
     */
    void* object() final;

    /**
     * \brief Return a holder which is able to modify its stored object
     *
     * The holder can return itself it it already meets the requirement,
     * otherwise it may return a new holder storing a copy of its object.
     *
     * \return Holder storing a writable object
     */
    AbstractObjectHolder* getWritable() final;

private:
    const T* m_object;  // Pointer to the object
    // Pointer to the actual derived part of the object (may be different than
    // m_object in case of multiple inheritance with offset).
    void* m_alignedPtr;
};

/**
 * \brief Typed specialization of AbstractObjectHolder for storage by reference
 */
template <typename T>
class ObjectHolderByRef final : public AbstractObjectHolder {
public:
    /**
     * \brief Construct the holder from an object
     *
     * \param object Pointer to the object to store
     */
    ObjectHolderByRef(T* object);

    /**
     * \brief Return a typeless pointer to the stored object
     *
     * \return Pointer to the object
     */
    void* object() final;

    /**
     * \brief Return a holder which is able to modify its stored object
     *
     * The holder can return itself it it already meets the requirement,
     * otherwise it may return a new holder storing a copy of its object.
     *
     * \return Holder storing a writable object
     */
    AbstractObjectHolder* getWritable() final;

private:
    T* m_object;  // Pointer to the object
    // Pointer to the actual derived part of the object (may be different than m_object
    // in case of multiple inheritance with offset)
    void* m_alignedPtr;
};

/**
 * \brief Typed specialization of AbstractObjectHolder for storage by copy
 */
template <typename T>
class ObjectHolderByCopy final : public AbstractObjectHolder {
public:
    /**
     * \brief Construct the holder from an object
     * \param object Object to store
     */
    ObjectHolderByCopy(const T* object);

    ObjectHolderByCopy(T&& object);

    ~ObjectHolderByCopy() {}

    /**
     * \brief Return a typeless pointer to the stored object
     * \return Pointer to the object
     */
    void* object() final;

    /**
     * \brief Return a holder which is able to modify its stored object
     *
     * The holder can return itself it it already meets the requirement,
     * otherwise it may return a new holder storing a copy of its object.
     *
     * \return Holder storing a writable object
     */
    AbstractObjectHolder* getWritable() final;

private:
    T m_object;  // Copy of the object
};

}  // namespace detail
}  // namespace Meta

namespace Meta {
namespace detail {

inline AbstractObjectHolder::~AbstractObjectHolder() {}

inline AbstractObjectHolder::AbstractObjectHolder() {}

template <typename T>
ObjectHolderByConstRef<T>::ObjectHolderByConstRef(const T* object) : m_object(object), m_alignedPtr(classCast(const_cast<T*>(object), classByType<T>(), classByObject(object))) {}

template <typename T>
void* ObjectHolderByConstRef<T>::object() {
    return m_alignedPtr;
}

template <typename T>
AbstractObjectHolder* ObjectHolderByConstRef<T>::getWritable() {
    // TODO: Dangerous?
    // We hold a read-only object: return a holder which stores a copy of it
    return nullptr;  // new ObjectHolderByCopy<T>(m_object); XXXX const ref?!
}

template <typename T>
ObjectHolderByRef<T>::ObjectHolderByRef(T* object) : m_object(object), m_alignedPtr(classCast(object, classByType<T>(), classByObject(object))) {}

template <typename T>
void* ObjectHolderByRef<T>::object() {
    return m_alignedPtr;
}

template <typename T>
AbstractObjectHolder* ObjectHolderByRef<T>::getWritable() {
    // We already store a modifiable object
    return this;
}

template <typename T>
ObjectHolderByCopy<T>::ObjectHolderByCopy(const T* object) : m_object(*object) {}

template <typename T>
ObjectHolderByCopy<T>::ObjectHolderByCopy(T&& object) : m_object(std::forward<T>(object)) {}

template <typename T>
void* ObjectHolderByCopy<T>::object() {
    return reinterpret_cast<void*>(&m_object);
}

template <typename T>
AbstractObjectHolder* ObjectHolderByCopy<T>::getWritable() {
    // We already store a modifiable object
    return this;
}

}  // namespace detail
}  // namespace Meta

#endif  // META_DETAIL_OBJECTHOLDER_HPP
