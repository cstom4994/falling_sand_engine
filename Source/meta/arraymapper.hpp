

#ifndef META_ARRAYMAPPER_HPP
#define META_ARRAYMAPPER_HPP

#include <list>
#include <vector>

#include "meta/config.hpp"

namespace MetaExt {

/**
 * \class ArrayMapper
 *
 * \brief Template providing a mapping between C++ arrays and Meta ArrayProperty
 *
 * ArrayMapper<T> must define the following members in order to make T fully compliant
 * with the system:
 *
 *  - `ElementType`: type of the elements stored in the array
 *  - `dynamic()`: tells if the array is dynamic (i.e. supports insert and remove)
 *  - `size()`: retrieve the size of the array
 *  - `get()`: get the value of an element
 *  - `set()`: set the value of an element
 *  - `insert()`: insert a new element
 *  - `remove()`: remove an element
 *
 * ValueMapper is specialized for every supported type, and can be specialized
 * for any of your own array types in order to extend the system.
 *
 * By default, ValueMapper supports the following types of array:
 *
 *  - `T[]`
 *  - `std::vector` & `std::vector<bool>`
 *  - `std::list`
 *
 * Here is an example of mapping for the `std::vector` class:
 *
 * \snippet this doc_arraymapper
 */

/** \cond NoDocumentation */

/*
 * Default. Not an array type.
 */
template <typename T>
struct ArrayMapper {
    static constexpr bool isArray = false;
};

/*
 * Specialization of ArrayMapper for built-in static arrays
 */
template <typename T, size_t N>
struct ArrayMapper<T[N]> {
    static constexpr bool isArray = true;
    typedef T ElementType;

    static bool dynamic() { return false; }

    static size_t size(T (&)[N]) { return N; }

    static const T& get(T (&arr)[N], size_t index) { return arr[index]; }

    static void set(T (&arr)[N], size_t index, const T& value) { arr[index] = value; }

    static void insert(T (&)[N], size_t, const T&) {}

    static void remove(T (&)[N], size_t) {}
};

/*
 * Specialization of ArrayMapper for std::array
 */
template <typename T, size_t N>
struct ArrayMapper<std::array<T, N>> {
    static constexpr bool isArray = true;
    typedef T ElementType;

    static bool dynamic() { return false; }

    static size_t size(const std::array<T, N>&) { return N; }

    static const T& get(const std::array<T, N>& arr, size_t index) { return arr[index]; }

    static void set(std::array<T, N>& arr, size_t index, const T& value) { arr[index] = value; }

    static void insert(std::array<T, N>&, size_t, const T&) {}

    static void remove(std::array<T, N>&, size_t) {}
};

/*
 * Specialization of ArrayMapper for boost::array
 */
// template <typename T, size_t N>
// struct ArrayMapper<boost::array<T, N> >
//{
//     static constexpr bool isArray = true;
//     typedef T ElementType;
//
//     static bool dynamic()
//     {
//         return false;
//     }
//
//     static size_t size(const boost::array<T, N>&)
//     {
//         return N;
//     }
//
//     static const T& get(const boost::array<T, N>& arr, size_t index)
//     {
//         return arr[index];
//     }
//
//     static void set(boost::array<T, N>& arr, size_t index, const T& value)
//     {
//         arr[index] = value;
//     }
//
//     static void insert(boost::array<T, N>&, size_t, const T&)
//     {
//     }
//
//     static void remove(boost::array<T, N>&, size_t)
//     {
//     }
// };

/*
 * Specialization of ArrayMapper for std::vector
 */
//! [doc_arraymapper]
template <typename T>
struct ArrayMapper<std::vector<T>> {
    static constexpr bool isArray = true;
    typedef T ElementType;

    static bool dynamic() { return true; }

    static size_t size(const std::vector<T>& arr) { return arr.size(); }

    static const T& get(const std::vector<T>& arr, size_t index) { return arr[index]; }

    static void set(std::vector<T>& arr, size_t index, const T& value) { arr[index] = value; }

    static void insert(std::vector<T>& arr, size_t before, const T& value) { arr.insert(arr.begin() + before, value); }

    static void remove(std::vector<T>& arr, size_t index) { arr.erase(arr.begin() + index); }
};
//! [doc_arraymapper]

/*
 * Specialization of ArrayMapper for std::vector<bool>
 * - See https://github.com/billyquith/ponder/issues/72
 */
template <>
struct ArrayMapper<std::vector<bool>> {
    static constexpr bool isArray = true;
    typedef bool ElementType;

    static bool dynamic() { return true; }

    static size_t size(const std::vector<bool>& arr) { return arr.size(); }

    static bool get(const std::vector<bool>& arr, size_t index) { return arr[index]; }

    static void set(std::vector<bool>& arr, size_t index, const bool& value) { arr[index] = value; }

    static void insert(std::vector<bool>& arr, size_t before, const bool& value) { arr.insert(arr.begin() + before, value); }

    static void remove(std::vector<bool>& arr, size_t index) { arr.erase(arr.begin() + index); }
};

/*
 * Specialization of ArrayMapper for std::list
 */
template <typename T>
struct ArrayMapper<std::list<T>> {
    static constexpr bool isArray = true;
    typedef T ElementType;

    static bool dynamic() { return true; }

    static size_t size(const std::list<T>& arr) { return arr.size(); }

    static const T& get(const std::list<T>& arr, size_t index) {
        typename std::list<T>::const_iterator it = arr.begin();
        std::advance(it, index);
        return *it;
    }

    static void set(std::list<T>& arr, size_t index, const T& value) {
        typename std::list<T>::iterator it = arr.begin();
        std::advance(it, index);
        *it = value;
    }

    static void insert(std::list<T>& arr, size_t before, const T& value) {
        typename std::list<T>::iterator it = arr.begin();
        std::advance(it, before);
        arr.insert(it, value);
    }

    static void remove(std::list<T>& arr, size_t index) {
        typename std::list<T>::iterator it = arr.begin();
        std::advance(it, index);
        arr.erase(it);
    }
};

/** \endcond NoDocumentation */

}  // namespace MetaExt

#endif  // META_ARRAYMAPPER_HPP
