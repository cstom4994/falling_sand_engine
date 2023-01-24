

#ifndef META_USES_LUA_HPP
#define META_USES_LUA_HPP

/**
 * \file
 * \brief This file contains the user Lua API for Meta
 */

#include "meta/class.hpp"
#include "meta/runtime.hpp"
#include "libs/lua/lua.hpp"

/**
 * \namespace Meta::lua
 * \brief Contains Meta Lua support public API.
 *
 * You can automatically generate Lua bindings for your declared API.
 *
 * \namespace Meta::lua::detail
 * \brief Meta Lua support hidden implementation details.
 *
 * \namespace Meta::lua::detail
 * \brief Meta Lua support hidden implementation details.
 */

namespace Meta {
namespace lua {

/**
 * \brief Expose a single Meta metaclass to a Lua state
 *
 * \param L Lua state in which to expose the class.
 * \param cls Metaclass instance in Meta.
 * \param exposeName The name of the class in the Lua state.
 */
void expose(lua_State *L, const Class &cls, const IdRef exposeName);

/**
 * \brief Expose a single Meta enumeration to a Lua state
 *
 * \param L Lua state in which to expose the class.
 * \param e Enum instance in Meta.
 * \param exposeName The name of the class in the Lua state.
 */
void expose(lua_State *L, const Enum &e, const IdRef exposeName);

namespace detail {

template <typename T, typename U = void>
struct Exposer {};

template <typename T>
struct Exposer<T, typename std::enable_if<std::is_class<T>::value>::type> {
    static inline void exposeType(lua_State *L, const IdRef exposeName) { expose(L, classByType<T>(), exposeName); }
};

template <typename T>
struct Exposer<T, typename std::enable_if<std::is_enum<T>::value>::type> {
    static inline void exposeType(lua_State *L, const IdRef exposeName) { expose(L, enumByType<T>(), exposeName); }
};

}  // namespace detail

/**
 * \brief Expose a single Meta type to a Lua state
 *
 * \param T Type to expose. Can be a class or enumeration.
 * \param L Lua state in which to expose the class.
 * \param exposeName The name of the class in the Lua state.
 */
template <typename T>
void expose(lua_State *L, const IdRef exposeName) {
    detail::Exposer<T>::exposeType(L, exposeName);
}

/**
 * \brief Push a copy of a UserObject onto the Lua stack
 *
 * You choose whether to copy or reference an object. E.g.
 * \code
 * pushUserObject(L, val.cref<UserObject>());  // reference
 * pushUserObject(L, val.to<UserObject>());    // copy
 * \endcode
 *
 * \param L Lua state to use.
 * \param uobj The UserObject to copy.
 * \return Number of items pushed onto the Lua stack
 */
int pushUserObject(lua_State *L, const UserObject &uobj);

/**
 * \brief Expose all existing Meta registered objects to a Lua state
 *
 * \param L Lua state in which to expose the objects.
 */
// void exposeAll(lua_State *L);

bool runString(lua_State *L, const char *luaCode);

}  // namespace lua
}  // namespace Meta

//----------------------------------------------------------------------------

// define once in client program to instance this

#define META_IF_LUA(...) __VA_ARGS__

// forward declare
namespace Meta {
namespace lua {
int pushUserObject(lua_State *L, const Meta::UserObject &uobj);
}
}  // namespace Meta

//-----------------------------------------------------------------------------

namespace MetaExt {

using namespace Meta;

inline UserObject *toUserObject(lua_State *L, int index) { return reinterpret_cast<UserObject *>(lua_touserdata(L, (int)index)); }

struct LuaTable {
    lua_State *L;
};

//-----------------------------------------------------------------------------
// Convert Lua call arguments to C++ types.

template <typename P, typename U = void>
struct LuaValueReader {};

template <typename P>
struct LuaValueReader<P, typename std::enable_if<std::is_integral<P>::value>::type> {
    typedef P ParamType;
    static inline ParamType convert(lua_State *L, size_t index) { return static_cast<ParamType>(luaL_checkinteger(L, (int)index)); }
};

template <typename P>
struct LuaValueReader<P, typename std::enable_if<std::is_floating_point<P>::value>::type> {
    typedef P ParamType;
    static inline ParamType convert(lua_State *L, size_t index) { return static_cast<ParamType>(luaL_checknumber(L, (int)index)); }
};

template <typename P>
struct LuaValueReader<P, typename std::enable_if<std::is_enum<P>::value>::type> {
    typedef P ParamType;
    static inline ParamType convert(lua_State *L, size_t index) {
        const lua_Integer i = luaL_checkinteger(L, (int)index);
        return static_cast<P>(i);
    }
};

template <typename P>
struct LuaValueReader<P, typename std::enable_if<std::is_same<std::string, typename detail::DataType<P>::Type>::value>::type> {
    typedef std::string ParamType;
    static inline ParamType convert(lua_State *L, size_t index) { return ParamType(luaL_checkstring(L, (int)index)); }
};

template <>
struct LuaValueReader<Meta::detail::string_view> {
    typedef Meta::detail::string_view ParamType;
    static inline ParamType convert(lua_State *L, size_t index) { return ParamType(luaL_checkstring(L, (int)index)); }
};

template <typename P>
struct LuaValueReader<P &, typename std::enable_if<detail::IsUserType<P>::value>::type> {
    typedef P &ParamType;
    typedef typename detail::DataType<ParamType>::Type DataType;

    static inline ParamType convert(lua_State *L, size_t index) {
        if (!lua_isuserdata(L, (int)index)) {
            luaL_error(L, "Argument %d: expecting user data", (int)index);
        }

        UserObject *uobj = reinterpret_cast<UserObject *>(lua_touserdata(L, (int)index));
        return uobj->ref<DataType>();
    }
};

template <typename P>
struct LuaValueReader<P *, typename std::enable_if<Meta::detail::IsUserType<P>::value>::type> {
    typedef P *ParamType;
    typedef typename Meta::detail::DataType<ParamType>::Type DataType;

    static inline ParamType convert(lua_State *L, size_t index) {
        if (!lua_isuserdata(L, (int)index)) {
            luaL_error(L, "Argument %d: expecting user data", (int)index);
        }

        UserObject *uobj = reinterpret_cast<UserObject *>(lua_touserdata(L, (int)index));
        return &uobj->ref<DataType>();
    }
};

// User function wants to parse a Lua table.
template <>
struct LuaValueReader<LuaTable> {
    typedef LuaTable ParamType;
    static inline ParamType convert(lua_State *L, size_t index) {
        luaL_checktype(L, (int)index, LUA_TTABLE);
        LuaTable t = {L};
        return t;
    }
};

//-----------------------------------------------------------------------------
// Write values to Lua. Push to stack.

template <typename T, typename U = void>
struct LuaValueWriter {};

template <typename T>
struct LuaValueWriter<T, typename std::enable_if<std::is_integral<T>::value>::type> {
    static inline int push(lua_State *L, T value) { return lua_pushinteger(L, value), 1; }
};

template <typename T>
struct LuaValueWriter<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
    static inline int push(lua_State *L, T value) { return lua_pushnumber(L, value), 1; }
};

template <>
struct LuaValueWriter<std::string> {
    static inline int push(lua_State *L, const std::string &value) { return lua_pushstring(L, value.data()), 1; }
};

template <>
struct LuaValueWriter<const char *>  // non-const object
{
    static inline int push(lua_State *L, const char *value) { return lua_pushstring(L, value), 1; }
};

template <>
struct LuaValueWriter<const char *const>  // const object
{
    static inline int push(lua_State *L, const char *value) { return lua_pushstring(L, value), 1; }
};

template <>
struct LuaValueWriter<Meta::detail::string_view> {
    static inline int push(lua_State *L, const Meta::detail::string_view &value) { return lua_pushstring(L, value.data()), 1; }
};

template <>
struct LuaValueWriter<UserObject> {
    static inline int push(lua_State *L, const UserObject &value) { return Meta::lua::pushUserObject(L, value); }
};

// Return tuples as individual elements. tuple<x,y> -> x,y
template <typename... R>
struct LuaValueWriter<const std::tuple<R...>> {
    template <size_t... Is>
    static inline void pushElements(lua_State *L, std::tuple<R...> const &value, META__SEQNS::index_sequence<Is...>) {
        const int r[sizeof...(R)] = {LuaValueWriter<R>::push(L, std::get<Is>(value))...};
        (void)r;
    }

    static inline int push(lua_State *L, std::tuple<R...> const &value) {
        typedef META__SEQNS::make_index_sequence<sizeof...(R)> Enumerator;
        pushElements(L, value, Enumerator());
        return sizeof...(R);
    }
};

// Non-const tuples are const to us.
template <typename... R>
struct LuaValueWriter<std::tuple<R...>> : public LuaValueWriter<const std::tuple<R...>> {};

}  // namespace MetaExt

namespace Meta {
namespace lua {
namespace detail {

using namespace MetaExt;

//-----------------------------------------------------------------------------
// Handle returning multiple values

template <typename R, typename U = void>
struct CallReturnMultiple;

template <typename R>
struct CallReturnMultiple<R> {
    static inline int value(lua_State *L, R &&o) { return LuaValueWriter<R>::push(L, o); }
};

//-----------------------------------------------------------------------------
// Handle returning copies

template <typename R, typename U = void>
struct CallReturnCopy;

template <typename R>
struct CallReturnCopy<R, typename std::enable_if<!Meta::detail::IsUserType<R>::value>::type> {
    // "no member named push" error here means the type returned is not covered.
    static inline int value(lua_State *L, R &&o) { return LuaValueWriter<R>::push(L, o); }
};

template <typename R>
struct CallReturnCopy<R, typename std::enable_if<Meta::detail::IsUserType<R>::value>::type> {
    static inline int value(lua_State *L, R &&o) { return LuaValueWriter<UserObject>::push(L, UserObject::makeCopy(std::forward<R>(o))); }
};

//-----------------------------------------------------------------------------
// Handle returning internal references

template <typename R, typename U = void>
struct CallReturnInternalRef;

template <typename R>
struct CallReturnInternalRef<R, typename std::enable_if<!Meta::detail::IsUserType<R>::value && !std::is_same<typename Meta::detail::DataType<R>::Type, UserObject>::value>::type> {
    static inline int value(lua_State *L, R &&o) { return LuaValueWriter<R>::push(L, o); }
};

template <typename R>
struct CallReturnInternalRef<R, typename std::enable_if<Meta::detail::IsUserType<R>::value || std::is_same<typename Meta::detail::DataType<R>::Type, UserObject>::value>::type> {
    static inline int value(lua_State *L, R &&o) { return LuaValueWriter<UserObject>::push(L, UserObject::makeRef(std::forward<R>(o))); }
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

template <typename... Ps, typename R>
struct ChooseCallReturner<std::tuple<policy::ReturnMultiple, Ps...>, R> {
    typedef CallReturnMultiple<R> type;
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
// Object function call helper to allow specialisation by return type. Applies policies.

template <typename P>
struct ConvertArgs {
    typedef LuaValueReader<P> Convertor;

    static typename Convertor::ParamType convert(lua_State *L, size_t index) { return Convertor::convert(L, index + 1); }
};

template <typename R, typename FTraits, typename FPolicies>
class CallHelper {
public:
    template <typename F, typename... A, size_t... Is>
    static int call(F func, lua_State *L, META__SEQNS::index_sequence<Is...>) {
        typedef typename ChooseCallReturner<FPolicies, R>::type CallReturner;
        return CallReturner::value(L, func(ConvertArgs<A>::convert(L, Is)...));
    }
};

// Specialization of CallHelper for functions returning void
template <typename FTraits, typename FPolicies>
class CallHelper<void, FTraits, FPolicies> {
public:
    template <typename F, typename... A, size_t... Is>
    static int call(F func, lua_State *L, META__SEQNS::index_sequence<Is...>) {
        func(ConvertArgs<A>::convert(L, Is)...);
        return 0;  // return nil
    }
};

//-----------------------------------------------------------------------------
// Convert traits to callable function wrapper. Generic for all function types.

template <typename R, typename P>
struct FunctionWrapper;

template <typename R, typename... P>
struct FunctionWrapper<R, std::tuple<P...>> {
    typedef typename std::function<R(P...)> Type;

    template <typename F, typename FTraits, typename FPolicies>
    static int call(F func, lua_State *L) {
        typedef META__SEQNS::make_index_sequence<sizeof...(P)> ArgEnumerator;

        return CallHelper<R, FTraits, FPolicies>::template call<F, P...>(func, L, ArgEnumerator());
    }
};

//-----------------------------------------------------------------------------
// Base for runtime function caller

class FunctionCaller {
public:
    FunctionCaller(const IdRef name, int (*fn)(lua_State *) = nullptr) : m_name(name), m_luaFunc(fn) {}

    FunctionCaller(const FunctionCaller &) = delete;  // no copying
    virtual ~FunctionCaller() {}

    const IdRef name() const { return m_name; }

    void pushFunction(lua_State *L) {
        lua_pushlightuserdata(L, (void *)this);
        lua_pushcclosure(L, m_luaFunc, 1);
    }

private:
    const IdRef m_name;
    int (*m_luaFunc)(lua_State *);
};

// The FunctionImpl class is a template which is specialized according to the
// underlying function prototype.
template <typename F, typename FTraits, typename FPolicies>
class FunctionCallerImpl : public FunctionCaller {
public:
    FunctionCallerImpl(IdRef name, F function) : FunctionCaller(name, &call), m_function(function) {}

private:
    typedef FunctionCallerImpl<F, FTraits, FPolicies> ThisType;

    typedef typename FTraits::Details::FunctionCallTypes CallTypes;
    typedef FunctionWrapper<typename FTraits::ExposedType, CallTypes> DispatchType;

    typename DispatchType::Type m_function;  // Object containing the actual function to call

    static int call(lua_State *L) {
        lua_pushvalue(L, lua_upvalueindex(1));
        ThisType *self = reinterpret_cast<ThisType *>(lua_touserdata(L, -1));
        lua_pop(L, 1);

        return DispatchType::template call<decltype(m_function), FTraits, FPolicies>(self->m_function, L);
    }
};

}  // namespace detail
}  // namespace lua
}  // namespace Meta

/**
 * \namespace Meta::uses
 * \brief Uses for Meta.
 * Code within this namespace uses the types registered with Meta for various purposes.
 */

namespace Meta {
namespace uses {

/**
 *  The "uses" are a way for users of the compile-time types to store
 *  information. For example, this may be templated code that uses the types only
 *  available during compilation. These may then be used at runtime. The idea is
 *  to decouple modules from the metaclass data to avoid complexity.
 *
 *  Each module supplies (pseudo-code):
 *
 *  \code
 *  struct Use_name {
 *      static module_ns::detail::PerConstructor_t* perConstructor(IdRef name, C constructor)
 *      static module_ns::detail::PerFunc_t* perFunction(IdRef name, F function)
 *  }
 *  \endcode
 */

/**
 *  \brief Runtime behaviour
 *
 *  This module provides runtime behaviour like creation of UserObjects and calling
 *  functions
 */
struct RuntimeUse {
    /// Factory for per-function runtime data
    template <typename F, typename FTraits, typename Policies_t>
    static runtime::detail::FunctionCaller *perFunction(IdRef name, F function) {
        return new runtime::detail::FunctionCallerImpl<F, FTraits, Policies_t>(name, function);
    }
};

/**
 *  \brief Lua behaviour
 *
 *  This module provides Lua support.
 */
struct LuaUse {
    /// Factory for per-function runtime data
    template <typename F, typename FTraits, typename Policies_t>
    static lua::detail::FunctionCaller *perFunction(IdRef name, F function) {
        return new lua::detail::FunctionCallerImpl<F, FTraits, Policies_t>(name, function);
    }
};

/**
 * \brief Global information on the compile-time type Uses.
 *
 *  - This can be extended for other modular uses
 */
struct Uses {
    enum {
        eRuntimeModule,            ///< Runtime module enumeration
        META_IF_LUA(eLuaModule, )  ///< Lua module enumeration
        eUseCount
    };

    /// Metadata uses we are using.
    typedef std::tuple<RuntimeUse META_IF_LUA(, LuaUse)> Users;

    /// Type that stores the per-function uses data
    typedef std::tuple<runtime::detail::FunctionCaller * META_IF_LUA(, lua::detail::FunctionCaller *)> PerFunctionUserData;

    // Access note:
    //  typedef typename std::tuple_element<I, PerFunctionUserData>::type PerFunc_t;
    //  PerFunc_t* std::get<I>(getUsesData());
};

static_assert(Uses::eUseCount == std::tuple_size<Uses::Users>::value, "Size mismatch");
static_assert(Uses::eUseCount == std::tuple_size<Uses::PerFunctionUserData>::value, "Size mismatch");

}  // namespace uses
}  // namespace Meta

#define META__LUA_METATBLS "_cpp_reflection_meta"
#define META__LUA_INSTTBLS "_instmt"

namespace Meta {
namespace lua {
namespace detail {

// push a Meta value onto the Lua state stack
static int pushValue(lua_State *L, const Meta::Value &val, policy::ReturnKind retPolicy = policy::ReturnKind::Copy) {
    switch (val.kind()) {
        case ValueKind::Boolean:
            lua_pushboolean(L, val.to<bool>());
            return 1;

        case ValueKind::Integer:
            lua_pushinteger(L, val.to<lua_Integer>());
            return 1;

        case ValueKind::Real:
            lua_pushnumber(L, val.to<lua_Number>());
            return 1;

        case ValueKind::String:
            lua_pushstring(L, val.to<std::string>().c_str());
            return 1;

        case ValueKind::Enum:
            lua_pushinteger(L, val.to<int>());
            return 1;

        case ValueKind::User:
            return pushUserObject(L, retPolicy == policy::ReturnKind::InternalRef ? val.cref<UserObject>() : val.to<UserObject>());
        default:
            luaL_error(L, "Unknown type in Meta value");
    }
    return 0;
}

// get a Lua stack value as a Meta value
static Value getValue(lua_State *L, int index, ValueKind typeExpected = ValueKind::None, int argIndex = -1) {
    if (index > lua_gettop(L)) return Value();

    const int typei = lua_type(L, index);

    // if we expect a type then override Lua type to force an error if incorrect
    if (typeExpected != ValueKind::None) {
        int vtype = LUA_TNIL;
        switch (typeExpected) {
            case ValueKind::Boolean:
                vtype = LUA_TBOOLEAN;
                break;

            case ValueKind::String:
                vtype = LUA_TSTRING;
                break;

            case ValueKind::Real:
            case ValueKind::Integer:
                vtype = LUA_TNUMBER;
                break;

            case ValueKind::User:
                vtype = LUA_TUSERDATA;
                break;

            default:;
        }

        if (vtype != typei) {
            if (argIndex < 0)
                luaL_error(L, "Expecting %s but got %s", lua_typename(L, vtype), lua_typename(L, typei));
            else
                luaL_error(L, "Argument %d: expecting %s but got %s", argIndex, lua_typename(L, vtype), lua_typename(L, typei));
        }
    }

    switch (typei) {
        case LUA_TNIL:
            return Value();

        case LUA_TBOOLEAN:
            return Value(lua_toboolean(L, index));

        case LUA_TNUMBER:
            return Value(lua_tonumber(L, index));

        case LUA_TSTRING:
            return Value(lua_tostring(L, index));

        case LUA_TUSERDATA: {
            void *ud = lua_touserdata(L, index);
            Meta::UserObject *uobj = (Meta::UserObject *)ud;
            return *uobj;
        }

        default:
            luaL_error(L, "Cannot convert %s to Meta value", lua_typename(L, typei));
    }

    return Value();  // no value
}

// obj[key]
static int l_inst_index(lua_State *L) {
    lua_pushvalue(L, lua_upvalueindex(1));
    const Class *cls = (const Class *)lua_touserdata(L, -1);

    void *ud = lua_touserdata(L, 1);  // userobj - (obj, key) -> obj[key]
    const IdRef key(lua_tostring(L, 2));

    // check if getting property value
    const Property *pp = nullptr;
    if (cls->tryProperty(key, pp)) {
        Meta::UserObject *uobj = (Meta::UserObject *)ud;
        return pushValue(L, pp->get(*uobj));
    }

    // check if calling function object
    const Function *fp = nullptr;
    if (cls->tryFunction(key, fp)) {
        lua::detail::FunctionCaller *caller = std::get<uses::Uses::eLuaModule>(*reinterpret_cast<const uses::Uses::PerFunctionUserData *>(fp->getUsesData()));

        caller->pushFunction(L);
        return 1;
    }

    return 0;
}

// obj[key] = value
static int l_inst_newindex(lua_State *L)  // (obj, key, value) obj[key] = value
{
    lua_pushvalue(L, lua_upvalueindex(1));
    const Class *cls = (const Class *)lua_touserdata(L, -1);

    void *ud = lua_touserdata(L, 1);  // userobj
    const std::string key(lua_tostring(L, 2));

    // check if assigning to a property
    const Property *pp = nullptr;
    if (cls->tryProperty(key, pp)) {
        Meta::UserObject *uobj = (Meta::UserObject *)ud;
        pp->set(*uobj, getValue(L, 3, pp->kind()));
    }

    return 0;
}

static int l_instance_create(lua_State *L) {
    // get Class* from class object
    const Meta::Class *cls = *(const Meta::Class **)lua_touserdata(L, 1);

    Meta::Args args;
    constexpr int c_argOffset = 2;  // 1st arg is userdata object
    const int nargs = lua_gettop(L) - (c_argOffset - 1);
    for (int i = c_argOffset; i < c_argOffset + nargs; ++i) {
        // there may be multiple constructors so don't check types
        args += getValue(L, i);
    }

    Meta::runtime::ObjectFactory fact(*cls);
    Meta::UserObject obj(fact.construct(args));
    if (obj == Meta::UserObject::nothing) {
        lua_pop(L, 1);  // pop new user data
        luaL_error(L, "Matching constructor not found");
    }

    void *ud = lua_newuserdata(L, sizeof(UserObject));  // +1
    new (ud) UserObject(obj);

    // set instance metatable
    lua_getmetatable(L, 1);                  // +1
    lua_pushliteral(L, META__LUA_INSTTBLS);  // +1
    lua_rawget(L, -2);                       // -1+1 -> mt
    lua_setmetatable(L, -3);                 // -1
    lua_pop(L, 1);

    return 1;
}

static int l_finalise(lua_State *L) {
    Meta::UserObject *const uo = (Meta::UserObject *)lua_touserdata(L, 1);  // userobj
    *uo = UserObject::nothing;

    return 0;
}

// Type.__index(key)
static int l_get_class_static(lua_State *L) {
    // get Class* from class object
    const Meta::Class *cls = *(const Meta::Class **)lua_touserdata(L, 1);

    const IdRef key(lua_tostring(L, 2));

    const Function *func = nullptr;
    if (cls->tryFunction(key, func)) {
        lua::detail::FunctionCaller *caller = std::get<uses::Uses::eLuaModule>(*reinterpret_cast<const uses::Uses::PerFunctionUserData *>(func->getUsesData()));

        caller->pushFunction(L);
        return 1;
    }

    return 0;  // not found
}

//
// Create instance metatable. This is shared between all instances of the class type
//
static void createInstanceMetatable(lua_State *L, const Class &cls) {
    lua_createtable(L, 0, 3);  // +1 mt

    lua_pushliteral(L, "__index");           // +1
    lua_pushlightuserdata(L, (void *)&cls);  // +1
    lua_pushcclosure(L, l_inst_index, 1);    // 0 +-
    lua_rawset(L, -3);                       // -2

    lua_pushliteral(L, "__newindex");         // +1
    lua_pushlightuserdata(L, (void *)&cls);   // +1
    lua_pushcclosure(L, l_inst_newindex, 1);  // 0 +-
    lua_rawset(L, -3);                        // -2

    lua_pushliteral(L, "__gc");        // +1
    lua_pushcfunction(L, l_finalise);  // 0 +-
    lua_rawset(L, -3);                 // -2

    lua_pushglobaltable(L);                  // +1
    lua_pushliteral(L, META__LUA_METATBLS);  // +1
    lua_rawget(L, -2);                       // 0 -+
    lua_pushstring(L, cls.name().data());    // +1 k
    lua_pushvalue(L, -4);                    // +1 v
    lua_rawset(L, -3);                       // -2 _G.METAS.CLSNAME = META
    lua_pop(L, 2);                           // -1 _G, _G.METAS
}

}  // namespace detail

void expose(lua_State *L, const Class &cls, const IdRef name) {
    using namespace detail;

    // ensure _G.META
    lua_pushglobaltable(L);                  // +1
    lua_pushliteral(L, META__LUA_METATBLS);  // +1
    lua_rawget(L, -2);                       // 0 -+
    if (lua_isnil(L, -1)) {
        // first time
        lua_pop(L, 1);                           // -1 pop nil
        lua_pushliteral(L, META__LUA_METATBLS);  // +1
        lua_createtable(L, 0, 20);               // +1
        lua_rawset(L, -3);                       // -2 _G[META] = {}
    }
    lua_pop(L, 1);  // -1 _G

    // class metatable, used to customise the typename
    lua_createtable(L, 0, 20);  // +1 metatable
    const int clsmt = lua_gettop(L);

    lua_pushliteral(L, "__call");             // +1 k
    lua_pushcfunction(L, l_instance_create);  // +1 v
    lua_rawset(L, -3);                        // -2 meta.__call = constructor_fn

    lua_pushliteral(L, "__index");             // +1 k
    lua_pushcfunction(L, l_get_class_static);  // +1 v
    lua_rawset(L, -3);                         // -2 meta.__index = get_class_statics

    // create instance metatable. store ref in the class metatable
    lua_pushliteral(L, META__LUA_INSTTBLS);  // +1 k
    createInstanceMetatable(L, cls);         // +1
    lua_rawset(L, -3);                       // -2 meta._inst_ = inst_mt

    // Want in Lua: ClassName(args) -> new instance
    lua_pushglobaltable(L);              // +1 global table
    lua_pushstring(L, id::c_str(name));  // +1 k

    // class proxy
    void *ud = lua_newuserdata(L, sizeof(Meta::Class *));  // +1 v
    *(const Meta::Class **)ud = &cls;
    lua_pushvalue(L, clsmt);  // +1 metatable
    lua_setmetatable(L, -2);  // -1

    lua_rawset(L, -3);  // -2 _G[CLASSNAME] = class_obj
    lua_pop(L, 2);      // -2 global,meta
}

void expose(lua_State *L, const Enum &enm, const IdRef name) {
    const auto nb = enm.size();
    lua_createtable(L, 0, (int)nb);
    for (size_t i = 0; i < nb; ++i) {
        auto const &p = enm.pair(i);
        lua_pushstring(L, id::c_str(p.name));
        lua_pushinteger(L, p.value);
        lua_settable(L, -3);
    }
    lua_setglobal(L, id::c_str(name));
}

int pushUserObject(lua_State *L, const UserObject &uobj) {
    Class const &cls = uobj.getClass();
    void *ud = lua_newuserdata(L, sizeof(UserObject));  // +1
    new (ud) UserObject(uobj);

    // set instance metatable
    lua_pushglobaltable(L);                  // +1   _G
    lua_pushliteral(L, META__LUA_METATBLS);  // +1
    lua_rawget(L, -2);                       // 0 -+ _G.META
    lua_pushstring(L, cls.name().data());    // +1
    lua_rawget(L, -2);                       // 0 -+ _G_META.MT
    lua_setmetatable(L, -4);                 // -1
    lua_pop(L, 2);
    return 1;
}

bool runString(lua_State *L, const char *luaCode) {
    const int ret = luaL_dostring(L, luaCode);
    if (ret == LUA_OK) return true;

    std::printf("Error: %s\n", lua_tostring(L, -1));
    return false;  // failed
}

}  // namespace lua
}  // namespace Meta

#endif  // META_USES_LUA_HPP
