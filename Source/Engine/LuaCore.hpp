// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_LUAMACHINE_HPP_
#define _METADOT_LUAMACHINE_HPP_

#include "Engine/LuaWrapper.hpp"

struct lua_State;

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
    void Attach();
    void Detach();

    struct
    {
        // LuaWrapper::LuaFunction Lang;
    } Func_;

    struct
    {
        LuaWrapper::LuaTable Biome;
    } Data_;
};

#endif