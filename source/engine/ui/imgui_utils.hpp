
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
#include "libs/md4c.h"

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

static_inline ImVec4 vec4_to_imvec4(const ME::MEvec4 &v4) { return {v4.x, v4.y, v4.z, v4.w}; }
static_inline ImColor vec4_to_imcolor(const ME::MEvec4 &v4) { return {v4.x * 255.0f, v4.y * 255.0f, v4.z * 255.0f, v4.w * 255.0f}; }

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

ME_INLINE ImVec4 ME_rgba2imvec(int r, int g, int b, int a = 255) {
    float newr = r / 255.f;
    float newg = g / 255.f;
    float newb = b / 255.f;
    float newa = a / 255.f;
    return ImVec4(newr, newg, newb, newa);
}

ME_INLINE ME::MEcolor ME_imvec2rgba(ImVec4 iv) {
    u8 newr = iv.x * 255;
    u8 newg = iv.y * 255;
    u8 newb = iv.z * 255;
    u8 newa = iv.w * 255;
    return ME::MEcolor(newr, newg, newb, newa);
}

namespace ImGuiHelper {

struct imgui_md {
    imgui_md();
    virtual ~imgui_md(){};
    int print(const std::string &str);
    bool m_table_border = true;
    bool m_table_header_highlight = true;

protected:
    virtual void BLOCK_DOC(bool);
    virtual void BLOCK_QUOTE(bool);
    virtual void BLOCK_UL(const MD_BLOCK_UL_DETAIL *, bool);
    virtual void BLOCK_OL(const MD_BLOCK_OL_DETAIL *, bool);
    virtual void BLOCK_LI(const MD_BLOCK_LI_DETAIL *, bool);
    virtual void BLOCK_HR(bool e);
    virtual void BLOCK_H(const MD_BLOCK_H_DETAIL *d, bool e);
    virtual void BLOCK_CODE(const MD_BLOCK_CODE_DETAIL *, bool);
    virtual void BLOCK_HTML(bool);
    virtual void BLOCK_P(bool);
    virtual void BLOCK_TABLE(const MD_BLOCK_TABLE_DETAIL *, bool);
    virtual void BLOCK_THEAD(bool);
    virtual void BLOCK_TBODY(bool);
    virtual void BLOCK_TR(bool);
    virtual void BLOCK_TH(const MD_BLOCK_TD_DETAIL *, bool);
    virtual void BLOCK_TD(const MD_BLOCK_TD_DETAIL *, bool);

    virtual void SPAN_EM(bool e);
    virtual void SPAN_STRONG(bool e);
    virtual void SPAN_A(const MD_SPAN_A_DETAIL *d, bool e);
    virtual void SPAN_IMG(const MD_SPAN_IMG_DETAIL *, bool);
    virtual void SPAN_CODE(bool);
    virtual void SPAN_DEL(bool);
    virtual void SPAN_LATEXMATH(bool);
    virtual void SPAN_LATEXMATH_DISPLAY(bool);
    virtual void SPAN_WIKILINK(const MD_SPAN_WIKILINK_DETAIL *, bool);
    virtual void SPAN_U(bool);

    ////////////////////////////////////////////////////////////////////////////

    struct image_info {
        ImTextureID texture_id;
        ImVec2 size;
        ImVec2 uv0;
        ImVec2 uv1;
        ImVec4 col_tint;
        ImVec4 col_border;
    };

    // use m_href to identify image
    virtual bool get_image(image_info &nfo) const;

    virtual ImFont *get_font() const;
    virtual ImVec4 get_color() const;

    // url == m_href
    virtual void open_url() const;

    // returns true if the term has been processed
    virtual bool render_entity(const char *str, const char *str_end);

    // returns true if the term has been processed
    virtual bool check_html(const char *str, const char *str_end);

    // called when '\n' in source text where it is not semantically meaningful
    virtual void soft_break();

    // e==true : enter
    // e==false : leave
    virtual void html_div(const std::string &dclass, bool e);
    ////////////////////////////////////////////////////////////////////////////

    // current state
    std::string m_href;  // empty if no link/image

    bool m_is_underline = false;
    bool m_is_strikethrough = false;
    bool m_is_em = false;
    bool m_is_strong = false;
    bool m_is_table_header = false;
    bool m_is_table_body = false;
    bool m_is_image = false;
    bool m_is_code = false;
    unsigned m_hlevel = 0;  // 0 - no heading

private:
    int text(MD_TEXTTYPE type, const char *str, const char *str_end);
    int block(MD_BLOCKTYPE type, void *d, bool e);
    int span(MD_SPANTYPE type, void *d, bool e);

    void render_text(const char *str, const char *str_end);

    void set_font(bool e);
    void set_color(bool e);
    void set_href(bool e, const MD_ATTRIBUTE &src);

    static void line(ImColor c, bool under);

    // table state
    int m_table_next_column = 0;
    ImVec2 m_table_last_pos;
    std::vector<float> m_table_col_pos;
    std::vector<float> m_table_row_pos;

    // list state
    struct list_info {
        unsigned cur_ol;
        char delim;
        bool is_ol;
    };
    std::vector<list_info> m_list_stack;

    std::vector<std::string> m_div_stack;

    MD_PARSER m_md;
};

struct dbgui_md : public imgui_md {
    // ImFont *get_font() const override {
    //     if (m_is_table_header) {
    //         return g_font_bold;
    //     }

    //    switch (m_hlevel) {
    //        case 0:
    //            return m_is_strong ? g_font_bold : g_font_regular;
    //        case 1:
    //            return g_font_bold_large;
    //        default:
    //            return g_font_bold;
    //    }
    //};

    ME_INLINE void open_url() const override {
        // platform dependent code
        SDL_OpenURL(m_href.c_str());
    }

    // ME_INLINE bool get_image(image_info &nfo) const override {
    //     // use m_href to identify images
    //     nfo.texture_id = g_texture1;
    //     nfo.size = {40, 20};
    //     nfo.uv0 = {0, 0};
    //     nfo.uv1 = {1, 1};
    //     nfo.col_tint = {1, 1, 1, 1};
    //     nfo.col_border = {0, 0, 0, 0};
    //     return true;
    // }

    ME_INLINE void html_div(const std::string &dclass, bool e) override {
        if (dclass == "red") {
            if (e) {
                m_table_border = false;
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
            } else {
                ImGui::PopStyleColor();
                m_table_border = true;
            }
        }
    }
};

ME_INLINE void markdown(const std::string &str) {
    static dbgui_md s_printer;
    s_printer.print(str);
}

bool color_picker_3U32(const char *label, ImU32 *color, ImGuiColorEditFlags flags = 0);

void file_browser(std::string &path);

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