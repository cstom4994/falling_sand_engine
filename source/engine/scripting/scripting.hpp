// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_SCRIPTING_HPP
#define ME_SCRIPTING_HPP

#include <functional>
#include <map>
#include <string>

#include "engine/core/macros.hpp"
#include "engine/engine.hpp"
#include "engine/scripting/lua_wrapper.hpp"
#include "engine/utils/module.hpp"
#include "engine/utils/utility.hpp"

struct lua_State;

namespace ME {

template <typename T>
ME_INLINE void struct_as(std::string &s, const char *table, const char *key, const T &value) {
    s += std::format("{0}.{1} = {2}\n", table, key, value);
}

template <>
ME_INLINE void struct_as(std::string &s, const char *table, const char *key, const std::string &value) {
    s += std::format("{0}.{1} = \"{2}\"\n", table, key, value);
}

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
    ME::meta::dostruct::for_each(_struct, [&](const char *name, const auto &value) {
        // METADOT_INFO("{} == {} ({})", name, value, typeid(value).name());
        struct_as(out, table_name, name, value);
    });
}

#define LoadLuaConfig(_struct, _luat, _c) _struct->_c = _luat[#_c].get<decltype(_struct->_c)>()

// template<typename T>
// void LoadLuaConfig(const T &_struct, lua_wrapper::LuaTable *luat) {
//     int idx = 0;
//     test_visitor vis;
//     ME::meta::dostruct::apply_visitor(vis, _struct);
//     ME::meta::dostruct::for_each(_struct, [&](const char *name, const auto &value) {
//         // (*ME::meta::dostruct::get_pointer<idx>()) =
//         //         (*luat)[name].get<decltype(ME::meta::dostruct::get<idx>(_struct))>();
//         // (*vis1.result[idx].first) = (*luat)[name].get<>();
//     });
// }

void print_error(lua_State *state, int result = 0);
void script_runfile(const char *filePath);

class scripting : public module<scripting> {
public:
    lua_wrapper::State s_lua;
    lua_State *L = nullptr;

    struct {
        lua_wrapper::LuaTable Biome;
    } Data_;

    scripting() { METADOT_BUG("[Scripting] init"); };
    ~scripting() { METADOT_BUG("[Scripting] end"); };

    void init();
    void end();
    void update();
    void update_render();
    void update_tick();

    ME_INLINE auto fast_call_func(std::string name) {
        auto &luawrap = this->s_lua;
        auto func = luawrap[name];
        return func;
    }

    ME_INLINE bool fast_load_lua(std::string path) {
        auto &luawrap = this->s_lua;
        return luawrap.dofile(path);
    }
};

}  // namespace ME

#endif
