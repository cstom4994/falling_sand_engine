﻿// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Engine/InEngine.h"
#include "LuaBinder.hpp"
#include "Engine/FileSystem.hpp"

#include "Engine/lib/lua/sol/sol.hpp"

#include "Engine/lib/lua/lua.hpp"

#include "LuaBinder_ImGui.hpp"

namespace MetaEngine {

    static void bindBasic(sol::state& state);

    void func1(std::string a) { std::cout << __FUNCTION__ << " :: " << a << std::endl; }
    void func2(std::string a) { std::cout << __FUNCTION__ << " :: " << a << std::endl; }

    struct MyStruct {
        void (*func1)(std::string);
        void (*func2)(std::string);
    };

    template<class T>
    void As(T arg) {
        //auto namespac = state["Test"].get_or_create<sol::table>();
        //namespac.set_function();
    }

    auto f = [](auto &...args) { (..., As(args)); };

    static void bindBasic(sol::state& state)
    {


        MyStruct myStruct;
        StructApply(myStruct, f);
    }

    void LuaBinder::bindEverything(sol::state& state)
    {
        bindBasic(state);

        MetaEngine::LuaBinder::ImGuiWarp::Init(state);

    }

}
