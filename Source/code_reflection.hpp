// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_CODEREFLECTION_HPP_
#define _METADOT_CODEREFLECTION_HPP_

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/core.hpp"
#include "core/cpp/name.hpp"
#include "core/cpp/static_relfection.hpp"

namespace Meta {
template <typename Str>
struct Typeof;
template <typename Str>
using Typeof_t = typename Meta::Typeof<Str>::type;
}  // namespace Meta

template <typename T>
struct is_std_tuple : std::false_type {};
template <typename... Ts>
struct is_std_tuple<std::tuple<Ts...>> : std::true_type {};
template <typename T>
constexpr bool is_std_tuple_v = is_std_tuple<T>::value;

template <typename T, typename Tuple>
constexpr auto tuple_init(Tuple &&t) {
    return std::apply([](auto &&...elems) { return T{std::forward<decltype(elems)>(elems)...}; }, std::forward<Tuple>(t));
}

template <typename Str, typename Value>
constexpr auto attr_init(Str, Value &&v) {
    using T = Meta::Typeof_t<Str>;
    if constexpr (is_std_tuple_v<std::decay_t<Value>>)
        return tuple_init<T>(std::forward<Value>(v));
    else
        return T{std::forward<Value>(v)};
}

#define TYPEOF_REGISTER(X)                                                     \
    template <>                                                                \
    struct Meta::Typeof<typename MetaEngine::StaticRefl::TypeInfo<X>::TName> { \
        using type = X;                                                        \
    }

namespace IamAfuckingNamespace {
int func1(float f, char c);
void func2(void);
void func_log_info(std::string info);
}  // namespace IamAfuckingNamespace

void InitCppReflection();

#pragma region AnyFunction

namespace Meta {

// https://stackoverflow.com/questions/26107041/how-can-i-determine-the-return-type-of-a-c11-member-function

template <typename T>
struct return_type;
template <typename R, typename... Args>
struct return_type<R (*)(Args...)> {
    using type = R;
};
template <typename R, typename C, typename... Args>
struct return_type<R (C::*)(Args...)> {
    using type = R;
};
template <typename R, typename C, typename... Args>
struct return_type<R (C::*)(Args...) const> {
    using type = R;
};
template <typename R, typename C, typename... Args>
struct return_type<R (C::*)(Args...) volatile> {
    using type = R;
};
template <typename R, typename C, typename... Args>
struct return_type<R (C::*)(Args...) const volatile> {
    using type = R;
};
template <typename T>
using return_type_t = typename return_type<T>::type;

struct AnyFunction {
public:
    struct type {
        const std::type_info *info;
        bool is_lvalue_reference, is_rvalue_reference;
        bool is_const, is_volatile;
        bool operator==(const type &r) const {
            return info == r.info && is_lvalue_reference == r.is_lvalue_reference && is_rvalue_reference == r.is_rvalue_reference && is_const == r.is_const && is_volatile == r.is_volatile;
        }
        bool operator!=(const type &r) const { return !(*this == r); }
        template <class T>
        static type capture() {
            return {&typeid(T), std::is_lvalue_reference<T>::value, std::is_rvalue_reference<T>::value, std::is_const<typename std::remove_reference<T>::type>::value,
                    std::is_volatile<typename std::remove_reference<T>::type>::value};
        }
    };

    class result {
        struct result_base {
            virtual ~result_base() {}
            virtual std::unique_ptr<result_base> clone() const = 0;
            virtual type get_type() const = 0;
            virtual void *get_address() = 0;
        };
        template <class T>
        struct typed_result : result_base {
            T x;
            typed_result(T x) : x(get((void *)&x, tag<T>{})) {}
            std::unique_ptr<result_base> clone() const { return std::unique_ptr<typed_result>(new typed_result(get((void *)&x, tag<T>{}))); }
            type get_type() const { return type::capture<T>(); }
            void *get_address() { return (void *)&x; }
        };
        std::unique_ptr<result_base> p;

    public:
        result() {}
        result(result &&r) : p(move(r.p)) {}
        result(const result &r) { *this = r; }
        result &operator=(result &&r) {
            p.swap(r.p);
            return *this;
        }
        result &operator=(const result &r) {
            p = r.p ? r.p->clone() : nullptr;
            return *this;
        }

        type get_type() const { return p ? p->get_type() : type::capture<void>(); }
        void *get_address() { return p ? p->get_address() : nullptr; }
        template <class T>
        T get_value() {
            METADOT_ASSERT_E(get_type() == type::capture<T>());
            return get(p->get_address(), tag<T>{});
        }

        template <class T>
        static result capture(T x) {
            result r;
            r.p.reset(new typed_result<T>(static_cast<T>(x)));
            return r;
        }
    };
    AnyFunction() : result_type{} {}
    AnyFunction(std::nullptr_t) : result_type{} {}
    template <class R, class... A>
    AnyFunction(R (*p)(A...)) : AnyFunction(p, tag<R>{}, tag<A...>{}, build_indices<sizeof...(A)>{}) {}
    template <class R, class... A>
    AnyFunction(std::function<R(A...)> f) : AnyFunction(f, tag<R>{}, tag<A...>{}, build_indices<sizeof...(A)>{}) {}
    template <class F>
    AnyFunction(F f) : AnyFunction(f, &F::operator()) {}

    explicit operator bool() const { return static_cast<bool>(func); }
    const std::vector<type> &get_parameter_types() const { return parameter_types; }
    const type &get_result_type() const { return result_type; }
    result invoke(void *const args[]) const { return func(args); }
    result invoke(std::initializer_list<void *> args) const { return invoke(args.begin()); }

private:
    template <class... T>
    struct tag {};
    template <std::size_t... IS>
    struct indices {};
    template <std::size_t N, std::size_t... IS>
    struct build_indices : build_indices<N - 1, N - 1, IS...> {};
    template <std::size_t... IS>
    struct build_indices<0, IS...> : indices<IS...> {};

    template <class T>
    static T get(void *arg, tag<T>) {
        return *reinterpret_cast<T *>(arg);
    }
    template <class T>
    static T &get(void *arg, tag<T &>) {
        return *reinterpret_cast<T *>(arg);
    }
    template <class T>
    static T &&get(void *arg, tag<T &&>) {
        return std::move(*reinterpret_cast<T *>(arg));
    }
    template <class F, class R, class... A, size_t... I>
    AnyFunction(F f, tag<R>, tag<A...>, indices<I...>) : parameter_types({type::capture<A>()...}), result_type(type::capture<R>()) {
        func = [f](void *const args[]) mutable { return result::capture<R>(f(get(args[I], tag<A>{})...)); };
    }
    template <class F, class... A, size_t... I>
    AnyFunction(F f, tag<void>, tag<A...>, indices<I...>) : parameter_types({type::capture<A>()...}), result_type(type::capture<void>()) {
        func = [f](void *const args[]) mutable { return f(get(args[I], tag<A>{})...), result{}; };
    }
    template <class F, class R>
    AnyFunction(F f, tag<R>, tag<>, indices<>) : parameter_types({}), result_type(type::capture<R>()) {
        func = [f](void *const args[]) mutable { return result::capture<R>(f()); };
    }
    template <class F>
    AnyFunction(F f, tag<void>, tag<>, indices<>) : parameter_types({}), result_type(type::capture<void>()) {
        func = [f](void *const args[]) mutable { return f(), result{}; };
    }
    template <class F, class R, class... A>
    AnyFunction(F f, R (F::*p)(A...)) : AnyFunction(f, tag<R>{}, tag<A...>{}, build_indices<sizeof...(A)>{}) {}
    template <class F, class R, class... A>
    AnyFunction(F f, R (F::*p)(A...) const) : AnyFunction(f, tag<R>{}, tag<A...>{}, build_indices<sizeof...(A)>{}) {}

    std::function<result(void *const *)> func;
    std::vector<type> parameter_types;
    type result_type;
};

}  // namespace Meta

#pragma endregion AnyFunction

namespace tmp {

// type list
template <typename... TS>
struct typelist {
    static constexpr auto size = sizeof...(TS);
};

template <typename T>
struct is_typelist : std::false_type {};

template <typename... TS>
struct is_typelist<typelist<TS...>> : std::true_type {};

// basic operations
template <typename T, typename TL>
struct push_back;
template <typename T, typename TL>
struct push_front;
template <typename TL>
struct pop_front;
template <typename TL, size_t I>
struct at;

template <typename T, typename... TS>
struct push_back<T, typelist<TS...>> {
    using type = typelist<TS..., T>;
};

template <typename T, typename... TS>
struct push_front<T, typelist<TS...>> {
    using type = typelist<T, TS...>;
};

template <typename T, typename... TS>
struct pop_front<typelist<T, TS...>> {
    using type = typelist<TS...>;
};

template <typename T, typename... TS>
struct at<typelist<T, TS...>, 0> {
    using type = T;
};

template <typename T, typename... TS, size_t I>
struct at<typelist<T, TS...>, I> {
    METADOT_STATIC_ASSERT(I < (1 + sizeof...(TS)), "Out of bounds access");
    using type = typename at<typelist<TS...>, I - 1>::type;
};

// 'filter'
template <typename TL, template <typename> class PRED>
struct filter;

template <template <typename> class PRED>
struct filter<typelist<>, PRED> {
    using type = typelist<>;
};

template <typename T, typename... TS, template <typename> class PRED>
struct filter<typelist<T, TS...>, PRED> {
    using remaining = typename filter<typelist<TS...>, PRED>::type;
    using type = typename std::conditional<PRED<T>::value, typename push_front<T, remaining>::type, remaining>::type;
};

// 'max' given a template binary predicate
template <typename TL, template <typename, typename> class PRED>
struct max;

template <typename T, template <typename, typename> class PRED>
struct max<typelist<T>, PRED> {
    using type = T;
};

template <typename... TS, template <typename, typename> class PRED>
struct max<typelist<TS...>, PRED> {
    using first = typename at<typelist<TS...>, 0>::type;
    using remaining_max = typename max<typename pop_front<typelist<TS...>>::type, PRED>::type;
    using type = typename std::conditional<PRED<first, remaining_max>::value, first, remaining_max>::type;
};

// 'find_ancestors'
namespace impl {

template <typename SRCLIST, typename DESTLIST>
struct find_ancestors {

    template <typename B>
    using negation = typename std::integral_constant<bool, !bool(B::value)>::type;

    template <typename T, typename U>
    using cmp = typename std::is_base_of<T, U>::type;
    using most_ancient = typename max<SRCLIST, cmp>::type;

    template <typename T>
    using not_most_ancient = typename negation<std::is_same<most_ancient, T>>::type;

    using type = typename find_ancestors<typename filter<SRCLIST, not_most_ancient>::type, typename push_back<most_ancient, DESTLIST>::type>::type;
};

template <typename DESTLIST>
struct find_ancestors<typelist<>, DESTLIST> {
    using type = DESTLIST;
};

}  // namespace impl

template <typename TL, typename T>
struct find_ancestors {
    METADOT_STATIC_ASSERT(is_typelist<TL>::value, "The first parameter is not a typelist");

    template <typename U>
    using base_of_T = typename std::is_base_of<U, T>::type;
    using src_list = typename filter<TL, base_of_T>::type;
    using type = typename impl::find_ancestors<src_list, typelist<>>::type;
};

}  // namespace tmp

using namespace tmp;

template <typename TL>
struct hierarchy_iterator {
    METADOT_STATIC_ASSERT(is_typelist<TL>::value, "Not a typelist");
    inline static void exec(void *_p) {
        using target_t = typename pop_front<TL>::type;
        if (auto ptr = static_cast<target_t *>(_p)) {
            printf("%s\n", typeid(typename at<TL, 0>::type).name());
            // LOG(INFO) << "hierarchy_iterator : " << typeid(typename at<TL, 0>::type).name();
            hierarchy_iterator<target_t>::exec(_p);
        }
    }
};

template <>
struct hierarchy_iterator<typelist<>> {
    inline static void exec(void *) {}
};

namespace Meta {

namespace detail {
// 构造一个可以隐式转换为任意类型的类型
template <class T>
struct any_converter {
    // 不能convert至自身
    template <class U, class = typename std::enable_if<!std::is_same<typename std::decay<T>::type, typename std::decay<U>::type>::value>::type>
    operator U() const noexcept;
};

template <class T, std::size_t I>
struct any_converter_tagged : any_converter<T> {};

// 判断T是否可以使用Args...进行聚合初始化：T{ std::declval<Args>()... }
template <class T, class... Args>
constexpr auto is_aggregate_constructible_impl(T &&, Args &&...args) -> decltype(T{{args}...}, std::true_type());
// 多加一个重载可以去掉讨厌的clang warning
template <class T, class Arg>
constexpr auto is_aggregate_constructible_impl(T &&, Arg &&args) -> decltype(T{args}, std::true_type());
// 这个函数千万别改成模板函数否则会死机的！
constexpr auto is_aggregate_constructible_impl(...) -> std::false_type;

template <class T, class... Args>
struct is_aggregate_constructible : decltype(is_aggregate_constructible_impl(std::declval<T>(), std::declval<Args>()...)) {};

template <class T, class Seq>
struct is_aggregate_constructible_with_n_args;
template <class T, std::size_t... I>
struct is_aggregate_constructible_with_n_args<T, std::index_sequence<I...>> : is_aggregate_constructible<T, any_converter_tagged<T, I>...> {};
// 这里添加一个元函数是用来支持沙雕GCC的，不知道为什么GCC里面变长模板参数不能作为嵌套的实参展开……
template <class T, std::size_t N>
struct is_aggregate_constructible_with_n_args_ex : is_aggregate_constructible_with_n_args<T, std::make_index_sequence<N>> {};

// （原）线性查找法
template <class T, class Seq = std::make_index_sequence<sizeof(T)>>
struct struct_member_count_impl1;
template <class T, std::size_t... I>
struct struct_member_count_impl1<T, std::index_sequence<I...>> : std::integral_constant<std::size_t, (... + !!(is_aggregate_constructible_with_n_args_ex<T, I + 1>::value))> {};

template <class B, class T, class U>
struct lazy_conditional : lazy_conditional<typename B::type, T, U> {};
template <class T, class U>
struct lazy_conditional<std::true_type, T, U> : T {};
template <class T, class U>
struct lazy_conditional<std::false_type, T, U> : U {};

// 二分查找法
template <class T, class Seq = std::index_sequence<0>>
struct struct_member_count_impl2;
template <class T, std::size_t... I>
struct struct_member_count_impl2<T, std::index_sequence<I...>>
    : lazy_conditional<std::conjunction<is_aggregate_constructible_with_n_args_ex<T, I + 1>...>, struct_member_count_impl2<T, std::index_sequence<I..., (I + sizeof...(I))...>>,
                       std::integral_constant<std::size_t, (... + !!(is_aggregate_constructible_with_n_args_ex<T, I + 1>::value))>>::type {};

}  // namespace detail

template <class T>
struct StructMemberCount : detail::struct_member_count_impl2<T> {};

#include "preprocesserflat.hpp"

namespace detail {

#define APPLYER_DEF(N)                                                                          \
    template <class T, class ApplyFunc>                                                         \
    auto StructApply_impl(T &&my_struct, ApplyFunc f, std::integral_constant<std::size_t, N>) { \
        auto &&[ENUM_PARAMS(x, N)] = std::forward<T>(my_struct);                                \
        return std::invoke(f, ENUM_PARAMS(x, N));                                               \
    }

ENUM_FOR_EACH(APPLYER_DEF, 128)
#undef APPLYER_DEF
}  // namespace detail

// StructApply : 把结构体解包为变长参数调用可调用对象f
template <class T, class ApplyFunc>
auto StructApply(T &&my_struct, ApplyFunc f) {
    return detail::StructApply_impl(std::forward<T>(my_struct), f, StructMemberCount<typename std::decay<T>::type>());
}

// StructTransformMeta : 把结构体各成员的类型作为变长参数调用元函数MetaFunc
template <class T, template <class...> class MetaFunc>
struct StructTransformMeta {
    struct FakeApplyer {
        template <class... Args>
        auto operator()(Args... args) -> MetaFunc<decltype(args)...>;
    };
    using type = decltype(StructApply(std::declval<T>(), FakeApplyer()));
};
}  // namespace Meta

namespace reflect {

//--------------------------------------------------------
// Base class of all type descriptors
//--------------------------------------------------------

struct TypeDescriptor {
    const char *name;
    size_t size;

    TypeDescriptor(const char *name, size_t size) : name{name}, size{size} {}
    virtual ~TypeDescriptor() {}
    virtual std::string getFullName() const { return name; }
    virtual void dump(const void *obj, int indentLevel = 0) const = 0;
};

//--------------------------------------------------------
// Finding type descriptors
//--------------------------------------------------------

// Declare the function template that handles primitive types such as int, std::string, etc.:
template <typename T>
TypeDescriptor *getPrimitiveDescriptor();

// A helper class to find TypeDescriptors in different ways:
struct DefaultResolver {
    template <typename T>
    static char func(decltype(&T::Reflection));
    template <typename T>
    static int func(...);
    template <typename T>
    struct IsReflected {
        enum { value = (sizeof(func<T>(nullptr)) == sizeof(char)) };
    };

    // This version is called if T has a static member named "Reflection":
    template <typename T, typename std::enable_if<IsReflected<T>::value, int>::type = 0>
    static TypeDescriptor *get() {
        return &T::Reflection;
    }

    // This version is called otherwise:
    template <typename T, typename std::enable_if<!IsReflected<T>::value, int>::type = 0>
    static TypeDescriptor *get() {
        return getPrimitiveDescriptor<T>();
    }
};

// This is the primary class template for finding all TypeDescriptors:
template <typename T>
struct TypeResolver {
    static TypeDescriptor *get() { return DefaultResolver::get<T>(); }
};

//--------------------------------------------------------
// Type descriptors for user-defined structs/classes
//--------------------------------------------------------

struct TypeDescriptor_Struct : TypeDescriptor {
    struct Member {
        const char *name;
        size_t offset;
        TypeDescriptor *type;
    };

    std::vector<Member> members;

    TypeDescriptor_Struct(void (*init)(TypeDescriptor_Struct *)) : TypeDescriptor{nullptr, 0} { init(this); }
    TypeDescriptor_Struct(const char *name, size_t size, const std::initializer_list<Member> &init) : TypeDescriptor{nullptr, 0}, members{init} {}
    virtual void dump(const void *obj, int indentLevel) const override {
        std::cout << name << " {" << std::endl;
        for (const Member &member : members) {
            std::cout << std::string(4 * (indentLevel + 1), ' ') << member.name << " = ";
            member.type->dump((char *)obj + member.offset, indentLevel + 1);
            std::cout << std::endl;
        }
        std::cout << std::string(4 * indentLevel, ' ') << "}";
    }
};

#define REFLECT_STRUCT()                              \
    friend struct reflect::DefaultResolver;           \
    static reflect::TypeDescriptor_Struct Reflection; \
    static void initReflection(reflect::TypeDescriptor_Struct *);

#define REFLECT_STRUCT_BEGIN(type)                                         \
    reflect::TypeDescriptor_Struct type::Reflection{type::initReflection}; \
    void type::initReflection(reflect::TypeDescriptor_Struct *typeDesc) {  \
        using T = type;                                                    \
        typeDesc->name = #type;                                            \
        typeDesc->size = sizeof(T);                                        \
        typeDesc->members = {

#define REFLECT_STRUCT_MEMBER(name) {#name, offsetof(T, name), reflect::TypeResolver<decltype(T::name)>::get()},

#define REFLECT_STRUCT_END() \
    }                        \
    ;                        \
    }

//--------------------------------------------------------
// Type descriptors for std::vector
//--------------------------------------------------------

struct TypeDescriptor_StdVector : TypeDescriptor {
    TypeDescriptor *itemType;
    size_t (*getSize)(const void *);
    const void *(*getItem)(const void *, size_t);

    template <typename ItemType>
    TypeDescriptor_StdVector(ItemType *) : TypeDescriptor{"std::vector<>", sizeof(std::vector<ItemType>)}, itemType{TypeResolver<ItemType>::get()} {
        getSize = [](const void *vecPtr) -> size_t {
            const auto &vec = *(const std::vector<ItemType> *)vecPtr;
            return vec.size();
        };
        getItem = [](const void *vecPtr, size_t index) -> const void * {
            const auto &vec = *(const std::vector<ItemType> *)vecPtr;
            return &vec[index];
        };
    }
    virtual std::string getFullName() const override { return std::string("std::vector<") + itemType->getFullName() + ">"; }
    virtual void dump(const void *obj, int indentLevel) const override {
        size_t numItems = getSize(obj);
        std::cout << getFullName();
        if (numItems == 0) {
            std::cout << "{}";
        } else {
            std::cout << "{" << std::endl;
            for (size_t index = 0; index < numItems; index++) {
                std::cout << std::string(4 * (indentLevel + 1), ' ') << "[" << index << "] ";
                itemType->dump(getItem(obj, index), indentLevel + 1);
                std::cout << std::endl;
            }
            std::cout << std::string(4 * indentLevel, ' ') << "}";
        }
    }
};

// Partially specialize TypeResolver<> for std::vectors:
template <typename T>
class TypeResolver<std::vector<T>> {
public:
    static TypeDescriptor *get() {
        static TypeDescriptor_StdVector typeDesc{(T *)nullptr};
        return &typeDesc;
    }
};

}  // namespace reflect

#endif