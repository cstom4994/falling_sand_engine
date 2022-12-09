// Copyright(c) 2022, KaoruXun

// Most modules refer to kaguya written by satoren,
// link to https://github.com/satoren/kaguya
// kaguya is distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _METADOT_LUAWRAPPER_HPP_
#define _METADOT_LUAWRAPPER_HPP_

#include "Core/Core.hpp"
#include "Core/Macros.hpp"
#include "Engine/CodeReflection.hpp"
#include "Libs/lua/lua.hpp"
#include "Libs/nameof.hpp"
#include "PreprocesserFlat.hpp"

#ifndef n4502
#define n4502

// See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4502.pdf.
template<typename...>
using void_t = void;

template<typename, template<typename> class, typename = void_t<>>
struct detect : std::false_type
{
};

template<typename T, template<typename> class Op>
struct detect<T, Op, void_t<Op<T>>> : std::true_type
{
};

#endif

// A portable and safe way to add byte offset to any pointer
// https://stackoverflow.com/questions/15934111/portable-and-safe-way-to-add-byte-offset-to-any-pointer
template<typename T>
inline void addOffset(std::ptrdiff_t offset, T *&ptr) {
    if (!ptr) return;
    ptr = (T *) ((unsigned char *) ptr + offset);
}

template<typename T>
inline T *addOffsetR(std::ptrdiff_t offset, T *ptr) {
    if (!ptr) return nullptr;
    return (T *) ((unsigned char *) ptr + offset);
}

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

namespace LuaStruct {
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

    int RequireMeta(lua_State *L);

    int CallOnNamedStructCreate(lua_State *L, std::string_view struct_name);

    template<class T>
    int CallOnStructCreate(lua_State *L) {
        // #1 = target lua struct
        auto struct_name = nameof::nameof_short_type<T>();
        return CallOnNamedStructCreate(L, struct_name);
    }

    template<class T, class = void>
    struct IsSupportedClass : std::false_type
    {
    };
    template<class T>
    struct IsSupportedClass<
            T, std::void_t<typename ClassTraits<T>::ThisClass, typename ClassTraits<T>::BaseClass,
                           typename ClassTraits<T>::PrivateData>> : std::true_type
    {
    };

    template<class T>
    struct GetAmountOfPrivateDataFields
        : std::integral_constant<
                  std::size_t,
                  Meta::StructMemberCount<typename ClassTraits<T>::PrivateData>::value +
                          GetAmountOfPrivateDataFields<typename ClassTraits<T>::BaseClass>::value>
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
        if constexpr (N >=
                      GetAmountOfPrivateDataFields<typename ClassTraits<T>::BaseClass>::value) {
            return StructApply(
                    static_cast<typename std::conditional<
                            std::is_const<TT>::value, const PrivateData &, PrivateData &>::type>(x),
                    [](auto &&...args) -> decltype(auto) {
                        return std::get<N - GetAmountOfPrivateDataFields<BaseClass>::value>(
                                std::tie(args...));
                    });
        } else {
            return GetStructMember<N>(
                    static_cast<typename std::conditional<std::is_const<TT>::value,
                                                          const BaseClass &, BaseClass &>::type>(
                            x));
        }
    }

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
        std::string_view GetTypeName() const override { return nameof::nameof_type<T>(); }
        bool IsConst() const override { return std::is_const<T>::value; }
    };
    namespace detail {
        // hackme : ref type as pointer when pushed by __index to support a.b.c=2
        template<class T>
        auto MetaIndexImpl2(lua_State *L, T &&arg) {
            if constexpr (std::disjunction<
                                  typename std::negation<
                                          IsCompleteType<typename std::remove_cv<T>::type>>::type,
                                  std::is_aggregate<typename std::remove_cv<T>::type>,
                                  IsSupportedClass<typename std::remove_cv<T>::type>>::value)
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
        std::type_index GetTypeID() const override { return typeid(void); }
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
            if (len < 0) { return luaL_error(L, "invalid new[] len %d", (int) len); }
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

    template<class T, class Base,
             bool = std::conjunction<
                     IsCompleteType<typename std::remove_cv<T>::type>,
                     std::negation<IsSupportedClass<typename std::remove_cv<T>::type>>>::value>
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
    class TypeInterfaceDecoratorToString<char, Base>
        : public TypeInterfaceDecoratorToString<const char, Base> {};

    template<class T, class Base,
             bool = std::conjunction<IsCompleteType<typename std::remove_cv<T>::type>,
                                     std::is_aggregate<typename std::remove_cv<T>::type>>::value,
             bool = std::conjunction<IsCompleteType<typename std::remove_cv<T>::type>,
                                     IsSupportedClass<typename std::remove_cv<T>::type>>::value>
    class TypeInterfaceDecoratorIndex : public Base {
    public:
        size_t GetMemberCount() const override { return 0; }
        int MetaIndex(lua_State *L) const override {
            if (lua_type(L, 2) == LUA_TSTRING) {
                if (!strcmp(lua_tostring(L, 2), "self")) return this->Copy(L);
                return 0;
            }
            return 0;
        }
        int MetaNewIndex(lua_State *L) const override {
            if (lua_type(L, 2) == LUA_TSTRING) {
                if (!strcmp(lua_tostring(L, 2), "self")) return this->Assign(L);
                return 0;
            }
            return 0;
        }
    };

    namespace detail {
        void TypeInterfaceStringIndexReplace(const ITypeInterface *ti, lua_State *L,
                                             std::string_view PathPrefix, std::string_view name);
    }

    template<class T, class Base>
    class TypeInterfaceDecoratorIndex<T, Base, true, false> : public Base {
    public:
        size_t GetMemberCount() const override { return StructMemberCount<T>(); }
        int MetaIndex(lua_State *L) const override {
            if (lua_type(L, 2) == LUA_TSTRING) {
                if (!strcmp(lua_tostring(L, 2), "self")) return this->Copy(L);
                detail::TypeInterfaceStringIndexReplace(
                        this, L, "struct/",
                        nameof::nameof_short_type<typename std::decay<T>::type>());
            }
            void *value = lua_touserdata(L, 1);
            int N = lua_tointeger(L, 2);
            T *object = reinterpret_cast<T *>(value);
            bool success = StructApply(*object, [L, N](auto &&...args) {
                return detail::MetaIndexImpl(L, N, std::make_index_sequence<sizeof...(args)>(),
                                             std::forward<decltype(args)>(args)...);
            });
            return success ? 1 : 0;
        }
        int MetaNewIndex(lua_State *L) const override {

            if (lua_type(L, 2) == LUA_TSTRING) {
                if (!strcmp(lua_tostring(L, 2), "self")) return this->Assign(L);
                detail::TypeInterfaceStringIndexReplace(
                        this, L, "struct/",
                        nameof::nameof_short_type<typename std::decay<T>::type>());
            }
            void *value = lua_touserdata(L, 1);
            int N = lua_tointeger(L, 2);
            T *object = reinterpret_cast<T *>(value);
            bool success = StructApply(*object, [L, N](auto &&...args) {
                return detail::MetaNewIndexImpl(L, N, std::make_index_sequence<sizeof...(args)>(),
                                                std::forward<decltype(args)>(args)...);
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
        static constexpr auto MemberCount =
                GetAmountOfPrivateDataFields<typename std::remove_const<T>::type>::value;
        size_t GetMemberCount() const override { return MemberCount; }
        template<std::size_t... I>
        int MetaIndexImpl(lua_State *L, T &objref, std::size_t N, std::index_sequence<I...>) const {
            static constexpr void (*meta_switch[sizeof...(I)])(lua_State * L, T & objref) = {
                    +[](lua_State *L, T &objref) {
                        detail::MetaIndexImpl2(L, GetStructMember<I>(objref));
                    }...};
            if (N < 1 || N > sizeof...(I)) return 0;
            meta_switch[N - 1](L, objref);
            return 1;
        }
        template<std::size_t... I>
        int MetaNewIndexImpl(lua_State *L, T &objref, std::size_t N,
                             std::index_sequence<I...>) const {
            static constexpr void (*meta_switch[sizeof...(I)])(lua_State * L, T & objref) = {
                    +[](lua_State *L, T &objref) { Get(L, -1, GetStructMember<I>(objref)); }...};
            if (N < 1 || N > sizeof...(I)) return 0;
            meta_switch[N - 1](L, objref);
            return 1;
        }
        int MetaIndex(lua_State *L) const override {
            if (lua_type(L, 2) == LUA_TSTRING) {
                if (!strcmp(lua_tostring(L, 2), "self")) return this->Copy(L);
            }
            detail::TypeInterfaceStringIndexReplace(
                    this, L, "class/", nameof::nameof_short_type<typename std::decay<T>::type>());
            void *value = lua_touserdata(L, 1);
            int N = lua_tointeger(L, 2);
            if (T *object = reinterpret_cast<T *>(value))
                return MetaIndexImpl(L, *object, N, std::make_index_sequence<MemberCount>());
            return 0;
        }
        int MetaNewIndex(lua_State *L) const override {
            if (lua_type(L, 2) == LUA_TSTRING) {
                if (!strcmp(lua_tostring(L, 2), "self")) return this->Assign(L);
            }
            detail::TypeInterfaceStringIndexReplace(
                    this, L, "class/", nameof::nameof_short_type<typename std::decay<T>::type>());
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
        struct IsFunctionType<T, std::void_t<decltype(std::function(std::declval<T>()))>>
            : std::true_type
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
        int MetaCallImpl2(lua_State *L, std::index_sequence<I...>,
                          TypeIdentity<std::function<Ret(Args...)>>) const {
            if (int argn = lua_gettop(L); argn - 1 < (int) sizeof...(I)) {
                luaL_error(L, "bad function call with unmatched args, excepted %d got %d",
                           static_cast<int>(sizeof...(I)), argn - 1);
                return 0;
            }

            void *value = lua_touserdata(L, 1);
            T *pfn = reinterpret_cast<T *>(value);

            // dont support lvalue reference
            //static_assert((... && !(std::is_lvalue_reference<Args>::value && !std::is_const<typename std::remove_reference<Args>::type>::value)));

            std::tuple<
                    typename std::remove_const<typename std::remove_reference<Args>::type>::type...>
                    args;
            (..., Get(L, I + 2, std::get<I>(args)));
            if constexpr (std::is_void_v<Ret>) {
                (*pfn)(std::get<I>(args)...);
            } else {
                Ret ret = (*pfn)(std::get<I>(args)...);
                Push(L, std::move(ret));
            }

            return (!std::is_void_v<Ret> + ... +
                    PushIfLvRef(L, std::get<I>(args),
                                std::bool_constant<(std::is_lvalue_reference<Args>::value &&
                                                    !std::is_const<typename std::remove_reference<
                                                            Args>::type>::value)>()));
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
    class TypeInterface
        : public TypeInterfaceDecoratorCommon<
                  T, TypeInterfaceDecoratorIndex<
                             T, TypeInterfaceDecoratorComplete<
                                        T, TypeInterfaceDecoratorCopy<
                                                   T, TypeInterfaceDecoratorCall<
                                                              T, TypeInterfaceDecoratorToString<
                                                                         T, ITypeInterface>>>>>> {
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
        if (const ITypeInterface *ti = GetTypeInterface(L, value)) return (ti->*pmemfn)(L);
        luaL_error(L, "unrecognized ref ptr %x", value);
        return 0;
    }

    int SetupTypeInterfaceTable(lua_State *L);
    int SetupBuiltinTypeInterface(lua_State *L);
    int SetupPtrMetaTable(lua_State *L);
    int SetupCppNewDelete(lua_State *L);

    struct PushLambda
    {
        template<class T>
        auto operator()(T &&in) {
            return Push(L, std::forward<T>(in));
        }
        lua_State *const L;
    };
    template<class T>
    auto PushBoolean(lua_State *L, T x) ->
            typename std::enable_if<std::is_same<T, bool>::value>::type {
        lua_pushboolean(L, x);
    }
    template<class T>
    auto PushInteger(lua_State *L, T x) ->
            typename std::enable_if<std::is_integral<T>::value &&
                                    !std::is_same<T, bool>::value>::type {
        lua_pushinteger(L, x);
    }
    template<class T>
    auto PushFloat(lua_State *L, T x) ->
            typename std::enable_if<std::is_floating_point<T>::value>::type {
        lua_pushnumber(L, x);
    }
    template<class T, class Ratio>
    auto PushFloat(lua_State *L, std::chrono::duration<T, Ratio> x) ->
            typename std::enable_if<std::is_floating_point<T>::value>::type {
        lua_pushnumber(
                L, std::chrono::duration_cast<std::chrono::duration<T, std::ratio<1>>>(x).count());
    }
    template<class Clock, class Duration>
    auto PushFloat(lua_State *L, const std::chrono::time_point<Clock, Duration> &x) ->
            typename std::enable_if<std::is_floating_point<typename Duration::rep>::value>::type {
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
    inline void PushNil(lua_State *L, std::nullptr_t = nullptr) { lua_pushboolean(L, false); }
    using std::begin;
    using std::end;
    template<class T>
    auto PushArray(lua_State *L, T &&v)
            -> decltype(begin(std::forward<T>(v)), end(std::forward<T>(v)), void()) {
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
            (lua_newtable(L), ...,
             (lua_pushinteger(L, I + 1), Push(L, std::forward<Args>(args)), lua_settable(L, -3)));
        }
    }// namespace detail
    template<class T>
    auto PushStruct(lua_State *L, T &&v) ->
            typename std::enable_if<std::is_aggregate<typename std::decay<T>::type>::value>::type {
        StructApply(v, [L](auto &&...args) {
            detail::PushStructImpl(L, std::make_index_sequence<sizeof...(args)>(),
                                   std::forward<decltype(args)>(args)...);
        });
        CallOnStructCreate<typename std::decay<T>::type>(L);
    }
    int PushLuaObjectByPtr(lua_State *L, void *Ptr);
    template<class T>
    void PushPointer(lua_State *L, T *ptr) {
        if (!ptr) return lua_pushboolean(L, false);
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
    inline void PushString(lua_State *L, char *ptr) { PushPointer(L, ptr); }
    inline void PushString(lua_State *L, const char *ptr) { lua_pushstring(L, ptr); }
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
        auto PushUnknownImpl(lua_State *L, T x, PriorityTag<5>, int = 0)
                -> decltype(PushInteger(L, x)) {
            return PushInteger(L, x);
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T x, PriorityTag<5>, float = 0)
                -> decltype(PushFloat(L, x)) {
            return PushFloat(L, x);
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T &&x, PriorityTag<4>)
                -> decltype(PushString(L, std::forward<T>(x))) {
            return PushString(L, std::forward<T>(x));
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T &&x, PriorityTag<3>)
                -> decltype(PushArray(L, std::forward<T>(x))) {
            return PushArray(L, std::forward<T>(x));
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T &&x, PriorityTag<2>)
                -> decltype(PushStruct(L, std::forward<T>(x))) {
            return PushStruct(L, std::forward<T>(x));
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T x, PriorityTag<1>)
                -> decltype(PushMemberPointer(L, x)) {
            return PushMemberPointer(L, x);
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T x, PriorityTag<0>) -> decltype(PushPointer(L, x)) {
            return PushPointer(L, x);
        }
        template<class T>
        auto PushUnknownImpl(lua_State *L, T x, PriorityTag<0>, std::nullptr_t = nullptr) ->
                typename std::enable_if<std::is_null_pointer<T>::value>::type {
            return PushNil(L, x);
        }
        using MaxPriorityTag = PriorityTag<10>;
        template<class T>
        auto PushImpl(lua_State *L, T &&value)
                -> decltype(PushUnknownImpl(L, std::forward<T>(value), MaxPriorityTag())) {
            return PushUnknownImpl(L, std::forward<T>(value), MaxPriorityTag());
        }
    }// namespace detail

    inline void GetBoolean(lua_State *L, int N, bool &x) { x = lua_toboolean(L, N); }
    template<class T>
    auto GetInteger(lua_State *L, int N, T &x) ->
            typename std::enable_if<std::is_integral<T>::value>::type {
        x = lua_tointeger(L, N);
    }
    template<class T>
    auto GetFloat(lua_State *L, int N, T &x) ->
            typename std::enable_if<std::is_floating_point<T>::value>::type {
        x = lua_tonumber(L, N);
    }
    template<class T, class Ratio>
    auto GetFloat(lua_State *L, int N, std::chrono::duration<T, Ratio> &x) ->
            typename std::enable_if<std::is_floating_point<T>::value>::type {
        T y;
        GetFloat(L, N, y);
        x = std::chrono::duration<T, std::ratio<1>>(y);
    }
    template<class Clock, class Duration>
    auto GetFloat(lua_State *L, int N, std::chrono::time_point<Clock, Duration> &x) ->
            typename std::enable_if<std::is_floating_point<typename Duration::rep>::value>::type {
        Duration z;
        GetFloat(L, N, z);
        x = std::chrono::time_point<Clock, Duration>() + z;
    }
    template<class T>
    auto GetEnum(lua_State *L, int N, T &x) ->
            typename std::enable_if<std::is_enum<T>::value>::type {
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
    inline void GetString(lua_State *L, int N, std::string &str) { str = lua_tostring(L, N); }
    inline void GetString(lua_State *L, int N, std::string_view &sv) { sv = lua_tostring(L, N); }
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
            (..., (lua_pushinteger(L, I + 1),
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
    auto GetStruct(lua_State *L, int N, T &v) ->
            typename std::enable_if<std::is_aggregate<typename std::decay<T>::type>::value>::type {
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
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<10>)
                -> decltype(GetCustom(L, N, x)) {
            return GetCustom(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<8>)
                -> decltype(GetBoolean(L, N, x)) {
            return GetBoolean(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<7>)
                -> decltype(GetEnum(L, N, x)) {
            return GetEnum(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<6>, int = 0)
                -> decltype(GetInteger(L, N, x)) {
            return GetInteger(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<6>, float = 0)
                -> decltype(GetFloat(L, N, x)) {
            return GetFloat(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<5>)
                -> decltype(GetString(L, N, x)) {
            return GetString(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<4>)
                -> decltype(GetEntity(L, N, x)) {
            return GetEntity(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<4>)
                -> decltype(GetVector(L, N, x)) {
            return GetVector(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<3>)
                -> decltype(GetArray(L, N, x)) {
            return GetArray(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<2>)
                -> decltype(GetStruct(L, N, x)) {
            return GetStruct(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<1>)
                -> decltype(GetMemberPointer(L, N, x)) {
            return GetMemberPointer(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<0>)
                -> decltype(GetPointer(L, N, x)) {
            return GetPointer(L, N, x);
        }
        template<class T>
        auto GetUnknownImpl(lua_State *L, int N, T &x, PriorityTag<0>, std::nullptr_t = nullptr) ->
                typename std::enable_if<std::is_null_pointer<T>::value>::type {
            return GetNil(L, N, x);
        }
        using MaxPriorityTag = PriorityTag<10>;
        template<class T>
        auto GetImpl(lua_State *L, int N, T &value)
                -> decltype(GetUnknownImpl(L, N, value, MaxPriorityTag())) {
            return GetUnknownImpl(L, N, value, MaxPriorityTag());
        }
    }// namespace detail

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
}// namespace LuaStruct

#define METAENGINE_LUAWRAPPER_USE_CPP11 1

#ifndef METAENGINE_LUAWRAPPER_NO_USERDATA_TYPE_CHECK
#define METAENGINE_LUAWRAPPER_NO_USERDATA_TYPE_CHECK 0
#endif

//If you want use registered class by LuaWrapper between multiple shared library,
//please switch to 1 for METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY and METAENGINE_LUAWRAPPER_NAME_BASED_TYPE_CHECK
#ifndef METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
#define METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY 0
#endif

#ifndef METAENGINE_LUAWRAPPER_NAME_BASED_TYPE_CHECK
#define METAENGINE_LUAWRAPPER_NAME_BASED_TYPE_CHECK                                                \
    METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
#endif

#ifdef METAENGINE_LUAWRAPPER_NO_VECTOR_AND_MAP_TO_TABLE
#define METAENGINE_LUAWRAPPER_NO_STD_VECTOR_TO_TABLE
#define METAENGINE_LUAWRAPPER_NO_STD_MAP_TO_TABLE
#endif

#if !METAENGINE_LUAWRAPPER_USE_CPP11
#ifndef METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS
///! max argument number for binding function. this define used C++03 only.
#define METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS 9
#endif

#ifndef METAENGINE_LUAWRAPPER_FUNCTION_MAX_TUPLE_SIZE
///! this define used C++03 only.
#define METAENGINE_LUAWRAPPER_FUNCTION_MAX_TUPLE_SIZE 9
#endif

#ifndef METAENGINE_LUAWRAPPER_FUNCTION_MAX_OVERLOADS
#define METAENGINE_LUAWRAPPER_FUNCTION_MAX_OVERLOADS 9
#endif

#endif

#ifndef METAENGINE_LUAWRAPPER_CLASS_MAX_BASE_CLASSES
#define METAENGINE_LUAWRAPPER_CLASS_MAX_BASE_CLASSES 9
#endif

#if defined(__GNUC__) ||                                                                           \
        defined(__clang__) && (!defined(_MSC_VER) && !defined(METADOT_PLATFORM_WINDOWS))
#define METAENGINE_LUAWRAPPER_USE_CXX_ABI_DEMANGLE
#endif

#ifndef METAENGINE_LUAWRAPPER_USE_SHARED_LUAREF
#define METAENGINE_LUAWRAPPER_USE_SHARED_LUAREF 0
#endif

#ifndef METAENGINE_LUAWRAPPER_NOEXCEPT
#if METAENGINE_LUAWRAPPER_USE_CPP11 && (!defined(_MSC_VER) || _MSC_VER >= 1900)
#define METAENGINE_LUAWRAPPER_NOEXCEPT noexcept
#else
#define METAENGINE_LUAWRAPPER_NOEXCEPT throw()
#endif
#endif

#ifndef METAENGINE_LUAWRAPPER_DEPRECATED_FEATURE
#if __cplusplus >= 201402L && defined(__has_cpp_attribute)
#if __has_cpp_attribute(deprecated)
// C++ standard deprecated
#define METAENGINE_LUAWRAPPER_DEPRECATED_FEATURE(MSG) [[deprecated(MSG)]]
#endif
#endif
#endif
#ifndef METAENGINE_LUAWRAPPER_DEPRECATED_FEATURE
#if defined(_MSC_VER)
// MSVC deprecated
#define METAENGINE_LUAWRAPPER_DEPRECATED_FEATURE(MSG) __declspec(deprecated(MSG))
#elif defined(__GNUC__) || defined(__clang__)
#define METAENGINE_LUAWRAPPER_DEPRECATED_FEATURE(MSG) __attribute__((deprecated))
#else
#define METAENGINE_LUAWRAPPER_DEPRECATED_FEATURE(MSG)
#endif

#endif

#define METAENGINE_LUAWRAPPER_UNUSED(V) (void) (V)

namespace LuaWrapper {
#if defined(_MSC_VER) && _MSC_VER <= 1500
    typedef unsigned char uint8_t;
    typedef int int32_t;
    typedef long long int64_t;
#endif

    namespace standard {
#if METAENGINE_LUAWRAPPER_USE_CPP11
        using namespace std;
#define METAENGINE_LUAWRAPPER_STATIC_ASSERT static_assert

#else
        using namespace boost;
#define METAENGINE_LUAWRAPPER_STATIC_ASSERT BOOST_STATIC_ASSERT_MSG
#endif
    }// namespace standard

#if LUA_VERSION_NUM > 502
    typedef lua_Integer luaInt;
#else
    typedef int32_t luaInt;
#endif
}// namespace LuaWrapper

#if defined(METAENGINE_LUAWRAPPER_USE_CXX_ABI_DEMANGLE)
#include <cxxabi.h>
#endif

namespace LuaWrapper {
    /// @addtogroup optional
    ///  @{

    struct bad_optional_access : std::exception
    {
    };
    struct nullopt_t
    {
    };

    /// @brief self implement for std::optional(C++17 feature).
    template<class T>
    class optional {
        typedef void (optional::*bool_type)() const;
        void this_type_does_not_support_comparisons() const {}

    public:
        optional() : value_(0){};
        optional(nullopt_t) : value_(0){};
        optional(const optional &other) : value_(0) {
            if (other) { value_ = new (&storage_) T(other.value()); }
        }
        optional(const T &value) { value_ = new (&storage_) T(value); }

        ~optional() { destruct(); }
        optional &operator=(nullopt_t) {
            destruct();
            return *this;
        }
        optional &operator=(const optional &other) {
            if (other) {
                *this = other.value();
            } else {
                destruct();
            }
            return *this;
        }
        optional &operator=(const T &value) {
            if (value_) {
                *value_ = value;
            } else {
                value_ = new (&storage_) T(value);
            }
            return *this;
        }

#if METAENGINE_LUAWRAPPER_USE_CPP11
        optional(optional &&other) : value_(0) {
            if (other) { value_ = new (&storage_) T(std::move(other.value())); }
        }
        optional(T &&value) { value_ = new (&storage_) T(std::move(value)); }
        optional &operator=(optional &&other) {
            if (other) {
                *this = std::move(other.value());
            } else {
                destruct();
            }
            return *this;
        }
        optional &operator=(T &&value) {
            if (value_) {
                *value_ = std::move(value);
            } else {
                value_ = new (&storage_) T(std::move(value));
            }
            return *this;
        }
#endif

        operator bool_type() const {
            this_type_does_not_support_comparisons();
            return value_ != 0 ? &optional::this_type_does_not_support_comparisons : 0;
        }
        T &value() {
            if (value_) { return *value_; }
            throw bad_optional_access();
        }
        const T &value() const {
            if (value_) { return *value_; }
            throw bad_optional_access();
        }

#if METAENGINE_LUAWRAPPER_USE_CPP11
        template<class U>
        T value_or(U &&default_value) const {
            if (value_) { return *value_; }
            return default_value;
        }
#else
        template<class U>
        T value_or(const U &default_value) const {
            if (value_) { return *value_; }
            return default_value;
        }
#endif
        const T *operator->() const {
            assert(value_);
            return value_;
        }
        T *operator->() {
            assert(value_);
            return value_;
        }
        const T &operator*() const {
            assert(value_);
            return *value_;
        }
        T &operator*() {
            assert(value_);
            return *value_;
        }

    private:
        void destruct() {
            if (value_) {
                value_->~T();
                value_ = 0;
            }
        }

        typename standard::aligned_storage<sizeof(T), standard::alignment_of<T>::value>::type
                storage_;

        T *value_;
    };

    /// @brief specialize optional for reference.
    /// sizeof(optional<T&>) == sizeof(T*)
    template<class T>
    class optional<T &> {
        typedef void (optional::*bool_type)() const;
        void this_type_does_not_support_comparisons() const {}

    public:
        optional() : value_(0){};
        optional(nullopt_t) : value_(0){};

        optional(const optional &other) : value_(other.value_) {}
        optional(T &value) : value_(&value) {}

        ~optional() {}
        optional &operator=(nullopt_t) {
            value_ = 0;
            return *this;
        }
        optional &operator=(const optional &other) {
            value_ = other.value_;
            return *this;
        }
        optional &operator=(T &value) {
            value_ = &value;
            return *this;
        }
        operator bool_type() const {
            this_type_does_not_support_comparisons();
            return value_ != 0 ? &optional::this_type_does_not_support_comparisons : 0;
        }
        T &value() {
            if (value_) { return *value_; }
            throw bad_optional_access();
        }
        const T &value() const {
            if (value_) { return *value_; }
            throw bad_optional_access();
        }

#if METAENGINE_LUAWRAPPER_USE_CPP11
        T &value_or(T &default_value) const {
            if (value_) { return *value_; }
            return default_value;
        }
#else
        T &value_or(T &default_value) const {
            if (value_) { return *value_; }
            return default_value;
        }
#endif

        const T *operator->() const {
            assert(value_);
            return value_;
        }
        T *operator->() {
            assert(value_);
            return value_;
        }
        const T &operator*() const {
            assert(value_);
            return *value_;
        }
        T &operator*() {
            assert(value_);
            return *value_;
        }

    private:
        T *value_;
    };

    /// @name relational operators
    /// @brief
    ///@{
    template<class T>
    bool operator==(const optional<T> &lhs, const optional<T> &rhs) {
        if (bool(lhs) != bool(rhs)) { return false; }
        if (bool(lhs) == false) { return true; }
        return lhs.value() == rhs.value();
    }
    template<class T>
    bool operator!=(const optional<T> &lhs, const optional<T> &rhs) {
        return !(lhs == rhs);
    }
    template<class T>
    bool operator<(const optional<T> &lhs, const optional<T> &rhs) {
        if (!bool(rhs)) { return false; }
        if (!bool(lhs)) { return true; }
        return lhs.value() < rhs.value();
    }
    template<class T>
    bool operator<=(const optional<T> &lhs, const optional<T> &rhs) {
        return !(rhs < lhs);
    }
    template<class T>
    bool operator>(const optional<T> &lhs, const optional<T> &rhs) {
        return rhs < lhs;
    }
    template<class T>
    bool operator>=(const optional<T> &lhs, const optional<T> &rhs) {
        return !(lhs < rhs);
    }
    /// @}

    /// @}
}// namespace LuaWrapper

namespace LuaWrapper {
    namespace traits {
        using standard::false_type;
        using standard::integral_constant;
        using standard::is_arithmetic;
        using standard::is_class;
        using standard::is_const;
        using standard::is_convertible;
        using standard::is_enum;
        using standard::is_floating_point;
        using standard::is_function;
        using standard::is_integral;
        using standard::is_lvalue_reference;
        using standard::is_pointer;
        using standard::is_same;
        using standard::is_union;
        using standard::is_void;
        using standard::remove_const;
        using standard::remove_cv;
        using standard::remove_pointer;
        using standard::remove_reference;
        using standard::remove_volatile;
        using standard::true_type;
#if METAENGINE_LUAWRAPPER_USE_CPP11
        using std::enable_if;
#else
        template<bool B, class T = void>
        struct enable_if : boost::enable_if_c<B, T>
        {
        };
#endif

        class Helper {};
        /// @brief Check if T_Mem is a member object of a type. That is true if it is
        /// not a member function
        /// Required as MSVC throws a COMDAT error when using is_member_object_pointer
        template<typename T_Mem>
        struct is_object
        {
            typedef typename standard::is_member_function_pointer<T_Mem Helper::*>::type NotResult;
            enum {
                value = !NotResult::value
            };
        };

        /// @brief Similar to std::decay but also removes const and volatile modifiers
        /// if T is neither an array nor a function
        template<class T>
        struct decay
        {
        private:
            ///@ If T is a reference type, the type referrered to by T.	Otherwise, T.
            typedef typename standard::remove_reference<T>::type U;

        public:
            typedef typename standard::conditional<
                    standard::is_array<U>::value, typename standard::remove_extent<U>::type *,
                    typename standard::conditional<
                            is_function<U>::value, typename standard::add_pointer<U>::type,
                            typename standard::remove_cv<U>::type>::type>::type type;
        };

        /// @brief Trait class that identifies whether T is a const reference type.
        template<class T>
        struct is_const_reference : false_type
        {
        };
        template<class T>
        struct is_const_reference<const T &> : true_type
        {
        };

        /// @brief Obtains the type T without top-level const and reference.
        template<typename T>
        struct remove_const_and_reference
        {
            /// @brief If T is const or reference, the same type as T but with the const
            /// reference removed.Otherwise, T
            typedef T type;
        };
        /// @brief Obtains the type T without top-level const and reference.
        template<typename T>
        struct remove_const_and_reference<T &>
        {
            /// @brief If T is const or reference, the same type as T but with the const
            /// reference removed.Otherwise, T
            typedef T type;
        };
        /// @brief Obtains the type T without top-level const and reference.
        template<typename T>
        struct remove_const_and_reference<const T>
        {
            /// @brief If T is const or reference, the same type as T but with the const
            /// reference removed.Otherwise, T
            typedef T type;
        };
        /// @brief Obtains the type T without top-level const and reference.
        template<typename T>
        struct remove_const_and_reference<const T &>
        {
            /// @brief If T is const or reference, the same type as T but with the const
            /// reference removed.Otherwise, T
            typedef T type;
        };

        /// @brief Obtains the type T without top-level const reference.
        template<typename T>
        struct remove_const_reference
        {
            /// @brief If T is const reference, the same type as T but with the const
            /// reference removed.Otherwise, T
            typedef T type;
        };
        /// @brief Obtains the type T without top-level const reference.
        template<typename T>
        struct remove_const_reference<const T &>
        {
            /// @brief If T is const reference, the same type as T but with the const
            /// reference removed.Otherwise, T
            typedef T type;
        };

        /// @brief Trait class that identifies whether T is a std::vector type.
        template<class T>
        struct is_std_vector : false_type
        {
        };
        template<class T, class A>
        struct is_std_vector<std::vector<T, A>> : true_type
        {
        };

        /// @brief Trait class that identifies whether T is a std::map type.
        template<class T>
        struct is_std_map : false_type
        {
        };
        template<class K, class V, class C, class A>
        struct is_std_map<std::map<K, V, C, A>> : true_type
        {
        };
    }// namespace traits

    /// @addtogroup lua_type_traits

    /// @ingroup lua_type_traits
    /// @brief If you want to customize the conversion to type of lua yourself ,
    ///  implement specialize of this class
    template<typename T, typename Enable = void>
    struct lua_type_traits
    {
        typedef void Registerable;

        typedef typename traits::decay<T>::type NCRT;
        typedef const NCRT &get_type;
        typedef optional<get_type> opt_type;
        typedef const NCRT &push_type;

        static bool checkType(lua_State *l, int index);
        static bool strictCheckType(lua_State *l, int index);

        static get_type get(lua_State *l, int index);
        static opt_type opt(lua_State *l, int index) METAENGINE_LUAWRAPPER_NOEXCEPT;
        static int push(lua_State *l, push_type v);
        static int push(lua_State *l, NCRT &&v);
    };

    /// @brief Trait class that identifies whether T is a userdata type.
    template<typename T, typename Enable = void>
    struct is_usertype : traits::false_type
    {
    };
    template<typename T>
    struct is_usertype<T, typename lua_type_traits<T>::Registerable> : traits::true_type
    {
    };

    /// @brief Trait class that identifies whether T is a registerable by
    /// UserdataMetatable.
    template<typename T>
    struct is_registerable : is_usertype<T>
    {
    };
}// namespace LuaWrapper

namespace LuaWrapper {
    // for lua version compatibility
    namespace compat {
#if LUA_VERSION_NUM >= 503
        inline int lua_rawgetp_rtype(lua_State *L, int idx, const void *ptr) {
            return lua_rawgetp(L, idx, ptr);
        }
        inline int lua_rawget_rtype(lua_State *L, int idx) { return lua_rawget(L, idx); }
        inline int lua_getfield_rtype(lua_State *L, int idx, const char *k) {
            return lua_getfield(L, idx, k);
        }
        inline int lua_gettable_rtype(lua_State *L, int idx) { return lua_gettable(L, idx); }
#elif LUA_VERSION_NUM == 502
        inline int lua_rawgetp_rtype(lua_State *L, int idx, const void *ptr) {
            lua_rawgetp(L, idx, ptr);
            return lua_type(L, -1);
        }
#elif LUA_VERSION_NUM < 502
        enum LUA_OPEQ {
            LUA_OPEQ,
            LUA_OPLT,
            LUA_OPLE
        };
        inline int lua_compare(lua_State *L, int index1, int index2, int op) {
            switch (op) {
                case LUA_OPEQ:
                    return lua_equal(L, index1, index2);
                case LUA_OPLT:
                    return lua_lessthan(L, index1, index2);
                case LUA_OPLE:
                    return lua_equal(L, index1, index2) || lua_lessthan(L, index1, index2);
                default:
                    return 0;
            }
        }

        inline void lua_pushglobaltable(lua_State *L) { lua_pushvalue(L, LUA_GLOBALSINDEX); }
        inline size_t lua_rawlen(lua_State *L, int index) {
            int type = lua_type(L, index);
            if (type != LUA_TSTRING && type != LUA_TTABLE && type != LUA_TUSERDATA &&
                type != LUA_TLIGHTUSERDATA) {
                return 0;
            }
            return lua_objlen(L, index);
        }

        inline int lua_resume(lua_State *L, lua_State *from, int nargs) {
            METAENGINE_LUAWRAPPER_UNUSED(from);
            return ::lua_resume(L, nargs);
        }
        inline int lua_absindex(lua_State *L, int idx) {
            return (idx > 0 || (idx <= LUA_REGISTRYINDEX)) ? idx : lua_gettop(L) + 1 + idx;
        }
        inline int lua_rawgetp_rtype(lua_State *L, int idx, const void *ptr) {
            int absidx = lua_absindex(L, idx);
            lua_pushlightuserdata(L, (void *) ptr);
            lua_rawget(L, absidx);
            return lua_type(L, -1);
        }
        inline void lua_rawsetp(lua_State *L, int idx, const void *ptr) {
            int absidx = lua_absindex(L, idx);
            lua_pushvalue(L, -1);
            lua_pushlightuserdata(L, (void *) ptr);
            lua_replace(L, -3);
            lua_rawset(L, absidx);
        }
        inline void luaL_requiref(lua_State *L, const char *modname, lua_CFunction openf, int glb) {

            lua_pushcfunction(L, openf);
            lua_pushstring(L, modname);
            lua_call(L, 1, 1);

            if (glb) {
                lua_pushvalue(L, -1);
                lua_setglobal(L, modname);
            }
        }
        inline lua_Number lua_tonumberx(lua_State *L, int index, int *isnum) {
            if (isnum) { *isnum = lua_isnumber(L, index); }
            return lua_tonumber(L, index);
        }
#endif
#if LUA_VERSION_NUM < 503
        inline void lua_seti(lua_State *L, int index, lua_Integer n) {
            int absidx = lua_absindex(L, index);
            lua_pushvalue(L, -1);
            lua_pushinteger(L, n);
            lua_replace(L, -3);
            lua_rawset(L, absidx);
        }
        inline int lua_geti(lua_State *L, int index, lua_Integer i) {
            int absidx = lua_absindex(L, index);
            lua_pushinteger(L, i);
            lua_rawget(L, absidx);
            return lua_type(L, -1);
        }
        inline int lua_getfield_rtype(lua_State *L, int idx, const char *k) {
            lua_getfield(L, idx, k);
            return lua_type(L, -1);
        }
        inline int lua_gettable_rtype(lua_State *L, int idx) {
            lua_gettable(L, idx);
            return lua_type(L, -1);
        }
        inline int lua_rawget_rtype(lua_State *L, int idx) {
            lua_rawget(L, idx);
            return lua_type(L, -1);
        }
#endif
#if LUA_VERSION_NUM < 501
        void lua_createtable(lua_State *L, int narr, int nrec) { lua_newtable(L); }
#endif
    }// namespace compat

    using namespace compat;
}// namespace LuaWrapper

namespace LuaWrapper {
    class LuaException : public std::exception {
        int status_;
        std::string what_;
        const char *what_c_;

    public:
        LuaException(int status, const char *what) throw() : status_(status), what_c_(what) {}
        LuaException(int status, const std::string &what)
            : status_(status), what_(what), what_c_(0) {}
        int status() const throw() { return status_; }
        const char *what() const throw() { return what_c_ ? what_c_ : what_.c_str(); }

        ~LuaException() throw() {}
    };
    class KaguyaException : public std::exception {
        std::string what_;
        const char *what_c_;

    public:
        KaguyaException(const char *what) throw() : what_c_(what) {}
        KaguyaException(const std::string &what) : what_(what), what_c_(0) {}
        const char *what() const throw() { return what_c_ ? what_c_ : what_.c_str(); }

        ~KaguyaException() throw() {}
    };
    class LuaTypeMismatch : public LuaException {
    public:
        LuaTypeMismatch() throw() : LuaException(0, "type mismatch!!") {}
        LuaTypeMismatch(const char *what) throw() : LuaException(0, what) {}
        LuaTypeMismatch(const std::string &what) : LuaException(0, what) {}
    };
    class LuaMemoryError : public LuaException {
    public:
        LuaMemoryError(int status, const char *what) throw() : LuaException(status, what) {}
        LuaMemoryError(int status, const std::string &what) : LuaException(status, what) {}
    };
    class LuaRuntimeError : public LuaException {
    public:
        LuaRuntimeError(int status, const char *what) throw() : LuaException(status, what) {}
        LuaRuntimeError(int status, const std::string &what) : LuaException(status, what) {}
    };
    class LuaErrorRunningError : public LuaException {
    public:
        LuaErrorRunningError(int status, const char *what) throw() : LuaException(status, what) {}
        LuaErrorRunningError(int status, const std::string &what) : LuaException(status, what) {}
    };
    class LuaGCError : public LuaException {
    public:
        LuaGCError(int status, const char *what) throw() : LuaException(status, what) {}
        LuaGCError(int status, const std::string &what) : LuaException(status, what) {}
    };
    class LuaUnknownError : public LuaException {
    public:
        LuaUnknownError(int status, const char *what) throw() : LuaException(status, what) {}
        LuaUnknownError(int status, const std::string &what) : LuaException(status, what) {}
    };

    class LuaSyntaxError : public LuaException {
    public:
        LuaSyntaxError(int status, const std::string &what) : LuaException(status, what) {}
    };

    namespace except {
        void OtherError(lua_State *state, const std::string &message);
        void typeMismatchError(lua_State *state, const std::string &message);
        void memoryError(lua_State *state, const char *message);
        bool checkErrorAndThrow(int status, lua_State *state);
    }// namespace except
}// namespace LuaWrapper

#if METAENGINE_LUAWRAPPER_USE_CPP11

namespace LuaWrapper {
    namespace util {
        struct null_type
        {
        };

        template<class... Args>
        struct TypeTuple
        {
        };
        template<class Ret, class... Args>
        struct FunctionSignatureType
        {
            typedef Ret result_type;
            typedef TypeTuple<Args...> argument_type_tuple;
            static const size_t argument_count = sizeof...(Args);
            typedef Ret (*c_function_type)(Args...);
        };
        template<typename T>
        struct FunctorSignature
        {
        };

        template<typename T, typename Ret, typename... Args>
        struct FunctorSignature<Ret (T::*)(Args...) const>
        {
            typedef FunctionSignatureType<Ret, Args...> type;
        };
        template<typename T, typename Ret, typename... Args>
        struct FunctorSignature<Ret (T::*)(Args...)>
        {
            typedef FunctionSignatureType<Ret, Args...> type;
        };

#if defined(_MSC_VER) && _MSC_VER < 1900
        template<typename T>
        struct FunctionSignature : public FunctorSignature<decltype(&T::operator())>
        {
        };
#else

        template<typename T, typename Enable = void>
        struct FunctionSignature;

        template<typename T, typename = void>
        struct has_operator_fn : std::false_type
        {
        };
        template<typename T>
        struct has_operator_fn<
                T,
                typename std::enable_if<!std::is_same<void, decltype(&T::operator())>::value>::type>
            : std::true_type
        {
        };

        template<typename T>
        struct FunctionSignature<T, typename std::enable_if<has_operator_fn<T>::value>::type>
            : public FunctorSignature<decltype(&T::operator())>
        {
        };
#endif

        template<typename T, typename Ret, typename... Args>
        struct FunctionSignature<Ret (T::*)(Args...)>
        {
            typedef FunctionSignatureType<Ret, T &, Args...> type;
        };
        template<typename T, typename Ret, typename... Args>
        struct FunctionSignature<Ret (T::*)(Args...) const>
        {
            typedef FunctionSignatureType<Ret, const T &, Args...> type;
        };

#if defined(_MSC_VER) && _MSC_VER >= 1900 || defined(__cpp_ref_qualifiers)
        template<typename T, typename Ret, typename... Args>
        struct FunctionSignature<Ret (T::*)(Args...) const &>
        {
            typedef FunctionSignatureType<Ret, const T &, Args...> type;
        };
        template<typename T, typename Ret, typename... Args>
        struct FunctionSignature<Ret (T::*)(Args...) const &&>
        {
            typedef FunctionSignatureType<Ret, const T &, Args...> type;
        };
#endif

        template<class Ret, class... Args>
        struct FunctionSignature<Ret (*)(Args...)>
        {
            typedef FunctionSignatureType<Ret, Args...> type;
        };
        template<class Ret, class... Args>
        struct FunctionSignature<Ret(Args...)>
        {
            typedef FunctionSignatureType<Ret, Args...> type;
        };

        template<typename F>
        struct FunctionResultType
        {
            typedef typename FunctionSignature<F>::type::result_type type;
        };

        template<std::size_t remain, class Arg, bool flag = remain <= 0>
        struct TypeIndexGet;

        template<std::size_t remain, class Arg, class... Args>
        struct TypeIndexGet<remain, TypeTuple<Arg, Args...>, true>
        {
            typedef Arg type;
        };

        template<std::size_t remain, class Arg, class... Args>
        struct TypeIndexGet<remain, TypeTuple<Arg, Args...>, false>
            : TypeIndexGet<remain - 1, TypeTuple<Args...>>
        {
        };
        template<int N, typename F>
        struct ArgumentType
        {
            typedef typename TypeIndexGet<
                    N, typename FunctionSignature<F>::type::argument_type_tuple>::type type;
        };

        namespace detail {
            template<class F, class ThisType, class... Args>
            auto invoke_helper(F &&f, ThisType &&this_, Args &&...args)
                    -> decltype((std::forward<ThisType>(this_).*f)(std::forward<Args>(args)...)) {
                return (std::forward<ThisType>(this_).*f)(std::forward<Args>(args)...);
            }

            template<class F, class... Args>
            auto invoke_helper(F &&f, Args &&...args) -> decltype(f(std::forward<Args>(args)...)) {
                return f(std::forward<Args>(args)...);
            }
        }// namespace detail
        template<class F, class... Args>
        typename FunctionResultType<typename traits::decay<F>::type>::type invoke(F &&f,
                                                                                  Args &&...args) {
            return detail::invoke_helper(std::forward<F>(f), std::forward<Args>(args)...);
        }
    }// namespace util
}// namespace LuaWrapper
#else

#include <string>

namespace LuaWrapper {
    namespace util {
        ///!
        struct null_type
        {
        };

#define METAENGINE_LUAWRAPPER_PP_STRUCT_TDEF_REP(N)                                                \
    METAENGINE_LUAWRAPPER_PP_CAT(typename A, N) = null_type
#define METAENGINE_LUAWRAPPER_PP_STRUCT_TEMPLATE_DEF_REPEAT(N)                                     \
    METAENGINE_LUAWRAPPER_PP_REPEAT_ARG(N, METAENGINE_LUAWRAPPER_PP_STRUCT_TDEF_REP)

        template<METAENGINE_LUAWRAPPER_PP_STRUCT_TEMPLATE_DEF_REPEAT(
                METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS)>
        struct TypeTuple
        {
        };

        template<typename F>
        struct TypeTupleSize;

#define METAENGINE_LUAWRAPPER_TYPE_TUPLE_SIZE_DEF(N)                                               \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    struct TypeTupleSize<TypeTuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>>               \
    {                                                                                              \
        static const size_t value = N;                                                             \
    };

        METAENGINE_LUAWRAPPER_TYPE_TUPLE_SIZE_DEF(0)
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_TYPE_TUPLE_SIZE_DEF)
#undef METAENGINE_LUAWRAPPER_TYPE_TUPLE_SIZE_DEF

        template<typename Ret, typename typetuple>
        struct CFuntionType;
#define METAENGINE_LUAWRAPPER_CFUNCTION_TYPE_DEF(N)                                                \
    template<typename Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>                  \
    struct CFuntionType<Ret, TypeTuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>>           \
    {                                                                                              \
        typedef Ret (*type)(METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N));                      \
    };

        METAENGINE_LUAWRAPPER_CFUNCTION_TYPE_DEF(0)
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_CFUNCTION_TYPE_DEF)
#undef METAENGINE_LUAWRAPPER_CFUNCTION_TYPE_DEF

        template<typename Ret, METAENGINE_LUAWRAPPER_PP_STRUCT_TEMPLATE_DEF_REPEAT(
                                       METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS)>
        struct FunctionSignatureType
        {
            typedef Ret result_type;
            typedef TypeTuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(
                    METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS)>
                    argument_type_tuple;
            static const size_t argument_count = TypeTupleSize<argument_type_tuple>::value;
            typedef typename CFuntionType<Ret, argument_type_tuple>::type c_function_type;
        };

        template<typename T, typename Enable = void>
        struct FunctionSignature;

#define METAENGINE_LUAWRAPPER_MEMBER_FUNCTION_SIGNATURE_DEF(N)                                     \
    template<typename T, typename Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>      \
    struct FunctionSignature<Ret (T::*)(METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N))>          \
    {                                                                                              \
        typedef FunctionSignatureType<Ret,                                                         \
                                      T & METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>  \
                type;                                                                              \
    };                                                                                             \
    template<typename T, typename Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>      \
    struct FunctionSignature<Ret (T::*)(METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)) const>    \
    {                                                                                              \
        typedef FunctionSignatureType<                                                             \
                Ret, const T & METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>             \
                type;                                                                              \
    };

#define METAENGINE_LUAWRAPPER_FUNCTION_SIGNATURE_DEF(N)                                            \
    template<class Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>                     \
    struct FunctionSignature<Ret (*)(METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N))>             \
    {                                                                                              \
        typedef FunctionSignatureType<Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>  \
                type;                                                                              \
    };                                                                                             \
    template<class Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>                     \
    struct FunctionSignature<Ret(METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N))>                 \
    {                                                                                              \
        typedef FunctionSignatureType<Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>  \
                type;                                                                              \
    };                                                                                             \
    template<class Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>                     \
    struct FunctionSignature<                                                                      \
            standard::function<Ret(METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N))>>              \
    {                                                                                              \
        typedef FunctionSignatureType<Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>  \
                type;                                                                              \
    };

        METAENGINE_LUAWRAPPER_MEMBER_FUNCTION_SIGNATURE_DEF(0)
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(
                METAENGINE_LUAWRAPPER_PP_DEC(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS),
                METAENGINE_LUAWRAPPER_MEMBER_FUNCTION_SIGNATURE_DEF)
#undef METAENGINE_LUAWRAPPER_MEMBER_FUNCTION_SIGNATURE_DEF
        METAENGINE_LUAWRAPPER_FUNCTION_SIGNATURE_DEF(0)
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_FUNCTION_SIGNATURE_DEF)
#undef METAENGINE_LUAWRAPPER_FUNCTION_SIGNATURE_DEF

        template<typename F>
        struct FunctionResultType
        {
            typedef typename FunctionSignature<F>::type::result_type type;
        };

        template<std::size_t remain, class result, bool flag = remain <= 0>
        struct TypeIndexGet
        {
        };

#define METAENGINE_LUAWRAPPER_TYPE_INDEX_GET_DEF(N)                                                \
    template<std::size_t remain, class arg METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)> \
    struct TypeIndexGet<                                                                           \
            remain, TypeTuple<arg METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>, true>   \
    {                                                                                              \
        typedef arg type;                                                                          \
    };                                                                                             \
    template<std::size_t remain, class arg METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)> \
    struct TypeIndexGet<                                                                           \
            remain, TypeTuple<arg METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>, false>  \
        : TypeIndexGet<remain - 1, TypeTuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>>     \
    {                                                                                              \
    };

        //		METAENGINE_LUAWRAPPER_TYPE_INDEX_GET_DEF(0);
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(
                METAENGINE_LUAWRAPPER_PP_DEC(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS),
                METAENGINE_LUAWRAPPER_TYPE_INDEX_GET_DEF)
#undef METAENGINE_LUAWRAPPER_TYPE_INDEX_GET_DEF

        template<std::size_t N, typename F>
        struct ArgumentType
        {
            typedef typename TypeIndexGet<
                    N, typename FunctionSignature<F>::type::argument_type_tuple>::type type;
        };

        namespace detail {

#define METAENGINE_LUAWRAPPER_INVOKE_HELPER_DEF(N)                                                 \
    template<class F, class ThisType METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N),       \
             typename T>                                                                           \
    typename FunctionResultType<F>::type invoke_helper(                                            \
            typename FunctionResultType<F>::type (T::*f)(                                          \
                    METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)),                              \
            ThisType this_ METAENGINE_LUAWRAPPER_PP_ARG_DEF_REPEAT_CONCAT(N)) {                    \
        return (this_.*f)(METAENGINE_LUAWRAPPER_PP_ARG_REPEAT(N));                                 \
    }                                                                                              \
    template<class F, class ThisType METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N),       \
             typename T>                                                                           \
    typename FunctionResultType<F>::type invoke_helper(                                            \
            typename FunctionResultType<F>::type (T::*f)(                                          \
                    METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)) const,                        \
            ThisType this_ METAENGINE_LUAWRAPPER_PP_ARG_DEF_REPEAT_CONCAT(N)) {                    \
        return (this_.*f)(METAENGINE_LUAWRAPPER_PP_ARG_REPEAT(N));                                 \
    }                                                                                              \
    template<class F METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>                       \
    typename FunctionResultType<F>::type invoke_helper(                                            \
            F f METAENGINE_LUAWRAPPER_PP_ARG_DEF_REPEAT_CONCAT(N)) {                               \
        return f(METAENGINE_LUAWRAPPER_PP_ARG_REPEAT(N));                                          \
    }

            METAENGINE_LUAWRAPPER_INVOKE_HELPER_DEF(0)
            METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                                METAENGINE_LUAWRAPPER_INVOKE_HELPER_DEF)
#undef METAENGINE_LUAWRAPPER_INVOKE_HELPER_DEF
        }// namespace detail

#define METAENGINE_LUAWRAPPER_INVOKE_DEF(N)                                                        \
    template<class F METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>                       \
    typename FunctionResultType<F>::type invoke(                                                   \
            F f METAENGINE_LUAWRAPPER_PP_ARG_DEF_REPEAT_CONCAT(N)) {                               \
        return detail::invoke_helper<F METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>(    \
                f METAENGINE_LUAWRAPPER_PP_ARG_REPEAT_CONCAT(N));                                  \
    }

        METAENGINE_LUAWRAPPER_INVOKE_DEF(0)
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_INVOKE_DEF)
#undef METAENGINE_LUAWRAPPER_INVOKE_DEF

#undef METAENGINE_LUAWRAPPER_PP_STRUCT_TDEF_REP
#undef METAENGINE_LUAWRAPPER_PP_STRUCT_TEMPLATE_DEF_REPEAT
    }// namespace util
}// namespace LuaWrapper
#endif

namespace LuaWrapper {
    namespace util {
        /// @brief save stack count and restore on destructor
        class ScopedSavedStack {
            lua_State *state_;
            int saved_top_index_;

        public:
            /// @brief save stack count
            /// @param state
            explicit ScopedSavedStack(lua_State *state)
                : state_(state), saved_top_index_(state_ ? lua_gettop(state_) : 0) {}

            /// @brief save stack count
            /// @param state
            /// @param count stack count
            explicit ScopedSavedStack(lua_State *state, int count)
                : state_(state), saved_top_index_(count) {}

            /// @brief restore stack count
            ~ScopedSavedStack() {
                if (state_) { lua_settop(state_, saved_top_index_); }
            }

        private:
            ScopedSavedStack(ScopedSavedStack const &);
            ScopedSavedStack &operator=(ScopedSavedStack const &);
        };
        inline void traceBack(lua_State *state, const char *message, int level = 0) {
#if LUA_VERSION_NUM >= 502
            luaL_traceback(state, state, message, level);
#else
            METAENGINE_LUAWRAPPER_UNUSED(level);
            lua_pushstring(state, message);
#endif
        }

        inline void stackDump(lua_State *L) {
            int i;
            int top = lua_gettop(L);
            for (i = 1; i <= top; i++) { /* repeat for each level */
                int t = lua_type(L, i);
                switch (t) {

                    case LUA_TSTRING: /* strings */
                        printf("`%s'", lua_tostring(L, i));
                        break;

                    case LUA_TBOOLEAN: /* booleans */
                        printf(lua_toboolean(L, i) ? "true" : "false");
                        break;

                    case LUA_TNUMBER: /* numbers */
                        printf("%g", lua_tonumber(L, i));
                        break;
                    case LUA_TUSERDATA:
                        if (luaL_getmetafield(L, i, "__name") == LUA_TSTRING) {
                            printf("userdata:%s", lua_tostring(L, -1));
                            lua_pop(L, 1);
                            break;
                        }
                    default: /* other values */
                        printf("%s", lua_typename(L, t));
                        break;
                }
                printf("  "); /* put a separator */
            }
            printf("\n"); /* end the listing */
        }

        inline void stackValueDump(std::ostream &os, lua_State *state, int stackIndex,
                                   int max_recursive = 2) {
            stackIndex = lua_absindex(state, stackIndex);
            util::ScopedSavedStack save(state);
            int type = lua_type(state, stackIndex);
            switch (type) {
                case LUA_TNONE:
                    os << "none";
                    break;
                case LUA_TNIL:
                    os << "nil";
                    break;
                case LUA_TBOOLEAN:
                    os << ((lua_toboolean(state, stackIndex) != 0) ? "true" : "false");
                    break;
                case LUA_TNUMBER:
                    os << lua_tonumber(state, stackIndex);
                    break;
                case LUA_TSTRING:
                    os << "'" << lua_tostring(state, stackIndex) << "'";
                    break;
                case LUA_TTABLE: {
                    os << "{";
                    if (max_recursive <= 1) {
                        os << "...";
                    } else {
                        lua_pushnil(state);
                        if ((lua_next(state, stackIndex) != 0)) {
                            stackValueDump(os, state, -2, max_recursive - 1);
                            os << "=";
                            stackValueDump(os, state, -1, max_recursive - 1);
                            lua_pop(state, 1);// pop value

                            while (lua_next(state, stackIndex) != 0) {
                                os << ",";
                                stackValueDump(os, state, -2, max_recursive - 1);
                                os << "=";
                                stackValueDump(os, state, -1, max_recursive - 1);
                                lua_pop(state, 1);// pop value
                            }
                        }
                    }
                    os << "}";
                } break;
                case LUA_TUSERDATA:
                case LUA_TLIGHTUSERDATA:
                case LUA_TTHREAD:
                    os << lua_typename(state, type) << "(" << lua_topointer(state, stackIndex)
                       << ")";
                    break;
                case LUA_TFUNCTION:
                    os << lua_typename(state, type);
                    break;
                default:
                    os << "unknown type value";
                    break;
            }
        }

#if LUA_VERSION_NUM >= 502
        inline lua_State *toMainThread(lua_State *state) {
            if (state) {
                lua_rawgeti(state, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
                lua_State *mainthread = lua_tothread(state, -1);
                lua_pop(state, 1);
                if (mainthread) { return mainthread; }
            }
            return state;
        }

#else
        inline lua_State *toMainThread(lua_State *state) {
            if (state) {
                // lua_pushthread return 1 if state is main thread
                bool state_is_main = lua_pushthread(state) == 1;
                lua_pop(state, 1);
                if (state_is_main) { return state; }
                lua_getfield(state, LUA_REGISTRYINDEX, "METAENGINE_LUAWRAPPER_REG_MAINTHREAD");
                lua_State *mainthread = lua_tothread(state, -1);
                lua_pop(state, 1);
                if (mainthread) { return mainthread; }
            }
            return state;
        }
        inline bool registerMainThread(lua_State *state) {
            if (lua_pushthread(state)) {
                lua_setfield(state, LUA_REGISTRYINDEX, "METAENGINE_LUAWRAPPER_REG_MAINTHREAD");
                return true;
            } else {
                lua_pop(state, 1);
                return false;
            }
        }
#endif

#if METAENGINE_LUAWRAPPER_USE_CPP11
        inline int push_args(lua_State *) { return 0; }
        template<class Arg, class... Args>
        inline int push_args(lua_State *l, Arg &&arg, Args &&...args) {
            int c = lua_type_traits<typename traits::decay<Arg>::type>::push(
                    l, std::forward<Arg>(arg));
            return c + push_args(l, std::forward<Args>(args)...);
        }
        template<class Arg, class... Args>
        inline int push_args(lua_State *l, const Arg &arg, Args &&...args) {
            int c = lua_type_traits<Arg>::push(l, arg);
            return c + push_args(l, std::forward<Args>(args)...);
        }
#else
        inline int push_args(lua_State *) { return 0; }

#define METAENGINE_LUAWRAPPER_PUSH_DEF(N)                                                          \
    c += lua_type_traits<METAENGINE_LUAWRAPPER_PP_CAT(A, N)>::push(                                \
            l, METAENGINE_LUAWRAPPER_PP_CAT(a, N));
#define METAENGINE_LUAWRAPPER_PUSH_ARG_DEF(N)                                                      \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    inline int push_args(lua_State *l, METAENGINE_LUAWRAPPER_PP_ARG_CR_DEF_REPEAT(N)) {            \
        int c = 0;                                                                                 \
        METAENGINE_LUAWRAPPER_PP_REPEAT(N, METAENGINE_LUAWRAPPER_PUSH_DEF)                         \
        return c;                                                                                  \
    }
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_PUSH_ARG_DEF)
#undef METAENGINE_LUAWRAPPER_PUSH_DEF
#undef METAENGINE_LUAWRAPPER_PUSH_ARG_DEF
#endif

#if METAENGINE_LUAWRAPPER_USE_CPP11
        template<typename T>
        inline bool one_push(lua_State *state, T &&v) {
            int count = util::push_args(state, std::forward<T>(v));
            if (count > 1) { lua_pop(state, count - 1); }
            return count != 0;
        }
#else
        template<typename T>
        inline bool one_push(lua_State *state, const T &v) {
            int count = util::push_args(state, v);
            if (count > 1) { lua_pop(state, count - 1); }
            return count != 0;
        }
#endif

        inline std::string pretty_name(const std::type_info &t) {
#if defined(METAENGINE_LUAWRAPPER_USE_CXX_ABI_DEMANGLE)
            int status = 0;
            char *demangle_name = abi::__cxa_demangle(t.name(), 0, 0, &status);
            struct deleter
            {
                char *data;
                deleter(char *d) : data(d) {}
                ~deleter() { std::free(data); }
            } d(demangle_name);
            return demangle_name;
#else
            return t.name();
#endif
        }
    }// namespace util
}// namespace LuaWrapper

namespace LuaWrapper {
    namespace types {
        template<typename T>
        struct typetag
        {
        };
    }// namespace types
#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
    inline const char *metatable_name_key() { return "\x80METAENGINE_LUAWRAPPER_N_KEY"; }
    inline const char *metatable_type_table_key() { return "\x80METAENGINE_LUAWRAPPER_T_KEY"; }
#else
    inline void *metatable_name_key() {
        static int key;
        return &key;
    }
    inline void *metatable_type_table_key() {
        static int key;
        return &key;
    }
#endif

    template<typename T>
    const std::type_info &metatableType() {
        return typeid(typename traits::decay<T>::type);
    }
    template<typename T>
    inline std::string metatableName() {
        return util::pretty_name(metatableType<T>());
    }

    struct ObjectWrapperBase
    {
        virtual const void *cget() = 0;
        virtual void *get() = 0;

        virtual const std::type_info &type() = 0;

        virtual const std::type_info &native_type() { return type(); }
        virtual void *native_get() { return get(); }

        ObjectWrapperBase() {}
        virtual ~ObjectWrapperBase() {}

    private:
        // noncopyable
        ObjectWrapperBase(const ObjectWrapperBase &);
        ObjectWrapperBase &operator=(const ObjectWrapperBase &);
    };

    template<class T>
    struct ObjectWrapper : ObjectWrapperBase
    {
#if METAENGINE_LUAWRAPPER_USE_CPP11
        template<class... Args>
        ObjectWrapper(Args &&...args) : object_(std::forward<Args>(args)...) {}
#else

        ObjectWrapper() : object_() {}
#define METAENGINE_LUAWRAPPER_OBJECT_WRAPPER_CONSTRUCTOR_DEF(N)                                    \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    ObjectWrapper(METAENGINE_LUAWRAPPER_PP_ARG_DEF_REPEAT(N))                                      \
        : object_(METAENGINE_LUAWRAPPER_PP_ARG_REPEAT(N)) {}

        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_OBJECT_WRAPPER_CONSTRUCTOR_DEF)
#undef METAENGINE_LUAWRAPPER_OBJECT_WRAPPER_CONSTRUCTOR_DEF
#endif

        virtual const std::type_info &type() { return metatableType<T>(); }

        virtual void *get() { return &object_; }
        virtual const void *cget() { return &object_; }

    private:
        T object_;
    };

    struct ObjectSharedPointerWrapper : ObjectWrapperBase
    {
        template<typename T>
        ObjectSharedPointerWrapper(const standard::shared_ptr<T> &sptr)
            : object_(standard::const_pointer_cast<typename standard::remove_const<T>::type>(sptr)),
              type_(metatableType<T>()),
              shared_ptr_type_(
                      metatableType<standard::shared_ptr<typename traits::decay<T>::type>>()),
              const_value_(traits::is_const<T>::value) {}

        template<typename T>
        ObjectSharedPointerWrapper(standard::shared_ptr<T> &&sptr)
            : object_(std::move(
                      standard::const_pointer_cast<typename standard::remove_const<T>::type>(
                              sptr))),
              type_(metatableType<T>()),
              shared_ptr_type_(
                      metatableType<standard::shared_ptr<typename traits::decay<T>::type>>()),
              const_value_(traits::is_const<T>::value) {}

        virtual const std::type_info &type() { return type_; }
        virtual void *get() { return const_value_ ? 0 : object_.get(); }
        virtual const void *cget() { return object_.get(); }
        standard::shared_ptr<void> object() const {
            return const_value_ ? standard::shared_ptr<void>() : object_;
        }
        standard::shared_ptr<const void> const_object() const { return object_; }
        const std::type_info &shared_ptr_type() const { return shared_ptr_type_; }

        virtual const std::type_info &native_type() {
            return metatableType<standard::shared_ptr<void>>();
        }
        virtual void *native_get() { return &object_; }

    private:
        standard::shared_ptr<void> object_;
        const std::type_info &type_;

        const std::type_info &shared_ptr_type_;
        bool const_value_;
    };

    template<typename T, typename ElementType = typename T::element_type>
    struct ObjectSmartPointerWrapper : ObjectWrapperBase
    {
        ObjectSmartPointerWrapper(const T &sptr) : object_(sptr) {}
        ObjectSmartPointerWrapper(T &&sptr) : object_(std::move(sptr)) {}

        virtual const std::type_info &type() { return metatableType<ElementType>(); }
        virtual void *get() { return object_ ? &(*object_) : 0; }
        virtual const void *cget() { return object_ ? &(*object_) : 0; }
        virtual const std::type_info &native_type() { return metatableType<T>(); }
        virtual void *native_get() { return &object_; }

    private:
        T object_;
    };

    template<class T>
    struct ObjectPointerWrapper : ObjectWrapperBase
    {
        ObjectPointerWrapper(T *ptr) : object_(ptr) {}

        virtual const std::type_info &type() { return metatableType<T>(); }
        virtual void *get() {
            if (traits::is_const<T>::value) { return 0; }
            return const_cast<void *>(static_cast<const void *>(object_));
        }
        virtual const void *cget() { return object_; }
        ~ObjectPointerWrapper() {}

    protected:
        T *object_;
    };

    // Customizable for ObjectPointerWrapper
    template<class T, class Enable = void>
    struct ObjectPointerWrapperType
    {
        typedef ObjectPointerWrapper<T> type;
    };

    // for internal use
    struct PointerConverter
    {
        template<typename T, typename F>
        static void *base_pointer_cast(void *from) {
            return static_cast<T *>(static_cast<F *>(from));
        }
        template<typename T, typename F>
        static standard::shared_ptr<void> base_shared_pointer_cast(
                const standard::shared_ptr<void> &from) {
            return standard::shared_ptr<T>(standard::static_pointer_cast<F>(from));
        }

        typedef void *(*convert_function_type)(void *);
        typedef standard::shared_ptr<void> (*shared_ptr_convert_function_type)(
                const standard::shared_ptr<void> &);
        typedef std::pair<std::string, std::string> convert_map_key;

        template<typename ToType, typename FromType>
        void add_type_conversion() {
            add_function(metatableType<ToType>(), metatableType<FromType>(),
                         &base_pointer_cast<ToType, FromType>);
            add_function(metatableType<standard::shared_ptr<ToType>>(),
                         metatableType<standard::shared_ptr<FromType>>(),
                         &base_shared_pointer_cast<ToType, FromType>);
        }

        template<typename TO>
        TO *get_pointer(ObjectWrapperBase *from) const {
            const std::type_info &to_type = metatableType<TO>();
            // unreachable
            // if (to_type == from->type())
            //{
            //	return static_cast<TO*>(from->get());
            //}
            std::map<convert_map_key, std::vector<convert_function_type>>::const_iterator match =
                    function_map_.find(convert_map_key(to_type.name(), from->type().name()));
            if (match != function_map_.end()) {
                return static_cast<TO *>(pcvt_list_apply(from->get(), match->second));
            }
            return 0;
        }
        template<typename TO>
        const TO *get_const_pointer(ObjectWrapperBase *from) const {
            const std::type_info &to_type = metatableType<TO>();
            // unreachable
            // if (to_type == from->type())
            //{
            //	return static_cast<const TO*>(from->cget());
            //}
            std::map<convert_map_key, std::vector<convert_function_type>>::const_iterator match =
                    function_map_.find(convert_map_key(to_type.name(), from->type().name()));
            if (match != function_map_.end()) {
                return static_cast<const TO *>(
                        pcvt_list_apply(const_cast<void *>(from->cget()), match->second));
            }
            return 0;
        }

        template<typename TO>
        standard::shared_ptr<TO> get_shared_pointer(ObjectSharedPointerWrapper *from) const {
            const std::type_info &to_type =
                    metatableType<standard::shared_ptr<typename traits::decay<TO>::type>>();
            // unreachable
            //			if (to_type == from->type())
            //			{
            //				return
            // standard::static_pointer_cast<TO>(from->object());
            //			}
            const std::type_info &from_type = from->shared_ptr_type();
            std::map<convert_map_key, std::vector<shared_ptr_convert_function_type>>::const_iterator
                    match = shared_ptr_function_map_.find(
                            convert_map_key(to_type.name(), from_type.name()));
            if (match != shared_ptr_function_map_.end()) {
                standard::shared_ptr<void> sptr = from->object();

                if (!sptr && standard::is_const<TO>::value) {
                    sptr = standard::const_pointer_cast<void>(from->const_object());
                }

                return standard::static_pointer_cast<TO>(pcvt_list_apply(sptr, match->second));
            }
            return standard::shared_ptr<TO>();
        }

        template<class T>
        T *get_pointer(ObjectWrapperBase *from, types::typetag<T>) {
            return get_pointer<T>(from);
        }
        template<class T>
        standard::shared_ptr<T> get_pointer(ObjectWrapperBase *from,
                                            types::typetag<standard::shared_ptr<T>>) {
            ObjectSharedPointerWrapper *ptr = dynamic_cast<ObjectSharedPointerWrapper *>(from);
            if (ptr) { return get_shared_pointer<T>(ptr); }
            return standard::shared_ptr<T>();
        }

        static int deleter(lua_State *state) {
            PointerConverter *ptr = (PointerConverter *) lua_touserdata(state, 1);
            ptr->~PointerConverter();
            return 0;
        }

        static PointerConverter &get(lua_State *state) {
#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
            const char *LuaWrapper_ptrcvt_key_ptr = "\x80METAENGINE_LUAWRAPPER_CVT_KEY";
#else
            static char LuaWrapper_ptrcvt_key_ptr;
#endif
            util::ScopedSavedStack save(state);
#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
            lua_pushstring(state, LuaWrapper_ptrcvt_key_ptr);
#else
            lua_pushlightuserdata(state, &LuaWrapper_ptrcvt_key_ptr);
#endif
            lua_rawget(state, LUA_REGISTRYINDEX);
            if (lua_isuserdata(state, -1)) {
                return *static_cast<PointerConverter *>(lua_touserdata(state, -1));
            } else {
                void *ptr =
                        lua_newuserdata(state, sizeof(PointerConverter));// dummy data for gc call
                PointerConverter *converter = new (ptr) PointerConverter();

                lua_createtable(state, 0, 2);
                lua_pushcclosure(state, &deleter, 0);
                lua_setfield(state, -2, "__gc");
                lua_pushvalue(state, -1);
                lua_setfield(state, -2, "__index");
                lua_setmetatable(state, -2);// set to userdata
#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
                lua_pushstring(state, LuaWrapper_ptrcvt_key_ptr);
#else
                lua_pushlightuserdata(state, &LuaWrapper_ptrcvt_key_ptr);
#endif
                lua_pushvalue(state, -2);
                lua_rawset(state, LUA_REGISTRYINDEX);
                return *converter;
            }
        }

    private:
        void add_function(const std::type_info &to_type, const std::type_info &from_type,
                          convert_function_type f) {
            std::map<convert_map_key, std::vector<convert_function_type>> add_map;
            for (std::map<convert_map_key, std::vector<convert_function_type>>::iterator it =
                         function_map_.begin();
                 it != function_map_.end(); ++it) {
                if (it->first.second == to_type.name()) {
                    std::vector<convert_function_type> newlist;
                    newlist.push_back(f);
                    newlist.insert(newlist.end(), it->second.begin(), it->second.end());
                    add_map[convert_map_key(it->first.first, from_type.name())] = newlist;
                }
            }
            function_map_.insert(add_map.begin(), add_map.end());

            std::vector<convert_function_type> flist;
            flist.push_back(f);
            function_map_[convert_map_key(to_type.name(), from_type.name())] = flist;
        }
        void add_function(const std::type_info &to_type, const std::type_info &from_type,
                          shared_ptr_convert_function_type f) {
            std::map<convert_map_key, std::vector<shared_ptr_convert_function_type>> add_map;
            for (std::map<convert_map_key, std::vector<shared_ptr_convert_function_type>>::iterator
                         it = shared_ptr_function_map_.begin();
                 it != shared_ptr_function_map_.end(); ++it) {
                if (it->first.second == to_type.name()) {
                    std::vector<shared_ptr_convert_function_type> newlist;
                    newlist.push_back(f);
                    newlist.insert(newlist.end(), it->second.begin(), it->second.end());
                    add_map[convert_map_key(it->first.first, from_type.name())] = newlist;
                }
            }
            shared_ptr_function_map_.insert(add_map.begin(), add_map.end());

            std::vector<shared_ptr_convert_function_type> flist;
            flist.push_back(f);
            shared_ptr_function_map_[convert_map_key(to_type.name(), from_type.name())] = flist;
        }

        void *pcvt_list_apply(void *ptr, const std::vector<convert_function_type> &flist) const {
            for (std::vector<convert_function_type>::const_iterator i = flist.begin();
                 i != flist.end(); ++i) {
                ptr = (*i)(ptr);
            }
            return ptr;
        }
        standard::shared_ptr<void> pcvt_list_apply(
                standard::shared_ptr<void> ptr,
                const std::vector<shared_ptr_convert_function_type> &flist) const {
            for (std::vector<shared_ptr_convert_function_type>::const_iterator i = flist.begin();
                 i != flist.end(); ++i) {

#if METAENGINE_LUAWRAPPER_USE_CPP11
                ptr = (*i)(std::move(ptr));
#else
                ptr = (*i)(ptr);
#endif
            }
            return ptr;
        }

        PointerConverter() {}

        std::map<convert_map_key, std::vector<convert_function_type>> function_map_;
        std::map<convert_map_key, std::vector<shared_ptr_convert_function_type>>
                shared_ptr_function_map_;

        PointerConverter(PointerConverter &);
        PointerConverter &operator=(PointerConverter &);
    };

    namespace detail {
        inline bool object_wrapper_type_check(lua_State *l, int index) {
#if METAENGINE_LUAWRAPPER_NO_USERDATA_TYPE_CHECK
            return lua_isuserdata(l, index) && !lua_islightuserdata(l, index);
#endif
            if (lua_getmetatable(l, index)) {
#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
                lua_pushstring(l, metatable_name_key());
#else
                lua_pushlightuserdata(l, metatable_name_key());
#endif
                int type = lua_rawget_rtype(l, -2);
                lua_pop(l, 2);
                return type == LUA_TSTRING;
            }
            return false;
        }
    }// namespace detail

    inline ObjectWrapperBase *object_wrapper(lua_State *l, int index) {
        if (detail::object_wrapper_type_check(l, index)) {
            ObjectWrapperBase *ptr = static_cast<ObjectWrapperBase *>(lua_touserdata(l, index));
            return ptr;
        }
        return 0;
    }

    template<typename RequireType>
    inline ObjectWrapperBase *object_wrapper(
            lua_State *l, int index, bool convert = true,
            types::typetag<RequireType> = types::typetag<RequireType>()) {
        if (detail::object_wrapper_type_check(l, index)) {
            ObjectWrapperBase *ptr = static_cast<ObjectWrapperBase *>(lua_touserdata(l, index));
#if METAENGINE_LUAWRAPPER_NAME_BASED_TYPE_CHECK
            if (strcmp(ptr->type().name(), metatableType<RequireType>().name()) == 0) {
#else
            if (ptr->type() == metatableType<RequireType>()) {
#endif
                return ptr;
            } else if (convert) {
                PointerConverter &pcvt = PointerConverter::get(l);
                return pcvt.get_pointer(ptr, types::typetag<RequireType>()) ? ptr : 0;
            }
            return 0;
        }
        return 0;
    }

    template<class T>
    T *get_pointer(lua_State *l, int index, types::typetag<T>) {
        int type = lua_type(l, index);

        if (type == LUA_TLIGHTUSERDATA) {
            return (T *) lua_topointer(l, index);
        } else if (type != LUA_TUSERDATA) {
            return 0;
        } else {
            ObjectWrapperBase *objwrapper = object_wrapper(l, index);
            if (objwrapper) {
                const std::type_info &to_type = metatableType<T>();
#if METAENGINE_LUAWRAPPER_NAME_BASED_TYPE_CHECK
                if (strcmp(objwrapper->type().name(), to_type.name()) == 0) {
#else
                if (objwrapper->type() == to_type) {
#endif
                    return static_cast<T *>(objwrapper->get());
                }
                if (objwrapper->native_type() == to_type) {
                    return static_cast<T *>(objwrapper->native_get());
                } else {
                    PointerConverter &pcvt = PointerConverter::get(l);
                    return pcvt.get_pointer<T>(objwrapper);
                }
            }
        }
        return 0;
    }
    template<class T>
    const T *get_const_pointer(lua_State *l, int index, types::typetag<T>) {
        int type = lua_type(l, index);

        if (type == LUA_TLIGHTUSERDATA) {
            return (T *) lua_topointer(l, index);
        } else if (type != LUA_TUSERDATA) {
            return 0;
        } else {
            ObjectWrapperBase *objwrapper = object_wrapper(l, index);
            if (objwrapper) {
#if METAENGINE_LUAWRAPPER_NAME_BASED_TYPE_CHECK
                if (strcmp(objwrapper->type().name(), metatableType<T>().name()) == 0) {
#else
                if (objwrapper->type() == metatableType<T>()) {
#endif
                    return static_cast<const T *>(objwrapper->cget());
                } else {
                    PointerConverter &pcvt = PointerConverter::get(l);
                    return pcvt.get_const_pointer<T>(objwrapper);
                }
            }
        }
        return 0;
    }
    template<class T>
    const T *get_pointer(lua_State *l, int index, types::typetag<const T>) {
        return get_const_pointer<T>(l, index, types::typetag<T>());
    }

    template<class T>
    standard::shared_ptr<T> get_shared_pointer(lua_State *l, int index, types::typetag<T>) {
        ObjectSharedPointerWrapper *ptr =
                dynamic_cast<ObjectSharedPointerWrapper *>(object_wrapper(l, index));
        if (ptr) {
            const std::type_info &from_type = ptr->shared_ptr_type();
            const std::type_info &to_type =
                    metatableType<standard::shared_ptr<typename traits::decay<T>::type>>();
#if METAENGINE_LUAWRAPPER_NAME_BASED_TYPE_CHECK
            if (strcmp(from_type.name(), to_type.name()) == 0) {
#else
            if (from_type == to_type) {
#endif
                if (standard::is_const<T>::value) {
                    return standard::static_pointer_cast<T>(
                            standard::const_pointer_cast<void>(ptr->const_object()));
                } else {
                    return standard::static_pointer_cast<T>(ptr->object());
                }
            }
            PointerConverter &pcvt = PointerConverter::get(l);
            return pcvt.get_shared_pointer<T>(ptr);
        }
        return standard::shared_ptr<T>();
    }
    inline standard::shared_ptr<void> get_shared_pointer(lua_State *l, int index,
                                                         types::typetag<void>) {
        ObjectSharedPointerWrapper *ptr =
                dynamic_cast<ObjectSharedPointerWrapper *>(object_wrapper(l, index));
        if (ptr) { return ptr->object(); }
        return standard::shared_ptr<void>();
    }
    inline standard::shared_ptr<const void> get_shared_pointer(lua_State *l, int index,
                                                               types::typetag<const void>) {
        ObjectSharedPointerWrapper *ptr =
                dynamic_cast<ObjectSharedPointerWrapper *>(object_wrapper(l, index));
        if (ptr) { return ptr->const_object(); }
        return standard::shared_ptr<const void>();
    }

    namespace class_userdata {
        template<typename T>
        inline void destructor(T *pointer) {
            if (pointer) { pointer->~T(); }
        }
        inline bool get_metatable(lua_State *l, const std::type_info &typeinfo) {
#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
            lua_pushstring(l, metatable_type_table_key());
#else
            lua_pushlightuserdata(l, metatable_type_table_key());
#endif
            int ttype = lua_rawget_rtype(l, LUA_REGISTRYINDEX);// get metatable registry table
            if (ttype != LUA_TTABLE) {
#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
                lua_pushstring(l, metatable_type_table_key());
#else
                lua_pushlightuserdata(l, metatable_type_table_key());
#endif
                lua_newtable(l);
                lua_rawset(l, LUA_REGISTRYINDEX);
                return false;
            }
#if METAENGINE_LUAWRAPPER_NAME_BASED_TYPE_CHECK
            lua_pushstring(l, typeinfo.name());
            int type = lua_rawget_rtype(l, -2);
#else
            int type = lua_rawgetp_rtype(l, -1, &typeinfo);
#endif
            lua_remove(l, -2);// remove metatable registry table
            return type != LUA_TNIL;
        }
        template<typename T>
        bool get_metatable(lua_State *l) {
            return get_metatable(l, metatableType<T>());
        }
        template<typename T>
        bool available_metatable(lua_State *l) {
            util::ScopedSavedStack save(l);
            return get_metatable<T>(l);
        }

        inline bool newmetatable(lua_State *l, const std::type_info &typeinfo, const char *name) {
            if (get_metatable(l, typeinfo))// already register
            {
                return false;//
            }
            lua_pop(l, 1);

// get metatable registry table
#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
            lua_getfield(l, LUA_REGISTRYINDEX, metatable_type_table_key());
#else
            lua_rawgetp_rtype(l, LUA_REGISTRYINDEX, metatable_type_table_key());
#endif

            int metaregindex = lua_absindex(l, -1);

            lua_createtable(l, 0, 2);
            lua_pushstring(l, name);
            lua_setfield(l, -2, "__name");// metatable.__name = name

#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
            lua_pushstring(l, metatable_name_key());
#else
            lua_pushlightuserdata(l, metatable_name_key());
#endif
            lua_pushstring(l, name);
            lua_rawset(l, -3);
#if METAENGINE_LUAWRAPPER_NAME_BASED_TYPE_CHECK
            lua_pushstring(l, typeinfo.name());
            lua_pushvalue(l, -2);
            lua_rawset(l, metaregindex);
#else
            lua_pushvalue(l, -1);
            lua_rawsetp(l, metaregindex, &typeinfo);
#endif
            lua_remove(l, metaregindex);// remove metatable registry table

            return true;
        }
        template<typename T>
        bool newmetatable(lua_State *l) {
            return newmetatable(l, metatableType<T>(), metatableName<T>().c_str());
        }

        template<typename T>
        inline int deleter(lua_State *state) {
            T *ptr = (T *) lua_touserdata(state, 1);
            ptr->~T();
            return 0;
        }
        struct UnknownType
        {
        };
        inline void setmetatable(lua_State *l, const std::type_info &typeinfo) {
            // if not available metatable, set unknown class metatable
            if (!get_metatable(l, typeinfo)) {
                lua_pop(l, 1);
                if (!get_metatable<UnknownType>(l)) {
                    lua_pop(l, 1);
                    newmetatable<UnknownType>(l);
                    lua_pushcclosure(l, &deleter<ObjectWrapperBase>, 0);
                    lua_setfield(l, -2, "__gc");
                }
            }
            lua_setmetatable(l, -2);
        }
        template<typename T>
        void setmetatable(lua_State *l) {
            setmetatable(l, metatableType<T>());
        }
    }// namespace class_userdata
    template<typename T>
    bool available_metatable(lua_State *l, types::typetag<T> = types::typetag<T>()) {
        return class_userdata::available_metatable<T>(l);
    }

    namespace util {
        template<typename T>
        inline bool object_checkType(lua_State *l, int index) {
            return object_wrapper<T>(l, index) != 0;
        }
        template<typename T>
        inline bool object_strictCheckType(lua_State *l, int index) {
            return object_wrapper<T>(l, index, false) != 0;
        }

        template<typename T>
        inline optional<T> object_getopt(lua_State *l, int index) {
            const typename traits::remove_reference<T>::type *pointer = get_const_pointer(
                    l, index, types::typetag<typename traits::remove_reference<T>::type>());
            if (!pointer) { return optional<T>(); }
            return *pointer;
        }

        template<typename T>
        inline T object_get(lua_State *l, int index) {
            const typename traits::remove_reference<T>::type *pointer = get_const_pointer(
                    l, index, types::typetag<typename traits::remove_reference<T>::type>());
            if (!pointer) { throw LuaTypeMismatch(); }
            return *pointer;
        }

        template<typename To>
        struct ConvertibleRegisterHelperProxy
        {
            template<typename DataType>
            ConvertibleRegisterHelperProxy(DataType v) : holder_(new DataHolder<DataType>(v)) {}
            operator To() { return holder_->get(); }

        private:
            struct DataHolderBase
            {
                virtual To get() const = 0;
                virtual ~DataHolderBase() {}
            };
            template<typename Type>
            class DataHolder : public DataHolderBase {
                typedef typename traits::decay<Type>::type DataType;

            public:
                explicit DataHolder(DataType v) : data_(v) {}
                virtual To get() const { return To(data_); }

            private:
                DataType data_;
            };
            standard::shared_ptr<DataHolderBase> holder_;
        };

#if METAENGINE_LUAWRAPPER_USE_CPP11
        template<typename T>
        inline int object_push(lua_State *l, T &&v) {
            typedef ObjectWrapper<typename traits::remove_const_and_reference<T>::type>
                    wrapper_type;
            void *storage = lua_newuserdata(l, sizeof(wrapper_type));
            new (storage) wrapper_type(std::forward<T>(v));
            class_userdata::setmetatable<T>(l);
            return 1;
        }

        namespace conv_helper_detail {
            template<class To>
            bool checkType(lua_State *, int) {
                return false;
            }
            template<class To, class From, class... Remain>
            bool checkType(lua_State *l, int index) {
                return lua_type_traits<From>::checkType(l, index) ||
                       checkType<To, Remain...>(l, index);
            }
            template<class To>
            bool strictCheckType(lua_State *, int) {
                return false;
            }
            template<class To, class From, class... Remain>
            bool strictCheckType(lua_State *l, int index) {
                return lua_type_traits<From>::strictCheckType(l, index) ||
                       strictCheckType<To, Remain...>(l, index);
                ;
            }

            template<class To>
            To get(lua_State *, int) {
                throw LuaTypeMismatch();
            }
            template<class To, class From, class... Remain>
            To get(lua_State *l, int index) {
                typedef optional<typename lua_type_traits<From>::get_type> opt_type;
                if (auto opt = lua_type_traits<opt_type>::get(l, index)) { return *opt; }
                return get<To, Remain...>(l, index);
            }
        }// namespace conv_helper_detail

        template<class To, class... From>
        struct ConvertibleRegisterHelper
        {
            typedef To get_type;

            static bool checkType(lua_State *l, int index) {
                if (object_checkType<To>(l, index)) { return true; }
                return conv_helper_detail::checkType<To, From...>(l, index);
            }
            static bool strictCheckType(lua_State *l, int index) {
                if (object_strictCheckType<To>(l, index)) { return true; }
                return conv_helper_detail::strictCheckType<To, From...>(l, index);
            }

            static get_type get(lua_State *l, int index) {
                if (auto opt = object_getopt<To>(l, index)) { return *opt; }
                return conv_helper_detail::get<get_type, From...>(l, index);
            }
        };
#else
        template<typename T>
        inline int object_push(lua_State *l, T v) {
            typedef ObjectWrapper<typename traits::remove_const_and_reference<T>::type>
                    wrapper_type;
            void *storage = lua_newuserdata(l, sizeof(wrapper_type));
            new (storage) wrapper_type(v);
            class_userdata::setmetatable<T>(l);
            return 1;
        }
        namespace conv_helper_detail {
#define METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_CHECK_TYPE_REP(N)                             \
    || lua_type_traits<METAENGINE_LUAWRAPPER_PP_CAT(A, N)>::checkType(state, index)
#define METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_STRICT_CHECK_TYPE_REP(N)                      \
    || lua_type_traits<METAENGINE_LUAWRAPPER_PP_CAT(A, N)>::strictCheckType(state, index)
#define METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_GET_OPT_TYPEDEF(N)                            \
    typedef optional<typename lua_type_traits<METAENGINE_LUAWRAPPER_PP_CAT(A, N)>::get_type>       \
            METAENGINE_LUAWRAPPER_PP_CAT(opt_type, N);
#define METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_GET_REP(N)                                    \
    if (METAENGINE_LUAWRAPPER_PP_CAT(opt_type, N) opt =                                            \
                lua_type_traits<METAENGINE_LUAWRAPPER_PP_CAT(opt_type, N)>::get(state, index)) {   \
        return *opt;                                                                               \
    } else

            template<typename To>
            bool checkType(lua_State *, int, TypeTuple<>) {
                return false;
            }
#define METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_CHECK_TYPE_DEF(N)                             \
    template<typename To, METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                         \
    bool checkType(lua_State *state, int index,                                                    \
                   TypeTuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>) {                   \
        return false METAENGINE_LUAWRAPPER_PP_REPEAT(                                              \
                N, METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_CHECK_TYPE_REP);                   \
    }

            METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(
                    METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                    METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_CHECK_TYPE_DEF)
#undef METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_CHECK_TYPE_DEF
            template<typename To>
            bool strictCheckType(lua_State *, int, TypeTuple<>) {
                return false;
            }
#define METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_ST_CHECK_TYPE_DEF(N)                          \
    template<typename To, METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                         \
    bool strictCheckType(lua_State *state, int index,                                              \
                         TypeTuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>) {             \
        return false METAENGINE_LUAWRAPPER_PP_REPEAT(                                              \
                N, METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_STRICT_CHECK_TYPE_REP);            \
    }
            METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(
                    METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                    METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_ST_CHECK_TYPE_DEF)
#undef METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_ST_CHECK_TYPE_DEF
#define METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_GET_DEF(N)                                    \
    template<typename To, METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                         \
    To get(lua_State *state, int index,                                                            \
           TypeTuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>) {                           \
        METAENGINE_LUAWRAPPER_PP_REPEAT(                                                           \
                N, METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_GET_OPT_TYPEDEF)                   \
        METAENGINE_LUAWRAPPER_PP_REPEAT(N, METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_GET_REP) { \
            throw LuaTypeMismatch();                                                               \
        }                                                                                          \
    }
            METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(
                    METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                    METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_GET_DEF)
#undef METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_GET_DEF

#undef METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_CHECK_TYPE_REP
#undef METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_STRICT_CHECK_TYPE_REP
#undef METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_ST_GET_REP
#undef METAENGINE_LUAWRAPPER_CONVERTIBLE_REG_HELPER_GET_REP
        }// namespace conv_helper_detail

#define METAENGINE_LUAWRAPPER_PP_CONVERTIBLE_REG_HELPER_DEF_REP(N)                                 \
    METAENGINE_LUAWRAPPER_PP_CAT(typename A, N) = null_type
#define METAENGINE_LUAWRAPPER_PP_CONVERTIBLE_REG_HELPER_DEF_REPEAT(N)                              \
    METAENGINE_LUAWRAPPER_PP_REPEAT_ARG(N, METAENGINE_LUAWRAPPER_PP_CONVERTIBLE_REG_HELPER_DEF_REP)

        template<typename To, METAENGINE_LUAWRAPPER_PP_CONVERTIBLE_REG_HELPER_DEF_REPEAT(
                                      METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS)>
        struct ConvertibleRegisterHelper
        {
            typedef To get_type;
            typedef TypeTuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(
                    METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS)>
                    conv_types;

            static bool checkType(lua_State *l, int index) {
                if (object_checkType<To>(l, index)) { return true; }
                return conv_helper_detail::checkType<To>(l, index, conv_types());
            }
            static bool strictCheckType(lua_State *l, int index) {
                if (object_strictCheckType<To>(l, index)) { return true; }
                return conv_helper_detail::strictCheckType<To>(l, index, conv_types());
            }
            static get_type get(lua_State *l, int index) {
                if (optional<To> opt = object_getopt<To>(l, index)) { return *opt; }
                return conv_helper_detail::get<get_type>(l, index, conv_types());
            }
        };
#undef METAENGINE_LUAWRAPPER_PP_CONVERTIBLE_REG_HELPER_DEF_REP
#undef METAENGINE_LUAWRAPPER_PP_CONVERTIBLE_REG_HELPER_DEF_REPEAT
#endif
    }// namespace util
}// namespace LuaWrapper

namespace LuaWrapper {
#if METAENGINE_LUAWRAPPER_USE_CPP11
    namespace detail {
        template<std::size_t... indexes>
        struct index_tuple
        {
        };
        template<std::size_t first, std::size_t last, class result = index_tuple<>,
                 bool flag = first >= last>
        struct index_range
        {
            using type = result;
        };
        template<std::size_t step, std::size_t last, std::size_t... indexes>
        struct index_range<step, last, index_tuple<indexes...>, false>
            : index_range<step + 1, last, index_tuple<indexes..., step>>
        {
        };

        template<std::size_t... Indexes, class... Args>
        int push_tuple(lua_State *l, index_tuple<Indexes...>, std::tuple<Args...> &&v) {
            return util::push_args(l, std::get<Indexes>(v)...);
        }
    }// namespace detail

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for std::tuple or boost::tuple
    template<class... Args>
    struct lua_type_traits<standard::tuple<Args...>>
    {
        static int push(lua_State *l, std::tuple<Args...> &&v) {
            typename detail::index_range<0, sizeof...(Args)>::type index;
            return detail::push_tuple(l, index, std::forward<std::tuple<Args...>>(v));
        }
    };
#else
#define METAENGINE_LUAWRAPPER_PP_GET_DATA(N) standard::get<N - 1>(v)
#define METAENGINE_LUAWRAPPER_PUSH_TUPLE_DEF(N)                                                    \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    struct lua_type_traits<standard::tuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>>       \
    {                                                                                              \
        static int push(                                                                           \
                lua_State *l,                                                                      \
                const standard::tuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)> &v) {       \
            return util::push_args(                                                                \
                    l, METAENGINE_LUAWRAPPER_PP_REPEAT_ARG(N, METAENGINE_LUAWRAPPER_PP_GET_DATA)); \
        }                                                                                          \
    };
    METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_TUPLE_SIZE,
                                        METAENGINE_LUAWRAPPER_PUSH_TUPLE_DEF)
#undef METAENGINE_LUAWRAPPER_PP_GET_DATA
#undef METAENGINE_LUAWRAPPER_PUSH_TUPLE_DEF
#endif
}// namespace LuaWrapper

namespace LuaWrapper {

    // default implements
    template<typename T, typename Enable>
    bool lua_type_traits<T, Enable>::checkType(lua_State *l, int index) {
        return object_wrapper<T>(l, index) != 0;
    }
    template<typename T, typename Enable>
    bool lua_type_traits<T, Enable>::strictCheckType(lua_State *l, int index) {
        return object_wrapper<T>(l, index, false) != 0;
    }
    template<typename T, typename Enable>
    typename lua_type_traits<T, Enable>::opt_type lua_type_traits<T, Enable>::opt(
            lua_State *l, int index) METAENGINE_LUAWRAPPER_NOEXCEPT {
        const typename traits::remove_reference<T>::type *pointer = get_const_pointer(
                l, index, types::typetag<typename traits::remove_reference<T>::type>());
        if (!pointer) { return opt_type(); }
        return *pointer;
    }
    template<typename T, typename Enable>
    typename lua_type_traits<T, Enable>::get_type lua_type_traits<T, Enable>::get(lua_State *l,
                                                                                  int index) {
        const typename traits::remove_reference<T>::type *pointer = get_const_pointer(
                l, index, types::typetag<typename traits::remove_reference<T>::type>());
        if (!pointer) { throw LuaTypeMismatch(); }
        return *pointer;
    }
    template<typename T, typename Enable>
    int lua_type_traits<T, Enable>::push(lua_State *l, push_type v) {
        return util::object_push(l, v);
    }

    template<typename T, typename Enable>
    int lua_type_traits<T, Enable>::push(lua_State *l, NCRT &&v) {
        return util::object_push(l, std::forward<NCRT>(v));
    }

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for const reference type
    template<typename T>
    struct lua_type_traits<T,
                           typename traits::enable_if<traits::is_const_reference<T>::value>::type>
        : lua_type_traits<typename traits::remove_const_reference<T>::type>
    {
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for lvalue reference type
    template<typename REF>
    struct lua_type_traits<
            REF,
            typename traits::enable_if<
                    traits::is_lvalue_reference<REF>::value &&
                    !traits::is_const<typename traits::remove_reference<REF>::type>::value>::type>
    {
        typedef void Registerable;

        typedef REF get_type;
        typedef optional<get_type> opt_type;
        typedef REF push_type;
        typedef typename traits::remove_reference<REF>::type T;

        static bool strictCheckType(lua_State *l, int index) {
            return object_wrapper<T>(l, index, false) != 0;
        }
        static bool checkType(lua_State *l, int index) {
            if (lua_type(l, index) == LUA_TLIGHTUSERDATA) { return true; }
            return object_wrapper<T>(l, index) != 0;
        }
        static get_type get(lua_State *l, int index) {
            T *pointer = get_pointer(l, index, types::typetag<T>());
            if (!pointer) { throw LuaTypeMismatch(); }
            return *pointer;
        }
        static opt_type opt(lua_State *l, int index) METAENGINE_LUAWRAPPER_NOEXCEPT {
            T *pointer = get_pointer(l, index, types::typetag<T>());
            if (!pointer) { return opt_type(); }
            return opt_type(*pointer);
        }
        static int push(lua_State *l, push_type v) {
            if (!available_metatable<T>(l)) {
                lua_pushlightuserdata(l, const_cast<typename traits::remove_const<T>::type *>(&v));
            } else {
                typedef typename ObjectPointerWrapperType<T>::type wrapper_type;
                void *storage = lua_newuserdata(l, sizeof(wrapper_type));
                new (storage) wrapper_type(&v);
                class_userdata::setmetatable<T>(l);
            }
            return 1;
        }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for pointer type
    template<typename PTR>
    struct lua_type_traits<
            PTR,
            typename traits::enable_if<
                    traits::is_pointer<typename traits::remove_const_reference<PTR>::type>::value &&
                    !traits::is_function<typename traits::remove_pointer<PTR>::type>::value>::type>
    {
        typedef void Registerable;

        typedef PTR get_type;
        typedef optional<get_type> opt_type;
        typedef PTR push_type;
        typedef typename traits::remove_pointer<PTR>::type T;

        static bool strictCheckType(lua_State *l, int index) {
            return object_wrapper<T>(l, index, false) != 0;
        }
        static bool checkType(lua_State *l, int index) {
            int type = lua_type(l, index);
            if (type == LUA_TLIGHTUSERDATA || type == LUA_TNIL || type == LUA_TNONE) {
                return true;
            }
            return object_wrapper<T>(l, index) != 0;
        }
        static get_type get(lua_State *l, int index) {
            int type = lua_type(l, index);
            if (type == LUA_TUSERDATA || type == LUA_TLIGHTUSERDATA) {
                return get_pointer(l, index, types::typetag<T>());
            }

            if (type == LUA_TNIL || type == LUA_TNONE) { return 0; }
            throw LuaTypeMismatch();
            return 0;
        }
        static opt_type opt(lua_State *l, int index) METAENGINE_LUAWRAPPER_NOEXCEPT {
            int type = lua_type(l, index);
            if (type == LUA_TUSERDATA || type == LUA_TLIGHTUSERDATA) {
                return get_pointer(l, index, types::typetag<T>());
            }
            if (type == LUA_TNIL || type == LUA_TNONE) { return opt_type(0); }
            return opt_type();
        }
        static int push(lua_State *l, push_type v) {
            if (!v) {
                lua_pushnil(l);
            } else if (!available_metatable<T>(l)) {
                lua_pushlightuserdata(l, const_cast<typename traits::remove_const<T>::type *>(v));
            } else {
                typedef typename ObjectPointerWrapperType<T>::type wrapper_type;
                void *storage = lua_newuserdata(l, sizeof(wrapper_type));
                new (storage) wrapper_type(v);
                class_userdata::setmetatable<T>(l);
            }
            return 1;
        }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for bool
    template<>
    struct lua_type_traits<bool>
    {
        typedef bool get_type;
        typedef optional<get_type> opt_type;
        typedef bool push_type;

        static bool strictCheckType(lua_State *l, int index) {
            return lua_type(l, index) == LUA_TBOOLEAN;
        }
        static bool checkType(lua_State *l, int index) {
            METAENGINE_LUAWRAPPER_UNUSED(l);
            METAENGINE_LUAWRAPPER_UNUSED(index);
            return true;
        }
        static bool get(lua_State *l, int index) { return l && lua_toboolean(l, index) != 0; }
        static opt_type opt(lua_State *l, int index) METAENGINE_LUAWRAPPER_NOEXCEPT {
            if (l) {
                return opt_type(lua_toboolean(l, index) != 0);
            } else {
                return opt_type();
            }
        }
        static int push(lua_State *l, bool s) {
            lua_pushboolean(l, s);
            return 1;
        }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for void
    template<>
    struct lua_type_traits<void>
    {
        typedef void *get_type;
        typedef void *push_type;

        static bool strictCheckType(lua_State *, int) { return true; }
        static bool checkType(lua_State *, int) { return true; }
        static get_type get(lua_State *, int) { return 0; }
        static int push(lua_State *, push_type) { return 0; }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for reference_wrapper
    template<typename T>
    struct lua_type_traits<standard::reference_wrapper<T>>
    {
        typedef const standard::reference_wrapper<T> &push_type;

        static int push(lua_State *l, push_type v) { return util::push_args(l, &v.get()); }
    };

    namespace detail {

        template<typename T, typename Enable = void>
        struct has_optional_get : traits::false_type
        {
        };
        template<typename T>
        struct has_optional_get<T,
                                typename traits::enable_if<!traits::is_same<
                                        void, typename lua_type_traits<T>::opt_type>::value>::type>
            : traits::true_type
        {
        };

        template<typename T>
        typename traits::enable_if<has_optional_get<T>::value, optional<T>>::type opt_helper(
                lua_State *state, int index) METAENGINE_LUAWRAPPER_NOEXCEPT {
            return lua_type_traits<T>::opt(state, index);
        }
        template<typename T>
        typename traits::enable_if<!has_optional_get<T>::value, optional<T>>::type opt_helper(
                lua_State *state, int index) {
            try {
                return lua_type_traits<T>::get(state, index);
            } catch (...) { return optional<T>(); }
        }
    }// namespace detail

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for optional
    template<typename T>
    struct lua_type_traits<optional<T>>
    {
        typedef const optional<T> &push_type;
        typedef optional<T> get_type;

        static bool strictCheckType(lua_State *l, int index) {
            return lua_type_traits<T>::strictCheckType(l, index);
        }
        static bool checkType(lua_State *l, int index) {
            METAENGINE_LUAWRAPPER_UNUSED(l);
            METAENGINE_LUAWRAPPER_UNUSED(index);
            return true;
        }
        static get_type get(lua_State *l, int index) METAENGINE_LUAWRAPPER_NOEXCEPT {
            return detail::opt_helper<T>(l, index);
        }

        static int push(lua_State *l, push_type v) METAENGINE_LUAWRAPPER_NOEXCEPT {
            if (v) {
                return util::push_args(l, v.value());
            } else {
                lua_pushnil(l);
            }
            return 1;
        }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for shared_ptr
    template<typename T>
    struct lua_type_traits<standard::shared_ptr<T>>
    {
        typedef const standard::shared_ptr<T> &push_type;
        typedef standard::shared_ptr<T> get_type;

        static bool strictCheckType(lua_State *l, int index) {
            ObjectSharedPointerWrapper *wrapper =
                    dynamic_cast<ObjectSharedPointerWrapper *>(object_wrapper(l, index));
            if (!wrapper) { return false; }
            const std::type_info &type =
                    metatableType<standard::shared_ptr<typename traits::decay<T>::type>>();
#if METAENGINE_LUAWRAPPER_NAME_BASED_TYPE_CHECK
            return strcmp(wrapper->shared_ptr_type().name(), type.name()) == 0;
#else
            return wrapper->shared_ptr_type() == type;
#endif
        }
        static bool checkType(lua_State *l, int index) {
            return get_shared_pointer(l, index, types::typetag<T>()) || lua_isnil(l, index);
        }
        static get_type get(lua_State *l, int index) {
            if (lua_isnil(l, index)) { return get_type(); }
            return get_shared_pointer(l, index, types::typetag<T>());
        }

        static int push(lua_State *l, push_type v) {
            if (v) {
                typedef ObjectSharedPointerWrapper wrapper_type;
                void *storage = lua_newuserdata(l, sizeof(wrapper_type));
                new (storage) wrapper_type(v);
                class_userdata::setmetatable<T>(l);
            } else {
                lua_pushnil(l);
            }
            return 1;
        }
    };
#if METAENGINE_LUAWRAPPER_USE_CPP11
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for unique_ptr
    template<typename T, typename Deleter>
    struct lua_type_traits<std::unique_ptr<T, Deleter>>
    {
        typedef std::unique_ptr<T, Deleter> &&push_type;
        typedef std::unique_ptr<T, Deleter> &get_type;
        typedef std::unique_ptr<T, Deleter> type;

        static bool strictCheckType(lua_State *l, int index) {
            return object_wrapper<type>(l, index, false) != 0;
        }
        static bool checkType(lua_State *l, int index) {
            return object_wrapper<type>(l, index) != 0 || lua_isnil(l, index);
        }
        static get_type get(lua_State *l, int index) {
            type *pointer = get_pointer(l, index, types::typetag<type>());
            if (!pointer) { throw LuaTypeMismatch(); }
            return *pointer;
        }

        static int push(lua_State *l, push_type v) {
            if (v) {
                typedef ObjectSmartPointerWrapper<type> wrapper_type;
                void *storage = lua_newuserdata(l, sizeof(wrapper_type));
                new (storage) wrapper_type(std::forward<push_type>(v));
                class_userdata::setmetatable<T>(l);
            } else {
                lua_pushnil(l);
            }
            return 1;
        }
    };
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for nullptr
    template<>
    struct lua_type_traits<std::nullptr_t>
    {
        typedef const std::nullptr_t &push_type;
        typedef std::nullptr_t get_type;
        typedef optional<get_type> opt_type;

        static bool checkType(lua_State *l, int index) { return lua_isnoneornil(l, index); }
        static bool strictCheckType(lua_State *l, int index) { return lua_isnil(l, index); }
        static opt_type opt(lua_State *l, int index) {
            if (!lua_isnoneornil(l, index)) { return opt_type(); }
            return nullptr;
        }
        static get_type get(lua_State *l, int index) {
            if (!lua_isnoneornil(l, index)) { throw LuaTypeMismatch(); }
            return nullptr;
        }

        static int push(lua_State *l, const std::nullptr_t &) {
            lua_pushnil(l);
            return 1;
        }
    };
#endif

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for ObjectWrapperBase*
    template<>
    struct lua_type_traits<ObjectWrapperBase *>
    {
        typedef ObjectWrapperBase *get_type;
        typedef ObjectWrapperBase *push_type;

        static bool strictCheckType(lua_State *l, int index) {
            return object_wrapper(l, index) != 0;
        }
        static bool checkType(lua_State *l, int index) { return object_wrapper(l, index) != 0; }
        static get_type get(lua_State *l, int index) { return object_wrapper(l, index); }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for native type of luathread(lua_State*)
    template<>
    struct lua_type_traits<lua_State *>
    {
        typedef lua_State *get_type;
        typedef lua_State *push_type;

        static bool strictCheckType(lua_State *l, int index) { return lua_isthread(l, index); }
        static bool checkType(lua_State *l, int index) { return lua_isthread(l, index); }
        static lua_State *get(lua_State *l, int index) { return lua_tothread(l, index); }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for floating point number value
    template<typename T>
    struct lua_type_traits<T, typename traits::enable_if<traits::is_floating_point<T>::value>::type>
    {
        typedef typename traits::remove_const_reference<T>::type get_type;
        typedef optional<get_type> opt_type;
        typedef lua_Number push_type;

        static bool strictCheckType(lua_State *l, int index) {
            return lua_type(l, index) == LUA_TNUMBER;
        }
        static bool checkType(lua_State *l, int index) { return lua_isnumber(l, index) != 0; }
        static opt_type opt(lua_State *l, int index) METAENGINE_LUAWRAPPER_NOEXCEPT {
            int isnum = 0;
            get_type num = static_cast<T>(lua_tonumberx(l, index, &isnum));
            if (!isnum) { return opt_type(); }
            return num;
        }
        static get_type get(lua_State *l, int index) {
            int isnum = 0;
            get_type num = static_cast<T>(lua_tonumberx(l, index, &isnum));
            if (!isnum) { throw LuaTypeMismatch(); }
            return num;
        }
        static int push(lua_State *l, lua_Number s) {
            lua_pushnumber(l, s);
            return 1;
        }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for integral number value
    template<typename T>
    struct lua_type_traits<T, typename traits::enable_if<traits::is_integral<T>::value>::type>
    {
        typedef typename traits::remove_const_reference<T>::type get_type;
        typedef optional<get_type> opt_type;
#if LUA_VERSION_NUM >= 503
        typedef lua_Integer push_type;

        static bool strictCheckType(lua_State *l, int index) {
            return lua_isinteger(l, index) != 0;
        }
        static bool checkType(lua_State *l, int index) { return lua_isnumber(l, index) != 0; }
        static opt_type opt(lua_State *l, int index) METAENGINE_LUAWRAPPER_NOEXCEPT {
            int isnum = 0;
            get_type num = static_cast<T>(lua_tointegerx(l, index, &isnum));
            if (!isnum) { return opt_type(); }
            return num;
        }
        static get_type get(lua_State *l, int index) {
            int isnum = 0;
            get_type num = static_cast<T>(lua_tointegerx(l, index, &isnum));
            if (!isnum) { throw LuaTypeMismatch(); }
            return num;
        }
        static int push(lua_State *l, lua_Integer s) {
            lua_pushinteger(l, s);
            return 1;
        }
#else
        typedef typename lua_type_traits<lua_Number>::push_type push_type;

        static bool strictCheckType(lua_State *l, int index) {
            return lua_type_traits<lua_Number>::strictCheckType(l, index);
        }
        static bool checkType(lua_State *l, int index) {
            return lua_type_traits<lua_Number>::checkType(l, index);
        }
        static get_type get(lua_State *l, int index) {
            return static_cast<get_type>(lua_type_traits<lua_Number>::get(l, index));
        }
        static opt_type opt(lua_State *l, int index) METAENGINE_LUAWRAPPER_NOEXCEPT {
            lua_type_traits<lua_Number>::opt_type v = lua_type_traits<lua_Number>::opt(l, index);
            if (!v) { return opt_type(); }
            return static_cast<get_type>(*v);
        }
        static int push(lua_State *l, push_type s) { return util::push_args(l, s); }
#endif
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for enum
    template<typename T>
    struct lua_type_traits<T, typename traits::enable_if<traits::is_enum<T>::value>::type>
    {
        typedef typename traits::remove_const_reference<T>::type get_type;
        typedef optional<get_type> opt_type;
        typedef typename traits::remove_const_reference<T>::type push_type;

        static bool strictCheckType(lua_State *l, int index) {
            return lua_type_traits<luaInt>::strictCheckType(l, index);
        }
        static bool checkType(lua_State *l, int index) {
            return lua_type_traits<luaInt>::checkType(l, index);
        }
        static opt_type opt(lua_State *l, int index) METAENGINE_LUAWRAPPER_NOEXCEPT {
            if (lua_type_traits<luaInt>::opt_type t = lua_type_traits<luaInt>::opt(l, index)) {
                return opt_type(static_cast<get_type>(*t));
            }
            return opt_type();
        }
        static get_type get(lua_State *l, int index) {
            return static_cast<get_type>(lua_type_traits<luaInt>::get(l, index));
        }
        static int push(lua_State *l, push_type s) {
            return util::push_args(l, static_cast<typename lua_type_traits<int64_t>::push_type>(s));
        }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for cstring
    template<>
    struct lua_type_traits<const char *>
    {
        typedef const char *get_type;
        typedef optional<get_type> opt_type;
        typedef const char *push_type;

        static bool strictCheckType(lua_State *l, int index) {
            return lua_type(l, index) == LUA_TSTRING;
        }
        static bool checkType(lua_State *l, int index) { return lua_isstring(l, index) != 0; }
        static get_type get(lua_State *l, int index) {
            const char *buffer = lua_tostring(l, index);
            if (!buffer) { throw LuaTypeMismatch(); }
            return buffer;
        }
        static opt_type opt(lua_State *l, int index) {
            const char *buffer = lua_tostring(l, index);
            if (!buffer) { return opt_type(); }
            return buffer;
        }
        static int push(lua_State *l, const char *s) {
            lua_pushstring(l, s);
            return 1;
        }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for cstring
    template<int N>
    struct lua_type_traits<char[N]>
    {
        typedef std::string get_type;
        typedef const char *push_type;

        static bool strictCheckType(lua_State *l, int index) {
            return lua_type(l, index) == LUA_TSTRING;
        }
        static bool checkType(lua_State *l, int index) { return lua_isstring(l, index) != 0; }
        static const char *get(lua_State *l, int index) {
            const char *buffer = lua_tostring(l, index);
            if (!buffer) { throw LuaTypeMismatch(); }
            return buffer;
        }
        static int push(lua_State *l, const char s[N]) {
            lua_pushlstring(l, s, s[N - 1] != '\0' ? N : N - 1);
            return 1;
        }
    };
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for cstring
    template<int N>
    struct lua_type_traits<const char[N]> : lua_type_traits<char[N]>
    {
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for std::string
    template<>
    struct lua_type_traits<std::string>
    {
        typedef std::string get_type;
        typedef optional<get_type> opt_type;
        typedef const std::string &push_type;

        static bool strictCheckType(lua_State *l, int index) {
            return lua_type(l, index) == LUA_TSTRING;
        }
        static bool checkType(lua_State *l, int index) { return lua_isstring(l, index) != 0; }
        static opt_type opt(lua_State *l, int index) METAENGINE_LUAWRAPPER_NOEXCEPT {
            size_t size = 0;
            const char *buffer = lua_tolstring(l, index, &size);
            if (!buffer) { return opt_type(); }
            return std::string(buffer, size);
        }
        static get_type get(lua_State *l, int index) {
            if (opt_type o = opt(l, index)) { return *o; }
            throw LuaTypeMismatch();
        }
        static int push(lua_State *l, const std::string &s) {
            lua_pushlstring(l, s.c_str(), s.size());
            return 1;
        }
    };

    struct NewTable
    {
        NewTable() : reserve_array_(0), reserve_record_(0) {}
        NewTable(int reserve_array, int reserve_record_)
            : reserve_array_(reserve_array), reserve_record_(reserve_record_) {}
        int reserve_array_;
        int reserve_record_;
    };
    struct NewThread
    {
    };
    struct GlobalTable
    {
    };
    struct NilValue
    {
    };

    struct NoTypeCheck
    {
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for NewTable, push only
    template<>
    struct lua_type_traits<NewTable>
    {
        static int push(lua_State *l, const NewTable &table) {
            lua_createtable(l, table.reserve_array_, table.reserve_record_);
            return 1;
        }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for NewThread, push only
    template<>
    struct lua_type_traits<NewThread>
    {
        static int push(lua_State *l, const NewThread &) {
            lua_newthread(l);
            return 1;
        }
    };
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for NilValue, similar to nullptr_t
    /// If you using C++11, recommend use nullptr instead.
    template<>
    struct lua_type_traits<NilValue>
    {
        typedef NilValue get_type;
        typedef optional<get_type> opt_type;
        typedef NilValue push_type;

        static bool checkType(lua_State *l, int index) { return lua_isnoneornil(l, index); }
        static bool strictCheckType(lua_State *l, int index) { return lua_isnil(l, index); }

        static opt_type opt(lua_State *l, int index) {
            if (!checkType(l, index)) { return opt_type(); }
            return NilValue();
        }
        static get_type get(lua_State *l, int index) {
            if (!checkType(l, index)) { throw LuaTypeMismatch(); }
            return NilValue();
        }
        static int push(lua_State *l, const NilValue &) {
            lua_pushnil(l);
            return 1;
        }
    };
    inline std::ostream &operator<<(std::ostream &os, const NilValue &) { return os << "nil"; }
    inline bool operator==(const NilValue &, const NilValue &) { return true; }
    inline bool operator!=(const NilValue &, const NilValue &) { return false; }

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for GlobalTable, push only
    template<>
    struct lua_type_traits<GlobalTable>
    {
        static int push(lua_State *l, const GlobalTable &) {
            lua_pushglobaltable(l);
            return 1;
        }
    };

    namespace detail {
        template<typename Derived>
        class LuaBasicTypeFunctions {
            template<class Other>
            friend class LuaBasicTypeFunctions;
            typedef void (LuaBasicTypeFunctions::*bool_type)() const;
            void this_type_does_not_support_comparisons() const {}

        public:
            enum value_type {
                TYPE_NONE = LUA_TNONE,                  //!< none type
                TYPE_NIL = LUA_TNIL,                    //!< nil type
                TYPE_BOOLEAN = LUA_TBOOLEAN,            //!< boolean type
                TYPE_LIGHTUSERDATA = LUA_TLIGHTUSERDATA,//!< light userdata type
                TYPE_NUMBER = LUA_TNUMBER,              //!< number type
                TYPE_STRING = LUA_TSTRING,              //!< string type
                TYPE_TABLE = LUA_TTABLE,                //!< table type
                TYPE_FUNCTION = LUA_TFUNCTION,          //!< function type
                TYPE_USERDATA = LUA_TUSERDATA,          //!< userdata type
                TYPE_THREAD = LUA_TTHREAD               //!< thread(coroutine) type
            };

            /// @brief If reference value is none or nil return true. Otherwise false.
            bool isNilref_() const {
                int t = type();
                return t == LUA_TNIL || t == LUA_TNONE;
            }

            /// @brief Equivalent to `#` operator for strings and tables with no
            /// metamethods.
            /// Follows Lua's reference manual documentation of `lua_rawlen`, ie. types
            /// other
            /// than tables, strings or userdatas return 0.
            /// @return Size of table, string length or userdata memory block size.
            size_t size() const {
                lua_State *state = state_();
                if (!state) { return 0; }
                util::ScopedSavedStack save(state);
                int index = pushStackIndex_(state);

                return lua_rawlen(state, index);
            }

            // return type
            int type() const {
                lua_State *state = state_();
                if (!state) { return LUA_TNONE; }
                util::ScopedSavedStack save(state);
                return lua_type(state, pushStackIndex_(state));
            }

            // return type name
            const char *typeName() const { return lua_typename(state_(), type()); }

            operator bool_type() const {
                lua_State *state = state_();
                if (!state) {
                    return 0;// hasn't lua_State
                }
                util::ScopedSavedStack save(state);
                int stackindex = pushStackIndex_(state);
                int t = lua_type(state, stackindex);
                if (t == LUA_TNONE) {
                    return 0;// none
                }
                return lua_toboolean(state, stackindex)
                               ? &LuaBasicTypeFunctions::this_type_does_not_support_comparisons
                               : 0;
            }

            /**
  * @name relational operators
  * @brief
  */
            //@{
            template<typename OtherDrived>
            inline bool operator==(const LuaBasicTypeFunctions<OtherDrived> &rhs) const {
                if (isNilref_() || rhs.isNilref_()) { return !isNilref_() == !rhs.isNilref_(); }
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                int index = pushStackIndex_(state);
                int rhsindex = rhs.pushStackIndex_(state);
                return lua_compare(state, index, rhsindex, LUA_OPEQ) != 0;
            }
            template<typename OtherDrived>
            inline bool operator<(const LuaBasicTypeFunctions<OtherDrived> &rhs) const {
                if (isNilref_() || rhs.isNilref_()) { return !isNilref_() != !rhs.isNilref_(); }
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                int index = pushStackIndex_(state);
                int rhsindex = rhs.pushStackIndex_(state);
                return lua_compare(state, index, rhsindex, LUA_OPLT) != 0;
            }
            template<typename OtherDrived>
            inline bool operator<=(const LuaBasicTypeFunctions<OtherDrived> &rhs) const {
                if (isNilref_() || rhs.isNilref_()) { return !isNilref_() == !rhs.isNilref_(); }
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                int index = pushStackIndex_(state);
                int rhsindex = rhs.pushStackIndex_(state);
                return lua_compare(state, index, rhsindex, LUA_OPLE) != 0;
            }
            template<typename OtherDrived>
            inline bool operator>=(const LuaBasicTypeFunctions<OtherDrived> &rhs) const {
                return rhs <= (*this);
            }
            template<typename OtherDrived>
            inline bool operator>(const LuaBasicTypeFunctions<OtherDrived> &rhs) const {
                return rhs < (*this);
            }
            template<typename OtherDrived>
            inline bool operator!=(const LuaBasicTypeFunctions<OtherDrived> &rhs) const {
                return !this->operator==(rhs);
            }

            template<typename T>
            inline typename traits::enable_if<
                    !traits::is_convertible<T *, LuaBasicTypeFunctions<T> *>::value, bool>::type
            operator==(const T &rhs) const {
                if (optional<typename lua_type_traits<T>::get_type> d = checkGet_<T>()) {
                    return *d == rhs;
                }
                return false;
            }
            template<typename T>
            inline typename traits::enable_if<
                    !traits::is_convertible<T *, LuaBasicTypeFunctions<T> *>::value, bool>::type
            operator!=(const T &rhs) const {
                return !((*this) == rhs);
            }
            //@}

            void dump(std::ostream &os) const {
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                int stackIndex = pushStackIndex_(state);
                util::stackValueDump(os, state, stackIndex);
            }

        private:
            lua_State *state_() const { return static_cast<const Derived *>(this)->state(); }
            int pushStackIndex_(lua_State *state) const {
                return static_cast<const Derived *>(this)->pushStackIndex(state);
            }
            template<typename T>
            optional<typename lua_type_traits<T>::get_type> checkGet_() const {
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                int stackindex = pushStackIndex_(state);
                return lua_type_traits<optional<typename lua_type_traits<T>::get_type>>::get(
                        state, stackindex);
            }
        };
        template<typename D>
        inline std::ostream &operator<<(std::ostream &os, const LuaBasicTypeFunctions<D> &ref) {
            ref.dump(os);
            return os;
        }
        /**
* @name relational operators
* @brief
*/
        //@{

#define METAENGINE_LUAWRAPPER_ENABLE_IF_NOT_LUAREF(RETTYPE)                                        \
    typename traits::enable_if<!traits::is_convertible<T *, LuaBasicTypeFunctions<T> *>::value,    \
                               RETTYPE>::type
        template<typename D, typename T>
        inline METAENGINE_LUAWRAPPER_ENABLE_IF_NOT_LUAREF(bool) operator==(
                const T &lhs, const LuaBasicTypeFunctions<D> &rhs) {
            return rhs == lhs;
        }
        template<typename D, typename T>
        inline METAENGINE_LUAWRAPPER_ENABLE_IF_NOT_LUAREF(bool) operator!=(
                const T &lhs, const LuaBasicTypeFunctions<D> &rhs) {
            return !(rhs == lhs);
        }
#undef METAENGINE_LUAWRAPPER_ENABLE_IF_NOT_LUAREF
        //@}
    }// namespace detail
}// namespace LuaWrapper

namespace LuaWrapper {
    inline const char *get_error_message(lua_State *state) {
        if (lua_type(state, -1) == LUA_TSTRING) {
            const char *message = lua_tostring(state, -1);
            return message ? message : "unknown error";
        } else {
            return "unknown error";
        }
    }
    inline int lua_pcall_wrap(lua_State *state, int argnum, int retnum) {
        int result = lua_pcall(state, argnum, retnum, 0);
        return result;
    }

    struct ErrorHandler
    {
        typedef standard::function<void(int, const char *)> function_type;

        static bool handle(const char *message, lua_State *state) {
            function_type *handler = getFunctionPointer(state);
            if (handler) {
                (*handler)(0, message);
                return true;
            }
            return false;
        }
        static bool handle(int status_code, const char *message, lua_State *state) {
            function_type *handler = getFunctionPointer(state);
            if (handler) {
                (*handler)(status_code, message);
                return true;
            }
            return false;
        }
        static bool handle(int status_code, lua_State *state) {
            function_type *handler = getFunctionPointer(state);
            if (handler) {
                (*handler)(status_code, get_error_message(state));
                return true;
            }
            return false;
        }

        static function_type getHandler(lua_State *state) {
            function_type *funptr = getFunctionPointer(state);
            if (funptr) { return *funptr; }
            return function_type();
        }

        static void unregisterHandler(lua_State *state) {
            if (state) {
                function_type *funptr = getFunctionPointer(state);
                if (funptr) { *funptr = function_type(); }
            }
        }
        static void registerHandler(lua_State *state, function_type f) {
            if (state) {
                function_type *funptr = getFunctionPointer(state);
                if (!funptr) {
                    util::ScopedSavedStack save(state);
#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
                    lua_pushstring(state, handlerRegistryKey());
#else
                    lua_pushlightuserdata(state, handlerRegistryKey());
#endif
                    void *ptr =
                            lua_newuserdata(state, sizeof(function_type));// dummy data for gc call
                    funptr = new (ptr) function_type();

                    // create function_type metatable
                    lua_newtable(state);
                    lua_pushcclosure(state, &error_handler_cleanner, 0);
                    lua_setfield(state, -2, "__gc");
                    lua_pushvalue(state, -1);
                    lua_setfield(state, -1, "__index");
                    lua_setmetatable(state, -2);

                    lua_rawset(state, LUA_REGISTRYINDEX);
                }
                *funptr = f;
            }
        }

        static void throwDefaultError(int status, const char *message = 0) {
            switch (status) {
                case LUA_ERRSYNTAX:
                    throw LuaSyntaxError(status,
                                         message ? std::string(message) : "unknown syntax error");
                case LUA_ERRRUN:
                    throw LuaRuntimeError(status,
                                          message ? std::string(message) : "unknown runtime error");
                case LUA_ERRMEM:
                    throw LuaMemoryError(status, message ? std::string(message)
                                                         : "lua memory allocation error");
                case LUA_ERRERR:
                    throw LuaErrorRunningError(status, message ? std::string(message)
                                                               : "unknown error running error");
#ifdef LUA_ERRGCMM
                case LUA_ERRGCMM:
                    throw LuaGCError(status, message ? std::string(message) : "unknown gc error");
#endif
                default:
                    throw LuaUnknownError(status,
                                          message ? std::string(message) : "lua unknown error");
            }
        }

    private:
#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
        static const char *handlerRegistryKey() {
            return "\x80METAENGINE_LUAWRAPPER_ERROR_HANDLER_REGISTRY_KEY";
        }
#else
        static void *handlerRegistryKey() {
            static void *key;
            return key;
        }
#endif
        static function_type *getFunctionPointer(lua_State *state) {
            if (state) {
                util::ScopedSavedStack save(state);
#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
                lua_pushstring(state, handlerRegistryKey());
#else
                lua_pushlightuserdata(state, handlerRegistryKey());
#endif
                lua_rawget(state, LUA_REGISTRYINDEX);
                function_type *ptr = (function_type *) lua_touserdata(state, -1);
                return ptr;
            }
            return 0;
        }

        ErrorHandler() {}

        ErrorHandler(const ErrorHandler &);
        ErrorHandler &operator=(const ErrorHandler &);

        static int error_handler_cleanner(lua_State *state) {
            function_type *ptr = (function_type *) lua_touserdata(state, 1);
            ptr->~function_type();
            return 0;
        }
    };

    namespace except {
        inline void OtherError(lua_State *state, const std::string &message) {
            if (ErrorHandler::handle(message.c_str(), state)) { return; }
        }
        inline void typeMismatchError(lua_State *state, const std::string &message) {
            if (ErrorHandler::handle(message.c_str(), state)) { return; }
        }
        inline void memoryError(lua_State *state, const char *message) {
            if (ErrorHandler::handle(message, state)) { return; }
        }
        inline bool checkErrorAndThrow(int status, lua_State *state) {
            if (status != 0 && status != LUA_YIELD) {
                ErrorHandler::handle(status, state);

                return false;
            }
            return true;
        }
    }// namespace except
}// namespace LuaWrapper

namespace LuaWrapper {
    /// @brief StackTop tag type
    struct StackTop
    {
    };

    namespace Ref {
        /// @brief NoMainCheck tag type
        struct NoMainCheck
        {
        };

        /// @brief reference to Lua stack value
        class StackRef {
        protected:
            lua_State *state_;
            int stack_index_;
            mutable bool pop_;
#if METAENGINE_LUAWRAPPER_USE_CPP11
            StackRef(StackRef &&src)
                : state_(src.state_), stack_index_(src.stack_index_), pop_(src.pop_) {
                src.pop_ = false;
            }
            StackRef &operator=(StackRef &&src) {
                state_ = src.state_;
                stack_index_ = src.stack_index_;
                pop_ = src.pop_;

                src.pop_ = false;
                return *this;
            }

            StackRef(const StackRef &src) = delete;
            StackRef &operator=(const StackRef &src) = delete;
#else
            StackRef(const StackRef &src)
                : state_(src.state_), stack_index_(src.stack_index_), pop_(src.pop_) {
                src.pop_ = false;
            }
            StackRef &operator=(const StackRef &src) {
                if (this != &src) {
                    state_ = src.state_;
                    stack_index_ = src.stack_index_;
                    pop_ = src.pop_;

                    src.pop_ = false;
                }
                return *this;
            }
#endif
            StackRef(lua_State *s, int index)
                : state_(s), stack_index_(lua_absindex(s, index)), pop_(true) {}
            StackRef(lua_State *s, int index, bool pop)
                : state_(s), stack_index_(lua_absindex(s, index)), pop_(pop) {}
            StackRef() : state_(0), stack_index_(0), pop_(false) {}
            ~StackRef() {
                if (state_ && pop_) {
                    if (lua_gettop(state_) >= stack_index_) {
                        lua_settop(state_, stack_index_ - 1);
                    }
                }
            }

        public:
            bool isNilref() const {
                return state_ == 0 || lua_type(state_, stack_index_) == LUA_TNIL;
            }

            int push() const {
                lua_pushvalue(state_, stack_index_);
                return 1;
            }
            int push(lua_State *state) const {
                lua_pushvalue(state_, stack_index_);
                if (state_ != state) {
                    lua_pushvalue(state_, stack_index_);
                    lua_xmove(state_, state, 1);
                }
                return 1;
            }

            int pushStackIndex(lua_State *state) const {
                if (state_ != state) {
                    lua_pushvalue(state_, stack_index_);
                    lua_xmove(state_, state, 1);
                    return lua_gettop(state);
                } else {
                    return stack_index_;
                }
            }
            lua_State *state() const { return state_; }
        };

        /// @brief Reference to Lua value. Retain reference by LUA_REGISTRYINDEX
        class RegistoryRef {
        public:
#if METAENGINE_LUAWRAPPER_USE_SHARED_LUAREF
            struct RefHolder
            {
                struct RefDeleter
                {
                    RefDeleter(lua_State *L) : state_(L) {}
                    void operator()(int *ref) {
                        luaL_unref(state_, LUA_REGISTRYINDEX, *ref);
                        delete ref;
                    }
                    lua_State *state_;
                };
                RefHolder(lua_State *L, int ref) : state_(L), ref_(new int(ref), RefDeleter(L)) {}

                RefHolder(const RefHolder &src) : state_(src.state_), ref_(src.ref_) {}
                RefHolder &operator=(const RefHolder &src) {
                    state_ = src.state_;
                    ref_ = src.ref_;
                    return *this;
                }

                RefHolder(RefHolder &&src) : state_(0), ref_() { swap(src); }
                RefHolder &operator=(RefHolder &&src) throw() {
                    swap(src);
                    return *this;
                }

                void swap(RefHolder &other) throw() {
                    std::swap(state_, other.state_);
                    std::swap(ref_, other.ref_);
                }
                int ref() const {
                    if (state_ && ref_) { return *ref_; }
                    return LUA_REFNIL;
                }
                void reset() { ref_.reset(); }
                lua_State *state() const { return state_; }

            private:
                lua_State *state_;
                standard::shared_ptr<int> ref_;
            };
#else
            struct RefHolder
            {
                RefHolder(lua_State *L, int ref) : state_(L), ref_(ref) {}
                RefHolder(const RefHolder &src) : state_(src.state_), ref_(LUA_REFNIL) {
                    if (state_) {
                        lua_rawgeti(state_, LUA_REGISTRYINDEX, src.ref_);
                        ref_ = luaL_ref(state_, LUA_REGISTRYINDEX);
                    }
                }
                RefHolder &operator=(const RefHolder &src) {
                    reset();
                    state_ = src.state_;
                    if (state_) {
                        lua_rawgeti(state_, LUA_REGISTRYINDEX, src.ref_);
                        ref_ = luaL_ref(state_, LUA_REGISTRYINDEX);
                    } else {
                        ref_ = LUA_REFNIL;
                    }
                    return *this;
                }
                RefHolder(RefHolder &&src) throw() : state_(src.state_), ref_(src.ref_) {
                    src.ref_ = LUA_REFNIL;
                }
                RefHolder &operator=(RefHolder &&src) throw() {
                    swap(src);
                    return *this;
                }
                void swap(RefHolder &other) throw() {
                    std::swap(state_, other.state_);
                    std::swap(ref_, other.ref_);
                }
                int ref() const {
                    if (state_) { return ref_; }
                    return LUA_REFNIL;
                }
                void reset() {
                    if (ref_ != LUA_REFNIL && state_) {
                        luaL_unref(state_, LUA_REGISTRYINDEX, ref_);
                        ref_ = LUA_REFNIL;
                    }
                }
                ~RefHolder() { reset(); }

                lua_State *state() const { return state_; }

            private:
                lua_State *state_;
                int ref_;
            };
#endif
            RegistoryRef(const RegistoryRef &src) : ref_(src.ref_) {}
            RegistoryRef &operator=(const RegistoryRef &src) {
                if (this != &src) { ref_ = src.ref_; }
                return *this;
            }

            static int ref_from_stacktop(lua_State *state) {
                if (state) {
                    return luaL_ref(state, LUA_REGISTRYINDEX);
                } else {
                    return LUA_REFNIL;
                }
            }
            RegistoryRef(RegistoryRef &&src) throw() : ref_(0, LUA_REFNIL) { swap(src); }
            RegistoryRef &operator=(RegistoryRef &&src) throw() {
                swap(src);
                return *this;
            }

            RegistoryRef() : ref_(0, LUA_REFNIL) {}
            RegistoryRef(lua_State *state) : ref_(state, LUA_REFNIL) {}

            RegistoryRef(lua_State *state, StackTop, NoMainCheck)
                : ref_(state, ref_from_stacktop(state)) {}

            RegistoryRef(lua_State *state, StackTop)
                : ref_(util::toMainThread(state), ref_from_stacktop(state)) {}

            void swap(RegistoryRef &other) throw() { ref_.swap(other.ref_); }

            template<typename T>
            RegistoryRef(lua_State *state, const T &v, NoMainCheck) : ref_(0, LUA_REFNIL) {
                if (!state) { return; }
                util::ScopedSavedStack save(state);
                util::one_push(state, v);
                ref_ = RefHolder(state, ref_from_stacktop(state));
            }
            template<typename T>
            RegistoryRef(lua_State *state, const T &v) : ref_(0, LUA_REFNIL) {
                if (!state) { return; }
                util::ScopedSavedStack save(state);
                util::one_push(state, v);
                ref_ = RefHolder(util::toMainThread(state), ref_from_stacktop(state));
            }
#if METAENGINE_LUAWRAPPER_USE_CPP11
            template<typename T>
            RegistoryRef(lua_State *state, T &&v, NoMainCheck) : ref_(0, LUA_REFNIL) {
                if (!state) { return; }
                util::ScopedSavedStack save(state);
                util::one_push(state, standard::forward<T>(v));
                ref_ = RefHolder(state, ref_from_stacktop(state));
            }
            template<typename T>
            RegistoryRef(lua_State *state, T &&v) : ref_(0, LUA_REFNIL) {
                if (!state) { return; }
                util::ScopedSavedStack save(state);
                util::one_push(state, standard::forward<T>(v));
                ref_ = RefHolder(util::toMainThread(state), ref_from_stacktop(state));
            }
#endif
            ~RegistoryRef() {
                try {
                    unref();
                } catch (...) {}// can't throw at Destructor
            }

            /// @brief push to Lua stack
            int push() const { return push(ref_.state()); }
            /// @brief push to Lua stack
            int push(lua_State *state) const {
                if (isNilref()) {
                    lua_pushnil(state);
                    return 1;
                }
#if LUA_VERSION_NUM >= 502
                if (state != ref_.state()) {// state check
                    assert(util::toMainThread(state) == util::toMainThread(ref_.state()));
                }
#endif
                lua_rawgeti(state, LUA_REGISTRYINDEX, ref_.ref());
                return 1;
            }

            int pushStackIndex(lua_State *state) const {
                push(state);
                return lua_gettop(state);
            }
            lua_State *state() const { return ref_.state(); }

            bool isNilref() const { return ref_.ref() == LUA_REFNIL; }

            void unref() { ref_.reset(); }

        private:
            RefHolder ref_;
        };
    }// namespace Ref
}// namespace LuaWrapper

namespace LuaWrapper {
    class LuaTable;
    class LuaFunction;

    class FunctionResults;

    /**
* status of coroutine
*/
    enum coroutine_status {
        COSTAT_RUNNING,  //!< coroutine is running
        COSTAT_SUSPENDED,//!< coroutine is suspended
        COSTAT_NORMAL,   //!<
        COSTAT_DEAD      //!< coroutine is dead
    };

    namespace detail {
        class FunctionResultProxy {
        public:
            template<typename RetType>
            static RetType ReturnValue(lua_State *state, int restatus, int retindex,
                                       types::typetag<RetType> tag);
            static FunctionResults ReturnValue(lua_State *state, int restatus, int retindex,
                                               types::typetag<FunctionResults> tag);
            static void ReturnValue(lua_State *state, int restatus, int retindex,
                                    types::typetag<void> tag);
        };

        template<typename Derived>
        class LuaFunctionImpl {
        private:
            lua_State *state_() const { return static_cast<const Derived *>(this)->state(); }
            int pushStackIndex_(lua_State *state) const {
                return static_cast<const Derived *>(this)->pushStackIndex(state);
            }
            int push_(lua_State *state) const {
                return static_cast<const Derived *>(this)->push(state);
            }

        public:
            /**
  * set function environment table
  */
            bool setFunctionEnv(const LuaTable &env);
            /**
  * set function environment to new table
  */
            bool setFunctionEnv(NewTable env);
            /**
  * get function environment table
  */
            LuaTable getFunctionEnv() const;

#if METAENGINE_LUAWRAPPER_USE_CPP11
            template<class Result, class... Args>
            Result call(Args &&...args) {
                lua_State *state = state_();
                if (!state) {
                    except::typeMismatchError(state, "nil");
                    return Result();
                }
                int argstart = lua_gettop(state) + 1;
                push_(state);
                int argnum = util::push_args(state, std::forward<Args>(args)...);
                int result = lua_pcall_wrap(state, argnum, LUA_MULTRET);
                except::checkErrorAndThrow(result, state);
                return detail::FunctionResultProxy::ReturnValue(state, result, argstart,
                                                                types::typetag<Result>());
            }

            template<class... Args>
            FunctionResults operator()(Args &&...args);
#else

#define METAENGINE_LUAWRAPPER_CALL_DEF(N)                                                          \
    template<class Result METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>                  \
    Result call(METAENGINE_LUAWRAPPER_PP_ARG_CR_DEF_REPEAT(N)) {                                   \
        lua_State *state = state_();                                                               \
        if (!state) {                                                                              \
            except::typeMismatchError(state, "attempt to call nil value");                         \
            return Result();                                                                       \
        }                                                                                          \
        int argstart = lua_gettop(state) + 1;                                                      \
        push_(state);                                                                              \
        int argnum = util::push_args(state METAENGINE_LUAWRAPPER_PP_ARG_REPEAT_CONCAT(N));         \
        int result = lua_pcall_wrap(state, argnum, LUA_MULTRET);                                   \
        except::checkErrorAndThrow(result, state);                                                 \
        return detail::FunctionResultProxy::ReturnValue(state, result, argstart,                   \
                                                        types::typetag<Result>());                 \
    }

            METAENGINE_LUAWRAPPER_CALL_DEF(0)
            METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                                METAENGINE_LUAWRAPPER_CALL_DEF)

#undef METAENGINE_LUAWRAPPER_RESUME_DEF

            inline FunctionResults operator()();

#define METAENGINE_LUAWRAPPER_OP_FN_DEF(N)                                                         \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    inline FunctionResults operator()(METAENGINE_LUAWRAPPER_PP_ARG_CR_DEF_REPEAT(N));

#define METAENGINE_LUAWRAPPER_FUNCTION_ARGS_DEF(N)
#define METAENGINE_LUAWRAPPER_CALL_ARGS(N) METAENGINE_LUAWRAPPER_PP_ARG_REPEAT(N)

            METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                                METAENGINE_LUAWRAPPER_OP_FN_DEF)
#undef METAENGINE_LUAWRAPPER_OP_FN_DEF

#undef METAENGINE_LUAWRAPPER_CALL_ARGS
#undef METAENGINE_LUAWRAPPER_FUNCTION_ARGS_DEF
#undef METAENGINE_LUAWRAPPER_CALL_DEF
#endif
        };

        template<typename Derived>
        class LuaThreadImpl {
        private:
            lua_State *state_() const { return static_cast<const Derived *>(this)->state(); }
            int pushStackIndex_(lua_State *state) const {
                return static_cast<const Derived *>(this)->pushStackIndex(state);
            }

        public:
#if METAENGINE_LUAWRAPPER_USE_CPP11
            template<class Result, class... Args>
            Result resume(Args &&...args) {
                lua_State *state = state_();
                if (!state) {
                    except::typeMismatchError(state, "attempt to call nil value");
                    return Result();
                }
                util::ScopedSavedStack save(state);
                int corStackIndex = pushStackIndex_(state);
                lua_State *thread = lua_tothread(state, corStackIndex);
                if (!thread) {
                    except::typeMismatchError(state, "not thread");
                    return Result();
                }
                int argstart = 1;// exist function in stack at first resume.
                if (lua_status(thread) == LUA_YIELD) { argstart = 0; }
                util::push_args(thread, std::forward<Args>(args)...);
                int argnum = lua_gettop(thread) - argstart;
                if (argnum < 0) { argnum = 0; }
                int nres = 0;
                int result = lua_resume(thread, state, argnum, &nres);
                except::checkErrorAndThrow(result, thread);
                return detail::FunctionResultProxy::ReturnValue(thread, result, 1,
                                                                types::typetag<Result>());
            }
            template<class... Args>
            FunctionResults operator()(Args &&...args);
#else

#define METAENGINE_LUAWRAPPER_RESUME_DEF(N)                                                        \
    template<class Result METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>                  \
    Result resume(METAENGINE_LUAWRAPPER_PP_ARG_CR_DEF_REPEAT(N)) {                                 \
        lua_State *state = state_();                                                               \
        if (!state) {                                                                              \
            except::typeMismatchError(state, "attempt to call nil value");                         \
            return Result();                                                                       \
        }                                                                                          \
        util::ScopedSavedStack save(state);                                                        \
        int corStackIndex = pushStackIndex_(state);                                                \
        lua_State *thread = lua_tothread(state, corStackIndex);                                    \
        if (!thread) {                                                                             \
            except::typeMismatchError(state, "not thread");                                        \
            return Result();                                                                       \
        }                                                                                          \
        int argstart = 1;                                                                          \
        if (lua_status(thread) == LUA_YIELD) { argstart = 0; }                                     \
        util::push_args(thread METAENGINE_LUAWRAPPER_PP_ARG_REPEAT_CONCAT(N));                     \
        int argnum = lua_gettop(thread) - argstart;                                                \
        if (argnum < 0) { argnum = 0; }                                                            \
        int result = lua_resume(thread, state, argnum);                                            \
        except::checkErrorAndThrow(result, thread);                                                \
        return detail::FunctionResultProxy::ReturnValue(thread, result, 1,                         \
                                                        types::typetag<Result>());                 \
    }

            METAENGINE_LUAWRAPPER_RESUME_DEF(0)
            METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                                METAENGINE_LUAWRAPPER_RESUME_DEF)

#undef METAENGINE_LUAWRAPPER_RESUME_DEF

            inline FunctionResults operator()();

#define METAENGINE_LUAWRAPPER_OP_FN_DEF(N)                                                         \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    inline FunctionResults operator()(METAENGINE_LUAWRAPPER_PP_ARG_CR_DEF_REPEAT(N));

            METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                                METAENGINE_LUAWRAPPER_OP_FN_DEF)

#undef METAENGINE_LUAWRAPPER_OP_FN_DEF
#endif

            //!
            //! @return state status
            //!
            int threadStatus() const {
                lua_State *state = state_();
                if (!state) {
                    except::typeMismatchError(state, "attempt to call nil value");
                    return LUA_ERRRUN;
                }
                util::ScopedSavedStack save(state);
                int corStackIndex = pushStackIndex_(state);
                lua_State *thread = lua_tothread(state, corStackIndex);

                if (!thread) {
                    except::typeMismatchError(state, "not thread");
                    return LUA_ERRRUN;
                }
                return lua_status(thread);
            }

            //! deprecate
            int thread_status() const { return threadStatus(); }

            //!
            //! @return coroutine status
            //!
            coroutine_status costatus(lua_State *l = 0) const {
                lua_State *state = state_();
                if (!state) { return COSTAT_DEAD; }
                util::ScopedSavedStack save(state);
                int corStackIndex = pushStackIndex_(state);
                lua_State *thread = lua_tothread(state, corStackIndex);

                if (!thread) {
                    except::typeMismatchError(state, "not thread");
                    return COSTAT_DEAD;
                } else if (thread == l) {
                    return COSTAT_RUNNING;
                } else {
                    switch (lua_status(thread)) {
                        case LUA_YIELD:
                            return COSTAT_SUSPENDED;
                        case 0:// LUA_OK
                        {
                            if (lua_gettop(thread) == 0) {
                                return COSTAT_DEAD;
                            } else {
                                return COSTAT_SUSPENDED;
                            }
                        }
                        default:
                            break;
                    }
                }
                return COSTAT_DEAD;
            }

            //!
            //! @return if coroutine status is dead, return true. Otherwise return false
            //!
            bool isThreadDead() const { return costatus() == COSTAT_DEAD; }

            /// @brief set function for thread running.
            void setFunction(const LuaFunction &f);

            /// @brief get lua thread
            lua_State *getthread() {
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                int corStackIndex = pushStackIndex_(state);
                return lua_tothread(state, corStackIndex);
            }
        };
    }// namespace detail
}// namespace LuaWrapper

namespace LuaWrapper {
    class LuaRef;
    class LuaStackRef;
    class LuaTable;
    template<typename KEY>
    class TableKeyReferenceProxy;
    class MemberFunctionBinder;

    namespace detail {

        struct table_proxy
        {

#if METAENGINE_LUAWRAPPER_USE_CPP11
            template<typename V, typename KEY>
            static void set(lua_State *state, int table_index, KEY &&key, V &&value) {
                util::one_push(state, std::forward<KEY>(key));
                util::one_push(state, std::forward<V>(value));
                lua_settable(state, table_index);
            }
            template<typename V>
            static void set(lua_State *state, int table_index, const char *key, V &&value) {
                util::one_push(state, std::forward<V>(value));
                lua_setfield(state, table_index, key);
            }
            template<typename V>
            static void set(lua_State *state, int table_index, const std::string &key, V &&value) {
                set(state, table_index, key.c_str(), std::forward<V>(value));
            }

            template<typename V>
            static void set(lua_State *state, int table_index, luaInt key, V &&value) {
                util::one_push(state, std::forward<V>(value));
                lua_seti(state, table_index, key);
            }
            template<typename V, typename KEY>
            static void rawset(lua_State *state, int table_index, KEY &&key, V &&value) {
                util::one_push(state, std::forward<KEY>(key));
                util::one_push(state, std::forward<V>(value));
                lua_rawset(state, table_index);
            }
            template<typename V>
            static void rawset(lua_State *state, int table_index, luaInt key, V &&value) {
                util::one_push(state, std::forward<V>(value));
                lua_rawseti(state, table_index, key);
            }
#else
            template<typename V, typename KEY>
            static void set(lua_State *state, int table_index, const KEY &key, const V &value) {
                util::one_push(state, key);
                util::one_push(state, value);
                lua_settable(state, table_index);
            }
            template<typename V>
            static void set(lua_State *state, int table_index, const char *key, const V &value) {
                util::one_push(state, value);
                lua_setfield(state, table_index, key);
            }
            template<typename V>
            static void set(lua_State *state, int table_index, const std::string &key,
                            const V &value) {
                util::one_push(state, value);
                lua_setfield(state, table_index, key.c_str());
            }

            template<typename V>
            static void set(lua_State *state, int table_index, luaInt key, const V &value) {
                util::one_push(state, value);
                lua_seti(state, table_index, key);
            }

            template<typename V, typename KEY>
            static void rawset(lua_State *state, int table_index, const KEY &key, const V &value) {
                util::one_push(state, key);
                util::one_push(state, value);
                lua_rawset(state, table_index);
            }
            template<typename V>
            static void rawset(lua_State *state, int table_index, luaInt key, const V &value) {
                util::one_push(state, value);
                lua_rawseti(state, table_index, key);
            }
#endif

#if METAENGINE_LUAWRAPPER_USE_CPP11
            template<typename KEY>
            static void get(lua_State *state, int table_index, KEY &&key) {
                util::one_push(state, std::forward<KEY>(key));
                lua_gettable(state, table_index);
            }
#endif
            template<typename KEY>
            static void get(lua_State *state, int table_index, const KEY &key) {
                util::one_push(state, key);
                lua_gettable(state, table_index);
            }
            static void get(lua_State *state, int table_index, const char *key) {
                lua_getfield(state, table_index, key);
            }
            static void get(lua_State *state, int table_index, const std::string &key) {
                lua_getfield(state, table_index, key.c_str());
            }
            static void get(lua_State *state, int table_index, luaInt key) {
                lua_geti(state, table_index, key);
            }
#if METAENGINE_LUAWRAPPER_USE_CPP11
            template<typename KEY>
            static void rawget(lua_State *state, int table_index, KEY &&key) {
                util::one_push(state, std::forward<KEY>(key));
                lua_rawget(state, table_index);
            }
#endif
            template<typename KEY>
            static void rawget(lua_State *state, int table_index, const KEY &key) {
                util::one_push(state, key);
                lua_rawget(state, table_index);
            }
            static void rawget(lua_State *state, int table_index, luaInt key) {
                lua_rawgeti(state, table_index, key);
            }
        };

        template<typename Derived>
        class LuaTableOrUserDataImpl {
        private:
            lua_State *state_() const { return static_cast<const Derived *>(this)->state(); }
            int pushStackIndex_(lua_State *state) const {
                return static_cast<const Derived *>(this)->pushStackIndex(state);
            }
            int push_(lua_State *state) const {
                return static_cast<const Derived *>(this)->push(state);
            }

        public:
            /// @brief set metatable
            /// @param table metatable
            bool setMetatable(const LuaTable &table);

            /// @brief get metatable
            LuaTable getMetatable() const;

            /// @brief table->*"function_name"() in c++ and table:function_name(); in lua
            /// is same
            /// @param function_name function_name in table
            MemberFunctionBinder operator->*(const char *function_name);

            /// @brief value = table[key];
            /// @param key key of table
            /// @return reference of field value
            template<typename T, typename KEY>
            typename lua_type_traits<T>::get_type getField(const KEY &key) const {
                lua_State *state = state_();
                typedef typename lua_type_traits<T>::get_type get_type;
                if (!state) {
                    except::typeMismatchError(state, "is nil");
                    return get_type();
                }
                util::ScopedSavedStack save(state);
                int stackIndex = pushStackIndex_(state);

                table_proxy::get(state, stackIndex, key);

                return lua_type_traits<T>::get(state, -1);
            }

            /// @brief value = table[key];
            /// @param key key of table
            /// @return reference of field value
            template<typename KEY>
            LuaStackRef getField(const KEY &key) const;

#if METAENGINE_LUAWRAPPER_USE_CPP11
            /// @brief table[key] = value;
            template<typename K, typename V>
            bool setField(K &&key, V &&value) {
                lua_State *state = state_();
                if (!state) {
                    except::typeMismatchError(state, "is nil");
                    return false;
                }
                util::ScopedSavedStack save(state);
                int stackIndex = pushStackIndex_(state);

                table_proxy::set(state, stackIndex, std::forward<K>(key), std::forward<V>(value));
                return true;
            }
#else
            /// @brief table[key] = value;
            template<typename K, typename V>
            bool setField(const K &key, const V &value) {
                lua_State *state = state_();
                if (!state) {
                    except::typeMismatchError(state, "is nil");
                    return false;
                }
                util::ScopedSavedStack save(state);
                int stackIndex = pushStackIndex_(state);

                table_proxy::set(state, stackIndex, key, value);

                return true;
            }
#endif

            /// @brief value = table[key];
            /// @param key key of table
            /// @return reference of field value
            template<typename K>
            LuaStackRef operator[](K key) const;

            /// @brief value = table[key];or table[key] = value;
            /// @param key key of table
            /// @return reference of field value
            template<typename K>
            TableKeyReferenceProxy<K> operator[](K key);
        };

        template<typename Derived>
        class LuaTableImpl {
        private:
            lua_State *state_() const { return static_cast<const Derived *>(this)->state(); }
            int pushStackIndex_(lua_State *state) const {
                return static_cast<const Derived *>(this)->pushStackIndex(state);
            }
            int push_(lua_State *state) const {
                return static_cast<const Derived *>(this)->push(state);
            }

            template<typename K, typename A>
            struct gettablekey
            {
                typedef K key_type;
                typedef void value_type;
                std::vector<K, A> &v_;
                gettablekey(std::vector<K, A> &v) : v_(v) {}
                void operator()(K key, const void *) { v_.push_back(key); }
            };
            template<typename V, typename A>
            struct gettablevalue
            {
                typedef void key_type;
                typedef V value_type;
                std::vector<V, A> &v_;
                gettablevalue(std::vector<V, A> &v) : v_(v) {}
                void operator()(const void *, V value) { v_.push_back(value); }
            };
            template<typename K, typename V, typename C, typename A>
            struct gettablemap
            {
                typedef K key_type;
                typedef V value_type;
                std::map<K, V, C, A> &m_;
                gettablemap(std::map<K, V, C, A> &m) : m_(m) {}
                void operator()(K key, V value) { m_[key] = value; }
            };

        public:
#if METAENGINE_LUAWRAPPER_USE_CPP11
            /// @brief rawset(table,key,value)
            template<typename K, typename V>
            bool setRawField(K &&key, V &&value) {
                lua_State *state = state_();
                if (!state) {
                    except::typeMismatchError(state, "is nil");
                    return false;
                }
                util::ScopedSavedStack save(state);
                int stackIndex = pushStackIndex_(state);

                table_proxy::rawset(state, stackIndex, std::forward<K>(key),
                                    std::forward<V>(value));

                return true;
            }
#else
            /// @brief rawset(table,key,value)
            template<typename K, typename V>
            bool setRawField(const K &key, const V &value) {
                lua_State *state = state_();
                if (!state) {
                    except::typeMismatchError(state, "is nil");
                    return false;
                }
                util::ScopedSavedStack save(state);
                int stackIndex = pushStackIndex_(state);
                table_proxy::rawset(state, stackIndex, key, value);
                return true;
            }
#endif
            /// @brief value = rawget(table,key);
            /// @param key key of table
            /// @return reference of field value
            template<typename T, typename KEY>
            typename lua_type_traits<T>::get_type getRawField(const KEY &key) const {
                lua_State *state = state_();
                typedef typename lua_type_traits<T>::get_type get_type;
                if (!state) {
                    except::typeMismatchError(state, "is nil");
                    return get_type();
                }
                util::ScopedSavedStack save(state);
                int stackIndex = pushStackIndex_(state);

                table_proxy::rawget(state, stackIndex, key);

                return lua_type_traits<T>::get(state, -1);
            }
            /// @brief value = rawget(table,key);
            /// @param key key of table
            /// @return reference of field value
            template<typename KEY>
            LuaStackRef getRawField(const KEY &key) const;

            /// @brief foreach table fields
            template<class K, class V, class Fun>
            void foreach_table(Fun f) const {
                lua_State *state = state_();
                if (!state) {
                    except::typeMismatchError(state, "is nil");
                    return;
                }
                util::ScopedSavedStack save(state);
                int stackIndex = pushStackIndex_(state);
                lua_pushnil(state);
                while (lua_next(state, stackIndex) != 0) {
                    // backup key
                    lua_pushvalue(state, -2);

                    f(lua_type_traits<K>::get(state, -1), lua_type_traits<V>::get(state, -2));
                    lua_pop(state, 2);// pop key and value
                }
            }

            /// @brief foreach table fields
            template<class K, class V, class Fun>
            void foreach_table_breakable(Fun f) const {
                lua_State *state = state_();
                if (!state) {
                    except::typeMismatchError(state, "is nil");
                    return;
                }
                util::ScopedSavedStack save(state);
                int stackIndex = pushStackIndex_(state);
                lua_pushnil(state);
                while (lua_next(state, stackIndex) != 0) {
                    lua_pushvalue(state, -2);// backup key

                    bool cont = f(lua_type_traits<K>::get(state, -1),
                                  lua_type_traits<V>::get(state, -2));
                    lua_pop(state, 2);// pop key and value
                    if (!cont) { break; }
                }
            }

            /// @brief If type is table or userdata, return keys.
            /// @return field keys
            template<typename K, typename A>
            std::vector<K, A> keys() const {
                std::vector<K, A> res;
                util::ScopedSavedStack save(state_());
                int stackIndex = pushStackIndex_(state_());
                size_t size = lua_rawlen(state_(), stackIndex);
                res.reserve(size);
                foreach_table<K, void>(gettablekey<K, A>(res));
                return res;
            }

            /// @brief If type is table or userdata, return keys.
            /// @return field keys
            template<typename K>
            std::vector<K> keys() const {
                return keys<K, std::allocator<K>>();
            }
            std::vector<LuaRef> keys() const;

            /// @brief If type is table or userdata, return values.
            /// @return field value
            template<typename V, typename A>
            std::vector<V, A> values() const {
                std::vector<V, A> res;
                util::ScopedSavedStack save(state_());
                int stackIndex = pushStackIndex_(state_());
                size_t size = lua_rawlen(state_(), stackIndex);
                res.reserve(size);
                foreach_table<void, V>(gettablevalue<V, A>(res));
                return res;
            }

            /// @brief If type is table or userdata, return values.
            /// @return field value
            template<typename V>
            std::vector<V> values() const {
                return values<V, std::allocator<V>>();
            }
            std::vector<LuaRef> values() const;

            /// @brief If type is table or userdata, return key value pair.
            /// @return key value pair
            template<typename K, typename V, typename C, typename A>
            std::map<K, V, C, A> map() const {
                std::map<K, V, C, A> res;
                foreach_table<K, V>(gettablemap<K, V, C, A>(res));
                return res;
            }

            /// @brief If type is table or userdata, return key value pair.
            /// @return key value pair
            template<typename K, typename V, typename C>
            std::map<K, V, C> map() const {
                return map<K, V, C, std::allocator<std::pair<const K, V>>>();
            }

            /// @brief If type is table or userdata, return key value pair.
            /// @return key value pair
            template<typename K, typename V>
            std::map<K, V> map() const {
                return map<K, V, std::less<K>>();
            }
            std::map<LuaRef, LuaRef> map() const;
        };

        template<typename Derived>
        class LuaUserDataImpl {
        private:
            lua_State *state_() const { return static_cast<const Derived *>(this)->state(); }
            int pushStackIndex_(lua_State *state) const {
                return static_cast<const Derived *>(this)->pushStackIndex(state);
            }
            int push_(lua_State *state) const {
                return static_cast<const Derived *>(this)->push(state);
            }

        public:
            /// @brief is type test
            template<typename T>
            bool isType() const {
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                return lua_type_traits<T>::strictCheckType(state, pushStackIndex_(state));
            }

            template<typename T>
            bool isConvertible() const {
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                return lua_type_traits<T>::checkType(state, pushStackIndex_(state));
            }

            template<typename T>
            bool typeTest() const {
                return isType<T>();
            }
            template<typename T>
            bool weakTypeTest() const {
                return isConvertible<T>();
            }

            template<typename T>
            typename lua_type_traits<T>::get_type get() const {
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                return lua_type_traits<T>::get(state, state ? pushStackIndex_(state) : 0);
            }

            template<typename T>
            operator T() const {
                return get<T>();
            }
        };
    }// namespace detail
}// namespace LuaWrapper

namespace LuaWrapper {
    class LuaRef;
    class LuaTable;
    template<typename KEY>
    class TableKeyReferenceProxy;
    class MemberFunctionBinder;

    namespace detail {

        template<typename Derived>
        class LuaVariantImpl : public LuaTableImpl<Derived>,
                               public LuaTableOrUserDataImpl<Derived>,
                               public detail::LuaFunctionImpl<Derived>,
                               public detail::LuaThreadImpl<Derived>,
                               public LuaBasicTypeFunctions<Derived> {
        private:
            lua_State *state_() const { return static_cast<const Derived *>(this)->state(); }
            int pushStackIndex_(lua_State *state) const {
                return static_cast<const Derived *>(this)->pushStackIndex(state);
            }

        public:
            using LuaBasicTypeFunctions<Derived>::type;
            using LuaBasicTypeFunctions<Derived>::typeName;

            /// @brief deprecated, use isType instead.
            template<typename T>
            bool typeTest() const {
                return isType<T>();
            }

            /// @brief deprecated, use isConvertible instead.
            template<typename T>
            bool weakTypeTest() const {
                return isConvertible<T>();
            }

            /// @brief is type test
            template<typename T>
            bool isType() const {
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                return lua_type_traits<T>::strictCheckType(state, pushStackIndex_(state));
            }

            template<typename T>
            bool isConvertible() const {
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                return lua_type_traits<T>::checkType(state, pushStackIndex_(state));
            }

            template<typename T>
            typename lua_type_traits<T>::get_type get() const {
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                return lua_type_traits<T>::get(state, state ? pushStackIndex_(state) : 0);
            }
            template<typename T, typename U>
            typename lua_type_traits<T>::get_type value_or(U v) const {
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                return lua_type_traits<optional<T>>::get(state, state ? pushStackIndex_(state) : 0)
                        .value_or(v);
            }

            // deprecated. use get<LuaWrapper::optional<T> >() instead;
            template<typename T>
            typename lua_type_traits<T>::get_type get(bool &was_valid,
                                                      bool allow_convertible = true) const {
                lua_State *state = state_();
                util::ScopedSavedStack save(state);
                int stackindex = pushStackIndex_(state);
                if (allow_convertible) {
                    was_valid = lua_type_traits<T>::checkType(state, stackindex);
                } else {
                    was_valid = lua_type_traits<T>::strictCheckType(state, stackindex);
                }
                if (was_valid) {
                    return lua_type_traits<T>::get(state, stackindex);
                } else {
                    return T();
                }
            }
            template<typename T>
            operator T() const {
                return get<T>();
            }

#if METAENGINE_LUAWRAPPER_USE_CPP11
            template<class... Args>
            FunctionResults operator()(Args &&...args);
#else
            inline FunctionResults operator()();

#define METAENGINE_LUAWRAPPER_OP_FN_DEF(N)                                                         \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    inline FunctionResults operator()(METAENGINE_LUAWRAPPER_PP_ARG_CR_DEF_REPEAT(N));

            METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                                METAENGINE_LUAWRAPPER_OP_FN_DEF)
#undef METAENGINE_LUAWRAPPER_OP_FN_DEF
#endif
        };
    }// namespace detail
}// namespace LuaWrapper

namespace LuaWrapper {
    namespace util {
        template<class Result>
        inline Result get_result_impl(lua_State *l, int startindex, types::typetag<Result>) {
            return lua_type_traits<Result>::get(l, startindex);
        }
#if METAENGINE_LUAWRAPPER_USE_CPP11
        inline standard::tuple<> get_result_tuple_impl(lua_State *, int,
                                                       types::typetag<standard::tuple<>>) {
            return standard::tuple<>();
        }
        template<typename T, typename... TYPES>
        inline standard::tuple<T, TYPES...> get_result_tuple_impl(
                lua_State *l, int index, types::typetag<standard::tuple<T, TYPES...>>) {
            return standard::tuple_cat(
                    standard::tuple<T>(lua_type_traits<T>::get(l, index)),
                    get_result_tuple_impl(l, index + 1,
                                          types::typetag<standard::tuple<TYPES...>>()));
        }
        template<typename... TYPES>
        inline standard::tuple<TYPES...> get_result_impl(
                lua_State *l, int startindex, types::typetag<standard::tuple<TYPES...>> tag) {
            return get_result_tuple_impl<TYPES...>(l, startindex, tag);
        }
#else

        inline standard::tuple<> get_result_impl(lua_State *l, int startindex,
                                                 types::typetag<standard::tuple<>>) {
            METAENGINE_LUAWRAPPER_UNUSED(l);
            METAENGINE_LUAWRAPPER_UNUSED(startindex);
            return standard::tuple<>();
        }

#define METAENGINE_LUAWRAPPER_GET_DEF(N)                                                           \
    lua_type_traits<METAENGINE_LUAWRAPPER_PP_CAT(A, N)>::get(l, N + startindex - 1)
#define METAENGINE_LUAWRAPPER_GET_TUPLE_DEF(N)                                                     \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    inline standard::tuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)> get_result_impl(       \
            lua_State *l, int startindex,                                                          \
            types::typetag<standard::tuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>>) {    \
        return standard::tuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>(                   \
                METAENGINE_LUAWRAPPER_PP_REPEAT_ARG(N, METAENGINE_LUAWRAPPER_GET_DEF));            \
    }
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_TUPLE_SIZE,
                                            METAENGINE_LUAWRAPPER_GET_TUPLE_DEF)
#undef METAENGINE_LUAWRAPPER_GET_DEF
#undef METAENGINE_LUAWRAPPER_GET_TUPLE_DEF
#endif

        template<class Result>
        inline Result get_result(lua_State *l, int startindex) {
            return get_result_impl(l, startindex, types::typetag<Result>());
        }
        template<>
        inline void get_result<void>(lua_State *, int) {}
    }// namespace util

    /// @addtogroup Lua_reference_types

    /// @ingroup Lua_reference_types
    /// @brief Reference to any Lua data.
    class LuaRef : public Ref::RegistoryRef, public detail::LuaVariantImpl<LuaRef> {
    private:
    public:
        LuaRef(const Ref::RegistoryRef &src) : Ref::RegistoryRef(src) {}
        LuaRef(const LuaRef &src) : Ref::RegistoryRef(src) {}
        LuaRef &operator=(const LuaRef &src) {
            static_cast<RegistoryRef &>(*this) = src;
            return *this;
        }

#if METAENGINE_LUAWRAPPER_USE_CPP11

        LuaRef(LuaRef &&src) : Ref::RegistoryRef(std::move(src)) {}

        LuaRef &operator=(LuaRef &&src) throw() {
            swap(src);
            return *this;
        }

        LuaRef(RegistoryRef &&src) throw() : Ref::RegistoryRef(std::move(src)) {}
        template<typename T>
        LuaRef(lua_State *state, T &&v, Ref::NoMainCheck)
            : Ref::RegistoryRef(state, std::move(v), Ref::NoMainCheck()) {}
        template<typename T>
        LuaRef(lua_State *state, T &&v) : Ref::RegistoryRef(state, std::move(v)) {}
#endif

        LuaRef() {}
        LuaRef(lua_State *state) : Ref::RegistoryRef(state) {}

        LuaRef(lua_State *state, StackTop, Ref::NoMainCheck)
            : Ref::RegistoryRef(state, StackTop(), Ref::NoMainCheck()) {}

        LuaRef(lua_State *state, StackTop) : Ref::RegistoryRef(state, StackTop()) {}

        template<typename T>
        LuaRef(lua_State *state, const T &v, Ref::NoMainCheck)
            : Ref::RegistoryRef(state, v, Ref::NoMainCheck()) {}
        template<typename T>
        LuaRef(lua_State *state, const T &v) : Ref::RegistoryRef(state, v) {}

        const void *native_pointer() const {
            util::ScopedSavedStack save(state());
            push(state());
            return lua_topointer(state(), -1);
        }
        static void putindent(std::ostream &os, int indent) {
            while (indent-- > 0) { os << "  "; }
        }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaRef
    template<>
    struct lua_type_traits<LuaRef>
    {
        typedef LuaRef get_type;
        typedef const LuaRef &push_type;

        static bool checkType(lua_State *l, int index) {
            METAENGINE_LUAWRAPPER_UNUSED(l);
            METAENGINE_LUAWRAPPER_UNUSED(index);
            return true;
        }
        static bool strictCheckType(lua_State *l, int index) {
            METAENGINE_LUAWRAPPER_UNUSED(l);
            METAENGINE_LUAWRAPPER_UNUSED(index);
            return false;
        }

        static get_type get(lua_State *l, int index) {
            lua_pushvalue(l, index);
            return LuaRef(l, StackTop());
        }
        static int push(lua_State *l, push_type v) { return v.push(l); }
    };
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaRef
    template<>
    struct lua_type_traits<const LuaRef &> : lua_type_traits<LuaRef>
    {
    };

    class LuaStackRef : public Ref::StackRef, public detail::LuaVariantImpl<LuaStackRef> {
    public:
        LuaStackRef() : Ref::StackRef() {}
        LuaStackRef(lua_State *s, int index) : Ref::StackRef(s, index, false) {}
        LuaStackRef(lua_State *s, int index, bool popAtDestruct)
            : Ref::StackRef(s, index, popAtDestruct) {}
#if METAENGINE_LUAWRAPPER_USE_CPP11
        LuaStackRef(LuaStackRef &&src) : Ref::StackRef(std::move(src)) { src.pop_ = false; }
        LuaStackRef &operator=(LuaStackRef &&src) {
            if (this != &src) {
                Ref::StackRef::operator=(std::move(src));
                src.pop_ = false;
            }
            return *this;
        }
        LuaStackRef(const LuaStackRef &src) = delete;
#else
        LuaStackRef(const LuaStackRef &src) : Ref::StackRef(src) { src.pop_ = false; }
        LuaStackRef &operator=(const LuaStackRef &src) {
            if (this != &src) {
                Ref::StackRef::operator=(src);
                src.pop_ = false;
            }
            return *this;
        }
#endif
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaStackRef
    template<>
    struct lua_type_traits<LuaStackRef>
    {
        typedef LuaStackRef get_type;
        typedef const LuaStackRef &push_type;

        static bool checkType(lua_State *l, int index) {
            METAENGINE_LUAWRAPPER_UNUSED(l);
            METAENGINE_LUAWRAPPER_UNUSED(index);
            return true;
        }
        static bool strictCheckType(lua_State *l, int index) {
            METAENGINE_LUAWRAPPER_UNUSED(l);
            METAENGINE_LUAWRAPPER_UNUSED(index);
            return false;
        }

        static get_type get(lua_State *l, int index) { return LuaStackRef(l, index); }
        static int push(lua_State *l, push_type v) { return v.push(l); }
    };
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaStackRef
    template<>
    struct lua_type_traits<const LuaStackRef &> : lua_type_traits<LuaStackRef>
    {
    };

    /// @ingroup Lua_reference_types
    /// @brief Reference to Lua userdata.
    class LuaUserData : public Ref::RegistoryRef,
                        public detail::LuaUserDataImpl<LuaUserData>,
                        public detail::LuaTableOrUserDataImpl<LuaUserData>,
                        public detail::LuaBasicTypeFunctions<LuaUserData> {

        void typecheck() {
            int t = type();
            if (t != TYPE_USERDATA && t != TYPE_LIGHTUSERDATA && t != TYPE_NIL && t != TYPE_NONE) {
                except::typeMismatchError(state(), "not user data");
                unref();
            }
        }

    public:
        operator LuaRef() {
            push(state());
            return LuaRef(state(), StackTop());
        }
        LuaUserData(lua_State *state, StackTop) : Ref::RegistoryRef(state, StackTop()) {
            typecheck();
        }
        template<typename TYPE>
        LuaUserData(lua_State *state, const TYPE &table) : Ref::RegistoryRef(state, table) {
            typecheck();
        }
        explicit LuaUserData(lua_State *state) : Ref::RegistoryRef(state, NilValue()) {
            typecheck();
        }
        LuaUserData() { typecheck(); }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaUserData
    template<>
    struct lua_type_traits<LuaUserData>
    {
        typedef LuaUserData get_type;
        typedef LuaUserData push_type;

        static bool strictCheckType(lua_State *l, int index) {
            return lua_type(l, index) == LUA_TUSERDATA;
        }
        static bool checkType(lua_State *l, int index) {
            return lua_type(l, index) == LUA_TUSERDATA || lua_isnil(l, index);
        }
        static LuaUserData get(lua_State *l, int index) {
            lua_pushvalue(l, index);
            return LuaUserData(l, StackTop());
        }
        static int push(lua_State *l, const LuaUserData &ref) { return ref.push(l); }
    };
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaUserData
    template<>
    struct lua_type_traits<const LuaUserData &> : lua_type_traits<LuaUserData>
    {
    };

    /// @ingroup Lua_reference_types
    /// @brief Reference to Lua table.
    class LuaTable : public Ref::RegistoryRef,
                     public detail::LuaTableImpl<LuaTable>,
                     public detail::LuaTableOrUserDataImpl<LuaTable>,
                     public detail::LuaBasicTypeFunctions<LuaTable> {

        void typecheck() {
            int t = type();
            if (t != TYPE_TABLE && t != TYPE_NIL && t != TYPE_NONE) {
                except::typeMismatchError(state(), "not table");
                unref();
            }
        }

    public:
        operator LuaRef() {
            push(state());
            return LuaRef(state(), StackTop());
        }
        LuaTable(lua_State *state, StackTop) : Ref::RegistoryRef(state, StackTop()) { typecheck(); }
        LuaTable(lua_State *state, const NewTable &table) : Ref::RegistoryRef(state, table) {
            typecheck();
        }
        explicit LuaTable(lua_State *state) : Ref::RegistoryRef(state, NewTable()) { typecheck(); }
        LuaTable() { typecheck(); }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaTable
    template<>
    struct lua_type_traits<LuaTable>
    {
        typedef LuaTable get_type;
        typedef LuaTable push_type;

        static bool strictCheckType(lua_State *l, int index) { return lua_istable(l, index); }
        static bool checkType(lua_State *l, int index) {
            return lua_istable(l, index) || lua_isnil(l, index);
        }
        static LuaTable get(lua_State *l, int index) {
            lua_pushvalue(l, index);
            return LuaTable(l, StackTop());
        }
        static int push(lua_State *l, const LuaTable &ref) { return ref.push(l); }
    };
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaTable
    template<>
    struct lua_type_traits<const LuaTable &> : lua_type_traits<LuaTable>
    {
    };

    /// @ingroup Lua_reference_types
    /// @brief Reference to Lua function.
    class LuaFunction : public Ref::RegistoryRef,
                        public detail::LuaFunctionImpl<LuaFunction>,
                        public detail::LuaBasicTypeFunctions<LuaFunction> {
        void typecheck() {
            int t = type();
            if (t != TYPE_FUNCTION && t != TYPE_NIL && t != TYPE_NONE) {
                except::typeMismatchError(state(), "not function");
                RegistoryRef::unref();
            }
        }

        struct LuaLoadStreamWrapper
        {
            LuaLoadStreamWrapper(std::istream &stream) : preloaded_(false), stream_(stream) {
                buffer_.reserve(512);
                skipComment();
                preloaded_ = !buffer_.empty();
            }

            void skipComment() {
                // skip bom
                const char *bom = "\xEF\xBB\xBF";
                const char *bomseq = bom;
                char c;
                while (stream_.get(c)) {
                    if (c != *bomseq)// not bom sequence
                    {
                        buffer_.assign(bom, bomseq);
                        buffer_.push_back(c);
                        break;
                    }
                    bomseq++;
                    if ('\0' == *bomseq) { return; }
                }

                // skip comment
                if (!buffer_.empty() && buffer_.front() == '#') {
                    buffer_.clear();
                    std::string comment;
                    std::getline(stream_, comment);
                }
            }

            static const char *getdata(lua_State *, void *ud, size_t *size) {
                LuaLoadStreamWrapper *loader = static_cast<LuaLoadStreamWrapper *>(ud);

                if (loader->preloaded_) {
                    loader->preloaded_ = false;
                } else {
                    loader->buffer_.clear();
                }

                char c = 0;
                while (loader->buffer_.size() < loader->buffer_.capacity() &&
                       loader->stream_.get(c)) {
                    loader->buffer_.push_back(c);
                }
                *size = loader->buffer_.size();
                return loader->buffer_.empty() ? 0 : &loader->buffer_[0];
            }

        private:
            bool preloaded_;
            std::vector<char> buffer_;
            std::istream &stream_;
        };

    public:
        /// @brief construct with state and function .
        /// @param state pointer to lua_State
        /// @param f execute function for lua thread. e.g.
        /// LuaWrapper::function(function_ptr),LuaWrapper::overload(function_ptr)
        template<typename F>
        LuaFunction(lua_State *state, F f) : Ref::RegistoryRef(state, f) {
            typecheck();
        }

        /// @brief construct with stack top value.
        /// @param state pointer to lua_State
        LuaFunction(lua_State *state, StackTop) : Ref::RegistoryRef(state, StackTop()) {
            typecheck();
        }

        /// @brief construct with nil reference.
        LuaFunction() {}

        /// @brief load lua code .
        /// @param state pointer to lua_State
        /// @param luacode string
        static LuaFunction loadstring(lua_State *state, const std::string &luacode) {
            return loadstring(state, luacode.c_str());
        }
        /// @brief load lua code .
        /// @param state pointer to lua_State
        /// @param luacode string
        static LuaFunction loadstring(lua_State *state, const char *luacode) {
            util::ScopedSavedStack save(state);
            int status = luaL_loadstring(state, luacode);

            if (status) {
                ErrorHandler::handle(status, state);
                lua_pushnil(state);
            }
            return LuaFunction(state, StackTop());
        }

        /// @brief If there are no errors,compiled file as a Lua function and return.
        ///  Otherwise send error message to error handler and return nil reference
        /// @param state pointer to lua_State
        /// @param file  file path of lua script
        /// @return reference of lua function
        static LuaFunction loadfile(lua_State *state, const std::string &file) {
            return loadfile(state, file.c_str());
        }

        /// @brief If there are no errors,compiled file as a Lua function and return.
        ///  Otherwise send error message to error handler and return nil reference
        /// @param state pointer to lua_State
        /// @param file  file path of lua script
        /// @return reference of lua function
        static LuaFunction loadfile(lua_State *state, const char *file) {
            util::ScopedSavedStack save(state);

            int status = luaL_loadfile(state, file);

            if (status) {
                ErrorHandler::handle(status, state);
                lua_pushnil(state);
            }
            return LuaFunction(state, StackTop());
        }

        /// @brief If there are no errors,compiled stream as a Lua function and
        /// return.
        ///  Otherwise send error message to error handler and return nil reference
        /// @param state pointer to lua_State
        /// @param stream  stream of lua script data
        /// @param chunkname  use for error message.
        /// @return reference of lua function
        static LuaStackRef loadstreamtostack(lua_State *state, std::istream &stream,
                                             const char *chunkname = 0) {
            LuaLoadStreamWrapper wrapper(stream);
#if LUA_VERSION_NUM >= 502
            int status = lua_load(state, &LuaLoadStreamWrapper::getdata, &wrapper, chunkname, 0);
#else
            int status = lua_load(state, &LuaLoadStreamWrapper::getdata, &wrapper, chunkname);
#endif
            if (status) {
                ErrorHandler::handle(status, state);
                lua_pushnil(state);
            }
            return LuaStackRef(state, -1, true);
        }

        /// @brief If there are no errors,compiled stream as a Lua function and
        /// return.
        ///  Otherwise send error message to error handler and return nil reference
        /// @param state pointer to lua_State
        /// @param stream  stream of lua script data
        /// @param chunkname  use for error message.
        /// @return reference of lua function
        static LuaFunction loadstream(lua_State *state, std::istream &stream,
                                      const char *chunkname = 0) {
            util::ScopedSavedStack save(state);
            LuaLoadStreamWrapper wrapper(stream);
#if LUA_VERSION_NUM >= 502
            int status = lua_load(state, &LuaLoadStreamWrapper::getdata, &wrapper, chunkname, 0);
#else
            int status = lua_load(state, &LuaLoadStreamWrapper::getdata, &wrapper, chunkname);
#endif
            if (status) {
                ErrorHandler::handle(status, state);
                lua_pushnil(state);
            }
            return LuaFunction(state, StackTop());
        }
    };

    /// @ingroup Lua_reference_types
    /// @brief Reference to Lua thread(coroutine).
    class LuaThread : public Ref::RegistoryRef,
                      public detail::LuaThreadImpl<LuaThread>,
                      public detail::LuaBasicTypeFunctions<LuaThread> {
        void typecheck() {
            int t = type();
            if (t != TYPE_THREAD && t != TYPE_NIL && t != TYPE_NONE) {
                except::typeMismatchError(state(), "not lua thread");
                RegistoryRef::unref();
            }
        }

    public:
        /// @brief construct with stack top value.
        LuaThread(lua_State *state, StackTop) : Ref::RegistoryRef(state, StackTop()) {
            typecheck();
        }
        /// @brief construct with new thread.
        LuaThread(lua_State *state, const NewThread &t) : Ref::RegistoryRef(state, t) {}
        /// @brief construct with nil reference.
        LuaThread(lua_State *state) : Ref::RegistoryRef(state, NewThread()) {}
        /// @brief construct with nil reference.
        LuaThread() {}
        /// @brief get lua thread
        operator lua_State *() { return getthread(); }
    };
}// namespace LuaWrapper

#if METAENGINE_LUAWRAPPER_USE_CPP11
#else
namespace std {
    template<>
    inline void swap(LuaWrapper::LuaUserData &a, LuaWrapper::LuaUserData &b) {
        a.swap(b);
    }
    template<>
    inline void swap(LuaWrapper::LuaTable &a, LuaWrapper::LuaTable &b) {
        a.swap(b);
    }
    template<>
    inline void swap(LuaWrapper::LuaFunction &a, LuaWrapper::LuaFunction &b) {
        a.swap(b);
    }
    template<>
    inline void swap(LuaWrapper::LuaThread &a, LuaWrapper::LuaThread &b) {
        a.swap(b);
    }
    template<>
    inline void swap(LuaWrapper::LuaRef &a, LuaWrapper::LuaRef &b) {
        a.swap(b);
    }
}// namespace std
#endif

#if METAENGINE_LUAWRAPPER_USE_CPP11

namespace LuaWrapper {
    namespace nativefunction {
        template<std::size_t... indexes>
        struct index_tuple
        {
        };

        template<std::size_t first, std::size_t last, class result = index_tuple<>,
                 bool flag = first >= last>
        struct index_range
        {
            using type = result;
        };

        template<std::size_t step, std::size_t last, std::size_t... indexes>
        struct index_range<step, last, index_tuple<indexes...>, false>
            : index_range<step + 1, last, index_tuple<indexes..., step>>
        {
        };

        template<class F, class Ret, class... Args, size_t... Indexes>
        int _call_apply(lua_State *state, F &&f, index_tuple<Indexes...>,
                        util::FunctionSignatureType<Ret, Args...>) {
            return util::push_args(state,
                                   util::invoke(f, lua_type_traits<Args>::get(state, Indexes)...));
        }
        template<class F, class... Args, size_t... Indexes>
        int _call_apply(lua_State *state, F &&f, index_tuple<Indexes...>,
                        util::FunctionSignatureType<void, Args...>) {
            METAENGINE_LUAWRAPPER_UNUSED(state);
            util::invoke(f, lua_type_traits<Args>::get(state, Indexes)...);
            return 0;
        }

        inline bool all_true() { return true; }
        template<class Arg, class... Args>
        bool all_true(Arg f, Args... args) {// check from backward and lazy evaluation
            return all_true(args...) && bool(f);
        }

        inline void join(std::string &, const char *) {}
        template<class Arg, class... Args>
        void join(std::string &result, const char *delim, const Arg &str, const Args &...args) {
            result += str;
            result += delim;
            join(result, delim, args...);
        }

        template<typename T>
        struct _wcheckeval
        {
            _wcheckeval(lua_State *s, int i, bool opt) : state(s), index(i), opt_arg(opt) {}
            lua_State *state;
            int index;
            bool opt_arg;
            operator bool() {
                return (opt_arg && lua_isnoneornil(state, index)) ||
                       lua_type_traits<T>::checkType(state, index);
            }
        };

        template<typename T>
        struct _scheckeval
        {
            _scheckeval(lua_State *s, int i, bool opt) : state(s), index(i), opt_arg(opt) {}
            lua_State *state;
            int index;
            bool opt_arg;
            operator bool() {
                return (opt_arg && lua_isnoneornil(state, index)) ||
                       lua_type_traits<T>::strictCheckType(state, index);
            }
        };

        template<class... Args, size_t... Indexes>
        bool _ctype_apply(lua_State *state, index_tuple<Indexes...>, util::TypeTuple<Args...>,
                          int opt_count) {
            METAENGINE_LUAWRAPPER_UNUSED(state);
            METAENGINE_LUAWRAPPER_UNUSED(opt_count);
            return all_true(
                    _wcheckeval<Args>(state, Indexes, sizeof...(Indexes) - opt_count < Indexes)...);
        }
        template<class... Args, size_t... Indexes>
        bool _sctype_apply(lua_State *state, index_tuple<Indexes...>, util::TypeTuple<Args...>,
                           int opt_count) {
            METAENGINE_LUAWRAPPER_UNUSED(state);
            METAENGINE_LUAWRAPPER_UNUSED(opt_count);
            return all_true(
                    _scheckeval<Args>(state, Indexes, sizeof...(Indexes) - opt_count < Indexes)...);
        }

        template<class... Args, size_t... Indexes>
        std::string _type_name_apply(index_tuple<Indexes...>, util::TypeTuple<Args...>,
                                     int opt_count) {
            METAENGINE_LUAWRAPPER_UNUSED(opt_count);
            std::string result;
            const int max_arg = sizeof...(Args);
            join(result, ",",
                 (((max_arg - opt_count < int(Indexes)) ? std::string("[OPT]") : std::string("")) +
                  util::pretty_name(typeid(Args)))...);
            return result;
        }

        template<class F>
        int call(lua_State *state, F &&f) {
            typedef typename traits::decay<F>::type ftype;
            typedef typename util::FunctionSignature<ftype>::type fsigtype;
            typedef typename index_range<1, fsigtype::argument_count + 1>::type index;
            return _call_apply(state, f, index(), fsigtype());
        }
        template<class F>
        bool checkArgTypes(lua_State *state, const F &, int opt_count = 0) {
            typedef typename traits::decay<F>::type ftype;
            typedef typename util::FunctionSignature<ftype>::type fsigtype;
            typedef typename index_range<1, fsigtype::argument_count + 1>::type index;
            typedef typename fsigtype::argument_type_tuple argument_type_tuple;
            return _ctype_apply(state, index(), argument_type_tuple(), opt_count);
        }
        template<class F>
        bool strictCheckArgTypes(lua_State *state, const F &, int opt_count = 0) {
            typedef typename traits::decay<F>::type ftype;
            typedef typename util::FunctionSignature<ftype>::type fsigtype;
            typedef typename index_range<1, fsigtype::argument_count + 1>::type index;
            typedef typename fsigtype::argument_type_tuple argument_type_tuple;
            return _sctype_apply(state, index(), argument_type_tuple(), opt_count);
        }

        template<class F>
        std::string argTypesName(const F &, int opt_count = 0) {
            typedef typename traits::decay<F>::type ftype;
            typedef typename util::FunctionSignature<ftype>::type fsigtype;
            typedef typename index_range<1, fsigtype::argument_count + 1>::type index;
            typedef typename fsigtype::argument_type_tuple argument_type_tuple;
            return _type_name_apply(index(), argument_type_tuple(), opt_count);
        }
        template<class F>
        int minArgCount(const F &) {
            typedef typename traits::decay<F>::type ftype;
            typedef typename util::FunctionSignature<ftype>::type fsigtype;
            return fsigtype::argument_count;
        }
        template<class F>
        int maxArgCount(const F &) {
            typedef typename traits::decay<F>::type ftype;
            typedef typename util::FunctionSignature<ftype>::type fsigtype;
            return fsigtype::argument_count;
        }

        // for constructor
        template<typename T>
        struct ConstructorFunctor;

        template<typename ClassType, typename... Args>
        struct ConstructorFunctor<util::FunctionSignatureType<ClassType, Args...>>
        {
            typedef util::FunctionSignatureType<ClassType, Args...> signature_type;
            typedef typename index_range<1, sizeof...(Args) + 1>::type get_index;

            template<size_t... Indexes>
            int invoke(lua_State *L, index_tuple<Indexes...>) const {
                typedef ObjectWrapper<ClassType> wrapper_type;
                void *storage = lua_newuserdata(L, sizeof(wrapper_type));
                try {
                    new (storage) wrapper_type(lua_type_traits<Args>::get(L, Indexes)...);
                } catch (...) {
                    lua_pop(L, 1);
                    throw;
                }

                class_userdata::setmetatable<ClassType>(L);
                return 1;
            }

            int operator()(lua_State *L) const { return invoke(L, get_index()); }

            bool checkArgTypes(lua_State *L, int opt_count = 0) const {
                return _ctype_apply(L, get_index(), typename signature_type::argument_type_tuple(),
                                    opt_count);
            }
            bool strictCheckArgTypes(lua_State *L, int opt_count = 0) const {
                return _sctype_apply(L, get_index(), typename signature_type::argument_type_tuple(),
                                     opt_count);
            }
            std::string argTypesName(int opt_count = 0) const {
                return _type_name_apply(get_index(), typename signature_type::argument_type_tuple(),
                                        opt_count);
            }
        };

        template<typename ClassType, typename... Args>
        struct ConstructorFunction;
        template<typename ClassType, typename... Args>
        struct ConstructorFunction<ClassType(Args...)>
        {
            typedef ConstructorFunctor<util::FunctionSignatureType<ClassType, Args...>> type;
        };
        template<typename ClassType, typename RetType, typename... Args>
        struct ConstructorFunction<ClassType,
                                   RetType(Args...)>// class type check version
        {
            typedef ConstructorFunctor<util::FunctionSignatureType<ClassType, Args...>> type;
        };
    }// namespace nativefunction
    using nativefunction::ConstructorFunction;
}// namespace LuaWrapper
#else

#include <string>

namespace LuaWrapper {
    namespace nativefunction {

#define METAENGINE_LUAWRAPPER_INVOKE_SIG_TARG_DEF_CONCAT_REP(N)                                    \
    , typename util::ArgumentType<N - 1, F>::type
#define METAENGINE_LUAWRAPPER_INVOKE_SIG_TARG_DEF_REPEAT_CONCAT(N)                                 \
    METAENGINE_LUAWRAPPER_PP_REPEAT(N, METAENGINE_LUAWRAPPER_INVOKE_SIG_TARG_DEF_CONCAT_REP)

#define METAENGINE_LUAWRAPPER_GET_REP(N)                                                           \
    , lua_type_traits<METAENGINE_LUAWRAPPER_PP_CAT(A, N)>::get(state, N)
#define METAENGINE_LUAWRAPPER_FUNC_DEF(N)                                                          \
    const util::FunctionSignatureType<Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>  \
            &fsig
#define METAENGINE_LUAWRAPPER_TYPENAME_REP(N)                                                      \
    +((MAX_ARG - opt_count < N) ? "[OPT]" : "") +                                                  \
            util::pretty_name(typeid(METAENGINE_LUAWRAPPER_PP_CAT(A, N))) + ","
#define METAENGINE_LUAWRAPPER_TYPECHECK_REP(N)                                                     \
    &&(((MAX_ARG - opt_count < N) && lua_isnoneornil(state, N)) ||                                 \
       lua_type_traits<METAENGINE_LUAWRAPPER_PP_CAT(A, N)>::checkType(state, N))
#define METAENGINE_LUAWRAPPER_STRICT_TYPECHECK_REP(N)                                              \
    &&(((MAX_ARG - opt_count < N) && lua_isnoneornil(state, N)) ||                                 \
       lua_type_traits<METAENGINE_LUAWRAPPER_PP_CAT(A, N)>::strictCheckType(state, N))
#define METAENGINE_LUAWRAPPER_CALL_FN_DEF(N)                                                       \
    template<typename F, typename Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>      \
    inline typename traits::enable_if<!traits::is_same<void, Ret>::value, int>::type _call_apply(  \
            lua_State *state, F &f, METAENGINE_LUAWRAPPER_FUNC_DEF(N)) {                           \
        METAENGINE_LUAWRAPPER_UNUSED(fsig);                                                        \
        return util::push_args(                                                                    \
                state,                                                                             \
                util::invoke<F METAENGINE_LUAWRAPPER_INVOKE_SIG_TARG_DEF_REPEAT_CONCAT(N)>(        \
                        f METAENGINE_LUAWRAPPER_PP_REPEAT(N, METAENGINE_LUAWRAPPER_GET_REP)));     \
    }                                                                                              \
    template<typename F, typename Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>      \
    inline typename traits::enable_if<traits::is_same<void, Ret>::value, int>::type _call_apply(   \
            lua_State *state, F &f, METAENGINE_LUAWRAPPER_FUNC_DEF(N)) {                           \
        METAENGINE_LUAWRAPPER_UNUSED(state);                                                       \
        METAENGINE_LUAWRAPPER_UNUSED(fsig);                                                        \
        util::invoke<F METAENGINE_LUAWRAPPER_INVOKE_SIG_TARG_DEF_REPEAT_CONCAT(N)>(                \
                f METAENGINE_LUAWRAPPER_PP_REPEAT(N, METAENGINE_LUAWRAPPER_GET_REP));              \
        return 0;                                                                                  \
    }                                                                                              \
    template<typename Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>                  \
    bool _ctype_apply(lua_State *state, METAENGINE_LUAWRAPPER_FUNC_DEF(N), int opt_count = 0) {    \
        METAENGINE_LUAWRAPPER_UNUSED(state);                                                       \
        METAENGINE_LUAWRAPPER_UNUSED(opt_count);                                                   \
        METAENGINE_LUAWRAPPER_UNUSED(fsig);                                                        \
        const int MAX_ARG = N;                                                                     \
        (void) MAX_ARG;                                                                            \
        return true METAENGINE_LUAWRAPPER_PP_REVERSE_REPEAT(N,                                     \
                                                            METAENGINE_LUAWRAPPER_TYPECHECK_REP);  \
    }                                                                                              \
    template<typename Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>                  \
    bool _sctype_apply(lua_State *state, METAENGINE_LUAWRAPPER_FUNC_DEF(N), int opt_count = 0) {   \
        METAENGINE_LUAWRAPPER_UNUSED(state);                                                       \
        METAENGINE_LUAWRAPPER_UNUSED(opt_count);                                                   \
        METAENGINE_LUAWRAPPER_UNUSED(fsig);                                                        \
        const int MAX_ARG = N;                                                                     \
        (void) MAX_ARG;                                                                            \
        return true METAENGINE_LUAWRAPPER_PP_REVERSE_REPEAT(                                       \
                N, METAENGINE_LUAWRAPPER_STRICT_TYPECHECK_REP);                                    \
    }                                                                                              \
    template<typename Ret METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>                  \
    std::string _type_name_apply(METAENGINE_LUAWRAPPER_FUNC_DEF(N), int opt_count) {               \
        METAENGINE_LUAWRAPPER_UNUSED(fsig);                                                        \
        METAENGINE_LUAWRAPPER_UNUSED(opt_count);                                                   \
        const int MAX_ARG = N;                                                                     \
        (void) MAX_ARG;                                                                            \
        return std::string()                                                                       \
                METAENGINE_LUAWRAPPER_PP_REPEAT(N, METAENGINE_LUAWRAPPER_TYPENAME_REP);            \
    }

        METAENGINE_LUAWRAPPER_CALL_FN_DEF(0)
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_CALL_FN_DEF)
#undef METAENGINE_LUAWRAPPER_CALL_FN_DEF

#undef METAENGINE_LUAWRAPPER_CALL_FN_DEF
#undef METAENGINE_LUAWRAPPER_FUNC_DEF

        template<class F>
        int call(lua_State *state, F f) {
            typedef typename traits::decay<F>::type ftype;
            typedef typename util::FunctionSignature<ftype>::type fsigtype;
            return _call_apply(state, f, fsigtype());
        }
        template<class F>
        bool checkArgTypes(lua_State *state, const F &, int opt_count = 0) {
            typedef typename traits::decay<F>::type ftype;
            typedef typename util::FunctionSignature<ftype>::type fsigtype;
            return _ctype_apply(state, fsigtype(), opt_count);
        }
        template<class F>
        bool strictCheckArgTypes(lua_State *state, const F &, int opt_count = 0) {
            typedef typename traits::decay<F>::type ftype;
            typedef typename util::FunctionSignature<ftype>::type fsigtype;
            return _sctype_apply(state, fsigtype(), opt_count);
        }

        template<class F>
        std::string argTypesName(const F &f, int opt_count = 0) {
            METAENGINE_LUAWRAPPER_UNUSED(f);
            typedef typename traits::decay<F>::type ftype;
            typedef typename util::FunctionSignature<ftype>::type fsigtype;
            return _type_name_apply(fsigtype(), opt_count);
        }
        template<class F>
        int minArgCount(const F &f) {
            METAENGINE_LUAWRAPPER_UNUSED(f);
            typedef typename traits::decay<F>::type ftype;
            typedef typename util::FunctionSignature<ftype>::type fsigtype;
            return fsigtype::argument_count;
        }
        template<class F>
        int maxArgCount(const F &f) {
            METAENGINE_LUAWRAPPER_UNUSED(f);
            typedef typename traits::decay<F>::type ftype;
            typedef typename util::FunctionSignature<ftype>::type fsigtype;
            return fsigtype::argument_count;
        }

        ///! for constructor
        template<typename T>
        struct ConstructorFunctor;

#define METAENGINE_LUAWRAPPER_CONSTRUCTOR_GET_REP(N)                                               \
    lua_type_traits<METAENGINE_LUAWRAPPER_PP_CAT(A, N)>::get(L, N)
#define METAENGINE_LUAWRAPPER_CONSTRUCTOR_CALL_FN_DEF(N)                                           \
    template<typename ClassType METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>            \
    struct ConstructorFunctor<util::FunctionSignatureType<                                         \
            ClassType METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>>                     \
    {                                                                                              \
        typedef util::FunctionSignatureType<                                                       \
                ClassType METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>                  \
                signature_type;                                                                    \
        int operator()(lua_State *L) const {                                                       \
            typedef ObjectWrapper<ClassType> wrapper_type;                                         \
            void *storage = lua_newuserdata(L, sizeof(wrapper_type));                              \
            try {                                                                                  \
                new (storage) wrapper_type(METAENGINE_LUAWRAPPER_PP_REPEAT_ARG(                    \
                        N, METAENGINE_LUAWRAPPER_CONSTRUCTOR_GET_REP));                            \
            } catch (...) {                                                                        \
                lua_pop(L, 1);                                                                     \
                throw;                                                                             \
            }                                                                                      \
            class_userdata::setmetatable<ClassType>(L);                                            \
            return 1;                                                                              \
        }                                                                                          \
        bool checkArgTypes(lua_State *L, int opt_count = 0) const {                                \
            return _ctype_apply(L, signature_type(), opt_count);                                   \
        }                                                                                          \
        bool strictCheckArgTypes(lua_State *L, int opt_count = 0) const {                          \
            return _sctype_apply(L, signature_type(), opt_count);                                  \
        }                                                                                          \
        std::string argTypesName(int opt_count = 0) const {                                        \
            METAENGINE_LUAWRAPPER_UNUSED(opt_count);                                               \
            return _type_name_apply(signature_type(), 0);                                          \
        }                                                                                          \
    };

        METAENGINE_LUAWRAPPER_CONSTRUCTOR_CALL_FN_DEF(0)
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_CONSTRUCTOR_CALL_FN_DEF)
#undef METAENGINE_LUAWRAPPER_CONSTRUCTOR_CALL_FN_DEF

        template<class ClassType, class FunType = void>
        struct ConstructorFunction;

#define METAENGINE_LUAWRAPPER_F_TO_CONSIG_TYPE_DEF(N)                                              \
    ConstructorFunctor<util::FunctionSignatureType<                                                \
            ClassType METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>>
#define METAENGINE_LUAWRAPPER_F_TO_CONSIG_DEF(N)                                                   \
    template<typename ClassType METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>            \
    struct ConstructorFunction<ClassType(METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N))>         \
    {                                                                                              \
        typedef METAENGINE_LUAWRAPPER_F_TO_CONSIG_TYPE_DEF(N) type;                                \
    };                                                                                             \
    template<typename ClassType,                                                                   \
             typename RetType METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)>              \
    struct ConstructorFunction<ClassType,                                                          \
                               RetType(METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N))>           \
    {                                                                                              \
        typedef METAENGINE_LUAWRAPPER_F_TO_CONSIG_TYPE_DEF(N) type;                                \
    };

        METAENGINE_LUAWRAPPER_F_TO_CONSIG_DEF(0)
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_F_TO_CONSIG_DEF)
#undef METAENGINE_LUAWRAPPER_F_TO_CONSIG_DEF
    }// namespace nativefunction
    using nativefunction::ConstructorFunction;
}// namespace LuaWrapper
#endif

namespace LuaWrapper {
    namespace fntuple {

#if METAENGINE_LUAWRAPPER_USE_CPP11 && !defined(METAENGINE_LUAWRAPPER_FUNCTION_MAX_OVERLOADS)
        // In Clang with libstdc++.
        // std::tuple elements is limited to 16 for template depth limit
        using std::get;
        using std::tuple;
        using std::tuple_element;
        using std::tuple_size;
#else
        using util::null_type;
// boost::tuple is max
#define METAENGINE_LUAWRAPPER_PP_STRUCT_TDEF_REP(N)                                                \
    METAENGINE_LUAWRAPPER_PP_CAT(typename A, N) = null_type
#define METAENGINE_LUAWRAPPER_PP_STRUCT_TEMPLATE_DEF_REPEAT(N)                                     \
    METAENGINE_LUAWRAPPER_PP_REPEAT_ARG(N, METAENGINE_LUAWRAPPER_PP_STRUCT_TDEF_REP)

        template<METAENGINE_LUAWRAPPER_PP_STRUCT_TEMPLATE_DEF_REPEAT(
                METAENGINE_LUAWRAPPER_PP_INC(METAENGINE_LUAWRAPPER_FUNCTION_MAX_OVERLOADS))>
        struct tuple
        {
        };
#undef METAENGINE_LUAWRAPPER_PP_STRUCT_TDEF_REP
#undef METAENGINE_LUAWRAPPER_PP_STRUCT_TEMPLATE_DEF_REPEAT

#define METAENGINE_LUAWRAPPER_FUNCTION_TUPLE_ELEMENT(N)                                            \
    METAENGINE_LUAWRAPPER_PP_CAT(A, N)                                                             \
    METAENGINE_LUAWRAPPER_PP_CAT(elem, N);
#define METAENGINE_LUAWRAPPER_FUNCTION_TUPLE_ELEMENT_INIT(N)                                       \
    METAENGINE_LUAWRAPPER_PP_CAT(elem, N)                                                          \
    (METAENGINE_LUAWRAPPER_PP_CAT(a, N))
#define METAENGINE_LUAWRAPPER_FUNCTION_TUPLE_IMPL_DEF(N)                                           \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    struct tuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>                                  \
    {                                                                                              \
        METAENGINE_LUAWRAPPER_PP_REPEAT(N, METAENGINE_LUAWRAPPER_FUNCTION_TUPLE_ELEMENT)           \
        tuple(METAENGINE_LUAWRAPPER_PP_ARG_DEF_REPEAT(N))                                          \
            : METAENGINE_LUAWRAPPER_PP_REPEAT_ARG(                                                 \
                      N, METAENGINE_LUAWRAPPER_FUNCTION_TUPLE_ELEMENT_INIT) {}                     \
    };

        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_OVERLOADS,
                                            METAENGINE_LUAWRAPPER_FUNCTION_TUPLE_IMPL_DEF)

        template<typename Tuple>
        struct tuple_size;

#define METAENGINE_LUAWRAPPER_TUPLE_SIZE_DEF(N)                                                    \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    struct tuple_size<tuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>>                      \
    {                                                                                              \
        static const size_t value = N;                                                             \
    };

        METAENGINE_LUAWRAPPER_TUPLE_SIZE_DEF(0)
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_OVERLOADS,
                                            METAENGINE_LUAWRAPPER_TUPLE_SIZE_DEF)
#undef METAENGINE_LUAWRAPPER_TUPLE_SIZE_DEF

        template<std::size_t remain, class result, bool flag = remain <= 0>
        struct tuple_element
        {
        };
#define METAENGINE_LUAWRAPPER_TUPLE_ELEMENT_DEF(N)                                                 \
    template<std::size_t remain, class arg METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)> \
    struct tuple_element<remain,                                                                   \
                         tuple<arg METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>, true>  \
    {                                                                                              \
        typedef arg type;                                                                          \
    };                                                                                             \
    template<std::size_t remain, class arg METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT_CONCAT(N)> \
    struct tuple_element<remain,                                                                   \
                         tuple<arg METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT_CONCAT(N)>, false> \
        : tuple_element<remain - 1, tuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>>        \
    {                                                                                              \
    };

        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_OVERLOADS,
                                            METAENGINE_LUAWRAPPER_TUPLE_ELEMENT_DEF)

#undef METAENGINE_LUAWRAPPER_TUPLE_SIZE_DEF

        template<size_t S, typename T>
        struct tuple_get_helper;
#define METAENGINE_LUAWRAPPER_TUPLE_GET_DEF(N)                                                     \
    template<typename T>                                                                           \
    struct tuple_get_helper<N, T>                                                                  \
    {                                                                                              \
        static typename tuple_element<N - 1, T>::type &get(T &t) {                                 \
            return t.METAENGINE_LUAWRAPPER_PP_CAT(elem, N);                                        \
        }                                                                                          \
        static const typename tuple_element<N - 1, T>::type &cget(const T &t) {                    \
            return t.METAENGINE_LUAWRAPPER_PP_CAT(elem, N);                                        \
        }                                                                                          \
    };
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_OVERLOADS,
                                            METAENGINE_LUAWRAPPER_TUPLE_GET_DEF)

        template<size_t S, typename T>
        typename tuple_element<S, T>::type &get(T &t) {
            return tuple_get_helper<S + 1, T>::get(t);
        }
        template<size_t S, typename T>
        const typename tuple_element<S, T>::type &get(const T &t) {
            return tuple_get_helper<S + 1, T>::cget(t);
        }
#endif
    }// namespace fntuple
}// namespace LuaWrapper

namespace LuaWrapper {
    struct FunctionImpl
    {
        virtual int invoke(lua_State *state) = 0;
        virtual std::string argTypesName() const = 0;
        virtual bool checkArgTypes(lua_State *state) const = 0;
        virtual bool strictCheckArgTypes(lua_State *state) const = 0;
        virtual int minArgCount() const = 0;
        virtual int maxArgCount() const = 0;
        virtual ~FunctionImpl() {}
    };
    struct PolymorphicInvoker
    {
        typedef standard::shared_ptr<FunctionImpl> holder_type;
        PolymorphicInvoker(const holder_type &fptr) : fnc(fptr) {}
        int invoke(lua_State *state) const { return fnc->invoke(state); }
        std::string argTypesName() const { return fnc->argTypesName(); }
        bool checkArgTypes(lua_State *state) const { return fnc->checkArgTypes(state); }
        bool strictCheckArgTypes(lua_State *state) const { return fnc->strictCheckArgTypes(state); }
        int minArgCount() const { return fnc->minArgCount(); }
        int maxArgCount() const { return fnc->maxArgCount(); }
        ~PolymorphicInvoker() {}

    private:
        holder_type fnc;
    };
    struct PolymorphicMemberInvoker : PolymorphicInvoker
    {
        PolymorphicMemberInvoker(const holder_type &fptr) : PolymorphicInvoker(fptr) {}
    };

    namespace nativefunction {
        template<size_t INDEX, typename F>
        typename lua_type_traits<typename util::ArgumentType<INDEX, F>::type>::get_type getArgument(
                lua_State *state) {
            return lua_type_traits<typename util::ArgumentType<INDEX, F>::type>::get(state,
                                                                                     INDEX + 1);
        }

        template<typename T, typename Enable = void>
        struct is_callable
            : traits::integral_constant<
                      bool,
                      !traits::is_same<void, typename util::FunctionSignature<T>::type>::value>
        {
        };

        template<class MemType, class T>
        struct is_callable<MemType T::*> : traits::integral_constant<bool, true>
        {
        };

        template<typename T>
        struct is_callable<ConstructorFunctor<T>> : traits::integral_constant<bool, true>
        {
        };

        // for constructors
        template<class T>
        int call(lua_State *state, ConstructorFunctor<T> &con) {
            return con(state);
        }
        template<class T>
        int call(lua_State *state, const ConstructorFunctor<T> &con) {
            return con(state);
        }
        template<class T>
        bool checkArgTypes(lua_State *state, const ConstructorFunctor<T> &con, int opt_count = 0) {
            return con.checkArgTypes(state, opt_count);
        }
        template<class T>
        bool strictCheckArgTypes(lua_State *state, const ConstructorFunctor<T> &con,
                                 int opt_count = 0) {
            return con.strictCheckArgTypes(state, opt_count);
        }
        template<class T>
        std::string argTypesName(const ConstructorFunctor<T> &con) {
            return con.argTypesName();
        }
        template<class T>
        int minArgCount(const ConstructorFunctor<T> &) {
            return ConstructorFunctor<T>::signature_type::argument_count;
        }
        template<class T>
        int maxArgCount(const ConstructorFunctor<T> &) {
            return ConstructorFunctor<T>::signature_type::argument_count;
        }

        // for data member
        // using is_member_function_pointer in MSVC2010 : fatal error LNK1179: invalid
        // or corrupt file: duplicate COMDAT
        // '?value@?$result_@P8ABC@test_02_classreg@@AE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ@?$is_mem_fun_pointer_select@$0A@@detail@boost@@2_NB'
        template<class MemType, class T>
        typename traits::enable_if<traits::is_object<MemType>::value, int>::type call(
                lua_State *state, MemType T::*mptr) {
            T *this_ = lua_type_traits<T *>::get(state, 1);
            if (lua_gettop(state) == 1) {
                if (!this_) {
                    const T &this_ = lua_type_traits<const T &>::get(state, 1);
                    if (is_usertype<MemType>::value && !traits::is_pointer<MemType>::value) {
                        return util::push_args(
                                state, standard::reference_wrapper<const MemType>(this_.*mptr));
                    } else {
                        return util::push_args(state, this_.*mptr);
                    }
                } else {
                    if (is_usertype<MemType>::value && !traits::is_pointer<MemType>::value) {
                        return util::push_args(state,
                                               standard::reference_wrapper<MemType>(this_->*mptr));
                    } else {
                        return util::push_args(state, this_->*mptr);
                    }
                }
            } else {
                if (!this_) { throw LuaTypeMismatch(); }
                this_->*mptr = lua_type_traits<MemType>::get(state, 2);
                return 0;
            }
        }
        template<class MemType, class T>
        typename traits::enable_if<traits::is_object<MemType>::value, bool>::type checkArgTypes(
                lua_State *state, MemType T::*, int opt_count = 0) {
            METAENGINE_LUAWRAPPER_UNUSED(opt_count);
            if (lua_gettop(state) >= 2) {
                // setter typecheck
                return lua_type_traits<MemType>::checkType(state, 2) &&
                       lua_type_traits<T>::checkType(state, 1);
            }
            // getter typecheck
            return lua_type_traits<T>::checkType(state, 1);
        }
        template<class MemType, class T>
        typename traits::enable_if<traits::is_object<MemType>::value, bool>::type
        strictCheckArgTypes(lua_State *state, MemType T::*, int opt_count = 0) {
            METAENGINE_LUAWRAPPER_UNUSED(opt_count);
            if (lua_gettop(state) == 2) {
                // setter typecheck
                return lua_type_traits<MemType>::strictCheckType(state, 2) &&
                       lua_type_traits<T>::strictCheckType(state, 1);
            }
            // getter typecheck
            return lua_type_traits<T>::strictCheckType(state, 1);
        }
        template<class MemType, class T>
        typename traits::enable_if<traits::is_object<MemType>::value, std::string>::type
        argTypesName(MemType T::*) {
            return util::pretty_name(typeid(T *)) + ",[OPT] " + util::pretty_name(typeid(MemType));
        }
        template<class MemType, class T>
        typename traits::enable_if<traits::is_object<MemType>::value, int>::type minArgCount(
                MemType T::*) {
            return 1;
        }
        template<class MemType, class T>
        typename traits::enable_if<traits::is_object<MemType>::value, int>::type maxArgCount(
                MemType T::*) {
            return 2;
        }

        inline int call(lua_State *state, const PolymorphicInvoker &f) { return f.invoke(state); }
        inline int call(lua_State *state, PolymorphicInvoker &f) { return f.invoke(state); }
        inline bool checkArgTypes(lua_State *state, const PolymorphicInvoker &f) {
            return f.checkArgTypes(state);
        }
        inline bool strictCheckArgTypes(lua_State *state, const PolymorphicInvoker &f) {
            return f.strictCheckArgTypes(state);
        }
        inline std::string argTypesName(const PolymorphicInvoker &f) { return f.argTypesName(); }
        inline int minArgCount(const PolymorphicInvoker &f) { return f.minArgCount(); }
        inline int maxArgCount(const PolymorphicInvoker &f) { return f.maxArgCount(); }

        template<>
        struct is_callable<PolymorphicInvoker> : traits::integral_constant<bool, true>
        {
        };

        inline int call(lua_State *state, const PolymorphicMemberInvoker &f) {
            return f.invoke(state);
        }
        inline int call(lua_State *state, PolymorphicMemberInvoker &f) { return f.invoke(state); }
        inline bool checkArgTypes(lua_State *state, const PolymorphicMemberInvoker &f) {
            return f.checkArgTypes(state);
        }
        inline bool strictCheckArgTypes(lua_State *state, const PolymorphicMemberInvoker &f) {
            return f.strictCheckArgTypes(state);
        }
        inline std::string argTypesName(const PolymorphicMemberInvoker &f) {
            return f.argTypesName();
        }
        inline int minArgCount(const PolymorphicMemberInvoker &f) { return f.minArgCount(); }
        inline int maxArgCount(const PolymorphicMemberInvoker &f) { return f.maxArgCount(); }

        template<>
        struct is_callable<PolymorphicMemberInvoker> : traits::integral_constant<bool, true>
        {
        };
    }// namespace nativefunction

    class VariadicArgType {
    public:
        VariadicArgType(lua_State *state, int startIndex)
            : state_(state), startIndex_(startIndex), endIndex_(lua_gettop(state) + 1) {
            if (startIndex_ > endIndex_) { endIndex_ = startIndex_; }
        }

        template<typename T>
        operator std::vector<T>() const {
            if (startIndex_ >= endIndex_) { return std::vector<T>(); }
            std::vector<T> result;
            result.reserve(endIndex_ - startIndex_);
            for (int index = startIndex_; index < endIndex_; ++index) {
                result.push_back(lua_type_traits<T>::get(state_, index));
            }
            return result;
        }

        struct reference : public Ref::StackRef, public detail::LuaVariantImpl<reference>
        {
            reference(lua_State *s, int index) : Ref::StackRef(s, index, false) {}

            const reference *operator->() const { return this; }
        };

        struct iterator
        {
            typedef std::random_access_iterator_tag iterator_category;
            typedef VariadicArgType::reference reference;
            typedef int difference_type;
            typedef reference value_type;
            typedef reference *pointer;

            iterator() : state_(0), stack_index_(0) {}
            iterator(lua_State *state, int index) : state_(state), stack_index_(index) {}
            reference operator*() const { return reference(state_, stack_index_); }
            reference operator->() const { return reference(state_, stack_index_); }
            iterator &operator++() {
                stack_index_++;
                return *this;
            }
            iterator operator++(int) { return iterator(state_, stack_index_++); }

            iterator &operator+=(int n) {
                stack_index_ += n;
                return *this;
            }
            iterator operator+(int n) const { return iterator(state_, stack_index_ + n); }
            iterator &operator--() {
                stack_index_--;
                return *this;
            }
            iterator operator--(int) { return iterator(state_, stack_index_--); }
            iterator &operator-=(int n) {
                stack_index_ -= n;
                return *this;
            }
            iterator operator-(int n) const { return iterator(state_, stack_index_ - n); }
            difference_type operator-(const iterator &n) { return stack_index_ - n.stack_index_; }

            reference operator[](difference_type offset) const {
                return reference(state_, stack_index_ + offset);
            }
            /**
    * @name relational operators
    * @brief
    */
            //@{
            bool operator==(const iterator &other) const {
                return state_ == other.state_ && stack_index_ == other.stack_index_;
            }
            bool operator!=(const iterator &other) const { return !(*this == other); }
            bool operator<(const iterator &other) const {
                return stack_index_ < other.stack_index_;
            }
            bool operator>(const iterator &other) const { return other < *this; }
            bool operator<=(const iterator &other) const { return other >= *this; }
            bool operator>=(const iterator &other) const { return !(*this < other); }
            //@}
        private:
            lua_State *state_;
            int stack_index_;
        };
        typedef iterator const_iterator;
        typedef reference const_reference;
        typedef reference value_type;

        iterator begin() { return iterator(state_, startIndex_); }
        iterator end() { return iterator(state_, endIndex_); }
        const_iterator cbegin() { return const_iterator(state_, startIndex_); }
        const_iterator cend() { return const_iterator(state_, endIndex_); }

        template<typename T>
        typename lua_type_traits<T>::get_type at(size_t index) const {
            if (index >= size()) { throw std::out_of_range("variadic arguments out of range"); }
            return lua_type_traits<T>::get(state_, startIndex_ + static_cast<int>(index));
        }

        reference at(size_t index) const {
            if (index >= size()) { throw std::out_of_range("variadic arguments out of range"); }
            return reference(state_, startIndex_ + static_cast<int>(index));
        }

        reference operator[](size_t index) const {
            return reference(state_, startIndex_ + static_cast<int>(index));
        }
        size_t size() const { return endIndex_ - startIndex_; }
        size_t empty() const { return endIndex_ == startIndex_; }

    private:
        lua_State *state_;
        int startIndex_;
        int endIndex_;
    };

    inline VariadicArgType::iterator operator+(int n, const VariadicArgType::iterator &i) {
        return i + n;
    }

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for VariadicArgType
    template<>
    struct lua_type_traits<VariadicArgType>
    {
        typedef VariadicArgType get_type;

        static bool strictCheckType(lua_State *l, int index) {
            METAENGINE_LUAWRAPPER_UNUSED(l);
            METAENGINE_LUAWRAPPER_UNUSED(index);
            return true;
        }
        static bool checkType(lua_State *l, int index) {
            METAENGINE_LUAWRAPPER_UNUSED(l);
            METAENGINE_LUAWRAPPER_UNUSED(index);
            return true;
        }
        static get_type get(lua_State *l, int index) { return VariadicArgType(l, index); }
    };

    namespace nativefunction {
        static const int MAX_OVERLOAD_SCORE = 255;
        template<typename Fn>
        uint8_t compute_function_matching_score(lua_State *state, const Fn &fn) {
            int argcount = lua_gettop(state);

            if (strictCheckArgTypes(state, fn)) {
                const int minargcount = minArgCount(fn);
                const int maxargcount = maxArgCount(fn);
                if (minargcount <= argcount && maxargcount >= argcount) {
                    return MAX_OVERLOAD_SCORE;
                } else {
                    int diff = std::min(std::abs(argcount - minargcount),
                                        std::abs(argcount - maxargcount));
                    return std::max(100 - diff, 51);
                }
            } else if (checkArgTypes(state, fn)) {
                const int minargcount = minArgCount(fn);
                const int maxargcount = maxArgCount(fn);
                if (minargcount <= argcount && maxargcount >= argcount) {
                    return 200;
                } else {
                    int diff = std::min(std::abs(argcount - minargcount),
                                        std::abs(argcount - maxargcount));
                    return std::max(50 - diff, 1);
                }
            } else {
                return 0;
            }
        }
        inline int pushArgmentTypeNames(lua_State *state, int top) {
            for (int i = 1; i <= top; i++) {
                if (i != 1) { lua_pushliteral(state, ","); }

                int type = lua_type(state, i);
                if (type == LUA_TUSERDATA) {
                    int nametype = luaL_getmetafield(state, i, "__name");
                    if (nametype != LUA_TSTRING) {
                        lua_pop(state, 1);
                        lua_pushstring(state, lua_typename(state, type));
                    }
                } else {
                    lua_pushstring(state, lua_typename(state, type));
                }
            }
            return lua_gettop(state) - top;
        }
    }// namespace nativefunction

#if METAENGINE_LUAWRAPPER_USE_CPP11

    namespace detail {
        template<typename Fn, typename... Functions>
        void function_match_scoring(lua_State *state, uint8_t *score_array, int current_index,
                                    const Fn &fn) {
            score_array[current_index] = nativefunction::compute_function_matching_score(state, fn);
        }
        template<typename Fn, typename... Functions>
        void function_match_scoring(lua_State *state, uint8_t *score_array, int current_index,
                                    const Fn &fn, const Functions &...fns) {
            score_array[current_index] = nativefunction::compute_function_matching_score(state, fn);
            if (score_array[current_index] < nativefunction::MAX_OVERLOAD_SCORE) {
                function_match_scoring(state, score_array, current_index + 1, fns...);
            }
        }
        template<typename... Functions>
        int best_function_index(lua_State *state, const Functions &...fns) {
            static const int fncount = sizeof...(fns);
            uint8_t score[fncount] = {};
            function_match_scoring(state, score, 0, fns...);
            uint8_t best_score = 0;
            int best_score_index = -1;
            for (int i = 0; i < fncount; ++i) {
                if (best_score < score[i]) {
                    best_score = score[i];
                    best_score_index = i;
                    if (best_score == nativefunction::MAX_OVERLOAD_SCORE) { break; }
                }
            }
            return best_score_index;
        }
        template<typename Fn>
        int invoke_index(lua_State *state, int index, int current_index, Fn &&fn) {
            METAENGINE_LUAWRAPPER_UNUSED(index);
            METAENGINE_LUAWRAPPER_UNUSED(current_index);
            return nativefunction::call(state, fn);
        }
        template<typename Fn, typename... Functions>
        int invoke_index(lua_State *state, int index, int current_index, Fn &&fn,
                         Functions &&...fns) {
            if (index == current_index) {
                return nativefunction::call(state, fn);
            } else {
                return invoke_index(state, index, current_index + 1, fns...);
            }
        }

        template<typename Fun>
        int best_match_invoke(lua_State *state, Fun &&fn) {
            return nativefunction::call(state, fn);
        }

        template<typename Fun, typename... Functions>
        int best_match_invoke(lua_State *state, Fun &&fn, Functions &&...fns) {
            int index = best_function_index(state, fn, fns...);
            if (index >= 0) {
                assert(size_t(index) <= sizeof...(fns));
                return invoke_index(state, index, 0, fn, fns...);
            } else {
                throw LuaTypeMismatch();
            }
            return 0;
        }

        template<typename TupleType, std::size_t... S>
        int invoke_tuple_impl(lua_State *state, TupleType &&tuple,
                              nativefunction::index_tuple<S...>) {
            return best_match_invoke(state, fntuple::get<S>(tuple)...);
        }
        template<typename TupleType>
        int invoke_tuple(lua_State *state, TupleType &&tuple) {
            typedef typename std::decay<TupleType>::type ttype;

            typedef typename nativefunction::index_range<0, fntuple::tuple_size<ttype>::value>::type
                    indexrange;

            return invoke_tuple_impl(state, tuple, indexrange());
        }

        template<typename Fun>
        void push_arg_typename(lua_State *state, const Fun &fn) {
            lua_pushliteral(state, "\t\t");
            lua_pushstring(state, nativefunction::argTypesName(fn).c_str());
            lua_pushliteral(state, "\n");
        }

        template<typename Fun, typename... Functions>
        void push_arg_typename(lua_State *state, const Fun &fn, const Functions &...fns) {
            lua_pushliteral(state, "\t\t");
            lua_pushstring(state, nativefunction::argTypesName(fn).c_str());
            lua_pushliteral(state, "\n");
            push_arg_typename(state, fns...);
        }
        template<typename TupleType, std::size_t... S>
        void push_arg_typename_tuple_impl(lua_State *state, TupleType &&tuple,
                                          nativefunction::index_tuple<S...>) {
            return push_arg_typename(state, fntuple::get<S>(tuple)...);
        }
        template<typename TupleType>
        void push_arg_typename_tuple(lua_State *state, TupleType &&tuple) {
            typedef typename std::decay<TupleType>::type ttype;
            typedef typename nativefunction::index_range<0, fntuple::tuple_size<ttype>::value>::type
                    indexrange;

            return push_arg_typename_tuple_impl(state, tuple, indexrange());
        }
    }// namespace detail

#else

    namespace detail {
#define METAENGINE_LUAWRAPPER_FUNCTION_SCOREING(N)                                                 \
    if (currentbestscore < nativefunction::MAX_OVERLOAD_SCORE) {                                   \
        int score = nativefunction::compute_function_matching_score(state,                         \
                                                                    fntuple::get<N - 1>(tuple));   \
        if (currentbestscore < score) {                                                            \
            currentbestscore = score;                                                              \
            currentbestindex = N;                                                                  \
        }                                                                                          \
    }
#define METAENGINE_LUAWRAPPER_FUNCTION_INVOKE(N)                                                   \
    if (currentbestindex == N) { return nativefunction::call(state, fntuple::get<N - 1>(tuple)); }

#define METAENGINE_LUAWRAPPER_ARG_PUSH_TYPENAMES(N)                                                \
    lua_pushliteral(state, "\t\t");                                                                \
    lua_pushstring(state, nativefunction::argTypesName(fntuple::get<N - 1>(tuple)).c_str());       \
    lua_pushliteral(state, "\n");
#define METAENGINE_LUAWRAPPER_TEMPLATE_PARAMETER(N)                                                \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>
#define METAENGINE_LUAWRAPPER_TUPLE_INVOKE_DEF(N)                                                  \
    METAENGINE_LUAWRAPPER_TEMPLATE_PARAMETER(N)                                                    \
    int invoke_tuple(lua_State *state,                                                             \
                     fntuple::tuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)> &tuple) {     \
        if (N == 1) { return nativefunction::call(state, fntuple::get<0>(tuple)); }                \
        int32_t currentbestscore = 0;                                                              \
        int32_t currentbestindex = -1;                                                             \
        METAENGINE_LUAWRAPPER_PP_REPEAT(N, METAENGINE_LUAWRAPPER_FUNCTION_SCOREING);               \
        METAENGINE_LUAWRAPPER_PP_REPEAT(N, METAENGINE_LUAWRAPPER_FUNCTION_INVOKE);                 \
        throw LuaTypeMismatch();                                                                   \
    }                                                                                              \
    METAENGINE_LUAWRAPPER_TEMPLATE_PARAMETER(N)                                                    \
    void push_arg_typename_tuple(                                                                  \
            lua_State *state,                                                                      \
            fntuple::tuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)> &tuple) {              \
        METAENGINE_LUAWRAPPER_PP_REPEAT(N, METAENGINE_LUAWRAPPER_ARG_PUSH_TYPENAMES);              \
    }

        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_OVERLOADS,
                                            METAENGINE_LUAWRAPPER_TUPLE_INVOKE_DEF)
#undef METAENGINE_LUAWRAPPER_TEMPLATE_PARAMETER
#undef METAENGINE_LUAWRAPPER_TUPLE_INVOKE_DEF
#undef METAENGINE_LUAWRAPPER_ARG_TYPENAMES
#undef METAENGINE_LUAWRAPPER_FUNCTION_INVOKE
#undef METAENGINE_LUAWRAPPER_FUNCTION_SCOREING

        template<typename TupleType>
        int invoke_tuple(lua_State *state, TupleType &tuple) {
            METAENGINE_LUAWRAPPER_UNUSED(state);
            METAENGINE_LUAWRAPPER_UNUSED(tuple);
            return 0;
        }
    }// namespace detail
#endif

    template<typename FunctionTuple>
    struct FunctionInvokerType
    {
        FunctionTuple functions;
        FunctionInvokerType(const FunctionTuple &t) : functions(t) {}
    };

    template<typename T>
    inline FunctionInvokerType<fntuple::tuple<T>> function(T f) {
        METAENGINE_LUAWRAPPER_STATIC_ASSERT(
                nativefunction::is_callable<typename traits::decay<T>::type>::value,
                "argument need callable");
        return FunctionInvokerType<fntuple::tuple<T>>(fntuple::tuple<T>(f));
    }

    template<typename FTYPE, typename T>
    inline FunctionInvokerType<fntuple::tuple<standard::function<FTYPE>>> function(T f) {
        return FunctionInvokerType<fntuple::tuple<standard::function<FTYPE>>>(
                fntuple::tuple<standard::function<FTYPE>>(standard::function<FTYPE>(f)));
    }
#if METAENGINE_LUAWRAPPER_USE_CPP11

    template<typename... Functions>
    FunctionInvokerType<fntuple::tuple<Functions...>> overload(Functions... fns) {
        return FunctionInvokerType<fntuple::tuple<Functions...>>(
                fntuple::tuple<Functions...>(fns...));
    }
#else
#define METAENGINE_LUAWRAPPER_FOVERLOAD_DEF(N)                                                     \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    FunctionInvokerType<fntuple::tuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>> overload( \
            METAENGINE_LUAWRAPPER_PP_ARG_DEF_REPEAT(N)) {                                          \
        typedef typename fntuple::tuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)> ttype;    \
        return FunctionInvokerType<ttype>(ttype(METAENGINE_LUAWRAPPER_PP_ARG_REPEAT(N)));          \
    }
    METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_OVERLOADS,
                                        METAENGINE_LUAWRAPPER_FOVERLOAD_DEF)
#undef METAENGINE_LUAWRAPPER_FOVERLOAD_DEF
#endif

    struct luacfunction
    {
        lua_CFunction ptr;

        luacfunction(lua_CFunction f) : ptr(f) {}
        operator lua_CFunction() { return ptr; }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for FunctionInvokerType
    template<typename FunctionTuple>
    struct lua_type_traits<FunctionInvokerType<FunctionTuple>>
    {
        typedef FunctionInvokerType<FunctionTuple> userdatatype;
        typedef const FunctionInvokerType<FunctionTuple> &push_type;

        static const char *build_arg_error_message(lua_State *state, const char *msg,
                                                   FunctionTuple *tuple) {
            int stack_top = lua_gettop(state);
            if (msg) { lua_pushstring(state, msg); }
            lua_pushliteral(state, "Argument mismatch:");
            nativefunction::pushArgmentTypeNames(state, stack_top);

            lua_pushliteral(state, "\t candidate is:\n");
            detail::push_arg_typename_tuple(state, *tuple);

            lua_concat(state, lua_gettop(state) - stack_top);
            return lua_tostring(state, -1);
        }

        static int invoke(lua_State *state) {
            FunctionTuple *t =
                    static_cast<FunctionTuple *>(lua_touserdata(state, lua_upvalueindex(1)));

            if (t) {
                try {
                    return detail::invoke_tuple(state, *t);
                } catch (LuaTypeMismatch &e) {
                    if (strcmp(e.what(), "type mismatch!!") == 0) {
                        util::traceBack(state, build_arg_error_message(state, "maybe...", t));
                    } else {
                        util::traceBack(state, e.what());
                    }
                } catch (std::exception &e) { util::traceBack(state, e.what()); } catch (...) {
                    util::traceBack(state, "Unknown exception");
                }
            }
            return lua_error(state);
        }

        inline static int tuple_destructor(lua_State *state) {
            FunctionTuple *f = static_cast<FunctionTuple *>(lua_touserdata(state, 1));
            if (f) { f->~FunctionTuple(); }
            return 0;
        }

        static int push(lua_State *state, push_type fns) {
            void *ptr = lua_newuserdata(state, sizeof(FunctionTuple));
            new (ptr) FunctionTuple(fns.functions);
            lua_createtable(state, 0, 2);
            lua_pushcclosure(state, &tuple_destructor, 0);
            lua_setfield(state, -2, "__gc");
            lua_pushvalue(state, -1);
            lua_setfield(state, -1, "__index");
            lua_setmetatable(state, -2);
            lua_pushcclosure(state, &invoke, 1);

            return 1;
        }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for c function
    template<typename T>
    struct lua_type_traits<T, typename traits::enable_if<traits::is_function<
                                      typename traits::remove_pointer<T>::type>::value>::type>
    {
        static int push(lua_State *l, T f) { return util::one_push(l, LuaWrapper::function(f)); }
    };
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for luacfunction
    template<>
    struct lua_type_traits<luacfunction>
    {
        typedef luacfunction push_type;
        typedef luacfunction get_type;
        static bool strictCheckType(lua_State *l, int index) {
            return lua_iscfunction(l, index) != 0;
        }
        static bool checkType(lua_State *l, int index) { return lua_iscfunction(l, index) != 0; }
        static get_type get(lua_State *l, int index) { return lua_tocfunction(l, index); }
        static int push(lua_State *l, push_type f) {
            lua_pushcfunction(l, f);
            return 1;
        }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for std::function or boost::function
    template<typename T>
    struct lua_type_traits<standard::function<T>>
    {
        typedef const standard::function<T> &push_type;
        typedef standard::function<T> get_type;

        static bool strictCheckType(lua_State *l, int index) {
            return lua_type(l, index) == LUA_TFUNCTION;
        }
        static bool checkType(lua_State *l, int index) {
            return lua_type(l, index) == LUA_TFUNCTION;
        }
        static get_type get(lua_State *l, int index) {
            if (!l || lua_type(l, index) != LUA_TFUNCTION) { return get_type(); }
            lua_pushvalue(l, index);
            return get_type(LuaFunction(l, StackTop()));
        }

        static int push(lua_State *l, push_type v) {
            return util::one_push(l, LuaWrapper::function(v));
        }
    };

    template<typename F, typename Ret = typename util::FunctionSignature<F>::type::result_type>
    struct OverloadFunctionImpl : LuaWrapper::FunctionImpl
    {
        typedef Ret result_type;
        typedef typename util::FunctionSignature<F>::type::c_function_type c_function_type;

        virtual result_type invoke_type(lua_State *state) = 0;

        virtual int invoke(lua_State *state) { return util::push_args(state, invoke_type(state)); }
        virtual std::string argTypesName() const {
            return nativefunction::argTypesName(c_function_type(0), maxArgCount() - minArgCount());
        }
        virtual bool checkArgTypes(lua_State *state) const {
            return LuaWrapper::nativefunction::checkArgTypes(state, c_function_type(0),
                                                             maxArgCount() - minArgCount());
        }
        virtual bool strictCheckArgTypes(lua_State *state) const {
            return LuaWrapper::nativefunction::strictCheckArgTypes(state, c_function_type(0),
                                                                   maxArgCount() - minArgCount());
        }
    };

    template<typename F>
    struct OverloadFunctionImpl<F, void> : LuaWrapper::FunctionImpl
    {
        typedef void result_type;
        typedef typename util::FunctionSignature<F>::type::c_function_type c_function_type;

        virtual result_type invoke_type(lua_State *state) = 0;

        virtual int invoke(lua_State *state) {
            invoke_type(state);
            return 0;
        }
        virtual std::string argTypesName() const {
            return nativefunction::argTypesName(c_function_type(0), maxArgCount() - minArgCount());
        }
        virtual bool checkArgTypes(lua_State *state) const {
            return LuaWrapper::nativefunction::checkArgTypes(state, c_function_type(0),
                                                             maxArgCount() - minArgCount());
        }
        virtual bool strictCheckArgTypes(lua_State *state) const {
            return LuaWrapper::nativefunction::strictCheckArgTypes(state, c_function_type(0),
                                                                   maxArgCount() - minArgCount());
        }
    };
}// namespace LuaWrapper

#define METAENGINE_LUAWRAPPER_INTERNAL_OVERLOAD_FUNCTION_GET_REP(N) getArgument<N - 1, F>(state)
#define METAENGINE_LUAWRAPPER_INTERNAL_OVERLOAD_FUNCTION_GET_REPEAT(N)                             \
    METAENGINE_LUAWRAPPER_PP_REPEAT_ARG(N, METAENGINE_LUAWRAPPER_INTERNAL_OVERLOAD_FUNCTION_GET_REP)
#define METAENGINE_LUAWRAPPER_INTERNAL_OVERLOAD_FUNCTION_INVOKE(N, FNAME, MINARG, MAXARG)          \
    if (argcount == METAENGINE_LUAWRAPPER_PP_ADD(MINARG, METAENGINE_LUAWRAPPER_PP_DEC(N))) {       \
        return FNAME(METAENGINE_LUAWRAPPER_INTERNAL_OVERLOAD_FUNCTION_GET_REPEAT(                  \
                METAENGINE_LUAWRAPPER_PP_ADD(MINARG, METAENGINE_LUAWRAPPER_PP_DEC(N))));           \
    }

#define METAENGINE_LUAWRAPPER_FUNCTION_OVERLOADS_INTERNAL(GENERATE_NAME, FNAME, MINARG, MAXARG,    \
                                                          CREATE_FN)                               \
                                                                                                   \
    struct GENERATE_NAME                                                                           \
    {                                                                                              \
        template<typename F>                                                                       \
        struct Function : LuaWrapper::OverloadFunctionImpl<F>                                      \
        {                                                                                          \
            typedef typename LuaWrapper::OverloadFunctionImpl<F>::result_type result_type;         \
            virtual result_type invoke_type(lua_State *state) {                                    \
                using namespace LuaWrapper::nativefunction;                                        \
                int argcount = lua_gettop(state);                                                  \
                METAENGINE_LUAWRAPPER_PP_REPEAT_DEF_VA_ARG(                                        \
                        METAENGINE_LUAWRAPPER_PP_INC(                                              \
                                METAENGINE_LUAWRAPPER_PP_SUB(MAXARG, MINARG)),                     \
                        METAENGINE_LUAWRAPPER_INTERNAL_OVERLOAD_FUNCTION_INVOKE, FNAME, MINARG,    \
                        MAXARG)                                                                    \
                throw LuaWrapper::LuaTypeMismatch("argument count mismatch");                      \
            }                                                                                      \
            virtual int minArgCount() const { return MINARG; }                                     \
            virtual int maxArgCount() const { return MAXARG; }                                     \
        };                                                                                         \
        template<typename F>                                                                       \
        LuaWrapper::PolymorphicInvoker::holder_type create(F) {                                    \
            LuaWrapper::OverloadFunctionImpl<F> *ptr = new Function<F>();                          \
            return LuaWrapper::PolymorphicInvoker::holder_type(ptr);                               \
        }                                                                                          \
        template<typename F>                                                                       \
        LuaWrapper::PolymorphicInvoker::holder_type create() {                                     \
            LuaWrapper::OverloadFunctionImpl<F> *ptr = new Function<F>();                          \
            return LuaWrapper::PolymorphicInvoker::holder_type(ptr);                               \
        }                                                                                          \
        LuaWrapper::PolymorphicInvoker operator()() { return CREATE_FN; }                          \
                                                                                                   \
    } GENERATE_NAME;

#define METAENGINE_LUAWRAPPER_INTERNAL_OVERLOAD_MEMBER_FUNCTION_GET_REP(N) getArgument<N, F>(state)
#define METAENGINE_LUAWRAPPER_INTERNAL_OVERLOAD_MEMBER_FUNCTION_GET_REPEAT(N)                      \
    METAENGINE_LUAWRAPPER_PP_REPEAT_ARG(                                                           \
            N, METAENGINE_LUAWRAPPER_INTERNAL_OVERLOAD_MEMBER_FUNCTION_GET_REP)
#define METAENGINE_LUAWRAPPER_INTERNAL_OVERLOAD_MEMBER_FUNCTION_INVOKE(N, FNAME, MINARG, MAXARG)   \
    if (argcount == 1 + METAENGINE_LUAWRAPPER_PP_ADD(MINARG, METAENGINE_LUAWRAPPER_PP_DEC(N))) {   \
        return (getArgument<0, F>(state))                                                          \
                .FNAME(METAENGINE_LUAWRAPPER_INTERNAL_OVERLOAD_MEMBER_FUNCTION_GET_REPEAT(         \
                        METAENGINE_LUAWRAPPER_PP_ADD(MINARG, METAENGINE_LUAWRAPPER_PP_DEC(N))));   \
    }

#define METAENGINE_LUAWRAPPER_MEMBER_FUNCTION_OVERLOADS_INTERNAL(GENERATE_NAME, CLASS, FNAME,      \
                                                                 MINARG, MAXARG, CREATE_FN)        \
                                                                                                   \
    struct GENERATE_NAME                                                                           \
    {                                                                                              \
        template<typename F>                                                                       \
        struct Function : LuaWrapper::OverloadFunctionImpl<F>                                      \
        {                                                                                          \
            typedef typename LuaWrapper::OverloadFunctionImpl<F>::result_type result_type;         \
            virtual result_type invoke_type(lua_State *state) {                                    \
                using namespace LuaWrapper::nativefunction;                                        \
                int argcount = lua_gettop(state);                                                  \
                METAENGINE_LUAWRAPPER_PP_REPEAT_DEF_VA_ARG(                                        \
                        METAENGINE_LUAWRAPPER_PP_INC(                                              \
                                METAENGINE_LUAWRAPPER_PP_SUB(MAXARG, MINARG)),                     \
                        METAENGINE_LUAWRAPPER_INTERNAL_OVERLOAD_MEMBER_FUNCTION_INVOKE, FNAME,     \
                        MINARG, MAXARG)                                                            \
                throw LuaWrapper::LuaTypeMismatch("argument count mismatch");                      \
            }                                                                                      \
            virtual int minArgCount() const { return MINARG + 1; }                                 \
            virtual int maxArgCount() const { return MAXARG + 1; }                                 \
        };                                                                                         \
        template<typename F>                                                                       \
        LuaWrapper::PolymorphicMemberInvoker::holder_type create(F f) {                            \
            METAENGINE_LUAWRAPPER_UNUSED(f);                                                       \
            LuaWrapper::OverloadFunctionImpl<F> *ptr = new Function<F>();                          \
            return LuaWrapper::PolymorphicMemberInvoker::holder_type(ptr);                         \
        }                                                                                          \
        template<typename F>                                                                       \
        LuaWrapper::PolymorphicMemberInvoker::holder_type create() {                               \
            LuaWrapper::OverloadFunctionImpl<F> *ptr = new Function<F>();                          \
            return LuaWrapper::PolymorphicMemberInvoker::holder_type(ptr);                         \
        }                                                                                          \
        LuaWrapper::PolymorphicMemberInvoker operator()() { return CREATE_FN; }                    \
                                                                                                   \
    } GENERATE_NAME;

/// @brief Generate wrapper function object for count based overloads with
/// nonvoid return function. Include default arguments parameter function
/// @param GENERATE_NAME generate function object name
/// @param FNAME target function name
/// @param MINARG minimum arguments count
/// @param MAXARG maximum arguments count
#define METAENGINE_LUAWRAPPER_FUNCTION_OVERLOADS(GENERATE_NAME, FNAME, MINARG, MAXARG)             \
    METAENGINE_LUAWRAPPER_FUNCTION_OVERLOADS_INTERNAL(GENERATE_NAME, FNAME, MINARG, MAXARG,        \
                                                      create(FNAME))

/// @brief Generate wrapper function object for count based overloads with
/// nonvoid return function. Include default arguments parameter function
/// @param GENERATE_NAME generate function object name
/// @param FNAME target function name
/// @param MINARG minimum arguments count
/// @param MAXARG maximum arguments count
/// @param SIGNATURE function signature. e,g, int(int)
#define METAENGINE_LUAWRAPPER_FUNCTION_OVERLOADS_WITH_SIGNATURE(GENERATE_NAME, FNAME, MINARG,      \
                                                                MAXARG, SIGNATURE)                 \
    METAENGINE_LUAWRAPPER_FUNCTION_OVERLOADS_INTERNAL(GENERATE_NAME, FNAME, MINARG, MAXARG,        \
                                                      create<SIGNATURE>())

/// @brief Generate wrapper function object for count based overloads with
/// nonvoid return function. Include default arguments parameter function
/// @param GENERATE_NAME generate function object name
/// @param CLASS target class name
/// @param FNAME target function name
/// @param MINARG minimum arguments count
/// @param MAXARG maximum arguments count
#define METAENGINE_LUAWRAPPER_MEMBER_FUNCTION_OVERLOADS(GENERATE_NAME, CLASS, FNAME, MINARG,       \
                                                        MAXARG)                                    \
    METAENGINE_LUAWRAPPER_MEMBER_FUNCTION_OVERLOADS_INTERNAL(GENERATE_NAME, CLASS, FNAME, MINARG,  \
                                                             MAXARG, create(&CLASS::FNAME))

/// @brief Generate wrapper function object for count based overloads with
/// nonvoid return function. Include default arguments parameter function
/// @param GENERATE_NAME generate function object name
/// @param CLASS target class name
/// @param FNAME target function name
/// @param MINARG minimum arguments count
/// @param MAXARG maximum arguments count
/// @param SIGNATURE function signature. e,g, int(Test::*)(int)
#define METAENGINE_LUAWRAPPER_MEMBER_FUNCTION_OVERLOADS_WITH_SIGNATURE(                            \
        GENERATE_NAME, CLASS, FNAME, MINARG, MAXARG, SIGNATURE)                                    \
    METAENGINE_LUAWRAPPER_MEMBER_FUNCTION_OVERLOADS_INTERNAL(GENERATE_NAME, CLASS, FNAME, MINARG,  \
                                                             MAXARG, create<SIGNATURE>())

namespace LuaWrapper {
    /// @brief any data holder class for push to lua
    class AnyDataPusher {
    public:
        int pushToLua(lua_State *state) const {
            if (empty()) {
                lua_pushnil(state);
                return 1;
            } else {
                return holder_->pushToLua(state);
            }
        }

        AnyDataPusher() : holder_() {}

        template<typename DataType>
        AnyDataPusher(const DataType &v) : holder_(new DataHolder<DataType>(v)) {}

#if METAENGINE_LUAWRAPPER_USE_CPP11
        AnyDataPusher(AnyDataPusher &&other) : holder_(std::move(other.holder_)) {}
        AnyDataPusher &operator=(AnyDataPusher &&rhs) {
            holder_ = std::move(rhs.holder_);
            return *this;
        }
        template<typename DataType>
        AnyDataPusher(DataType &&v) : holder_(new DataHolder<DataType>(std::move(v))) {}
#endif
        AnyDataPusher(const AnyDataPusher &other) : holder_(other.holder_) {}
        AnyDataPusher &operator=(const AnyDataPusher &other) {
            holder_ = other.holder_;
            return *this;
        }

        bool empty() const { return !holder_.get(); }

    private:
        struct DataHolderBase
        {
            virtual int pushToLua(lua_State *data) const = 0;
            //			virtual DataHolderBase * clone(void) = 0;
            virtual ~DataHolderBase() {}
        };
        template<typename Type>
        class DataHolder : public DataHolderBase {
            typedef typename traits::decay<Type>::type DataType;

        public:
#if METAENGINE_LUAWRAPPER_USE_CPP11
            explicit DataHolder(DataType &&v) : data_(std::forward<DataType>(v)) {}
#else
            explicit DataHolder(const DataType &v) : data_(v) {}
#endif
            virtual int pushToLua(lua_State *state) const { return util::push_args(state, data_); }

        private:
            DataType data_;
        };
        // specialize for string literal
        template<int N>
        struct DataHolder<const char[N]> : DataHolder<std::string>
        {
            explicit DataHolder(const char *v)
                : DataHolder<std::string>(std::string(v, v[N - 1] != '\0' ? v + N : v + N - 1)) {}
        };
        template<int N>
        struct DataHolder<char[N]> : DataHolder<std::string>
        {
            explicit DataHolder(const char *v)
                : DataHolder<std::string>(std::string(v, v[N - 1] != '\0' ? v + N : v + N - 1)) {}
        };
        standard::shared_ptr<DataHolderBase> holder_;
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for AnyDataPusher
    template<>
    struct lua_type_traits<AnyDataPusher>
    {
        static int push(lua_State *l, const AnyDataPusher &data) { return data.pushToLua(l); }
    };
}// namespace LuaWrapper

namespace LuaWrapper {
    /// @brief function result value proxy class.
    /// don't direct use.
    class FunctionResults : public Ref::StackRef, public detail::LuaVariantImpl<FunctionResults> {
        FunctionResults(lua_State *state, int return_status, int startIndex)
            : Ref::StackRef(state, startIndex, true), state_(state), resultStatus_(return_status),
              resultCount_(lua_gettop(state) + 1 - startIndex) {}
        FunctionResults(lua_State *state, int return_status, int startIndex, int endIndex)
            : Ref::StackRef(state, startIndex, true), state_(state), resultStatus_(return_status),
              resultCount_(endIndex - startIndex) {}
        friend class detail::FunctionResultProxy;

    public:
        FunctionResults() : Ref::StackRef(), state_(0), resultStatus_(0), resultCount_(0) {}
        FunctionResults(lua_State *state)
            : Ref::StackRef(state, 0, true), state_(state), resultStatus_(0), resultCount_(0) {}
#if METAENGINE_LUAWRAPPER_USE_CPP11
        FunctionResults(FunctionResults &&src)
            : Ref::StackRef(std::move(src)), state_(src.state_), resultStatus_(src.resultStatus_),
              resultCount_(src.resultCount_) {
            src.state_ = 0;
        }
#else
        FunctionResults(const FunctionResults &src)
            : Ref::StackRef(src), state_(src.state_), resultStatus_(src.resultStatus_),
              resultCount_(src.resultCount_) {
            src.state_ = 0;
        }
#endif

        ~FunctionResults() {}

        struct reference : public Ref::StackRef, public detail::LuaVariantImpl<reference>
        {
            reference(lua_State *s, int index) : Ref::StackRef(s, index, false) {}

            reference *operator->() { return this; }
            const reference *operator->() const { return this; }
        };
        template<typename T>
        struct iterator_base
        {
            iterator_base(lua_State *s, int i) : state(s), stack_index(i) {}
            template<typename U>
            iterator_base(const iterator_base<U> &other)
                : state(other.state), stack_index(other.stack_index) {}
            T operator*() const { return reference(state, stack_index); }
            T operator->() const { return reference(state, stack_index); }
            const iterator_base &operator++() {
                stack_index++;
                return *this;
            }
            iterator_base operator++(int) { return iterator_base(state, stack_index++); }

            iterator_base operator+=(int n) {
                stack_index += n;
                return iterator_base(state, stack_index);
            }
            /**
    * @name relational operators
    * @brief
    */
            //@{
            bool operator==(const iterator_base &other) const {
                return state == other.state && stack_index == other.stack_index;
            }
            bool operator!=(const iterator_base &other) const { return !(*this == other); }
            //@}
            int index() const { return stack_index; }

        private:
            template<typename U>
            friend struct iterator_base;
            lua_State *state;
            int stack_index;
        };
        typedef iterator_base<reference> iterator;
        typedef iterator_base<const reference> const_iterator;
        typedef reference const_reference;

        iterator begin() { return iterator(state_, stack_index_); }
        iterator end() { return iterator(state_, stack_index_ + resultCount_); }
        const_iterator begin() const { return const_iterator(state_, stack_index_); }
        const_iterator end() const { return const_iterator(state_, stack_index_ + resultCount_); }
        const_iterator cbegin() const { return const_iterator(state_, stack_index_); }
        const_iterator cend() const { return const_iterator(state_, stack_index_ + resultCount_); }

        template<class Result>
        Result get_result(types::typetag<Result> = types::typetag<Result>()) const {
            return util::get_result<Result>(state_, stack_index_);
        }
        LuaStackRef get_result(types::typetag<LuaStackRef>) const {
            pop_ = 0;
            return LuaStackRef(state_, stack_index_, true);
        }
        lua_State *state() const { return state_; }

        template<typename T>
        typename lua_type_traits<T>::get_type result_at(size_t index) const {
            if (index >= result_size()) { throw std::out_of_range("function result out of range"); }
            return lua_type_traits<T>::get(state_, stack_index_ + static_cast<int>(index));
        }
        reference result_at(size_t index) const {
            if (index >= result_size()) { throw std::out_of_range("function result out of range"); }
            return reference(state_, stack_index_ + static_cast<int>(index));
        }

        size_t result_size() const { return resultCount_; }

        size_t resultStatus() const { return resultStatus_; }

        operator LuaStackRef() {
            pop_ = 0;
            return LuaStackRef(state_, stack_index_, true);
        }

    private:
        mutable lua_State *state_;
        int resultStatus_;
        int resultCount_;
    };

    namespace detail {
        template<typename RetType>
        inline RetType FunctionResultProxy::ReturnValue(lua_State *state, int return_status,
                                                        int retindex, types::typetag<RetType>) {
            return FunctionResults(state, return_status, retindex)
                    .get_result(types::typetag<RetType>());
        }
        inline FunctionResults FunctionResultProxy::ReturnValue(lua_State *state, int return_status,
                                                                int retindex,
                                                                types::typetag<FunctionResults>) {
            return FunctionResults(state, return_status, retindex);
        }
        inline void FunctionResultProxy::ReturnValue(lua_State *state, int return_status,
                                                     int retindex, types::typetag<void>) {
            METAENGINE_LUAWRAPPER_UNUSED(return_status);
            lua_settop(state, retindex - 1);
        }

#if METAENGINE_LUAWRAPPER_USE_CPP11
        template<typename Derived>
        template<class... Args>
        FunctionResults LuaFunctionImpl<Derived>::operator()(Args &&...args) {
            return this->template call<FunctionResults>(std::forward<Args>(args)...);
        }

        template<typename Derived>
        template<class... Args>
        FunctionResults LuaThreadImpl<Derived>::operator()(Args &&...args) {
            return this->template resume<FunctionResults>(std::forward<Args>(args)...);
        }
        template<typename Derived>
        template<class... Args>
        FunctionResults LuaVariantImpl<Derived>::operator()(Args &&...args) {
            int t = type();
            if (t == LUA_TTHREAD) {
                return this->template resume<FunctionResults>(std::forward<Args>(args)...);
            } else if (t == LUA_TFUNCTION) {
                return this->template call<FunctionResults>(std::forward<Args>(args)...);
            } else {
                except::typeMismatchError(state_(), " is not function or thread");
                return FunctionResults(state_());
            }
        }
#else
#define METAENGINE_LUAWRAPPER_TEMPLATE_PARAMETER(N) template<typename Derived>
#define METAENGINE_LUAWRAPPER_FUNCTION_ARGS_DEF(N)
#define METAENGINE_LUAWRAPPER_CALL_ARGS(N)

#define METAENGINE_LUAWRAPPER_OP_FN_DEF(N)                                                         \
    METAENGINE_LUAWRAPPER_TEMPLATE_PARAMETER(N)                                                    \
    inline FunctionResults LuaFunctionImpl<Derived>::operator()(                                   \
            METAENGINE_LUAWRAPPER_FUNCTION_ARGS_DEF(N)) {                                          \
        return this->template call<FunctionResults>(METAENGINE_LUAWRAPPER_CALL_ARGS(N));           \
    }                                                                                              \
    METAENGINE_LUAWRAPPER_TEMPLATE_PARAMETER(N)                                                    \
    inline FunctionResults LuaThreadImpl<Derived>::operator()(                                     \
            METAENGINE_LUAWRAPPER_FUNCTION_ARGS_DEF(N)) {                                          \
        return this->template resume<FunctionResults>(METAENGINE_LUAWRAPPER_CALL_ARGS(N));         \
    }                                                                                              \
    METAENGINE_LUAWRAPPER_TEMPLATE_PARAMETER(N)                                                    \
    inline FunctionResults LuaVariantImpl<Derived>::operator()(                                    \
            METAENGINE_LUAWRAPPER_FUNCTION_ARGS_DEF(N)) {                                          \
        int t = type();                                                                            \
        if (t == LUA_TTHREAD) {                                                                    \
            return this->template resume<FunctionResults>(METAENGINE_LUAWRAPPER_CALL_ARGS(N));     \
        } else if (t == LUA_TFUNCTION) {                                                           \
            return this->template call<FunctionResults>(METAENGINE_LUAWRAPPER_CALL_ARGS(N));       \
        } else {                                                                                   \
            except::typeMismatchError(state_(), " is not function or thread");                     \
            return FunctionResults(state_());                                                      \
        }                                                                                          \
    }

        METAENGINE_LUAWRAPPER_OP_FN_DEF(0)

#undef METAENGINE_LUAWRAPPER_TEMPLATE_PARAMETER
#undef METAENGINE_LUAWRAPPER_FUNCTION_ARGS_DEF
#undef METAENGINE_LUAWRAPPER_CALL_ARGS
#define METAENGINE_LUAWRAPPER_TEMPLATE_PARAMETER(N)                                                \
    template<typename Derived>                                                                     \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>
#define METAENGINE_LUAWRAPPER_FUNCTION_ARGS_DEF(N) METAENGINE_LUAWRAPPER_PP_ARG_CR_DEF_REPEAT(N)
#define METAENGINE_LUAWRAPPER_CALL_ARGS(N) METAENGINE_LUAWRAPPER_PP_ARG_REPEAT(N)

        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_OP_FN_DEF)
#undef METAENGINE_LUAWRAPPER_OP_FN_DEF
#undef METAENGINE_LUAWRAPPER_TEMPLATE_PARAMETER

#undef METAENGINE_LUAWRAPPER_CALL_ARGS
#undef METAENGINE_LUAWRAPPER_FUNCTION_ARGS_DEF
#undef METAENGINE_LUAWRAPPER_CALL_DEF
#undef METAENGINE_LUAWRAPPER_OP_FN_DEF
#endif
    }// namespace detail

    inline std::ostream &operator<<(std::ostream &os, const FunctionResults &res) {
        for (FunctionResults::const_iterator it = res.begin(); it != res.end(); ++it) {
            if (it != res.begin()) { os << ","; }
            util::stackValueDump(os, res.state(), it.index());
        }

        return os;
    }

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for FunctionResults
    template<>
    struct lua_type_traits<FunctionResults>
    {
        static int push(lua_State *l, const FunctionResults &ref) {
            int size = 0;
            for (FunctionResults::const_iterator it = ref.cbegin(); it != ref.cend(); ++it) {
                size += it->push(l);
            }
            return size;
        }
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for FunctionResults::reference
    template<>
    struct lua_type_traits<FunctionResults::reference>
    {
        static int push(lua_State *l, const FunctionResults::reference &ref) { return ref.push(l); }
    };
    template<unsigned int I>
    FunctionResults::reference get(const FunctionResults &res) {
        return res.result_at(I);
    }

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaFunction
    template<>
    struct lua_type_traits<LuaFunction>
    {
        typedef LuaFunction get_type;
        typedef LuaFunction push_type;

        static bool strictCheckType(lua_State *l, int index) { return lua_isfunction(l, index); }
        static bool checkType(lua_State *l, int index) {
            return lua_isfunction(l, index) || lua_isnil(l, index);
        }
        static LuaFunction get(lua_State *l, int index) {
            lua_pushvalue(l, index);
            return LuaFunction(l, StackTop());
        }
        static int push(lua_State *l, const LuaFunction &ref) { return ref.push(l); }
    };
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaFunction
    template<>
    struct lua_type_traits<const LuaFunction &> : lua_type_traits<LuaFunction>
    {
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaThread
    template<>
    struct lua_type_traits<LuaThread>
    {
        typedef LuaThread get_type;
        typedef LuaThread push_type;

        static bool strictCheckType(lua_State *l, int index) { return lua_isthread(l, index); }
        static bool checkType(lua_State *l, int index) {
            return lua_isthread(l, index) || lua_isnil(l, index);
        }
        static LuaThread get(lua_State *l, int index) {
            lua_pushvalue(l, index);
            return LuaThread(l, StackTop());
        }
        static int push(lua_State *l, const LuaThread &ref) { return ref.push(l); }
    };
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaThread
    template<>
    struct lua_type_traits<const LuaThread &> : lua_type_traits<LuaThread>
    {
    };

    /**
* @brief table and function binder.
* state["table"]->*"fun"() is table:fun() in Lua
* @param arg... function args
*/
    class MemberFunctionBinder {
    public:
        template<class T>
        MemberFunctionBinder(LuaRef self, T key) : self_(self), fun_(self_.getField(key)) {}

#define METAENGINE_LUAWRAPPER_DELEGATE_LUAREF fun_
#define METAENGINE_LUAWRAPPER_DELEGATE_FIRST_ARG self_
#define METAENGINE_LUAWRAPPER_DELEGATE_FIRST_ARG_C METAENGINE_LUAWRAPPER_DELEGATE_FIRST_ARG,

#if METAENGINE_LUAWRAPPER_USE_CPP11
        template<class... Args>
        FunctionResults operator()(Args &&...args) {
            return METAENGINE_LUAWRAPPER_DELEGATE_LUAREF(
                    METAENGINE_LUAWRAPPER_DELEGATE_FIRST_ARG_C std::forward<Args>(args)...);
        }

        template<class Result, class... Args>
        Result call(Args &&...args) {
            return METAENGINE_LUAWRAPPER_DELEGATE_LUAREF.call<Result>(
                    METAENGINE_LUAWRAPPER_DELEGATE_FIRST_ARG_C std::forward<Args>(args)...);
        }
#else

#define METAENGINE_LUAWRAPPER_OP_FN_DEF(N)                                                         \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    FunctionResults operator()(METAENGINE_LUAWRAPPER_PP_ARG_CR_DEF_REPEAT(N)) {                    \
        return METAENGINE_LUAWRAPPER_DELEGATE_LUAREF(                                              \
                METAENGINE_LUAWRAPPER_DELEGATE_FIRST_ARG_C METAENGINE_LUAWRAPPER_PP_ARG_REPEAT(    \
                        N));                                                                       \
    }

        /**
  * @brief If type is function, call lua function.
  * If type is lua thread,start or resume lua thread.
  * Otherwise send error message to error handler
  */
        FunctionResults operator()() {
            return METAENGINE_LUAWRAPPER_DELEGATE_LUAREF(METAENGINE_LUAWRAPPER_DELEGATE_FIRST_ARG);
        }
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_OP_FN_DEF)

#undef METAENGINE_LUAWRAPPER_OP_FN_DEF

#define METAENGINE_LUAWRAPPER_CALL_DEF(N)                                                          \
    template<class Result, METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                        \
    Result call(METAENGINE_LUAWRAPPER_PP_ARG_CR_DEF_REPEAT(N)) {                                   \
        return METAENGINE_LUAWRAPPER_DELEGATE_LUAREF.call<Result>(                                 \
                METAENGINE_LUAWRAPPER_DELEGATE_FIRST_ARG_C METAENGINE_LUAWRAPPER_PP_ARG_REPEAT(    \
                        N));                                                                       \
    }

        template<class Result>
        Result call() {
            return METAENGINE_LUAWRAPPER_DELEGATE_LUAREF.call<Result>(
                    METAENGINE_LUAWRAPPER_DELEGATE_FIRST_ARG);
        }
        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_CALL_DEF)

#undef METAENGINE_LUAWRAPPER_CALL_DEF
#endif

#undef METAENGINE_LUAWRAPPER_DELEGATE_FIRST_ARG_C
#undef METAENGINE_LUAWRAPPER_DELEGATE_FIRST_ARG
#undef METAENGINE_LUAWRAPPER_DELEGATE_LUAREF

    private:
        LuaRef self_;// Table or Userdata
        LuaFunction fun_;
    };

    typedef MemberFunctionBinder mem_fun_binder;// for backward conpatible
}// namespace LuaWrapper

#define METAENGINE_LUAWRAPPER_PROPERTY_PREFIX "_prop_"

namespace LuaWrapper {

#define METAENGINE_LUAWRAPPER_PP_STRUCT_TDEF_REP(N) METAENGINE_LUAWRAPPER_PP_CAT(class A, N) = void
#define METAENGINE_LUAWRAPPER_PP_STRUCT_TEMPLATE_DEF_REPEAT(N)                                     \
    METAENGINE_LUAWRAPPER_PP_REPEAT_ARG(N, METAENGINE_LUAWRAPPER_PP_STRUCT_TDEF_REP)

    template<METAENGINE_LUAWRAPPER_PP_STRUCT_TEMPLATE_DEF_REPEAT(
            METAENGINE_LUAWRAPPER_CLASS_MAX_BASE_CLASSES)>
    struct MultipleBase
    {
    };
#undef METAENGINE_LUAWRAPPER_PP_STRUCT_TDEF_REP
#undef METAENGINE_LUAWRAPPER_PP_STRUCT_TEMPLATE_DEF_REPEAT
}// namespace LuaWrapper

namespace LuaWrapper {
    struct LuaCodeChunk
    {
        LuaCodeChunk(const std::string &src, const std::string &name = "")
            : code_(src), chunk_name_(name) {}
        LuaCodeChunk(const char *src, const char *name = "")
            : code_(src), chunk_name_(name ? name : "") {}
        std::string code_;
        std::string chunk_name_;
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaCodeChunk
    template<>
    struct lua_type_traits<LuaCodeChunk>
    {
        static int push(lua_State *state, const LuaCodeChunk &ref) {
            int status = luaL_loadbuffer(state, ref.code_.c_str(), ref.code_.size(),
                                         ref.chunk_name_.empty() ? ref.code_.c_str()
                                                                 : ref.chunk_name_.c_str());
            if (!except::checkErrorAndThrow(status, state)) { return 0; }
            return 1;
        }
    };
    struct LuaCodeChunkExecute : LuaCodeChunk
    {
        LuaCodeChunkExecute(const std::string &src, const std::string &name = "")
            : LuaCodeChunk(src, name) {}
        LuaCodeChunkExecute(const char *src, const char *name = "") : LuaCodeChunk(src, name) {}
    };
    typedef LuaCodeChunkExecute LuaCodeChunkResult;
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for LuaCodeChunkResult
    template<>
    struct lua_type_traits<LuaCodeChunkExecute>
    {
        static int push(lua_State *state, const LuaCodeChunkExecute &ref) {
            int status = luaL_loadbuffer(state, ref.code_.c_str(), ref.code_.size(),
                                         ref.chunk_name_.empty() ? ref.code_.c_str()
                                                                 : ref.chunk_name_.c_str());
            if (!except::checkErrorAndThrow(status, state)) { return 0; }
            status = lua_pcall_wrap(state, 0, 1);
            if (!except::checkErrorAndThrow(status, state)) { return 0; }
            return 1;
        }
    };

    namespace Metatable {
        typedef std::map<std::string, AnyDataPusher> PropMapType;
        typedef std::map<std::string, AnyDataPusher> MemberMapType;

        inline bool is_property_key(const char *keyname) {
            return keyname && strncmp(keyname, METAENGINE_LUAWRAPPER_PROPERTY_PREFIX,
                                      sizeof(METAENGINE_LUAWRAPPER_PROPERTY_PREFIX) - 1) != 0;
        }
        inline int property_index_function(lua_State *L) {
            // Lua
            // local arg = {...};local metatable = arg[1];
            // return function(table, index)
            // if string.find(index,METAENGINE_LUAWRAPPER_PROPERTY_PREFIX)~=0 then
            // local propfun = metatable[METAENGINE_LUAWRAPPER_PROPERTY_PREFIX ..index];
            // if propfun then return propfun(table) end
            // end
            // return metatable[index]
            // end
            static const int table = 1;
            static const int key = 2;
            static const int metatable = lua_upvalueindex(1);
            const char *strkey = lua_tostring(L, key);

            if (lua_type(L, 1) == LUA_TUSERDATA && is_property_key(strkey)) {
                int type = lua_getfield_rtype(
                        L, metatable,
                        (METAENGINE_LUAWRAPPER_PROPERTY_PREFIX + std::string(strkey)).c_str());
                if (type == LUA_TFUNCTION) {
                    lua_pushvalue(L, table);
                    lua_call(L, 1, 1);
                    return 1;
                }
            }
            lua_pushvalue(L, key);
            lua_gettable(L, metatable);
            return 1;
        }
        inline int property_newindex_function(lua_State *L) {
            // Lua
            // local arg = {...};local metatable = arg[1];
            // return function(table, index, value)
            // if type(table) == 'userdata' then
            // if string.find(index,METAENGINE_LUAWRAPPER_PROPERTY_PREFIX)~=0 then
            // local propfun = metatable[METAENGINE_LUAWRAPPER_PROPERTY_PREFIX..index];
            // if propfun then return propfun(table,value) end
            // end
            // end
            // rawset(table,index,value)
            // end
            static const int table = 1;
            static const int key = 2;
            static const int value = 3;
            static const int metatable = lua_upvalueindex(1);
            const char *strkey = lua_tostring(L, 2);

            if (lua_type(L, 1) == LUA_TUSERDATA && is_property_key(strkey)) {
                int type = lua_getfield_rtype(
                        L, metatable,
                        (METAENGINE_LUAWRAPPER_PROPERTY_PREFIX + std::string(strkey)).c_str());
                if (type == LUA_TFUNCTION) {
                    lua_pushvalue(L, table);
                    lua_pushvalue(L, value);
                    lua_call(L, 2, 0);
                    return 0;
                } else if (type == LUA_TNIL) {
                    return luaL_error(L, "setting unknown property (%s) to userdata.", strkey);
                }
            }
            lua_pushvalue(L, key);
            lua_pushvalue(L, value);
            lua_rawset(L, table);
            return 0;
        }

        inline int multiple_base_index_function(lua_State *L) {
            // Lua
            // local arg = {...};local metabases = arg[1];
            // return function(t, k)
            // for i = 1,#metabases do
            // local v = metabases[i][k]
            // if v then
            // t[k] = v
            // return v end
            // end
            // end
            static const int table = 1;
            static const int key = 2;
            static const int metabases = lua_upvalueindex(1);

            lua_pushnil(L);
            while (lua_next(L, metabases) != 0) {
                if (lua_type(L, -1) == LUA_TTABLE) {
                    lua_pushvalue(L, key);
                    int type = lua_gettable_rtype(L, -2);
                    if (type != LUA_TNIL) {
                        lua_pushvalue(L, key);
                        lua_pushvalue(L, -2);
                        lua_settable(L, table);
                        return 1;
                    }
                }
                lua_settop(L, 3);// pop value
            }
            return 0;
        }

        inline int call_constructor_function(lua_State *L) {
            // function(t,...) return t.new(...) end
            lua_getfield(L, 1, "new");
            lua_replace(L, 1);
            lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
            return lua_gettop(L);
        }
        inline void get_call_constructor_metatable(lua_State *L) {
#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
            static const char *key = "METAENGINE_LUAWRAPPER_CALL_CONSTRUCTOR_METATABLE_KEY";
            lua_pushstring(L, key);
#else
            static int key = 0;
            lua_pushlightuserdata(L, &key);
#endif
            int ttype = lua_rawget_rtype(L, LUA_REGISTRYINDEX);
            if (ttype != LUA_TTABLE) {
                lua_pop(L, 1);
                lua_createtable(L, 0, 1);
                lua_pushstring(L, "__call");
                lua_pushcfunction(L, &call_constructor_function);
                lua_rawset(L, -3);
#if METAENGINE_LUAWRAPPER_SUPPORT_MULTIPLE_SHARED_LIBRARY
                lua_pushstring(L, key);
#else
                lua_pushlightuserdata(L, &key);
#endif
                lua_pushvalue(L, -2);
                lua_rawset(L, LUA_REGISTRYINDEX);
            }
        }

        inline void setMembers(lua_State *state, int metatable_index,
                               const MemberMapType &member_map, const PropMapType &property_map) {
            for (MemberMapType::const_iterator it = member_map.begin(); it != member_map.end();
                 ++it) {
                util::one_push(state, it->first);
                util::one_push(state, it->second);
                lua_rawset(state, metatable_index);
            }
            for (PropMapType::const_iterator it = property_map.begin(); it != property_map.end();
                 ++it) {
                util::one_push(state, METAENGINE_LUAWRAPPER_PROPERTY_PREFIX + it->first);
                util::one_push(state, it->second);
                lua_rawset(state, metatable_index);
            }
        }

        inline void setPropertyIndexMetamethod(lua_State *state, int metatable_index) {
            lua_pushstring(state, "__index");
            lua_pushvalue(state, metatable_index);
            lua_pushcclosure(state, &property_index_function, 1);
            lua_rawset(state, metatable_index);
        }

        inline void setPropertyNewIndexMetamethod(lua_State *state, int metatable_index) {
            lua_pushstring(state, "__newindex");
            lua_pushvalue(state, metatable_index);
            lua_pushcclosure(state, &property_newindex_function, 1);
            lua_rawset(state, metatable_index);
        }
        inline void setMultipleBase(lua_State *state, int metatable_index,
                                    int metabase_array_index) {
            lua_createtable(state, 0, 1);
            int newmetaindex = lua_gettop(state);
            lua_pushstring(state, "__index");
            lua_pushvalue(state, metabase_array_index);// bind metabase_array to
                                                       // multiple_base_index_function
            lua_pushcclosure(state, &multiple_base_index_function, 1);
            lua_rawset(state,
                       newmetaindex);// newmeta["__index"] = multiple_base_index_function
            lua_setmetatable(state, metatable_index);// metatable.setMetatable(newmeta);
        }
    }// namespace Metatable

    /// class binding interface.
    template<typename class_type, typename base_class_type = void>
    class UserdataMetatable {
    public:
        UserdataMetatable() {
            addStaticFunction("__gc", &class_userdata::destructor<ObjectWrapperBase>);

            METAENGINE_LUAWRAPPER_STATIC_ASSERT(
                    is_registerable<class_type>::value || !traits::is_std_vector<class_type>::value,
                    "std::vector is binding to lua-table by default.If "
                    "you wants register for std::vector yourself,"
                    "please define METAENGINE_LUAWRAPPER_NO_STD_VECTOR_TO_TABLE");

            METAENGINE_LUAWRAPPER_STATIC_ASSERT(
                    is_registerable<class_type>::value || !traits::is_std_map<class_type>::value,
                    "std::map is binding to lua-table by default.If you "
                    "wants register for std::map yourself,"
                    "please define METAENGINE_LUAWRAPPER_NO_STD_MAP_TO_TABLE");

            // can not register push specialized class
            METAENGINE_LUAWRAPPER_STATIC_ASSERT(is_registerable<class_type>::value,
                                                "Can not register specialized of type conversion "
                                                "class. e.g. std::tuple");
        }

        bool pushCreateMetatable(lua_State *state) const {
            if (!class_userdata::newmetatable<class_type>(state)) {
                except::OtherError(state, typeid(class_type *).name() +
                                                  std::string(" is already registered"));
                return false;
            }
            int metatable_index = lua_gettop(state);
            Metatable::setMembers(state, metatable_index, member_map_, property_map_);

            if (!traits::is_same<base_class_type, void>::value ||
                !property_map_.empty())// if base class has property and derived class
                                       // doesn't, need property access
                                       // metamethod
            {
                if (member_map_.count("__index") == 0) {
                    Metatable::setPropertyIndexMetamethod(state, metatable_index);
                }

                if (member_map_.count("__newindex") == 0) {
                    Metatable::setPropertyNewIndexMetamethod(state, metatable_index);
                }
            } else {
                if (member_map_.count("__index") == 0) {
                    lua_pushstring(state, "__index");
                    lua_pushvalue(state, metatable_index);
                    lua_rawset(state, metatable_index);
                }
            }

            set_base_metatable(state, metatable_index, types::typetag<base_class_type>());

            if (lua_getmetatable(state, metatable_index))// get base_metatable
            {
                lua_pushstring(state, "__call");
                lua_pushcfunction(state, &Metatable::call_constructor_function);
                lua_rawset(state, -3);// base_metatable["__call"] =
                                      // Metatable::call_constructor_function
            } else {
                Metatable::get_call_constructor_metatable(state);
                lua_setmetatable(state, metatable_index);
            }
            lua_settop(state, metatable_index);
            return true;
        }
        LuaTable createMatatable(lua_State *state) const {
            util::ScopedSavedStack save(state);
            if (!pushCreateMetatable(state)) { return LuaTable(); }
            return LuaStackRef(state, -1);
        }

#if METAENGINE_LUAWRAPPER_USE_CPP11
        template<typename... ArgTypes>
        UserdataMetatable &setConstructors() {
            addOverloadedFunctions("new",
                                   typename ConstructorFunction<class_type, ArgTypes>::type()...);
            return *this;
        }
#else
#define METAENGINE_LUAWRAPPER_SET_CON_TYPE_DEF(N)                                                  \
    typename ConstructorFunction<class_type, METAENGINE_LUAWRAPPER_PP_CAT(A, N)>::type()
#define METAENGINE_LUAWRAPPER_SET_CON_FN_DEF(N)                                                    \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    inline UserdataMetatable &setConstructors() {                                                  \
        addOverloadedFunctions("new", METAENGINE_LUAWRAPPER_PP_REPEAT_ARG(                         \
                                              N, METAENGINE_LUAWRAPPER_SET_CON_TYPE_DEF));         \
        return *this;                                                                              \
    }

        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_OVERLOADS,
                                            METAENGINE_LUAWRAPPER_SET_CON_FN_DEF)
#undef METAENGINE_LUAWRAPPER_SET_CON_FN_DEF
#undef METAENGINE_LUAWRAPPER_SET_CON_TYPE_DEF

#endif

        /// @brief add member property with getter function.(experimental)
        /// @param name function name for lua
        /// @param mem bind member data
        template<typename Ret>
        UserdataMetatable &addProperty(const char *name, Ret class_type::*mem) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered.");
                return *this;
            }
            property_map_[name] = AnyDataPusher(LuaWrapper::function(mem));
            return *this;
        }

        /// @brief add member property with getter function.(experimental)
        /// @param name function name for lua
        /// @param getter getter function
        template<typename GetType>
        UserdataMetatable &addProperty(const char *name, GetType (class_type::*getter)() const) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered.");
                return *this;
            }
            property_map_[name] = AnyDataPusher(LuaWrapper::function(getter));
            return *this;
        }

        /// @brief add member property with setter, getter functions.(experimental)
        /// @param name function name for lua
        /// @param getter getter function
        /// @param setter setter function
        template<typename GetType>
        UserdataMetatable &addProperty(const char *name, GetType (*getter)(const class_type *)) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered.");
                return *this;
            }
            property_map_[name] = AnyDataPusher(function(getter));
            return *this;
        }
        /// @brief add member property with setter, getter functions.(experimental)
        /// @param name function name for lua
        /// @param getter getter function
        /// @param setter setter function
        template<typename GetType>
        UserdataMetatable &addProperty(const char *name, GetType (*getter)(const class_type &)) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered.");
                return *this;
            }
            property_map_[name] = AnyDataPusher(function(getter));
            return *this;
        }

        /// @brief add member property with setter, getter functions.(experimental)
        /// @param name function name for lua
        /// @param getter getter function
        /// @param setter setter function
        template<typename GetType, typename SetType>
        UserdataMetatable &addProperty(const char *name, GetType (class_type::*getter)() const,
                                       void (class_type::*setter)(SetType)) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered.");
                return *this;
            }
            property_map_[name] = AnyDataPusher(overload(getter, setter));
            return *this;
        }

        /// @brief add member property with external setter, getter
        /// functions.(experimental)
        /// @param name function name for lua
        /// @param getter getter function
        /// @param setter setter function
        template<typename GetType, typename SetType>
        UserdataMetatable &addProperty(const char *name, GetType (*getter)(const class_type *),
                                       void (*setter)(class_type *, SetType)) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered.");
                return *this;
            }
            property_map_[name] = AnyDataPusher(overload(getter, setter));
            return *this;
        }

        /// @brief add member property with external setter, getter
        /// functions.(experimental)
        /// @param name function name for lua
        /// @param getter getter function
        /// @param setter setter function
        template<typename GetType, typename SetType>
        UserdataMetatable &addProperty(const char *name, GetType (*getter)(const class_type &),
                                       void (*setter)(class_type &, SetType)) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered.");
                return *this;
            }
            property_map_[name] = AnyDataPusher(overload(getter, setter));
            return *this;
        }

        /// @brief add member property with getter function.(experimental)
        /// @param name function name for lua
        /// @param getter getter function
        template<typename GetterType>
        UserdataMetatable &addPropertyAny(const char *name, GetterType getter) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered.");
                return *this;
            }
            property_map_[name] = AnyDataPusher(function(getter));
            return *this;
        }
        /// @brief add member property with setter, getter functions.(experimental)
        /// @param name function name for lua
        /// @param getter getter function
        /// @param setter setter function
        template<typename GetterType, typename SetterType>
        UserdataMetatable &addPropertyAny(const char *name, GetterType getter, SetterType setter) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered.");
                return *this;
            }
            property_map_[name] = AnyDataPusher(overload(getter, setter));
            return *this;
        }

        /// @brief add non member function
        /// @param name function name for lua
        /// @param f function
        template<typename Fun>
        UserdataMetatable &addStaticFunction(const char *name, Fun f) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered.");
                return *this;
            }
            member_map_[name] = AnyDataPusher(LuaWrapper::function(f));
            return *this;
        }

#if METAENGINE_LUAWRAPPER_USE_CPP11
        /// @brief assign overloaded from functions.
        /// @param name name for lua
        /// @param f functions
        template<typename... Funcs>
        UserdataMetatable &addOverloadedFunctions(const char *name, Funcs... f) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered.");
                return *this;
            }

            member_map_[name] = AnyDataPusher(overload(f...));

            return *this;
        }

        /// @brief assign data by argument value.
        /// @param name name for lua
        /// @param d data
        template<typename Data>
        UserdataMetatable &addStaticField(const char *name, Data &&d) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered.");
                return *this;
            }
            member_map_[name] = AnyDataPusher(std::forward<Data>(d));
            return *this;
        }
#else

#define METAENGINE_LUAWRAPPER_ADD_OVERLOAD_FUNCTION_DEF(N)                                         \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    inline UserdataMetatable &addOverloadedFunctions(                                              \
            const char *name, METAENGINE_LUAWRAPPER_PP_ARG_CR_DEF_REPEAT(N)) {                     \
        if (has_key(name)) {                                                                       \
            throw KaguyaException(std::string(name) + " is already registered.");                  \
            return *this;                                                                          \
        }                                                                                          \
        member_map_[name] =                                                                        \
                AnyDataPusher(LuaWrapper::overload(METAENGINE_LUAWRAPPER_PP_ARG_REPEAT(N)));       \
        return *this;                                                                              \
    }

        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_OVERLOADS,
                                            METAENGINE_LUAWRAPPER_ADD_OVERLOAD_FUNCTION_DEF)
#undef METAENGINE_LUAWRAPPER_ADD_OVERLOAD_FUNCTION_DEF

        /// @brief assign data by argument value.
        /// @param name name for lua
        /// @param d data
        template<typename Data>
        UserdataMetatable &addStaticField(const char *name, const Data &d) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered.");
                return *this;
            }
            member_map_[name] = AnyDataPusher(d);
            return *this;
        }
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1800
        // can not write  Ret class_type::* f on MSC++2013
        template<typename Fun>
        UserdataMetatable &addFunction(const char *name, Fun f) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered. To "
                                                          "overload a function, use "
                                                          "addOverloadedFunctions");
                return *this;
            }
            member_map_[name] = AnyDataPusher(LuaWrapper::function(f));
            return *this;
        }
#else
        /// @brief assign function
        /// @param name name for lua
        /// @param f pointer to member function.
        template<typename Ret>
        UserdataMetatable &addFunction(const char *name, Ret class_type::*f) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered. To "
                                                          "overload a function, use "
                                                          "addOverloadedFunctions");
                return *this;
            }
            member_map_[name] = AnyDataPusher(LuaWrapper::function(f));
            return *this;
        }
#endif
        /// @brief assign function
        /// @param name name for lua
        /// @param f member function object.
        UserdataMetatable &addFunction(const char *name, PolymorphicMemberInvoker f) {
            if (has_key(name)) {
                throw KaguyaException(std::string(name) + " is already registered. To "
                                                          "overload a function, use "
                                                          "addOverloadedFunctions");
                return *this;
            }
            member_map_[name] = AnyDataPusher(LuaWrapper::function(f));
            return *this;
        }

    private:
        void set_base_metatable(lua_State *, int, types::typetag<void>) const {}
        template<class Base>
        void set_base_metatable(lua_State *state, int metatable_index, types::typetag<Base>) const {
            class_userdata::get_metatable<Base>(state);
            lua_setmetatable(state,
                             metatable_index);// metatable.setMetatable(newmeta);
            PointerConverter &pconverter = PointerConverter::get(state);
            pconverter.add_type_conversion<Base, class_type>();
        }

#if METAENGINE_LUAWRAPPER_USE_CPP11

        template<typename Base>
        void metatables(lua_State *state, int metabase_array_index, PointerConverter &pvtreg,
                        types::typetag<MultipleBase<Base>>) const {
            class_userdata::get_metatable<Base>(state);
            lua_rawseti(state, metabase_array_index, lua_rawlen(state, metabase_array_index) + 1);
            pvtreg.add_type_conversion<Base, class_type>();
        }
        template<typename Base, typename... Remain>
        void metatables(lua_State *state, int metabase_array_index, PointerConverter &pvtreg,
                        types::typetag<MultipleBase<Base, Remain...>>) const {
            class_userdata::get_metatable<Base>(state);
            lua_rawseti(state, metabase_array_index, lua_rawlen(state, metabase_array_index) + 1);
            pvtreg.add_type_conversion<Base, class_type>();
            metatables(state, metabase_array_index, pvtreg,
                       types::typetag<MultipleBase<Remain...>>());
        }

        template<typename... Bases>
        void set_base_metatable(lua_State *state, int metatable_index,
                                types::typetag<MultipleBase<Bases...>> metatypes) const {
            PointerConverter &pconverter = PointerConverter::get(state);

            lua_createtable(state, sizeof...(Bases), 0);
            int metabase_array_index = lua_gettop(state);
            metatables(state, metabase_array_index, pconverter, metatypes);
            Metatable::setMultipleBase(state, metatable_index, metabase_array_index);
        }

#else
#define METAENGINE_LUAWRAPPER_GET_BASE_METATABLE(N)                                                \
    class_userdata::get_metatable<METAENGINE_LUAWRAPPER_PP_CAT(A, N)>(state);                      \
    lua_rawseti(state, metabase_array_index, lua_rawlen(state, metabase_array_index) + 1);         \
    pconverter.add_type_conversion<METAENGINE_LUAWRAPPER_PP_CAT(A, N), class_type>();
#define METAENGINE_LUAWRAPPER_MULTIPLE_INHERITANCE_SETBASE_DEF(N)                                  \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    void set_base_metatable(                                                                       \
            lua_State *state, int metatable_index,                                                 \
            types::typetag<MultipleBase<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>>) const { \
        PointerConverter &pconverter = PointerConverter::get(state);                               \
        lua_createtable(state, N, 0);                                                              \
        int metabase_array_index = lua_gettop(state);                                              \
        METAENGINE_LUAWRAPPER_PP_REPEAT(N, METAENGINE_LUAWRAPPER_GET_BASE_METATABLE);              \
        Metatable::setMultipleBase(state, metatable_index, metabase_array_index);                  \
    }

        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_CLASS_MAX_BASE_CLASSES,
                                            METAENGINE_LUAWRAPPER_MULTIPLE_INHERITANCE_SETBASE_DEF)
#undef METAENGINE_LUAWRAPPER_MULTIPLE_INHERITANCE_SETBASE_DEF
#undef METAENGINE_LUAWRAPPER_GET_BASE_METATABLE
#endif

        bool has_key(const std::string &key) {
            if (member_map_.count(key) > 0) { return true; }
            if (property_map_.count(key) > 0) { return true; }
            std::string propkey = METAENGINE_LUAWRAPPER_PROPERTY_PREFIX + key;
            if (member_map_.count(propkey) > 0)//_prop_keyname is reserved for property
            {
                return true;
            }
            return false;
        }

        Metatable::PropMapType property_map_;
        Metatable::MemberMapType member_map_;
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for UserdataMetatable
    template<typename T, typename Base>
    struct lua_type_traits<UserdataMetatable<T, Base>>
    {
        typedef const UserdataMetatable<T, Base> &push_type;

        static int push(lua_State *l, push_type ref) {
            ref.pushCreateMetatable(l);
            return 1;
        }
    };
}// namespace LuaWrapper

namespace LuaWrapper {
    class State;

    /**
* This class is the type returned by members of non-const LuaRef(Table) when
* directly accessing its elements.
*/
    template<typename KEY>
    class TableKeyReferenceProxy : public detail::LuaVariantImpl<TableKeyReferenceProxy<KEY>> {
    public:
        int pushStackIndex(lua_State *state) const {
            push(state);
            return lua_gettop(state);
        }
        lua_State *state() const { return state_; }

        friend class LuaRef;
        friend class State;

        //! this is not copy.same assign from referenced value.
        TableKeyReferenceProxy &operator=(const TableKeyReferenceProxy &src) {
            detail::table_proxy::set(state_, table_index_, key_, src);
            return *this;
        }

        //! assign from T
        template<typename T>
        TableKeyReferenceProxy &operator=(const T &src) {
            detail::table_proxy::set(state_, table_index_, key_, src);
            return *this;
        }
#if METAENGINE_LUAWRAPPER_USE_CPP11
        template<typename T>
        TableKeyReferenceProxy &operator=(T &&src) {
            detail::table_proxy::set(state_, table_index_, key_, std::forward<T>(src));
            return *this;
        }
#endif

        bool isNilref() const {
            if (!state_) { return false; }
            util::ScopedSavedStack save(state_);
            push(state_);
            return lua_isnoneornil(state_, -1);
        }

        //! register class metatable to lua and set to table
        template<typename T, typename P>
        void setClass(const UserdataMetatable<T, P> &reg) {
            set_class(reg);
        }

        //! set function
        template<typename T>
        void setFunction(T f) {
            detail::table_proxy::set(state_, table_index_, key_, LuaWrapper::function(f));
        }

        int push(lua_State *state) const {
            int type = lua_type(state_, table_index_);
            if (type != LUA_TTABLE && type != LUA_TUSERDATA) {
                lua_pushnil(state);
                return 1;
            }

            detail::table_proxy::get(state_, table_index_, key_);

            if (state_ != state) { lua_xmove(state_, state, 1); }
            return 1;
        }

        int push() const { return push(state_); }

        int type() const {
            util::ScopedSavedStack save(state_);
            push();
            return lua_type(state_, -1);
        }

        ~TableKeyReferenceProxy() {
            if (state_) { lua_settop(state_, stack_top_); }
        }

        ///!constructs the reference. Accessible only to LuaWrapper::LuaRef itself
        TableKeyReferenceProxy(const TableKeyReferenceProxy &src)
            : state_(src.state_), stack_top_(src.stack_top_), table_index_(src.table_index_),
              key_(src.key_) {
            src.state_ = 0;
        }

        ///!constructs the reference. Accessible only to LuaWrapper::LuaRef itself
        TableKeyReferenceProxy(lua_State *state, int table_index, KEY key, int revstacktop)
            : state_(state), stack_top_(revstacktop), table_index_(table_index), key_(key) {}

    private:
        template<typename T, typename P>
        void set_class(const UserdataMetatable<T, P> &reg) {
            detail::table_proxy::set(state_, table_index_, key_, reg);
        }

        ///!constructs the reference. Accessible only to LuaWrapper::LuaRef itself
        TableKeyReferenceProxy(lua_State *state, int table_index, const KEY &key, int revstacktop,
                               const NoTypeCheck &)
            : state_(state), stack_top_(revstacktop), table_index_(table_index), key_(key) {}

        TableKeyReferenceProxy(const LuaTable &table, const KEY &key)
            : state_(table.state()), stack_top_(lua_gettop(state_)), key_(key) {
            util::one_push(state_, table);
            table_index_ = stack_top_ + 1;
        }
        TableKeyReferenceProxy(const LuaRef &table, const KEY &key)
            : state_(table.state()), stack_top_(lua_gettop(state_)), key_(key) {
            util::one_push(state_, table);
            table_index_ = stack_top_ + 1;
            int t = lua_type(state_, table_index_);
            if (t != LUA_TTABLE) {
                except::typeMismatchError(state_,
                                          lua_typename(state_, t) + std::string(" is not table"));
            }
        }

        mutable lua_State *state_;// mutable for RVO unsupported compiler
        int stack_top_;
        int table_index_;
        KEY key_;
    };

    template<typename KEY>
    inline std::ostream &operator<<(std::ostream &os, const TableKeyReferenceProxy<KEY> &ref) {
        lua_State *state = ref.state();
        util::ScopedSavedStack save(state);
        int stackIndex = ref.pushStackIndex(state);
        util::stackValueDump(os, state, stackIndex);
        return os;
    }

    namespace detail {
        template<typename T>
        inline bool LuaFunctionImpl<T>::setFunctionEnv(const LuaTable &env) {
            lua_State *state = state_();
            if (!state) { return false; }
            util::ScopedSavedStack save(state);
            int stackIndex = pushStackIndex_(state);
            int t = lua_type(state, stackIndex);
            if (t != LUA_TFUNCTION) {
                except::typeMismatchError(state,
                                          lua_typename(state, t) + std::string(" is not function"));
                return false;
            }
            env.push(state);
#if LUA_VERSION_NUM >= 502
            lua_setupvalue(state, stackIndex, 1);
#else
            lua_setfenv(state, stackIndex);
#endif
            return true;
        }
        template<typename T>
        inline bool LuaFunctionImpl<T>::setFunctionEnv(NewTable) {
            return setFunctionEnv(LuaTable(state_()));
        }

        template<typename T>
        inline LuaTable LuaFunctionImpl<T>::getFunctionEnv() const {
            lua_State *state = state_();
            util::ScopedSavedStack save(state);
            if (!state) {
                except::typeMismatchError(state, "is nil");
                return LuaTable();
            }
            int stackIndex = pushStackIndex_(state);
            int t = lua_type(state, stackIndex);
            if (t != LUA_TFUNCTION) {
                except::typeMismatchError(state,
                                          lua_typename(state, t) + std::string(" is not function"));
                return LuaTable();
            }
#if LUA_VERSION_NUM >= 502
            lua_getupvalue(state, stackIndex, 1);
#else
            lua_getfenv(state, stackIndex);
#endif
            return LuaTable(state, StackTop());
        }

        template<typename T>
        void LuaThreadImpl<T>::setFunction(const LuaFunction &f) {
            lua_State *corstate = getthread();
            if (corstate) {
                lua_settop(corstate, 0);
                f.push(corstate);
            }
        }

        template<typename T>
        bool LuaTableOrUserDataImpl<T>::setMetatable(const LuaTable &table) {
            lua_State *state = state_();
            if (!state) {
                except::typeMismatchError(state, "is nil");
                return false;
            }
            util::ScopedSavedStack save(state);
            int stackindex = pushStackIndex_(state);
            int t = lua_type(state, stackindex);
            if (t != LUA_TTABLE && t != LUA_TUSERDATA) {
                except::typeMismatchError(state,
                                          lua_typename(state, t) + std::string(" is not table"));
                return false;
            }
            table.push();
            return lua_setmetatable(state, stackindex) != 0;
        }
        template<typename T>
        LuaTable LuaTableOrUserDataImpl<T>::getMetatable() const {
            lua_State *state = state_();
            if (!state) {
                except::typeMismatchError(state, "is nil");
                return LuaTable();
            }
            util::ScopedSavedStack save(state);
            int stackindex = pushStackIndex_(state);
            int t = lua_type(state, stackindex);
            if (t != LUA_TTABLE && t != LUA_TUSERDATA) {
                except::typeMismatchError(state,
                                          lua_typename(state, t) + std::string(" is not table"));
                return LuaTable();
            }
            if (!lua_getmetatable(state, stackindex)) { lua_pushnil(state); }
            return LuaTable(state, StackTop());
        }
        template<typename T>
        MemberFunctionBinder LuaTableOrUserDataImpl<T>::operator->*(const char *function_name) {
            push_(state_());
            return MemberFunctionBinder(LuaRef(state_(), StackTop()), function_name);
        }

        template<typename T>
        template<typename KEY>
        LuaStackRef LuaTableOrUserDataImpl<T>::getField(const KEY &key) const {
            lua_State *state = state_();
            if (!state) {
                except::typeMismatchError(state, "is nil");
                return LuaStackRef();
            }
            push_(state);
            detail::table_proxy::get(state, lua_gettop(state), key);
            lua_remove(state, -2);// remove table
            return LuaStackRef(state, -1, true);
        }
        template<typename T>
        template<typename KEY>
        LuaStackRef LuaTableImpl<T>::getRawField(const KEY &key) const {
            lua_State *state = state_();
            if (!state) {
                except::typeMismatchError(state, "is nil");
                return LuaStackRef();
            }
            push_(state);
            detail::table_proxy::rawget(state, lua_gettop(state), key);
            lua_remove(state, -2);// remove table
            return LuaStackRef(state, -1, true);
        }

        template<typename T>
        template<typename KEY>
        LuaStackRef LuaTableOrUserDataImpl<T>::operator[](KEY key) const {
            return getField(key);
        }

        template<typename T>
        std::vector<LuaRef> LuaTableImpl<T>::values() const {
            return values<LuaRef>();
        }
        template<typename T>
        std::vector<LuaRef> LuaTableImpl<T>::keys() const {
            return keys<LuaRef>();
        }
        template<typename T>
        std::map<LuaRef, LuaRef> LuaTableImpl<T>::map() const {
            return map<LuaRef, LuaRef>();
        }

        template<typename T>
        template<typename K>
        TableKeyReferenceProxy<K> LuaTableOrUserDataImpl<T>::operator[](K key) {
            lua_State *state = state_();
            int stack_top = lua_gettop(state);
            int stackindex = pushStackIndex_(state);
            return TableKeyReferenceProxy<K>(state, stackindex, key, stack_top);
        }
    }// namespace detail

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for TableKeyReferenceProxy<KEY>
    template<typename KEY>
    struct lua_type_traits<TableKeyReferenceProxy<KEY>>
    {
        static int push(lua_State *l, const TableKeyReferenceProxy<KEY> &ref) {
            return ref.push(l);
        }
    };

#if METAENGINE_LUAWRAPPER_USE_CPP11
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for std::array<T, A>
    template<typename T, size_t S>
    struct lua_type_traits<std::array<T, S>>
    {
        typedef std::array<T, S> get_type;
        typedef const std::array<T, S> &push_type;

        static bool checkType(lua_State *l, int index) {
            if (lua_type(l, index) != LUA_TTABLE) { return false; }

            LuaStackRef table(l, index);
            if (table.size() != S) { return false; }// TODO
            bool valid = true;
            table.foreach_table_breakable<LuaStackRef, LuaStackRef>(
                    [&](const LuaStackRef &k, const LuaStackRef &v) {
                        valid = valid && k.typeTest<size_t>() && v.typeTest<T>();
                        return valid;
                    });
            return valid;
        }
        static bool strictCheckType(lua_State *l, int index) {
            if (lua_type(l, index) != LUA_TTABLE) { return false; }

            LuaStackRef table(l, index);
            if (table.size() != S) { return false; }// TODO
            bool valid = true;
            table.foreach_table_breakable<LuaStackRef, LuaStackRef>(
                    [&](const LuaStackRef &k, const LuaStackRef &v) {
                        valid = valid && k.typeTest<size_t>() && v.typeTest<T>();
                        return valid;
                    });
            return valid;
        }
        static get_type get(lua_State *l, int index) {
            if (lua_type(l, index) != LUA_TTABLE) {
                except::typeMismatchError(l, std::string("type mismatch"));
                return get_type();
            }
            LuaStackRef t(l, index);
            if (t.size() != S)// TODO
            {
                except::typeMismatchError(l, std::string("type mismatch"));
            }
            get_type res;
            t.foreach_table<size_t, const T &>([&](size_t k, const T &v) {
                if (k > 0 && k <= S) { res[k - 1] = v; }
            });
            return res;
        }
        static int push(lua_State *l, push_type v) {
            lua_createtable(l, int(S), 0);
            for (size_t i = 0; i < S; ++i) {
                util::one_push(l, v[i]);
                lua_rawseti(l, -2, i + 1);
            }
            return 1;
        }
    };
#endif
#ifndef METAENGINE_LUAWRAPPER_NO_STD_VECTOR_TO_TABLE

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for std::vector<T, A>
    template<typename T, typename A>
    struct lua_type_traits<std::vector<T, A>>
    {
        typedef std::vector<T, A> get_type;
        typedef const std::vector<T, A> &push_type;
        struct checkTypeForEach
        {
            checkTypeForEach(bool &valid) : valid_(valid) {}
            bool &valid_;
            bool operator()(const LuaStackRef &k, const LuaStackRef &v) {
                valid_ = k.typeTest<size_t>() && v.weakTypeTest<T>();
                return valid_;
            }
        };
        struct strictCheckTypeForEach
        {
            strictCheckTypeForEach(bool &valid) : valid_(valid) {}
            bool &valid_;
            bool operator()(const LuaStackRef &k, const LuaStackRef &v) {
                valid_ = k.typeTest<size_t>() && v.typeTest<T>();
                return valid_;
            }
        };

        static bool checkType(lua_State *l, int index) {
            LuaStackRef table(l, index);
            if (table.type() != LuaRef::TYPE_TABLE) { return false; }

            bool valid = true;
            table.foreach_table_breakable<LuaStackRef, LuaStackRef>(checkTypeForEach(valid));
            return valid;
        }
        static bool strictCheckType(lua_State *l, int index) {
            LuaStackRef table(l, index);
            if (table.type() != LuaRef::TYPE_TABLE) { return false; }

            bool valid = true;
            table.foreach_table_breakable<LuaStackRef, LuaStackRef>(strictCheckTypeForEach(valid));
            return valid;
        }

        static get_type get(lua_State *l, int index) {
            if (lua_type(l, index) != LUA_TTABLE) {
                except::typeMismatchError(l, std::string("type mismatch"));
                return get_type();
            }
            return LuaStackRef(l, index).values<T>();
        }
#if METAENGINE_LUAWRAPPER_USE_CPP11
        typedef std::vector<T, A> &&move_push_type;
        static int push(lua_State *l, move_push_type v) {
            lua_createtable(l, int(v.size()), 0);
            int count = 1;// array is 1 origin in Lua
            for (typename std::vector<T, A>::iterator it = v.begin(); it != v.end(); ++it) {
                util::one_push(l, static_cast<T &&>(*it));
                lua_rawseti(l, -2, count++);
            }
            return 1;
        }
#endif
        static int push(lua_State *l, push_type v) {
            lua_createtable(l, int(v.size()), 0);
            int count = 1;// array is 1 origin in Lua
            for (typename std::vector<T, A>::const_iterator it = v.begin(); it != v.end(); ++it) {
                util::one_push(l, static_cast<const T &>(*it));
                lua_rawseti(l, -2, count++);
            }
            return 1;
        }
    };
#endif

#ifndef METAENGINE_LUAWRAPPER_NO_STD_MAP_TO_TABLE
    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for std::map<K, V, C, A>
    template<typename K, typename V, typename C, typename A>
    struct lua_type_traits<std::map<K, V, C, A>>
    {
        typedef std::map<K, V, C, A> get_type;
        typedef const std::map<K, V, C, A> &push_type;

        struct checkTypeForEach
        {
            checkTypeForEach(bool &valid) : valid_(valid) {}
            bool &valid_;
            bool operator()(const LuaStackRef &k, const LuaStackRef &v) {
                valid_ = k.weakTypeTest<K>() && v.weakTypeTest<V>();
                return valid_;
            }
        };
        struct strictCheckTypeForEach
        {
            strictCheckTypeForEach(bool &valid) : valid_(valid) {}
            bool &valid_;
            bool operator()(const LuaStackRef &k, const LuaStackRef &v) {
                valid_ = k.typeTest<K>() && v.typeTest<V>();
                return valid_;
            }
        };
        static bool checkType(lua_State *l, int index) {
            LuaStackRef table(l, index);
            if (table.type() != LuaRef::TYPE_TABLE) { return false; }

            bool valid = true;
            table.foreach_table_breakable<LuaStackRef, LuaStackRef>(checkTypeForEach(valid));
            return valid;
        }
        static bool strictCheckType(lua_State *l, int index) {
            LuaStackRef table(l, index);
            if (table.type() != LuaRef::TYPE_TABLE) { return false; }

            bool valid = true;
            table.foreach_table_breakable<LuaStackRef, LuaStackRef>(strictCheckTypeForEach(valid));
            return valid;
        }

        static get_type get(lua_State *l, int index) {
            if (lua_type(l, index) != LUA_TTABLE) {
                except::typeMismatchError(l, std::string("type mismatch"));
                return get_type();
            }
            return LuaStackRef(l, index).map<K, V>();
        }
        static int push(lua_State *l, push_type v) {
            lua_createtable(l, 0, int(v.size()));
            for (typename std::map<K, V, C, A>::const_iterator it = v.begin(); it != v.end();
                 ++it) {
                util::one_push(l, it->first);
                util::one_push(l, it->second);
                lua_rawset(l, -3);
            }
            return 1;
        }
    };
#endif

    struct TableDataElement
    {
        typedef std::pair<AnyDataPusher, AnyDataPusher> keyvalue_type;

        template<typename Value>
        TableDataElement(Value value) : keyvalue(keyvalue_type(AnyDataPusher(), value)) {}

        template<typename Key, typename Value>
        TableDataElement(Key key, Value value) : keyvalue(keyvalue_type(key, value)) {}
        std::pair<AnyDataPusher, AnyDataPusher> keyvalue;
    };

    struct TableData
    {
        typedef std::pair<AnyDataPusher, AnyDataPusher> data_type;

#if METAENGINE_LUAWRAPPER_USE_CPP11
        TableData(std::initializer_list<TableDataElement> list)
            : elements(list.begin(), list.end()) {}
#endif
        template<typename IT>
        TableData(IT beg, IT end) : elements(beg, end) {}

        TableData() {}
        std::vector<TableDataElement> elements;
    };

    /// @ingroup lua_type_traits
    /// @brief lua_type_traits for TableData
    template<>
    struct lua_type_traits<TableData>
    {
        static int push(lua_State *l, const TableData &list) {
            lua_createtable(l, int(list.elements.size()), int(list.elements.size()));
            int count = 1;// array is 1 origin in Lua
            for (std::vector<TableDataElement>::const_iterator it = list.elements.begin();
                 it != list.elements.end(); ++it) {
                const TableDataElement &v = *it;
                if (v.keyvalue.first.empty()) {
                    util::one_push(l, v.keyvalue.second);
                    lua_rawseti(l, -2, count++);
                } else {
                    util::one_push(l, v.keyvalue.first);
                    util::one_push(l, v.keyvalue.second);
                    lua_rawset(l, -3);
                }
            }
            return 1;
        }
    };
}// namespace LuaWrapper

namespace LuaWrapper {
    /// @addtogroup State
    /// @{

    /// @brief Load library info type @see State::openlibs @see State::State(const
    /// LoadLibs &libs)
    typedef std::pair<std::string, lua_CFunction> LoadLib;

    /// @brief Load libraries info @see State::openlibs @see State::State(const
    /// LoadLibs &libs)
    typedef std::vector<LoadLib> LoadLibs;

    /// @brief return no load library type @see State::State(const LoadLibs &libs)
    inline LoadLibs NoLoadLib() { return LoadLibs(); }

    /// @brief All load standard libraries type @see State::openlibs
    struct AllLoadLibs
    {
    };

    template<typename Allocator>
    void *AllocatorFunction(void *ud, void *ptr, size_t osize, size_t nsize) {
        Allocator *allocator = static_cast<Allocator *>(ud);
        if (nsize == 0) {
            allocator->deallocate(ptr, osize);
        } else if (ptr) {
            return allocator->reallocate(ptr, nsize);
        } else {
            return allocator->allocate(nsize);
        }
        return 0;
    }

    struct DefaultAllocator
    {
        typedef void *pointer;
        typedef size_t size_type;
        pointer allocate(size_type n) { return std::malloc(n); }
        pointer reallocate(pointer p, size_type n) { return std::realloc(p, n); }
        void deallocate(pointer p, size_type n) {
            METAENGINE_LUAWRAPPER_UNUSED(n);
            std::free(p);
        }
    };

    /// lua_State wrap class
    class State {
        standard::shared_ptr<void> allocator_holder_;
        lua_State *state_;
        bool created_;

        // non copyable
        State(const State &);
        State &operator=(const State &);

        static int initializing_panic(lua_State *L) {
            ErrorHandler::throwDefaultError(lua_status(L), lua_tostring(L, -1));
            return 0;// return to Lua to abort
        }
        static int default_panic(lua_State *L) {
            if (ErrorHandler::handle(lua_status(L), L)) { return 0; }
            fprintf(stderr, "PANIC: unprotected error in call to Lua API (%s)\n",
                    lua_tostring(L, -1));
            fflush(stderr);
            return 0;// return to Lua to abort
        }
        static void stderror_out(int status, const char *message) {
            METAENGINE_LUAWRAPPER_UNUSED(status);
            std::cerr << message << std::endl;
        }

        template<typename Libs>
        void init(const Libs &lib) {
            if (state_) {
                lua_atpanic(state_, &initializing_panic);
                try {
                    if (!ErrorHandler::getHandler(state_)) { setErrorHandler(&stderror_out); }
                    registerMainThreadIfNeeded();
                    openlibs(lib);
                    lua_atpanic(state_, &default_panic);
                } catch (const LuaException &) {
                    lua_close(state_);
                    state_ = 0;
                }
            }
        }

        void registerMainThreadIfNeeded() {
#if LUA_VERSION_NUM < 502
            if (state_) { util::registerMainThread(state_); }
#endif
        }

    public:
        /// @brief create Lua state with lua standard library
        State() : allocator_holder_(), state_(luaL_newstate()), created_(true) {
            init(AllLoadLibs());
        }

        /// @brief create Lua state with lua standard library. Can not use this
        /// constructor at luajit. error message is 'Must use luaL_newstate() for 64
        /// bit target'
        /// @param allocator allocator for memory allocation @see DefaultAllocator
        template<typename Allocator>
        State(standard::shared_ptr<Allocator> allocator)
            : allocator_holder_(allocator),
              state_(lua_newstate(&AllocatorFunction<Allocator>, allocator_holder_.get())),
              created_(true) {
            init(AllLoadLibs());
        }

        /// @brief create Lua state with (or without) libraries.
        /// @param libs load libraries
        /// e.g. LoadLibs libs;libs.push_back(LoadLib("libname",libfunction));State
        /// state(libs);
        /// e.g. State state({{"libname",libfunction}}); for c++ 11
        State(const LoadLibs &libs) : allocator_holder_(), state_(luaL_newstate()), created_(true) {
            init(libs);
        }

        /// @brief create Lua state with (or without) libraries. Can not use this
        /// constructor at luajit. error message is 'Must use luaL_newstate() for 64
        /// bit target'
        /// @param libs load libraries
        /// @param allocator allocator for memory allocation @see DefaultAllocator
        template<typename Allocator>
        State(const LoadLibs &libs, standard::shared_ptr<Allocator> allocator)
            : allocator_holder_(allocator),
              state_(lua_newstate(&AllocatorFunction<Allocator>, allocator_holder_.get())),
              created_(true) {
            init(libs);
        }

        /// @brief construct using created lua_State.
        /// @param lua created lua_State. It is not call lua_close() in this class
        State(lua_State *lua) : state_(lua), created_(false) {
            if (state_) {
                registerMainThreadIfNeeded();
                if (!ErrorHandler::getHandler(state_)) { setErrorHandler(&stderror_out); }
            }
        }
        ~State() {
            if (created_ && state_) { lua_close(state_); }
        }

        /// @brief set error handler for lua error.
        void setErrorHandler(
                standard::function<void(int statuscode, const char *message)> errorfunction) {
            if (!state_) { return; }
            util::ScopedSavedStack save(state_);
            ErrorHandler::registerHandler(state_, errorfunction);
        }

        /// @brief load all lua standard library
        void openlibs(AllLoadLibs = AllLoadLibs()) {
            if (!state_) { return; }
            luaL_openlibs(state_);
        }

        /// @brief load lua library
        LuaStackRef openlib(const LoadLib &lib) {
            if (!state_) { return LuaStackRef(); }
            luaL_requiref(state_, lib.first.c_str(), lib.second, 1);
            return LuaStackRef(state_, -1, true);
        }
        /// @brief load lua library
        LuaStackRef openlib(std::string name, lua_CFunction f) { return openlib(LoadLib(name, f)); }

        /// @brief load lua libraries
        void openlibs(const LoadLibs &libs) {
            for (LoadLibs::const_iterator it = libs.begin(); it != libs.end(); ++it) {
                openlib(*it);
            }
        }

        /// @brief If there are no errors,compiled file as a Lua function and return.
        ///  Otherwise send error message to error handler and return nil reference
        /// @param file  file path of lua script
        /// @return reference of lua function
        LuaFunction loadfile(const std::string &file) {
            return LuaFunction::loadfile(state_, file);
        }
        /// @brief If there are no errors,compiled file as a Lua function and return.
        ///  Otherwise send error message to error handler and return nil reference
        /// @param file  file path of lua script
        /// @return reference of lua function
        LuaFunction loadfile(const char *file) { return LuaFunction::loadfile(state_, file); }

        /// @brief If there are no errors,compiled stream as a Lua function and
        /// return.
        ///  Otherwise send error message to error handler and return nil reference
        /// @param stream stream of lua script
        /// @param chunkname chunkname of lua script
        /// @return reference of lua function
        LuaFunction loadstream(std::istream &stream, const char *chunkname = 0) {
            return LuaFunction::loadstream(state_, stream, chunkname);
        }
        /// @brief Loads and runs the given stream.
        /// @param stream stream of lua script
        /// @param chunkname chunkname of lua script
        /// @param env execute env table
        /// @return If there are no errors, returns true.Otherwise return false
        bool dostream(std::istream &stream, const char *chunkname = 0,
                      const LuaTable &env = LuaTable()) {
            util::ScopedSavedStack save(state_);
            LuaStackRef f = LuaFunction::loadstreamtostack(state_, stream, chunkname);
            if (!f) {// load failed
                return false;
            }
            if (!env.isNilref()) { f.setFunctionEnv(env); }

            FunctionResults ret = f.call<FunctionResults>();
            return !ret.resultStatus();
        }

        /// @brief If there are no errors,compiled string as a Lua function and
        /// return.
        ///  Otherwise send error message to error handler and return nil reference
        /// @param str lua code
        /// @return reference of lua function
        LuaFunction loadstring(const std::string &str) {
            return LuaFunction::loadstring(state_, str);
        }
        /// @brief If there are no errors,compiled string as a Lua function and
        /// return.
        ///  Otherwise send error message to error handler and return nil reference
        /// @param str lua code
        /// @return reference of lua function
        LuaFunction loadstring(const char *str) { return LuaFunction::loadstring(state_, str); }

        /// @brief Loads and runs the given file.
        /// @param file file path of lua script
        /// @param env execute env table
        /// @return If there are no errors, returns true.Otherwise return false
        bool dofile(const std::string &file, const LuaTable &env = LuaTable()) {
            return dofile(file.c_str(), env);
        }

        /// @brief Loads and runs the given file.
        /// @param file file path of lua script
        /// @param env execute env table
        /// @return If there are no errors, returns true.Otherwise return false
        bool dofile(const char *file, const LuaTable &env = LuaTable()) {
            util::ScopedSavedStack save(state_);

            int status = luaL_loadfile(state_, file);

            if (status) {
                ErrorHandler::handle(status, state_);
                return false;
            }

            if (!env.isNilref()) {// register _ENV
                env.push();
#if LUA_VERSION_NUM >= 502
                lua_setupvalue(state_, -2, 1);
#else
                lua_setfenv(state_, -2);
#endif
            }

            status = lua_pcall_wrap(state_, 0, LUA_MULTRET);
            if (status) {
                ErrorHandler::handle(status, state_);
                return false;
            }
            return true;
        }

        /// @brief Loads and runs the given string.
        /// @param str lua script cpde
        /// @param env execute env table
        /// @return If there are no errors, returns true.Otherwise return false
        bool dostring(const char *str, const LuaTable &env = LuaTable()) {
            util::ScopedSavedStack save(state_);

            int status = luaL_loadstring(state_, str);
            if (status) {
                ErrorHandler::handle(status, state_);
                return false;
            }
            if (!env.isNilref()) {// register _ENV
                env.push();
#if LUA_VERSION_NUM >= 502
                lua_setupvalue(state_, -2, 1);
#else
                lua_setfenv(state_, -2);
#endif
            }
            status = lua_pcall_wrap(state_, 0, LUA_MULTRET);
            if (status) {
                ErrorHandler::handle(status, state_);
                return false;
            }
            return true;
        }
        /// @brief Loads and runs the given string.
        /// @param str lua script cpde
        /// @param env execute env table
        /// @return If there are no errors, returns true.Otherwise return false
        bool dostring(const std::string &str, const LuaTable &env = LuaTable()) {
            return dostring(str.c_str(), env);
        }

        /// @brief Loads and runs the given string.
        /// @param str lua script cpde
        /// @return If there are no errors, returns true.Otherwise return false
        bool operator()(const std::string &str) { return dostring(str); }

        /// @brief Loads and runs the given string.
        /// @param str lua script cpde
        /// @return If there are no errors, returns true.Otherwise return false
        bool operator()(const char *str) { return dostring(str); }

        /// @brief return element reference from global table
        /// @param str table key
        /// @return proxy class for reference to table.
        TableKeyReferenceProxy<std::string> operator[](const std::string &str) {
            int stack_top = lua_gettop(state_);
            util::push_args(state_, GlobalTable());
            int table_index = stack_top + 1;
            return TableKeyReferenceProxy<std::string>(state_, table_index, str, stack_top,
                                                       NoTypeCheck());
        }

        /// @brief return element reference from global table
        /// @param str table key
        /// @return proxy class for reference to table.

        TableKeyReferenceProxy<const char *> operator[](const char *str) {
            int stack_top = lua_gettop(state_);
            util::push_args(state_, GlobalTable());
            int table_index = stack_top + 1;
            return TableKeyReferenceProxy<const char *>(state_, table_index, str, stack_top,
                                                        NoTypeCheck());
        }

        /// @brief return global table
        /// @return global table.
        LuaTable globalTable() { return newRef(GlobalTable()); }

        /// @brief create new Lua reference from argument value
        /// @return Lua reference.
        template<typename T>
        LuaRef newRef(const T &value) {
            return LuaRef(state_, value);
        }
#if METAENGINE_LUAWRAPPER_USE_CPP11

        /// @brief create new Lua reference from argument value
        /// @return Lua reference.
        template<typename T>
        LuaRef newRef(T &&value) {
            return LuaRef(state_, std::forward<T>(value));
        }
#endif

        /// @brief create new Lua table
        /// @return Lua table reference.
        LuaTable newTable() { return LuaTable(state_); }

        /// @brief create new Lua table
        /// @param reserve_array reserved array count
        /// @param reserve_record reserved record count
        /// @return Lua table reference.
        LuaTable newTable(int reserve_array, int reserve_record) {
            return LuaTable(state_, NewTable(reserve_array, reserve_record));
        }

        /// @brief create new Lua thread
        /// @return Lua thread reference.
        LuaThread newThread() { return LuaThread(state_); }

        /// @brief create new Lua thread with lua function
        /// @param f function
        /// @return Lua thread reference.
        LuaThread newThread(const LuaFunction &f) {
            LuaThread cor(state_);
            cor.setFunction(f);
            return cor;
        }

        /// @brief argument value push to stack.
        /// @param value value
        template<typename T>
        void pushToStack(T value) {
            util::push_args(state_, value);
        }

        /// @brief pop from stack.
        /// @return reference to pop value.
        LuaRef popFromStack() { return LuaRef(state_, StackTop()); }

        /// @brief Garbage Collection of Lua
        struct GCType
        {
            GCType(lua_State *state) : state_(state) {}

            /// @brief Performs a full garbage-collection cycle.
            void collect() { lua_gc(state_, LUA_GCCOLLECT, 0); }
            /// @brief Performs an incremental step of garbage collection.
            /// @return If returns true,the step finished a collection cycle.
            bool step() { return lua_gc(state_, LUA_GCSTEP, 0) == 1; }

            /// @brief Performs an incremental step of garbage collection.
            /// @param size the collector will perform as if that amount of memory (in
            /// KBytes) had been allocated by Lua.
            bool step(int size) { return lua_gc(state_, LUA_GCSTEP, size) == 1; }

            /// @brief enable gc
            void restart() { enable(); }

            /// @brief disable gc
            void stop() { disable(); }

            /// @brief returns the total memory in use by Lua in Kbytes.
            int count() const { return lua_gc(state_, LUA_GCCOUNT, 0); }

            /// @brief sets arg as the new value for the pause of the collector. Returns
            /// the previous value for pause.
            int steppause(int value) { return lua_gc(state_, LUA_GCSETPAUSE, value); }

            ///  @brief sets arg as the new value for the step multiplier of the
            ///  collector. Returns the previous value for step.
            int setstepmul(int value) { return lua_gc(state_, LUA_GCSETSTEPMUL, value); }

            /// @brief enable gc
            void enable() { lua_gc(state_, LUA_GCRESTART, 0); }

            /// @brief disable gc
            void disable() { lua_gc(state_, LUA_GCSTOP, 0); }
#if LUA_VERSION_NUM >= 502

            /// @brief returns a boolean that tells whether the collector is running
            bool isrunning() const { return isenabled(); }

            /// @brief returns a boolean that tells whether the collector is running
            bool isenabled() const { return lua_gc(state_, LUA_GCISRUNNING, 0) != 0; }
#endif

        private:
            lua_State *state_;
        };

        // /@brief  return Garbage collection interface.
        GCType gc() const { return GCType(state_); }
        /// @brief performs a full garbage-collection cycle.
        void garbageCollect() { gc().collect(); }

        /// @brief returns the current amount of memory (in Kbytes) in use by Lua.
        size_t useKBytes() const { return size_t(gc().count()); }

        /// @brief create Table and push to stack.
        /// using for Lua module
        /// @return return Lua Table Reference
        LuaTable newLib() {
            LuaTable newtable = newTable();
            newtable.push(state_);
            return newtable;
        }

        /// @brief return lua_State*.
        /// @return lua_State*
        lua_State *state() { return state_; };

        /// @brief check valid lua_State.
        bool isInvalid() const { return !state_; }
    };

    /// @}
}// namespace LuaWrapper

namespace LuaWrapper {

    template<typename RefTuple, typename GetTuple>
    struct ref_tuple
    {
        RefTuple tref;
        ref_tuple(const RefTuple &va) : tref(va) {}
        void operator=(const FunctionResults &fres) {
            tref = fres.get_result(types::typetag<GetTuple>());
        }
        template<class T>
        void operator=(const T &fres) {
            tref = fres;
        }
    };
#if METAENGINE_LUAWRAPPER_USE_CPP11
    template<class... Args>
    ref_tuple<standard::tuple<Args &...>, standard::tuple<Args...>> tie(Args &...va) {
        typedef standard::tuple<Args &...> RefTuple;
        typedef standard::tuple<Args...> GetTuple;
        return ref_tuple<RefTuple, GetTuple>(RefTuple(va...));
    }
#else
#define METAENGINE_LUAWRAPPER_VARIADIC_REFARG_REP(N)                                               \
    METAENGINE_LUAWRAPPER_PP_CAT(A, N) & METAENGINE_LUAWRAPPER_PP_CAT(a, N)
#define METAENGINE_LUAWRAPPER_VARIADIC_TREFARG_REP(N) METAENGINE_LUAWRAPPER_PP_CAT(A, N) &
#define METAENGINE_LUAWRAPPER_TEMPLATE_REFARG_REPEAT(N)                                            \
    METAENGINE_LUAWRAPPER_PP_REPEAT_ARG(N, METAENGINE_LUAWRAPPER_VARIADIC_TREFARG_REP)
#define METAENGINE_LUAWRAPPER_REF_TUPLE(N)                                                         \
    standard::tuple<METAENGINE_LUAWRAPPER_TEMPLATE_REFARG_REPEAT(N)>
#define METAENGINE_LUAWRAPPER_GET_TUPLE(N)                                                         \
    standard::tuple<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>
#define METAENGINE_LUAWRAPPER_REF_TUPLE_DEF(N)                                                     \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    ref_tuple<METAENGINE_LUAWRAPPER_REF_TUPLE(N), METAENGINE_LUAWRAPPER_GET_TUPLE(N)> tie(         \
            METAENGINE_LUAWRAPPER_PP_REPEAT_ARG(N, METAENGINE_LUAWRAPPER_VARIADIC_REFARG_REP)) {   \
        return ref_tuple<METAENGINE_LUAWRAPPER_REF_TUPLE(N), METAENGINE_LUAWRAPPER_GET_TUPLE(N)>(  \
                METAENGINE_LUAWRAPPER_REF_TUPLE(N)(METAENGINE_LUAWRAPPER_PP_ARG_REPEAT(N)));       \
    }

    METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_TUPLE_SIZE,
                                        METAENGINE_LUAWRAPPER_REF_TUPLE_DEF)
#undef METAENGINE_LUAWRAPPER_VARIADIC_REFARG_REP
#undef METAENGINE_LUAWRAPPER_TEMPLATE_REFARG_REPEAT
#undef METAENGINE_LUAWRAPPER_REF_TUPLE
#undef METAENGINE_LUAWRAPPER_GET_TUPLE
#undef METAENGINE_LUAWRAPPER_REF_TUPLE_DEF
#endif
}// namespace LuaWrapper

/// @brief start define binding
#define METAENGINE_LUAWRAPPER_BINDINGS(MODULE_NAME)                                                \
                                                                                                   \
    void METAENGINE_LUAWRAPPER_PP_CAT(LuaWrapper_bind_internal_, MODULE_NAME)();                   \
                                                                                                   \
    int METAENGINE_LUAWRAPPER_PP_CAT(luaopen_, MODULE_NAME)(lua_State * L) {                       \
        return LuaWrapper::detail::bind_internal(                                                  \
                L, &METAENGINE_LUAWRAPPER_PP_CAT(LuaWrapper_bind_internal_, MODULE_NAME));         \
    }                                                                                              \
                                                                                                   \
    void METAENGINE_LUAWRAPPER_PP_CAT(LuaWrapper_bind_internal_, MODULE_NAME)()

namespace LuaWrapper {
    namespace detail {
        struct scope_stack
        {
            LuaTable current_scope() { return !stack.empty() ? stack.back() : LuaTable(); }
            static scope_stack &instance() {
                static scope_stack instance_;
                return instance_;
            }
            void push(const LuaTable &table) { stack.push_back(table); }
            void pop() { stack.pop_back(); }

        private:
            scope_stack() {}
            scope_stack(const scope_stack &);
            scope_stack &operator=(const scope_stack &);

            std::vector<LuaTable> stack;
        };
    }// namespace detail

    /// @ingroup another_binding_api
    /// @brief binding scope
    struct scope
    {
        scope(const std::string &name) : pushed_(true) {
            detail::scope_stack &stack = detail::scope_stack::instance();
            LuaTable current = stack.current_scope();
            if (!current[name]) { current[name] = NewTable(); }
            scope_table_ = current[name];
            stack.push(scope_table_);
        }
        scope(const LuaTable &t) : pushed_(true) {
            scope_table_ = t;
            detail::scope_stack::instance().push(scope_table_);
        }
        scope() : pushed_(false) {
            detail::scope_stack &stack = detail::scope_stack::instance();
            scope_table_ = stack.current_scope();
        }

        TableKeyReferenceProxy<std::string> attr(const std::string &name) {
            return scope_table_.operator[]<std::string>(name);
        }
        LuaTable table() { return scope_table_; }

        ~scope() {
            if (pushed_) { detail::scope_stack::instance().pop(); }
        }

    private:
        LuaTable scope_table_;
        bool pushed_;
    };

    namespace detail {
        inline int bind_internal(lua_State *L, void (*bindfn)()) {
            int count = lua_gettop(L);
            LuaWrapper::State state(L);
            LuaTable l = state.newTable();
            l.push();
            scope scope(l);
            bindfn();
            return lua_gettop(L) - count;
        }
    }// namespace detail

    /// @ingroup another_binding_api
    /// @brief define class binding
    template<typename ClassType, typename BaseType = void>
    struct class_ : private UserdataMetatable<ClassType, BaseType>
    {
        class_(const std::string &name) : name_(name) {}
        ~class_() {
            LuaTable scope = detail::scope_stack::instance().current_scope();
            if (scope) { scope[name_].setClass(*this); }
        }

#if METAENGINE_LUAWRAPPER_USE_CPP11
        template<typename... Args>
        class_ &constructor() {
            this->template setConstructors<ClassType(Args...)>();
            return *this;
        }

        template<typename... Constructors>
        class_ &constructors() {
            this->template setConstructors<Constructors...>();
            return *this;
        }
#else
        class_ &constructor() {
            this->template setConstructors<ClassType()>();
            return *this;
        }

#define METAENGINE_LUAWRAPPER_ADD_CON_FN_DEF(N)                                                    \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    class_ &constructor() {                                                                        \
        this->template setConstructors<ClassType(                                                  \
                METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N))>();                               \
        return *this;                                                                              \
    }

        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_ADD_CON_FN_DEF)
#undef METAENGINE_LUAWRAPPER_ADD_CON_FN_DEF

        class_ &constructors() {
            this->template setConstructors<ClassType()>();
            return *this;
        }

#define METAENGINE_LUAWRAPPER_ADD_CONS_FN_DEF(N)                                                   \
    template<METAENGINE_LUAWRAPPER_PP_TEMPLATE_DEF_REPEAT(N)>                                      \
    class_ &constructors() {                                                                       \
        this->template setConstructors<METAENGINE_LUAWRAPPER_PP_TEMPLATE_ARG_REPEAT(N)>();         \
        return *this;                                                                              \
    }

        METAENGINE_LUAWRAPPER_PP_REPEAT_DEF(METAENGINE_LUAWRAPPER_FUNCTION_MAX_ARGS,
                                            METAENGINE_LUAWRAPPER_ADD_CONS_FN_DEF)
#undef METAENGINE_LUAWRAPPER_ADD_CONS_FN_DEF
#endif

        /// @ingroup another_binding_api
        /// @brief function binding
        template<typename F>
        class_ &function(const char *name, F f) {
            this->addFunction(name, f);
            return *this;
        }

        template<typename F>
        class_ &function(const char *name, FunctionInvokerType<F> f) {
            this->addStaticField(name, f);
            return *this;
        }

        /// @ingroup another_binding_api
        /// @brief property binding
        template<typename F>
        class_ &property(const char *name, F f) {
            this->addProperty(name, f);
            return *this;
        }

        /// @ingroup another_binding_api
        /// @brief property binding with getter and sette function
        template<typename Getter, typename Setter>
        class_ &property(const char *name, Getter getter, Setter setter) {
            this->addProperty(name, getter, setter);
            return *this;
        }

        /// @brief class function binding
        template<typename F>
        class_ &class_function(const char *name, F f) {
            this->addStaticFunction(name, f);
            return *this;
        }

        template<typename F>
        class_ &class_function(const char *name, FunctionInvokerType<F> f) {
            this->addStaticField(name, f);
            return *this;
        }

        /// @ingroup another_binding_api
        /// @brief function binding
        template<typename F>
        class_ &def(const char *name, F f) {
            this->addFunction(name, f);
            return *this;
        }

        /// @ingroup another_binding_api
        /// @brief property binding with getter and sette function
        template<typename Getter, typename Setter>
        class_ &add_property(const char *name, Getter getter, Setter setter) {
            property(name, getter, setter);
            return *this;
        }

        /// @ingroup another_binding_api
        /// @brief property binding
        template<typename F>
        class_ &add_property(const char *name, F f) {
            property(name, f);
            return *this;
        }

        /// @ingroup another_binding_api
        /// @brief static property binding
        template<typename Data>
        class_ &add_static_property(const char *name, Data data) {
            this->addStaticField(name, data);
            return *this;
        }

    private:
        std::string name_;
    };

    template<typename EnumType>
    struct enum_
    {
        enum_(const std::string &name) : enum_scope_table_(scope(name).table()) {}
        enum_ &value(const char *name, EnumType data) {
            enum_scope_table_[name] = data;
            return *this;
        }

    private:
        LuaTable enum_scope_table_;
    };

    /// @ingroup another_binding_api
    /// @brief function binding
    template<typename F>
    void function(const char *name, F f) {
        LuaTable scope = detail::scope_stack::instance().current_scope();
        if (scope) { scope[name] = LuaWrapper::function(f); }
    }
    template<typename F>
    void function(const char *name, FunctionInvokerType<F> f) {
        LuaTable scope = detail::scope_stack::instance().current_scope();
        if (scope) { scope[name] = f; }
    }

    /// @ingroup another_binding_api
    /// @brief function binding
    template<typename F>
    void def(const char *name, F f) {
        LuaWrapper::function(name, f);
    }

    /// @ingroup another_binding_api
    /// @brief function binding
    template<typename T>
    void constant(const char *name, T v) {
        LuaTable scope = detail::scope_stack::instance().current_scope();
        if (scope) { scope[name] = v; }
    }

#pragma region PodBind

    namespace PodBind {

        class LuaException : public std::exception {
        private:
            lua_State *m_L;
            std::string m_what;

        public:
            //----------------------------------------------------------------------------
            LuaException(lua_State *L, int /*code*/) : m_L(L) { whatFromStack(); }

            LuaException(std::string what) : m_L(nullptr) { m_what = what; }

            //----------------------------------------------------------------------------

            LuaException(lua_State *L, char const *, char const *, long) : m_L(L) {
                whatFromStack();
            }

            //----------------------------------------------------------------------------

            ~LuaException() throw() {}

            //----------------------------------------------------------------------------

            char const *what() const throw() { return m_what.c_str(); }

            //============================================================================
            /**
      Throw an exception.

      This centralizes all the exceptions thrown, so that we can set
      breakpoints before the stack is unwound, or otherwise customize the
      behavior.
      */
            template<class Exception>
            static void Throw(Exception e) {
                throw e;
            }

            //----------------------------------------------------------------------------
            /**
      Wrapper for lua_pcall that throws.
      */
            static void pcall(lua_State *L, int nargs = 0, int nresults = 0, int msgh = 0) {
                int code = lua_pcall(L, nargs, nresults, msgh);

                if (code != LUA_OK) Throw(LuaException(L, code));
            }

            //----------------------------------------------------------------------------

        protected:
            void whatFromStack() {
                if (lua_gettop(m_L) > 0) {
                    char const *s = lua_tostring(m_L, -1);
                    m_what = s ? s : "";
                } else {
                    // stack is empty
                    m_what = "missing error";
                }
            }
        };

    };// namespace PodBind

    template<bool cond, typename U>
    using enable_if_t = typename std::enable_if<cond, U>::type;

    namespace PodBind {

        // traits

        // constructor
        template<typename T>
        using create_t = decltype(std::declval<T>().create(std::declval<lua_State *>()));
        template<typename T>
        using has_create = detect<T, create_t>;

        // members
        template<typename T>
        using members_t = decltype(std::declval<T>().members());
        template<typename T>
        using has_members = detect<T, members_t>;

        // properties
        template<typename T>
        using properties_t = decltype(std::declval<T>().properties());
        template<typename T>
        using has_properties = detect<T, properties_t>;

        // any other stuff you might want to add to the metatable.
        template<typename T>
        using extras_t = decltype(std::declval<T>().setExtraMeta(std::declval<lua_State *>()));
        template<typename T>
        using has_extras = detect<T, extras_t>;

        // helper struct
        struct bind_properties
        {
            const char *name;
            lua_CFunction getter;
            lua_CFunction setter;
        };

        // Called when Lua object is indexed: obj[ndx]
        int LuaBindingIndex(lua_State *L);
        // Called whe Lua object index is assigned: obj[ndx] = blah
        int LuaBindingNewIndex(lua_State *L);

        // Gets the table of extra values assigned to an instance.
        int LuaBindGetExtraValuesTable(lua_State *L, int index);

        void LuaBindingSetProperties(lua_State *L, bind_properties *properties);

        // If the object at 'index' is a userdata with a metatable containing a __upcast
        // function, then replaces the userdata at 'index' in the stack with the result
        // of calling __upcast.
        // Otherwise the object at index is replaced with nil.
        int LuaBindingUpCast(lua_State *L, int index);

        // Check the number of arguments are as expected.
        // Throw an error if not.
        void CheckArgCount(lua_State *L, int expected);

        // B - the binding class / struct
        // T - the class you are binding to Lua.

        // Shared pointer version
        // Use this for classes that need to be shared between C++ and Lua,
        // or are expensive to copy. Think of it as like "by Reference".
        template<class B, class T>
        struct Binding
        {

            // Push the object on to the Lua stack
            static void push(lua_State *L, const std::shared_ptr<T> &sp) {

                if (sp == nullptr) {
                    lua_pushnil(L);
                    return;
                }

                void *ud = lua_newuserdata(L, sizeof(std::shared_ptr<T>));

                new (ud) std::shared_ptr<T>(sp);

                luaL_setmetatable(L, B::class_name);
            }

            static void setMembers(lua_State *L, std::true_type) {
                luaL_setfuncs(L, B::members(), 0);
            }

            static void setMembers(lua_State *L, std::false_type) {
                // Nada.
            }

            static void setProperties(lua_State *L, std::true_type) {
                bind_properties *props = B::properties();
                LuaBindingSetProperties(L, props);
            }

            static void setProperties(lua_State *L, std::false_type) {
                // Nada.
            }

            static void setExtras(lua_State *L, std::true_type) { B::setExtraMeta(L); }

            static void setExtras(lua_State *, std::false_type) {
                // Nada.
            }

            static int construct(lua_State *L, std::true_type) {
                // Remove table from stack.
                lua_remove(L, 1);
                // Call create.
                return B::create(L);
            }

            static int construct(lua_State *L, std::false_type) {
                return luaL_error(L, "Can not create an instance of %s.", B::class_name);
            }

            static int construct(lua_State *L) {
                has_create<B> createTrait;
                return construct(L, createTrait);
            }

            static int pairs(lua_State *L) {
                lua_getglobal(L, "pairs");
                luaL_getmetatable(L, B::class_name);
                lua_pcall(L, 1, LUA_MULTRET, 0);
                return 3;
            }

            // Create metatable and register Lua constructor
            static void register_class(lua_State *L) {
                has_members<B> membersTrait;
                has_properties<B> propTrait;
                has_extras<B> extrasTrait;

                lua_newtable(L);// Class access
                lua_newtable(L);// Class access metatable

                luaL_newmetatable(L, B::class_name);
                setMembers(L, membersTrait);
                lua_pushcfunction(L, LuaBindingIndex);
                lua_setfield(L, -2, "__index");
                lua_pushcfunction(L, LuaBindingNewIndex);
                lua_setfield(L, -2, "__newindex");
                lua_pushcfunction(L, destroy);
                lua_setfield(L, -2, "__gc");
                lua_pushcfunction(L, close);
                lua_setfield(L, -2, "__close");
                lua_newtable(L);// __properties
                setProperties(L, propTrait);
                lua_setfield(L, -2, "__properties");
                setExtras(L, extrasTrait);

                lua_setfield(L, -2, "__index");// Set metatable as index table.

                lua_pushcfunction(L, construct);
                lua_setfield(L, -2, "__call");

                lua_pushcfunction(L, pairs);
                lua_setfield(L, -2, "__pairs");

                lua_setmetatable(L, -2);
                lua_setglobal(L, B::class_name);
            }

            // Called when Lua object is garbage collected.
            static int destroy(lua_State *L) {
                void *ud = luaL_checkudata(L, 1, B::class_name);

                auto sp = static_cast<std::shared_ptr<T> *>(ud);

                // Explicitly called, as this was 'placement new'd
                sp->~shared_ptr();

                return 0;
            }

            // Called when Lua object goes out of scope with the <close> annotation
            static int close(lua_State *L) {
                void *ud = luaL_checkudata(L, 1, B::class_name);

                auto sp = static_cast<std::shared_ptr<T> *>(ud);

                sp->reset();

                return 0;
            }

            // Grab object shared pointer from the Lua stack
            static const std::shared_ptr<T> &fromStack(lua_State *L, int index) {
                void *ud = luaL_checkudata(L, index, B::class_name);

                auto sp = static_cast<std::shared_ptr<T> *>(ud);

                return *sp;
            }

            static const std::shared_ptr<T> &fromStackThrow(lua_State *L, int index) {
                void *ud = luaL_testudata(L, index, B::class_name);

                if (ud == nullptr) throw LuaException("Unexpected item on Lua stack.");

                auto sp = static_cast<std::shared_ptr<T> *>(ud);

                return *sp;
            }
        };

        // Plain Old Data POD version.
        // Use this for simpler classes/structures where coping is fairly cheap, and
        // C++ and Lua do not need to operate on the same instance.
        // Think of this as "by Value"
        template<class B, class T>
        struct PODBinding
        {

            // Push the object on to the Lua stack
            static void push(lua_State *L, const T &value) {
                void *ud = lua_newuserdata(L, sizeof(T));

                new (ud) T(value);

                luaL_setmetatable(L, B::class_name);
            }

            static void setMembers(lua_State *L, std::true_type) {
                luaL_setfuncs(L, B::members(), 0);
            }

            static void setMembers(lua_State *, std::false_type) {
                // Nada.
            }

            static void setProperties(lua_State *L, std::true_type) {
                bind_properties *props = B::properties();
                LuaBindingSetProperties(L, props);
            }

            static void setProperties(lua_State *, std::false_type) {
                // Nada.
            }

            static void setExtras(lua_State *L, std::true_type) { B::setExtraMeta(L); }

            static void setExtras(lua_State *, std::false_type) {
                // Nada.
            }

            static int construct(lua_State *L, std::true_type) {
                // Remove table from stack.
                lua_remove(L, 1);
                // Call create.
                return B::create(L);
            }

            static int construct(lua_State *L, std::false_type) {
                return luaL_error(L, "Can not create an instance of %s.", B::class_name);
            }

            static int construct(lua_State *L) {
                has_create<B> createTrait;
                return construct(L, createTrait);
            }

            static int pairs(lua_State *L) {
                lua_getglobal(L, "pairs");
                luaL_getmetatable(L, B::class_name);
                lua_pcall(L, 1, LUA_MULTRET, 0);
                return 3;
            }

            // Create metatable and register Lua constructor
            static void register_class(lua_State *L) {
                has_members<B> membersTrait;
                has_properties<B> propTrait;
                has_extras<B> extrasTrait;

                lua_newtable(L);// Class access
                lua_newtable(L);// Class access metatable

                luaL_newmetatable(L, B::class_name);
                setMembers(L, membersTrait);
                lua_pushcfunction(L, LuaBindingIndex);
                lua_setfield(L, -2, "__index");
                lua_pushcfunction(L, LuaBindingNewIndex);
                lua_setfield(L, -2, "__newindex");
                //lua_pushcfunction( L, destroy );  -- if you need to destruct a POD
                //lua_setfield( L, -2, "__gc" );    -- set __gc to destory in the members.
                lua_newtable(L);// __properties
                setProperties(L, propTrait);
                lua_setfield(L, -2, "__properties");
                setExtras(L, extrasTrait);

                lua_setfield(L, -2, "__index");

                lua_pushcfunction(L, construct);
                lua_setfield(L, -2, "__call");

                lua_pushcfunction(L, pairs);
                lua_setfield(L, -2, "__pairs");

                lua_setmetatable(L, -2);
                lua_setglobal(L, B::class_name);
            }

            // This is still here should a POD really need
            // destructing. Shouldn't be a common case.
            static int destroy(lua_State *L) {
                void *ud = luaL_checkudata(L, 1, B::class_name);

                auto p = static_cast<T *>(ud);

                // Explicitly called, as this was 'placement new'd
                p->~T();

                return 0;
            }

            // Grab object pointer from the Lua stack
            static T &fromStack(lua_State *L, int index) {
                void *ud = luaL_checkudata(L, index, B::class_name);

                auto p = static_cast<T *>(ud);

                return *p;
            }

            static T &fromStackThrow(lua_State *L, int index) {
                void *ud = luaL_testudata(L, index, B::class_name);

                if (ud == nullptr) throw LuaException("Unexpected item on Lua stack.");

                auto p = static_cast<T *>(ud);

                return *p;
            }
        };

    };// namespace PodBind

#include <string>
#include <tuple>// For std::ignore

    namespace PodBind {

        template<typename T>
        struct LuaStack;

        //------------------------------------------------------------------------------
        /**
  Push an object onto the Lua stack.
  */
        template<class T>
        inline void lua_push(lua_State *L, T t) {
            LuaStack<T>::push(L, t);
        }

        //------------------------------------------------------------------------------
        /**
  Receive the lua_State* as an argument.
  */
        template<>
        struct LuaStack<lua_State *>
        {
            static lua_State *get(lua_State *L, int) { return L; }
        };

        //------------------------------------------------------------------------------
        /**
  Push a lua_CFunction.
  */
        template<>
        struct LuaStack<lua_CFunction>
        {
            static void push(lua_State *L, lua_CFunction f) { lua_pushcfunction(L, f); }

            static lua_CFunction get(lua_State *L, int index) { return lua_tocfunction(L, index); }
        };

        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `int`.
  */
        template<>
        struct LuaStack<int>
        {
            static inline void push(lua_State *L, int value) {
                lua_pushinteger(L, static_cast<lua_Integer>(value));
            }

            static inline int get(lua_State *L, int index) {
                return static_cast<int>(luaL_checkinteger(L, index));
            }
        };

        template<>
        struct LuaStack<int const &>
        {
            static inline void push(lua_State *L, int value) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            }

            static inline int get(lua_State *L, int index) {
                return static_cast<int>(luaL_checknumber(L, index));
            }
        };
        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `unsigned int`.
  */
        template<>
        struct LuaStack<unsigned int>
        {
            static inline void push(lua_State *L, unsigned int value) {
                lua_pushinteger(L, static_cast<lua_Integer>(value));
            }

            static inline unsigned int get(lua_State *L, int index) {
                return static_cast<unsigned int>(luaL_checkinteger(L, index));
            }
        };

        template<>
        struct LuaStack<unsigned int const &>
        {
            static inline void push(lua_State *L, unsigned int value) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            }

            static inline unsigned int get(lua_State *L, int index) {
                return static_cast<unsigned int>(luaL_checknumber(L, index));
            }
        };

        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `unsigned char`.
  */
        template<>
        struct LuaStack<unsigned char>
        {
            static inline void push(lua_State *L, unsigned char value) {
                lua_pushinteger(L, static_cast<lua_Integer>(value));
            }

            static inline unsigned char get(lua_State *L, int index) {
                return static_cast<unsigned char>(luaL_checkinteger(L, index));
            }
        };

        template<>
        struct LuaStack<unsigned char const &>
        {
            static inline void push(lua_State *L, unsigned char value) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            }

            static inline unsigned char get(lua_State *L, int index) {
                return static_cast<unsigned char>(luaL_checknumber(L, index));
            }
        };

        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `short`.
  */
        template<>
        struct LuaStack<short>
        {
            static inline void push(lua_State *L, short value) {
                lua_pushinteger(L, static_cast<lua_Integer>(value));
            }

            static inline short get(lua_State *L, int index) {
                return static_cast<short>(luaL_checkinteger(L, index));
            }
        };

        template<>
        struct LuaStack<short const &>
        {
            static inline void push(lua_State *L, short value) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            }

            static inline short get(lua_State *L, int index) {
                return static_cast<short>(luaL_checknumber(L, index));
            }
        };

        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `unsigned short`.
  */
        template<>
        struct LuaStack<unsigned short>
        {
            static inline void push(lua_State *L, unsigned short value) {
                lua_pushinteger(L, static_cast<lua_Integer>(value));
            }

            static inline unsigned short get(lua_State *L, int index) {
                return static_cast<unsigned short>(luaL_checkinteger(L, index));
            }
        };

        template<>
        struct LuaStack<unsigned short const &>
        {
            static inline void push(lua_State *L, unsigned short value) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            }

            static inline unsigned short get(lua_State *L, int index) {
                return static_cast<unsigned short>(luaL_checknumber(L, index));
            }
        };

        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `long`.
  */
        template<>
        struct LuaStack<long>
        {
            static inline void push(lua_State *L, long value) {
                lua_pushinteger(L, static_cast<lua_Integer>(value));
            }

            static inline long get(lua_State *L, int index) {
                return static_cast<long>(luaL_checkinteger(L, index));
            }
        };

        template<>
        struct LuaStack<long const &>
        {
            static inline void push(lua_State *L, long value) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            }

            static inline long get(lua_State *L, int index) {
                return static_cast<long>(luaL_checknumber(L, index));
            }
        };

        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `unsigned long`.
  */
        template<>
        struct LuaStack<unsigned long>
        {
            static inline void push(lua_State *L, unsigned long value) {
                lua_pushinteger(L, static_cast<lua_Integer>(value));
            }

            static inline unsigned long get(lua_State *L, int index) {
                return static_cast<unsigned long>(luaL_checkinteger(L, index));
            }
        };

        template<>
        struct LuaStack<unsigned long const &>
        {
            static inline void push(lua_State *L, unsigned long value) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            }

            static inline unsigned long get(lua_State *L, int index) {
                return static_cast<unsigned long>(luaL_checknumber(L, index));
            }
        };

        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `float`.
  */
        template<>
        struct LuaStack<float>
        {
            static inline void push(lua_State *L, float value) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            }

            static inline float get(lua_State *L, int index) {
                return static_cast<float>(luaL_checknumber(L, index));
            }
        };

        template<>
        struct LuaStack<float const &>
        {
            static inline void push(lua_State *L, float value) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            }

            static inline float get(lua_State *L, int index) {
                return static_cast<float>(luaL_checknumber(L, index));
            }
        };

        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `double`.
  */
        template<>
        struct LuaStack<double>
        {
            static inline void push(lua_State *L, double value) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            }

            static inline double get(lua_State *L, int index) {
                return static_cast<double>(luaL_checknumber(L, index));
            }
        };

        template<>
        struct LuaStack<double const &>
        {
            static inline void push(lua_State *L, double value) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            }

            static inline double get(lua_State *L, int index) {
                return static_cast<double>(luaL_checknumber(L, index));
            }
        };

        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `bool`.
  */
        template<>
        struct LuaStack<bool>
        {
            static inline void push(lua_State *L, bool value) { lua_pushboolean(L, value ? 1 : 0); }

            static inline bool get(lua_State *L, int index) {
                return lua_toboolean(L, index) ? true : false;
            }
        };

        template<>
        struct LuaStack<bool const &>
        {
            static inline void push(lua_State *L, bool value) { lua_pushboolean(L, value ? 1 : 0); }

            static inline bool get(lua_State *L, int index) {
                return lua_toboolean(L, index) ? true : false;
            }
        };

        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `char`.
  */
        template<>
        struct LuaStack<char>
        {
            static inline void push(lua_State *L, char value) {
                char str[2] = {value, 0};
                lua_pushstring(L, str);
            }

            static inline char get(lua_State *L, int index) {
                return luaL_checkstring(L, index)[0];
            }
        };

        template<>
        struct LuaStack<char const &>
        {
            static inline void push(lua_State *L, char value) {
                char str[2] = {value, 0};
                lua_pushstring(L, str);
            }

            static inline char get(lua_State *L, int index) {
                return luaL_checkstring(L, index)[0];
            }
        };

        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `float`.
  */
        template<>
        struct LuaStack<char const *>
        {
            static inline void push(lua_State *L, char const *str) {
                if (str != 0) lua_pushstring(L, str);
                else
                    lua_pushnil(L);
            }

            static inline char const *get(lua_State *L, int index) {
                return lua_isnil(L, index) ? 0 : luaL_checkstring(L, index);
            }
        };

        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `std::string`.
  */
        template<>
        struct LuaStack<std::string>
        {
            static inline void push(lua_State *L, std::string const &str) {
                lua_pushlstring(L, str.c_str(), str.size());
            }

            static inline std::string get(lua_State *L, int index) {
                size_t len;
                const char *str = luaL_checklstring(L, index, &len);
                return std::string(str, len);
            }
        };

        //------------------------------------------------------------------------------
        /**
  LuaStack specialization for `std::string const&`.
  */
        template<>
        struct LuaStack<std::string const &>
        {
            static inline void push(lua_State *L, std::string const &str) {
                lua_pushlstring(L, str.c_str(), str.size());
            }

            static inline std::string get(lua_State *L, int index) {
                size_t len;
                const char *str = luaL_checklstring(L, index, &len);
                return std::string(str, len);
            }
        };

    };// namespace PodBind

    namespace PodBind {

        struct LuaNil
        {
        };

        template<>
        struct LuaStack<LuaNil>
        {
            static inline void push(lua_State *L, LuaNil const &nil) { lua_pushnil(L); }
        };

        class LuaRef;

        class LuaRefBase {
        protected:
            lua_State *m_L;
            int m_ref;

            class StackPopper {
                lua_State *m_L;
                int m_count;

            public:
                StackPopper(lua_State *L, int count = 1) : m_L(L), m_count(count) {}
                ~StackPopper() { lua_pop(m_L, m_count); }
            };

            struct FromStack
            {
            };

            // These constructors as destructor are protected as this
            // class should not be used directly.

            LuaRefBase(lua_State *L, FromStack) : m_L(L) {
                m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
            }

            LuaRefBase(lua_State *L, int ref) : m_L(L), m_ref(ref) {}

            ~LuaRefBase() { luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref); }

        public:
            virtual void push() const { lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_ref); }

            std::string tostring() const {
                lua_getglobal(m_L, "tostring");
                push();
                lua_call(m_L, 1, 1);
                const char *str = lua_tostring(m_L, 1);
                lua_pop(m_L, 1);
                return std::string(str);
            }

            int type() const {
                int result;
                push();
                result = lua_type(m_L, -1);
                lua_pop(m_L, 1);
                return result;
            }

            inline bool isNil() const { return type() == LUA_TNIL; }
            inline bool isNumber() const { return type() == LUA_TNUMBER; }
            inline bool isString() const { return type() == LUA_TSTRING; }
            inline bool isTable() const { return type() == LUA_TTABLE; }
            inline bool isFunction() const { return type() == LUA_TFUNCTION; }
            inline bool isUserdata() const { return type() == LUA_TUSERDATA; }
            inline bool isThread() const { return type() == LUA_TTHREAD; }
            inline bool isLightUserdata() const { return type() == LUA_TLIGHTUSERDATA; }
            inline bool isBool() const { return type() == LUA_TBOOLEAN; }

            template<typename... Args>
            inline LuaRef const operator()(Args... args) const;

            template<typename... Args>
            inline void call(int ret, Args... args) const;

            template<typename T>
            void append(T v) const {
                push();
                LuaStack<T>::push(m_L, v);
                luaL_ref(m_L, -2);
                lua_pop(m_L, 1);
            }

            template<typename T>
            T cast() {
                push();
                return LuaStack<T>::get(m_L, -1);
            }

            template<typename T>
            operator T() {
                return cast<T>();
            }
        };

        template<typename K>
        class LuaTableElement : public LuaRefBase {
            friend class LuaRef;

        private:
            K m_key;

            // This constructor has to be public, so that the operator[]
            // with a differing template type can call it.
            // I could not find a way to 'friend' it.
        public:
            // Expects on the Lua stack
            // 1 - The table
            LuaTableElement(lua_State *L, K key) : LuaRefBase(L, FromStack()), m_key(key) {}

            void push() const override {
                lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_ref);
                LuaStack<K>::push(m_L, m_key);
                lua_gettable(m_L, -2);
                lua_remove(m_L, -2);
            }

            // Assign a new value to this table/key.
            template<typename T>
            LuaTableElement &operator=(T v) {
                StackPopper p(m_L);
                lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_ref);
                LuaStack<K>::push(m_L, m_key);
                LuaStack<T>::push(m_L, v);
                lua_settable(m_L, -3);
                return *this;
            }

            template<typename NK>
            LuaTableElement<NK> operator[](NK key) const {
                push();
                return LuaTableElement<NK>(m_L, key);
            }
        };

        template<typename K>
        struct LuaStack<LuaTableElement<K>>
        {
            static inline void push(lua_State *L, LuaTableElement<K> const &e) { e.push(); }
        };

        class LuaRef : public LuaRefBase {
            friend LuaRefBase;

        private:
            LuaRef(lua_State *L, FromStack fs) : LuaRefBase(L, fs) {}

        public:
            LuaRef(lua_State *L) : LuaRefBase(L, LUA_REFNIL) {}

            LuaRef(lua_State *L, const std::string &global) : LuaRefBase(L, LUA_REFNIL) {
                lua_getglobal(m_L, global.c_str());
                m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
            }

            LuaRef(LuaRef const &other) : LuaRefBase(other.m_L, LUA_REFNIL) {
                other.push();
                m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
            }

            LuaRef(LuaRef &&other) noexcept : LuaRefBase(other.m_L, other.m_ref) {
                other.m_ref = LUA_REFNIL;
            }

            LuaRef &operator=(LuaRef &&other) noexcept {
                if (this == &other) return *this;

                std::swap(m_L, other.m_L);
                std::swap(m_ref, other.m_ref);

                return *this;
            }

            LuaRef &operator=(LuaRef const &other) {
                if (this == &other) return *this;
                luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);
                other.push();
                m_L = other.m_L;
                m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
                return *this;
            }

            template<typename K>
            LuaRef &operator=(LuaTableElement<K> &&other) noexcept {
                luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);
                other.push();
                m_L = other.m_L;
                m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
                return *this;
            }

            template<typename K>
            LuaRef &operator=(LuaTableElement<K> const &other) {
                luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);
                other.push();
                m_L = other.m_L;
                m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
                return *this;
            }

            template<typename K>
            LuaTableElement<K> operator[](K key) const {
                push();
                return LuaTableElement<K>(m_L, key);
            }

            static LuaRef fromStack(lua_State *L, int index = -1) {
                lua_pushvalue(L, index);
                return LuaRef(L, FromStack());
            }

            static LuaRef newTable(lua_State *L) {
                lua_newtable(L);
                return LuaRef(L, FromStack());
            }

            static LuaRef getGlobal(lua_State *L, char const *name) {
                lua_getglobal(L, name);
                return LuaRef(L, FromStack());
            }
        };

        template<>
        struct LuaStack<LuaRef>
        {
            static inline void push(lua_State *L, LuaRef const &r) { r.push(); }
        };

        template<>
        inline LuaRef const LuaRefBase::operator()() const {
            push();
            LuaException::pcall(m_L, 0, 1);
            return LuaRef(m_L, FromStack());
        }

        template<typename... Args>
        inline LuaRef const LuaRefBase::operator()(Args... args) const {
            const int n = sizeof...(Args);
            push();
            // Initializer expansion trick to call push for each arg.
            // https://stackoverflow.com/questions/25680461/variadic-template-pack-expansion
            int dummy[] = {0, ((void) LuaStack<Args>::push(m_L, std::forward<Args>(args)), 0)...};
            std::ignore = dummy;
            LuaException::pcall(m_L, n, 1);
            return LuaRef(m_L, FromStack());
        }

        template<>
        inline void LuaRefBase::call(int ret) const {
            push();
            LuaException::pcall(m_L, 0, ret);
            return;// Return values, if any, are left on the Lua stack.
        }

        template<typename... Args>
        inline void LuaRefBase::call(int ret, Args... args) const {
            const int n = sizeof...(Args);
            push();
            // Initializer expansion trick to call push for each arg.
            // https://stackoverflow.com/questions/25680461/variadic-template-pack-expansion
            int dummy[] = {0, ((void) LuaStack<Args>::push(m_L, std::forward<Args>(args)), 0)...};
            std::ignore = dummy;
            LuaException::pcall(m_L, n, ret);
            return;// Return values, if any, are left on the Lua stack.
        }

        template<>
        inline LuaRef LuaRefBase::cast() {
            push();
            return LuaRef(m_L, FromStack());
        }

    };// namespace PodBind

#pragma endregion PodBind

#pragma region LuaA

        /*
** Open / Close
*/

#define LUAA_REGISTRYPREFIX "lautoc_"

    void luaA_open(lua_State *L);
    void luaA_close(lua_State *L);

    /*
** Types
*/

#define luaA_type(L, type) luaA_type_add(L, #type, sizeof(type))

    enum {
        LUAA_INVALID_TYPE = -1
    };

    typedef lua_Integer luaA_Type;
    typedef int (*luaA_Pushfunc)(lua_State *, luaA_Type, const void *);
    typedef void (*luaA_Tofunc)(lua_State *, luaA_Type, void *, int);

    luaA_Type luaA_type_add(lua_State *L, const char *type, size_t size);
    luaA_Type luaA_type_find(lua_State *L, const char *type);

    const char *luaA_typename(lua_State *L, luaA_Type id);
    size_t luaA_typesize(lua_State *L, luaA_Type id);

    /*
** Stack
*/

#define luaA_push(L, type, c_in) luaA_push_type(L, luaA_type(L, type), c_in)
#define luaA_to(L, type, c_out, index) luaA_to_type(L, luaA_type(L, type), c_out, index)

#define luaA_conversion(L, type, push_func, to_func)                                               \
    luaA_conversion_type(L, luaA_type(L, type), push_func, to_func);
#define luaA_conversion_push(L, type, func) luaA_conversion_push_type(L, luaA_type(L, type), func)
#define luaA_conversion_to(L, type, func) luaA_conversion_to_type(L, luaA_type(L, type), func)

#define luaA_conversion_registered(L, type) luaA_conversion_registered_type(L, luaA_type(L, type));
#define luaA_conversion_push_registered(L, type)                                                   \
    luaA_conversion_push_registered_typ(L, luaA_type(L, type));
#define luaA_conversion_to_registered(L, type)                                                     \
    luaA_conversion_to_registered_type(L, luaA_type(L, type));

    int luaA_push_type(lua_State *L, luaA_Type type, const void *c_in);
    void luaA_to_type(lua_State *L, luaA_Type type, void *c_out, int index);

    void luaA_conversion_type(lua_State *L, luaA_Type type_id, luaA_Pushfunc push_func,
                              luaA_Tofunc to_func);
    void luaA_conversion_push_type(lua_State *L, luaA_Type type_id, luaA_Pushfunc func);
    void luaA_conversion_to_type(lua_State *L, luaA_Type type_id, luaA_Tofunc func);

    bool luaA_conversion_registered_type(lua_State *L, luaA_Type type);
    bool luaA_conversion_push_registered_type(lua_State *L, luaA_Type type);
    bool luaA_conversion_to_registered_type(lua_State *L, luaA_Type type);

    int luaA_push_bool(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_char(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_signed_char(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_unsigned_char(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_short(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_unsigned_short(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_int(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_unsigned_int(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_long(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_unsigned_long(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_long_long(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_unsigned_long_long(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_float(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_double(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_long_double(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_char_ptr(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_const_char_ptr(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_void_ptr(lua_State *L, luaA_Type, const void *c_in);
    int luaA_push_void(lua_State *L, luaA_Type, const void *c_in);

    void luaA_to_bool(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_char(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_signed_char(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_unsigned_char(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_short(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_unsigned_short(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_int(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_unsigned_int(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_long(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_unsigned_long(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_long_long(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_unsigned_long_long(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_float(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_double(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_long_double(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_char_ptr(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_const_char_ptr(lua_State *L, luaA_Type, void *c_out, int index);
    void luaA_to_void_ptr(lua_State *L, luaA_Type, void *c_out, int index);

/*
** Structs
*/
#define LUAA_INVALID_MEMBER_NAME NULL

#define luaA_struct(L, type) luaA_struct_type(L, luaA_type(L, type))
#define luaA_struct_member(L, type, member, member_type)                                           \
    luaA_struct_member_type(L, luaA_type(L, type), #member, luaA_type(L, member_type),             \
                            offsetof(type, member))

#define luaA_struct_push(L, type, c_in) luaA_struct_push_type(L, luaA_type(L, type), c_in)
#define luaA_struct_push_member(L, type, member, c_in)                                             \
    luaA_struct_push_member_offset_type(L, luaA_type(L, type), offsetof(type, member), c_in)
#define luaA_struct_push_member_name(L, type, member, c_in)                                        \
    luaA_struct_push_member_name_type(L, luaA_type(L, type), member, c_in)

#define luaA_struct_to(L, type, c_out, index)                                                      \
    luaA_struct_to_type(L, luaA_type(L, type), c_out, index)
#define luaA_struct_to_member(L, type, member, c_out, index)                                       \
    luaA_struct_to_member_offset_type(L, luaA_type(L, type), offsetof(type, member), c_out, index)
#define luaA_struct_to_member_name(L, type, member, c_out, index)                                  \
    luaA_struct_to_member_name_type(L, luaA_type(L, type), member, c_out, index)

#define luaA_struct_has_member(L, type, member)                                                    \
    luaA_struct_has_member_offset_type(L, luaA_type(L, type), offsetof(type, member))
#define luaA_struct_has_member_name(L, type, member)                                               \
    luaA_struct_has_member_name_type(L, luaA_type(L, type), member)

#define luaA_struct_typeof_member(L, type, member)                                                 \
    luaA_struct_typeof_member_offset_type(L, luaA_type(L, type), offsetof(type, member))
#define luaA_struct_typeof_member_name(L, type, member)                                            \
    luaA_struct_typeof_member_name_type(L, luaA_type(L, type), member)

#define luaA_struct_registered(L, type) luaA_struct_registered_type(L, luaA_type(L, type))
#define luaA_struct_next_member_name(L, type, member)                                              \
    luaA_struct_next_member_name_type(L, luaA_type(L, type), member)

    void luaA_struct_type(lua_State *L, luaA_Type type);
    void luaA_struct_member_type(lua_State *L, luaA_Type type, const char *member,
                                 luaA_Type member_type, size_t offset);

    int luaA_struct_push_type(lua_State *L, luaA_Type type, const void *c_in);
    int luaA_struct_push_member_offset_type(lua_State *L, luaA_Type type, size_t offset,
                                            const void *c_in);
    int luaA_struct_push_member_name_type(lua_State *L, luaA_Type type, const char *member,
                                          const void *c_in);

    void luaA_struct_to_type(lua_State *L, luaA_Type type, void *c_out, int index);
    void luaA_struct_to_member_offset_type(lua_State *L, luaA_Type type, size_t offset, void *c_out,
                                           int index);
    void luaA_struct_to_member_name_type(lua_State *L, luaA_Type type, const char *member,
                                         void *c_out, int index);

    bool luaA_struct_has_member_offset_type(lua_State *L, luaA_Type type, size_t offset);
    bool luaA_struct_has_member_name_type(lua_State *L, luaA_Type type, const char *member);

    luaA_Type luaA_struct_typeof_member_offset_type(lua_State *L, luaA_Type type, size_t offset);
    luaA_Type luaA_struct_typeof_member_name_type(lua_State *L, luaA_Type type, const char *member);

    bool luaA_struct_registered_type(lua_State *L, luaA_Type type);

    const char *luaA_struct_next_member_name_type(lua_State *L, luaA_Type type, const char *member);

    /*
** Enums
*/

#define luaA_enum(L, type) luaA_enum_type(L, luaA_type(L, type), sizeof(type))
#define luaA_enum_value(L, type, value)                                                            \
    luaA_enum_value_type(L, luaA_type(L, type), (const type[]){value}, #value);
#define luaA_enum_value_name(L, type, value, name)                                                 \
    luaA_enum_value_type(L, luaA_type(L, type), (const type[]){value}, name);

#define luaA_enum_push(L, type, c_in) luaA_enum_push_type(L, luaA_type(L, type), c_in)
#define luaA_enum_to(L, type, c_out, index) luaA_enum_to_type(L, luaA_type(L, type), c_out, index)

#define luaA_enum_has_value(L, type, value)                                                        \
    luaA_enum_has_value_type(L, luaA_type(L, type), (const type[]){value})
#define luaA_enum_has_name(L, type, name) luaA_enum_has_name_type(L, luaA_type(L, type), name)

#define luaA_enum_registered(L, type) luaA_enum_registered_type(L, luaA_type(L, type))
#define luaA_enum_next_value_name(L, type, member)                                                 \
    luaA_enum_next_value_name_type(L, luaA_type(L, type), member)

    void luaA_enum_type(lua_State *L, luaA_Type type, size_t size);
    void luaA_enum_value_type(lua_State *L, luaA_Type type, const void *value, const char *name);

    int luaA_enum_push_type(lua_State *L, luaA_Type type, const void *c_in);
    void luaA_enum_to_type(lua_State *L, luaA_Type type, void *c_out, int index);

    bool luaA_enum_has_value_type(lua_State *L, luaA_Type type, const void *value);
    bool luaA_enum_has_name_type(lua_State *L, luaA_Type type, const char *name);

    bool luaA_enum_registered_type(lua_State *L, luaA_Type type);
    const char *luaA_enum_next_value_name_type(lua_State *L, luaA_Type type, const char *member);

    /*
** Functions
*/

#define LUAA_EVAL(...) __VA_ARGS__

/* Join Three Strings */
#define LUAA_JOIN2(X, Y) X##Y
#define LUAA_JOIN3(X, Y, Z) X##Y##Z

/* workaround for MSVC VA_ARGS expansion */
#define LUAA_APPLY(FUNC, ARGS) LUAA_EVAL(FUNC ARGS)

/* Argument Counter */
#define LUAA_COUNT(...) LUAA_COUNT_COLLECT(_, ##__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define LUAA_COUNT_COLLECT(...) LUAA_COUNT_SHIFT(__VA_ARGS__)
#define LUAA_COUNT_SHIFT(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _N, ...) _N

/* Detect Void */
#define LUAA_VOID(X) LUAA_JOIN2(LUAA_VOID_, X)
#define LUAA_VOID_void
#define LUAA_CHECK_N(X, N, ...) N
#define LUAA_CHECK(...) LUAA_CHECK_N(__VA_ARGS__, , )
#define LUAA_SUFFIX(X) LUAA_SUFFIX_CHECK(LUAA_VOID(X))
#define LUAA_SUFFIX_CHECK(X) LUAA_CHECK(LUAA_JOIN2(LUAA_SUFFIX_, X))
#define LUAA_SUFFIX_ ~, _void,

/* Declaration and Register Macros */
#define LUAA_DECLARE(func, ret_t, count, suffix, ...)                                              \
    LUAA_APPLY(LUAA_JOIN3(luaA_function_declare, count, suffix), (func, ret_t, ##__VA_ARGS__))
//#define LUAA_DECLARE(func, ret_t, count, suffix, ...) LUAA_APPLY(LUAA_JOIN3(luaA_function_declare, count, suffix), (func, ret_t, ##__VA_ARGS__))
#define LUAA_REGISTER(L, func, ret_t, count, ...)                                                  \
    LUAA_APPLY(LUAA_JOIN2(luaA_function_register, count), (L, func, ret_t, ##__VA_ARGS__))

/*
** MSVC does not allow nested functions
** so function is wrapped in nested struct
*/
#ifdef _MSC_VER

#define luaA_function_declare0(func, ret_t)                                                        \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) { *(ret_t *) out = func(); };             \
    }

#define luaA_function_declare0_void(func, ret_t)                                                   \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) { func(); };                              \
    }

#define luaA_function_declare1(func, ret_t, arg0_t)                                                \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            *(ret_t *) out = func(a0);                                                             \
        };                                                                                         \
    }

#define luaA_function_declare1_void(func, ret_t, arg0_t)                                           \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            func(a0);                                                                              \
        };                                                                                         \
    }

#define luaA_function_declare2(func, ret_t, arg0_t, arg1_t)                                        \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            *(ret_t *) out = func(a0, a1);                                                         \
        };                                                                                         \
    }

#define luaA_function_declare2_void(func, ret_t, arg0_t, arg1_t)                                   \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            func(a0, a1);                                                                          \
        };                                                                                         \
    }

#define luaA_function_declare3(func, ret_t, arg0_t, arg1_t, arg2_t)                                \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            *(ret_t *) out = func(a0, a1, a2);                                                     \
        };                                                                                         \
    }

#define luaA_function_declare3_void(func, ret_t, arg0_t, arg1_t, arg2_t)                           \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            func(a0, a1, a2);                                                                      \
        };                                                                                         \
    }

#define luaA_function_declare4(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t)                        \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            *(ret_t *) out = func(a0, a1, a2, a3);                                                 \
        };                                                                                         \
    }

#define luaA_function_declare4_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t)                   \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            func(a0, a1, a2, a3);                                                                  \
        };                                                                                         \
    }

#define luaA_function_declare5(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t)                \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t));                                              \
            *(ret_t *) out = func(a0, a1, a2, a3, a4);                                             \
        };                                                                                         \
    }

#define luaA_function_declare5_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t)           \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t));                                              \
            func(a0, a1, a2, a3, a4);                                                              \
        };                                                                                         \
    }

#define luaA_function_declare6(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t)        \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t));                                              \
            arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t));                             \
            *(ret_t *) out = func(a0, a1, a2, a3, a4, a5);                                         \
        };                                                                                         \
    }

#define luaA_function_declare6_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t)   \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t));                                              \
            arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t));                             \
            func(a0, a1, a2, a3, a4, a5);                                                          \
        };                                                                                         \
    }

#define luaA_function_declare7(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,        \
                               arg6_t)                                                             \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t));                                              \
            arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t));                             \
            arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));            \
            *(ret_t *) out = func(a0, a1, a2, a3, a4, a5, a6);                                     \
        };                                                                                         \
    }

#define luaA_function_declare7_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,   \
                                    arg6_t)                                                        \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t));                                              \
            arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t));                             \
            arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));            \
            func(a0, a1, a2, a3, a4, a5, a6);                                                      \
        };                                                                                         \
    }

#define luaA_function_declare8(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,        \
                               arg6_t, arg7_t)                                                     \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t));                                              \
            arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t));                             \
            arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));            \
            arg7_t a7 = *(arg7_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +            \
                                     sizeof(arg6_t));                                              \
            *(ret_t *) out = func(a0, a1, a2, a3, a4, a5, a6, a7);                                 \
        };                                                                                         \
    }

#define luaA_function_declare8_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,   \
                                    arg6_t, arg7_t)                                                \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t));                                              \
            arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t));                             \
            arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));            \
            arg7_t a7 = *(arg7_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +            \
                                     sizeof(arg6_t));                                              \
            func(a0, a1, a2, a3, a4, a5, a6, a7);                                                  \
        };                                                                                         \
    }

#define luaA_function_declare9(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,        \
                               arg6_t, arg7_t, arg8_t)                                             \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t));                                              \
            arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t));                             \
            arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));            \
            arg7_t a7 = *(arg7_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +            \
                                     sizeof(arg6_t));                                              \
            arg8_t a8 = *(arg8_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +            \
                                     sizeof(arg6_t) + sizeof(arg7_t));                             \
            *(ret_t *) out = func(a0, a1, a2, a3, a4, a5, a6, a7, a8);                             \
        };                                                                                         \
    }

#define luaA_function_declare9_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,   \
                                    arg6_t, arg7_t, arg8_t)                                        \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t));                                              \
            arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t));                             \
            arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));            \
            arg7_t a7 = *(arg7_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +            \
                                     sizeof(arg6_t));                                              \
            arg8_t a8 = *(arg8_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +            \
                                     sizeof(arg6_t) + sizeof(arg7_t));                             \
            func(a0, a1, a2, a3, a4, a5, a6, a7, a8);                                              \
        };                                                                                         \
    }

#define luaA_function_declare10(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,       \
                                arg6_t, arg7_t, arg8_t, arg9_t)                                    \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t));                                              \
            arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t));                             \
            arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));            \
            arg7_t a7 = *(arg7_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +            \
                                     sizeof(arg6_t));                                              \
            arg8_t a8 = *(arg8_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +            \
                                     sizeof(arg6_t) + sizeof(arg7_t));                             \
            arg9_t a9 = *(arg9_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +            \
                                     sizeof(arg6_t) + sizeof(arg7_t) + sizeof(arg8_t));            \
            *(ret_t *) out = func(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);                         \
        };                                                                                         \
    }

#define luaA_function_declare10_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,  \
                                     arg6_t, arg7_t, arg8_t, arg9_t)                               \
    struct __luaA_wrap_##func                                                                      \
    {                                                                                              \
        static void __luaA_##func(char *out, char *args) {                                         \
            arg0_t a0 = *(arg0_t *) args;                                                          \
            arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                       \
            arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                      \
            arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));     \
            arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t));                                              \
            arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t));                             \
            arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));            \
            arg7_t a7 = *(arg7_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +            \
                                     sizeof(arg6_t));                                              \
            arg8_t a8 = *(arg8_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +            \
                                     sizeof(arg6_t) + sizeof(arg7_t));                             \
            arg9_t a9 = *(arg9_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +     \
                                     sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +            \
                                     sizeof(arg6_t) + sizeof(arg7_t) + sizeof(arg8_t));            \
            func(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);                                          \
        };                                                                                         \
    }

#define luaA_function_register0(L, func, ret_t)                                                    \
    luaA_function_register_type(L, func, (luaA_Func) __luaA_wrap_##func::__luaA_##func, #func,     \
                                luaA_type(L, ret_t), 0)

#define luaA_function_register1(L, func, ret_t, arg0_t)                                            \
    luaA_function_register_type(L, func, (luaA_Func) __luaA_wrap_##func::__luaA_##func, #func,     \
                                luaA_type(L, ret_t), 1, luaA_type(L, arg0_t))

#define luaA_function_register2(L, func, ret_t, arg0_t, arg1_t)                                    \
    luaA_function_register_type(L, func, (luaA_Func) __luaA_wrap_##func::__luaA_##func, #func,     \
                                luaA_type(L, ret_t), 2, luaA_type(L, arg0_t),                      \
                                luaA_type(L, arg1_t))

#define luaA_function_register3(L, func, ret_t, arg0_t, arg1_t, arg2_t)                            \
    luaA_function_register_type(L, func, (luaA_Func) __luaA_wrap_##func::__luaA_##func, #func,     \
                                luaA_type(L, ret_t), 3, luaA_type(L, arg0_t),                      \
                                luaA_type(L, arg1_t), luaA_type(L, arg2_t))

#define luaA_function_register4(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t)                    \
    luaA_function_register_type(L, func, (luaA_Func) __luaA_wrap_##func::__luaA_##func, #func,     \
                                luaA_type(L, ret_t), 4, luaA_type(L, arg0_t),                      \
                                luaA_type(L, arg1_t), luaA_type(L, arg2_t), luaA_type(L, arg3_t))

#define luaA_function_register5(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t)            \
    luaA_function_register_type(L, func, (luaA_Func) __luaA_wrap_##func::__luaA_##func, #func,     \
                                luaA_type(L, ret_t), 5, luaA_type(L, arg0_t),                      \
                                luaA_type(L, arg1_t), luaA_type(L, arg2_t), luaA_type(L, arg3_t),  \
                                luaA_type(L, arg4_t))

#define luaA_function_register6(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t)    \
    luaA_function_register_type(L, func, (luaA_Func) __luaA_wrap_##func::__luaA_##func, #func,     \
                                luaA_type(L, ret_t), 6, luaA_type(L, arg0_t),                      \
                                luaA_type(L, arg1_t), luaA_type(L, arg2_t), luaA_type(L, arg3_t),  \
                                luaA_type(L, arg4_t), luaA_type(L, arg5_t))

#define luaA_function_register7(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,    \
                                arg6_t)                                                            \
    luaA_function_register_type(L, func, (luaA_Func) __luaA_wrap_##func::__luaA_##func, #func,     \
                                luaA_type(L, ret_t), 7, luaA_type(L, arg0_t),                      \
                                luaA_type(L, arg1_t), luaA_type(L, arg2_t), luaA_type(L, arg3_t),  \
                                luaA_type(L, arg4_t), luaA_type(L, arg5_t), luaA_type(L, arg6_t))

#define luaA_function_register8(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,    \
                                arg6_t, arg7_t)                                                    \
    luaA_function_register_type(L, func, (luaA_Func) __luaA_wrap_##func::__luaA_##func, #func,     \
                                luaA_type(L, ret_t), 8, luaA_type(L, arg0_t),                      \
                                luaA_type(L, arg1_t), luaA_type(L, arg2_t), luaA_type(L, arg3_t),  \
                                luaA_type(L, arg4_t), luaA_type(L, arg5_t), luaA_type(L, arg6_t),  \
                                luaA_type(L, arg7_t))

#define luaA_function_register9(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,    \
                                arg6_t, arg7_t, arg8_t)                                            \
    luaA_function_register_type(L, func, (luaA_Func) __luaA_wrap_##func::__luaA_##func, #func,     \
                                luaA_type(L, ret_t), 9, luaA_type(L, arg0_t),                      \
                                luaA_type(L, arg1_t), luaA_type(L, arg2_t), luaA_type(L, arg3_t),  \
                                luaA_type(L, arg4_t), luaA_type(L, arg5_t), luaA_type(L, arg6_t),  \
                                luaA_type(L, arg7_t), luaA_type(L, arg8_t))

#define luaA_function_register10(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,   \
                                 arg6_t, arg7_t, arg8_t, arg9_t)                                   \
    luaA_function_register_type(L, func, (luaA_Func) __luaA_wrap_##func::__luaA_##func, #func,     \
                                luaA_type(L, ret_t), 10, luaA_type(L, arg0_t),                     \
                                luaA_type(L, arg1_t), luaA_type(L, arg2_t), luaA_type(L, arg3_t),  \
                                luaA_type(L, arg4_t), luaA_type(L, arg5_t), luaA_type(L, arg6_t),  \
                                luaA_type(L, arg7_t), luaA_type(L, arg8_t), luaA_type(L, arg9_t))

#else

#define luaA_function_declare0(func, ret_t)                                                        \
    void __luaA_##func(void *out, void *args) { *(ret_t *) out = func(); }

#define luaA_function_declare0_void(func, ret_t)                                                   \
    void __luaA_##func(void *out, void *args) { func(); }

#define luaA_function_declare1(func, ret_t, arg0_t)                                                \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        *(ret_t *) out = func(a0);                                                                 \
    }

#define luaA_function_declare1_void(func, ret_t, arg0_t)                                           \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        func(a0);                                                                                  \
    }

#define luaA_function_declare2(func, ret_t, arg0_t, arg1_t)                                        \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        *(ret_t *) out = func(a0, a1);                                                             \
    }

#define luaA_function_declare2_void(func, ret_t, arg0_t, arg1_t)                                   \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        func(a0, a1);                                                                              \
    }

#define luaA_function_declare3(func, ret_t, arg0_t, arg1_t, arg2_t)                                \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        *(ret_t *) out = func(a0, a1, a2);                                                         \
    }

#define luaA_function_declare3_void(func, ret_t, arg0_t, arg1_t, arg2_t)                           \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        func(a0, a1, a2);                                                                          \
    }

#define luaA_function_declare4(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t)                        \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        *(ret_t *) out = func(a0, a1, a2, a3);                                                     \
    }

#define luaA_function_declare4_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t)                   \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        func(a0, a1, a2, a3);                                                                      \
    }

#define luaA_function_declare5(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t)                \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t));                                                  \
        *(ret_t *) out = func(a0, a1, a2, a3, a4);                                                 \
    }

#define luaA_function_declare5_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t)           \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t));                                                  \
        func(a0, a1, a2, a3, a4);                                                                  \
    }

#define luaA_function_declare6(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t)        \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t));                                                  \
        arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t));                                 \
        *(ret_t *) out = func(a0, a1, a2, a3, a4, a5);                                             \
    }

#define luaA_function_declare6_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t)   \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t));                                                  \
        arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t));                                 \
        func(a0, a1, a2, a3, a4, a5);                                                              \
    }

#define luaA_function_declare7(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,        \
                               arg6_t)                                                             \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t));                                                  \
        arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t));                                 \
        arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));                \
        *(ret_t *) out = func(a0, a1, a2, a3, a4, a5, a6);                                         \
    }

#define luaA_function_declare7_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,   \
                                    arg6_t)                                                        \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t));                                                  \
        arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t));                                 \
        arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));                \
        func(a0, a1, a2, a3, a4, a5, a6);                                                          \
    }

#define luaA_function_declare8(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,        \
                               arg6_t, arg7_t)                                                     \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t));                                                  \
        arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t));                                 \
        arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));                \
        arg7_t a7 =                                                                                \
                *(arg7_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +             \
                             sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) + sizeof(arg6_t));   \
        *(ret_t *) out = func(a0, a1, a2, a3, a4, a5, a6, a7);                                     \
    }

#define luaA_function_declare8_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,   \
                                    arg6_t, arg7_t)                                                \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t));                                                  \
        arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t));                                 \
        arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));                \
        arg7_t a7 =                                                                                \
                *(arg7_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +             \
                             sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) + sizeof(arg6_t));   \
        func(a0, a1, a2, a3, a4, a5, a6, a7);                                                      \
    }

#define luaA_function_declare9(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,        \
                               arg6_t, arg7_t, arg8_t)                                             \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t));                                                  \
        arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t));                                 \
        arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));                \
        arg7_t a7 =                                                                                \
                *(arg7_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +             \
                             sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) + sizeof(arg6_t));   \
        arg8_t a8 = *(arg8_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +                \
                                 sizeof(arg6_t) + sizeof(arg7_t));                                 \
        *(ret_t *) out = func(a0, a1, a2, a3, a4, a5, a6, a7, a8);                                 \
    }

#define luaA_function_declare9_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,   \
                                    arg6_t, arg7_t, arg8_t)                                        \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t));                                                  \
        arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t));                                 \
        arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));                \
        arg7_t a7 =                                                                                \
                *(arg7_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +             \
                             sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) + sizeof(arg6_t));   \
        arg8_t a8 = *(arg8_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +                \
                                 sizeof(arg6_t) + sizeof(arg7_t));                                 \
        func(a0, a1, a2, a3, a4, a5, a6, a7, a8);                                                  \
    }

#define luaA_function_declare10(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,       \
                                arg6_t, arg7_t, arg8_t, arg9_t)                                    \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t));                                                  \
        arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t));                                 \
        arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));                \
        arg7_t a7 =                                                                                \
                *(arg7_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +             \
                             sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) + sizeof(arg6_t));   \
        arg8_t a8 = *(arg8_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +                \
                                 sizeof(arg6_t) + sizeof(arg7_t));                                 \
        arg9_t a9 = *(arg9_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +                \
                                 sizeof(arg6_t) + sizeof(arg7_t) + sizeof(arg8_t));                \
        *(ret_t *) out = func(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);                             \
    }

#define luaA_function_declare10_void(func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,  \
                                     arg6_t, arg7_t, arg8_t, arg9_t)                               \
    void __luaA_##func(void *out, void *args) {                                                    \
        arg0_t a0 = *(arg0_t *) args;                                                              \
        arg1_t a1 = *(arg1_t *) (args + sizeof(arg0_t));                                           \
        arg2_t a2 = *(arg2_t *) (args + sizeof(arg0_t) + sizeof(arg1_t));                          \
        arg3_t a3 = *(arg3_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t));         \
        arg4_t a4 = *(arg4_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t));                                                  \
        arg5_t a5 = *(arg5_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t));                                 \
        arg6_t a6 = *(arg6_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t));                \
        arg7_t a7 =                                                                                \
                *(arg7_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +             \
                             sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) + sizeof(arg6_t));   \
        arg8_t a8 = *(arg8_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +                \
                                 sizeof(arg6_t) + sizeof(arg7_t));                                 \
        arg9_t a9 = *(arg9_t *) (args + sizeof(arg0_t) + sizeof(arg1_t) + sizeof(arg2_t) +         \
                                 sizeof(arg3_t) + sizeof(arg4_t) + sizeof(arg5_t) +                \
                                 sizeof(arg6_t) + sizeof(arg7_t) + sizeof(arg8_t));                \
        func(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);                                              \
    }

#define luaA_function_register0(L, func, ret_t)                                                    \
    luaA_function_register_type(L, func, __luaA_##func, #func, luaA_type(L, ret_t), 0)

#define luaA_function_register1(L, func, ret_t, arg0_t)                                            \
    luaA_function_register_type(L, func, __luaA_##func, #func, luaA_type(L, ret_t), 1,             \
                                luaA_type(L, arg0_t))

#define luaA_function_register2(L, func, ret_t, arg0_t, arg1_t)                                    \
    luaA_function_register_type(L, func, __luaA_##func, #func, luaA_type(L, ret_t), 2,             \
                                luaA_type(L, arg0_t), luaA_type(L, arg1_t))

#define luaA_function_register3(L, func, ret_t, arg0_t, arg1_t, arg2_t)                            \
    luaA_function_register_type(L, func, __luaA_##func, #func, luaA_type(L, ret_t), 3,             \
                                luaA_type(L, arg0_t), luaA_type(L, arg1_t), luaA_type(L, arg2_t))

#define luaA_function_register4(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t)                    \
    luaA_function_register_type(L, func, __luaA_##func, #func, luaA_type(L, ret_t), 4,             \
                                luaA_type(L, arg0_t), luaA_type(L, arg1_t), luaA_type(L, arg2_t),  \
                                luaA_type(L, arg3_t))

#define luaA_function_register5(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t)            \
    luaA_function_register_type(L, func, __luaA_##func, #func, luaA_type(L, ret_t), 5,             \
                                luaA_type(L, arg0_t), luaA_type(L, arg1_t), luaA_type(L, arg2_t),  \
                                luaA_type(L, arg3_t), luaA_type(L, arg4_t))

#define luaA_function_register6(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t)    \
    luaA_function_register_type(L, func, __luaA_##func, #func, luaA_type(L, ret_t), 6,             \
                                luaA_type(L, arg0_t), luaA_type(L, arg1_t), luaA_type(L, arg2_t),  \
                                luaA_type(L, arg3_t), luaA_type(L, arg4_t), luaA_type(L, arg5_t))

#define luaA_function_register7(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,    \
                                arg6_t)                                                            \
    luaA_function_register_type(L, func, __luaA_##func, #func, luaA_type(L, ret_t), 7,             \
                                luaA_type(L, arg0_t), luaA_type(L, arg1_t), luaA_type(L, arg2_t),  \
                                luaA_type(L, arg3_t), luaA_type(L, arg4_t), luaA_type(L, arg5_t),  \
                                luaA_type(L, arg6_t))

#define luaA_function_register8(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,    \
                                arg6_t, arg7_t)                                                    \
    luaA_function_register_type(L, func, __luaA_##func, #func, luaA_type(L, ret_t), 8,             \
                                luaA_type(L, arg0_t), luaA_type(L, arg1_t), luaA_type(L, arg2_t),  \
                                luaA_type(L, arg3_t), luaA_type(L, arg4_t), luaA_type(L, arg5_t),  \
                                luaA_type(L, arg6_t), luaA_type(L, arg7_t))

#define luaA_function_register9(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,    \
                                arg6_t, arg7_t, arg8_t)                                            \
    luaA_function_register_type(L, func, __luaA_##func, #func, luaA_type(L, ret_t), 9,             \
                                luaA_type(L, arg0_t), luaA_type(L, arg1_t), luaA_type(L, arg2_t),  \
                                luaA_type(L, arg3_t), luaA_type(L, arg4_t), luaA_type(L, arg5_t),  \
                                luaA_type(L, arg6_t), luaA_type(L, arg7_t), luaA_type(L, arg8_t))

#define luaA_function_register10(L, func, ret_t, arg0_t, arg1_t, arg2_t, arg3_t, arg4_t, arg5_t,   \
                                 arg6_t, arg7_t, arg8_t, arg9_t)                                   \
    luaA_function_register_type(L, func, __luaA_##func, #func, luaA_type(L, ret_t), 10,            \
                                luaA_type(L, arg0_t), luaA_type(L, arg1_t), luaA_type(L, arg2_t),  \
                                luaA_type(L, arg3_t), luaA_type(L, arg4_t), luaA_type(L, arg5_t),  \
                                luaA_type(L, arg6_t), luaA_type(L, arg7_t), luaA_type(L, arg8_t),  \
                                luaA_type(L, arg9_t))

#endif

#define luaA_function(L, func, ret_t, ...)                                                         \
    luaA_function_declare(func, ret_t, ##__VA_ARGS__);                                             \
    luaA_function_register(L, func, ret_t, ##__VA_ARGS__)
#define luaA_function_declare(func, ret_t, ...)                                                    \
    LUAA_DECLARE(func, ret_t, LUAA_COUNT(__VA_ARGS__), LUAA_SUFFIX(ret_t), ##__VA_ARGS__)
#define luaA_function_register(L, func, ret_t, ...)                                                \
    LUAA_REGISTER(L, func, ret_t, LUAA_COUNT(__VA_ARGS__), ##__VA_ARGS__)

    enum {
        LUAA_RETURN_STACK_SIZE = 256,
        LUAA_ARGUMENT_STACK_SIZE = 2048
    };

    typedef void (*luaA_Func)(void *, void *);

    int luaA_call(lua_State *L, void *func_ptr);
    int luaA_call_name(lua_State *L, const char *func_name);

    void luaA_function_register_type(lua_State *L, void *src_func, luaA_Func auto_func,
                                     const char *name, luaA_Type ret_tid, int num_args, ...);

#pragma endregion LuaA

    int metadot_preload(lua_State *L, lua_CFunction f, const char *name);
    void metadot_load(lua_State *L, const luaL_Reg *l, const char *name);
    void metadot_loadover(lua_State *L, const luaL_Reg *l, const char *name);

}// namespace LuaWrapper

#endif
