#ifndef _METADOT_CPP_TLIST_HPP_
#define _METADOT_CPP_TLIST_HPP_

#include "typelist.hpp"

namespace MetaEngine {
template <template <typename...> class... Ts>
struct TemplateList {};

template <typename TList>
struct TLength;
template <typename TList>
constexpr std::size_t TLength_v = TLength<TList>::value;

template <typename TList>
struct TIsEmpty;
template <typename TList>
constexpr bool TIsEmpty_v = TIsEmpty<TList>::value;

// TFront will introduce new template
// template<typename TList> struct TFront;

template <typename TList, template <typename...> class T>
struct TPushFront;
template <typename TList, template <typename...> class T>
using TPushFront_t = typename TPushFront<TList, T>::type;

template <typename TList, template <typename...> class T>
struct TPushBack;
template <typename TList, template <typename...> class T>
using TPushBack_t = typename TPushBack<TList, T>::type;

template <typename TList>
struct TPopFront;
template <typename TList>
using TPopFront_t = typename TPopFront<TList>::type;

// TAt will introduce new template
// template<typename TList, std::size_t N> struct TAt;

template <typename TList, template <typename...> class T>
struct TContain;
template <typename TList, template <typename...> class T>
static constexpr bool TContain_v = TContain<TList, T>::value;

template <typename TList, template <typename I, template <typename...> class X> class Op, typename I>
struct TAccumulate;
template <typename TList, template <typename I, template <typename...> class X> class Op, typename I>
using TAccumulate_t = typename TAccumulate<TList, Op, I>::type;

template <typename TList>
struct TReverse;
template <typename TList>
using TReverse_t = typename TReverse<TList>::type;

template <typename TList0, typename TList1>
struct TConcat;
template <typename TList0, typename TList1>
using TConcat_t = typename TConcat<TList0, TList1>::type;

template <typename TList, template <template <typename...> class T> class Op>
struct TTransform;

template <typename TList, template <template <typename...> class T> class Op>
using TTransform_t = typename TTransform<TList, Op>::type;

// TSelect will introduce new template
// template<typename TList, std::size_t... Indices> struct TSelect;

template <typename TList, typename Instance>
struct TExistGenericity;
template <typename TList, typename Instance>
constexpr bool TExistGenericity_v = TExistGenericity<TList, Instance>::value;

template <typename TList, typename InstanceList>
struct TExistGenericities;
template <typename TList, typename InstanceList>
constexpr bool TExistGenericities_v = TExistGenericities<TList, InstanceList>::value;

template <typename TList, typename ArgList>
struct TInstance;
template <typename TList, typename ArgList>
using TInstance_t = typename TInstance<TList, ArgList>::type;

template <typename TList, typename InstanceList>
struct TCanGeneralizeFromList;
template <typename TList, typename InstanceList>
constexpr bool TCanGeneralizeFromList_v = TCanGeneralizeFromList<TList, InstanceList>::value;
}  // namespace MetaEngine

namespace MetaEngine {
template <template <typename...> class... Ts>
struct TLength<TemplateList<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <typename TList>
struct TIsEmpty : std::bool_constant<TLength_v<TList> == 0> {};

/*
// TFront will introduce new template
template<template<typename...> class Head, template<typename...> class... Tail>
struct TFront<TemplateList<Head, Tail...>> {
    template<typename... Ts>
    using Ttype = Head<Ts...>;
};
*/

template <template <typename...> class T, template <typename...> class... Ts>
struct TPushFront<TemplateList<Ts...>, T> : std::type_identity<TemplateList<T, Ts...>> {};

template <template <typename...> class T, template <typename...> class... Ts>
struct TPushBack<TemplateList<Ts...>, T> : std::type_identity<TemplateList<Ts..., T>> {};

template <template <typename...> class Head, template <typename...> class... Tail>
struct TPopFront<TemplateList<Head, Tail...>> : std::type_identity<TemplateList<Tail...>> {};

/*
// TAt will introduce new template
template<typename TList>
struct TAt<TList, 0> {
    template<typename... Ts>
    using Ttype = typename TFront<TList>::template Ttype<Ts...>;
};

template<typename TList, std::size_t N>
struct TAt : TAt<TPopFront_t<TList>, N - 1> { };
*/

template <template <typename I, template <typename...> class X> class Op, typename I>
struct TAccumulate<TemplateList<>, Op, I> : std::type_identity<I> {};

template <template <typename I, template <typename...> class X> class Op, typename I, template <typename...> class THead, template <typename...> class... TTail>
struct TAccumulate<TemplateList<THead, TTail...>, Op, I> : TAccumulate<TemplateList<TTail...>, Op, typename Op<I, THead>::type> {};

template <typename TList>
struct TReverse : TAccumulate<TList, TPushFront, TemplateList<>> {};

template <typename TList0, typename TList1>
struct TConcat : TAccumulate<TList1, TPushBack, TList0> {};

template <template <template <typename...> class T> class Op, template <typename...> class... Ts>
struct TTransform<TemplateList<Ts...>, Op> : std::type_identity<TemplateList<Op<Ts>::template Ttype...>> {};

// TSelect
// template<typename TList, std::size_t... Indices>
// struct TSelect : std::type_identity<TemplateList<TAt<TList, Indices>::template Ttype...>> {};

template <template <typename...> class... Ts, typename Instance>
struct TExistGenericity<TemplateList<Ts...>, Instance> : std::bool_constant<(is_instance_of_v<Instance, Ts> || ...)> {};

template <typename ArgList, template <typename...> class... Ts>
struct TInstance<TemplateList<Ts...>, ArgList> : std::type_identity<TypeList<Instantiate_t<ArgList, Ts>...>> {};

template <typename TList, typename... Instances>
struct TExistGenericities<TList, TypeList<Instances...>> : std::bool_constant<(TExistGenericity_v<TList, Instances> && ...)> {};

template <typename InstanceList, template <typename...> class... Ts>
struct TCanGeneralizeFromList<TemplateList<Ts...>, InstanceList> : std::bool_constant<(ExistInstance_v<InstanceList, Ts> && ...)> {};

template <template <typename...> class... Ts, template <typename...> class T>
struct TContain<TemplateList<Ts...>, T> : std::bool_constant<(is_same_typename_template_v<Ts, T> || ...)> {};
}  // namespace MetaEngine

#endif