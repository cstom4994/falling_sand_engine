

/** \cond NoDocumentation */
#ifndef META_PONDERTYPE_HPP
#define META_PONDERTYPE_HPP
/** \endcond NoDocumentation */

#include "engine/meta/config.hpp"
#include "engine/meta/typeid.hpp"
#include "type.hpp"

namespace Meta {

namespace detail {
template <typename T>
struct StaticTypeDecl;
template <typename T>
constexpr const char* staticTypeName(T&);
void ensureTypeRegistered(TypeId const& id, void (*registerFunc)());
}  // namespace detail

/**
 * \file ponder/pondertype.hpp
 *
 * \def META_TYPE(TYPE)
 *
 * \brief Macro used to register a C++ type to Meta
 *
 * Every type manipulated by Meta must be registered with META_TYPE(), META_AUTO_TYPE()
 * or their NONCOPYABLE versions.
 *
 * Example:
 *
 * \code
 * class MyClass
 * {
 *     class MyNestedClass
 *     {
 *     };
 * };
 *
 * META_TYPE(MyClass)
 * META_TYPE(MyClass::MyNestedClass)
 * \endcode
 *
 * \note This macro handles types that contain commas, e.g. `Data<float,int,int>`.
 *
 * \sa META_TYPE(), META_AUTO_TYPE(), \ref eg_page_declare
 */
#define META_TYPE(...)                                                          \
    namespace Meta {                                                            \
    namespace detail {                                                          \
    template <>                                                                 \
    struct StaticTypeDecl<__VA_ARGS__> {                                        \
        static TypeId id(bool = true) { return calcTypeId<__VA_ARGS__>(); }     \
        static constexpr const char* name(bool = true) { return #__VA_ARGS__; } \
        static constexpr bool defined = true, copyable = true;                  \
    };                                                                          \
    }                                                                           \
    }

/**
 * \brief Macro used to register a C++ type to Meta with automatic, on-demand metaclass creation
 *
 * Using this macro rather than META_TYPE() will make Meta automatically call
 * the provided registration function the first time the metaclass is requested.
 * This is useful when you don't want to have to manually call an "init" function to
 * create your metaclass.
 *
 * Every type manipulated by Meta must be registered with META_TYPE(), META_AUTO_TYPE()
 * or their NONCOPYABLE versions.
 *
 * \note This macro will fail with types that contain commas, e.g. `Data<float,int,int>`. Instead,
 *       use META_TYPE().
 *
 * Example:
 *
 * \code
 * class MyClass
 * {
 * public:
 *
 *     static void registerMetaClass();
 * };
 * META_AUTO_TYPE(MyClass, &MyClass::registerMetaClass)
 *
 * void MyClass::registerMetaClass()
 * {
 *     Meta::Class::declare<MyClass>("MyClass")
 *         // ... declarations ... ;
 * }
 * \endcode
 *
 * \sa META_TYPE(), \ref eg_page_declare, \ref eg_page_shapes
 */
#define META_AUTO_TYPE(TYPE, REGISTER_FN)                                                     \
    namespace Meta {                                                                          \
    namespace detail {                                                                        \
    template <>                                                                               \
    struct StaticTypeDecl<TYPE> {                                                             \
        static TypeId id(bool checkRegister = true) {                                         \
            if (checkRegister) detail::ensureTypeRegistered(calcTypeId<TYPE>(), REGISTER_FN); \
            return calcTypeId<TYPE>();                                                        \
        }                                                                                     \
        static const char* name(bool checkRegister = true) {                                  \
            if (checkRegister) detail::ensureTypeRegistered(calcTypeId<TYPE>(), REGISTER_FN); \
            return #TYPE;                                                                     \
        }                                                                                     \
        static constexpr bool defined = true, copyable = true;                                \
    };                                                                                        \
    }                                                                                         \
    }
   // TODO - ensureTypeRegistered() called every time referenced!

/**
 * \brief Macro used to register a non-copyable C++ type to Meta
 *
 * Disabled copy and assignment cannot be detected at compile-time, thus users have to
 * explicitly tell Meta when a type is not copyable/assignable. Objects of a non-copyable
 * class can be modified through their metaproperties, but they can't be written with a
 * single call to replace to whole object.
 *
 * Every type manipulated by Meta must be registered with META_TYPE(), META_AUTO_TYPE()
 * or their NONCOPYABLE versions.
 *
 * Example:
 *
 * \code
 * class NonCopyable : util::NonCopyable
 * {
 *     int x;
 * };
 * META_TYPE_NONCOPYABLE(NonCopyable)
 *
 * class MyClass
 * {
 *     NonCopyable* nc;
 * };
 * META_TYPE(MyClass)
 *
 * MyClass c;
 * const Meta::Class& m1 = Meta::classByObject(c);
 * const Meta::Class& m2 = Meta::classByObject(c.nc);
 * const Meta::Property& p1 = m1.property("nc");
 * const Meta::Property& p2 = m2.property("x");
 * p1.set(c, NonCopyable()); // ERROR
 * p2.set(p1.get(c).to<Meta::UserObject>(), 10); // OK
 * \endcode
 *
 * \sa META_TYPE()
 */
#define META_TYPE_NONCOPYABLE(TYPE)                            \
    namespace Meta {                                           \
    namespace detail {                                         \
    template <>                                                \
    struct StaticTypeDecl<TYPE> {                              \
        static const char* name(bool = true) { return #TYPE; } \
        static constexpr bool defined = true, copyable = true; \
    };                                                         \
    }                                                          \
    }

/**
 * \brief Macro used to register a non-copyable C++ type to Meta with automatic
 *        metaclass creation
 *
 * Using this macro rather than META_TYPE_NONCOPYABLE will make Meta automatically call
 * the provided registration function the first time the metaclass is requested.
 * This is useful when you don't want to have to manually call an "init" function to
 * create your metaclass.
 *
 * Every type manipulated by Meta must be registered with META_TYPE(), META_AUTO_TYPE()
 * or their NONCOPYABLE versions.
 *
 * \sa META_AUTO_TYPE(), META_TYPE_NONCOPYABLE()
 */
#define META_AUTO_TYPE_NONCOPYABLE(TYPE, REGISTER_FN)                                         \
    namespace Meta {                                                                          \
    namespace detail {                                                                        \
    template <>                                                                               \
    struct StaticTypeDecl<TYPE> {                                                             \
        static TypeId id(bool checkRegister = true) {                                         \
            if (checkRegister) detail::ensureTypeRegistered(calcTypeId<TYPE>(), REGISTER_FN); \
            return calcTypeId<TYPE>();                                                        \
        }                                                                                     \
        static const char* name(bool checkRegister = true) {                                  \
            if (checkRegister) detail::ensureTypeRegistered(calcTypeId<TYPE>(), REGISTER_FN); \
            return #TYPE;                                                                     \
        }                                                                                     \
        static constexpr bool defined = true, copyable = true;                                \
    };                                                                                        \
    }                                                                                         \
    }

/**
 * \brief Macro used to activate the Meta RTTI system into a hierarchy of classes
 *
 * This macro must be inserted in both base and derived classes if you want Meta
 * to be able to retrieve the dynamic type of *polymorphic objects*.
 *
 * \note This macro does not need to be inserted into all Meta classes being declared,
 *       only ones which would like to support features like downcasting via polymorphism.
 *       See \ref eg_page_shapes for an example.
 *
 * Example:
 *
 * \code
 * class MyBase
 * {
 *     META_POLYMORPHIC()
 * };
 *
 * class MyDerived : public MyBase
 * {
 *     META_POLYMORPHIC()
 * };
 *
 * MyBase* b = new MyDerived;
 * const Meta::Class& mc = Meta::classByObject(b);
 * // mc == metaclass of MyDerived
 * \endcode
 *
 * \sa \ref eg_page_shapes
 */
#define META_POLYMORPHIC()                                                                   \
public:                                                                                      \
    virtual Meta::TypeId ponderClassId() const { return Meta::detail::staticTypeId(*this); } \
                                                                                             \
private:

}  // namespace Meta

/** \cond NoDocumentation */
#endif  // META_PONDERTYPE_HPP
/** \endcond NoDocumentation */
