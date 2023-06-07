// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_SCRIPTING_HPP
#define ME_SCRIPTING_HPP

#include <mono/metadata/object-forward.h>

#include <functional>
#include <map>
#include <string>

#include "engine/core/cpp/struct.hpp"
#include "engine/core/macros.hpp"
#include "engine/core/utils/utility.hpp"
#include "engine/engine.h"
#include "engine/scripting/lua_wrapper.hpp"

struct lua_State;

#pragma region struct_as

template <typename T>
ME_INLINE void struct_as(std::string &s, const char *table, const char *key, const T &value) {
    s += std::format("{0}.{1} = {2}\n", table, key, value);
}

template <>
ME_INLINE void struct_as(std::string &s, const char *table, const char *key, const std::string &value) {
    s += std::format("{0}.{1} = \"{2}\"\n", table, key, value);
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
    ME::Struct::for_each(_struct, [&](const char *name, const auto &value) {
        // METADOT_INFO("{} == {} ({})", name, value, typeid(value).name());
        struct_as(out, table_name, name, value);
    });
}

#define LoadLuaConfig(_struct, _luat, _c) _struct->_c = _luat[#_c].get<decltype(_struct->_c)>()

// template<typename T>
// void LoadLuaConfig(const T &_struct, LuaWrapper::LuaTable *luat) {
//     int idx = 0;
//     test_visitor vis;
//     ME::Struct::apply_visitor(vis, _struct);
//     ME::Struct::for_each(_struct, [&](const char *name, const auto &value) {
//         // (*ME::Struct::get_pointer<idx>()) =
//         //         (*luat)[name].get<decltype(ME::Struct::get<idx>(_struct))>();
//         // (*vis1.result[idx].first) = (*luat)[name].get<>();
//     });
// }

struct LuaCore {
    LuaWrapper::State s_lua;
    lua_State *L;

    struct {
        LuaWrapper::LuaTable Biome;
    } Data_;
};

class MonoLayer {
private:
    MonoObject *callEntryMethod(const char *methodName, void *obj = nullptr, void **params = nullptr, MonoObject **ex = nullptr);

public:
    bool hotSwapEnable = true;
    void onAttach();
    void onDetach();
    void onUpdate();

    void reloadAssembly();
    bool isMonoLoaded();
};

void print_error(lua_State *state, int result = 0);
void script_runfile(const char *filePath);

class Scripting : public ME::singleton<Scripting> {
public:
    LuaCore *Lua;
    MonoLayer Mono;

    Scripting() { METADOT_BUG("Scripting CSingleton Init"); };
    ~Scripting() { METADOT_BUG("Scripting CSingleton End"); };

    void Init();
    void End();
    void Update();
    void UpdateRender();
    void UpdateTick();

private:
    friend class ME::singleton<Scripting>;
};

#endif
