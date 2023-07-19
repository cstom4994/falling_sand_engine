
#ifndef ME_IMGUIHELPER_HPP
#define ME_IMGUIHELPER_HPP

#include <array>
#include <cassert>
#include <deque>
#include <filesystem>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "engine/core/basic_types.h"
#include "engine/core/macros.hpp"
#include "engine/core/mathlib.hpp"
#include "engine/core/sdl_wrapper.h"
#include "engine/renderer/renderer_gpu.h"
#include "engine/utils/pfr.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_internal.h"
#include "libs/imgui/imgui_stdlib.h"

#if defined(_WIN32)
#define ME_IMM32
#include <Windows.h>
#include <tchar.h>
#else
#include <sys/stat.h>
#endif

#pragma region ImGuiAuto

#ifndef ME_GUI_TREE_MAX_ELEMENT_SIZE
#define ME_GUI_TREE_MAX_ELEMENT_SIZE sizeof(std::string)  // larger values generate less tree nodes
#endif
#ifndef ME_GUI_TREE_MAX_TUPLE_ELEMENTS
#define ME_GUI_TREE_MAX_TUPLE_ELEMENTS 3  // larger values generate less tree nodes
#endif

static_inline ImVec4 vec4_to_imvec4(const MEvec4 &v4) { return {v4.x, v4.y, v4.z, v4.w}; }
static_inline ImColor vec4_to_imcolor(const MEvec4 &v4) { return {v4.x * 255.0f, v4.y * 255.0f, v4.z * 255.0f, v4.w * 255.0f}; }

namespace ImGui {
//      IMGUI::AUTO()
//      =============

// This is the function that this libary implements. (It's just a wrapper around the class ImGui::Auto_t<AnyType>)
template <typename AnyType>
void Auto(AnyType &anything, const std::string &name = std::string());

//      HELPER FUNCTIONS
//      ================

// same as std::as_const in c++17
template <class T>
constexpr std::add_const_t<T> &as_const(T &t) noexcept {
    return t;
}

#pragma region DETAIL
namespace detail {

template <typename T>
bool AutoExpand(const std::string &name, T &value);
template <typename Container>
bool AutoContainerTreeNode(const std::string &name, Container &cont);
template <typename Container>
bool AutoContainerValues(const std::string &name,
                         Container &cont);  // Container must have .size(), .begin() and .end() methods and ::value_type.
template <typename Container>
bool AutoMapContainerValues(const std::string &name,
                            Container &map);  // Same as above but that iterates over pairs
template <typename Container>
void AutoContainerPushFrontButton(Container &cont);
template <typename Container>
void AutoContainerPushBackButton(Container &cont);
template <typename Container>
void AutoContainerPopFrontButton(Container &cont);
template <typename Container>
void AutoContainerPopBackButton(Container &cont);
template <typename Key, typename Value>
void AutoMapKeyValue(Key &key, Value &value);

template <class T>
constexpr std::add_const_t<T> &as_const(T &t) noexcept {
    return t;
}  // same as std::as_const in c++17
template <std::size_t I, typename... Args>
void AutoTupleRecurse(std::tuple<Args...> &tpl, std::enable_if_t<0 != I> * = 0);
template <std::size_t I, typename... Args>
inline void AutoTupleRecurse(std::tuple<Args...> &tpl, std::enable_if_t<0 == I> * = 0) {}  // End of recursion.
template <std::size_t I, typename... Args>
void AutoTupleRecurse(const std::tuple<Args...> &tpl, std::enable_if_t<0 != I> * = 0);
template <std::size_t I, typename... Args>
inline void AutoTupleRecurse(const std::tuple<Args...> &tpl, std::enable_if_t<0 == I> * = 0) {}  // End of recursion.
template <typename... Args>
void AutoTuple(const std::string &name, std::tuple<Args...> &tpl);
template <typename... Args>
void AutoTuple(const std::string &name, const std::tuple<Args...> &tpl);

template <typename T, std::size_t N>
using c_array_t = T[N];  // so arrays are regular types and can be used in macro

// template<typename Container> inline std::size_t AutoContainerSize(Container &cont, std::enable_if_t< std::is_array_v<Container>>*=0){    return sizeof(Container) /
// sizeof(decltype(*std::begin(cont)));} template<typename Container> inline std::size_t AutoContainerSize(Container &cont, std::enable_if_t<!std::is_array_v<Container>> *= 0) { return cont.size(); }

}  // namespace detail
#pragma endregion

//      PRIMARY TEMPLATE
//      ================
// This implements the struct to tuple scenario
template <typename AnyType>
struct Auto_t {
    static void Auto(AnyType &anything, const std::string &name) {
        static_assert(
                !std::is_reference_v<AnyType> && std::is_copy_constructible_v<std::remove_all_extents_t<AnyType>> && !std::is_polymorphic_v<AnyType> &&
                        ME::cpp::pfr::detail::is_aggregate_initializable_n<AnyType, ME::cpp::pfr::detail::detect_fields_count_dispatch<AnyType>(ME::cpp::pfr::detail::size_t_<sizeof(AnyType) * 8>{},
                                                                                                                                                1L)>::value,  // If the above is not a constexpr
                expression,
                // you are yousing an invalid type
                "This type cannot be converted to a tuple.");
        auto tuple = ME::cpp::pfr::structure_tie(anything);
        ImGui::detail::AutoTuple("Struct " + name, tuple);
    }
};  // ImGui::Auto_t<>::Auto()
}  // namespace ImGui

// Implementation of ImGui::Auto()
template <typename AnyType>
inline void ImGui::Auto(AnyType &anything, const std::string &name) {
    ImGui::Auto_t<AnyType>::Auto(anything, name);
}

//      HELPER FUNCTIONS
//      ================

#pragma region UTILS

template <typename T>
bool ImGui::detail::AutoExpand(const std::string &name, T &value) {
    if (sizeof(T) <= ME_GUI_TREE_MAX_ELEMENT_SIZE) {
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

template <typename Container>
bool ImGui::detail::AutoContainerTreeNode(const std::string &name, Container &cont) {
    // std::size_t size = ImGui::detail::AutoContainerSize(cont);
    std::size_t size = cont.size();
    if (ImGui::CollapsingHeader(name.c_str())) {
        size_t elemsize = sizeof(decltype(*std::begin(cont)));
        ImGui::Text("大小: %d, 非动态元素大小: %d bytes", (int)size, (int)elemsize);
        return true;
    } else {
        float label_width = CalcTextSize(name.c_str()).x + ImGui::GetTreeNodeToLabelSpacing() + 5;
        std::string sizetext = "(大小 = " + std::to_string(size) + ')';
        float sizet_width = CalcTextSize(sizetext.c_str()).x;
        float avail_width = ImGui::GetContentRegionAvail().x;
        if (avail_width > label_width + sizet_width) {
            ImGui::SameLine(avail_width - sizet_width);
            ImGui::TextUnformatted(sizetext.c_str());
        }
        return false;
    }
}
template <typename Container>
bool ImGui::detail::AutoContainerValues(const std::string &name, Container &cont) {
    if (ImGui::detail::AutoContainerTreeNode(name, cont)) {
        ImGui::Indent();
        ImGui::PushID(name.c_str());
        std::size_t i = 0;
        for (auto &elem : cont) {
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
template <typename Container>
bool ImGui::detail::AutoMapContainerValues(const std::string &name, Container &cont) {
    if (ImGui::detail::AutoContainerTreeNode(name, cont)) {
        ImGui::Indent();
        std::size_t i = 0;
        for (auto &elem : cont) {
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
template <typename Container>
void ImGui::detail::AutoContainerPushFrontButton(Container &cont) {
    if (ImGui::SmallButton("Push Front")) cont.emplace_front();
}
template <typename Container>
void ImGui::detail::AutoContainerPushBackButton(Container &cont) {
    if (ImGui::SmallButton("Push Back ")) cont.emplace_back();
}
template <typename Container>
void ImGui::detail::AutoContainerPopFrontButton(Container &cont) {
    if (!cont.empty() && ImGui::SmallButton("Pop Front ")) cont.pop_front();
}
template <typename Container>
void ImGui::detail::AutoContainerPopBackButton(Container &cont) {
    if (!cont.empty() && ImGui::SmallButton("Pop Back  ")) cont.pop_back();
}
template <typename Key, typename Value>
void ImGui::detail::AutoMapKeyValue(Key &key, Value &value) {
    bool b_k = sizeof(Key) <= ME_GUI_TREE_MAX_ELEMENT_SIZE;
    bool b_v = sizeof(Value) <= ME_GUI_TREE_MAX_ELEMENT_SIZE;
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

template <std::size_t I, typename... Args>
void ImGui::detail::AutoTupleRecurse(std::tuple<Args...> &tpl, std::enable_if_t<0 != I> *) {
    ImGui::detail::AutoTupleRecurse<I - 1, Args...>(tpl);  // first draw smaller indeces
    using type = decltype(std::get<I - 1>(tpl));
    std::string str = '<' + std::to_string(I) + ">: " + (std::is_const_v<type> ? "const " : "") + typeid(type).name();
    ImGui::detail::AutoExpand(str, std::get<I - 1>(tpl));
}
template <std::size_t I, typename... Args>
void ImGui::detail::AutoTupleRecurse(const std::tuple<Args...> &tpl, std::enable_if_t<0 != I> *) {
    ImGui::detail::AutoTupleRecurse<I - 1, const Args...>(tpl);  // first draw smaller indeces
    using type = decltype(std::get<I - 1>(tpl));
    std::string str = '<' + std::to_string(I) + ">: " + "const " + typeid(type).name();
    ImGui::detail::AutoExpand(str, ImGui::as_const(std::get<I - 1>(tpl)));
}
template <typename... Args>
void ImGui::detail::AutoTuple(const std::string &name, std::tuple<Args...> &tpl) {
    constexpr std::size_t tuple_size = sizeof(decltype(tpl));
    constexpr std::size_t tuple_numelems = sizeof...(Args);
    if (tuple_size <= ME_GUI_TREE_MAX_ELEMENT_SIZE && tuple_numelems <= ME_GUI_TREE_MAX_TUPLE_ELEMENTS) {
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
template <typename... Args>
void ImGui::detail::AutoTuple(const std::string &name,
                              const std::tuple<Args...> &tpl)  // same but const
{
    constexpr std::size_t tuple_size = sizeof(std::tuple<Args...>);
    constexpr std::size_t tuple_numelems = sizeof...(Args);
    if (tuple_size <= ME_GUI_TREE_MAX_ELEMENT_SIZE && tuple_numelems <= ME_GUI_TREE_MAX_TUPLE_ELEMENTS) {
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

#pragma endregion

//      HELPER MACROS
//      =============

#define UNPACK(...) __VA_ARGS__  // for unpacking parentheses. It is needed for macro arguments with commmas
// Enclose templatespec, AND typespec in parentheses in this version. Useful if there are commas in the argument.
#define ME_GUI_DEFINE_BEGIN_P(templatespec, typespec)    \
    namespace ImGui {                                    \
    UNPACK templatespec struct Auto_t<UNPACK typespec> { \
        static void Auto(UNPACK typespec &var, const std::string &name) {
// If macro arguments have no commmas inside use this version without parentheses
#define ME_GUI_DEFINE_BEGIN(templatespec, typespec) ME_GUI_DEFINE_BEGIN_P((templatespec), (typespec))  // when there are no commas in types, use this without parentheses
#define ME_GUI_DEFINE_END \
    }                     \
    }                     \
    ;                     \
    }
#define ME_GUI_DEFINE_INLINE_P(template_spec, type_spec, code) ME_GUI_DEFINE_BEGIN_P(template_spec, type_spec) code ME_GUI_DEFINE_END
#define ME_GUI_DEFINE_INLINE(template_spec, type_spec, code) ME_GUI_DEFINE_INLINE_P((template_spec), (type_spec), code)

#ifndef ME_GUI_INPUT_FLOAT1
#define ME_GUI_INPUT_FLOAT1 ImGui::DragFloat
#endif
#ifndef ME_GUI_INPUT_FLOAT2
#define ME_GUI_INPUT_FLOAT2 ImGui::DragFloat2
#endif
#ifndef ME_GUI_INPUT_FLOAT3
#define ME_GUI_INPUT_FLOAT3 ImGui::DragFloat3
#endif
#ifndef ME_GUI_INPUT_FLOAT4
#define ME_GUI_INPUT_FLOAT4 ImGui::DragFloat4
#endif
#ifndef ME_GUI_INPUT_INT1
#define ME_GUI_INPUT_INT1 ImGui::InputInt
#endif
#ifndef ME_GUI_INPUT_INT2
#define ME_GUI_INPUT_INT2 ImGui::InputInt2
#endif
#ifndef ME_GUI_INPUT_INT3
#define ME_GUI_INPUT_INT3 ImGui::InputInt3
#endif
#ifndef ME_GUI_INPUT_INT4
#define ME_GUI_INPUT_INT4 ImGui::InputInt4
#endif
#ifndef ME_GUI_NULLPTR_COLOR
#define ME_GUI_NULLPTR_COLOR ImVec4(1.0, 0.5, 0.5, 1.0)
#endif

//      SPECIALIZATIONS
//      ===============

#pragma region STRINGS

ME_GUI_DEFINE_BEGIN(template <>, const char *)
if (name.empty())
    ImGui::TextUnformatted(var);
else
    ImGui::Text("%s=%s", name.c_str(), var);
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN_P((template <std::size_t N>), (const detail::c_array_t<char, N>))
if (name.empty())
    ImGui::TextUnformatted(var, var + N - 1);
else
    ImGui::Text("%s=%s", name.c_str(), var);
ME_GUI_DEFINE_END

ME_GUI_DEFINE_INLINE(template <>, char *, const char *tmp = var; ImGui::Auto_t<const char *>::Auto(tmp, name);)
ME_GUI_DEFINE_INLINE(template <>, char *const, const char *tmp = var; ImGui::Auto_t<const char *>::Auto(tmp, name);)
ME_GUI_DEFINE_INLINE(template <>, const char *const, const char *tmp = var; ImGui::Auto_t<const char *>::Auto(tmp, name);)
ME_GUI_DEFINE_BEGIN(template <>, std::string)
const std::size_t lines = var.find('\n');
if (var.find('\n') != std::string::npos)
    ImGui::InputTextMultiline(name.c_str(), const_cast<char *>(var.c_str()), 256);
else
    ImGui::InputText(name.c_str(), const_cast<char *>(var.c_str()), 256);
ME_GUI_DEFINE_END
ME_GUI_DEFINE_BEGIN(template <>, const std::string)
if (name.empty())
    ImGui::TextUnformatted(var.c_str(), var.c_str() + var.length());
else
    ImGui::Text("%s=%s", name.c_str(), var.c_str());
ME_GUI_DEFINE_END

#pragma endregion

#pragma region NUMBERS

ME_GUI_DEFINE_INLINE(template <>, float, ME_GUI_INPUT_FLOAT1(name.c_str(), &var);)
ME_GUI_DEFINE_INLINE(template <>, int, ME_GUI_INPUT_INT1(name.c_str(), &var);)
ME_GUI_DEFINE_INLINE(template <>, unsigned int, ME_GUI_INPUT_INT1(name.c_str(), (int *)&var);)
ME_GUI_DEFINE_INLINE(template <>, bool, ImGui::Checkbox(name.c_str(), &var);)
ME_GUI_DEFINE_INLINE(template <>, ImVec2, ME_GUI_INPUT_FLOAT2(name.c_str(), &var.x);)
ME_GUI_DEFINE_INLINE(template <>, ImVec4, ME_GUI_INPUT_FLOAT4(name.c_str(), &var.x);)
ME_GUI_DEFINE_INLINE(template <>, const float, ImGui::Auto_t<const std::string>::Auto(std::to_string(var), name);)
ME_GUI_DEFINE_INLINE(template <>, const int, ImGui::Auto_t<const std::string>::Auto(std::to_string(var), name);)
ME_GUI_DEFINE_INLINE(template <>, const unsigned, ImGui::Auto_t<const std::string>::Auto(std::to_string(var), name);)
ME_GUI_DEFINE_INLINE(template <>, const bool, ImGui::Auto_t<const std::string>::Auto(std::to_string(var), name);)
ME_GUI_DEFINE_INLINE(template <>, const ImVec2, ImGui::Text("%s(%f,%f)", (name.empty() ? "" : name + "=").c_str(), var.x, var.y);)
ME_GUI_DEFINE_INLINE(template <>, const ImVec4, ImGui::Text("%s(%f,%f,%f,%f)", (name.empty() ? "" : name + "=").c_str(), var.x, var.y, var.z, var.w);)

#define INTERNAL_NUM(_c, _imn)                                                                           \
    ME_GUI_DEFINE_INLINE(template <>, _c, ImGui::InputScalar(name.c_str(), ImGuiDataType_##_imn, &var);) \
    ME_GUI_DEFINE_INLINE(template <>, const _c, ImGui::Auto_t<const std::string>::Auto(std::to_string(var), name);)

INTERNAL_NUM(u8, U8)
INTERNAL_NUM(u16, U16)
INTERNAL_NUM(u64, U64)
INTERNAL_NUM(i8, S8)
INTERNAL_NUM(i16, S16)
INTERNAL_NUM(i64, S64)

ME_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<float, 1>), ME_GUI_INPUT_FLOAT1(name.c_str(), &var[0]);)
ME_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<float, 1>), ImGui::Text("%s%f", (name.empty() ? "" : name + "=").c_str(), var[0]);)
ME_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<float, 2>), ME_GUI_INPUT_FLOAT2(name.c_str(), &var[0]);)
ME_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<float, 2>), ImGui::Text("%s(%f,%f)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1]);)
ME_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<float, 3>), ME_GUI_INPUT_FLOAT3(name.c_str(), &var[0]);)
ME_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<float, 3>), ImGui::Text("%s(%f,%f,%f)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1], var[2]);)
ME_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<float, 4>), ME_GUI_INPUT_FLOAT4(name.c_str(), &var[0]);)
ME_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<float, 4>), ImGui::Text("%s(%f,%f,%f,%f)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1], var[2], var[3]);)

ME_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<int, 1>), ME_GUI_INPUT_INT1(name.c_str(), &var[0]);)
ME_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<int, 1>), ImGui::Text("%s%d", (name.empty() ? "" : name + "=").c_str(), var[0]);)
ME_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<int, 2>), ME_GUI_INPUT_INT2(name.c_str(), &var[0]);)
ME_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<int, 2>), ImGui::Text("%s(%d,%d)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1]);)
ME_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<int, 3>), ME_GUI_INPUT_INT3(name.c_str(), &var[0]);)
ME_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<int, 3>), ImGui::Text("%s(%d,%d,%d)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1], var[2]);)
ME_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<int, 4>), ME_GUI_INPUT_INT4(name.c_str(), &var[0]);)
ME_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<int, 4>), ImGui::Text("%s(%d,%d,%d,%d)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1], var[2], var[3]);)

#pragma endregion

#pragma region POINTERS and ARRAYS
ME_GUI_DEFINE_BEGIN(template <typename T>, T *)
if (var != nullptr)
    ImGui::detail::AutoExpand<T>("Pointer " + name, *var);
else
    ImGui::TextColored(ME_GUI_NULLPTR_COLOR, "%s=NULL", name.c_str());
ME_GUI_DEFINE_END
ME_GUI_DEFINE_BEGIN(template <typename T>, T *const)
if (var != nullptr)
    ImGui::detail::AutoExpand<T>("Pointer " + name, *var);
else
    ImGui::TextColored(ME_GUI_NULLPTR_COLOR, "%s=NULL", name.c_str());
ME_GUI_DEFINE_END

ME_GUI_DEFINE_INLINE_P((template <typename T, std::size_t N>), (std::array<T, N>), ImGui::detail::AutoContainerValues("array " + name, var);)
ME_GUI_DEFINE_INLINE_P((template <typename T, std::size_t N>), (const std::array<T, N>), ImGui::detail::AutoContainerValues("array " + name, var);)
ME_GUI_DEFINE_INLINE_P((template <typename T, std::size_t N>), (detail::c_array_t<T, N>), ImGui::detail::AutoContainerValues("Array " + name, *(std::array<T, N> *)(&var));)
ME_GUI_DEFINE_INLINE_P((template <typename T, std::size_t N>), (const detail::c_array_t<T, N>), ImGui::detail::AutoContainerValues("Array " + name, *(const std::array<T, N> *)(&var));)

#pragma endregion

#pragma region PAIRS and TUPLES

ME_GUI_DEFINE_BEGIN_P((template <typename T1, typename T2>), (std::pair<T1, T2>))
if ((std::is_fundamental_v<T1> || std::is_same_v<std::string, T1>)&&(std::is_fundamental_v<T2> || std::is_same_v<std::string, T2>)) {
    float width = ImGui::CalcItemWidth();
    ImGui::PushItemWidth(width * 0.4 - 10);  // a bit less than half
    ImGui::detail::AutoExpand<T1>(name + ".first", var.first);
    ImGui::SameLine();
    ImGui::detail::AutoExpand<T2>(name + ".second", var.second);
    ImGui::PopItemWidth();
} else {
    ImGui::detail::AutoExpand<T1>(name + ".first", var.first);
    ImGui::detail::AutoExpand<T2>(name + ".second", var.second);
}

ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN_P((template <typename T1, typename T2>), (const std::pair<T1, T2>))
ImGui::detail::AutoExpand<const T1>(name + ".first", var.first);
if (std::is_fundamental_v<T1> && std::is_fundamental_v<T2>) ImGui::SameLine();
ImGui::detail::AutoExpand<const T2>(name + ".second", var.second);
ME_GUI_DEFINE_END

#ifdef _TUPLE_
ME_GUI_DEFINE_INLINE(template <typename... Args>, std::tuple<Args...>, ImGui::detail::AutoTuple("Tuple " + name, var);)
ME_GUI_DEFINE_INLINE(template <typename... Args>, const std::tuple<Args...>, ImGui::detail::AutoTuple("Tuple " + name, var);)
#endif  //_TUPLE_

#pragma endregion

#pragma region CONTAINERS
ME_GUI_DEFINE_BEGIN(template <typename T>, std::vector<T>)
if (ImGui::detail::AutoContainerValues<std::vector<T>>("Vector " + name, var)) {
    ImGui::PushID(name.c_str());
    ImGui::Indent();
    ImGui::detail::AutoContainerPushBackButton(var);
    if (!var.empty()) ImGui::SameLine();
    ImGui::detail::AutoContainerPopBackButton(var);
    ImGui::PopID();
    ImGui::Unindent();
}
ME_GUI_DEFINE_END
ME_GUI_DEFINE_BEGIN(template <>, std::vector<bool>)
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
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN(template <typename T>, const std::vector<T>)
ImGui::detail::AutoContainerValues<const std::vector<T>>("Vector " + name, var);
ME_GUI_DEFINE_END
ME_GUI_DEFINE_BEGIN(template <>, const std::vector<bool>)
if (ImGui::detail::AutoContainerTreeNode<const std::vector<bool>>("Vector " + name, var)) {
    ImGui::Indent();
    for (int i = 0; i < var.size(); ++i) {
        ImGui::Bullet();
        ImGui::Auto_t<const bool>::Auto(var[i], '[' + std::to_string(i) + ']');
    }
    ImGui::Unindent();
}
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN(template <typename T>, std::list<T>)
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
ME_GUI_DEFINE_END
ME_GUI_DEFINE_BEGIN(template <typename T>, const std::list<T>)
ImGui::detail::AutoContainerValues<const std::list<T>>("List " + name, var);
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN(template <typename T>, std::deque<T>)
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
ME_GUI_DEFINE_END
ME_GUI_DEFINE_BEGIN(template <typename T>, const std::deque<T>)
ImGui::detail::AutoContainerValues<const std::deque<T>>("Deque " + name, var);
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN(template <typename T>, std::forward_list<T>)
if (ImGui::detail::AutoContainerValues<std::forward_list<T>>("Forward list " + name, var)) {
    ImGui::PushID(name.c_str());
    ImGui::Indent();
    ImGui::detail::AutoContainerPushFrontButton(var);
    if (!var.empty()) ImGui::SameLine();
    ImGui::detail::AutoContainerPopFrontButton(var);
    ImGui::PopID();
    ImGui::Unindent();
}
ME_GUI_DEFINE_END
ME_GUI_DEFINE_BEGIN(template <typename T>, const std::forward_list<T>)
ImGui::detail::AutoContainerValues<const std::forward_list<T>>("Forward list " + name, var);
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN(template <typename T>, std::set<T>)
ImGui::detail::AutoContainerValues<std::set<T>>("Set " + name, var);
// todo insert
ME_GUI_DEFINE_END
ME_GUI_DEFINE_BEGIN(template <typename T>, const std::set<T>)
ImGui::detail::AutoContainerValues<const std::set<T>>("Set " + name, var);
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN(template <typename T>, std::unordered_set<T>)
ImGui::detail::AutoContainerValues<std::unordered_set<T>>("Unordered set " + name, var);
// todo insert
ME_GUI_DEFINE_END
ME_GUI_DEFINE_BEGIN(template <typename T>, const std::unordered_set<T>)
ImGui::detail::AutoContainerValues<const std::unordered_set<T>>("Unordered set " + name, var);
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN_P((template <typename K, typename V>), (std::map<K, V>))
ImGui::detail::AutoMapContainerValues<std::map<K, V>>("Map " + name, var);
// todo insert
ME_GUI_DEFINE_END
ME_GUI_DEFINE_BEGIN_P((template <typename K, typename V>), (const std::map<K, V>))
ImGui::detail::AutoMapContainerValues<const std::map<K, V>>("Map " + name, var);
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN_P((template <typename K, typename V>), (std::unordered_map<K, V>))
ImGui::detail::AutoMapContainerValues<std::unordered_map<K, V>>("Unordered map " + name, var);
// todo insert
ME_GUI_DEFINE_END
ME_GUI_DEFINE_BEGIN_P((template <typename K, typename V>), (const std::unordered_map<K, V>))
ImGui::detail::AutoMapContainerValues<const std::unordered_map<K, V>>("Unordered map " + name, var);
ME_GUI_DEFINE_END

#pragma endregion

#pragma region FUNCTIONS

ME_GUI_DEFINE_INLINE(template <>, std::add_pointer_t<void()>, if (ImGui::Button(name.c_str())) var();)
ME_GUI_DEFINE_INLINE(template <>, const std::add_pointer_t<void()>, if (ImGui::Button(name.c_str())) var();)

#pragma endregion

// ME_GUI_DEFINE_BEGIN(template <typename T>, std::vector<T>)
// if (ImGui::detail::AutoContainerValues<std::vector<T>>("MEVector " + name, var)) {
//     ImGui::PushID(name.c_str());
//     ImGui::Indent();
//     ImGui::detail::AutoContainerPushBackButton(var);
//     if (!var.empty()) ImGui::SameLine();
//     ImGui::detail::AutoContainerPopBackButton(var);
//     ImGui::PopID();
//     ImGui::Unindent();
// }
// ME_GUI_DEFINE_END

// ME_GUI_DEFINE_BEGIN(template <typename T>, const std::vector<T>)
// ImGui::detail::AutoContainerValues<const std::vector<T>>("MEVector " + name, var);
// ME_GUI_DEFINE_END

#pragma endregion ImGuiAuto

#pragma region ImString

class MEimstr {

public:
    MEimstr();
    MEimstr(size_t len);
    MEimstr(char *string);
    explicit MEimstr(const char *string);
    MEimstr(const MEimstr &other);
    ~MEimstr();

    char &operator[](size_t pos);
    operator char *();
    bool operator==(const char *string);
    bool operator!=(const char *string);
    bool operator==(MEimstr &string);
    bool operator!=(const MEimstr &string);
    MEimstr &operator=(const char *string);
    MEimstr &operator=(const MEimstr &other);

    inline size_t size() const { return m_data ? strlen(m_data) + 1 : 0; }
    void reserve(size_t len);
    char *get();
    const char *c_str() const;
    bool empty() const;
    int refcount() const;
    void ref();
    void unref();

private:
    char *m_data;
    int *m_ref_count;
};

#pragma endregion ImString

ME_INLINE ImVec4 ME_rgba2imvec(int r, int g, int b, int a = 255) {
    float newr = r / 255.f;
    float newg = g / 255.f;
    float newb = b / 255.f;
    float newa = a / 255.f;
    return ImVec4(newr, newg, newb, newa);
}

ME_INLINE MEcolor ME_imvec2rgba(ImVec4 iv) {
    u8 newr = iv.x * 255;
    u8 newg = iv.y * 255;
    u8 newb = iv.z * 255;
    u8 newa = iv.w * 255;
    return MEcolor(newr, newg, newb, newa);
}

namespace ImGui {
//-----------------------------------------------------------------------------
// Basic types
//-----------------------------------------------------------------------------

struct Link;
struct MarkdownConfig;

struct MarkdownLinkCallbackData  // for both links and images
{
    const char *text;  // text between square brackets []
    int textLength;
    const char *link;  // text between brackets ()
    int linkLength;
    void *userData;
    bool isImage;  // true if '!' is detected in front of the link syntax
};

struct MarkdownTooltipCallbackData  // for tooltips
{
    MarkdownLinkCallbackData linkData;
    const char *linkIcon;
};

struct MarkdownImageData {
    bool isValid = false;                    // if true, will draw the image
    bool useLinkCallback = false;            // if true, linkCallback will be called when image is clicked
    ImTextureID user_texture_id = 0;         // see ImGui::Image
    ImVec2 size = ImVec2(100.0f, 100.0f);    // see ImGui::Image
    ImVec2 uv0 = ImVec2(0, 0);               // see ImGui::Image
    ImVec2 uv1 = ImVec2(1, 1);               // see ImGui::Image
    ImVec4 tint_col = ImVec4(1, 1, 1, 1);    // see ImGui::Image
    ImVec4 border_col = ImVec4(0, 0, 0, 0);  // see ImGui::Image
};

enum class MarkdownFormatType {
    NORMAL_TEXT,
    HEADING,
    UNORDERED_LIST,
    LINK,
    EMPHASIS,
};

struct MarkdownFormatInfo {
    MarkdownFormatType type = MarkdownFormatType::NORMAL_TEXT;
    int32_t level = 0;         // Set for headings: 1 for H1, 2 for H2 etc.
    bool itemHovered = false;  // Currently only set for links when mouse hovered, only valid when start_ == false
    const MarkdownConfig *config = NULL;
};

typedef void MarkdownLinkCallback(MarkdownLinkCallbackData data);
typedef void MarkdownTooltipCallback(MarkdownTooltipCallbackData data);

inline void defaultMarkdownTooltipCallback(MarkdownTooltipCallbackData data_) {
    if (data_.linkData.isImage) {
        ImGui::SetTooltip("%.*s", data_.linkData.linkLength, data_.linkData.link);
    } else {
        ImGui::SetTooltip("%s Open in browser\n%.*s", data_.linkIcon, data_.linkData.linkLength, data_.linkData.link);
    }
}

typedef MarkdownImageData MarkdownImageCallback(MarkdownLinkCallbackData data);
typedef void MarkdownFormalCallback(const MarkdownFormatInfo &markdownFormatInfo_, bool start_);

inline void defaultMarkdownFormatCallback(const MarkdownFormatInfo &markdownFormatInfo_, bool start_);

struct MarkdownHeadingFormat {
    ImFont *font;    // ImGui font
    bool separator;  // if true, an underlined separator is drawn after the header
};

// Configuration struct for Markdown
// - linkCallback is called when a link is clicked on
// - linkIcon is a string which encode a "Link" icon, if available in the current font (e.g. linkIcon = ICON_FA_LINK with FontAwesome + IconFontCppHeaders
// https://github.com/juliettef/IconFontCppHeaders)
// - headingFormats controls the format of heading H1 to H3, those above H3 use H3 format
struct MarkdownConfig {
    static const int NUMHEADINGS = 3;

    MarkdownLinkCallback *linkCallback = NULL;
    MarkdownTooltipCallback *tooltipCallback = NULL;
    MarkdownImageCallback *imageCallback = NULL;
    const char *linkIcon = "";  // icon displayd in link tooltip
    MarkdownHeadingFormat headingFormats[NUMHEADINGS] = {{NULL, true}, {NULL, true}, {NULL, true}};
    void *userData = NULL;
    MarkdownFormalCallback *formatCallback = defaultMarkdownFormatCallback;
};

//-----------------------------------------------------------------------------
// External interface
//-----------------------------------------------------------------------------

inline void Markdown(const char *markdown_, size_t markdownLength_, const MarkdownConfig &mdConfig_);

//-----------------------------------------------------------------------------
// Internals
//-----------------------------------------------------------------------------

struct TextRegion;
struct Line;
inline void UnderLine(ImColor col_);
inline void RenderLine(const char *markdown_, Line &line_, TextRegion &textRegion_, const MarkdownConfig &mdConfig_);

struct TextRegion {
    TextRegion() : indentX(0.0f) {}
    ~TextRegion() { ResetIndent(); }

    // ImGui::TextWrapped will wrap at the starting position
    // so to work around this we render using our own wrapping for the first line
    void RenderTextWrapped(const char *text_, const char *text_end_, bool bIndentToHere_ = false) {
        float scale = ImGui::GetIO().FontGlobalScale;
        float widthLeft = GetContentRegionAvail().x;
        const char *endLine = ImGui::GetFont()->CalcWordWrapPositionA(scale, text_, text_end_, widthLeft);
        ImGui::TextUnformatted(text_, endLine);
        if (bIndentToHere_) {
            float indentNeeded = GetContentRegionAvail().x - widthLeft;
            if (indentNeeded) {
                ImGui::Indent(indentNeeded);
                indentX += indentNeeded;
            }
        }
        widthLeft = GetContentRegionAvail().x;
        while (endLine < text_end_) {
            text_ = endLine;
            if (*text_ == ' ') {
                ++text_;
            }  // skip a space at start of line
            endLine = ImGui::GetFont()->CalcWordWrapPositionA(scale, text_, text_end_, widthLeft);
            if (text_ == endLine) {
                endLine++;
            }
            ImGui::TextUnformatted(text_, endLine);
        }
    }

    void RenderListTextWrapped(const char *text_, const char *text_end_) {
        ImGui::Bullet();
        ImGui::SameLine();
        RenderTextWrapped(text_, text_end_, true);
    }

    bool RenderLinkText(const char *text_, const char *text_end_, const Link &link_, const char *markdown_, const MarkdownConfig &mdConfig_, const char **linkHoverStart_);

    void RenderLinkTextWrapped(const char *text_, const char *text_end_, const Link &link_, const char *markdown_, const MarkdownConfig &mdConfig_, const char **linkHoverStart_,
                               bool bIndentToHere_ = false);

    void ResetIndent() {
        if (indentX > 0.0f) {
            ImGui::Unindent(indentX);
        }
        indentX = 0.0f;
    }

private:
    float indentX;
};

// Text that starts after a new line (or at beginning) and ends with a newline (or at end)
struct Line {
    bool isHeading = false;
    bool isEmphasis = false;
    bool isUnorderedListStart = false;
    bool isLeadingSpace = true;  // spaces at start of line
    int leadSpaceCount = 0;
    int headingCount = 0;
    int emphasisCount = 0;
    int lineStart = 0;
    int lineEnd = 0;
    int lastRenderPosition = 0;  // lines may get rendered in multiple pieces
};

struct TextBlock {  // subset of line
    int start = 0;
    int stop = 0;
    int size() const { return stop - start; }
};

struct Link {
    enum LinkState {
        NO_LINK,
        HAS_SQUARE_BRACKET_OPEN,
        HAS_SQUARE_BRACKETS,
        HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN,
    };
    LinkState state = NO_LINK;
    TextBlock text;
    TextBlock url;
    bool isImage = false;
    int num_brackets_open = 0;
};

struct Emphasis {
    enum EmphasisState {
        NONE,
        LEFT,
        MIDDLE,
        RIGHT,
    };
    EmphasisState state = NONE;
    TextBlock text;
    char sym;
};

inline void UnderLine(ImColor col_) {
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    min.y = max.y;
    ImGui::GetWindowDrawList()->AddLine(min, max, col_, 1.0f);
}

inline void RenderLine(const char *markdown_, Line &line_, TextRegion &textRegion_, const MarkdownConfig &mdConfig_) {
    // indent
    int indentStart = 0;
    if (line_.isUnorderedListStart)  // ImGui unordered list render always adds one indent
    {
        indentStart = 1;
    }
    for (int j = indentStart; j < line_.leadSpaceCount / 2; ++j)  // add indents
    {
        ImGui::Indent();
    }

    // render
    MarkdownFormatInfo formatInfo;
    formatInfo.config = &mdConfig_;
    int textStart = line_.lastRenderPosition + 1;
    int textSize = line_.lineEnd - textStart;
    if (line_.isUnorderedListStart)  // render unordered list
    {
        formatInfo.type = MarkdownFormatType::UNORDERED_LIST;
        mdConfig_.formatCallback(formatInfo, true);
        const char *text = markdown_ + textStart + 1;
        textRegion_.RenderListTextWrapped(text, text + textSize - 1);
    } else if (line_.isHeading)  // render heading
    {
        formatInfo.level = line_.headingCount;
        formatInfo.type = MarkdownFormatType::HEADING;
        mdConfig_.formatCallback(formatInfo, true);
        const char *text = markdown_ + textStart + 1;
        textRegion_.RenderTextWrapped(text, text + textSize - 1);
    } else if (line_.isEmphasis)  // render emphasis
    {
        formatInfo.level = line_.emphasisCount;
        formatInfo.type = MarkdownFormatType::EMPHASIS;
        mdConfig_.formatCallback(formatInfo, true);
        const char *text = markdown_ + textStart;
        textRegion_.RenderTextWrapped(text, text + textSize);
    } else  // render a normal paragraph chunk
    {
        formatInfo.type = MarkdownFormatType::NORMAL_TEXT;
        mdConfig_.formatCallback(formatInfo, true);
        const char *text = markdown_ + textStart;
        textRegion_.RenderTextWrapped(text, text + textSize);
    }
    mdConfig_.formatCallback(formatInfo, false);

    // unindent
    for (int j = indentStart; j < line_.leadSpaceCount / 2; ++j) {
        ImGui::Unindent();
    }
}

// render markdown
inline void Markdown(const char *markdown_, size_t markdownLength_, const MarkdownConfig &mdConfig_) {
    static const char *linkHoverStart = NULL;  // we need to preserve status of link hovering between frames
    ImGuiStyle &style = ImGui::GetStyle();
    Line line;
    Link link;
    Emphasis em;
    TextRegion textRegion;

    char c = 0;
    for (int i = 0; i < (int)markdownLength_; ++i) {
        c = markdown_[i];  // get the character at index
        if (c == 0) {
            break;
        }  // shouldn't happen but don't go beyond 0.

        // If we're at the beginning of the line, count any spaces
        if (line.isLeadingSpace) {
            if (c == ' ') {
                ++line.leadSpaceCount;
                continue;
            } else {
                line.isLeadingSpace = false;
                line.lastRenderPosition = i - 1;
                if ((c == '*') && (line.leadSpaceCount >= 2)) {
                    if (((int)markdownLength_ > i + 1) && (markdown_[i + 1] == ' '))  // space after '*'
                    {
                        line.isUnorderedListStart = true;
                        ++i;
                        ++line.lastRenderPosition;
                    }
                    // carry on processing as could be emphasis
                } else if (c == '#') {
                    line.headingCount++;
                    bool bContinueChecking = true;
                    int j = i;
                    while (++j < (int)markdownLength_ && bContinueChecking) {
                        c = markdown_[j];
                        switch (c) {
                            case '#':
                                line.headingCount++;
                                break;
                            case ' ':
                                line.lastRenderPosition = j - 1;
                                i = j;
                                line.isHeading = true;
                                bContinueChecking = false;
                                break;
                            default:
                                line.isHeading = false;
                                bContinueChecking = false;
                                break;
                        }
                    }
                    if (line.isHeading) {
                        // reset emphasis status, we do not support emphasis around headers for now
                        em = Emphasis();
                        continue;
                    }
                }
            }
        }

        // Test to see if we have a link
        switch (link.state) {
            case Link::NO_LINK:
                if (c == '[' && !line.isHeading)  // we do not support headings with links for now
                {
                    link.state = Link::HAS_SQUARE_BRACKET_OPEN;
                    link.text.start = i + 1;
                    if (i > 0 && markdown_[i - 1] == '!') {
                        link.isImage = true;
                    }
                }
                break;
            case Link::HAS_SQUARE_BRACKET_OPEN:
                if (c == ']') {
                    link.state = Link::HAS_SQUARE_BRACKETS;
                    link.text.stop = i;
                }
                break;
            case Link::HAS_SQUARE_BRACKETS:
                if (c == '(') {
                    link.state = Link::HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN;
                    link.url.start = i + 1;
                    link.num_brackets_open = 1;
                }
                break;
            case Link::HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN:
                if (c == '(') {
                    ++link.num_brackets_open;
                } else if (c == ')') {
                    --link.num_brackets_open;
                }
                if (link.num_brackets_open == 0) {
                    // reset emphasis status, we do not support emphasis around links for now
                    em = Emphasis();
                    // render previous line content
                    line.lineEnd = link.text.start - (link.isImage ? 2 : 1);
                    RenderLine(markdown_, line, textRegion, mdConfig_);
                    line.leadSpaceCount = 0;
                    link.url.stop = i;
                    line.isUnorderedListStart = false;  // the following text shouldn't have bullets
                    ImGui::SameLine(0.0f, 0.0f);
                    if (link.isImage)  // it's an image, render it.
                    {
                        bool drawnImage = false;
                        bool useLinkCallback = false;
                        if (mdConfig_.imageCallback) {
                            MarkdownImageData imageData =
                                    mdConfig_.imageCallback({markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start, link.url.size(), mdConfig_.userData, true});
                            useLinkCallback = imageData.useLinkCallback;
                            if (imageData.isValid) {
                                ImGui::Image(imageData.user_texture_id, imageData.size, imageData.uv0, imageData.uv1, imageData.tint_col, imageData.border_col);
                                drawnImage = true;
                            }
                        }
                        if (!drawnImage) {
                            ImGui::Text("( Image %.*s not loaded )", link.url.size(), markdown_ + link.url.start);
                        }
                        if (ImGui::IsItemHovered()) {
                            if (ImGui::IsMouseReleased(0) && mdConfig_.linkCallback && useLinkCallback) {
                                mdConfig_.linkCallback({markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start, link.url.size(), mdConfig_.userData, true});
                            }
                            if (link.text.size() > 0 && mdConfig_.tooltipCallback) {
                                mdConfig_.tooltipCallback({{markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start, link.url.size(), mdConfig_.userData, true}, mdConfig_.linkIcon});
                            }
                        }
                    } else  // it's a link, render it.
                    {
                        textRegion.RenderLinkTextWrapped(markdown_ + link.text.start, markdown_ + link.text.start + link.text.size(), link, markdown_, mdConfig_, &linkHoverStart, false);
                    }
                    ImGui::SameLine(0.0f, 0.0f);
                    // reset the link by reinitializing it
                    link = Link();
                    line.lastRenderPosition = i;
                    break;
                }
        }

        // Test to see if we have emphasis styling
        switch (em.state) {
            case Emphasis::NONE:
                if (link.state == Link::NO_LINK && !line.isHeading) {
                    int next = i + 1;
                    int prev = i - 1;
                    if ((c == '*' || c == '_') && (i == line.lineStart || markdown_[prev] == ' ' || markdown_[prev] == '\t')  // empasis must be preceded by whitespace or line start
                        && (int)markdownLength_ > next                                                                        // emphasis must precede non-whitespace
                        && markdown_[next] != ' ' && markdown_[next] != '\n' && markdown_[next] != '\t') {
                        em.state = Emphasis::LEFT;
                        em.sym = c;
                        em.text.start = i;
                        line.emphasisCount = 1;
                        continue;
                    }
                }
                break;
            case Emphasis::LEFT:
                if (em.sym == c) {
                    ++line.emphasisCount;
                    continue;
                } else {
                    em.text.start = i;
                    em.state = Emphasis::MIDDLE;
                }
                break;
            case Emphasis::MIDDLE:
                if (em.sym == c) {
                    em.state = Emphasis::RIGHT;
                    em.text.stop = i;
                    // pass through to case Emphasis::RIGHT
                } else {
                    break;
                }
            case Emphasis::RIGHT:
                if (em.sym == c) {
                    if (line.emphasisCount < 3 && (i - em.text.stop + 1 == line.emphasisCount)) {
                        // render text up to emphasis
                        int lineEnd = em.text.start - line.emphasisCount;
                        if (lineEnd > line.lineStart) {
                            line.lineEnd = lineEnd;
                            RenderLine(markdown_, line, textRegion, mdConfig_);
                            ImGui::SameLine(0.0f, 0.0f);
                            line.isUnorderedListStart = false;
                            line.leadSpaceCount = 0;
                        }
                        line.isEmphasis = true;
                        line.lastRenderPosition = em.text.start - 1;
                        line.lineStart = em.text.start;
                        line.lineEnd = em.text.stop;
                        RenderLine(markdown_, line, textRegion, mdConfig_);
                        ImGui::SameLine(0.0f, 0.0f);
                        line.isEmphasis = false;
                        line.lastRenderPosition = i;
                        em = Emphasis();
                    }
                    continue;
                } else {
                    em.state = Emphasis::NONE;
                    // render text up to here
                    int start = em.text.start - line.emphasisCount;
                    if (start < line.lineStart) {
                        line.lineEnd = line.lineStart;
                        line.lineStart = start;
                        line.lastRenderPosition = start - 1;
                        RenderLine(markdown_, line, textRegion, mdConfig_);
                        line.lineStart = line.lineEnd;
                        line.lastRenderPosition = line.lineStart - 1;
                    }
                }
                break;
        }

        // handle end of line (render)
        if (c == '\n') {
            // first check if the line is a horizontal rule
            line.lineEnd = i;
            if (em.state == Emphasis::MIDDLE && line.emphasisCount >= 3 && (line.lineStart + line.emphasisCount) == i) {
                ImGui::Separator();
            } else {
                // render the line: multiline emphasis requires a complex implementation so not supporting
                RenderLine(markdown_, line, textRegion, mdConfig_);
            }

            // reset the line and emphasis state
            line = Line();
            em = Emphasis();

            line.lineStart = i + 1;
            line.lastRenderPosition = i;

            textRegion.ResetIndent();

            // reset the link
            link = Link();
        }
    }

    if (em.state == Emphasis::LEFT && line.emphasisCount >= 3) {
        ImGui::Separator();
    } else {
        // render any remaining text if last char wasn't 0
        if (markdownLength_ && line.lineStart < (int)markdownLength_ && markdown_[line.lineStart] != 0) {
            // handle both null terminated and non null terminated strings
            line.lineEnd = (int)markdownLength_;
            if (0 == markdown_[line.lineEnd - 1]) {
                --line.lineEnd;
            }
            RenderLine(markdown_, line, textRegion, mdConfig_);
        }
    }
}

inline bool TextRegion::RenderLinkText(const char *text_, const char *text_end_, const Link &link_, const char *markdown_, const MarkdownConfig &mdConfig_, const char **linkHoverStart_) {
    MarkdownFormatInfo formatInfo;
    formatInfo.config = &mdConfig_;
    formatInfo.type = MarkdownFormatType::LINK;
    mdConfig_.formatCallback(formatInfo, true);
    ImGui::PushTextWrapPos(-1.0f);
    ImGui::TextUnformatted(text_, text_end_);
    ImGui::PopTextWrapPos();

    bool bThisItemHovered = ImGui::IsItemHovered();
    if (bThisItemHovered) {
        *linkHoverStart_ = markdown_ + link_.text.start;
    }
    bool bHovered = bThisItemHovered || (*linkHoverStart_ == (markdown_ + link_.text.start));

    formatInfo.itemHovered = bHovered;
    mdConfig_.formatCallback(formatInfo, false);

    if (bHovered) {
        if (ImGui::IsMouseReleased(0) && mdConfig_.linkCallback) {
            mdConfig_.linkCallback({markdown_ + link_.text.start, link_.text.size(), markdown_ + link_.url.start, link_.url.size(), mdConfig_.userData, false});
        }
        if (mdConfig_.tooltipCallback) {
            mdConfig_.tooltipCallback({{markdown_ + link_.text.start, link_.text.size(), markdown_ + link_.url.start, link_.url.size(), mdConfig_.userData, false}, mdConfig_.linkIcon});
        }
    }
    return bThisItemHovered;
}

inline void TextRegion::RenderLinkTextWrapped(const char *text_, const char *text_end_, const Link &link_, const char *markdown_, const MarkdownConfig &mdConfig_, const char **linkHoverStart_,
                                              bool bIndentToHere_) {
    float scale = ImGui::GetIO().FontGlobalScale;
    float widthLeft = GetContentRegionAvail().x;
    const char *endLine = ImGui::GetFont()->CalcWordWrapPositionA(scale, text_, text_end_, widthLeft);
    bool bHovered = RenderLinkText(text_, endLine, link_, markdown_, mdConfig_, linkHoverStart_);
    if (bIndentToHere_) {
        float indentNeeded = GetContentRegionAvail().x - widthLeft;
        if (indentNeeded) {
            ImGui::Indent(indentNeeded);
            indentX += indentNeeded;
        }
    }
    widthLeft = GetContentRegionAvail().x;
    while (endLine < text_end_) {
        text_ = endLine;
        if (*text_ == ' ') {
            ++text_;
        }  // skip a space at start of line
        endLine = ImGui::GetFont()->CalcWordWrapPositionA(scale, text_, text_end_, widthLeft);
        if (text_ == endLine) {
            endLine++;
        }
        bool bThisLineHovered = RenderLinkText(text_, endLine, link_, markdown_, mdConfig_, linkHoverStart_);
        bHovered = bHovered || bThisLineHovered;
    }
    if (!bHovered && *linkHoverStart_ == markdown_ + link_.text.start) {
        *linkHoverStart_ = NULL;
    }
}

inline void defaultMarkdownFormatCallback(const MarkdownFormatInfo &markdownFormatInfo_, bool start_) {
    switch (markdownFormatInfo_.type) {
        case MarkdownFormatType::NORMAL_TEXT:
            break;
        case MarkdownFormatType::EMPHASIS: {
            MarkdownHeadingFormat fmt;
            // default styling for emphasis uses last headingFormats - for your own styling
            // implement EMPHASIS in your formatCallback
            if (markdownFormatInfo_.level == 1) {
                // normal emphasis
                if (start_) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                } else {
                    ImGui::PopStyleColor();
                }
            } else {
                // strong emphasis
                fmt = markdownFormatInfo_.config->headingFormats[MarkdownConfig::NUMHEADINGS - 1];
                if (start_) {
                    if (fmt.font) {
                        ImGui::PushFont(fmt.font);
                    }
                } else {
                    if (fmt.font) {
                        ImGui::PopFont();
                    }
                }
            }
            break;
        }
        case MarkdownFormatType::HEADING: {
            MarkdownHeadingFormat fmt;
            if (markdownFormatInfo_.level > MarkdownConfig::NUMHEADINGS) {
                fmt = markdownFormatInfo_.config->headingFormats[MarkdownConfig::NUMHEADINGS - 1];
            } else {
                fmt = markdownFormatInfo_.config->headingFormats[markdownFormatInfo_.level - 1];
            }
            if (start_) {
                if (fmt.font) {
                    ImGui::PushFont(fmt.font);
                }
                ImGui::NewLine();
            } else {
                if (fmt.separator) {
                    ImGui::Separator();
                    ImGui::NewLine();
                } else {
                    ImGui::NewLine();
                }
                if (fmt.font) {
                    ImGui::PopFont();
                }
            }
            break;
        }
        case MarkdownFormatType::UNORDERED_LIST:
            break;
        case MarkdownFormatType::LINK:
            if (start_) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
            } else {
                ImGui::PopStyleColor();
                if (markdownFormatInfo_.itemHovered) {
                    ImGui::UnderLine(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
                } else {
                    ImGui::UnderLine(ImGui::GetStyle().Colors[ImGuiCol_Button]);
                }
            }
            break;
    }
}

}  // namespace ImGui

namespace ImGuiHelper {

bool color_picker_3U32(const char *label, ImU32 *color, ImGuiColorEditFlags flags = 0);

std::string file_browser(const std::string &path);

enum class Alignment : unsigned char {
    kHorizontalCenter = 1 << 0,
    kVerticalCenter = 1 << 1,
    kCenter = kHorizontalCenter | kVerticalCenter,
};

inline void AlignedText(const std::string &text, Alignment align, const float &width = 0.0F) {
    const auto alignment = static_cast<unsigned char>(align);
    const auto text_size = ImGui::CalcTextSize(text.c_str());
    const auto wind_size = ImGui::GetContentRegionAvail();
    if (alignment & static_cast<unsigned char>(Alignment::kHorizontalCenter)) {
        if (width < 0.1F) {
            ImGui::SetCursorPosX((wind_size.x - text_size.x) * 0.5F);
        } else {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (width - text_size.x) * 0.5F);
        }
    }
    if (alignment & static_cast<unsigned char>(Alignment::kVerticalCenter)) {
        ImGui::AlignTextToFramePadding();
    }

    ImGui::TextUnformatted(text.c_str());
}

inline auto CheckButton(const std::string &label, bool checked, const ImVec2 &size) -> bool {
    if (checked) {
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    } else {
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_TabUnfocusedActive]);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]);
    }
    if (ImGui::Button(label.c_str(), size)) {
        checked = !checked;
    }

    ImGui::PopStyleColor(2);

    return checked;
}

inline auto ButtonTab(std::vector<std::string> &tabs, int &index) -> int {
    auto checked = 1 << index;
    std::string tab_names;
    std::for_each(tabs.begin(), tabs.end(), [&tab_names](const auto item) { tab_names += item; });
    const auto tab_width = ImGui::GetContentRegionAvail().x;
    const auto tab_btn_width = tab_width / static_cast<float>(tabs.size());
    const auto h = ImGui::CalcTextSize(tab_names.c_str()).y;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, h);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, h);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]);
    ImGui::BeginChild(tab_names.c_str(), {tab_width, h + ImGui::GetStyle().FramePadding.y * 2}, false, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    for (int i = 0; i < tabs.size(); ++i) {
        auto &tab = tabs[i];

        // if current tab is checkd, uncheck otheres
        if (CheckButton(tab, checked & (1 << i), ImVec2{tab_btn_width, 0})) {
            checked = 0;
            checked = 1 << i;
        }

        if (i != tabs.size() - 1) {
            ImGui::SameLine();
        }
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
    ImGui::EndChild();

    index = 0;
    while (checked / 2) {
        checked = checked / 2;
        ++index;
    }

    return index;
}

inline void SwitchButton(std::string &&label, bool &checked) {
    float height = ImGui::GetFrameHeight();
    float width = height * 1.55F;
    float radius = height * 0.50F;
    const auto frame_width = ImGui::GetContentRegionAvail().x;

    AlignedText("    " + label, Alignment::kVerticalCenter);
    ImGui::SameLine();

    ImGui::SetCursorPosX(frame_width - width);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    if (ImGui::InvisibleButton(label.c_str(), ImVec2(width, height))) {
        checked = !checked;
    }
    ImU32 col_bg = 0;
    if (checked) {
        col_bg = ImColor(ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    } else {
        col_bg = ImColor(ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    }
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), col_bg, radius);
    draw_list->AddCircleFilled(ImVec2(checked ? (pos.x + width - radius) : (pos.x + radius), pos.y + radius), radius - 1.5F, IM_COL32_WHITE);
}

inline void Comb(std::string &&icon, std::string &&label, const std::vector<const char *> &items, int &index) {
    const auto p_w = ImGui::GetContentRegionAvail().x;
    AlignedText(icon + "    " + label, Alignment::kVerticalCenter);
    ImGui::SameLine();
    ImGui::SetCursorPosX(p_w - 150.0F - ImGui::GetStyle().FramePadding.x);
    ImGui::SetNextItemWidth(150.0F);
    ImGui::Combo((std::string("##") + label).c_str(), &index, items.data(), static_cast<int>(items.size()));
}

inline void InputInt(std::string &&icon, std::string &&label, int &value) {
    const auto p_w = ImGui::GetContentRegionAvail().x;
    AlignedText(icon + "    " + label, Alignment::kVerticalCenter);
    ImGui::SameLine();
    ImGui::SetCursorPosX(p_w - 100.0F - ImGui::GetStyle().FramePadding.x);
    ImGui::SetNextItemWidth(100.0F);
    ImGui::InputInt((std::string("##") + label).c_str(), &value);
}

inline void ListSeparator(float indent = 30.0F) {
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return;
    }

    ImGuiContext &g = *GImGui;

    float thickness_draw = 1.0F;
    float thickness_layout = 0.0F;
    // Horizontal Separator
    float x1 = window->Pos.x + indent;
    float x2 = window->Pos.x + window->Size.x;

    // FIXME-WORKRECT: old hack (#205) until we decide of consistent behavior with WorkRect/Indent and Separator
    if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID) {
        x1 += window->DC.Indent.x;
    }

    // We don't provide our width to the layout so that it doesn't get feed back into AutoFit
    const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + thickness_draw));
    ImGui::ItemSize(ImVec2(0.0F, thickness_layout));
    const bool item_visible = ImGui::ItemAdd(bb, 0);
    if (item_visible) {
        // Draw
        window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), ImGui::GetColorU32(ImGuiCol_Separator));
    }
}

}  // namespace ImGuiHelper

#ifdef ME_IMM32

#if defined(_WIN32)
#include <CommCtrl.h>
#include <Windows.h>
#include <tchar.h>
#include <vcruntime_string.h>
#if !defined(IMGUI_IMM_COMMAND_BEGIN)
#define IMGUI_IMM_COMMAND_BEGIN (WM_APP + 0x200)
#endif
#endif

struct ME_imgui_imm {

    enum { IMGUI_IMM_COMMAND = IMGUI_IMM_COMMAND_BEGIN, IMGUI_IMM_END };

    enum { IMGUI_IMM_COMMAND_NOP = 0u, IMGUI_IMM_COMMAND_SUBCLASSIFY, IMGUI_IMM_COMMAND_COMPOSITION_COMPLETE, IMGUI_IMM_COMMAND_CLEANUP };

    struct imm_candidate_list {
        std::vector<std::string> list_utf8;
        std::vector<std::string>::size_type selection;

        imm_candidate_list() : list_utf8{}, selection(0) {}
        imm_candidate_list(const imm_candidate_list &rhv) = default;
        imm_candidate_list(imm_candidate_list &&rhv) noexcept : list_utf8(), selection(0) { *this = std::move(rhv); }

        ~imm_candidate_list() = default;

        inline imm_candidate_list &operator=(const imm_candidate_list &rhv) = default;

        inline imm_candidate_list &operator=(imm_candidate_list &&rhv) noexcept {
            if (this == &rhv) {
                return *this;
            }
            std::swap(list_utf8, rhv.list_utf8);
            std::swap(selection, rhv.selection);
            return *this;
        }
        inline void clear() {
            list_utf8.clear();
            selection = 0;
        }

        static imm_candidate_list cocreate(const CANDIDATELIST *const src, const size_t src_size);
    };

    static constexpr int candidate_window_num = 9;

    bool is_open;
    std::unique_ptr<char[]> comp_conved_utf8;
    std::unique_ptr<char[]> comp_target_utf8;
    std::unique_ptr<char[]> comp_unconv_utf8;
    bool show_ime_candidate_list;
    int request_candidate_list_str_commit;  // 候选列表在1更新后，请求移动到下一个转换候选
    imm_candidate_list candidate_list;

    ME_imgui_imm()
        : is_open(false), comp_conved_utf8(nullptr), comp_target_utf8(nullptr), comp_unconv_utf8(nullptr), show_ime_candidate_list(false), request_candidate_list_str_commit(false), candidate_list() {}

    ~ME_imgui_imm() = default;
    void operator()();

private:
    bool update_candidate_window(HWND hWnd);

    static LRESULT WINAPI imm_communication_subclassproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT imm_communication_subclassproc_implement(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, ME_imgui_imm &comm);
    BOOL subclassify_impl(HWND hWnd);

public:
    template <typename type_t>
    inline BOOL subclassify(type_t hWnd);

    template <>
    inline BOOL subclassify<HWND>(HWND hWnd) {
        return subclassify_impl(hWnd);
    }
};

// template <>
// inline BOOL ME_imgui_imm::subclassify<GLFWwindow *>(GLFWwindow *window) {
//     return this->subclassify(glfwGetWin32Window(window));
// }

template <>
inline BOOL ME_imgui_imm::subclassify<SDL_Window *>(SDL_Window *window) {
    SDL_SysWMinfo info{};
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info)) {
        IM_ASSERT(IsWindow(info.info.win.window));
        return this->subclassify(info.info.win.window);
    }
    return FALSE;
}

int common_control_initialize();

#endif

void ShowAutoTestWindow();

#endif