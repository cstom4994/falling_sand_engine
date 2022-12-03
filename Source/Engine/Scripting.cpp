// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Engine/Scripting.hpp"
#include "Core/Core.hpp"
#include "Core/DebugImpl.hpp"
#include "Core/Global.hpp"
#include "Engine/JsWrapper.hpp"
#include "Engine/LuaCore.hpp"
#include "Engine/Memory.hpp"
#include "Engine/MuDSL.hpp"
#include "Game/FileSystem.hpp"
#include "Game/Game.hpp"
#include "quickjs/quickjs.h"
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#if 0

MuDSL::Int integrationExample(MuDSL::Int a, MuDSL::Int b) {
    return (a * b) + b;
}

void integrationExample() {

#define VAR_HERE() int, float, std::string
    using Type_Tuple = std::tuple<VAR_HERE()>;

    Type_Tuple tpt;
    std::apply([&](auto &&...args) {
        (printf("this type %s\n", typeid(args).name()), ...);
    },
               tpt);


    // Demo c++ integration
    // Step 1: Create a function wrapper
    auto newfunc = MuCore->newFunction("integrationExample", [](const MuDSL::List &args) {
        // MuDSL doesn't enforce argument counts, so make sure you have enough
        if (args.size() < 2) {
            return std::make_shared<MuDSL::Value>();
        }
        // Dereference arguments
        auto a = *args[0];
        auto b = *args[1];
        // Coerce types
        a.hardconvert(MuDSL::Type::Int);
        b.hardconvert(MuDSL::Type::Int);
        // Call c++ code
        auto result = integrationExample(a.getInt(), b.getInt());
        // Wrap and return
        return std::make_shared<MuDSL::Value>(result);
    });

    // Step 2: Call into MuDSL
    // send command into script interperereter
    MuCore->readLine("i = integrationExample(4, 3);");

    // get a value from the interpereter
    auto varRef = MuCore->resolveVariable("i");

    // or just call a function directly
    varRef = MuCore->callFunctionWithArgs(newfunc, MuDSL::Int(4), MuDSL::Int(3));

    // Setp 3: Unwrap your result
    // if the type is known
    int64_t i = varRef->getInt();
    std::cout << i << "\n";

    // visit style
    std::visit([](auto &&arg) { std::cout << arg << "\n"; }, varRef->value);

    // switch style
    switch (varRef->getType()) {
        case MuDSL::Type::Int:
            std::cout << varRef->getInt() << "\n";
            break;
        case MuDSL::Type::Float:
            std::cout << varRef->getFloat() << "\n";
            break;
        case MuDSL::Type::String:
            std::cout << varRef->getString() << "\n";
            break;
        default:
            break;
    }

    // create a MuDSL class from C++:
    MuCore->newClass("beansClass", {{"color", std::make_shared<MuDSL::Value>("white")}},
                     // constructor is required
                     [](MuDSL::Class *classs, MuDSL::ScopeRef scope, const MuDSL::List &vars) {
                         if (vars.size() > 0) {
                             MuCore->resolveVariable("color", classs, scope) = vars[0];
                         }
                         return std::make_shared<MuDSL::Value>();
                     },
                     // add as many functions as you want
                     {
                             {"changeColor", [](MuDSL::Class *classs, MuDSL::ScopeRef scope, const MuDSL::List &vars) {
                                  if (vars.size() > 0) {
                                      MuCore->resolveVariable("color", classs, scope) = vars[0];
                                  }
                                  return std::make_shared<MuDSL::Value>();
                              }},
                             {"isRipe", [](MuDSL::Class *classs, MuDSL::ScopeRef scope, const MuDSL::List &) {
                                  auto color = MuCore->resolveVariable("color", classs, scope);
                                  if (color->getType() == MuDSL::Type::String) { return std::make_shared<MuDSL::Value>(color->getString() == "brown"); }
                                  return std::make_shared<MuDSL::Value>(false);
                              }},
                     });

    // use the class
    MuCore->readLine("bean = beansClass(\"grey\");");
    MuCore->readLine("ripe = bean.isRipe();");

    // get values from the interpereter
    auto beanRef = MuCore->resolveVariable("bean");
    auto ripeRef = MuCore->resolveVariable("ripe");

    // read the values!
    if (beanRef->getType() == MuDSL::Type::Class && ripeRef->getType() == MuDSL::Type::Int) {
        auto colorRef = beanRef->getClass()->variables["color"];
        if (colorRef->getType() == MuDSL::Type::String) {
            std::cout << "My bean is " << beanRef->getClass()->variables["color"]->getString() << " and it is " << (ripeRef->getBool() ? "ripe" : "unripe") << "\n";
        }
    }
}

#endif

void Scripts::Init() {
    METADOT_NEW(C, MuDSL, MuDSL::MuDSLInterpreter, MuDSL::ModulePrivilege::allPrivilege);
    LoadMuFuncs();
    std::string init_src = FUtil::readFileString("data/init.mu");
    MuDSL->evaluate(init_src);
    // auto end = MuDSL->callFunction("init");

    LuaCore *L = nullptr;
    METADOT_NEW(C, L, LuaCore);
    LuaMap.insert(std::make_pair("LuaCore", L));
    L->Attach();

    METADOT_NEW(C, JsRuntime, JsWrapper::Runtime);
    METADOT_NEW(C, JsContext, JsWrapper::Context, *this->JsRuntime);

    // test_js();
    global.game->GameSystem_.gsw.Init();
    global.game->GameSystem_.gsw.Bind();
}

void Scripts::End() {

    METADOT_DELETE_EX(C, JsContext, Context, JsWrapper::Context);
    METADOT_DELETE_EX(C, JsRuntime, Runtime, JsWrapper::Runtime);

    auto L = LuaMap["LuaCore"];
    L->Detach();
    LuaMap.erase("LuaCore");
    METADOT_DELETE(C, L, LuaCore);

    auto end = MuDSL->callFunction("end");
    METADOT_DELETE_EX(C, MuDSL, MuDSLInterpreter, MuDSL::MuDSLInterpreter);
}

void Scripts::Update() {
    if (auto LuaCore = LuaMap["LuaCore"]) LuaCore->Update();
}

void Scripts::LoadMuFuncs() { METADOT_ASSERT_E(MuDSL); }