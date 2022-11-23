// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_LUALAYER_HPP_
#define _METADOT_LUALAYER_HPP_

#include "Libs/lua/sol/sol.hpp"

struct lua_State;

class LuaLayer {
private:
    sol::state s_lua;
    lua_State *m_L;
    void print_error(lua_State *state);

public:
    LuaLayer() { this->onAttach(); };
    ~LuaLayer() = default;

    lua_State *getLuaState() { return m_L; }
    sol::state *getSolState();
    void runScriptInConsole(lua_State *L, const char *c);

    void runScriptFromFile(lua_State *L, const std::string &filePath);
    void onUpdate();
    void onAttach();
    void onDetach();

    template<typename T>
    inline void set_variable(const std::string &name, T value) {
        this->s_lua[name] = value;
    }

    template<typename T>
    inline T get_variable(const std::string &name) {
        return this->s_lua.get<T>(name);
    }
};

#endif