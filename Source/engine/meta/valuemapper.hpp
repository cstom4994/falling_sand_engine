

#pragma once
#ifndef META_VALUEMAPPER_HPP
#define META_VALUEMAPPER_HPP

#include "engine/meta/config.hpp"
#include "engine/meta/enum.hpp"
#include "engine/meta/enumobject.hpp"
#include "engine/meta/userobject.hpp"
#include "engine/meta/arraymapper.hpp"
#include "engine/meta/errors.hpp"
#include "engine/meta/util.hpp"
#include "engine/meta/valueref.hpp"

/**
 * \namespace MetaExt
 * \brief Meta user extendable namespace.
 * 
 * C++ only allows specialisation within the same namespace as the type you 
 * are extending. To avoid clashes with the ponder namespace we use `MetaExt` 
 * to provide a safe place to place custom specialisations.
 * \see ValueMapper.
 */

namespace MetaExt
{
    template <typename T, typename C = void> struct ValueMapper;
}

namespace Meta {

/**
 * \brief Map a C++ type to a Meta type
 *
 * This function simply returns the mapping defined by ValueMapper (i.e. \c ValueMapper<T>::type).
 *
 * \return Meta type which T maps to
 */
template <typename T>
inline ValueKind mapType()
{
    return MetaExt::ValueMapper<typename detail::DataType<T>::Type>::kind;
}

} // namespace Meta


namespace MetaExt {
    
/**
 * \class ValueMapper
 *
 * \brief Template providing a mapping between C++ types/values and Meta types/values
 *
 * ValueMapper<T> defines a mapping to and from type T to a Value. It defines three things in
 * order to make T fully compliant with the system:
 *
 * \li The abstract Meta type that T is mapped to
 * \li A function to convert from T *to* the mapped Meta type
 * \li A function to convert *from* all supported Meta types to T
 *
 * Pseudo-code:
 * \code
 * template <> struct ValueMapper<TypeSpecialised>
 * {
 *     static PonderValueKind to(ValueKind value)      { return convertToPonderType(value); }
 *     static TypeSpecialised from(PonderValueKind pv) { return convertPonderToType(pc); }
 * };
 * \endcode
 *
 * ValueMapper is specialized for every supported type, and can be specialized
 * for any of your own types in order to extend the system.
 *
 * Here is an example of mapping for a custom string class:
 *
 * \code
 * namespace MetaExt
 * {
 *     template <>
 *     struct ValueMapper<MyStringClass>
 *     {
 *         // The corresponding Meta type is "string"
 *         static constexpr Meta::ValueKind kind = Meta::ValueKind::String;
 *  
 *         // Convert from MyStringClass to Meta::String
 *         static Meta::String to(const MyStringClass& source)
 *         {
 *             return source.to_std_string();
 *         }
 * 
 *         // Convert from any type to MyStringClass
 *         // Be smart, just reuse ValueMapper<Meta::String> :)
 *         template <typename T>
 *         static MyStringClass from(const T& source)
 *         {
 *             return MyStringClass(ValueMapper<Meta::String>::from(source));
 *         }
 *     };
 * }
 * \endcode
 *
 * Generic version of ValueMapper -- T doesn't match with any specialization
 * and is thus treated as a user object    
 */
    
/** \cond NoDocumentation */
    
template <typename T, typename C>
struct ValueMapper
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::User;
    
    static Meta::UserObject to(const T& source) {return Meta::UserObject(source);}

    static T from(bool)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Boolean,Meta::mapType<T>()));}
    static T from(long)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Integer,Meta::mapType<T>()));}
    static T from(double)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Real,   Meta::mapType<T>()));}
    static T from(const Meta::String&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::String, Meta::mapType<T>()));}
    static T from(const Meta::EnumObject&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Enum,   Meta::mapType<T>()));}
    static T from(const Meta::detail::ValueRef&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Reference, Meta::mapType<T>()));}
    static T from(const Meta::UserObject& source)
        {return source.get<T>();}
};

/**
 * Specialization of ValueMapper for abstract types
 */
template <typename T>
struct ValueMapper<T, typename std::enable_if<std::is_abstract<T>::value>::type>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::User;
    
    static Meta::UserObject to(const T& source) {return Meta::UserObject(source);}
};

/**
 * Specialization of ValueMapper for pointers to basic types
 *  - Used for pass by-reference parameters that are non-registered types.
 */
template <typename T>
struct ValueMapper<T*, typename std::enable_if<!Meta::detail::hasStaticTypeDecl<T>()>::type>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::Reference;

    static Meta::detail::ValueRef to(T* source) {return Meta::detail::ValueRef::make(source);}

    static T* from(const Meta::detail::ValueRef& source) {return source.getRef<T>();}

    static T from(bool)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Boolean,Meta::mapType<T>()));}
    static T from(long)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Integer,Meta::mapType<T>()));}
    static T from(double)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Real,   Meta::mapType<T>()));}
    static T from(const Meta::String&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::String, Meta::mapType<T>()));}
    static T from(const Meta::EnumObject&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Enum,   Meta::mapType<T>()));}
    static T from(const Meta::UserObject&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::User,   Meta::mapType<T>()));}
};

///**
// * Specialization of ValueMapper for pointers to basic types
// *  - Used for pass by-reference parameters that are non-registered types.
// */
//template <typename T>
//struct ValueMapper<T&, typename std::enable_if<!Meta::detail::hasStaticTypeDecl<T>()>::type>
//{
//    static constexpr Meta::ValueKind kind = Meta::ValueKind::User;
//
//    static Meta::UserObject to(T& source) {return Meta::UserObject::makeRef(source);}
//
//    static T from(const Meta::UserObject& source) {return *static_cast<T*>(source.pointer());}
//
//    static T from(bool)
//    {META_ERROR(Meta::BadType(Meta::ValueKind::Boolean,Meta::mapType<T>()));}
//    static T from(long)
//    {META_ERROR(Meta::BadType(Meta::ValueKind::Integer,Meta::mapType<T>()));}
//    static T from(double)
//    {META_ERROR(Meta::BadType(Meta::ValueKind::Real,   Meta::mapType<T>()));}
//    static T from(const Meta::String&)
//    {META_ERROR(Meta::BadType(Meta::ValueKind::String, Meta::mapType<T>()));}
//    static T from(const Meta::EnumObject&)
//    {META_ERROR(Meta::BadType(Meta::ValueKind::Enum,   Meta::mapType<T>()));}
//};

/**
 * Specialization of ValueMapper for booleans
 */
template <>
struct ValueMapper<bool>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::Boolean;
    
    static bool to(bool source) {return source;}

    static bool from(bool source)                  {return source;}
    static bool from(long source)                  {return source != 0;}
    static bool from(double source)                {return source != 0.;}
    static bool from(const Meta::String& source) {return Meta::detail::convert<bool>(source);}
    static bool from(const Meta::EnumObject& source) {return source.value() != 0;}
    static bool from(const Meta::UserObject& source) {return source.pointer() != nullptr;}
    static bool from(const Meta::detail::ValueRef& source) {return source.getRef<bool>();}
};

/**
 * Specialization of ValueMapper for integers
 */
template <typename T>
struct ValueMapper<T,
    typename std::enable_if<
                 std::is_integral<T>::value
                 && !std::is_const<T>::value     // to avoid conflict with ValueMapper<const T>
             >::type >
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::Integer;
    static long to(T source) {return static_cast<long>(source);}

    static T from(bool source)                    {return static_cast<T>(source);}
    static T from(long source)                    {return static_cast<T>(source);}
    static T from(double source)                  {return static_cast<T>(source);}
    static T from(const Meta::String& source)   {return Meta::detail::convert<T>(source);}
    static T from(const Meta::EnumObject& source) {return static_cast<T>(source.value());}
    static T from(const Meta::UserObject&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::User, Meta::ValueKind::Integer));}
    static T from(const Meta::detail::ValueRef& source) {return *source.getRef<T>();}
};

/*
 * Specialization of ValueMapper for reals
 */
template <typename T>
struct ValueMapper<T,
    typename std::enable_if<
                 std::is_floating_point<T>::value
                 && !std::is_const<T>::value // to avoid conflict with ValueMapper<const T>
             >::type >
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::Real;
    static double to(T source) {return static_cast<double>(source);}

    static T from(bool source)                    {return static_cast<T>(source);}
    static T from(long source)                    {return static_cast<T>(source);}
    static T from(double source)                  {return static_cast<T>(source);}
    static T from(const Meta::String& source)   {return Meta::detail::convert<T>(source);}
    static T from(const Meta::EnumObject& source) {return static_cast<T>(source.value());}
    static T from(const Meta::UserObject&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::User, Meta::ValueKind::Real));}
    static T from(const Meta::detail::ValueRef& source) {return *source.getRef<T>();}
};

/**
 * Specialization of ValueMapper for Meta::String
 */
template <>
struct ValueMapper<Meta::String>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::String;
    static const Meta::String& to(const Meta::String& source) {return source;}
    
    static Meta::String from(bool source)
        {return Meta::detail::convert<Meta::String>(source);}
    static Meta::String from(long source)
        {return Meta::detail::convert<Meta::String>(source);}
    static Meta::String from(double source)
        {return Meta::detail::convert<Meta::String>(source);}
    static Meta::String from(const Meta::String& source)
        {return source;}
    static Meta::String from(const Meta::EnumObject& source)
        {return Meta::String(source.name());}
    static Meta::String from(const Meta::UserObject&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::User, Meta::ValueKind::String));}
    static Meta::String from(const Meta::detail::ValueRef& source)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Reference, Meta::ValueKind::String));}
};

// TODO - Add Meta::is_string() ?
template <>
struct ValueMapper<const Meta::String> : ValueMapper<Meta::String> {};

template <>
struct ValueMapper<Meta::detail::string_view>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::String;
    
    static Meta::String to(const Meta::detail::string_view& sv)
        {return Meta::String(sv.data(), sv.length());}
    template <typename T>
    static Meta::detail::string_view from(const T& source)
        {return Meta::detail::string_view(ValueMapper<Meta::String>::from(source));}
};

/**
 * Specialization of ValueMapper for const char*.
 * Conversions to const char* are disabled (can't return a pointer to a temporary)
 */
template <>
struct ValueMapper<const char*>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::String;
    static Meta::String to(const char* source) {return Meta::String(source);}
    
    template <typename T>
    static const char* from(const T&)
    {
        // If you get this error, it means you're trying to cast
        // a Meta::Value to a const char*, which is not allowed
        return T::CONVERSION_TO_CONST_CHAR_PTR_IS_NOT_ALLOWED();
    }
};

/**
 * Specialization of ValueMapper for arrays.
 * No conversion allowed, only type mapping is provided.
 *
 * Warning: special case for char[] and const char[], they are strings not arrays
 */
template <typename T>
struct ValueMapper<T,
    typename std::enable_if<
            MetaExt::ArrayMapper<T>::isArray
            && !std::is_same<typename MetaExt::ArrayMapper<T>::ElementType, char>::value
            && !std::is_same<typename MetaExt::ArrayMapper<T>::ElementType, const char>::value
        >::type >
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::Array;
};

/**
 * Specializations of ValueMapper for char arrays.
 * Conversion to char[N] is disabled (can't return an array).
 */
template <size_t N>
struct ValueMapper<char[N]>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::String;
    static Meta::String to(const char (&source)[N]) {return Meta::String(source);}
};
template <size_t N>
struct ValueMapper<const char[N]>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::String;
    static Meta::String to(const char (&source)[N]) {return Meta::String(source);}
};

/**
 * Specialization of ValueMapper for enum types
 */
template <typename T>
struct ValueMapper<T, typename std::enable_if<std::is_enum<T>::value>::type>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::Enum;
    static Meta::EnumObject to(T source) {return Meta::EnumObject(source);}

    static T from(bool source)      {return static_cast<T>(static_cast<long>(source));}
    static T from(long source)      {return static_cast<T>(source);}
    static T from(double source)    {return static_cast<T>(static_cast<long>(source));}
    static T from(const Meta::EnumObject& source)
        {return static_cast<T>(source.value());}
    static T from(const Meta::UserObject&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::User, Meta::ValueKind::Enum));}
    static T from(const Meta::detail::ValueRef& source)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Reference, Meta::ValueKind::Enum));}

    // The string -> enum conversion involves a little more work:
    // we try two different conversions (as a name and as a value)
    static T from(const Meta::String& source)
    {
        // Get the metaenum of T, if any
        const Meta::Enum* metaenum = Meta::enumByTypeSafe<T>();

        // First try as a name
        if (metaenum && metaenum->hasName(source))
            return static_cast<T>(metaenum->value(source));

        // Then try as a number
        long value = Meta::detail::convert<long>(source);
        if (!metaenum || metaenum->hasValue(value))
            return static_cast<T>(value);

        // Not a valid enum name or number: throw an error
        META_ERROR(Meta::BadType(Meta::ValueKind::String, Meta::ValueKind::Enum));
    }
};

/**
 * Specialization of ValueMapper for EnumObject
 */
template <>
struct ValueMapper<Meta::EnumObject>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::Enum;
    static const Meta::EnumObject& to(const Meta::EnumObject& source) {return source;}
    static const Meta::EnumObject& from(const Meta::EnumObject& source) {return source;}

    static Meta::EnumObject from(bool)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Boolean, Meta::ValueKind::Enum));}
    static Meta::EnumObject from(long)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Integer, Meta::ValueKind::Enum));}
    static Meta::EnumObject from(double)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Real,   Meta::ValueKind::Enum));}
    static Meta::EnumObject from(const Meta::String&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::String, Meta::ValueKind::Enum));}
    static Meta::EnumObject from(const Meta::UserObject&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Enum,   Meta::ValueKind::Enum));}
    static Meta::EnumObject from(const Meta::detail::ValueRef& source)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Reference, Meta::ValueKind::Enum));}
};

/**
 * Specialization of ValueMapper for Meta::ValueKind.
 */
template <>
struct ValueMapper<Meta::ValueKind>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::String;
    static Meta::String to(Meta::ValueKind source)
        {return Meta::String(Meta::detail::valueKindAsString(source));}
};

/**
 * Specialization of ValueMapper for UserObject
 */
template <>
struct ValueMapper<Meta::UserObject>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::User;
    static const Meta::UserObject& to(const Meta::UserObject& source) {return source;}
    static const Meta::UserObject& from(const Meta::UserObject& source) {return source;}

    static Meta::UserObject from(bool)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Boolean, Meta::ValueKind::User));}
    static Meta::UserObject from(long)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Integer, Meta::ValueKind::User));}
    static Meta::UserObject from(double)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Real,   Meta::ValueKind::User));}
    static Meta::UserObject from(const Meta::String&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::String, Meta::ValueKind::User));}
    static Meta::UserObject from(const Meta::EnumObject&)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Enum,   Meta::ValueKind::User));}
    static Meta::UserObject from(const Meta::detail::ValueRef& source)
        {META_ERROR(Meta::BadType(Meta::ValueKind::Reference, Meta::ValueKind::User));}
};

/**
 * Specialization of ValueMapper for void.
 * Conversion to void should never happen, the only aim of this
 * specialization is to define the proper type mapping.
 */
template <>
struct ValueMapper<void>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::None;
};

/**
 * Specialization of ValueMapper for NoType.
 * Conversion to NoType should never happen, the only aim of this
 * specialization is to define the proper mapped type.
 */
template <>
struct ValueMapper<Meta::NoType>
{
    static constexpr Meta::ValueKind kind = Meta::ValueKind::None;
};

/*----------------------------------------------------------------------
 * Modifiers.
 *  - Modify type to avoid supporting every variation above, e.g. const.
 */
    
/**
 * Show error for references. Not allowed.
 */
template <typename T>
struct ValueMapper<const T&>
{
    typedef int ReferencesNotAllowed[-(int)sizeof(T)];
};

/**
 * Show error for references using smart pointers.
 */
template <template <typename> class T, typename U>
struct ValueMapper<T<U>,
    typename std::enable_if<Meta::detail::IsSmartPointer<T<U>,U>::value>::type>
{
    typedef int ReferencesNotAllowed[-(int)sizeof(U)];
};

/** \endcond NoDocumentation */

} // namespace MetaExt

#endif // META_VALUEMAPPER_HPP
