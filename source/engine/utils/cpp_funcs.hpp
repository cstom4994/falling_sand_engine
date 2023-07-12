

#ifndef ME_CPP_FUNCS_HPP
#define ME_CPP_FUNCS_HPP

#include <string>

#include "external/entt/meta/factory.hpp"
#include "external/entt/meta/resolve.hpp"

//[[nodiscard]] entt::id_type get_type_id(const ME::lua::LuaRef &obj) {
//    const auto f = obj["type_id"].get<ME::lua::LuaFunction()>();
//    assert(f.isNilref() && "type_id not exposed to lua!");
//    return f.isNilref() ? f().get<entt::id_type>() : -1;
//}

template <typename T>
T &Get(entt::registry *ecs, entt::entity entity, ME::lua::State *lua) {
    T &instance = registry.get_or_emplace<Type>(entity);
    (*lua)["Get"].get_or_create<sol::table>()[entt::type_info<T>::id()] = &instance;
    return instance;
}

template <typename T>
T &Set(entt::registry *ecs, entt::entity entity, const T &instance) {
    return ecs.emplace_or_replace<T>(entity, instance);
}
// template <typename T>
// void extend_meta_type() {
//     entt::meta<T>().template func<&Get<T>, entt::as_ref_t>("Get").template func<&Set<T>>("Set");
// }
//  void GetComponent(entt::registry* ecs, ME::lua::State* lua, entt::entity entity, ME::lua::VariadicArgType component) {
//      for (auto id : component) {
//          if (auto type = entt::resolve_type(id.as<entt::id_type>()); type) type.func("get"_hs).invoke({}, std::ref(ecs), entity, lua);
//      }
//  }
// template <typename Type>
// struct entt::type_index<Type> {
//    static id_type value() {
//        static const id_type value = (extend_meta_type<Type>(), internal::type_index::next());
//        return value;
//    }
//};

template <typename T>
[[nodiscard]] entt::id_type deduce_type(T &&obj) {
    switch (obj.get_type()) {
        // in lua: registry:has(e, Transform.type_id())
        case sol::type::number:
            return obj.as<entt::id_type>();
        // in lua: registry:has(e, Transform)
        case sol::type::table:
            return get_type_id(obj);
    }
    assert(false);
    return -1;
}

// @see
// https://github.com/skypjack/entt/wiki/Crash-Course:-runtime-reflection-system

template <typename... Args>
inline auto invoke_meta_func(entt::meta_type meta_type, entt::id_type function_id, Args &&...args) {
    if (!meta_type) {
        // TODO: Warning message
    } else {
        if (auto meta_function = meta_type.func(function_id); meta_function) return meta_function.invoke({}, std::forward<Args>(args)...);
    }
    return entt::meta_any{};
}

template <typename... Args>
inline auto invoke_meta_func(entt::id_type type_id, entt::id_type function_id, Args &&...args) {
    return invoke_meta_func(entt::resolve(type_id), function_id, std::forward<Args>(args)...);
}

namespace ME {

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace ME

// ME METypeInfo Implement
#define ImplTemplateTypeInfo(CLASS) \
    template <class T>              \
    inline constexpr METypeInfo GetTypeInfo<CLASS<T>> = {#CLASS, Selector::None, {&GetTypeInfo<T>}}
#define ImplTemplateTypeInfoMultiArgs(CLASS) \
    template <class... T>                    \
    inline constexpr METypeInfo GetTypeInfo<CLASS<T...>> = {#CLASS, Selector::None, (GetArgTypeInfo<T...>)}
#define ImplTypeInfo(CLASS) \
    template <>             \
    inline constexpr METypeInfo GetTypeInfo<CLASS> = {#CLASS, Selector::None};

#ifndef __cpp_char8_t
enum class char8_t : unsigned char {};
#endif

enum class Selector : unsigned {
    None = 0u,
    Const = 1u << 1u,
    Extent = Const << 1u,
    Volatile = Extent << 1u,
    Reference = Volatile << 1u,
    LValueReference = Reference << 1u,
    ConstLValueRef = Const | LValueReference,
    ConstRef = Const | Reference,
    ConstExtent = Const | Extent,
    ConstVolatile = Const | Volatile
};

constexpr Selector operator|(Selector lhs, Selector rhs) { return static_cast<Selector>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)); }

constexpr bool operator&&(Selector lhs, Selector rhs) { return static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs); }

struct METypeInfo;

typedef std::array<METypeInfo const *, 10> ArgsList;

static constexpr char VOID_TYPE_NAME[] = "void";
static constexpr char FUNCTION_TYPE_NAME[] = "";
static constexpr char POINTER_TYPE_NAME[] = "";

#pragma pack(push, 1)
struct METypeInfo {
    const char *typeId = VOID_TYPE_NAME;  // pointer to static storage
    const Selector qualifier;
    METypeInfo const *pReturnTypeInfo = nullptr;  // pointer to static storage
    const ArgsList arrArgTypeInfo{};
    constexpr METypeInfo(const char *id, const Selector q) : typeId{id}, qualifier{q} {}

    constexpr METypeInfo(const char *id, const Selector q, const ArgsList &child) : typeId{id}, qualifier{q}, arrArgTypeInfo(child) {}

    constexpr METypeInfo(const METypeInfo &assign, const Selector qMerge)
        : typeId{assign.typeId}, qualifier{assign.qualifier | qMerge}, pReturnTypeInfo{assign.pReturnTypeInfo}, arrArgTypeInfo{assign.arrArgTypeInfo} {}

    constexpr METypeInfo(const Selector q, const METypeInfo &ret, const ArgsList &args) : typeId{FUNCTION_TYPE_NAME}, qualifier{q}, pReturnTypeInfo{&ret}, arrArgTypeInfo{args} {}

    friend constexpr bool operator==(const METypeInfo &lhs, const METypeInfo &rhs) {
        const auto eq = lhs.typeId == rhs.typeId && lhs.qualifier == rhs.qualifier;
        if (!eq) return false;
        if (lhs.pReturnTypeInfo != rhs.pReturnTypeInfo) return false;
        if (lhs.pReturnTypeInfo != nullptr) return *lhs.pReturnTypeInfo == *rhs.pReturnTypeInfo;
        for (size_t i = 0; i < lhs.arrArgTypeInfo.size(); ++i) {
            if (lhs.arrArgTypeInfo[i] != rhs.arrArgTypeInfo[i]) return false;
            if (lhs.arrArgTypeInfo[i] == nullptr) return eq;
            if (lhs.arrArgTypeInfo[i]->typeId == nullptr) return false;
        }
        return eq;
    }

    friend constexpr bool operator!=(const METypeInfo &lhs, const METypeInfo &rhs) { return !(lhs == rhs); }
};
#pragma pack(pop)

template <class T>
struct RemoveAllQualifiers {
    typedef T Type;
};

template <class T>
struct RemoveAllQualifiers<T *> : RemoveAllQualifiers<T> {};

template <class T>
struct RemoveAllQualifiers<T &> : RemoveAllQualifiers<T> {};

template <class T>
struct RemoveAllQualifiers<T &&> : RemoveAllQualifiers<T> {};

template <class T>
struct RemoveAllQualifiers<T const> : RemoveAllQualifiers<T> {};

template <class T>
struct RemoveAllQualifiers<T volatile> : RemoveAllQualifiers<T> {};

template <class T>
struct RemoveAllQualifiers<T const volatile> : RemoveAllQualifiers<T> {};

template <class T>
using RemoveAllQualifierType = typename RemoveAllQualifiers<T>::Type;

template <class T>
constexpr inline Selector GetTypeQualifier = Selector::None;
template <class T>
constexpr inline Selector GetTypeQualifier<const T> = Selector::Const | GetTypeQualifier<T>;
template <class T>
constexpr inline Selector GetTypeQualifier<volatile T> = Selector::Volatile | GetTypeQualifier<T>;
template <class T>
constexpr inline Selector GetTypeQualifier<T &> = Selector::Reference | GetTypeQualifier<T>;
template <class T>
constexpr inline Selector GetTypeQualifier<T &&> = Selector::LValueReference | GetTypeQualifier<T>;
template <class T>
constexpr inline Selector GetTypeQualifier<T[]> = Selector::Extent | GetTypeQualifier<T>;
template <class T, size_t s>
constexpr inline Selector GetTypeQualifier<T[s]> = Selector::Extent | GetTypeQualifier<T>;
template <class T>
constexpr inline Selector GetTypeQualifier<const volatile T> = Selector::ConstVolatile | GetTypeQualifier<T>;
template <class T>
constexpr inline Selector GetTypeQualifier<const volatile T[]> = Selector::ConstVolatile | Selector::Extent | GetTypeQualifier<T>;

template <class T>
constexpr inline METypeInfo GetTypeInfo = {VOID_TYPE_NAME, GetTypeQualifier<T>};  // undefined types
template <>
constexpr inline METypeInfo GetTypeInfo<void> = {VOID_TYPE_NAME, Selector::None};
template <class T>
constexpr inline METypeInfo GetTypeInfo<T &> = {GetTypeInfo<T>, Selector::Reference};
template <class T>
constexpr inline METypeInfo GetTypeInfo<T *> = {POINTER_TYPE_NAME, Selector::None, {&GetTypeInfo<T>}};
template <class T>
constexpr inline METypeInfo GetTypeInfo<T &&> = {GetTypeInfo<T>, Selector::LValueReference};
template <class T>
constexpr inline METypeInfo GetTypeInfo<T[]> = {GetTypeInfo<T>, Selector::Extent};
template <class T>
constexpr inline METypeInfo GetTypeInfo<const T> = {GetTypeInfo<T>, Selector::Const};
template <class T>
constexpr inline METypeInfo GetTypeInfo<volatile T> = {GetTypeInfo<T>, Selector::Volatile};
template <class T>
constexpr inline METypeInfo GetTypeInfo<const volatile T> = {GetTypeInfo<T>, Selector::ConstVolatile};
template <class T, size_t s>
constexpr inline METypeInfo GetTypeInfo<T[s]> = {GetTypeInfo<T>, Selector::Extent};
template <class T>
constexpr inline METypeInfo GetTypeInfo<T *[]> = {GetTypeInfo<T *>, Selector::Extent};

template <class... Args>
constexpr inline ArgsList GetArgTypeInfo = {&GetTypeInfo<void>};

template <class Arg>
constexpr inline ArgsList GetArgTypeInfo<Arg> = {&GetTypeInfo<Arg>};
template <class Arg1, class Arg2>
constexpr inline ArgsList GetArgTypeInfo<Arg1, Arg2> = {&GetTypeInfo<Arg1>, &GetTypeInfo<Arg2>};
template <class Arg1, class Arg2, class Arg3>
constexpr inline ArgsList GetArgTypeInfo<Arg1, Arg2, Arg3> = {&GetTypeInfo<Arg1>, &GetTypeInfo<Arg2>, &GetTypeInfo<Arg3>};
template <class Arg1, class Arg2, class Arg3, class Arg4>
constexpr inline ArgsList GetArgTypeInfo<Arg1, Arg2, Arg3, Arg4> = {&GetTypeInfo<Arg1>, &GetTypeInfo<Arg2>, &GetTypeInfo<Arg3>, &GetTypeInfo<Arg4>};
template <class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
constexpr inline ArgsList GetArgTypeInfo<Arg1, Arg2, Arg3, Arg4, Arg5> = {&GetTypeInfo<Arg1>, &GetTypeInfo<Arg2>, &GetTypeInfo<Arg3>, &GetTypeInfo<Arg4>, &GetTypeInfo<Arg5>};
template <class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
constexpr inline ArgsList GetArgTypeInfo<Arg1, Arg2, Arg3, Arg4, Arg5, Arg6> = {&GetTypeInfo<Arg1>, &GetTypeInfo<Arg2>, &GetTypeInfo<Arg3>, &GetTypeInfo<Arg4>, &GetTypeInfo<Arg5>, &GetTypeInfo<Arg6>};
template <class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
constexpr inline ArgsList GetArgTypeInfo<Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7> = {&GetTypeInfo<Arg1>, &GetTypeInfo<Arg2>, &GetTypeInfo<Arg3>, &GetTypeInfo<Arg4>,
                                                                                      &GetTypeInfo<Arg5>, &GetTypeInfo<Arg6>, &GetTypeInfo<Arg7>};
template <class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8>
constexpr inline ArgsList GetArgTypeInfo<Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8> = {&GetTypeInfo<Arg1>, &GetTypeInfo<Arg2>, &GetTypeInfo<Arg3>, &GetTypeInfo<Arg4>,
                                                                                            &GetTypeInfo<Arg5>, &GetTypeInfo<Arg6>, &GetTypeInfo<Arg7>, &GetTypeInfo<Arg8>};
template <class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8, class Arg9>
constexpr inline ArgsList GetArgTypeInfo<Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9> = {&GetTypeInfo<Arg1>, &GetTypeInfo<Arg2>, &GetTypeInfo<Arg3>, &GetTypeInfo<Arg4>, &GetTypeInfo<Arg5>,
                                                                                                  &GetTypeInfo<Arg6>, &GetTypeInfo<Arg7>, &GetTypeInfo<Arg8>, &GetTypeInfo<Arg9>};
template <class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8, class Arg9, class Arg10>
constexpr inline ArgsList GetArgTypeInfo<Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9, Arg10> = {&GetTypeInfo<Arg1>, &GetTypeInfo<Arg2>, &GetTypeInfo<Arg3>, &GetTypeInfo<Arg4>,
                                                                                                         &GetTypeInfo<Arg5>, &GetTypeInfo<Arg6>, &GetTypeInfo<Arg7>, &GetTypeInfo<Arg8>,
                                                                                                         &GetTypeInfo<Arg9>, &GetTypeInfo<Arg10>};

template <class R, class Arg>
constexpr inline METypeInfo GetTypeInfo<R (*)(Arg)> = {Selector::None, GetTypeInfo<R>, {&GetTypeInfo<Arg>}};
template <class R, class Arg>
constexpr inline METypeInfo GetTypeInfo<const R (*)(Arg)> = {Selector::Const, GetTypeInfo<R>, {&GetTypeInfo<Arg>}};
template <class R, class... Args>
constexpr inline METypeInfo GetTypeInfo<R (*)(Args...)> = {Selector::None, GetTypeInfo<R>, GetArgTypeInfo<Args...>};
template <class R, class... Args>
constexpr inline METypeInfo GetTypeInfo<const R (*)(Args...)> = {Selector::Const, GetTypeInfo<R>, GetArgTypeInfo<Args...>};

inline std::string TypeName(const METypeInfo &info);

inline std::string FormatArgList(const ArgsList &args) {
    if (args[0] == nullptr) return "";
    auto s = TypeName(*args[0]);
    auto it = args.begin();
    s.reserve(200);
    for (++it; it != args.end(); ++it) {
        const auto &arg = *it;
        if (arg == nullptr || arg->typeId == nullptr) return s;
        s.append(", ");
        s.append(TypeName(*arg));
    }
    return s;
}

inline std::string TypeName(const METypeInfo &info) {
    std::string name;
    const auto qualifier = info.qualifier;
    name.reserve(200);
    if (qualifier && Selector::Const) name.append("const ");
    if (qualifier && Selector::Volatile) name.append("volatile ");
    if (info.typeId == FUNCTION_TYPE_NAME) {
        name.append(TypeName(*info.pReturnTypeInfo));
        name.append("(*)(");
        name.append(FormatArgList(info.arrArgTypeInfo));
        name.push_back(')');
    } else if (info.typeId == POINTER_TYPE_NAME) {
        name.append(FormatArgList(info.arrArgTypeInfo));
        name.push_back('*');
    } else {
        name += info.typeId;
        if (info.arrArgTypeInfo[0] != nullptr) {
            name.push_back('<');
            name.append(FormatArgList(info.arrArgTypeInfo));
            name.push_back('>');
        }
    }

    if (qualifier && Selector::Reference)
        name.push_back('&');
    else {
        if (qualifier && Selector::Extent) name.append("[]");
    }
    if (qualifier && Selector::LValueReference) name.append(2, '&');
    return name;
}

#define TypeNameOf(x) TypeName(GetTypeInfo<decltype(x)>)

/* declare all classes in namespace std */
namespace std {
/* supported */
class any;
template <class>
struct char_traits;
template <class>
class allocator;
template <class>
struct hash;
template <class>
struct equal_to;
template <class>
struct optional;
template <class>
struct greater;
template <class>
struct greater_equal;
template <class>
struct less;
template <class>
struct less_equal;
template <class>
struct plus;
template <class>
struct minus;
template <class>
struct multiplies;
template <class>
struct divides;
template <class>
struct modulus;
template <class>
struct negate;
template <class>
struct logical_and;
template <class>
struct logical_or;
template <class>
struct logical_not;
template <class>
struct bit_and;
template <class>
struct bit_or;
template <class>
struct bit_xor;
template <class>
struct bit_not;
template <class>
struct unary_negate;
template <class>
struct binary_negate;
template <class>
class initializer_list;
//    template <class, class, class> class basic_string;
//    template <class, class> class basic_string_view;
template <class, class>
class vector;
template <class, class>
class deque;
template <class, class>
class list;
template <class, class>
class forward_list;
template <class, class>
class queue;
template <class, class>
class stack;
template <class, class>
struct pair;
template <class, class>
class basic_ios;
template <class, class>
class basic_streambuf;
template <class, class>
class basic_istream;
template <class, class>
class basic_ostream;
template <class, class>
class basic_iostream;
template <class, class>
class basic_filebuf;
template <class, class>
class basic_ifstream;
template <class, class>
class basic_ofstream;
template <class, class>
class basic_fstream;
// template <class, class, class> class basic_stringbuf;
// template <class, class, class> class basic_istringstream;
// template <class, class, class> class basic_ostringstream;
// template <class, class, class> class basic_stringstream;
template <class...>
class tuple;
template <class...>
class variant;
template <class, class, class>
class set;
template <class, class, class>
class multiset;
template <class, class, class, class>
class map;
template <class, class, class, class>
class multimap;
template <class, class, class>
class priority_queue;
template <class, class, class, class>
class unordered_set;
template <class, class, class, class, class>
class unordered_map;
template <class, class, class, class>
class unordered_multiset;
template <class, class, class, class, class>
class unordered_multimap;
/* non type template (unsupported) */
template <class, size_t>
class array;
}  // namespace std

/* implement METypeInfo for static types */
ImplTypeInfo(char);
ImplTypeInfo(wchar_t);
ImplTypeInfo(char8_t);
ImplTypeInfo(char16_t);
ImplTypeInfo(char32_t);
ImplTypeInfo(signed char);
ImplTypeInfo(unsigned char);
ImplTypeInfo(int);
ImplTypeInfo(long);
ImplTypeInfo(long long);
ImplTypeInfo(unsigned int);
ImplTypeInfo(unsigned long);
ImplTypeInfo(unsigned long long);
ImplTypeInfo(float);
ImplTypeInfo(double);
ImplTypeInfo(long double);

/* implement METypeInfo for std:: types */
ImplTypeInfo(std::byte);
ImplTypeInfo(std::any);
ImplTemplateTypeInfo(std::char_traits);
ImplTemplateTypeInfo(std::allocator);
ImplTemplateTypeInfo(std::hash);
ImplTemplateTypeInfo(std::equal_to);
ImplTemplateTypeInfo(std::optional);
ImplTemplateTypeInfo(std::greater);
ImplTemplateTypeInfo(std::greater_equal);
ImplTemplateTypeInfo(std::less);
ImplTemplateTypeInfo(std::less_equal);
ImplTemplateTypeInfo(std::plus);
ImplTemplateTypeInfo(std::minus);
ImplTemplateTypeInfo(std::multiplies);
ImplTemplateTypeInfo(std::divides);
ImplTemplateTypeInfo(std::modulus);
ImplTemplateTypeInfo(std::negate);
ImplTemplateTypeInfo(std::logical_and);
ImplTemplateTypeInfo(std::logical_or);
ImplTemplateTypeInfo(std::logical_not);
ImplTemplateTypeInfo(std::bit_and);
ImplTemplateTypeInfo(std::bit_or);
ImplTemplateTypeInfo(std::bit_xor);
ImplTemplateTypeInfo(std::bit_not);
ImplTemplateTypeInfo(std::unary_negate);
ImplTemplateTypeInfo(std::binary_negate);
ImplTemplateTypeInfo(std::initializer_list);
ImplTemplateTypeInfoMultiArgs(std::basic_string);
ImplTemplateTypeInfoMultiArgs(std::basic_string_view);
ImplTemplateTypeInfoMultiArgs(std::vector);
ImplTemplateTypeInfoMultiArgs(std::deque);
ImplTemplateTypeInfoMultiArgs(std::queue);
ImplTemplateTypeInfoMultiArgs(std::list);
ImplTemplateTypeInfoMultiArgs(std::forward_list);
ImplTemplateTypeInfoMultiArgs(std::stack);
ImplTemplateTypeInfoMultiArgs(std::tuple);
ImplTemplateTypeInfoMultiArgs(std::pair);
ImplTemplateTypeInfoMultiArgs(std::basic_ios);
ImplTemplateTypeInfoMultiArgs(std::basic_streambuf);
ImplTemplateTypeInfoMultiArgs(std::basic_istream);
ImplTemplateTypeInfoMultiArgs(std::basic_ostream);
ImplTemplateTypeInfoMultiArgs(std::basic_iostream);
ImplTemplateTypeInfoMultiArgs(std::basic_filebuf);
ImplTemplateTypeInfoMultiArgs(std::basic_ifstream);
ImplTemplateTypeInfoMultiArgs(std::basic_ofstream);
ImplTemplateTypeInfoMultiArgs(std::basic_fstream);
ImplTemplateTypeInfoMultiArgs(std::basic_stringbuf);
ImplTemplateTypeInfoMultiArgs(std::basic_istringstream);
ImplTemplateTypeInfoMultiArgs(std::basic_ostringstream);
ImplTemplateTypeInfoMultiArgs(std::basic_stringstream);
ImplTemplateTypeInfoMultiArgs(std::variant);
ImplTemplateTypeInfoMultiArgs(std::set);
ImplTemplateTypeInfoMultiArgs(std::multiset);
ImplTemplateTypeInfoMultiArgs(std::map);
ImplTemplateTypeInfoMultiArgs(std::multimap);
ImplTemplateTypeInfoMultiArgs(std::priority_queue);
ImplTemplateTypeInfoMultiArgs(std::unordered_set);
ImplTemplateTypeInfoMultiArgs(std::unordered_map);
ImplTemplateTypeInfoMultiArgs(std::unordered_multiset);
ImplTemplateTypeInfoMultiArgs(std::unordered_multimap);

/* implement METypeInfo for ME Types */
// ImplTypeInfo(Int8);
// ImplTypeInfo(UInt8);
// ImplTypeInfo(Int16);
// ImplTypeInfo(UInt16);
// ImplTypeInfo(Int32);
// ImplTypeInfo(UInt32);
// ImplTypeInfo(Int64);
// ImplTypeInfo(UInt64);
// ImplTypeInfo(Float32);
// ImplTypeInfo(Float64);
// ImplTypeInfo(byte);

// ImplTypeInfo(MarkdownData);s

#endif  // !ME_CPP_FUNCS_HPP
