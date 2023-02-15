// Copyright(c) 2022-2023, KaoruXun

// This source file may include
// https://github.com/maildrop/DearImGui-with-IMM32 (MIT) by TOGURO Mikito
// https://github.com/mekhontsev/imgui_md (MIT) by mekhontsev
// https://github.com/ocornut/imgui (MIT) by Omar Cornut

#ifndef _METADOT_IMGUIHELPER_HPP_
#define _METADOT_IMGUIHELPER_HPP_

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "libs/imgui/imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "libs/imgui/imgui_internal.h"

#if defined(_WIN32)
#define _METADOT_IMM32
#else
#include <sys/stat.h>
#endif

#include "core/core.hpp"
#include "libs/imgui/md4c.h"

// Backend API
bool ImGui_ImplOpenGL3_Init();
void ImGui_ImplOpenGL3_Shutdown();
void ImGui_ImplOpenGL3_NewFrame();
void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData *draw_data);

// (Optional) Called by Init/NewFrame/Shutdown
bool ImGui_ImplOpenGL3_CreateFontsTexture();
void ImGui_ImplOpenGL3_DestroyFontsTexture();
bool ImGui_ImplOpenGL3_CreateDeviceObjects();
void ImGui_ImplOpenGL3_DestroyDeviceObjects();

struct SDL_Window;
struct SDL_Renderer;
typedef union SDL_Event SDL_Event;

bool ImGui_ImplSDL2_Init(SDL_Window *window, void *sdl_gl_context);
void ImGui_ImplSDL2_Shutdown();
void ImGui_ImplSDL2_NewFrame();
bool ImGui_ImplSDL2_ProcessEvent(const SDL_Event *event);

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
static inline void ImGui_ImplSDL2_NewFrame(SDL_Window *) { ImGui_ImplSDL2_NewFrame(); }  // 1.84: removed unnecessary parameter
#endif

#pragma once

#pragma region ImGuiError

namespace ImGui {

static std::string format(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char smallBuffer[1024];
    size_t size = vsnprintf(smallBuffer, sizeof smallBuffer, format, args);
    va_end(args);

    if (size < 1024) return std::string(smallBuffer);

    char *buffer = (char *)ImGui::MemAlloc(size + 1);

    va_start(args, format);
    vsnprintf(buffer, size + 1, format, args);
    va_end(args);
    return std::string(buffer);
}

#ifndef IMGUICSS_LOG_ERROR
#define IMGUICSS_LOG_ERROR(desc, file, line) std::cout << desc << " " << file << ":" << line << "\n";
#endif

#if defined(IMGUICSS_NO_EXCEPTIONS)
#if defined(IMGUICSS_LOG_ERROR)
#define IMGUICSS_EXCEPTION(cls, ...) IMGUICSS_LOG_ERROR(format(__VA_ARGS__), __FILE__, __LINE__)
#else
#define IMGUICSS_EXCEPTION(cls, ...)
#endif
#else
#define IMGUICSS_EXCEPTION(cls, ...) throw cls(ImGui::format(__VA_ARGS__))
#endif

/**
 * Style related errors
 */
class StyleError : public std::runtime_error {
public:
    StyleError(const std::string &detailedMessage) : runtime_error("Style error: " + detailedMessage) {}
};

/**
 * Raised when script execution fails
 */
class ScriptError : public std::runtime_error {
public:
    ScriptError(const std::string &detailedMessage) : runtime_error("Script execution error: " + detailedMessage) {}
};

/**
 * Element creation failure (incorrect arguments, missing required arguments)
 */
class ElementError : public std::runtime_error {
public:
    ElementError(const std::string &detailedMessage) : runtime_error("Element build error: " + detailedMessage) {}
};

}  // namespace ImGui

#pragma endregion ImGuiError

#pragma region ImGuiAuto

#include <string>

#ifndef METAENGINE_GUI_TREE_MAX_ELEMENT_SIZE
#define METAENGINE_GUI_TREE_MAX_ELEMENT_SIZE sizeof(std::string)  // larger values generate less tree nodes
#endif
#ifndef METAENGINE_GUI_TREE_MAX_TUPLE_ELEMENTS
#define METAENGINE_GUI_TREE_MAX_TUPLE_ELEMENTS 3  // larger values generate less tree nodes
#endif

#include <tuple>
#define _TUPLE_
#include <vector>
#define _VECTOR_
#include <deque>
#define _DEQUE_
#include <map>
#define _MAP_
#include <list>
#define _LIST_
#include <forward_list>
#define _FORWARD_LIST_
#include <array>
#define _ARRAY_
#include <set>
#define _SET_
#include <unordered_set>
#define _UNORDERED_SET_
#include <unordered_map>
#define _UNORDERED_MAP_

#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>

#include "internal/builtin_pfr.hpp"
#define METAENGINE_GUI_STRUCT_TO_TUPLE BuiltinPFR::pfr::structure_tie

namespace ImGui {
//		IMGUI::AUTO()
//		=============

// This is the function that this libary implements. (It's just a wrapper around the class ImGui::Auto_t<AnyType>)
template <typename AnyType>
void Auto(AnyType &anything, const std::string &name = std::string());

//		Test function showcasing all the features of ImGui::Auto()
void ShowAutoTestWindow();

//		HELPER FUNCTIONS
//		================

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

#ifdef _TUPLE_  // For tuples

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
#endif  // _TUPLE_

template <typename T, std::size_t N>
using c_array_t = T[N];  // so arrays are regular types and can be used in macro

// template<typename Container> inline std::size_t AutoContainerSize(Container &cont, std::enable_if_t< std::is_array_v<Container>>*=0){	return sizeof(Container) /
// sizeof(decltype(*std::begin(cont)));} template<typename Container> inline std::size_t AutoContainerSize(Container &cont, std::enable_if_t<!std::is_array_v<Container>> *= 0) { return cont.size(); }

}  // namespace detail
#pragma endregion

//		PRIMARY TEMPLATE
//		================
// This implements the struct to tuple scenario
template <typename AnyType>
struct Auto_t {
    static void Auto(AnyType &anything, const std::string &name) {
#ifndef METAENGINE_GUI_STRUCT_TO_TUPLE
        static_assert(false, "TODO: fix for this compiler! (at least C++14 is required)")
#endif
                static_assert(
                        !std::is_reference_v<AnyType> && std::is_copy_constructible_v<std::remove_all_extents_t<AnyType>> && !std::is_polymorphic_v<AnyType> &&
                                BuiltinPFR::pfr::detail::is_aggregate_initializable_n<AnyType, BuiltinPFR::pfr::detail::detect_fields_count_dispatch<AnyType>(
                                                                                                       BuiltinPFR::pfr::detail::size_t_<sizeof(AnyType) * 8>{},
                                                                                                       1L)>::value,  // If the above is not a constexpr expression, you are yousing an invalid type
                        "This type cannot be converted to a tuple.");
        auto tuple = METAENGINE_GUI_STRUCT_TO_TUPLE(anything);
        ImGui::detail::AutoTuple("Struct " + name, tuple);
    }
};  // ImGui::Auto_t<>::Auto()
}  // namespace ImGui

// Implementation of ImGui::Auto()
template <typename AnyType>
inline void ImGui::Auto(AnyType &anything, const std::string &name) {
    ImGui::Auto_t<AnyType>::Auto(anything, name);
}

//		HELPER FUNCTIONS
//		================

#pragma region UTILS

template <typename T>
bool ImGui::detail::AutoExpand(const std::string &name, T &value) {
    if (sizeof(T) <= METAENGINE_GUI_TREE_MAX_ELEMENT_SIZE) {
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
        ImGui::Text("size = %d, non dynamic elemsize = %d bytes", (int)size, (int)elemsize);
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
    bool b_k = sizeof(Key) <= METAENGINE_GUI_TREE_MAX_ELEMENT_SIZE;
    bool b_v = sizeof(Value) <= METAENGINE_GUI_TREE_MAX_ELEMENT_SIZE;
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
    if (tuple_size <= METAENGINE_GUI_TREE_MAX_ELEMENT_SIZE && tuple_numelems <= METAENGINE_GUI_TREE_MAX_TUPLE_ELEMENTS) {
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
    if (tuple_size <= METAENGINE_GUI_TREE_MAX_ELEMENT_SIZE && tuple_numelems <= METAENGINE_GUI_TREE_MAX_TUPLE_ELEMENTS) {
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

//		HELPER MACROS
//		=============

#define UNPACK(...) __VA_ARGS__  // for unpacking parentheses. It is needed for macro arguments with commmas
// Enclose templatespec, AND typespec in parentheses in this version. Useful if there are commas in the argument.
#define METAENGINE_GUI_DEFINE_BEGIN_P(templatespec, typespec) \
    namespace ImGui {                                         \
    UNPACK templatespec struct Auto_t<UNPACK typespec> {      \
        static void Auto(UNPACK typespec &var, const std::string &name) {
// If macro arguments have no commmas inside use this version without parentheses
#define METAENGINE_GUI_DEFINE_BEGIN(templatespec, typespec) METAENGINE_GUI_DEFINE_BEGIN_P((templatespec), (typespec))  // when there are no commas in types, use this without parentheses
#define METAENGINE_GUI_DEFINE_END \
    }                             \
    }                             \
    ;                             \
    }
#define METAENGINE_GUI_DEFINE_INLINE_P(template_spec, type_spec, code) METAENGINE_GUI_DEFINE_BEGIN_P(template_spec, type_spec) code METAENGINE_GUI_DEFINE_END
#define METAENGINE_GUI_DEFINE_INLINE(template_spec, type_spec, code) METAENGINE_GUI_DEFINE_INLINE_P((template_spec), (type_spec), code)

#include <string>
#include <type_traits>
#include <utility>

#ifndef METAENGINE_GUI_INPUT_FLOAT1
#define METAENGINE_GUI_INPUT_FLOAT1 ImGui::DragFloat
#endif
#ifndef METAENGINE_GUI_INPUT_FLOAT2
#define METAENGINE_GUI_INPUT_FLOAT2 ImGui::DragFloat2
#endif
#ifndef METAENGINE_GUI_INPUT_FLOAT3
#define METAENGINE_GUI_INPUT_FLOAT3 ImGui::DragFloat3
#endif
#ifndef METAENGINE_GUI_INPUT_FLOAT4
#define METAENGINE_GUI_INPUT_FLOAT4 ImGui::DragFloat4
#endif
#ifndef METAENGINE_GUI_INPUT_INT1
#define METAENGINE_GUI_INPUT_INT1 ImGui::InputInt
#endif
#ifndef METAENGINE_GUI_INPUT_INT2
#define METAENGINE_GUI_INPUT_INT2 ImGui::InputInt2
#endif
#ifndef METAENGINE_GUI_INPUT_INT3
#define METAENGINE_GUI_INPUT_INT3 ImGui::InputInt3
#endif
#ifndef METAENGINE_GUI_INPUT_INT4
#define METAENGINE_GUI_INPUT_INT4 ImGui::InputInt4
#endif
#ifndef METAENGINE_GUI_NULLPTR_COLOR
#define METAENGINE_GUI_NULLPTR_COLOR ImVec4(1.0, 0.5, 0.5, 1.0)
#endif

//		SPECIALIZATIONS
//		===============

#pragma region STRINGS

METAENGINE_GUI_DEFINE_BEGIN(template <>, const char *)
if (name.empty())
    ImGui::TextUnformatted(var);
else
    ImGui::Text("%s=%s", name.c_str(), var);
METAENGINE_GUI_DEFINE_END

METAENGINE_GUI_DEFINE_BEGIN_P((template <std::size_t N>), (const detail::c_array_t<char, N>))
if (name.empty())
    ImGui::TextUnformatted(var, var + N - 1);
else
    ImGui::Text("%s=%s", name.c_str(), var);
METAENGINE_GUI_DEFINE_END

METAENGINE_GUI_DEFINE_INLINE(template <>, char *, const char *tmp = var; ImGui::Auto_t<const char *>::Auto(tmp, name);)
METAENGINE_GUI_DEFINE_INLINE(template <>, char *const, const char *tmp = var; ImGui::Auto_t<const char *>::Auto(tmp, name);)
METAENGINE_GUI_DEFINE_INLINE(template <>, const char *const, const char *tmp = var; ImGui::Auto_t<const char *>::Auto(tmp, name);)
METAENGINE_GUI_DEFINE_BEGIN(template <>, std::string)
const std::size_t lines = var.find('\n');
if (var.find('\n') != std::string::npos)
    ImGui::InputTextMultiline(name.c_str(), const_cast<char *>(var.c_str()), 256);
else
    ImGui::InputText(name.c_str(), const_cast<char *>(var.c_str()), 256);
METAENGINE_GUI_DEFINE_END
METAENGINE_GUI_DEFINE_BEGIN(template <>, const std::string)
if (name.empty())
    ImGui::TextUnformatted(var.c_str(), var.c_str() + var.length());
else
    ImGui::Text("%s=%s", name.c_str(), var.c_str());
METAENGINE_GUI_DEFINE_END

#pragma endregion

#pragma region NUMBERS

METAENGINE_GUI_DEFINE_INLINE(template <>, float, METAENGINE_GUI_INPUT_FLOAT1(name.c_str(), &var);)
METAENGINE_GUI_DEFINE_INLINE(template <>, int, METAENGINE_GUI_INPUT_INT1(name.c_str(), &var);)
METAENGINE_GUI_DEFINE_INLINE(template <>, unsigned int, METAENGINE_GUI_INPUT_INT1(name.c_str(), (int *)&var);)
METAENGINE_GUI_DEFINE_INLINE(template <>, bool, ImGui::Checkbox(name.c_str(), &var);)
METAENGINE_GUI_DEFINE_INLINE(template <>, ImVec2, METAENGINE_GUI_INPUT_FLOAT2(name.c_str(), &var.x);)
METAENGINE_GUI_DEFINE_INLINE(template <>, ImVec4, METAENGINE_GUI_INPUT_FLOAT4(name.c_str(), &var.x);)
METAENGINE_GUI_DEFINE_INLINE(template <>, const float, ImGui::Auto_t<const std::string>::Auto(std::to_string(var), name);)
METAENGINE_GUI_DEFINE_INLINE(template <>, const int, ImGui::Auto_t<const std::string>::Auto(std::to_string(var), name);)
METAENGINE_GUI_DEFINE_INLINE(template <>, const unsigned, ImGui::Auto_t<const std::string>::Auto(std::to_string(var), name);)
METAENGINE_GUI_DEFINE_INLINE(template <>, const bool, ImGui::Auto_t<const std::string>::Auto(std::to_string(var), name);)
METAENGINE_GUI_DEFINE_INLINE(template <>, const ImVec2, ImGui::Text("%s(%f,%f)", (name.empty() ? "" : name + "=").c_str(), var.x, var.y);)
METAENGINE_GUI_DEFINE_INLINE(template <>, const ImVec4, ImGui::Text("%s(%f,%f,%f,%f)", (name.empty() ? "" : name + "=").c_str(), var.x, var.y, var.z, var.w);)

METAENGINE_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<float, 1>), METAENGINE_GUI_INPUT_FLOAT1(name.c_str(), &var[0]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<float, 1>), ImGui::Text("%s%f", (name.empty() ? "" : name + "=").c_str(), var[0]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<float, 2>), METAENGINE_GUI_INPUT_FLOAT2(name.c_str(), &var[0]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<float, 2>), ImGui::Text("%s(%f,%f)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<float, 3>), METAENGINE_GUI_INPUT_FLOAT3(name.c_str(), &var[0]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<float, 3>), ImGui::Text("%s(%f,%f,%f)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1], var[2]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<float, 4>), METAENGINE_GUI_INPUT_FLOAT4(name.c_str(), &var[0]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<float, 4>), ImGui::Text("%s(%f,%f,%f,%f)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1], var[2], var[3]);)

METAENGINE_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<int, 1>), METAENGINE_GUI_INPUT_INT1(name.c_str(), &var[0]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<int, 1>), ImGui::Text("%s%d", (name.empty() ? "" : name + "=").c_str(), var[0]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<int, 2>), METAENGINE_GUI_INPUT_INT2(name.c_str(), &var[0]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<int, 2>), ImGui::Text("%s(%d,%d)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<int, 3>), METAENGINE_GUI_INPUT_INT3(name.c_str(), &var[0]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<int, 3>), ImGui::Text("%s(%d,%d,%d)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1], var[2]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (detail::c_array_t<int, 4>), METAENGINE_GUI_INPUT_INT4(name.c_str(), &var[0]);)
METAENGINE_GUI_DEFINE_INLINE_P((template <>), (const detail::c_array_t<int, 4>), ImGui::Text("%s(%d,%d,%d,%d)", (name.empty() ? "" : name + "=").c_str(), var[0], var[1], var[2], var[3]);)

#pragma endregion

#pragma region POINTERS and ARRAYS
METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, T *)
if (var != nullptr)
    ImGui::detail::AutoExpand<T>("Pointer " + name, *var);
else
    ImGui::TextColored(METAENGINE_GUI_NULLPTR_COLOR, "%s=NULL", name.c_str());
METAENGINE_GUI_DEFINE_END
METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, T *const)
if (var != nullptr)
    ImGui::detail::AutoExpand<T>("Pointer " + name, *var);
else
    ImGui::TextColored(METAENGINE_GUI_NULLPTR_COLOR, "%s=NULL", name.c_str());
METAENGINE_GUI_DEFINE_END
#ifdef _ARRAY_
METAENGINE_GUI_DEFINE_INLINE_P((template <typename T, std::size_t N>), (std::array<T, N>), ImGui::detail::AutoContainerValues("array " + name, var);)
METAENGINE_GUI_DEFINE_INLINE_P((template <typename T, std::size_t N>), (const std::array<T, N>), ImGui::detail::AutoContainerValues("array " + name, var);)
METAENGINE_GUI_DEFINE_INLINE_P((template <typename T, std::size_t N>), (detail::c_array_t<T, N>), ImGui::detail::AutoContainerValues("Array " + name, *(std::array<T, N> *)(&var));)
METAENGINE_GUI_DEFINE_INLINE_P((template <typename T, std::size_t N>), (const detail::c_array_t<T, N>), ImGui::detail::AutoContainerValues("Array " + name, *(const std::array<T, N> *)(&var));)
#endif

#pragma endregion

#pragma region PAIRS and TUPLES

METAENGINE_GUI_DEFINE_BEGIN_P((template <typename T1, typename T2>), (std::pair<T1, T2>))
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

METAENGINE_GUI_DEFINE_END

METAENGINE_GUI_DEFINE_BEGIN_P((template <typename T1, typename T2>), (const std::pair<T1, T2>))
ImGui::detail::AutoExpand<const T1>(name + ".first", var.first);
if (std::is_fundamental_v<T1> && std::is_fundamental_v<T2>) ImGui::SameLine();
ImGui::detail::AutoExpand<const T2>(name + ".second", var.second);
METAENGINE_GUI_DEFINE_END

#ifdef _TUPLE_
METAENGINE_GUI_DEFINE_INLINE(template <typename... Args>, std::tuple<Args...>, ImGui::detail::AutoTuple("Tuple " + name, var);)
METAENGINE_GUI_DEFINE_INLINE(template <typename... Args>, const std::tuple<Args...>, ImGui::detail::AutoTuple("Tuple " + name, var);)
#endif  //_TUPLE_

#pragma endregion

#pragma region CONTAINERS
#ifdef _VECTOR_
METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, std::vector<T>)
if (ImGui::detail::AutoContainerValues<std::vector<T>>("Vector " + name, var)) {
    ImGui::PushID(name.c_str());
    ImGui::Indent();
    ImGui::detail::AutoContainerPushBackButton(var);
    if (!var.empty()) ImGui::SameLine();
    ImGui::detail::AutoContainerPopBackButton(var);
    ImGui::PopID();
    ImGui::Unindent();
}
METAENGINE_GUI_DEFINE_END
METAENGINE_GUI_DEFINE_BEGIN(template <>, std::vector<bool>)
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
METAENGINE_GUI_DEFINE_END

METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, const std::vector<T>)
ImGui::detail::AutoContainerValues<const std::vector<T>>("Vector " + name, var);
METAENGINE_GUI_DEFINE_END
METAENGINE_GUI_DEFINE_BEGIN(template <>, const std::vector<bool>)
if (ImGui::detail::AutoContainerTreeNode<const std::vector<bool>>("Vector " + name, var)) {
    ImGui::Indent();
    for (int i = 0; i < var.size(); ++i) {
        ImGui::Bullet();
        ImGui::Auto_t<const bool>::Auto(var[i], '[' + std::to_string(i) + ']');
    }
    ImGui::Unindent();
}
METAENGINE_GUI_DEFINE_END
#endif

#ifdef _LIST_
METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, std::list<T>)
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
METAENGINE_GUI_DEFINE_END
METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, const std::list<T>)
ImGui::detail::AutoContainerValues<const std::list<T>>("List " + name, var);
METAENGINE_GUI_DEFINE_END
#endif  // _LIST_

#ifdef _DEQUE_
METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, std::deque<T>)
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
METAENGINE_GUI_DEFINE_END
METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, const std::deque<T>)
ImGui::detail::AutoContainerValues<const std::deque<T>>("Deque " + name, var);
METAENGINE_GUI_DEFINE_END
#endif  // _DEQUE_

#ifdef _FORWARD_LIST_
METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, std::forward_list<T>)
if (ImGui::detail::AutoContainerValues<std::forward_list<T>>("Forward list " + name, var)) {
    ImGui::PushID(name.c_str());
    ImGui::Indent();
    ImGui::detail::AutoContainerPushFrontButton(var);
    if (!var.empty()) ImGui::SameLine();
    ImGui::detail::AutoContainerPopFrontButton(var);
    ImGui::PopID();
    ImGui::Unindent();
}
METAENGINE_GUI_DEFINE_END
METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, const std::forward_list<T>)
ImGui::detail::AutoContainerValues<const std::forward_list<T>>("Forward list " + name, var);
METAENGINE_GUI_DEFINE_END
#endif  // _FORWARD_LIST_

#ifdef _SET_
METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, std::set<T>)
ImGui::detail::AutoContainerValues<std::set<T>>("Set " + name, var);
// todo insert
METAENGINE_GUI_DEFINE_END
METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, const std::set<T>)
ImGui::detail::AutoContainerValues<const std::set<T>>("Set " + name, var);
METAENGINE_GUI_DEFINE_END
#endif  // _SET_

#ifdef _UNORDERED_SET_
METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, std::unordered_set<T>)
ImGui::detail::AutoContainerValues<std::unordered_set<T>>("Unordered set " + name, var);
// todo insert
METAENGINE_GUI_DEFINE_END
METAENGINE_GUI_DEFINE_BEGIN(template <typename T>, const std::unordered_set<T>)
ImGui::detail::AutoContainerValues<const std::unordered_set<T>>("Unordered set " + name, var);
METAENGINE_GUI_DEFINE_END
#endif  // _UNORDERED_SET_

#ifdef _MAP_
METAENGINE_GUI_DEFINE_BEGIN_P((template <typename K, typename V>), (std::map<K, V>))
ImGui::detail::AutoMapContainerValues<std::map<K, V>>("Map " + name, var);
// todo insert
METAENGINE_GUI_DEFINE_END
METAENGINE_GUI_DEFINE_BEGIN_P((template <typename K, typename V>), (const std::map<K, V>))
ImGui::detail::AutoMapContainerValues<const std::map<K, V>>("Map " + name, var);
METAENGINE_GUI_DEFINE_END
#endif  // _MAP_

#ifdef _UNORDERED_MAP_
METAENGINE_GUI_DEFINE_BEGIN_P((template <typename K, typename V>), (std::unordered_map<K, V>))
ImGui::detail::AutoMapContainerValues<std::unordered_map<K, V>>("Unordered map " + name, var);
// todo insert
METAENGINE_GUI_DEFINE_END
METAENGINE_GUI_DEFINE_BEGIN_P((template <typename K, typename V>), (const std::unordered_map<K, V>))
ImGui::detail::AutoMapContainerValues<const std::unordered_map<K, V>>("Unordered map " + name, var);
METAENGINE_GUI_DEFINE_END
#endif  // _UNORDERED_MAP_

#pragma endregion

#pragma region FUNCTIONS

METAENGINE_GUI_DEFINE_INLINE(template <>, std::add_pointer_t<void()>, if (ImGui::Button(name.c_str())) var();)
METAENGINE_GUI_DEFINE_INLINE(template <>, const std::add_pointer_t<void()>, if (ImGui::Button(name.c_str())) var();)

#pragma endregion

#pragma endregion ImGuiAuto

#pragma region ImString

class ImString {

public:
    ImString();

    ImString(size_t len);

    ImString(char *string);

    explicit ImString(const char *string);

    ImString(const ImString &other);

    ~ImString();

    char &operator[](size_t pos);

    operator char *();

    bool operator==(const char *string);

    bool operator!=(const char *string);

    bool operator==(ImString &string);

    bool operator!=(const ImString &string);

    ImString &operator=(const char *string);

    ImString &operator=(const ImString &other);

    inline size_t size() const { return mData ? strlen(mData) + 1 : 0; }

    void reserve(size_t len);

    char *get();

    const char *c_str() const;

    bool empty() const;

    int refcount() const;

    void ref();

    void unref();

private:
    char *mData;
    int *mRefCount;
};

#pragma endregion ImString

struct ImGuiMarkdown {
    ImGuiMarkdown();
    virtual ~ImGuiMarkdown(){};

    int print(const std::string &text);

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
    virtual bool render_html(const char *str, const char *str_end);

    // called when '\n' in source text where it is not semantically meaningful
    virtual void soft_break();
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
    unsigned m_hlevel = 0;  // 0 - no heading

private:
    int text(MD_TEXTTYPE type, const char *str, const char *str_end);
    int block(MD_BLOCKTYPE type, void *d, bool e);
    int span(MD_SPANTYPE type, void *d, bool e);

    void render_text(const char *str, const char *str_end);

    void set_font(bool e);
    void set_color(bool e);
    void set_href(bool e, const MD_ATTRIBUTE &src);

    // table state
    ImVec2 m_table_last_pos;
    int m_table_next_column = 0;
    ImVector<float> m_table_col_pos;
    ImVector<float> m_table_row_pos;

    // list state
    struct list_info {
        bool is_ol;
        char delim;
        unsigned cur_ol;
    };
    ImVector<list_info> m_list_stack;

    static void line(ImColor c, bool under);

    MD_PARSER m_md;
};

METAENGINE_GUI_DEFINE_BEGIN(template <>, MarkdownData)
ImGuiMarkdown markdown;
markdown.print(var.data);
METAENGINE_GUI_DEFINE_END

namespace ImGuiHelper {

enum class Alignment : unsigned char {
    kHorizontalCenter = 1 << 0,
    kVerticalCenter = 1 << 1,
    kCenter = kHorizontalCenter | kVerticalCenter,
};

/**
 * @brief Render text with alignment
 */
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

inline void SwitchButton(std::string &&icon, std::string &&label, bool &checked) {
    float height = ImGui::GetFrameHeight();
    float width = height * 1.55F;
    float radius = height * 0.50F;
    const auto frame_width = ImGui::GetContentRegionAvail().x;

    AlignedText(icon + "    " + label, Alignment::kVerticalCenter);
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

struct test_markdown : public ImGuiMarkdown {
    void open_url() const override { system(std::string("start msedge " + m_href).c_str()); }

    bool get_image(image_info &nfo) const override {
        // use m_href to identify images
        nfo.size = {40, 20};
        nfo.uv0 = {0, 0};
        nfo.uv1 = {1, 1};
        nfo.col_tint = {1, 1, 1, 1};
        nfo.col_border = {0, 0, 0, 0};
        return true;
    }
};

/**
   Dear ImGui with IME on-the-spot translation routines.
   author: TOGURO Mikito , mit@shalab.net
*/

#if !defined(imgex_hpp_HEADER_GUARD_7556619d_62b7_4f3b_b364_f02af36a3bbc)
#define imgex_hpp_HEADER_GUARD_7556619d_62b7_4f3b_b364_f02af36a3bbc 1

#if defined(_WIN32)
#include <Windows.h>
#include <tchar.h>
#endif /* defined( _WIN32 ) */

#include "libs/imgui/imgui.h"

#if defined(__cplusplus)
#include <cassert>
#else /* defined( __cplusplus ) */
#include <assert.h>
#endif /* defined( __cplusplus ) */

#if !defined(VERIFY_ASSERT)
#if defined(IM_ASSERT)
#define VERIFY_ASSERT(exp) IM_ASSERT(exp)
#else /* defined(IM_ASSERT) */
#define VERIFY_ASSERT(exp) assert(exp)
#endif /* defined(IM_ASSERT) */
#endif /* !defined( VERIFY_ASSERT ) */

#if !defined(VERIFY)
#if defined(NDEBUG)
#define VERIFY(exp)  \
    do {             \
        (void)(exp); \
    } while (0)
#else /* defined( NDEBUG ) */
#define VERIFY(exp) VERIFY_ASSERT(exp)
#endif /* defined( NDEBUG ) */
#endif /* !defined( VERIFY ) */

#if defined(__cplusplus)

namespace imgex {
// composition of flags
namespace implements {
template <typename first_t>
constexpr inline unsigned int composite_flags_0(first_t first) {
    return static_cast<unsigned int>(first);
}
template <typename first_t, typename... tail_t>
constexpr inline unsigned int composite_flags_0(first_t first, tail_t... tail) {
    return static_cast<unsigned int>(first) | composite_flags_0(tail...);
}
}  // namespace implements

template <typename require_t, typename... tail_t>
constexpr inline require_t composite_flags(tail_t... tail) {
    return static_cast<require_t>(implements::composite_flags_0(tail...));
}
}  // namespace imgex

#endif
#endif

#ifndef _IMGUI_IMM32_
#define _IMGUI_IMM32_

#if defined(_WIN32)
#include <Windows.h>
#include <commctrl.h>
#include <tchar.h>

#if !defined(WM_IMGUI_IMM32_COMMAND_BEGIN)
#define WM_IMGUI_IMM32_COMMAND_BEGIN (WM_APP + 0x200)
#endif /* !defined( WM_IMGUI_IMM32_COMMAND_BEGIN ) */

#endif /* defined( _WIN32 ) */

#if defined(_WIN32)

struct ImGUIIMMCommunication {

    enum { WM_IMGUI_IMM32_COMMAND = WM_IMGUI_IMM32_COMMAND_BEGIN, WM_IMGUI_IMM32_END };

    enum { WM_IMGUI_IMM32_COMMAND_NOP = 0u, WM_IMGUI_IMM32_COMMAND_SUBCLASSIFY, WM_IMGUI_IMM32_COMMAND_COMPOSITION_COMPLETE, WM_IMGUI_IMM32_COMMAND_CLEANUP };

    struct IMMCandidateList {
        std::vector<std::string> list_utf8;
        std::vector<std::string>::size_type selection;

        IMMCandidateList() : list_utf8{}, selection(0) {}
        IMMCandidateList(const IMMCandidateList &rhv) = default;
        IMMCandidateList(IMMCandidateList &&rhv) noexcept : list_utf8(), selection(0) { *this = std::move(rhv); }

        ~IMMCandidateList() = default;

        inline IMMCandidateList &operator=(const IMMCandidateList &rhv) = default;

        inline IMMCandidateList &operator=(IMMCandidateList &&rhv) noexcept {
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
#
        static IMMCandidateList cocreate(const CANDIDATELIST *const src, const size_t src_size);
    };

    static constexpr int candidate_window_num = 9;

    bool is_open;
    std::unique_ptr<char[]> comp_conved_utf8;
    std::unique_ptr<char[]> comp_target_utf8;
    std::unique_ptr<char[]> comp_unconv_utf8;
    bool show_ime_candidate_list;
    int request_candidate_list_str_commit;  // 1の時に candidate list が更新された後に、次の変換候補へ移る要請をする
    IMMCandidateList candidate_list;

    ImGUIIMMCommunication()
        : is_open(false), comp_conved_utf8(nullptr), comp_target_utf8(nullptr), comp_unconv_utf8(nullptr), show_ime_candidate_list(false), request_candidate_list_str_commit(false), candidate_list() {}

    ~ImGUIIMMCommunication() = default;
    void operator()();

private:
    bool update_candidate_window(HWND hWnd);

    static LRESULT WINAPI imm_communication_subClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT imm_communication_subClassProc_implement(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, ImGUIIMMCommunication &comm);
    BOOL subclassify_impl(HWND hWnd);

public:
    template <typename type_t>
    inline BOOL subclassify(type_t hWnd);

    template <>
    inline BOOL subclassify<HWND>(HWND hWnd) {
        return subclassify_impl(hWnd);
    }
};

#endif

// #if defined( _WIN32 )
//
// #define GLFW_EXPOSE_NATIVE_WIN32
// #include <GLFW/glfw3.h>
// #include <GLFW/glfw3native.h>
//
// template<>
// inline BOOL
// ImGUIIMMCommunication::subclassify<GLFWwindow*>(GLFWwindow* window)
//{
//     return this->subclassify(glfwGetWin32Window(window));
// }
// #endif

#if defined(__cplusplus)

#if defined(_WIN32)

#include "sdl_wrapper.h"

template <>
inline BOOL ImGUIIMMCommunication::subclassify<SDL_Window *>(SDL_Window *window) {
    SDL_SysWMinfo info{};
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info)) {
        IM_ASSERT(IsWindow(info.info.win.window));
        return this->subclassify(info.info.win.window);
    }
    return FALSE;
}

#endif
#endif
#endif

namespace ImGuiWidget {
void PlotFlame(const char *label, void (*values_getter)(float *start, float *end, ImU8 *level, const char **caption, const void *data, int idx), const void *data, int values_count,
               int values_offset = 0, const char *overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0));

using ImGuiFileBrowserFlags = int;

enum ImGuiFileBrowserFlags_ {
    ImGuiFileBrowserFlags_SelectDirectory = 1 << 0,    // select directory instead of regular file
    ImGuiFileBrowserFlags_EnterNewFilename = 1 << 1,   // allow user to enter new filename when selecting regular file
    ImGuiFileBrowserFlags_NoModal = 1 << 2,            // file browsing window is modal by default. specify this to use a popup window
    ImGuiFileBrowserFlags_NoTitleBar = 1 << 3,         // hide window title bar
    ImGuiFileBrowserFlags_NoStatusBar = 1 << 4,        // hide status bar at the bottom of browsing window
    ImGuiFileBrowserFlags_CloseOnEsc = 1 << 5,         // close file browser when pressing 'ESC'
    ImGuiFileBrowserFlags_CreateNewDir = 1 << 6,       // allow user to create new directory
    ImGuiFileBrowserFlags_MultipleSelection = 1 << 7,  // allow user to select multiple files. this will hide ImGuiFileBrowserFlags_EnterNewFilename
};

class FileBrowser {
public:
    // pwd is set to current working directory by default
    explicit FileBrowser(ImGuiFileBrowserFlags flags = 0);

    FileBrowser(const FileBrowser &copyFrom);

    FileBrowser &operator=(const FileBrowser &copyFrom);

    // set the window position (in pixels)
    // default is centered
    void SetWindowPos(int posx, int posy) noexcept;

    // set the window size (in pixels)
    // default is (700, 450)
    void SetWindowSize(int width, int height) noexcept;

    // set the window title text
    void SetTitle(std::string title);

    // open the browsing window
    void Open();

    // close the browsing window
    void Close();

    // the browsing window is opened or not
    bool IsOpened() const noexcept;

    // display the browsing window if opened
    void Display();

    // returns true when there is a selected filename and the "ok" button was clicked
    bool HasSelected() const noexcept;

    // set current browsing directory
    bool SetPwd(const std::filesystem::path &pwd = std::filesystem::current_path());

    // get current browsing directory
    const std::filesystem::path &GetPwd() const noexcept;

    // returns selected filename. make sense only when HasSelected returns true
    // when ImGuiFileBrowserFlags_MultipleSelection is enabled, only one of
    // selected filename will be returned
    std::filesystem::path GetSelected() const;

    // returns all selected filenames.
    // when ImGuiFileBrowserFlags_MultipleSelection is enabled, use this
    // instead of GetSelected
    std::vector<std::filesystem::path> GetMultiSelected() const;

    // set selected filename to empty
    void ClearSelected();

    // (optional) set file type filters. eg. { ".h", ".cpp", ".hpp" }
    // ".*" matches any file types
    void SetTypeFilters(const std::vector<std::string> &typeFilters);

    // set currently applied type filter
    // default value is 0 (the first type filter)
    void SetCurrentTypeFilterIndex(int index);

    // when ImGuiFileBrowserFlags_EnterNewFilename is set
    // this function will pre-fill the input dialog with a filename.
    void SetInputName(std::string_view input);

private:
    template <class Functor>
    struct ScopeGuard {
        ScopeGuard(Functor &&t) : func(std::move(t)) {}

        ~ScopeGuard() { func(); }

    private:
        Functor func;
    };

    struct FileRecord {
        bool isDir = false;
        std::filesystem::path name;
        std::string showName;
        std::filesystem::path extension;
    };

    static std::string ToLower(const std::string &s);

    void UpdateFileRecords();

    void SetPwdUncatched(const std::filesystem::path &pwd);

    bool IsExtensionMatched(const std::filesystem::path &extension) const;

#ifdef _WIN32
    static std::uint32_t GetDrivesBitMask();
#endif

    // for c++17 compatibility

#if defined(__cpp_lib_char8_t)
    static std::string u8StrToStr(std::u8string s);
#endif
    static std::string u8StrToStr(std::string s);

    int width_;
    int height_;
    int posX_;
    int posY_;
    ImGuiFileBrowserFlags flags_;

    std::string title_;
    std::string openLabel_;

    bool openFlag_;
    bool closeFlag_;
    bool isOpened_;
    bool ok_;
    bool posIsSet_;

    std::string statusStr_;

    std::vector<std::string> typeFilters_;
    unsigned int typeFilterIndex_;
    bool hasAllFilter_;

    std::filesystem::path pwd_;
    std::set<std::filesystem::path> selectedFilenames_;

    std::vector<FileRecord> fileRecords_;

    // IMPROVE: truncate when selectedFilename_.length() > inputNameBuf_.size() - 1
    static constexpr size_t INPUT_NAME_BUF_SIZE = 512;
    std::unique_ptr<std::array<char, INPUT_NAME_BUF_SIZE>> inputNameBuf_;

    std::string openNewDirLabel_;
    std::unique_ptr<std::array<char, INPUT_NAME_BUF_SIZE>> newDirNameBuf_;

#ifdef _WIN32
    uint32_t drives_;
#endif
};
}  // namespace ImGuiWidget

inline ImGuiWidget::FileBrowser::FileBrowser(ImGuiFileBrowserFlags flags)
    : width_(700),
      height_(450),
      posX_(0),
      posY_(0),
      flags_(flags),
      openFlag_(false),
      closeFlag_(false),
      isOpened_(false),
      ok_(false),
      posIsSet_(false),
      inputNameBuf_(std::make_unique<std::array<char, INPUT_NAME_BUF_SIZE>>()) {
    if (flags_ & ImGuiFileBrowserFlags_CreateNewDir) {
        newDirNameBuf_ = std::make_unique<std::array<char, INPUT_NAME_BUF_SIZE>>();
    }

    inputNameBuf_->front() = '\0';
    inputNameBuf_->back() = '\0';
    SetTitle("file browser");
    SetPwd(std::filesystem::current_path());

    typeFilters_.clear();
    typeFilterIndex_ = 0;
    hasAllFilter_ = false;

#ifdef _WIN32
    drives_ = GetDrivesBitMask();
#endif
}

inline ImGuiWidget::FileBrowser::FileBrowser(const FileBrowser &copyFrom) : FileBrowser() { *this = copyFrom; }

inline ImGuiWidget::FileBrowser &ImGuiWidget::FileBrowser::operator=(const FileBrowser &copyFrom) {
    width_ = copyFrom.width_;
    height_ = copyFrom.height_;

    posX_ = copyFrom.posX_;
    posY_ = copyFrom.posY_;

    flags_ = copyFrom.flags_;
    SetTitle(copyFrom.title_);

    openFlag_ = copyFrom.openFlag_;
    closeFlag_ = copyFrom.closeFlag_;
    isOpened_ = copyFrom.isOpened_;
    ok_ = copyFrom.ok_;
    posIsSet_ = copyFrom.posIsSet_;

    statusStr_ = "";

    typeFilters_ = copyFrom.typeFilters_;
    typeFilterIndex_ = copyFrom.typeFilterIndex_;
    hasAllFilter_ = copyFrom.hasAllFilter_;

    pwd_ = copyFrom.pwd_;
    selectedFilenames_ = copyFrom.selectedFilenames_;

    fileRecords_ = copyFrom.fileRecords_;

    *inputNameBuf_ = *copyFrom.inputNameBuf_;

    openNewDirLabel_ = copyFrom.openNewDirLabel_;
    if (flags_ & ImGuiFileBrowserFlags_CreateNewDir) {
        newDirNameBuf_ = std::make_unique<std::array<char, INPUT_NAME_BUF_SIZE>>();
        *newDirNameBuf_ = *copyFrom.newDirNameBuf_;
    }

#ifdef _WIN32
    drives_ = copyFrom.drives_;
#endif

    return *this;
}

inline void ImGuiWidget::FileBrowser::SetWindowPos(int posx, int posy) noexcept {
    posX_ = posx;
    posY_ = posy;
    posIsSet_ = true;
}

inline void ImGuiWidget::FileBrowser::SetWindowSize(int width, int height) noexcept {
    assert(width > 0 && height > 0);
    width_ = width;
    height_ = height;
}

inline void ImGuiWidget::FileBrowser::SetTitle(std::string title) {
    title_ = std::move(title);
    openLabel_ = title_ + "##filebrowser_" + std::to_string(reinterpret_cast<size_t>(this));
    openNewDirLabel_ = "new dir##new_dir_" + std::to_string(reinterpret_cast<size_t>(this));
}

inline void ImGuiWidget::FileBrowser::Open() {
    ClearSelected();
    UpdateFileRecords();
    statusStr_ = std::string();
    openFlag_ = true;
    closeFlag_ = false;
}

inline void ImGuiWidget::FileBrowser::Close() {
    ClearSelected();
    statusStr_ = std::string();
    closeFlag_ = true;
    openFlag_ = false;
}

inline bool ImGuiWidget::FileBrowser::IsOpened() const noexcept { return isOpened_; }

inline void ImGuiWidget::FileBrowser::Display() {
    ImGui::PushID(this);
    ScopeGuard exitThis([this] {
        openFlag_ = false;
        closeFlag_ = false;
        ImGui::PopID();
    });

    if (openFlag_) {
        ImGui::OpenPopup(openLabel_.c_str());
    }
    isOpened_ = false;

    // open the popup window

    if (openFlag_ && (flags_ & ImGuiFileBrowserFlags_NoModal)) {
        if (posIsSet_) ImGui::SetNextWindowPos(ImVec2(static_cast<float>(posX_), static_cast<float>(posY_)));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width_), static_cast<float>(height_)));
    } else {
        if (posIsSet_) ImGui::SetNextWindowPos(ImVec2(static_cast<float>(posX_), static_cast<float>(posY_)), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width_), static_cast<float>(height_)), ImGuiCond_FirstUseEver);
    }
    if (flags_ & ImGuiFileBrowserFlags_NoModal) {
        if (!ImGui::BeginPopup(openLabel_.c_str())) {
            return;
        }
    } else if (!ImGui::BeginPopupModal(openLabel_.c_str(), nullptr, flags_ & ImGuiFileBrowserFlags_NoTitleBar ? ImGuiWindowFlags_NoTitleBar : 0)) {
        return;
    }

    isOpened_ = true;
    ScopeGuard endPopup([] { ImGui::EndPopup(); });

    // display elements in pwd

#ifdef _WIN32
    char currentDrive = static_cast<char>(pwd_.c_str()[0]);
    char driveStr[] = {currentDrive, ':', '\0'};

    PushItemWidth(4 * GetFontSize());
    if (BeginCombo("##select_drive", driveStr)) {
        ScopeGuard guard([&] { EndCombo(); });

        for (int i = 0; i < 26; ++i) {
            if (!(drives_ & (1 << i))) {
                continue;
            }

            char driveCh = static_cast<char>('A' + i);
            char selectableStr[] = {driveCh, ':', '\0'};
            bool selected = currentDrive == driveCh;

            if (Selectable(selectableStr, selected) && !selected) {
                char newPwd[] = {driveCh, ':', '\\', '\0'};
                SetPwd(newPwd);
            }
        }
    }
    PopItemWidth();

    SameLine();
#endif

    int secIdx = 0, newPwdLastSecIdx = -1;
    for (const auto &sec : pwd_) {
#ifdef _WIN32
        if (secIdx == 1) {
            ++secIdx;
            continue;
        }
#endif

        ImGui::PushID(secIdx);
        if (secIdx > 0) {
            ImGui::SameLine();
        }
        if (ImGui::SmallButton(u8StrToStr(sec.u8string()).c_str())) {
            newPwdLastSecIdx = secIdx;
        }
        ImGui::PopID();

        ++secIdx;
    }

    if (newPwdLastSecIdx >= 0) {
        int i = 0;
        std::filesystem::path newPwd;
        for (const auto &sec : pwd_) {
            if (i++ > newPwdLastSecIdx) {
                break;
            }
            newPwd /= sec;
        }

#ifdef _WIN32
        if (newPwdLastSecIdx == 0) {
            newPwd /= "\\";
        }
#endif

        SetPwd(newPwd);
    }

    ImGui::SameLine();

    if (ImGui::SmallButton("*")) {
        UpdateFileRecords();

        std::set<std::filesystem::path> newSelectedFilenames;
        for (auto &name : selectedFilenames_) {
            auto it = std::find_if(fileRecords_.begin(), fileRecords_.end(), [&](const FileRecord &record) { return name == record.name; });

            if (it != fileRecords_.end()) {
                newSelectedFilenames.insert(name);
            }
        }

        if (inputNameBuf_ && (*inputNameBuf_)[0]) {
            newSelectedFilenames.insert(inputNameBuf_->data());
        }
    }

    if (newDirNameBuf_) {
        ImGui::SameLine();
        if (ImGui::SmallButton("+")) {
            ImGui::OpenPopup(openNewDirLabel_.c_str());
            (*newDirNameBuf_)[0] = '\0';
        }

        if (ImGui::BeginPopup(openNewDirLabel_.c_str())) {
            ScopeGuard endNewDirPopup([] { ImGui::EndPopup(); });

            ImGui::InputText("name", newDirNameBuf_->data(), newDirNameBuf_->size());
            ImGui::SameLine();

            if (ImGui::Button("ok") && (*newDirNameBuf_)[0] != '\0') {
                ScopeGuard closeNewDirPopup([] { ImGui::CloseCurrentPopup(); });
                if (create_directory(pwd_ / newDirNameBuf_->data())) {
                    UpdateFileRecords();
                } else {
                    statusStr_ = "failed to create " + std::string(newDirNameBuf_->data());
                }
            }
        }
    }

    // browse files in a child window

    float reserveHeight = ImGui::GetFrameHeightWithSpacing();
    std::filesystem::path newPwd;
    bool setNewPwd = false;
    if (!(flags_ & ImGuiFileBrowserFlags_SelectDirectory) && (flags_ & ImGuiFileBrowserFlags_EnterNewFilename)) reserveHeight += ImGui::GetFrameHeightWithSpacing();
    {
        ImGui::BeginChild("ch", ImVec2(0, -reserveHeight), true, (flags_ & ImGuiFileBrowserFlags_NoModal) ? ImGuiWindowFlags_AlwaysHorizontalScrollbar : 0);
        ScopeGuard endChild([] { ImGui::EndChild(); });

        for (auto &rsc : fileRecords_) {
            if (!rsc.isDir && !IsExtensionMatched(rsc.extension)) {
                continue;
            }

            if (!rsc.name.empty() && rsc.name.c_str()[0] == '$') {
                continue;
            }

            bool selected = selectedFilenames_.find(rsc.name) != selectedFilenames_.end();

            if (ImGui::Selectable(rsc.showName.c_str(), selected, ImGuiSelectableFlags_DontClosePopups)) {
                const bool multiSelect =
                        (flags_ & ImGuiFileBrowserFlags_MultipleSelection) && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && (ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift);

                if (selected) {
                    if (!multiSelect) {
                        selectedFilenames_.clear();
                    } else {
                        selectedFilenames_.erase(rsc.name);
                    }

                    (*inputNameBuf_)[0] = '\0';
                } else if (rsc.name != "..") {
                    if ((rsc.isDir && (flags_ & ImGuiFileBrowserFlags_SelectDirectory)) || (!rsc.isDir && !(flags_ & ImGuiFileBrowserFlags_SelectDirectory))) {
                        if (multiSelect) {
                            selectedFilenames_.insert(rsc.name);
                        } else {
                            selectedFilenames_ = {rsc.name};
                        }

                        if (!(flags_ & ImGuiFileBrowserFlags_SelectDirectory)) {
#ifdef _MSC_VER
                            strcpy_s(inputNameBuf_->data(), inputNameBuf_->size(), u8StrToStr(rsc.name.u8string()).c_str());
#else
                            std::strncpy(inputNameBuf_->data(), u8StrToStr(rsc.name.u8string()).c_str(), inputNameBuf_->size() - 1);
#endif
                        }
                    }
                } else {
                    if (!multiSelect) {
                        selectedFilenames_.clear();
                    }
                }
            }

            if (ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0)) {
                if (rsc.isDir) {
                    setNewPwd = true;
                    newPwd = (rsc.name != "..") ? (pwd_ / rsc.name) : pwd_.parent_path();
                } else if (!(flags_ & ImGuiFileBrowserFlags_SelectDirectory)) {
                    selectedFilenames_ = {rsc.name};
                    ok_ = true;
                    ImGui::CloseCurrentPopup();
                }
            }
        }
    }

    if (setNewPwd) {
        SetPwd(newPwd);
    }

    if (!(flags_ & ImGuiFileBrowserFlags_SelectDirectory) && (flags_ & ImGuiFileBrowserFlags_EnterNewFilename)) {
        ImGui::PushID(this);
        ScopeGuard popTextID([] { ImGui::PopID(); });

        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("", inputNameBuf_->data(), inputNameBuf_->size()) && inputNameBuf_->at(0) != '\0') {
            selectedFilenames_ = {inputNameBuf_->data()};
        }
        ImGui::PopItemWidth();
    }

    if (!(flags_ & ImGuiFileBrowserFlags_SelectDirectory)) {
        if (ImGui::Button(" ok ") && !selectedFilenames_.empty()) {
            ok_ = true;
            ImGui::CloseCurrentPopup();
        }
    } else {
        if (ImGui::Button(" ok ")) {
            ok_ = true;
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();

    bool shouldExit = ImGui::Button("cancel") || closeFlag_ ||
                      ((flags_ & ImGuiFileBrowserFlags_CloseOnEsc) && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape));
    if (shouldExit) {
        ImGui::CloseCurrentPopup();
    }

    if (!statusStr_.empty() && !(flags_ & ImGuiFileBrowserFlags_NoStatusBar)) {
        ImGui::SameLine();
        ImGui::Text("%s", statusStr_.c_str());
    }

    if (!typeFilters_.empty()) {
        ImGui::SameLine();
        ImGui::PushItemWidth(8 * ImGui::GetFontSize());
        if (ImGui::BeginCombo("##type_filters", typeFilters_[typeFilterIndex_].c_str())) {
            ScopeGuard guard([&] { ImGui::EndCombo(); });

            for (size_t i = 0; i < typeFilters_.size(); ++i) {
                bool selected = i == typeFilterIndex_;
                if (ImGui::Selectable(typeFilters_[i].c_str(), selected) && !selected) {
                    typeFilterIndex_ = static_cast<unsigned int>(i);
                }
            }
        }
        ImGui::PopItemWidth();
    }
}

inline bool ImGuiWidget::FileBrowser::HasSelected() const noexcept { return ok_; }

inline bool ImGuiWidget::FileBrowser::SetPwd(const std::filesystem::path &pwd) {
    try {
        SetPwdUncatched(pwd);
        return true;
    } catch (const std::exception &err) {
        statusStr_ = std::string("last error: ") + err.what();
    } catch (...) {
        statusStr_ = "last error: unknown";
    }

    SetPwdUncatched(std::filesystem::current_path());
    return false;
}

inline const class std::filesystem::path &ImGuiWidget::FileBrowser::GetPwd() const noexcept { return pwd_; }

inline std::filesystem::path ImGuiWidget::FileBrowser::GetSelected() const {
    // when ok_ is true, selectedFilenames_ may be empty if SelectDirectory
    // is enabled. return pwd in that case.
    if (selectedFilenames_.empty()) {
        return pwd_;
    }
    return pwd_ / *selectedFilenames_.begin();
}

inline std::vector<std::filesystem::path> ImGuiWidget::FileBrowser::GetMultiSelected() const {
    if (selectedFilenames_.empty()) {
        return {pwd_};
    }

    std::vector<std::filesystem::path> ret;
    ret.reserve(selectedFilenames_.size());
    for (auto &s : selectedFilenames_) {
        ret.push_back(pwd_ / s);
    }

    return ret;
}

inline void ImGuiWidget::FileBrowser::ClearSelected() {
    selectedFilenames_.clear();
    (*inputNameBuf_)[0] = '\0';
    ok_ = false;
}

inline void ImGuiWidget::FileBrowser::SetTypeFilters(const std::vector<std::string> &_typeFilters) {
    typeFilters_.clear();

    // remove duplicate filter names due to case unsensitivity on windows

#ifdef _WIN32

    std::vector<std::string> typeFilters;
    for (auto &rawFilter : _typeFilters) {
        std::string lowerFilter = ToLower(rawFilter);
        auto it = std::find(typeFilters.begin(), typeFilters.end(), lowerFilter);
        if (it == typeFilters.end()) {
            typeFilters.push_back(std::move(lowerFilter));
        }
    }

#else

    auto &typeFilters = _typeFilters;

#endif

    // insert auto-generated filter

    if (typeFilters.size() > 1) {
        hasAllFilter_ = true;
        std::string allFiltersName = std::string();
        for (size_t i = 0; i < typeFilters.size(); ++i) {
            if (typeFilters[i] == std::string_view(".*")) {
                hasAllFilter_ = false;
                break;
            }

            if (i > 0) {
                allFiltersName += ",";
            }
            allFiltersName += typeFilters[i];
        }

        if (hasAllFilter_) {
            typeFilters_.push_back(std::move(allFiltersName));
        }
    }

    std::copy(typeFilters.begin(), typeFilters.end(), std::back_inserter(typeFilters_));

    typeFilterIndex_ = 0;
}

inline void ImGuiWidget::FileBrowser::SetCurrentTypeFilterIndex(int index) { typeFilterIndex_ = static_cast<unsigned int>(index); }

inline void ImGuiWidget::FileBrowser::SetInputName(std::string_view input) {
    if (flags_ & ImGuiFileBrowserFlags_EnterNewFilename) {
        if (input.size() >= static_cast<size_t>(INPUT_NAME_BUF_SIZE)) {
            // If input doesn't fit trim off characters
            input = input.substr(0, INPUT_NAME_BUF_SIZE - 1);
        }
        std::copy(input.begin(), input.end(), inputNameBuf_->begin());
        inputNameBuf_->at(input.size()) = '\0';
        selectedFilenames_ = {inputNameBuf_->data()};
    }
}

inline std::string ImGuiWidget::FileBrowser::ToLower(const std::string &s) {
    std::string ret = s;
    for (char &c : ret) {
        c = static_cast<char>(std::tolower(c));
    }
    return ret;
}

inline void ImGuiWidget::FileBrowser::UpdateFileRecords() {
    fileRecords_ = {FileRecord{true, "..", "[D] ..", ""}};

    for (auto &p : std::filesystem::directory_iterator(pwd_)) {
        FileRecord rcd;

        if (p.is_regular_file()) {
            rcd.isDir = false;
        } else if (p.is_directory()) {
            rcd.isDir = true;
        } else {
            continue;
        }

        rcd.name = p.path().filename();
        if (rcd.name.empty()) {
            continue;
        }

        rcd.extension = p.path().filename().extension();

        rcd.showName = (rcd.isDir ? "[D] " : "[F] ") + u8StrToStr(p.path().filename().u8string());
        fileRecords_.push_back(rcd);
    }

    std::sort(fileRecords_.begin(), fileRecords_.end(), [](const FileRecord &L, const FileRecord &R) { return (L.isDir ^ R.isDir) ? L.isDir : (L.name < R.name); });
}

inline void ImGuiWidget::FileBrowser::SetPwdUncatched(const std::filesystem::path &pwd) {
    pwd_ = absolute(pwd);
    UpdateFileRecords();
    selectedFilenames_.clear();
    (*inputNameBuf_)[0] = '\0';
}

inline bool ImGuiWidget::FileBrowser::IsExtensionMatched(const std::filesystem::path &_extension) const {
#ifdef _WIN32
    std::filesystem::path extension = ToLower(_extension.string());
#else
    auto &extension = _extension;
#endif

    // no type filters
    if (typeFilters_.empty()) {
        return true;
    }

    // invalid type filter index
    if (static_cast<size_t>(typeFilterIndex_) >= typeFilters_.size()) {
        return true;
    }

    // all type filters
    if (hasAllFilter_ && typeFilterIndex_ == 0) {
        for (size_t i = 1; i < typeFilters_.size(); ++i) {
            if (extension == typeFilters_[i]) {
                return true;
            }
        }
        return false;
    }

    // universal filter
    if (typeFilters_[typeFilterIndex_] == std::string_view(".*")) {
        return true;
    }

    // regular filter
    return extension == typeFilters_[typeFilterIndex_];
}

#if defined(__cpp_lib_char8_t)
inline std::string ImGuiWidget::FileBrowser::u8StrToStr(std::u8string s) { return std::string(s.begin(), s.end()); }
#endif

inline std::string ImGuiWidget::FileBrowser::u8StrToStr(std::string s) { return s; }

#ifdef _WIN32

#ifndef _INC_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN

#define IMGUI_FILEBROWSER_UNDEF_WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN

#endif  // #ifndef WIN32_LEAN_AND_MEAN

#include <windows.h>

#ifdef IMGUI_FILEBROWSER_UNDEF_WIN32_LEAN_AND_MEAN
#undef IMGUI_FILEBROWSER_UNDEF_WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif  // #ifdef IMGUI_FILEBROWSER_UNDEF_WIN32_LEAN_AND_MEAN

#endif  // #ifdef _INC_WINDOWS

inline std::uint32_t ImGuiWidget::FileBrowser::GetDrivesBitMask() {
    DWORD mask = GetLogicalDrives();
    uint32_t ret = 0;
    for (int i = 0; i < 26; ++i) {
        if (!(mask & (1 << i))) {
            continue;
        }
        char rootName[4] = {static_cast<char>('A' + i), ':', '\\', '\0'};
        UINT type = GetDriveTypeA(rootName);
        if (type == DRIVE_REMOVABLE || type == DRIVE_FIXED || type == DRIVE_REMOTE) {
            ret |= (1 << i);
        }
    }
    return ret;
}

#endif

#endif
