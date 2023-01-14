

#pragma once
#ifndef META_DETAIL_ISSMARTPOINTER_HPP
#define META_DETAIL_ISSMARTPOINTER_HPP

#include "engine/meta/config.hpp"
#include <type_traits>
#include <memory>


namespace Meta {

template<class T>
T* get_pointer(T *p)
{
    return p;
}
    
template<class T>
T* get_pointer(std::unique_ptr<T> const& p)
{
    return p.get();
}

template<class T>
T* get_pointer(std::shared_ptr<T> const& p)
{
    return p.get();
}

namespace detail {
    
/**
 * \brief Utility class which tells at compile-time if a type T is a smart pointer to a type U
 */
template <typename T, typename U>
struct IsSmartPointer
{    
    enum {value = false};
};

template <typename T, typename U>
struct IsSmartPointer<std::unique_ptr<T>, U>
{
    enum {value = true};
};

template <typename T, typename U>
struct IsSmartPointer<std::shared_ptr<T>, U>
{
    enum {value = true};
};

    
} // namespace detail
} // namespace Meta

#endif // META_DETAIL_ISSMARTPOINTER_HPP
