

#ifndef META_DETAIL_TYPEID_HPP
#define META_DETAIL_TYPEID_HPP

#include <typeindex>

#include "meta/objecttraits.hpp"
#include "meta/type.hpp"

namespace Meta {
namespace detail {

// Calculate TypeId for T
template <typename T>
inline TypeId calcTypeId() {
    return TypeId(typeid(T));
}

/**
 * \brief Utility class to get the Meta identifier associated to a C++ type
 *
 * A compiler error will be triggered if requesting the identifier of a type
 * which hasn't been registered with the META_TYPE macro.
 */
template <typename T>
struct StaticTypeDecl {
    static constexpr bool defined = false, copyable = true;
    typedef T type;

    static TypeId id(bool = true) {
        // If you get this error, it means you didn't register your class/enum T with
        // the META_TYPE macro
        return T::META_TYPE_NOT_REGISTERED();
    }

    static const char* name(bool = true) {
        // If you get this error, it means you didn't register your class/enum T with
        // the META_TYPE macro
        return T::META_TYPE_NOT_REGISTERED();
    }
};

// Test if T is declared
template <typename T>
constexpr bool hasStaticTypeDecl() {
    return StaticTypeDecl<typename DataType<T>::Type>::defined;
}

template <typename T>
static inline TypeId staticTypeId() {
    return StaticTypeDecl<typename DataType<T>::Type>::id();
}

template <typename T>
static inline TypeId staticTypeId(const T&) {
    return StaticTypeDecl<typename DataType<T>::Type>::id();
}

template <typename T>
static inline const char* staticTypeName() {
    return StaticTypeDecl<typename DataType<T>::Type>::name();
}

template <typename T>
static inline const char* staticTypeName(const T&) {
    return StaticTypeDecl<typename DataType<T>::Type>::name();
}

/* Utility class used to check at compile-time if a type T implements the Meta RTTI
 */
template <typename T>
struct HasPonderRtti {
    template <typename U, TypeId (U::*)() const>
    struct TestForMember {};
    template <typename U>
    static std::true_type check(TestForMember<U, &U::ponderClassId>*);
    template <typename U>
    static std::false_type check(...);

    static constexpr bool value = std::is_same<decltype(check<T>(0)), std::true_type>::value;
};

/**
 * \brief Utility class to get the Meta identifier associated to a C++ object
 *
 * If the object has a dynamic type which is different from its static type
 * (i.e. `Base* obj = new Derived`), and both classes use the
 * META_POLYMORPHIC macro, then the system is able to return the identifier of
 * the true dynamic type of the object.
 */
template <typename T, typename E = void>
struct DynamicTypeDecl {
    static TypeId id(const T& object) {
        typedef TypeTraits<const T&> PropTraits;
        typename PropTraits::PointerType pointer = PropTraits::getPointer(object);
        static_assert(PropTraits::kind != ReferenceKind::None, "");
        static_assert(std::is_pointer<decltype(pointer)>::value, "Not pointer");
        return pointer != nullptr ? pointer->ponderClassId() : staticTypeId<T>();
    }
};

/* Specialization of DynamicTypeDecl for types that don't implement Meta RTTI
 */
template <typename T>
struct DynamicTypeDecl<T, typename std::enable_if<!HasPonderRtti<T>::value>::type> {
    static TypeId id(const T&) { return staticTypeId<T>(); }
};

template <typename T>
inline TypeId getTypeId() {
    return staticTypeId<T>();
}

template <typename T>
inline TypeId getTypeId(T& object) {
    return DynamicTypeDecl<T>::id(object);
}

/* Utility class to get a valid Meta identifier from a C++ type even if the type wasn't declared
 */
template <typename T, typename E = void>
struct SafeTypeId {
    static TypeId id() { return staticTypeId<T>(); }
    static TypeId name(T& object) { return DynamicTypeDecl<T>::id(object); }
};

/* Return the dynamic type identifier of a C++ object even if it doesn't exist (i.e. it can't fail)
 */
template <typename T>
inline TypeId safeTypeId() {
    return SafeTypeId<typename DataType<T>::Type>::id();
}

template <typename T>
inline TypeId safeTypeId(const T& object) {
    return SafeTypeId<T>::get(object);
}

/* Utility class to get a valid Meta identifier from a C++ type even if the type wasn't declared
 */
template <typename T, typename E = void>
struct SafeTypeName {
    static const char* name() { return staticTypeName<T>(); }
    static const char* name(T& object) { return DynamicTypeDecl<T>::name(object); }
};

/**
 * Specialization of SafeTypeName for types that have no Meta id
 */
template <typename T>
struct SafeTypeName<T, typename std::enable_if<!hasStaticTypeDecl<T>()>::type> {
    static constexpr const char* name() { return ""; }
    static constexpr const char* name(const T&) { return ""; }
};

/**
 * Specialization of SafeTypeName needed because "const void&" is not a valid expression
 */
template <>
struct SafeTypeName<void> {
    static constexpr const char* name() { return ""; }
};

/**
 * \brief Return the dynamic type identifier of a C++ object even if it doesn't exist
 *        (i.e. it can't fail)
 */
template <typename T>
const char* safeTypeName() {
    return SafeTypeName<typename DataType<T>::Type>::name();
}

template <typename T>
const char* safeTypeName(const T& object) {
    return SafeTypeName<T>::get(object);
}

}  // namespace detail
}  // namespace Meta

#endif  // META_DETAIL_TYPEID_HPP
