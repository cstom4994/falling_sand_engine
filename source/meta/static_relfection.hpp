#ifndef ME_CPP_SF_HPP
#define ME_CPP_SF_HPP

#include <cassert>
#include <string_view>
#include <tuple>
#include <utility>

#include "core/cpp/templatelist.hpp"
#include "core/cpp/tstr.hpp"
#include "core/cpp/type.hpp"

namespace ME::meta::static_refl {

using namespace MetaEngine;

template <typename Name, typename T>
struct NamedValue;

template <typename Name, typename T>
struct NamedValueBase {
    using TName = Name;
    static constexpr std::string_view name = Name::View();
    static constexpr bool has_value = !std::is_void_v<T>;

    template <typename Str>
    static constexpr bool NameIs(Str = {}) noexcept {
        return std::is_same_v<Str, Name>;
    }

    template <typename U>
    static constexpr bool ValueTypeIs() noexcept {
        return std::is_same_v<T, U>;
    }

    template <typename U>
    static constexpr bool ValueTypeIsSameWith(U) noexcept {
        return ValueTypeIs<U>();
    }
};

template <typename Name, typename T>
struct NamedValue : NamedValueBase<Name, T> {
    T value;

    constexpr NamedValue(Name, T value) : value{value} {}

    template <typename U>
    constexpr bool operator==(U v) const {
        if constexpr (std::is_same_v<T, U>)
            return value == v;
        else
            return false;
    }
};

template <typename Name>
struct NamedValue<Name, void> : NamedValueBase<Name, void> {
    constexpr NamedValue(Name) {}
    template <typename U>
    constexpr bool operator==(U) const {
        return false;
    }
};

template <typename Name>
NamedValue(Name) -> NamedValue<Name, void>;
template <typename T, typename Name>
NamedValue(Name, T) -> NamedValue<Name, T>;

// [summary]
// attribute for field, class, enum
// [member]
// T value (T == void -> no value)
template <typename Name, typename T>
struct Attr;

template <typename Name>
Attr(Name) -> Attr<Name, void>;

template <typename Name, typename Value>
Attr(Name, Value) -> Attr<Name, Value>;

template <typename Name, typename T>
struct Attr : NamedValue<Name, T> {
    constexpr Attr(Name name, T value) : NamedValue<Name, T>{name, value} {}
};

template <typename Name>
struct Attr<Name, void> : NamedValue<Name, void> {
    constexpr Attr(Name name) : NamedValue<Name, void>{name} {}
};

template <typename Signature>
constexpr auto WrapConstructor();
template <typename T>
constexpr auto WrapDestructor();
}  // namespace ME::meta::static_refl

namespace ME::meta::static_refl::detail {
template <typename T, template <typename...> class U>
struct IsInstance : std::false_type {};
template <template <typename...> class U, typename... Ts>
struct IsInstance<U<Ts...>, U> : std::true_type {};

template <typename Ints>
struct IntegerSequenceTraits;

template <typename T, T N0, T... Ns>
struct IntegerSequenceTraits<std::integer_sequence<T, N0, Ns...>> {
    static constexpr std::size_t head = N0;
    static constexpr auto tail = std::integer_sequence<T, Ns...>{};
};

template <typename Signature>
struct ConstructorWrapper;
template <typename T, typename... Args>
struct ConstructorWrapper<T(Args...)> {
    static constexpr auto run() {
        return static_cast<void (*)(T*, Args...)>([](T* ptr, Args... args) {
#ifdef assert
            assert(ptr != nullptr);
#endif
            /*return*/ new (ptr) T{std::forward<Args>(args)...};
        });
    }
};
}  // namespace ME::meta::static_refl::detail

namespace ME::meta::static_refl {
// Signature : T(Args...)
// ->
// void(*)(T*, Args...)
template <typename Signature>
constexpr auto WrapConstructor() {
    return detail::ConstructorWrapper<Signature>::run();
}

template <typename T>
constexpr auto WrapDestructor() {
    return static_cast<void (*)(T*)>([](T* ptr) {
        assert(ptr != nullptr);
        ptr->~T();
    });
}

// Elems is named value
template <typename... Elems>
struct ElemList {
    std::tuple<Elems...> elems;
    static constexpr std::size_t size = sizeof...(Elems);

    constexpr ElemList(Elems... elems) : elems{elems...} {}

    template <typename Acc, typename Func>
    constexpr auto Accumulate(Acc acc, Func&& func) const;

    template <typename Func>
    constexpr void ForEach(Func&& func) const;

    template <typename Func>
    constexpr std::size_t FindIf(Func&& func) const;

    template <typename Name>
    constexpr const auto& Find(Name = {}) const;

    template <typename T>
    constexpr std::size_t FindValue(const T& value) const;

    template <typename T, typename Str>
    constexpr const T* ValuePtrOfName(Str name) const;

    template <typename T, typename Str>
    constexpr const T& ValueOfName(Str name) const {
        return *ValuePtrOfName<T>(name);
    }

    template <typename T, typename Char = char>
    constexpr std::basic_string_view<Char> NameOfValue(const T& value) const;

    template <typename Name>
    static constexpr bool Contains(Name = {});

    template <std::size_t N>
    constexpr const auto& Get() const;

    template <typename Elem>
    constexpr auto Push(Elem e) const;

    template <typename Elem>
    constexpr auto Insert(Elem e) const;

#define MetaDotRefl_ElemList_GetByValue(list, value) list.Get<list.FindValue(value)>()
};
}  // namespace ME::meta::static_refl

namespace ME::meta::static_refl::detail {
template <typename List, typename Func, typename Acc, std::size_t... Ns>
constexpr auto Accumulate(const List& list, Func&& func, Acc acc, std::index_sequence<Ns...>) {
    if constexpr (sizeof...(Ns) > 0) {
        using IST_N = IntegerSequenceTraits<std::index_sequence<Ns...>>;
        return Accumulate(list, std::forward<Func>(func), std::forward<Func>(func)(std::move(acc), list.template Get<IST_N::head>()), IST_N::tail);
    } else
        return acc;
}

template <typename List, typename Func, std::size_t... Ns>
constexpr std::size_t FindIf(const List& list, Func&& func, std::index_sequence<Ns...>) {
    if constexpr (sizeof...(Ns) > 0) {
        using IST = IntegerSequenceTraits<std::index_sequence<Ns...>>;
        return std::forward<Func>(func)(list.template Get<IST::head>()) ? IST::head : FindIf(list, std::forward<Func>(func), IST::tail);
    } else
        return static_cast<std::size_t>(-1);
}

template <typename Name>
struct IsSameNameWith {
    template <typename T>
    struct Ttype : std::is_same<typename T::TName, Name> {};
};
}  // namespace ME::meta::static_refl::detail

namespace ME::meta::static_refl {
template <typename... Elems>
template <typename Init, typename Func>
constexpr auto ElemList<Elems...>::Accumulate(Init init, Func&& func) const {
    return detail::Accumulate(*this, std::forward<Func>(func), std::move(init), std::make_index_sequence<size>{});
}

template <typename... Elems>
template <typename Func>
constexpr void ElemList<Elems...>::ForEach(Func&& func) const {
    Accumulate(0, [&](auto, const auto& field) {
        std::forward<Func>(func)(field);
        return 0;
    });
}

template <typename... Elems>
template <typename Func>
constexpr std::size_t ElemList<Elems...>::FindIf(Func&& func) const {
    return detail::FindIf(*this, std::forward<Func>(func), std::make_index_sequence<size>{});
}

template <typename... Elems>
template <typename Name>
constexpr const auto& ElemList<Elems...>::Find(Name) const {
    /*static_assert(Contains<Name>());
    constexpr std::size_t idx = []() {
        constexpr decltype(Name::View()) names[]{ Elems::name... };
        for (std::size_t i = 0; i < sizeof...(Elems); i++) {
            if (Name::View() == names[i])
                return i;
        }
        return static_cast<std::size_t>(-1);
    }();
    static_assert(idx != static_cast<std::size_t>(-1));
    return Get<idx>();*/
    return Get<ME::FindIf_v<TypeList<Elems...>, detail::IsSameNameWith<Name>::template Ttype>>();
}

template <typename... Elems>
template <typename T>
constexpr std::size_t ElemList<Elems...>::FindValue(const T& value) const {
    return FindIf([&value](const auto& e) { return e == value; });
}

template <typename... Elems>
template <typename T, typename Str>
constexpr const T* ElemList<Elems...>::ValuePtrOfName(Str name) const {
    const T* value = nullptr;
    FindIf([name, &value](const auto& ele) {
        using Elem = std::decay_t<decltype(ele)>;
        if constexpr (Elem::template ValueTypeIs<T>()) {
            if (ele.name == name) {
                value = &ele.value;
                return true;
            } else
                return false;
        } else
            return false;
    });
    return value;
}

template <typename... Elems>
template <typename T, typename Char>
constexpr std::basic_string_view<Char> ElemList<Elems...>::NameOfValue(const T& value) const {
    std::basic_string_view<Char> name;
    FindIf([value, &name](auto ele) {
        using Elem = std::decay_t<decltype(ele)>;
        if constexpr (Elem::template ValueTypeIs<T>()) {
            if (ele.value == value) {
                name = Elem::name;
                return true;
            } else
                return false;
        } else
            return false;
    });
    return name;
}

template <typename... Elems>
template <typename Name>
constexpr bool ElemList<Elems...>::Contains(Name) {
    return (Elems::template NameIs<Name>() || ...);
}

template <typename... Elems>
template <std::size_t N>
constexpr const auto& ElemList<Elems...>::Get() const {
    return std::get<N>(elems);
}

template <typename... Elems>
template <typename Elem>
constexpr auto ElemList<Elems...>::Push(Elem e) const {
    return std::apply([e](const auto&... elems) { return ElemList<Elems..., Elem>{elems..., e}; }, elems);
}

template <typename... Elems>
template <typename Elem>
constexpr auto ElemList<Elems...>::Insert(Elem e) const {
    if constexpr ((std::is_same_v<Elems, Elem> || ...))
        return *this;
    else
        return Push(e);
}

template <typename... Attrs>
struct AttrList : ElemList<Attrs...> {
    constexpr AttrList(Attrs... attrs) : ElemList<Attrs...>{attrs...} {}
};

// TypeInfoBase, attrs, fields
template <typename T>
struct TypeInfo;

template <typename T, bool IsVirtual = false>
struct Base {
    static constexpr auto info = TypeInfo<T>{};
    static constexpr bool is_virtual = IsVirtual;
};

template <typename... Bases>
struct BaseList : ElemList<Bases...> {
    constexpr BaseList(Bases... bases) : ElemList<Bases...>{bases...} {};
};
}  // namespace ME::meta::static_refl

namespace ME::meta::static_refl::detail {
template <typename T>
struct FieldTraits;

template <typename Obj, typename Value, bool isStatic, bool isFunction>
struct FieldTraitsBase {
    using object_type = Obj;
    using value_type = Value;
    static constexpr bool is_static = isStatic;
    static constexpr bool is_func = isFunction;
};

// non-static member pointer
template <typename Object, typename T>
struct FieldTraits<T Object::*> : FieldTraitsBase<Object, T, false, std::is_function_v<T>> {};

// static member pointer
template <typename T>
struct FieldTraits<T*> : FieldTraitsBase<void, T, true, std::is_function_v<T>> {};

// enum / static constexpr
template <typename T>
struct FieldTraits : FieldTraitsBase<void, T, true, false> {};
}  // namespace ME::meta::static_refl::detail

namespace ME::meta::static_refl {
template <typename Name, typename T, typename AList>
struct Field : detail::FieldTraits<T>, NamedValue<Name, T> {
    static_assert(detail::IsInstance<AList, AttrList>::value);
    static_assert(!std::is_void_v<T>);

    AList attrs;

    constexpr Field(Name name, T value, AList attrs = {}) : NamedValue<Name, T>{name, value}, attrs{attrs} {}
};

template <typename Name, typename T>
Field(Name, T) -> Field<Name, T, AttrList<>>;

// Field's (name, value_type) must be unique
template <typename... Fields>
struct FieldList : ElemList<Fields...> {
    constexpr FieldList(Fields... fields) : ElemList<Fields...>{fields...} {};
};

template <typename T, typename... Bases>
struct TypeInfoBase {
    using Type = T;
    using TName = decltype(type_name<T>());
    static constexpr std::string_view name = TName::View();
    static constexpr BaseList bases = {Bases{}...};

    template <typename Derived>
    static constexpr auto&& Forward(Derived&& derived) noexcept;

    static constexpr auto VirtualBases();

    template <typename Init, typename Func>
    static constexpr auto DFS_Acc(Init&& init, Func&& func);

    template <typename Func>
    static constexpr void DFS_ForEach(Func&& func);

    template <typename U, typename Func>
    static constexpr void ForEachVarOf(U&& obj, Func&& func);
};
}  // namespace ME::meta::static_refl

namespace ME::meta::static_refl::detail {
template <typename TI, typename U, typename Func>
constexpr void ForEachNonVirtualVarOf(TI info, U&& obj, Func&& func) {
    info.fields.ForEach([&](const auto& field) {
        using Fld = std::remove_const_t<std::remove_reference_t<decltype(field)>>;
        if constexpr (!Fld::is_static && !Fld::is_func) std::forward<Func>(func)(field, std::forward<U>(obj).*(field.value));
    });
    info.bases.ForEach([&](auto base) {
        if constexpr (!base.is_virtual) {
            ForEachNonVirtualVarOf(base.info, base.info.Forward(std::forward<U>(obj)), std::forward<Func>(func));
        }
    });
}
}  // namespace ME::meta::static_refl::detail

namespace ME::meta::static_refl {
template <typename T, typename... Bases>
template <typename Derived>
constexpr auto&& TypeInfoBase<T, Bases...>::Forward(Derived&& derived) noexcept {
    static_assert(std::is_base_of_v<Type, std::decay_t<Derived>>);
    using DecayDerived = std::decay_t<Derived>;
    if constexpr (std::is_same_v<const DecayDerived&, Derived>)
        return static_cast<const Type&>(derived);
    else if constexpr (std::is_same_v<DecayDerived&, Derived>)
        return static_cast<Type&>(derived);
    else if constexpr (std::is_same_v<DecayDerived, Derived>)
        return static_cast<Type&&>(derived);
    else
        static_assert(true);  // volitile
}

template <typename T, typename... Bases>
constexpr auto TypeInfoBase<T, Bases...>::VirtualBases() {
    return bases.Accumulate(ElemList<>{}, [](auto acc, auto base) {
        constexpr auto vbs = base.info.VirtualBases();
        auto concated = vbs.Accumulate(acc, [](auto acc, auto vb) { return acc.Insert(vb); });
        if constexpr (base.is_virtual)
            return concated.Insert(base.info);
        else
            return concated;
    });
}

template <std::size_t Depth, typename T, typename Acc, typename Func>
constexpr auto detail_DFS_Acc(T type, Acc&& acc, Func&& func) {
    return type.bases.Accumulate(std::forward<Acc>(acc), [&](auto&& acc, auto base) {
        if constexpr (base.is_virtual) {
            return detail_DFS_Acc<Depth + 1>(base.info, std::forward<decltype(acc)>(acc), std::forward<Func>(func));
        } else {
            return detail_DFS_Acc<Depth + 1>(base.info, std::forward<Func>(func)(std::forward<decltype(acc)>(acc), base.info, Depth + 1), std::forward<Func>(func));
        }
    });
}

template <typename T, typename... Bases>
template <typename Init, typename Func>
constexpr auto TypeInfoBase<T, Bases...>::DFS_Acc(Init&& init, Func&& func) {
    return detail_DFS_Acc<0>(TypeInfo<Type>{},
                             VirtualBases().Accumulate(std::forward<Func>(func)(std::forward<Init>(init), TypeInfo<Type>{}, 0),
                                                       [&](auto&& acc, auto vb) { return std::forward<Func>(func)(std::forward<decltype(acc)>(acc), vb, 1); }),
                             std::forward<Func>(func));
}

template <typename T, typename... Bases>
template <typename Func>
constexpr void TypeInfoBase<T, Bases...>::DFS_ForEach(Func&& func) {
    DFS_Acc(0, [&](auto, auto t, std::size_t depth) {
        std::forward<Func>(func)(t, depth);
        return 0;
    });
}

template <typename T, typename... Bases>
template <typename U, typename Func>
constexpr void TypeInfoBase<T, Bases...>::ForEachVarOf(U&& obj, Func&& func) {
    static_assert(std::is_same_v<Type, std::decay_t<U>>);
    VirtualBases().ForEach([&](auto vb) {
        vb.fields.ForEach([&](const auto& field) {
            using Fld = std::remove_const_t<std::remove_reference_t<decltype(field)>>;
            if constexpr (!Fld::is_static && !Fld::is_func) std::forward<Func>(func)(field, std::forward<U>(obj).*(field.value));
        });
    });
    detail::ForEachNonVirtualVarOf(TypeInfo<Type>{}, std::forward<U>(obj), std::forward<Func>(func));
}
}  // namespace ME::meta::static_refl

#endif