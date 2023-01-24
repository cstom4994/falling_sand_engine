

#ifndef META_DETAIL_VALUEPROVIDER_HPP
#define META_DETAIL_VALUEPROVIDER_HPP

#include "meta/args.hpp"
#include "meta/class.hpp"
#include "meta/classget.hpp"
#include "meta/valuemapper.hpp"

namespace Meta {
namespace detail {

/*
 * Implementation of ValueProvider
 * Generic version, use default constructor
 */
template <typename T, ValueKind Type>
struct ValueProviderImpl {
    T operator()() { return T(); }
};

/*
 * Specialization for user types: use metaclass to instantiate
 * so that we get an exception rather than a compile error
 * if the type has no default constructor
 */
template <typename T>
struct ValueProviderImpl<T, ValueKind::User> {
    ValueProviderImpl()
        : m_value(0)  // classByType<T>().construct(Args::empty).template get<T*>()) // XXXX
    {}
    ~ValueProviderImpl() {}  // {classByType<T>().destroy(m_value);}
    T& operator()() { return *m_value; }
    T* m_value;
};

/*
 * Specialization for pointer to primitive types: use new to allocate objects
 * Here we assume that the caller will take ownership of the returned value
 */
template <typename T, ValueKind Type>
struct ValueProviderImpl<T*, Type> {
    T* operator()() { return new T; }
};

/*
 * Specialization for pointer to user types: use metaclass to allocate objects
 * Here we assume that the caller will take ownership of the returned value
 */
template <typename T>
struct ValueProviderImpl<T*, ValueKind::User> {
    T* operator()() { return classByType<T>().construct().template get<T*>(); }
};

/*
 * Helper structure to instantiate new values based on their type
 */
template <typename T>
struct ValueProvider : ValueProviderImpl<T, MetaExt::ValueMapper<T>::kind> {};

}  // namespace detail
}  // namespace Meta

#endif  // META_DETAIL_VALUEPROVIDER_HPP
