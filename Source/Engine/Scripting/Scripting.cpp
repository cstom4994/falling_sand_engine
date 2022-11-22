// Copyright(c) 2022, KaoruXun All rights reserved.


#include "Engine/Memory/Memory.hpp"
#include "Engine/Scripting/LuaLayer.hpp"
#include "Engine/Scripting/MuCore.hpp"
#include "Game/Core.hpp"
#include "Game/DebugImpl.hpp"
#include "Game/FileSystem.hpp"
#include <iostream>
#include <memory>
#include <vector>

MuScript::MuScriptInterpreter *MuCore = nullptr;
LuaLayer *LuaCore = nullptr;

void LoadMuFuncs() {

    METADOT_ASSERT_E(MuCore);

    auto loadFunc = [](const MuScript::List &args) {
        METADOT_NEW(C, LuaCore, LuaLayer);
        LuaCore->getSolState()->script("METADOT_INFO(\'LuaLayer Inited\')");
        return std::make_shared<MuScript::Value>();
    };
    auto loadLua = MuCore->newFunction("loadLua", loadFunc);

    auto endFunc = [](const MuScript::List &args) {
        LuaCore->getSolState()->script("METADOT_INFO(\'LuaLayer End\')");
        METADOT_DELETE(C, LuaCore, LuaLayer);
        return std::make_shared<MuScript::Value>();
    };
    auto endLua = MuCore->newFunction("endLua", endFunc);
}

void METAENGINE_Scripting_Init() {
    METADOT_NEW(C, MuCore, MuScript::MuScriptInterpreter, MuScript::ModulePrivilege::allPrivilege);

    LoadMuFuncs();

    std::string init_src = MetaEngine::FUtil::readFileString("data/init.mu");
    MuCore->evaluate(init_src);

    auto end = MuCore->callFunction("init");
}

void METAENGINE_Scripting_End() {

    auto end = MuCore->callFunction("end");

    METADOT_DELETE(C, MuCore, MuScriptInterpreter);
}

void METAENGINE_Scripting_Update() {
    if (LuaCore) LuaCore->onUpdate();
}

#if 1

MuScript::Int integrationExample(MuScript::Int a, MuScript::Int b) {
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
    auto newfunc = MuCore->newFunction("integrationExample", [](const MuScript::List &args) {
        // MuScript doesn't enforce argument counts, so make sure you have enough
        if (args.size() < 2) {
            return std::make_shared<MuScript::Value>();
        }
        // Dereference arguments
        auto a = *args[0];
        auto b = *args[1];
        // Coerce types
        a.hardconvert(MuScript::Type::Int);
        b.hardconvert(MuScript::Type::Int);
        // Call c++ code
        auto result = integrationExample(a.getInt(), b.getInt());
        // Wrap and return
        return std::make_shared<MuScript::Value>(result);
    });

    // Step 2: Call into MuScript
    // send command into script interperereter
    MuCore->readLine("i = integrationExample(4, 3);");

    // get a value from the interpereter
    auto varRef = MuCore->resolveVariable("i");

    // or just call a function directly
    varRef = MuCore->callFunctionWithArgs(newfunc, MuScript::Int(4), MuScript::Int(3));

    // Setp 3: Unwrap your result
    // if the type is known
    int64_t i = varRef->getInt();
    std::cout << i << "\n";

    // visit style
    std::visit([](auto &&arg) { std::cout << arg << "\n"; }, varRef->value);

    // switch style
    switch (varRef->getType()) {
        case MuScript::Type::Int:
            std::cout << varRef->getInt() << "\n";
            break;
        case MuScript::Type::Float:
            std::cout << varRef->getFloat() << "\n";
            break;
        case MuScript::Type::String:
            std::cout << varRef->getString() << "\n";
            break;
        default:
            break;
    }

    // create a MuScript class from C++:
    MuCore->newClass("beansClass", {{"color", std::make_shared<MuScript::Value>("white")}},
                     // constructor is required
                     [](MuScript::Class *classs, MuScript::ScopeRef scope, const MuScript::List &vars) {
                         if (vars.size() > 0) {
                             MuCore->resolveVariable("color", classs, scope) = vars[0];
                         }
                         return std::make_shared<MuScript::Value>();
                     },
                     // add as many functions as you want
                     {
                             {"changeColor", [](MuScript::Class *classs, MuScript::ScopeRef scope, const MuScript::List &vars) {
                                  if (vars.size() > 0) {
                                      MuCore->resolveVariable("color", classs, scope) = vars[0];
                                  }
                                  return std::make_shared<MuScript::Value>();
                              }},
                             {"isRipe", [](MuScript::Class *classs, MuScript::ScopeRef scope, const MuScript::List &) {
                                  auto color = MuCore->resolveVariable("color", classs, scope);
                                  if (color->getType() == MuScript::Type::String) { return std::make_shared<MuScript::Value>(color->getString() == "brown"); }
                                  return std::make_shared<MuScript::Value>(false);
                              }},
                     });

    // use the class
    MuCore->readLine("bean = beansClass(\"grey\");");
    MuCore->readLine("ripe = bean.isRipe();");

    // get values from the interpereter
    auto beanRef = MuCore->resolveVariable("bean");
    auto ripeRef = MuCore->resolveVariable("ripe");

    // read the values!
    if (beanRef->getType() == MuScript::Type::Class && ripeRef->getType() == MuScript::Type::Int) {
        auto colorRef = beanRef->getClass()->variables["color"];
        if (colorRef->getType() == MuScript::Type::String) {
            std::cout << "My bean is " << beanRef->getClass()->variables["color"]->getString() << " and it is " << (ripeRef->getBool() ? "ripe" : "unripe") << "\n";
        }
    }
}

#endif