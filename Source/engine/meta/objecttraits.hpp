

#ifndef META_DETAIL_OBJECTTRAITS_HPP
#define META_DETAIL_OBJECTTRAITS_HPP

#include <list>
#include <vector>

#include "engine/meta/type.hpp"
#include "rawtype.hpp"

namespace Meta {
namespace detail {

/*
 * - ReferenceType: the reference type closest to T which allows to have direct access
 *   to the object (T& for raw types and references, T* for pointer types)
 *   Note: not T& reference type!
 * - PointerType: the pointer type closest to T which allows to have direct access to
 *   the object (T*)
 * - DataType: the actual raw type of the object (removes all indirections, as well const
 *   and reference modifiers)
 *
 * - isWritable: true if the type allows to modify the object (non-const references and pointers)
 * - isRef: true if the type is a reference type (references and pointers)
 *
 * - get(void*): get a direct access to an object given by a typeless pointer (in other
 *   words, converts from void* to ReferenceType)
 * - getPointer(T): get a direct pointer to an object, regardless its original
 *   storage / modifiers (in other words, convert from T to PointerType)
 */

template <typename T, typename E = void>
struct TypeTraits {
    static constexpr ReferenceKind kind = ReferenceKind::Instance;
    typedef T Type;
    typedef T& ReferenceType;
    typedef T* PointerType;
    typedef T DereferencedType;
    static_assert(!std::is_void<T>::value, "Incorrect type details");
    typedef typename DataType<T>::Type DataType;
    static constexpr bool isWritable = !std::is_const<DereferencedType>::value;
    static constexpr bool isRef = false;

    static inline ReferenceType get(void* pointer) { return *static_cast<T*>(pointer); }
    static inline PointerType getPointer(T& value) { return &value; }
    static inline PointerType getPointer(T* value) { return value; }
};

// void
template <>
struct TypeTraits<void> {
    static constexpr ReferenceKind kind = ReferenceKind::None;
    typedef void T;
    typedef T* ReferenceType;
    typedef T* PointerType;
    typedef T DereferencedType;
    typedef typename DataType<T>::Type DataType;
    static constexpr bool isWritable = false;
    static constexpr bool isRef = false;

    static inline ReferenceType get(void* pointer) { return 0; }
    static inline PointerType getPointer(T* value) { return value; }
};

// Raw pointers
template <typename T>
struct TypeTraits<T*> {
    static constexpr ReferenceKind kind = ReferenceKind::Pointer;
    typedef T* Type;
    typedef T* ReferenceType;
    typedef T* PointerType;
    typedef T DereferencedType;
    typedef typename DataType<T>::Type DataType;
    static constexpr bool isWritable = !std::is_const<DereferencedType>::value;
    static constexpr bool isRef = true;

    static inline ReferenceType get(void* pointer) { return static_cast<T*>(pointer); }
    static inline PointerType getPointer(T& value) { return &value; }
    static inline PointerType getPointer(T* value) { return value; }
};

// References
template <typename T>
struct TypeTraits<T&> {
    static constexpr ReferenceKind kind = ReferenceKind::Reference;
    typedef T& Type;
    typedef T& ReferenceType;
    typedef T* PointerType;
    typedef T DereferencedType;
    typedef typename DataType<T>::Type DataType;
    static constexpr bool isWritable = !std::is_const<DereferencedType>::value;
    static constexpr bool isRef = true;

    static inline ReferenceType get(void* pointer) { return *static_cast<T*>(pointer); }
    static inline PointerType getPointer(T& value) { return &value; }
    static inline PointerType getPointer(T* value) { return value; }
};

// Base class for smart pointers
template <class P, typename T>
struct SmartPointerReferenceTraits {
    typedef P Type;
    static constexpr ReferenceKind kind = ReferenceKind::SmartPointer;
    typedef T& ReferenceType;
    typedef P PointerType;
    typedef T DereferencedType;
    typedef typename DataType<T>::Type DataType;
    static constexpr bool isWritable = !std::is_const<DereferencedType>::value;
    static constexpr bool isRef = true;

    static inline ReferenceType get(void* pointer) { return *static_cast<P*>(pointer); }
    static inline PointerType getPointer(P& value) { return get_pointer(value); }
};

// std::shared_ptr<>
template <typename T>
struct TypeTraits<std::shared_ptr<T>> : public SmartPointerReferenceTraits<std::shared_ptr<T>, T> {};

// Built-in arrays []
template <typename T, size_t N>
struct TypeTraits<T[N], typename std::enable_if<std::is_array<T>::value>::type> {
    static constexpr ReferenceKind kind = ReferenceKind::BuiltinArray;
    typedef T Type[N];
    typedef typename DataType<T>::Type DataType;
    typedef T (&ReferenceType)[N];
    typedef T* PointerType;
    typedef T DereferencedType[N];
    static constexpr size_t Size = N;
    static constexpr bool isWritable = !std::is_const<T>::value;
    static constexpr bool isRef = false;
};

// template <typename C, typename T, size_t S>
// struct MemberTraits<std::array<T,S>(C::*)>
//{
//     typedef std::array<T,S>(C::*Type);
//     typedef C ClassType;
//     typedef std::array<T,S> ExposedType;
//     typedef typename DataType<T>::Type DataType;
//     //static constexpr bool isWritable = !std::is_const<DataType>::value;
//
//     class Access
//     {
//     public:
//         Access(Type d) : data(d) {}
//         ExposedType& getter(ClassType& c) const { return c.*data; }
//     private:
//         Type data;
//     };
// };
//
// template <typename C, typename T>
// struct MemberTraits<std::vector<T>(C::*)>
//{
//     typedef std::vector<T>(C::*Type);
//     typedef C ClassType;
//     typedef std::vector<T> ExposedType;
//     typedef typename DataType<T>::Type DataType;
//
//     class Access
//     {
//     public:
//         Access(Type d) : data(d) {}
//         ExposedType& getter(ClassType& c) const { return c.*data; }
//     private:
//         Type data;
//     };
// };
//
// template <typename C, typename T>
// struct MemberTraits<std::list<T>(C::*)>
//{
//     typedef std::list<T>(C::*Type);
//     typedef C ClassType;
//     typedef std::list<T> ExposedType;
//     typedef typename DataType<T>::Type DataType;
//
//     class Access
//     {
//     public:
//         Access(Type d) : data(d) {}
//         ExposedType& getter(ClassType& c) const { return c.*data; }
//     private:
//         Type data;
//     };
// };

}  // namespace detail
}  // namespace Meta

#endif  // META_DETAIL_OBJECTTRAITS_HPP
