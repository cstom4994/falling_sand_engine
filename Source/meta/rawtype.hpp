

#ifndef META_DETAIL_RAWTYPE_HPP
#define META_DETAIL_RAWTYPE_HPP

#include <memory>

namespace Meta {
namespace detail {

/**
 * \brief Utility class which tells at compile-time if a type T is a smart pointer to a type U
 *
 * To detect a smart pointer type, we check using SFINAE if T implements an operator -> returning a U*
 */
template <typename T, typename U>
struct IsSmartPointer {
    static constexpr bool value = false;
};

template <typename T, typename U>
struct IsSmartPointer<std::unique_ptr<T>, U> {
    static constexpr bool value = true;
};

template <typename T, typename U>
struct IsSmartPointer<std::shared_ptr<T>, U> {
    static constexpr bool value = true;
};

/**
 * \class DataType
 *
 * \brief Helper structure used to extract the raw type of a composed type
 *
 * DataType<T> recursively removes const, reference and pointer modifiers from the given type.
 * In other words:
 *
 * \li DataType<T>::Type == T
 * \li DataType<const T>::Type == DataType<T>::Type
 * \li DataType<T&>::Type == DataType<T>::Type
 * \li DataType<const T&>::Type == DataType<T>::Type
 * \li DataType<T*>::Type == DataType<T>::Type
 * \li DataType<const T*>::Type == DataType<T>::Type
 *
 * \remark DataType is able to detect smart pointers and properly extract the stored type
 */

// Generic version -- T doesn't match with any of our specialization, and is thus considered a raw data type
//  - int -> int, int[] -> int, int* -> int.
template <typename T, typename E = void>
struct DataType {
    typedef T Type;
};

// const
template <typename T>
struct DataType<const T> : public DataType<T> {};

template <typename T>
struct DataType<T&> : public DataType<T> {};

template <typename T>
struct DataType<T*> : public DataType<T> {};

template <typename T, size_t N>
struct DataType<T[N]> : public DataType<T> {};

// smart pointer
template <template <typename> class T, typename U>
struct DataType<T<U>, typename std::enable_if<IsSmartPointer<T<U>, U>::value>::type> {
    typedef typename DataType<U>::Type Type;
};

}  // namespace detail

template <class T>
T* get_pointer(T* p) {
    return p;
}

template <class T>
T* get_pointer(std::unique_ptr<T> const& p) {
    return p.get();
}

template <class T>
T* get_pointer(std::shared_ptr<T> const& p) {
    return p.get();
}

}  // namespace Meta

#endif  // META_DETAIL_RAWTYPE_HPP
