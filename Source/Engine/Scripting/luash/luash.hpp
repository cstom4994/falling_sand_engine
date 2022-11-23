
#ifndef _METADOT_LUASH_HPP_
#define _METADOT_LUASH_HPP_

#include "Engine/Meta/Refl.hpp"
#include "Libs/lua/lua.hpp"
#include "Libs/nameof.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

template<class T>
struct TypeIdentity : std::enable_if<true, T>
{
};
template<class T>
struct type_identity : TypeIdentity<T>
{
};

template<std::size_t N>
struct PriorityTag : PriorityTag<N - 1>
{
};
template<>
struct PriorityTag<0>
{
};


template<class T, class = void>
struct IsCompleteType : std::false_type
{
};
template<class T>
struct IsCompleteType<T, decltype(sizeof(T) > 0, void())> : std::true_type
{
};

typedef struct lua_State lua_State;

namespace luash {
    int OpenLibs(lua_State *L);

    template<class T>
    auto Push(lua_State *L, T &&);
    template<class T>
    auto Get(lua_State *L, int N, T &out);

    template<class T>
    void RegisterGlobal(lua_State *L, const char *name, T &&what);
    template<class T>
    void SetupRefMetaTable(lua_State *L, T *ptr);

    class ITypeInterface;
    void SetupRefTypeInterface(lua_State *L, void *ptr, const ITypeInterface *ti);
    void ReleaseRefTypeInterface(lua_State *L, void *ptr);

    // LuaObject at -1, pops it
    int LinkPtrToLuaObject(lua_State *L, void *Ptr);
    int PushLuaObjectByPtr(lua_State *L, void *Ptr);
    int RemoveLuaObject(lua_State *L, void *Ptr);

    template<class T, class = void>
    struct ClassTraits;// ThisClass, BaseClass, Members
}// namespace luash


namespace luash {
    int RequireMeta(lua_State *L);

    int CallOnNamedStructCreate(lua_State *L, std::string_view struct_name);

    template<class T>
    int CallOnStructCreate(lua_State *L) {
        // #1 = target lua struct
        auto struct_name = nameof::nameof_short_type<T>();
        return CallOnNamedStructCreate(L, struct_name);
    }
}// namespace luash


namespace luash {
    template<class T, class = void>
    struct IsSupportedClass : std::false_type
    {
    };
    template<class T>
    struct IsSupportedClass<T, std::void_t<typename ClassTraits<T>::ThisClass, typename ClassTraits<T>::BaseClass, typename ClassTraits<T>::PrivateData>> : std::true_type
    {
    };

    template<class T>
    struct GetAmountOfPrivateDataFields : std::integral_constant<std::size_t,
                                                                 MetaEngine::StructMemberCount<typename ClassTraits<T>::PrivateData>::value + GetAmountOfPrivateDataFields<typename ClassTraits<T>::BaseClass>::value>
    {
    };
    template<>
    struct GetAmountOfPrivateDataFields<void> : std::integral_constant<std::size_t, 0>
    {
    };

    template<std::size_t N, class TT>
    decltype(auto) GetStructMember(TT &&x) {
        using T = typename std::decay<TT>::type;
        using BaseClass = typename ClassTraits<T>::BaseClass;
        using PrivateData = typename ClassTraits<T>::PrivateData;
        if constexpr (N >= GetAmountOfPrivateDataFields<typename ClassTraits<T>::BaseClass>::value) {
            return StructApply(static_cast<typename std::conditional<std::is_const<TT>::value, const PrivateData &, PrivateData &>::type>(x), [](auto &&...args) -> decltype(auto) { return std::get<N - GetAmountOfPrivateDataFields<BaseClass>::value>(std::tie(args...)); });
        } else {
            return GetStructMember<N>(static_cast<typename std::conditional<std::is_const<TT>::value, const BaseClass &, BaseClass &>::type>(x));
        }
    }
}// namespace luash

namespace luash {
    class ITypeInterface {
    public:
        // constexpr variable don't need virtual destructor
        //virtual ~ITypeInterface() = 0;

        virtual std::string_view GetTypeName() const = 0;
        virtual std::type_index GetTypeID() const = 0;
        virtual bool IsConst() const = 0;
        virtual size_t GetMemberCount() const = 0;
        virtual int Copy(lua_State *L) const = 0;
        virtual int Assign(lua_State *L) const = 0;
        virtual int NewCopy(lua_State *L) const = 0;
        virtual int NewScalar(lua_State *L) const = 0;
        virtual int NewVector(lua_State *L) const = 0;
        virtual int DeleteScalar(lua_State *L) const = 0;
        virtual int DeleteVector(lua_State *L) const = 0;
        virtual int MetaIndex(lua_State *L) const = 0;
        virtual int MetaNewIndex(lua_State *L) const = 0;
        virtual int MetaCall(lua_State *L) const = 0;
        virtual int MetaToString(lua_State *L) const = 0;
        /*
		virtual int MetaCall(lua_State* L) = 0;
		virtual int MetaAdd(lua_State* L) = 0;
		virtual int MetaSub(lua_State* L) = 0;
		virtual int MetaMul(lua_State* L) = 0;
		virtual int MetaDiv(lua_State* L) = 0;
		virtual int MetaMod(lua_State* L) = 0;
		virtual int MetaUnm(lua_State* L) = 0;
		virtual int MetaConcat(lua_State* L) = 0;
		virtual int MetaEq(lua_State* L) = 0;
		virtual int MetaLt(lua_State* L) = 0;
		virtual int MetaLe(lua_State* L) = 0;
		*/
    };

    template<class T, class Base>
    class TypeInterfaceDecoratorCommon : public Base {
    public:
        std::string_view GetTypeName() const override {
            return nameof::nameof_type<T>();
        }
        bool IsConst() const override {
            return std::is_const<T>::value;
        }
    };
    namespace detail {
        // hackme : ref type as pointer when pushed by __index to support a.b.c=2
        template<class T>
        auto MetaIndexImpl2(lua_State *L, T &&arg) {
            if constexpr (std::disjunction<typename std::negation<IsCompleteType<typename std::remove_cv<T>::type>>::type, std::is_aggregate<typename std::remove_cv<T>::type>, IsSupportedClass<typename std::remove_cv<T>::type>>::value)
                Push(L, &arg);
            else
                Push(L, arg);
            return true;
        }
        template<std::size_t... I, class... Args>
        bool MetaIndexImpl(lua_State *L, int N, std::index_sequence<I...>, Args &&...args) {
            return (... || ((I == N - 1) ? MetaIndexImpl2(L, std::forward<Args>(args)) : false));
        }
        template<class T>
        bool MetaNewIndexImpl2(lua_State *L, int N, T &arg) {
            Get(L, 3, arg);
            return true;
        }
        template<class T>
        bool MetaNewIndexImpl2(lua_State *L, int N, const T &arg) {
            // cannot set const value
            return false;
        }
        template<std::size_t... I, class... Args>
        bool MetaNewIndexImpl(lua_State *L, int N, std::index_sequence<I...>, Args &...args) {
            return (... || ((I == N - 1) ? MetaNewIndexImpl2(L, N, args) : false));
        }
    }// namespace detail
    template<class T, class Base, bool = IsCompleteType<typename std::remove_cv<T>::type>::value>
    class TypeInterfaceDecoratorComplete : public Base {
    public:
        std::type_index GetTypeID() const override {
            return typeid(void);
        }
        int NewScalar(lua_State *L) const override {
            std::string type(this->GetTypeName());
            return luaL_error(L, "can not new incomplete ref type %s", type.c_str());
        }
        int NewVector(lua_State *L) const override {
            std::string type(this->GetTypeName());
            return luaL_error(L, "can not new incomplete ref type %s", type.c_str());
        }
        int DeleteScalar(lua_State *L) const override {
            std::string type(this->GetTypeName());
            return luaL_error(L, "can not delete incomplete ref type %s", type.c_str());
        }
        int DeleteVector(lua_State *L) const override {
            std::string type(this->GetTypeName());
            return luaL_error(L, "can not delete incomplete ref type %s", type.c_str());
        }
    };
    template<class T, class Base>
    class TypeInterfaceDecoratorComplete<T, Base, true> : public Base {
    public:
        std::type_index GetTypeID() const override {
            return typeid(typename std::remove_cv<T>::type);
        }
        int NewScalar(lua_State *L) const override {
            auto new_object = new T();
            Push(L, new_object);
            return 1;
        }
        int NewVector(lua_State *L) const override {
            auto len = lua_tointeger(L, 1);
            if (len < 0) {
                return luaL_error(L, "invalid new[] len %d", (int) len);
            }
            auto new_object = new T[len]{};
            Push(L, new_object);
            return 1;
        }
        int DeleteScalar(lua_State *L) const override {
            auto value = lua_touserdata(L, 1);
            auto object = reinterpret_cast<T *>(value);
            delete object;
            return 0;
        }
        int DeleteVector(lua_State *L) const override {
            auto value = lua_touserdata(L, 1);
            auto object = reinterpret_cast<T *>(value);
            delete[] object;
            return 0;
        }
    };

    template<class T, class Base, bool = std::conjunction<IsCompleteType<typename std::remove_cv<T>::type>, std::negation<IsSupportedClass<typename std::remove_cv<T>::type>>>::value>
    class TypeInterfaceDecoratorCopy : public Base {
    public:
        int Copy(lua_State *L) const override {
            std::string type(this->GetTypeName());
            return luaL_error(L, "can not copy unsupported ref type %s", type.c_str());
        }
        int Assign(lua_State *L) const override {
            std::string type(this->GetTypeName());
            return luaL_error(L, "can not assign unsupported ref type %s", type.c_str());
        }
        int NewCopy(lua_State *L) const override {
            std::string type(this->GetTypeName());
            return luaL_error(L, "can not new unsupported ref type %s", type.c_str());
        }
    };

    template<class T, class Base>
    class TypeInterfaceDecoratorCopy<T, Base, true> : public Base {
    public:
        int Copy(lua_State *L) const override {
            auto value = lua_touserdata(L, 1);
            auto object = reinterpret_cast<T *>(value);
            Push(L, *object);
            return 1;
        }
        int NewCopy(lua_State *L) const override {
            auto value = lua_touserdata(L, 1);
            auto object = reinterpret_cast<T *>(value);
            auto new_object = new auto(*object);
            Push(L, new_object);
            return 1;
        }
        int Assign(lua_State *L) const override {
            if constexpr (!std::is_copy_assignable_v<T>) {
                std::string type(this->GetTypeName());
                return luaL_error(L, "can not assign const ref type %s", type.c_str());
            } else {
                auto value = lua_touserdata(L, 1);
                auto object = reinterpret_cast<T *>(value);
                Get(L, 2, *object);
                return 1;
            }
        }
    };

    template<class T, class Base>
    class TypeInterfaceDecoratorToString : public Base {
        int MetaToString(lua_State *L) const override {
            auto value = lua_touserdata(L, 1);
            auto object = reinterpret_cast<T *>(value);
            std::string type(this->GetTypeName());
            lua_pushfstring(L, "(%s *)0x%x", type.c_str(), object);
            return 1;
        }
    };
    template<class Base>
    class TypeInterfaceDecoratorToString<const char, Base> : public Base {
        int MetaToString(lua_State *L) const override {
            auto value = lua_touserdata(L, 1);
            const char *object = static_cast<const char *>(value);
            lua_pushstring(L, object);
            return 1;
        }
    };
    template<class Base>
    class TypeInterfaceDecoratorToString<char, Base> : public TypeInterfaceDecoratorToString<const char, Base> {};

    template<class T, class Base,
             bool = std::conjunction<IsCompleteType<typename std::remove_cv<T>::type>, std::is_aggregate<typename std::remove_cv<T>::type>>::value,
             bool = std::conjunction<IsCompleteType<typename std::remove_cv<T>::type>, IsSupportedClass<typename std::remove_cv<T>::type>>::value>
    class TypeInterfaceDecoratorIndex : public Base {
    public:
        size_t GetMemberCount() const override {
            return 0;
        }
        int MetaIndex(lua_State *L) const override {
            if (lua_type(L, 2) == LUA_TSTRING) {
                if (!strcmp(lua_tostring(L, 2), "self"))
                    return this->Copy(L);
                return 0;
            }
            return 0;
        }
        int MetaNewIndex(lua_State *L) const override {
            if (lua_type(L, 2) == LUA_TSTRING) {
                if (!strcmp(lua_tostring(L, 2), "self"))
                    return this->Assign(L);
                return 0;
            }
            return 0;
        }
    };

    namespace detail {
        void TypeInterfaceStringIndexReplace(const ITypeInterface *ti, lua_State *L, std::string_view PathPrefix, std::string_view name);
    }

    template<class T, class Base>
    class TypeInterfaceDecoratorIndex<T, Base, true, false> : public Base {
    public:
        size_t GetMemberCount() const override {
            return StructMemberCount<T>();
        }
        int MetaIndex(lua_State *L) const override {
            if (lua_type(L, 2) == LUA_TSTRING) {
                if (!strcmp(lua_tostring(L, 2), "self"))
                    return this->Copy(L);
                detail::TypeInterfaceStringIndexReplace(this, L, "struct/", nameof::nameof_short_type<typename std::decay<T>::type>());
            }
            void *value = lua_touserdata(L, 1);
            int N = lua_tointeger(L, 2);
            T *object = reinterpret_cast<T *>(value);
            bool success = StructApply(*object, [L, N](auto &&...args) {
                return detail::MetaIndexImpl(L, N, std::make_index_sequence<sizeof...(args)>(), std::forward<decltype(args)>(args)...);
            });
            return success ? 1 : 0;
        }
        int MetaNewIndex(lua_State *L) const override {

            if (lua_type(L, 2) == LUA_TSTRING) {
                if (!strcmp(lua_tostring(L, 2), "self"))
                    return this->Assign(L);
                detail::TypeInterfaceStringIndexReplace(this, L, "struct/", nameof::nameof_short_type<typename std::decay<T>::type>());
            }
            void *value = lua_touserdata(L, 1);
            int N = lua_tointeger(L, 2);
            T *object = reinterpret_cast<T *>(value);
            bool success = StructApply(*object, [L, N](auto &&...args) {
                return detail::MetaNewIndexImpl(L, N, std::make_index_sequence<sizeof...(args)>(), std::forward<decltype(args)>(args)...);
            });
            if (!success) {
                std::string type(this->GetTypeName());
                return luaL_error(L, "can not assign field %s[%d]", type.c_str(), N);
            }
            return success;
        }
    };


    template<class T, class Base>
    class TypeInterfaceDecoratorIndex<T, Base, false, true> : public Base {
    public:
        static constexpr auto MemberCount = GetAmountOfPrivateDataFields<typename std::remove_const<T>::type>::value;
        size_t GetMemberCount() const override {
            return MemberCount;
        }
        template<std::size_t... I>
        int MetaIndexImpl(lua_State *L, T &objref, std::size_t N, std::index_sequence<I...>) const {
            static constexpr void (*meta_switch[sizeof...(I)])(lua_State * L, T & objref) = {
                    +[](lua_State *L, T &objref) { detail::MetaIndexImpl2(L, GetStructMember<I>(objref)); }...};
            if (N < 1 || N > sizeof...(I))
                return 0;
            meta_switch[N - 1](L, objref);
            return 1;
        }
        template<std::size_t... I>
        int MetaNewIndexImpl(lua_State *L, T &objref, std::size_t N, std::index_sequence<I...>) const {
            static constexpr void (*meta_switch[sizeof...(I)])(lua_State * L, T & objref) = {
                    +[](lua_State *L, T &objref) { Get(L, -1, GetStructMember<I>(objref)); }...};
            if (N < 1 || N > sizeof...(I))
                return 0;
            meta_switch[N - 1](L, objref);
            return 1;
        }
        int MetaIndex(lua_State *L) const override {
            if (lua_type(L, 2) == LUA_TSTRING) {
                if (!strcmp(lua_tostring(L, 2), "self"))
                    return this->Copy(L);
            }
            detail::TypeInterfaceStringIndexReplace(this, L, "class/", nameof::nameof_short_type<typename std::decay<T>::type>());
            void *value = lua_touserdata(L, 1);
            int N = lua_tointeger(L, 2);
            if (T *object = reinterpret_cast<T *>(value))
                return MetaIndexImpl(L, *object, N, std::make_index_sequence<MemberCount>());
            return 0;
        }
        int MetaNewIndex(lua_State *L) const override {
            if (lua_type(L, 2) == LUA_TSTRING) {
                if (!strcmp(lua_tostring(L, 2), "self"))
                    return this->Assign(L);
            }
            detail::TypeInterfaceStringIndexReplace(this, L, "class/", nameof::nameof_short_type<typename std::decay<T>::type>());
            void *value = lua_touserdata(L, 1);
            int N = lua_tointeger(L, 2);
            T *object = reinterpret_cast<T *>(value);
            if (T *object = reinterpret_cast<T *>(value))
                return MetaNewIndexImpl(L, *object, N, std::make_index_sequence<MemberCount>());
            return 0;
        }
    };

    namespace detail {
        template<class T, class = void>
        struct IsFunctionType : std::false_type
        {
        };
        template<class T>
        struct IsFunctionType<T, std::void_t<decltype(std::function(std::declval<T>()))>> : std::true_type
        {
        };
    }// namespace detail

    template<class T, class Base, bool = detail::IsFunctionType<T>::value>
    class TypeInterfaceDecoratorCall : public Base {
        int MetaCall(lua_State *L) const override {
            std::string type(this->GetTypeName());
            luaL_error(L, "attempt to call a non function ref type %s", type.c_str());
            return 0;
        }
    };

    template<class T>
    inline bool PushIfLvRef(lua_State *L, T &obj, std::true_type) {
        Push(L, obj);
        return true;
    }
    template<class T>
    constexpr bool PushIfLvRef(lua_State *L, T &&obj, std::false_type) {
        return false;
    }

    template<class T, class Base>
    class TypeInterfaceDecoratorCall<T, Base, true> : public Base {
        template<std::size_t... I, class Ret, class... Args>
        int MetaCallImpl2(lua_State *L, std::index_sequence<I...>, TypeIdentity<std::function<Ret(Args...)>>) const {
            if (int argn = lua_gettop(L); argn - 1 < (int) sizeof...(I)) {
                luaL_error(L, "bad function call with unmatched args, excepted %d got %d", static_cast<int>(sizeof...(I)), argn - 1);
                return 0;
            }

            void *value = lua_touserdata(L, 1);
            T *pfn = reinterpret_cast<T *>(value);

            // dont support lvalue reference
            //static_assert((... && !(std::is_lvalue_reference<Args>::value && !std::is_const<typename std::remove_reference<Args>::type>::value)));

            std::tuple<typename std::remove_const<typename std::remove_reference<Args>::type>::type...> args;
            (..., Get(L, I + 2, std::get<I>(args)));
            if constexpr (std::is_void_v<Ret>) {
                (*pfn)(std::get<I>(args)...);
            } else {
                Ret ret = (*pfn)(std::get<I>(args)...);
                Push(L, std::move(ret));
            }

            return (!std::is_void_v<Ret> + ... + PushIfLvRef(L, std::get<I>(args), std::bool_constant<(std::is_lvalue_reference<Args>::value && !std::is_const<typename std::remove_reference<Args>::type>::value)>()));
        }

        template<std::size_t... I, class Ret, class... Args>
        int MetaCallImpl(lua_State *L, TypeIdentity<std::function<Ret(Args...)>> tag) const {
            return MetaCallImpl2(L, std::index_sequence_for<Args...>(), tag);
        }

        int MetaCall(lua_State *L) const override {
            return MetaCallImpl(L, TypeIdentity<decltype(std::function(std::declval<T>()))>());
        }
    };

    template<class T>
    class TypeInterface : public TypeInterfaceDecoratorCommon<T,
                                                              TypeInterfaceDecoratorIndex<T,
                                                                                          TypeInterfaceDecoratorComplete<T,
                                                                                                                         TypeInterfaceDecoratorCopy<T,
                                                                                                                                                    TypeInterfaceDecoratorCall<T,
                                                                                                                                                                               TypeInterfaceDecoratorToString<T,
                                                                                                                                                                                                              ITypeInterface>>>>>> {
    public:
        TypeInterface();
    };

    template<class T>
    inline TypeInterface<T> TypeInterfaceSingleton;

    std::unordered_map<std::string_view, const ITypeInterface *> &KnownTypeInterfaceMap();

    template<class T>
    inline TypeInterface<T>::TypeInterface() {
        KnownTypeInterfaceMap().emplace(this->GetTypeName(), this);
    }

    template<class T>
    const ITypeInterface *CreateTypeInterface() {
        const ITypeInterface *ti = &TypeInterfaceSingleton<T>;
        return ti;
    }

    const ITypeInterface *GetTypeInterfaceTop(lua_State *L);

    const ITypeInterface *GetTypeInterface(lua_State *L, void *ptr);

    // function(value, i)
    template<int (ITypeInterface::*pmemfn)(lua_State *) const>
    int RefValueMetaDispatch(lua_State *L) {
        void *value = lua_touserdata(L, 1);
        if (const ITypeInterface *ti = GetTypeInterface(L, value))
            return (ti->*pmemfn)(L);
        luaL_error(L, "unrecognized ref ptr %x", value);
        return 0;
    }

    int SetupTypeInterfaceTable(lua_State *L);
    int SetupBuiltinTypeInterface(lua_State *L);
    int SetupPtrMetaTable(lua_State *L);
    int SetupCppNewDelete(lua_State *L);
}// namespace luash


namespace luash {
    struct PushLambda
    {
        template<class T>
        auto operator()(T &&in) { return Push(L, std::forward<T>(in)); }
        lua_State *const L;
    };
    template<class T>
    auto PushBoolean(lua_State *L, T x) -> typename std::enable_if<std::is_same<T, bool>::value>::type {
        lua_pushboolean(L, x);
    }
    template<class T>
    auto PushInteger(lua_State *L, T x) -> typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value>::type {
        lua_pushinteger(L, x);
    }
    template<class T>
    auto PushFloat(lua_State *L, T x) -> typename std::enable_if<std::is_floating_point<T>::value>::type {
        lua_pushnumber(L, x);
    }
    template<class T, class Ratio>
    auto PushFloat(lua_State *L, std::chrono::duration<T, Ratio> x) -> typename std::enable_if<std::is_floating_point<T>::value>::type {
        lua_pushnumber(L, std::chrono::duration_cast<std::chrono::duration<T, std::ratio<1>>>(x).count());
    }
    template<class Clock, class Duration>
    auto PushFloat(lua_State *L, const std::chrono::time_point<Clock, Duration> &x) -> typename std::enable_if<std::is_floating_point<typename Duration::rep>::value>::type {
        PushFloat(L, x.time_since_epoch());
    }
    template<class T>
    auto PushEnum(lua_State *L, T x) -> typename std::enable_if<std::is_enum<T>::value>::type {
        return Push(L, static_cast<typename std::underlying_type<T>::type>(x));
    }
    inline void PushString(lua_State *L, char *str);
    inline void PushString(lua_State *L, const char *str);
    inline void PushString(lua_State *L, const std::string &str) {
        lua_pushlstring(L, str.c_str(), str.size());
    }
    inline void PushString(lua_State *L, std::string_view str) {
        lua_pushlstring(L, str.data(), str.size());
    }
    inline void PushNil(lua_State *L, std::nullptr_t = nullptr) {
        lua_pushboolean(L, false);
    }
    using std::begin;
    using std::end;
    template<class T>
    auto PushArray(lua_State *L, T &&v) -> decltype(begin(std::forward<T>(v)), end(std::forward<T>(v)), void()) {
        lua_newtable(L);
        int i = 0;
        for (auto &&x: std::forward<T>(v)) {
            lua_pushinteger(L, ++i);
            Push(L, std::forward<decltype(x)>(x));
            lua_settable(L, -3);
        }
    }
    namespace detail {
        template<std::size_t... I, class... Args>
        void PushStructImpl(lua_State *L, std::index_sequence<I...>, Args &&...args) {
            (lua_newtable(L), ..., (lua_pushinteger(L, I + 1), Push(L, std::forward<Args>(args)), lua_settable(L, -3)));
        }
    }// namespace detail
    template<class T>
    auto PushStruct(lua_State *L, T &&v) -> typename std::enable_if<std::is_aggregate<typename std::decay<T>::type>::value>::type {
        StructApply(v, [L](auto &&...args) {
            detail::PushStructImpl(L, std::make_index_sequence<sizeof...(args)>(), std::forward<decltype(args)>(args)...);
        });
        CallOnStructCreate<typename std::decay<T>::type>(L);
    }
    int PushLuaObjectByPtr(lua_State *L, void *Ptr);
    template<class T>
    void PushPointer(lua_State *L, T *ptr) {
        if (!ptr)
            return lua_pushboolean(L, false);
        if (!PushLuaObjectByPtr(L, (void *) ptr)) {
            lua_pushlightuserdata(L, (void *) ptr);// #1 = ptr
            SetupRefTypeInterface(L, (void *) ptr, CreateTypeInterface<T>());
        }
    }
    // just unsupported
    template<class T, class U>
    void PushMemberPointer(lua_State *L, U T::*mem_ptr) {
        return lua_pushboolean(L, false);
    }
    template<class T, class U>
    void PushMemberPointer(lua_State *L, const U T::*const_mem_ptr) {
        return lua_pushboolean(L, false);
    }
    /*
	template<class T, class R, class...A> void PushMemberPointer(lua_State* L, R (T::* mem_fn_ptr)(A...))
	{
		lua_pushlightuserdata(L, nullptr);
	}
	template<class T, class R, class...A> void PushMemberPointer(lua_State* L, R(T::* const_mem_fn_ptr)(A...) const)
	{
		lua_pushlightuserdata(L, nullptr);
	}
	*/
    inline void PushString(lua_State *L, char *ptr) {
        PushPointer(L, ptr);
    }
    inline void PushString(lua_State *L, const char *ptr) {
        lua_pushstring(L, ptr);
    }
    template<class T>
    auto PushCustom(lua_State *L, const T &x) -> decltype(x.LuaPush(L)) {
        return x.LuaPush(L);
    }

    namespace detail {
        template<class T>
        auto PushUnknownImpl(lua_State *L, T &&x, PriorityTag<10>) -> decltype(PushCustom(L, x)) {
            return PushCustom(L, x);
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T x, PriorityTag<7>) -> decltype(PushBoolean(L, x)) {
            return PushBoolean(L, x);
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T x, PriorityTag<6>) -> decltype(PushEnum(L, x)) {
            return PushEnum(L, x);
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T x, PriorityTag<5>, int = 0) -> decltype(PushInteger(L, x)) {
            return PushInteger(L, x);
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T x, PriorityTag<5>, float = 0) -> decltype(PushFloat(L, x)) {
            return PushFloat(L, x);
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T &&x, PriorityTag<4>) -> decltype(PushString(L, std::forward<T>(x))) {
            return PushString(L, std::forward<T>(x));
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T &&x, PriorityTag<3>) -> decltype(PushArray(L, std::forward<T>(x))) {
            return PushArray(L, std::forward<T>(x));
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T &&x, PriorityTag<2>) -> decltype(PushStruct(L, std::forward<T>(x))) {
            return PushStruct(L, std::forward<T>(x));
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T x, PriorityTag<1>) -> decltype(PushMemberPointer(L, x)) {
            return PushMemberPointer(L, x);
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T x, PriorityTag<0>) -> decltype(PushPointer(L, x)) {
            return PushPointer(L, x);
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T x, PriorityTag<0>, std::nullptr_t = nullptr) -> typename std::enable_if<std::is_null_pointer<T>::value>::type {
            return PushNil(L, x);
        }
        using MaxPriorityTag = PriorityTag<10>;
        template<class T>
        auto PushImpl(lua_State *L, T &&value) -> decltype(PushUnknownImpl(L, std::forward<T>(value), MaxPriorityTag())) {
            return PushUnknownImpl(L, std::forward<T>(value), MaxPriorityTag());
        }
    }// namespace detail
}// namespace luash


namespace luash {
    inline void GetBoolean(lua_State *L, int N, bool &x) {
        x = lua_toboolean(L, N);
    }
    template<class T>
    auto GetInteger(lua_State *L, int N, T &x) -> typename std::enable_if<std::is_integral<T>::value>::type {
        x = lua_tointeger(L, N);
    }
    template<class T>
    auto GetFloat(lua_State *L, int N, T &x) -> typename std::enable_if<std::is_floating_point<T>::value>::type {
        x = lua_tonumber(L, N);
    }
    template<class T, class Ratio>
    auto GetFloat(lua_State *L, int N, std::chrono::duration<T, Ratio> &x) -> typename std::enable_if<std::is_floating_point<T>::value>::type {
        T y;
        GetFloat(L, N, y);
        x = std::chrono::duration<T, std::ratio<1>>(y);
    }
    template<class Clock, class Duration>
    auto GetFloat(lua_State *L, int N, std::chrono::time_point<Clock, Duration> &x) -> typename std::enable_if<std::is_floating_point<typename Duration::rep>::value>::type {
        Duration z;
        GetFloat(L, N, z);
        x = std::chrono::time_point<Clock, Duration>() + z;
    }
    template<class T>
    auto GetEnum(lua_State *L, int N, T &x) -> typename std::enable_if<std::is_enum<T>::value>::type {
        typename std::underlying_type<T>::type y;
        GetInteger(L, N, y);
        x = static_cast<T>(y);
    }
    inline void GetString(lua_State *L, int N, const char *&sz) {
        if (void *data = lua_touserdata(L, N)) {
            sz = reinterpret_cast<const char *>(data);
        } else {
            sz = lua_tostring(L, N);
        }
    }
    inline void GetString(lua_State *L, int N, std::string &str) {
        str = lua_tostring(L, N);
    }
    inline void GetString(lua_State *L, int N, std::string_view &sv) {
        sv = lua_tostring(L, N);
    }
    inline void GetNil(lua_State *L, int N, std::nullptr_t &np) {}
    template<class T, std::size_t Size>
    auto GetArray(lua_State *L, int N, std::array<T, Size> &arr) {
        lua_pushvalue(L, N);
        // #1 = arr
        for (std::size_t i = 0; i < Size; ++i) {
            lua_pushinteger(L, i + 1);
            // #2 = i + 1
            lua_gettable(L, -2);
            // #2 = arr[i+1]
            Get(L, -1, arr[i]);
            //
            lua_pop(L, 1);
            // #1 = arr
        }
        lua_pop(L, 1);
        // #0
    }
    template<class T, std::size_t Size>
    auto GetArray(lua_State *L, int N, T (&arr)[Size]) {
        lua_pushvalue(L, N);
        // #1 = arr
        for (std::size_t i = 0; i < Size; ++i) {
            lua_pushinteger(L, i + 1);
            // #2 = i + 1
            lua_gettable(L, -2);
            // #2 = arr[i+1]
            Get(L, -1, arr[i]);
            //
            lua_pop(L, 1);
            // #1 = arr
        }
        lua_pop(L, 1);
        // #0
    }
    template<class T>
    auto GetArray(lua_State *L, int N, T &v) -> decltype(v.begin(), v.end(), v.resize(0), void()) {
        lua_pushvalue(L, N);// #1 = arr
        const auto Size = lua_rawlen(L, -1);
        v.resize(Size);
        int i = 0;
        for (auto &x: v) {
            lua_pushinteger(L, ++i);// #2 = i + 1
            lua_gettable(L, -2);    // #2 = arr[i+1]
            Get(L, -1, x);
            lua_pop(L, 1);// #1 = arr
        }
        lua_pop(L, 1);// #0
    }
    namespace detail {
        template<std::size_t... I, class... Args>
        void GetStructImpl(lua_State *L, std::index_sequence<I...>, Args &...args) {
            // #1 = struct
            (..., (
                          lua_pushinteger(L, I + 1),
                          // #2 = I+1
                          lua_gettable(L, -2),
                          // #2 = vec[I+1]
                          Get(L, -1, args),
                          //
                          lua_pop(L, 1)
                          // #1 = struct
                          ));
        }
    }// namespace detail
    template<class T>
    auto GetStruct(lua_State *L, int N, T &v) -> typename std::enable_if<std::is_aggregate<typename std::decay<T>::type>::value>::type {
        lua_pushvalue(L, N);
        // #1 = struct
        StructApply(v, [L](auto &...args) {
            detail::GetStructImpl(L, std::make_index_sequence<sizeof...(args)>(), args...);
        });
        lua_pop(L, 1);
        // #0
    }

    bool GetPtrByLuaObject(lua_State *L, int N, void *&);
    template<class T>
    inline void GetPointer(lua_State *L, int N, T *&out) {
        if (void *ptr = nullptr; GetPtrByLuaObject(L, N, ptr)) {
            out = reinterpret_cast<T *>(ptr);
            return;
        }
        void *ptr = lua_touserdata(L, N);
        if (const ITypeInterface *ti = GetTypeInterface(L, ptr)) {
            out = reinterpret_cast<T *>(ptr);
            return;
        }
        out = nullptr;
        return;
    }
    // just unsupported
    template<class C, class U>
    void GetMemberPointer(lua_State *L, int N, U C::*&mem_ptr) {
        mem_ptr = nullptr;
    }
    template<class C, class U>
    void GetMemberPointer(lua_State *L, int N, const U C::*&const_mem_ptr) {
        const_mem_ptr = nullptr;
    }
    /*
	template<class C, class R, class...A> void GetMemberPointer(lua_State* L, int N, R(C::* &mem_fn_ptr)(A...))
	{
		mem_fn_ptr = nullptr;
	}
	template<class C, class R, class...A> void GetMemberPointer(lua_State* L, int N, R(C::* &const_mem_fn_ptr)(A...) const)
	{
		const_mem_fn_ptr = nullptr;
	}
	*/
    template<class T>
    auto GetCustom(lua_State *L, int N, T &x) -> decltype(x.LuaGet(L, N)) {
        return x.LuaGet(L, N);
    }

    namespace detail {
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<10>) -> decltype(GetCustom(L, N, x)) {
            return GetCustom(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<8>) -> decltype(GetBoolean(L, N, x)) {
            return GetBoolean(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<7>) -> decltype(GetEnum(L, N, x)) {
            return GetEnum(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<6>, int = 0) -> decltype(GetInteger(L, N, x)) {
            return GetInteger(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<6>, float = 0) -> decltype(GetFloat(L, N, x)) {
            return GetFloat(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<5>) -> decltype(GetString(L, N, x)) {
            return GetString(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<4>) -> decltype(GetEntity(L, N, x)) {
            return GetEntity(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<4>) -> decltype(GetVector(L, N, x)) {
            return GetVector(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<3>) -> decltype(GetArray(L, N, x)) {
            return GetArray(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<2>) -> decltype(GetStruct(L, N, x)) {
            return GetStruct(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<1>) -> decltype(GetMemberPointer(L, N, x)) {
            return GetMemberPointer(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<0>) -> decltype(GetPointer(L, N, x)) {
            return GetPointer(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<0>, std::nullptr_t = nullptr) -> typename std::enable_if<std::is_null_pointer<T>::value>::type {
            return GetNil(L, N, x);
        }
        using MaxPriorityTag = PriorityTag<10>;
        template<class T>
        auto GetImpl(lua_State *L, int N, T &value) -> decltype(GetUnknownImpl(L, N, value, MaxPriorityTag())) {
            return GetUnknownImpl(L, N, value, MaxPriorityTag());
        }
    }// namespace detail
}// namespace luash


namespace luash {
    template<class T>
    auto Push(lua_State *L, T &&what) {
        return detail::PushImpl(L, std::forward<T>(what));
    }
    template<class T>
    auto Get(lua_State *L, int N, T &out) {
        return detail::GetImpl(L, N, out);
    }
    template<class T>
    void RegisterGlobal(lua_State *L, const char *name, T &&what) {
        Push(L, std::forward<T>(what));
        lua_setglobal(L, name);
    }
}// namespace luash

#endif