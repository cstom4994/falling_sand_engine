// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.


#pragma once

#include "imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#if defined (_WIN32)
#define _METADOT_IMM32
#else
#include <sys/stat.h>
#endif

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl.h"

#ifndef IMGUI_AUTO_TREE_MAX_ELEMENT_SIZE
#define IMGUI_AUTO_TREE_MAX_ELEMENT_SIZE sizeof(std::string)//larger values generate less tree nodes
#endif
#ifndef IMGUI_AUTO_TREE_MAX_TUPLE_ELEMENTS
#define IMGUI_AUTO_TREE_MAX_TUPLE_ELEMENTS 3//larger values generate less tree nodes
#endif


#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

#include "Engine/Core.hpp"

#include "Engine/ImGuiHelper.inc"


#include <boost/pfr/core.hpp>
#include <tuple>
#include <typeinfo>
#define IMGUI_AUTO_STRUCT_TO_TUPLE boost::pfr::structure_tie

#if __cplusplus <= 201402L
#error "TODO: fix for this compiler! (at least C++14 is required)"
#endif

namespace ImGui {
    //		IMGUI::AUTO()
    //		=============

    // This is the function that this libary implements. (It's just a wrapper around the class ImGui::Auto_t<AnyType>)
    template<typename AnyType>
    void Auto(AnyType &anything, const std::string &name = std::string());

    //		HELPER FUNCTIONS
    //		================

    //same as std::as_const in c++17
    template<class T>
    constexpr std::add_const_t<T> &as_const(T &t) noexcept { return t; }

#pragma region DETAIL
    namespace detail {

        template<typename T>
        bool AutoExpand(const std::string &name, T &value);
        template<typename Container>
        bool AutoContainerTreeNode(const std::string &name, Container &cont);
        template<typename Container>
        bool AutoContainerValues(const std::string &name, Container &cont);//Container must have .size(), .begin() and .end() methods and ::value_type.
        template<typename Container>
        bool AutoMapContainerValues(const std::string &name, Container &map);// Same as above but that iterates over pairs
        template<typename Container>
        void AutoContainerPushFrontButton(Container &cont);
        template<typename Container>
        void AutoContainerPushBackButton(Container &cont);
        template<typename Container>
        void AutoContainerPopFrontButton(Container &cont);
        template<typename Container>
        void AutoContainerPopBackButton(Container &cont);
        template<typename Key, typename Value>
        void AutoMapKeyValue(Key &key, Value &value);

#ifdef _TUPLE_//For tuples

        template<class T>
        constexpr std::add_const_t<T> &as_const(T &t) noexcept { return t; }//same as std::as_const in c++17
        template<std::size_t I, typename... Args>
        void AutoTupleRecurse(std::tuple<Args...> &tpl, std::enable_if_t<0 != I> * = 0);
        template<std::size_t I, typename... Args>
        inline void AutoTupleRecurse(std::tuple<Args...> &tpl, std::enable_if_t<0 == I> * = 0) {}//End of recursion.
        template<std::size_t I, typename... Args>
        void AutoTupleRecurse(const std::tuple<Args...> &tpl, std::enable_if_t<0 != I> * = 0);
        template<std::size_t I, typename... Args>
        inline void AutoTupleRecurse(const std::tuple<Args...> &tpl, std::enable_if_t<0 == I> * = 0) {}//End of recursion.
        template<typename... Args>
        void AutoTuple(const std::string &name, std::tuple<Args...> &tpl);
        template<typename... Args>
        void AutoTuple(const std::string &name, const std::tuple<Args...> &tpl);
#endif// _TUPLE_

        template<typename T, std::size_t N>
        using c_array_t = T[N];//so arrays are regular types and can be used in macro

        //template<typename Container> inline std::size_t AutoContainerSize(Container &cont, std::enable_if_t< std::is_array_v<Container>>*=0){	return sizeof(Container) / sizeof(decltype(*std::begin(cont)));}
        //template<typename Container> inline std::size_t AutoContainerSize(Container &cont, std::enable_if_t<!std::is_array_v<Container>> *= 0) { return cont.size(); }

    }// namespace detail
#pragma endregion

    //		PRIMARY TEMPLATE
    //		================
    // This implements the struct to tuple scenario
    template<typename AnyType>
    struct Auto_t
    {
        static void Auto(AnyType &anything, const std::string &name) {
#ifndef IMGUI_AUTO_STRUCT_TO_TUPLE
            static_assert(false, "TODO: fix for this compiler! (at least C++14 is required)")
#endif
            /*
                static_assert(!std::is_reference_v<AnyType> && std::is_copy_constructible_v<std::remove_all_extents_t<AnyType>> && !std::is_polymorphic_v<AnyType> &&
                boost::pfr::detail::is_aggregate_initializable_n<AnyType,
                boost::pfr::detail::detect_fields_count_dispatch<AnyType>(boost::pfr::detail::size_t_<sizeof(AnyType)*8>{}, 1L)
                >::value,		// If the above is not a constexpr expression, you are yousing an invalid type
                "This type cannot be converted to a tuple.");
                */

            //    auto tuple = IMGUI_AUTO_STRUCT_TO_TUPLE(anything);
            //ImGui::detail::AutoTuple("Struct " + name, tuple);
        }
    };//ImGui::Auto_t<>::Auto()
}// namespace ImGui

// Implementation of ImGui::Auto()
template<typename AnyType>
inline void ImGui::Auto(AnyType &anything, const std::string &name) {
    ImGui::Auto_t<AnyType>::Auto(anything, name);
}

//		HELPER FUNCTIONS
//		================

#pragma region UTILS

template<typename T>
bool ImGui::detail::AutoExpand(const std::string &name, T &value) {
    if (sizeof(T) <= IMGUI_AUTO_TREE_MAX_ELEMENT_SIZE) {
        ImGui::PushID(name.c_str());
        ImGui::Bullet();
        ImGui::Auto_t<T>::Auto(value, name);
        ImGui::PopID();
        return true;
    } else if (ImGui::TreeNode(name.c_str())) {
        ImGui::Auto_t<T>::Auto(value, name);
        ImGui::TreePop();
        return true;
    } else
        return false;
}

template<typename Container>
bool ImGui::detail::AutoContainerTreeNode(const std::string &name, Container &cont) {
    //std::size_t size = ImGui::detail::AutoContainerSize(cont);
    std::size_t size = cont.size();
    if (ImGui::CollapsingHeader(name.c_str())) {
        size_t elemsize = sizeof(decltype(*std::begin(cont)));
        ImGui::Text("size = %d, non dynamic elemsize = %d bytes", size, elemsize);
        return true;
    } else {
        float label_width = CalcTextSize(name.c_str()).x + ImGui::GetTreeNodeToLabelSpacing() + 5;
        std::string sizetext = "(size = " + std::to_string(size) + ')';
        float sizet_width = CalcTextSize(sizetext.c_str()).x;
        float avail_width = ImGui::GetContentRegionAvail().x;
        if (avail_width > label_width + sizet_width) {
            ImGui::SameLine(avail_width - sizet_width);
            ImGui::TextUnformatted(sizetext.c_str());
        }
        return false;
    }
}
template<typename Container>
bool ImGui::detail::AutoContainerValues(const std::string &name, Container &cont) {
    if (ImGui::detail::AutoContainerTreeNode(name, cont)) {
        ImGui::Indent();
        ImGui::PushID(name.c_str());
        std::size_t i = 0;
        for (auto &elem: cont) {
            std::string itemname = "[" + std::to_string(i) + ']';
            ImGui::detail::AutoExpand(itemname, elem);
            ++i;
        }
        ImGui::PopID();
        ImGui::Unindent();
        return true;
    } else
        return false;
}
template<typename Container>
bool ImGui::detail::AutoMapContainerValues(const std::string &name, Container &cont) {
    if (ImGui::detail::AutoContainerTreeNode(name, cont)) {
        ImGui::Indent();
        std::size_t i = 0;
        for (auto &elem: cont) {
            ImGui::PushID(i);
            AutoMapKeyValue(elem.first, elem.second);
            ImGui::PopID();
            ++i;
        }
        ImGui::Unindent();
        return true;
    } else
        return false;
}
template<typename Container>
void ImGui::detail::AutoContainerPushFrontButton(Container &cont) {
    if (ImGui::SmallButton("Push Front")) cont.emplace_front();
}
template<typename Container>
void ImGui::detail::AutoContainerPushBackButton(Container &cont) {
    if (ImGui::SmallButton("Push Back ")) cont.emplace_back();
}
template<typename Container>
void ImGui::detail::AutoContainerPopFrontButton(Container &cont) {
    if (!cont.empty() && ImGui::SmallButton("Pop Front ")) cont.pop_front();
}
template<typename Container>
void ImGui::detail::AutoContainerPopBackButton(Container &cont) {
    if (!cont.empty() && ImGui::SmallButton("Pop Back  ")) cont.pop_back();
}
template<typename Key, typename Value>
void ImGui::detail::AutoMapKeyValue(Key &key, Value &value) {
    bool b_k = sizeof(Key) <= IMGUI_AUTO_TREE_MAX_ELEMENT_SIZE;
    bool b_v = sizeof(Value) <= IMGUI_AUTO_TREE_MAX_ELEMENT_SIZE;
    if (b_k) {
        ImGui::TextUnformatted("[");
        ImGui::SameLine();
        ImGui::Auto_t<Key>::Auto(key, "");
        ImGui::SameLine();
        ImGui::TextUnformatted("]");
        if (b_v) ImGui::SameLine();
        ImGui::Auto_t<Value>::Auto(value, "Value");
    } else {
        ImGui::Auto_t<Key>::Auto(key, "Key");
        ImGui::Auto_t<Value>::Auto(value, "Value");
    }
}

#if defined(_MSC_VER)

template<std::size_t I, typename... Args>
void ImGui::detail::AutoTupleRecurse(std::tuple<Args...> &tpl, std::enable_if_t<0 != I> *) {
    ImGui::detail::AutoTupleRecurse<I - 1, Args...>(tpl);// first draw smaller indeces
    using type = decltype(std::get<I - 1>(tpl));
    std::string str = '<' + std::to_string(I) + ">: " + (std::is_const_v<type> ? "const " : "") + typeid(type).name();
    ImGui::detail::AutoExpand(str, std::get<I - 1>(tpl));
}
template<std::size_t I, typename... Args>
void ImGui::detail::AutoTupleRecurse(const std::tuple<Args...> &tpl, std::enable_if_t<0 != I> *) {
    ImGui::detail::AutoTupleRecurse<I - 1, const Args...>(tpl);// first draw smaller indeces
    using type = decltype(std::get<I - 1>(tpl));
    std::string str = '<' + std::to_string(I) + ">: " + "const " + typeid(type).name();
    ImGui::detail::AutoExpand(str, ImGui::as_const(std::get<I - 1>(tpl)));
}
template<typename... Args>
void ImGui::detail::AutoTuple(const std::string &name, std::tuple<Args...> &tpl) {
    constexpr std::size_t tuple_size = sizeof(decltype(tpl));
    constexpr std::size_t tuple_numelems = sizeof...(Args);
    if (tuple_size <= IMGUI_AUTO_TREE_MAX_ELEMENT_SIZE && tuple_numelems <= IMGUI_AUTO_TREE_MAX_TUPLE_ELEMENTS) {
        ImGui::TextUnformatted((name + " (" + std::to_string(tuple_size) + " bytes)").c_str());
        ImGui::PushID(name.c_str());
        ImGui::Indent();
        ImGui::detail::AutoTupleRecurse<tuple_numelems, Args...>(tpl);
        ImGui::Unindent();
        ImGui::PopID();
    } else if (ImGui::TreeNode((name + " (" + std::to_string(tuple_size) + " bytes)").c_str())) {
        ImGui::detail::AutoTupleRecurse<tuple_numelems, Args...>(tpl);
        ImGui::TreePop();
    }
}
template<typename... Args>
void ImGui::detail::AutoTuple(const std::string &name, const std::tuple<Args...> &tpl)//same but const
{
    constexpr std::size_t tuple_size = sizeof(std::tuple<Args...>);
    constexpr std::size_t tuple_numelems = sizeof...(Args);
    if (tuple_size <= IMGUI_AUTO_TREE_MAX_ELEMENT_SIZE && tuple_numelems <= IMGUI_AUTO_TREE_MAX_TUPLE_ELEMENTS) {
        ImGui::TextUnformatted((name + " !(" + std::to_string(tuple_size) + " bytes)").c_str());
        ImGui::PushID(name.c_str());
        ImGui::Indent();
        ImGui::detail::AutoTupleRecurse<tuple_numelems, Args...>(tpl);
        ImGui::Unindent();
        ImGui::PopID();
    } else if (ImGui::TreeNode((name + " (" + std::to_string(tuple_size) + " bytes)").c_str())) {
        ImGui::detail::AutoTupleRecurse<tuple_numelems, Args...>(tpl);
        ImGui::TreePop();
    }
}

#endif

#pragma endregion

//		HELPER MACROS
//		=============

#define UNPACK(...) __VA_ARGS__//for unpacking parentheses. It is needed for macro arguments with commmas
//Enclose templatespec, AND typespec in parentheses in this version. Useful if there are commas in the argument.
#define IMGUI_AUTO_DEFINE_BEGIN_P(templatespec, typespec)  \
    namespace ImGui {                                      \
        UNPACK templatespec struct Auto_t<UNPACK typespec> \
        {                                                  \
            static void Auto(UNPACK typespec &var, const std::string &name) {
//If macro arguments have no commmas inside use this version without parentheses
#define IMGUI_AUTO_DEFINE_BEGIN(templatespec, typespec) IMGUI_AUTO_DEFINE_BEGIN_P((templatespec), (typespec))//when there are no commas in types, use this without parentheses
#define IMGUI_AUTO_DEFINE_END \
    }                         \
    }                         \
    ;                         \
    }
#define IMGUI_AUTO_DEFINE_INLINE_P(template_spec, type_spec, code) IMGUI_AUTO_DEFINE_BEGIN_P(template_spec, type_spec) code IMGUI_AUTO_DEFINE_END
#define IMGUI_AUTO_DEFINE_INLINE(template_spec, type_spec, code) IMGUI_AUTO_DEFINE_INLINE_P((template_spec), (type_spec), code)


#pragma region ImplBase

#ifndef IMGUI_AUTO_INPUT_FLOAT1
#define IMGUI_AUTO_INPUT_FLOAT1 ImGui::DragFloat
#endif
#ifndef IMGUI_AUTO_INPUT_FLOAT2
#define IMGUI_AUTO_INPUT_FLOAT2 ImGui::DragFloat2
#endif
#ifndef IMGUI_AUTO_INPUT_FLOAT3
#define IMGUI_AUTO_INPUT_FLOAT3 ImGui::DragFloat3
#endif
#ifndef IMGUI_AUTO_INPUT_FLOAT4
#define IMGUI_AUTO_INPUT_FLOAT4 ImGui::DragFloat4
#endif
#ifndef IMGUI_AUTO_INPUT_INT1
#define IMGUI_AUTO_INPUT_INT1 ImGui::InputInt
#endif
#ifndef IMGUI_AUTO_INPUT_INT2
#define IMGUI_AUTO_INPUT_INT2 ImGui::InputInt2
#endif
#ifndef IMGUI_AUTO_INPUT_INT3
#define IMGUI_AUTO_INPUT_INT3 ImGui::InputInt3
#endif
#ifndef IMGUI_AUTO_INPUT_INT4
#define IMGUI_AUTO_INPUT_INT4 ImGui::InputInt4
#endif
#ifndef IMGUI_AUTO_NULLPTR_COLOR
#define IMGUI_AUTO_NULLPTR_COLOR ImVec4(1.0, 0.5, 0.5, 1.0)
#endif

//		SPECIALIZATIONS
//		===============

#pragma region STRINGS

IMGUI_AUTO_DEFINE_BEGIN(template<>, const char *)
if (name.empty()) ImGui::TextUnformatted(var);
else
    ImGui::Text("%s=%s", name.c_str(), var);
IMGUI_AUTO_DEFINE_END

IMGUI_AUTO_DEFINE_BEGIN_P((template<std::size_t N>), (const detail::c_array_t<char, N>) )
if (name.empty()) ImGui::TextUnformatted(var, var + N - 1);
else
    ImGui::Text("%s=%s", name.c_str(), var);
IMGUI_AUTO_DEFINE_END

IMGUI_AUTO_DEFINE_INLINE(template<>, char *, const char *tmp = var; ImGui::Auto_t<const char *>::Auto(tmp, name);)
IMGUI_AUTO_DEFINE_INLINE(template<>, char *const, const char *tmp = var; ImGui::Auto_t<const char *>::Auto(tmp, name);)
IMGUI_AUTO_DEFINE_INLINE(template<>, const char *const, const char *tmp = var; ImGui::Auto_t<const char *>::Auto(tmp, name);)
IMGUI_AUTO_DEFINE_BEGIN(template<>, std::string)
const std::size_t lines = var.find('\n');
if (var.find('\n') != std::string::npos) ImGui::InputTextMultiline(name.c_str(), const_cast<char *>(var.c_str()), 256);
else
    ImGui::InputText(name.c_str(), const_cast<char *>(var.c_str()), 256);
IMGUI_AUTO_DEFINE_END

IMGUI_AUTO_DEFINE_BEGIN(template<>, const std::string)
if (name.empty()) ImGui::TextUnformatted(var.c_str(), var.c_str() + var.length());
else
    ImGui::Text("%s=%s", name.c_str(), var.c_str());
IMGUI_AUTO_DEFINE_END

IMGUI_AUTO_DEFINE_BEGIN(template<>, MarkdownData)
static test_markdown s_printer;
s_printer.print(var.data.c_str());
IMGUI_AUTO_DEFINE_END

#pragma endregion

#pragma region NUMBERS

IMGUI_AUTO_DEFINE_INLINE(template<>, float, IMGUI_AUTO_INPUT_FLOAT1(name.c_str(), &var);)
IMGUI_AUTO_DEFINE_INLINE(template<>, int, IMGUI_AUTO_INPUT_INT1(name.c_str(), &var);)
IMGUI_AUTO_DEFINE_INLINE(template<>, unsigned int, IMGUI_AUTO_INPUT_INT1(name.c_str(), (int *) &var);)
IMGUI_AUTO_DEFINE_INLINE(template<>, bool, ImGui::Checkbox(name.c_str(), &var);)
IMGUI_AUTO_DEFINE_INLINE(template<>, ImVec2, IMGUI_AUTO_INPUT_FLOAT2(name.c_str(), &var.x);)
IMGUI_AUTO_DEFINE_INLINE(template<>, ImVec4, IMGUI_AUTO_INPUT_FLOAT4(name.c_str(), &var.x);)
IMGUI_AUTO_DEFINE_INLINE(template<>, const float, ImGui::Auto_t<const std::string>::Auto(std::to_string(var), name);)
IMGUI_AUTO_DEFINE_INLINE(template<>, const int, ImGui::Auto_t<const std::string>::Auto(std::to_string(var), name);)
IMGUI_AUTO_DEFINE_INLINE(template<>, const unsigned, ImGui::Auto_t<const std::string>::Auto(std::to_string(var), name);)
IMGUI_AUTO_DEFINE_INLINE(template<>, const bool, ImGui::Auto_t<const std::string>::Auto(std::to_string(var), name);)
IMGUI_AUTO_DEFINE_INLINE(template<>, const ImVec2, ImGui::Text("%s(%f,%f)", (name.empty() ? "" : name + "=").c_str(), var.x, var.y);)
IMGUI_AUTO_DEFINE_INLINE(template<>, const ImVec4, ImGui::Text("%s(%f,%f,%f,%f)", (name.empty() ? "" : name + "=").c_str(), var.x, var.y, var.z, var.w);)

IMGUI_AUTO_DEFINE_INLINE_P((template<>), (detail::c_array_t<float, 1>), IMGUI_AUTO_INPUT_FLOAT1(name.c_str(), &var[0]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (const detail::c_array_t<float, 1>), ImGui::Text("%s%f", (name.empty() ? "" : name + "=").c_str(), var[0]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (detail::c_array_t<float, 2>), IMGUI_AUTO_INPUT_FLOAT2(name.c_str(), &var[0]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (const detail::c_array_t<float, 2>), ImGui::Text("%s(%f,%f)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (detail::c_array_t<float, 3>), IMGUI_AUTO_INPUT_FLOAT3(name.c_str(), &var[0]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (const detail::c_array_t<float, 3>), ImGui::Text("%s(%f,%f,%f)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1], var[2]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (detail::c_array_t<float, 4>), IMGUI_AUTO_INPUT_FLOAT4(name.c_str(), &var[0]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (const detail::c_array_t<float, 4>), ImGui::Text("%s(%f,%f,%f,%f)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1], var[2], var[3]);)

IMGUI_AUTO_DEFINE_INLINE_P((template<>), (detail::c_array_t<int, 1>), IMGUI_AUTO_INPUT_INT1(name.c_str(), &var[0]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (const detail::c_array_t<int, 1>), ImGui::Text("%s%d", (name.empty() ? "" : name + "=").c_str(), var[0]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (detail::c_array_t<int, 2>), IMGUI_AUTO_INPUT_INT2(name.c_str(), &var[0]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (const detail::c_array_t<int, 2>), ImGui::Text("%s(%d,%d)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (detail::c_array_t<int, 3>), IMGUI_AUTO_INPUT_INT3(name.c_str(), &var[0]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (const detail::c_array_t<int, 3>), ImGui::Text("%s(%d,%d,%d)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1], var[2]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (detail::c_array_t<int, 4>), IMGUI_AUTO_INPUT_INT4(name.c_str(), &var[0]);)
IMGUI_AUTO_DEFINE_INLINE_P((template<>), (const detail::c_array_t<int, 4>), ImGui::Text("%s(%d,%d,%d,%d)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1], var[2], var[3]);)

#pragma endregion

#pragma region POINTERS and ARRAYS
IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, T *)
if (var != nullptr) ImGui::detail::AutoExpand<T>("Pointer " + name, *var);
else
    ImGui::TextColored(IMGUI_AUTO_NULLPTR_COLOR, "%s=NULL", name.c_str());
IMGUI_AUTO_DEFINE_END
IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, T *const)
if (var != nullptr) ImGui::detail::AutoExpand<T>("Pointer " + name, *var);
else
    ImGui::TextColored(IMGUI_AUTO_NULLPTR_COLOR, "%s=NULL", name.c_str());
IMGUI_AUTO_DEFINE_END
#ifdef _ARRAY_
IMGUI_AUTO_DEFINE_INLINE_P((template<typename T, std::size_t N>), (std::array<T, N>), ImGui::detail::AutoContainerValues("array " + name, var);)
IMGUI_AUTO_DEFINE_INLINE_P((template<typename T, std::size_t N>), (const std::array<T, N>), ImGui::detail::AutoContainerValues("array " + name, var);)
IMGUI_AUTO_DEFINE_INLINE_P((template<typename T, std::size_t N>), (detail::c_array_t<T, N>), ImGui::detail::AutoContainerValues("Array " + name, *(std::array<T, N> *) (&var));)
IMGUI_AUTO_DEFINE_INLINE_P((template<typename T, std::size_t N>), (const detail::c_array_t<T, N>), ImGui::detail::AutoContainerValues("Array " + name, *(const std::array<T, N> *) (&var));)
#endif

#pragma endregion

#pragma region PAIRS and TUPLES

IMGUI_AUTO_DEFINE_BEGIN_P((template<typename T1, typename T2>), (std::pair<T1, T2>) )
if ((std::is_fundamental_v<T1> || std::is_same_v<std::string, T1>) &&(std::is_fundamental_v<T2> || std::is_same_v<std::string, T2>) ) {
    float width = ImGui::CalcItemWidth();
    ImGui::PushItemWidth(width * 0.4 - 10);//a bit less than half
    ImGui::detail::AutoExpand<T1>(name + ".first", var.first);
    ImGui::SameLine();
    ImGui::detail::AutoExpand<T2>(name + ".second", var.second);
    ImGui::PopItemWidth();
} else {
    ImGui::detail::AutoExpand<T1>(name + ".first", var.first);
    ImGui::detail::AutoExpand<T2>(name + ".second", var.second);
}

IMGUI_AUTO_DEFINE_END

IMGUI_AUTO_DEFINE_BEGIN_P((template<typename T1, typename T2>), (const std::pair<T1, T2>) )
ImGui::detail::AutoExpand<const T1>(name + ".first", var.first);
if (std::is_fundamental_v<T1> && std::is_fundamental_v<T2>) ImGui::SameLine();
ImGui::detail::AutoExpand<const T2>(name + ".second", var.second);
IMGUI_AUTO_DEFINE_END

#ifdef _TUPLE_
IMGUI_AUTO_DEFINE_INLINE(template<typename... Args>, std::tuple<Args...>, ImGui::detail::AutoTuple("Tuple " + name, var);)
IMGUI_AUTO_DEFINE_INLINE(template<typename... Args>, const std::tuple<Args...>, ImGui::detail::AutoTuple("Tuple " + name, var);)
#endif//_TUPLE_

#pragma endregion

#pragma region CONTAINERS
#ifdef _VECTOR_
IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, std::vector<T>)
if (ImGui::detail::AutoContainerValues<std::vector<T>>("Vector " + name, var)) {
    ImGui::PushID(name.c_str());
    ImGui::Indent();
    ImGui::detail::AutoContainerPushBackButton(var);
    if (!var.empty()) ImGui::SameLine();
    ImGui::detail::AutoContainerPopBackButton(var);
    ImGui::PopID();
    ImGui::Unindent();
}
IMGUI_AUTO_DEFINE_END
IMGUI_AUTO_DEFINE_BEGIN(template<>, std::vector<bool>)
if (ImGui::detail::AutoContainerTreeNode<std::vector<bool>>("Vector " + name, var)) {
    ImGui::Indent();
    for (int i = 0; i < var.size(); ++i) {
        bool b = var[i];
        ImGui::Bullet();
        ImGui::Auto_t<bool>::Auto(b, '[' + std::to_string(i) + ']');
        var[i] = b;
    }
    ImGui::PushID(name.c_str());
    ImGui::Indent();
    ImGui::detail::AutoContainerPushBackButton(var);
    if (!var.empty()) ImGui::SameLine();
    ImGui::detail::AutoContainerPopBackButton(var);
    ImGui::PopID();
    ImGui::Unindent();
    ImGui::Unindent();
}
IMGUI_AUTO_DEFINE_END

IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, const std::vector<T>)
ImGui::detail::AutoContainerValues<const std::vector<T>>("Vector " + name, var);
IMGUI_AUTO_DEFINE_END
IMGUI_AUTO_DEFINE_BEGIN(template<>, const std::vector<bool>)
if (ImGui::detail::AutoContainerTreeNode<const std::vector<bool>>("Vector " + name, var)) {
    ImGui::Indent();
    for (int i = 0; i < var.size(); ++i) {
        ImGui::Bullet();
        ImGui::Auto_t<const bool>::Auto(var[i], '[' + std::to_string(i) + ']');
    }
    ImGui::Unindent();
}
IMGUI_AUTO_DEFINE_END
#endif

#ifdef _LIST_
IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, std::list<T>)
if (ImGui::detail::AutoContainerValues<std::list<T>>("List " + name, var)) {
    ImGui::PushID(name.c_str());
    ImGui::Indent();
    ImGui::detail::AutoContainerPushFrontButton(var);
    ImGui::SameLine();
    ImGui::detail::AutoContainerPushBackButton(var);
    ImGui::detail::AutoContainerPopFrontButton(var);
    if (!var.empty()) ImGui::SameLine();
    ImGui::detail::AutoContainerPopBackButton(var);
    ImGui::PopID();
    ImGui::Unindent();
}
IMGUI_AUTO_DEFINE_END
IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, const std::list<T>)
ImGui::detail::AutoContainerValues<const std::list<T>>("List " + name, var);
IMGUI_AUTO_DEFINE_END
#endif// _LIST_

#ifdef _DEQUE_
IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, std::deque<T>)
if (ImGui::detail::AutoContainerValues<std::deque<T>>("Deque " + name, var)) {
    ImGui::PushID(name.c_str());
    ImGui::Indent();
    ImGui::detail::AutoContainerPushFrontButton(var);
    ImGui::SameLine();
    ImGui::detail::AutoContainerPushBackButton(var);
    ImGui::detail::AutoContainerPopFrontButton(var);
    if (!var.empty()) ImGui::SameLine();
    ImGui::detail::AutoContainerPopBackButton(var);
    ImGui::PopID();
    ImGui::Unindent();
}
IMGUI_AUTO_DEFINE_END
IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, const std::deque<T>)
ImGui::detail::AutoContainerValues<const std::deque<T>>("Deque " + name, var);
IMGUI_AUTO_DEFINE_END
#endif// _DEQUE_

#ifdef _FORWARD_LIST_
IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, std::forward_list<T>)
if (ImGui::detail::AutoContainerValues<std::forward_list<T>>("Forward list " + name, var)) {
    ImGui::PushID(name.c_str());
    ImGui::Indent();
    ImGui::detail::AutoContainerPushFrontButton(var);
    if (!var.empty()) ImGui::SameLine();
    ImGui::detail::AutoContainerPopFrontButton(var);
    ImGui::PopID();
    ImGui::Unindent();
}
IMGUI_AUTO_DEFINE_END
IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, const std::forward_list<T>)
ImGui::detail::AutoContainerValues<const std::forward_list<T>>("Forward list " + name, var);
IMGUI_AUTO_DEFINE_END
#endif// _FORWARD_LIST_

#ifdef _SET_
IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, std::set<T>)
ImGui::detail::AutoContainerValues<std::set<T>>("Set " + name, var);
//todo insert
IMGUI_AUTO_DEFINE_END
IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, const std::set<T>)
ImGui::detail::AutoContainerValues<const std::set<T>>("Set " + name, var);
IMGUI_AUTO_DEFINE_END
#endif// _SET_

#ifdef _UNORDERED_SET_
IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, std::unordered_set<T>)
ImGui::detail::AutoContainerValues<std::unordered_set<T>>("Unordered set " + name, var);
//todo insert
IMGUI_AUTO_DEFINE_END
IMGUI_AUTO_DEFINE_BEGIN(template<typename T>, const std::unordered_set<T>)
ImGui::detail::AutoContainerValues<const std::unordered_set<T>>("Unordered set " + name, var);
IMGUI_AUTO_DEFINE_END
#endif// _UNORDERED_SET_

#ifdef _MAP_
IMGUI_AUTO_DEFINE_BEGIN_P((template<typename K, typename V>), (std::map<K, V>) )
ImGui::detail::AutoMapContainerValues<std::map<K, V>>("Map " + name, var);
//todo insert
IMGUI_AUTO_DEFINE_END
IMGUI_AUTO_DEFINE_BEGIN_P((template<typename K, typename V>), (const std::map<K, V>) )
ImGui::detail::AutoMapContainerValues<const std::map<K, V>>("Map " + name, var);
IMGUI_AUTO_DEFINE_END
#endif// _MAP_

#ifdef _UNORDERED_MAP_
IMGUI_AUTO_DEFINE_BEGIN_P((template<typename K, typename V>), (std::unordered_map<K, V>) )
ImGui::detail::AutoMapContainerValues<std::unordered_map<K, V>>("Unordered map " + name, var);
//todo insert
IMGUI_AUTO_DEFINE_END
IMGUI_AUTO_DEFINE_BEGIN_P((template<typename K, typename V>), (const std::unordered_map<K, V>) )
ImGui::detail::AutoMapContainerValues<const std::unordered_map<K, V>>("Unordered map " + name, var);
IMGUI_AUTO_DEFINE_END
#endif// _UNORDERED_MAP_

#pragma endregion

#pragma region FUNCTIONS

IMGUI_AUTO_DEFINE_INLINE(template<>, std::add_pointer_t<void()>, if (ImGui::Button(name.c_str())) var();)
IMGUI_AUTO_DEFINE_INLINE(template<>, const std::add_pointer_t<void()>, if (ImGui::Button(name.c_str())) var();)

#pragma endregion

#pragma endregion
