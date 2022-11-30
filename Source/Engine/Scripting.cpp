// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Engine/Scripting.hpp"
#include "Core/Core.hpp"
#include "Core/DebugImpl.hpp"
#include "Engine/Memory.hpp"
#include "Engine/JsMachine.hpp"
#include "Engine/LuaMachine.hpp"
#include "Engine/MuDSL.hpp"
#include "Game/FileSystem.hpp"
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

class MyClass {
public:
    MyClass() {}
    MyClass(std::vector<int>) {}

    double member_variable = 5.5;
    std::string member_function(const std::string &s) { return "Hello, " + s; }
};

void println(JsMachine::rest<std::string> args) {
    for (auto const &arg: args) METADOT_INFO("{}", arg);
}

void test_js() {
    JsMachine::Runtime runtime;
    JsMachine::Context context(runtime);
    try {
        // export classes as a module
        auto &module = context.addModule("MyModule");
        module.function<&println>("println");
        module.class_<MyClass>("MyClass")
                .constructor<>()
                .constructor<std::vector<int>>("MyClassA")
                .fun<&MyClass::member_variable>("member_variable")
                .fun<&MyClass::member_function>("member_function");
        // import module
        context.eval(R"xxx(
            import * as my from 'MyModule';
            globalThis.my = my;
        )xxx",
                     "<import>", JS_EVAL_TYPE_MODULE);
        // evaluate js code
        context.eval(R"xxx(
            let v1 = new my.MyClass();
            v1.member_variable = 1;
            let v2 = new my.MyClassA([1,2,3]);
            function my_callback(str) {
              my.println("at callback:", v2.member_function(str));
            }
        )xxx");

        // callback
        auto cb = (std::function<void(const std::string &)>) context.eval("my_callback");
        cb("world");

        // passing c++ objects to JS
        auto lambda = context.eval("x=>my.println(x.member_function('lambda'))").as<std::function<void(JsMachine::shared_ptr<MyClass>)>>();
        auto v3 = JsMachine::make_shared<MyClass>(context.ctx, std::vector{1, 2, 3});
        lambda(v3);
    } catch (JsMachine::exception) {
        auto exc = context.getException();
        std::cerr << (std::string) exc << std::endl;
        if ((bool) exc["stack"])
            std::cerr << (std::string) exc["stack"] << std::endl;
    }
}


void Scripts::Init() {
    METADOT_NEW(C, MuDSL, MuDSL::MuDSLInterpreter, MuDSL::ModulePrivilege::allPrivilege);

    LoadMuFuncs();

    std::string init_src = FUtil::readFileString("data/init.mu");
    MuDSL->evaluate(init_src);

    auto end = MuDSL->callFunction("init");

    test_js();
}

void Scripts::End() {

    auto end = MuDSL->callFunction("end");

    METADOT_DELETE_EX(C, MuDSL, MuDSLInterpreter, MuDSL::MuDSLInterpreter);
}

void Scripts::Update() {
    if (auto LuaCore = LuaMap["LuaCore"]) LuaCore->Update();
}

void Scripts::LoadMuFuncs() {

    METADOT_ASSERT_E(MuDSL);

    auto loadFunc = [&](const MuDSL::List &args) {
        LuaMachine *LuaCore = nullptr;
        METADOT_NEW(C, LuaCore, LuaMachine);
        LuaMap.insert(std::make_pair("LuaCore", LuaCore));
        LuaCore->Attach();
        LuaCore->getSolState()->script("METADOT_INFO(\'LuaLayer Inited\')");
        return std::make_shared<MuDSL::Value>();
    };
    auto loadLua = MuDSL->newFunction("loadLua", loadFunc);

    auto endFunc = [&](const MuDSL::List &args) {
        auto LuaCore = LuaMap["LuaCore"];
        LuaCore->getSolState()->script("METADOT_INFO(\'LuaLayer End\')");
        LuaCore->Detach();
        LuaMap.erase("LuaCore");
        METADOT_DELETE(C, LuaCore, LuaMachine);
        return std::make_shared<MuDSL::Value>();
    };
    auto endLua = MuDSL->newFunction("endLua", endFunc);
}