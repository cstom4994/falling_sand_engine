// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_SCRIPTING_HPP_
#define _METADOT_SCRIPTING_HPP_

#include "JsWrapper.hpp"
#include <map>
#include <string>

#include "Engine/LuaWrapper.hpp"
#include "Libs/VisitStruct.hpp"

struct lua_State;

#pragma region struct_as

template<typename T>
METADOT_INLINE void struct_as(std::string &s, const char *table, const char *key, const T &value) {
    s += MetaEngine::Format("{0}.{1} = {2}\n", table, key, value);
}

template<>
METADOT_INLINE void struct_as(std::string &s, const char *table, const char *key,
                              const std::string &value) {
    s += MetaEngine::Format("{0}.{1} = \"{2:s}\"\n", table, key, value);
}

#pragma endregion struct_as

using ppair = std::pair<const char *, const void *>;

struct test_visitor
{
    std::vector<ppair> result;

    template<typename T>
    void operator()(const char *name, const T &t) {
        result.emplace_back(ppair{name, static_cast<const void *>(&t)});
    }
};

template<typename T>
void SaveLuaConfig(const T &_struct, std::string &out) {
    visit_struct::for_each(_struct, [&](const char *name, const auto &value) {
        //METADOT_INFO("{} == {} ({})", name, value, typeid(value).name());
        struct_as(out, "mytable", name, value);
    });
}

#define LoadLuaConfig(_struct, _luat, _c) _struct._c = _luat[#_c].get<decltype(_struct._c)>()

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

struct LuaCore
{
private:
    LuaWrapper::State s_lua;
    lua_State *m_L;
    void print_error(lua_State *state);

public:
    lua_State *getLuaState() { return m_L; }
    LuaWrapper::State *GetWrapper() { return &s_lua; }
    void RunScriptInConsole(lua_State *L, const char *c);

    void RunScriptFromFile(const std::string &filePath);
    void Update();
    void Init();
    void End();

    struct
    {
        // LuaWrapper::LuaFunction Lang;
    } Func_;

    struct
    {
        LuaWrapper::LuaTable Biome;
    } Data_;
};

namespace MuDSL {
    class MuDSLInterpreter;
}

struct Scripts
{
    MuDSL::MuDSLInterpreter *MuDSL = nullptr;
    LuaCore *LuaRuntime;

    JsWrapper::Runtime *JsRuntime = nullptr;
    JsWrapper::Context *JsContext = nullptr;

    void Init();
    void End();
    void UpdateRender();
    void UpdateTick();
    void LoadMuFuncs();
};

#endif