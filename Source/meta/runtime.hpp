

/**
 * \file
 * \brief Runtime uses for Meta registered data.
 */

#ifndef META_USES_RUNTIME_HPP
#define META_USES_RUNTIME_HPP

#include "meta/class.hpp"
#include "meta/constructor.hpp"
#include "meta/rawtype.hpp"
#include "meta/util.hpp"
#include "meta/value.hpp"

namespace Meta {
namespace runtime {
namespace detail {

//-----------------------------------------------------------------------------
// Handle returning copies

template <typename R, typename U = void>
struct CallReturnCopy;

template <typename R>
struct CallReturnCopy<R, typename std::enable_if<!Meta::detail::IsUserType<R>::value>::type> {
    static inline Value value(R &&o) { return Value(o); }
};

template <typename R>
struct CallReturnCopy<R, typename std::enable_if<Meta::detail::IsUserType<R>::value>::type> {
    static_assert(!std::is_pointer<R>::value, "Cannot return unowned pointer. Use Meta::policy::ReturnInternalRef?");
    static inline Value value(R &&o) { return Value(UserObject::makeCopy(std::forward<R>(o))); }
};

//-----------------------------------------------------------------------------
// Handle returning internal references

template <typename R, typename U = void>
struct CallReturnInternalRef;

template <typename R>
struct CallReturnInternalRef<R, typename std::enable_if<!Meta::detail::IsUserType<R>::value && !std::is_same<typename Meta::detail::DataType<R>::Type, UserObject>::value>::type> {
    static inline Value value(R &&o) { return Value(o); }
};

template <typename R>
struct CallReturnInternalRef<R, typename std::enable_if<Meta::detail::IsUserType<R>::value || std::is_same<typename Meta::detail::DataType<R>::Type, UserObject>::value>::type> {
    static inline Value value(R &&o) { return Value(UserObject::makeRef(std::forward<R>(o))); }
};

//-----------------------------------------------------------------------------
// Choose which returner to use, based on policy
//  - map policy kind to actionable policy type

template <typename Policies_t, typename R>
struct ChooseCallReturner;

template <typename... Ps, typename R>
struct ChooseCallReturner<std::tuple<policy::ReturnCopy, Ps...>, R> {
    typedef CallReturnCopy<R> type;
};

template <typename... Ps, typename R>
struct ChooseCallReturner<std::tuple<policy::ReturnInternalRef, Ps...>, R> {
    typedef CallReturnInternalRef<R> type;
};

template <typename R>
struct ChooseCallReturner<std::tuple<>, R>  // default
{
    typedef CallReturnCopy<R> type;
};

template <typename P, typename... Ps, typename R>
struct ChooseCallReturner<std::tuple<P, Ps...>, R>  // recurse
{
    typedef typename ChooseCallReturner<std::tuple<Ps...>, R>::type type;
};

//-----------------------------------------------------------------------------

/*
 * Helper function which converts an argument to a C++ type
 *
 * The main purpose of this function is to convert any BadType error to
 * a BadArgument one.
 */
template <int TFrom, typename TTo>
struct ConvertArg {
    typedef typename std::remove_reference<TTo>::type ReturnType;
    static ReturnType convert(const Args &args, size_t index) {
        try {
            return args[index].to<typename std::remove_reference<TTo>::type>();
        } catch (const BadType &) {
            META_ERROR(BadArgument(args[index].kind(), mapType<TTo>(), index, "?"));
        }
    }
};

// Specialisation for returning references.
template <typename TTo>
struct ConvertArg<(int)ValueKind::User, TTo &> {
    typedef TTo &ReturnType;
    static ReturnType convert(const Args &args, size_t index) {
        auto &&uobj = const_cast<Value &>(args[index]).ref<UserObject>();
        if (uobj.pointer() == nullptr) META_ERROR(NullObject(&uobj.getClass()));
        return uobj.ref<TTo>();
    }
};

// Specialisation for returning const references.
template <typename TTo>
struct ConvertArg<(int)ValueKind::User, const TTo &> {
    typedef const TTo &ReturnType;
    static ReturnType convert(const Args &args, size_t index) {
        auto &&uobj = args[index].cref<UserObject>();
        if (uobj.pointer() == nullptr) META_ERROR(NullObject(&uobj.getClass()));
        return uobj.cref<TTo>();
    }
};

//-----------------------------------------------------------------------------
// Object function call helper to allow specialisation by return type. Applies policies.

template <typename A>
struct ConvertArgs {
    typedef typename Meta::detail::DataType<A>::Type Raw;
    static constexpr ValueKind kind = MetaExt::ValueMapper<Raw>::kind;
    typedef ConvertArg<(int)kind, A> Convertor;

    static typename Convertor::ReturnType convert(const Args &args, size_t index) { return Convertor::convert(args, index); }
};

template <typename R, typename FTraits, typename FPolicies>
class CallHelper {
public:
    template <typename F, typename... A, size_t... Is>
    static Value call(F func, const Args &args, META__SEQNS::index_sequence<Is...>) {
        typedef typename ChooseCallReturner<FPolicies, R>::type CallReturner;
        return CallReturner::value(func(ConvertArgs<A>::convert(args, Is)...));
    }
};

// Specialization of CallHelper for functions returning void
template <typename FTraits, typename FPolicies>
class CallHelper<void, FTraits, FPolicies> {
public:
    template <typename F, typename... A, size_t... Is>
    static Value call(F func, const Args &args, META__SEQNS::index_sequence<Is...>) {
        func(ConvertArgs<A>::convert(args, Is)...);
        return Value::nothing;
    }
};

//-----------------------------------------------------------------------------
// Convert traits to callable function wrapper. Generic for all function types.

template <typename R, typename A>
struct FunctionWrapper;

template <typename R, typename... A>
struct FunctionWrapper<R, std::tuple<A...>> {
    typedef typename std::function<R(A...)> Type;

    template <typename F, typename FTraits, typename FPolicies>
    static Value call(F func, const Args &args) {
        typedef META__SEQNS::make_index_sequence<sizeof...(A)> ArgEnumerator;
        return CallHelper<R, FTraits, FPolicies>::template call<F, A...>(func, args, ArgEnumerator());
    }
};

//-----------------------------------------------------------------------------
// Base for runtime function caller

class FunctionCaller {
public:
    FunctionCaller(const IdRef name) : m_name(name) {}
    virtual ~FunctionCaller() {}

    FunctionCaller(const FunctionCaller &) = delete;  // no copying

    const IdRef name() const { return m_name; }

    virtual Value execute(const Args &args) const = 0;

private:
    const IdRef m_name;
};

// The FunctionImpl class is a template which is specialized according to the
// underlying function prototype.
template <typename F, typename FTraits, typename FPolicies>
class FunctionCallerImpl final : public FunctionCaller {
public:
    FunctionCallerImpl(IdRef name, F function) : FunctionCaller(name), m_function(function) {}

private:
    typedef typename FTraits::Details::FunctionCallTypes CallTypes;
    typedef FunctionWrapper<typename FTraits::ExposedType, CallTypes> DispatchType;

    typename DispatchType::Type m_function;  // Object containing the actual function to call

    Value execute(const Args &args) const final { return DispatchType::template call<decltype(m_function), FTraits, FPolicies>(m_function, args); }
};

}  // namespace detail
}  // namespace runtime
}  // namespace Meta

/**
 * \namespace Meta::runtime
 * \brief Contains Meta runtime support public API.
 */

namespace Meta {
namespace runtime {

static inline void destroy(const UserObject &uo);

namespace detail {

template <typename... A>
struct ArgsBuilder {
    static Args makeArgs(A &&...args) { return Args(std::forward<A>(args)...); }
};

template <>
struct ArgsBuilder<Args> {
    static Args makeArgs(const Args &args) { return args; }
    static Args makeArgs(Args &&args) { return std::move(args); }
};

template <>
struct ArgsBuilder<void> {
    static Args makeArgs(const Args &args) { return Args::empty; }
};

struct UserObjectDeleter {
    void operator()(UserObject *uo) { destroy(*uo); }
};

}  // namespace detail

/**
 * \brief This object is used to create instances of metaclasses
 *
 * There are helpers for this class, see Meta::runtime::construct() and
 * Meta::runtime::create().
 *
 * Example of use:
 * \code
 * runtime::ObjectFactory fact(classByType<MyClass>.function("fpp"));
 * fact.create("bar");
 * \endcode
 *
 */
class ObjectFactory {
public:
    /**
     * \brief Constructor
     *
     * \param cls The Class to be called
     * \return a Class reference
     */
    ObjectFactory(const Class &cls) : m_class(cls) {}

    /**
     * \brief Get the class begin used
     *
     * \return a Class reference
     */
    const Class &getClass() const { return m_class; }

    /**
     * \brief Construct a new instance of the C++ class bound to the metaclass
     *
     * If no constructor can match the provided arguments, UserObject::nothing
     * is returned. If a pointer is provided then placement new is used instead of
     * the new instance being dynamically allocated using new.
     * The new instance is wrapped into a UserObject.
     *
     * \note It must be destroyed with the appropriate destruction function:
     * Class::destroy for new and Class::destruct for placement new.
     *
     * \param args Arguments to pass to the constructor (empty by default)
     * \param ptr Optional pointer to the location to construct the object (placement new)
     * \return New instance wrapped into a UserObject, or UserObject::nothing if it failed
     * \sa create()
     */
    UserObject construct(const Args &args = Args::empty, void *ptr = nullptr) const;

    /**
     * \brief Create a new instance of the class bound to the metaclass
     *
     * Create an object without having to create an Args list. See notes for Class::construct().
     * If you need to create an argument list at runtime and use it to create an object then
     * see Class::construct().
     *
     * \param args An argument list.
     * \return New instance wrapped into a UserObject, or UserObject::nothing if it failed
     * \sa construct()
     */
    template <typename... A>
    UserObject create(A... args) const;

    /**
     * \brief Destroy an instance of the C++ class bound to the metaclass
     *
     * This function must be called to destroy every instance created with
     * Class::construct.
     *
     * \param object Object to be destroyed
     *
     * \see construct
     */
    void destroy(const UserObject &object) const;

    /**
     * \brief Destruct an object created using placement new
     *
     * This function must be called to destroy every instance created with
     * Class::construct.
     *
     * \param object Object to be destroyed
     *
     * \see construct
     */
    void destruct(const UserObject &object) const;

private:
    const Class &m_class;
};

/**
 * \brief This object is used to invoke a object member function, or method
 *
 * There are helpers for this class, see Meta::runtime::call() and
 * Meta::runtime::callStatic().
 *
 */
class ObjectCaller {
public:
    /**
     * \brief Constructor
     *
     * \param fn The Function to be called
     * \return a Function reference
     */
    ObjectCaller(const Function &fn);

    /**
     * \brief Get the function begin used
     *
     * \return a Function reference
     */
    const Function &function() const { return m_func; }

    /**
     * \brief Call the function
     *
     * \param obj Object
     * \param args Arguments to pass to the function, for example "Meta::Args::empty"
     *
     * \return Value returned by the function call
     *
     * \code
     * runtime::ObjectCaller caller(classByType<MyClass>.function("foo"));
     * caller.call(instancem, "bar");
     * \endcode
     *
     * \throw ForbiddenCall the function is not callable
     * \throw NullObject object is invalid
     * \throw NotEnoughArguments too few arguments are provided
     * \throw BadArgument one of the arguments can't be converted to the requested type
     */
    template <typename... A>
    Value call(const UserObject &obj, A &&...args);

private:
    const Function &m_func;
    runtime::detail::FunctionCaller *m_caller;
};

/**
 * \brief This object is used to invoke a function
 *
 * There are helpers for this class, see Meta::runtime::call() and
 * Meta::runtime::callStatic().
 *
 */
class FunctionCaller {
public:
    /**
     * \brief Constructor
     *
     * \param f The function to call
     */
    FunctionCaller(const Function &f);

    /**
     * \brief Get the function begin used
     *
     * \return a Function reference
     */
    const Function &function() const { return m_func; }

    /**
     * \brief Call the static function
     *
     * \param args Arguments to pass to the function, for example "Meta::Args::empty"
     *
     * \return Value returned by the function call
     *
     * \code
     * runtime::FunctionCaller caller(classByType<MyClass>.function("fpp"));
     * caller.call(Args("bar"));
     * \endcode
     *
     * \throw NotEnoughArguments too few arguments are provided
     * \throw BadArgument one of the arguments can't be converted to the requested type
     */
    template <typename... A>
    Value call(A... args);

private:
    const Function &m_func;
    runtime::detail::FunctionCaller *m_caller;
};

//--------------------------------------------------------------------------------------
// Helpers

/**
 * \brief Create instance of metaclass as a UserObject
 *
 * This is a helper function which uses ObjectFactory to create the instance.
 *
 * \param cls The metaclass to make an instance of
 * \param args The constructor arguments for the class instance
 * \return A UserObject which owns an instance of the metaclass.
 *
 * \snippet simple.cpp eg_simple_create
 *
 * \sa destroy()
 */
template <typename... A>
static inline UserObject create(const Class &cls, A... args) {
    return ObjectFactory(cls).create(args...);
}

typedef std::unique_ptr<UserObject> UniquePtr;

inline UniquePtr makeUniquePtr(UserObject *obj) { return UniquePtr(obj); }

template <typename... A>
static inline UniquePtr createUnique(const Class &cls, A... args) {
    auto p = new UserObject;
    *p = create(cls, args...);
    return makeUniquePtr(p);
}

/**
 * \brief Destroy a UserObject instance
 *
 * This is a helper function which uses ObjectFactory to destroy the instance.
 *
 * \param obj Reference to UserObject instance to destroy
 *
 * \sa create()
 */
static inline void destroy(const UserObject &obj) { ObjectFactory(obj.getClass()).destroy(obj); }

/**
 * \brief Call a member function
 *
 * This is a helper function which uses ObjectCaller to call the member function.
 *
 * \param fn The Function to call
 * \param obj Reference to UserObject instance to destroy
 * \param args Arguments for the function
 * \return The return value. This is NoType if function return type return is `void`.
 *
 * \sa callStatic(), Class::function()
 */
template <typename... A>
static inline Value call(const Function &fn, const UserObject &obj, A &&...args) {
    return ObjectCaller(fn).call(obj, detail::ArgsBuilder<A...>::makeArgs(std::forward<A>(args)...));
}

/**
 * \brief Call a non-member function
 *
 * This is a helper function which uses FunctionCaller to call the function.
 *
 * \param fn The Function to call
 * \param args Arguments for the function
 * \return The return value. This is NoType if function return type return is `void`.
 *
 * \sa call(), Class::function()
 */
template <typename... A>
static inline Value callStatic(const Function &fn, A &&...args) {
    return FunctionCaller(fn).call(detail::ArgsBuilder<A...>::makeArgs(std::forward<A>(args)...));
}

}  // namespace runtime
}  // namespace Meta

//--------------------------------------------------------------------------------------
// .inl

namespace Meta {
namespace runtime {

template <typename... A>
inline UserObject ObjectFactory::create(A... args) const {
    Args a(args...);
    return construct(a);
}

template <typename... A>
inline Value ObjectCaller::call(const UserObject &obj, A &&...vargs) {
    if (obj.pointer() == nullptr) META_ERROR(NullObject(&obj.getClass()));

    Args args(detail::ArgsBuilder<A...>::makeArgs(std::forward<A>(vargs)...));

    // Check the number of arguments
    if (args.count() < m_func.paramCount()) META_ERROR(NotEnoughArguments(m_func.name(), args.count(), m_func.paramCount()));

    args.insert(0, obj);

    return m_caller->execute(args);
}

template <typename... A>
inline Value FunctionCaller::call(A... vargs) {
    Args args(detail::ArgsBuilder<A...>::makeArgs(vargs...));

    // Check the number of arguments
    if (args.count() < m_func.paramCount()) META_ERROR(NotEnoughArguments(m_func.name(), args.count(), m_func.paramCount()));

    return m_caller->execute(args);
}

}  // namespace runtime
}  // namespace Meta

//--------------------------------------------------------------------------------------

// define once in client program to instance this
#ifdef META_USES_RUNTIME_IMPL

namespace Meta {
namespace runtime {

UserObject ObjectFactory::construct(const Args &args, void *ptr) const {
    // Search an arguments match among the list of available constructors
    for (size_t nb = m_class.constructorCount(), i = 0; i < nb; ++i) {
        const Constructor &constructor = *m_class.constructor(i);
        if (constructor.matches(args)) {
            // Match found: use the constructor to create the new instance
            return constructor.create(ptr, args);
        }
    }

    return UserObject::nothing;  // no match found
}

void ObjectFactory::destroy(const UserObject &object) const {
    m_class.destruct(object, false);

    const_cast<UserObject &>(object) = UserObject::nothing;
}

void ObjectFactory::destruct(const UserObject &object) const {
    m_class.destruct(object, true);

    const_cast<UserObject &>(object) = UserObject::nothing;
}

ObjectCaller::ObjectCaller(const Function &f) : m_func(f), m_caller(std::get<uses::Uses::eRuntimeModule>(*reinterpret_cast<const uses::Uses::PerFunctionUserData *>(m_func.getUsesData()))) {}

FunctionCaller::FunctionCaller(const Function &f) : m_func(f), m_caller(std::get<uses::Uses::eRuntimeModule>(*reinterpret_cast<const uses::Uses::PerFunctionUserData *>(m_func.getUsesData()))) {}

}  // namespace runtime
}  // namespace Meta

#endif  // META_USES_RUNTIME_IMPL

#endif  // META_USES_RUNTIME_HPP
