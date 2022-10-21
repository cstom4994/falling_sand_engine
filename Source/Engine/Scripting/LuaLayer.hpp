// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

#include "Game/ModuleStack.h"

#include "Libs/lua/sol/sol.hpp"

struct lua_State;

namespace MetaEngine {

    struct LuaConsole;

    class LuaLayer : public Module
    {
    private:
        LuaConsole* m_console;
        sol::state s_lua;
        lua_State* m_L;
        void print_error(lua_State* state);

    public:
        LuaLayer() : Module("LuaLayer") {};
        ~LuaLayer() override = default;

        lua_State* getLuaState() { return m_L; }
        sol::state* getSolState();
        void printToLuaConsole(lua_State* L, const char* c);
        void runScriptInConsole(lua_State* L, const char* c);

        void runScriptFromFile(lua_State* L, const std::string& filePath);
        void onUpdate() override;
        void onAttach() override;
        void onDetach() override;
        void onImGuiRender() override;

        template<typename T>
        inline void set_variable(const std::string& name, T value)
        {
            this->s_lua[name] = value;
        }

        template<typename T>
        inline T get_variable(const std::string& name)
        {
            return this->s_lua.get<T>(name);
        }
    };
}
