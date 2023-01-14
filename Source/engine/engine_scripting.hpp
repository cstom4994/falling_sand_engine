// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_SCRIPTING_HPP_
#define _METADOT_SCRIPTING_HPP_

#include <functional>
#include <map>
#include <string>

#include "core/macros.h"
#include "engine.h"
#include "engine/scripting/lua_wrapper.hpp"
#include "libs/visitstruct.hpp"

struct lua_State;

#pragma region struct_as

template <typename T>
METADOT_INLINE void struct_as(std::string &s, const char *table, const char *key, const T &value) {
    s += MetaEngine::Format("{0}.{1} = {2}\n", table, key, value);
}

template <>
METADOT_INLINE void struct_as(std::string &s, const char *table, const char *key, const std::string &value) {
    s += MetaEngine::Format("{0}.{1} = \"{2}\"\n", table, key, value);
}

#pragma endregion struct_as

using ppair = std::pair<const char *, const void *>;

struct test_visitor {
    std::vector<ppair> result;

    template <typename T>
    void operator()(const char *name, const T &t) {
        result.emplace_back(ppair{name, static_cast<const void *>(&t)});
    }
};

template <typename T>
void SaveLuaConfig(const T &_struct, const char *table_name, std::string &out) {
    visit_struct::for_each(_struct, [&](const char *name, const auto &value) {
        // METADOT_INFO("{} == {} ({})", name, value, typeid(value).name());
        struct_as(out, table_name, name, value);
    });
}

#define LoadLuaConfig(_struct, _luat, _c) _struct->_c = _luat[#_c].get<decltype(_struct->_c)>()

// template<typename T>
// void LoadLuaConfig(const T &_struct, LuaWrapper::LuaTable *luat) {
//     int idx = 0;
//     test_visitor vis;
//     visit_struct::apply_visitor(vis, _struct);
//     visit_struct::for_each(_struct, [&](const char *name, const auto &value) {
//         // (*visit_struct::get_pointer<idx>()) =
//         //         (*luat)[name].get<decltype(visit_struct::get<idx>(_struct))>();
//         // (*vis1.result[idx].first) = (*luat)[name].get<>();
//     });
// }

struct LuaCoreCpp {
    LuaWrapper::State s_lua;
    LuaCoreC *C;

    struct {
        LuaWrapper::LuaTable Biome;
    } Data_;
};

void print_error(lua_State *state);
void RunScriptInConsole(const char *c);
void RunScriptFromFile(const char *filePath);

void UpdateLuaCoreCpp(LuaCoreCpp *_struct);
void InitLuaCoreCpp(LuaCoreCpp *_struct);
void EndLuaCoreCpp(LuaCoreCpp *_struct);

struct Scripts {
    LuaCoreCpp *LuaCoreCpp;

    void Init(Meta::AnyFunction &gamescriptwrap_init, Meta::AnyFunction &gamescriptwrap_bind);
    void End(Meta::AnyFunction &gamescriptwrap_end);
    void UpdateRender();
    void UpdateTick();
};

#endif