// Copyright(c) 2022, KaoruXun All rights reserved.

#include "engine/scripting.hpp"
#include "core/core.hpp"
#include "core/debug_impl.hpp"
#include "core/global.hpp"
#include "engine/code_reflection.hpp"
#include "engine/domainlang.hpp"
#include "engine/engine_funcwrap.hpp"
#include "engine/imgui_impl.hpp"
#include "engine/js_wrapper.hpp"
#include "engine/lua_wrapper.hpp"
#include "engine/memory.hpp"
#include "engine/scripting.hpp"
#include "game/background.hpp"
#include "engine/filesystem.h"
#include "game/game.hpp"
#include "game/game_datastruct.hpp"

#include "game/utils.hpp"
#include "libs/lua/ffi.h"
#include "quickjs/quickjs.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <cstring>

void func1(std::string a) { std::cout << __FUNCTION__ << " :: " << a << std::endl; }
void func2(std::string a) { std::cout << __FUNCTION__ << " :: " << a << std::endl; }

struct MyStruct
{
    void (*func1)(std::string);
    void (*func2)(std::string);
};

template<class T>
void As(T arg) {
    //auto namespac = state["Test"].get_or_create<sol::table>();
    //namespac.set_function();
}

auto f = [](auto &...args) { (..., As(args)); };

static void bindBasic() {

    MyStruct myStruct;
    Meta::StructApply(myStruct, f);
}

#define LUACON_ERROR_PREF "[error]"
#define LUACON_TRACE_PREF "[trace]"
#define LUACON_INFO_PREF "[info_]"
#define LUACON_WARN_PREF "[warn_]"

static std::string proxy_print(lua_State *L, const char *channel) {
    auto argNumber = lua_gettop(L);
    std::string s;
    s.reserve(100);
    for (int i = 1; i < argNumber + 1; ++i) {
        s += lua_tostring(L, i);
        s += i != argNumber ? ", " : "";
    }
    // (std::string(channel) + s).c_str()
    return s;
}

static int metadot_info(lua_State *L) {
    METADOT_INFO("[LUA] %s", proxy_print(L, LUACON_INFO_PREF).c_str());
    return 0;
}

static int metadot_trace(lua_State *L) {
    METADOT_TRACE("[LUA] %s", proxy_print(L, LUACON_TRACE_PREF).c_str());
    return 0;
}

static int metadot_error(lua_State *L) {
    METADOT_ERROR("[LUA] %s", proxy_print(L, LUACON_ERROR_PREF).c_str());
    return 0;
}

static int metadot_warn(lua_State *L) {
    METADOT_WARN("[LUA] %s", proxy_print(L, LUACON_WARN_PREF).c_str());
    return 0;
}

static int catch_panic(lua_State *L) {
    auto message = lua_tostring(L, -1);
    METADOT_ERROR("[LUA] PANIC ERROR: %s", message);
    return 0;
}

static int metadot_run_lua_file_script(lua_State *L) {
    std::string string = lua_tostring(L, 1);
    auto LuaCore = global.scripts->LuaRuntime;
    METADOT_ASSERT_E(LuaCore);
    if (SUtil::startsWith(string, "Script:"))
        SUtil::replaceWith(string, "Script:", METADOT_RESLOC("data/scripts/"));
    LuaCore->RunScriptFromFile(string.c_str());
    return 0;
}

static int metadot_exit(lua_State *L) {
    //s_lua_layer->closeConsole();
    return 0;
}

//returns table with pairs of path and isDirectory
static int ls(lua_State *L) {
    if (!lua_isstring(L, 1)) {
        METADOT_WARN("invalid lua argument");
        return 0;
    }
    auto string = lua_tostring(L, 1);
    if (!std::filesystem::is_directory(string)) {
        METADOT_WARN("%d is not directory", string);
        return 0;
    }

    lua_newtable(L);
    int i = 0;
    for (auto &p: std::filesystem::directory_iterator(string)) {
        lua_pushnumber(L, i + 1);// parent table index
        lua_newtable(L);
        lua_pushstring(L, "path");
        lua_pushstring(L, p.path().generic_string().c_str());
        lua_settable(L, -3);
        lua_pushstring(L, "isDirectory");
        lua_pushboolean(L, p.is_directory());
        lua_settable(L, -3);
        lua_settable(L, -3);
        i++;
    }
    return 1;
}

void LuaCore::print_error(lua_State *state) {
    const char *message = lua_tostring(state, -1);
    METADOT_ERROR("LuaScript ERROR:\n  %s", (message ? message : "no message"));
    lua_pop(state, 1);
}

static std::string s_couroutineFileSrc;
static char buf[1024];

static std::string readStringFromFile(const char *filePath) {
    FUTIL_ASSERT_EXIST(filePath);

    FILE *f = fopen(filePath, "r");
    if (!f) return "";
    int read = 0;
    std::string out;
    while ((read = fread(buf, 1, 1024, f)) != 0) out += std::string_view(buf, read);
    fclose(f);
    return out;
}

void LuaCore::Init() {
    m_L = s_lua.state();

    luaopen_base(m_L);
    luaL_openlibs(m_L);

    metadot_bind_image(m_L);
    metadot_bind_gpu(m_L);
    metadot_bind_fs(m_L);
    metadot_bind_lz4(m_L);

    LuaWrapper::metadot_preload(m_L, luaopen_ffi, "ffi");
    lua_getglobal(m_L, "require");
    lua_pushstring(m_L, "ffi");
    lua_call(m_L, 1, 0);

    lua_atpanic(m_L, catch_panic);
    lua_register(m_L, "METADOT_TRACE", metadot_trace);
    lua_register(m_L, "METADOT_INFO", metadot_info);
    lua_register(m_L, "METADOT_WARN", metadot_warn);
    lua_register(m_L, "METADOT_ERROR", metadot_error);
    lua_register(m_L, "METADOT_RUN_FILE", metadot_run_lua_file_script);
    lua_register(m_L, "runf", metadot_run_lua_file_script);
    lua_register(m_L, "exit", metadot_exit);
    lua_register(m_L, "ls", ls);

    // s_lua.set_function("METADOT_RESLOC", [](const std::string &a) { return METADOT_RESLOC(a); });

    s_lua["METADOT_RESLOC"] = LuaWrapper::function([](const char *a) { return METADOT_RESLOC(a); });

    s_lua.dostring(
            MetaEngine::Format("package.path = "
                               "'{1}/?.lua;{0}/?.lua;{0}/libs/?.lua;{0}/libs/?/init.lua;{0}/libs/"
                               "?/?.lua;' .. package.path",
                               METADOT_RESLOC("data/scripts"), FUtil_getExecutableFolderPath()),
            s_lua.globalTable());

    s_lua.dostring(
            MetaEngine::Format("package.cpath = "
                               "'{1}/?.{2};{0}/?.{2};{0}/libs/?.{2};{0}/libs/?/init.{2};{0}/libs/"
                               "?/?.{2};' .. package.cpath",
                               METADOT_RESLOC("data/scripts"), FUtil_getExecutableFolderPath(),
                               "dylib"),
            s_lua.globalTable());

    s_couroutineFileSrc = readStringFromFile(METADOT_RESLOC("data/scripts/coroutines.lua"));
    RunScriptFromFile("data/scripts/startup.lua");
}

void LuaCore::End() {}

void LuaCore::RunScriptInConsole(lua_State *L, const char *c) {
    luaL_loadstring(m_L, c);
    auto result = lua_pcall(m_L, 0, LUA_MULTRET, 0);

    if (result != LUA_OK) {
        print_error(m_L);
        return;
    }
}

void LuaCore::RunScriptFromFile(const char *filePath) {
    FUTIL_ASSERT_EXIST(filePath);

    int result = luaL_loadfile(m_L, METADOT_RESLOC(filePath));
    if (result != LUA_OK) {
        print_error(m_L);
        return;
    }
    result = lua_pcall(m_L, 0, LUA_MULTRET, 0);

    if (result != LUA_OK) { print_error(m_L); }
}

void LuaCore::Update() {
    //todo store lua bytecode version instead (dont load it every tick)
    //lua_dump(m_L, &byteCodeWriterCallback, nullptr,false);
    //call coroutes
    luaL_loadstring(m_L, s_couroutineFileSrc.c_str());
    auto result = lua_pcall(m_L, 0, LUA_MULTRET, 0);

    if (result != LUA_OK) {
        print_error(m_L);
        return;
    }
}

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
    char *init_src = futil_readfilestring("data/init.mu");
    MuDSL->evaluate(init_src);
    gc_free(&gc, init_src);
    // auto end = MuDSL->callFunction("init");

    METADOT_NEW(C, LuaRuntime, LuaCore);
    LuaRuntime->Init();

    METADOT_NEW(C, JsRuntime, JsWrapper::Runtime);
    METADOT_NEW(C, JsContext, JsWrapper::Context, *this->JsRuntime);

    // test_js();
    global.game->GameSystem_.gameScriptwrap.Init();
    global.game->GameSystem_.gameScriptwrap.Bind();
}

void Scripts::End() {

    METADOT_DELETE_EX(C, JsContext, Context, JsWrapper::Context);
    METADOT_DELETE_EX(C, JsRuntime, Runtime, JsWrapper::Runtime);

    LuaRuntime->End();
    METADOT_DELETE(C, LuaRuntime, LuaCore);

    auto end = MuDSL->callFunction("end");
    METADOT_DELETE_EX(C, MuDSL, MuDSLInterpreter, MuDSL::MuDSLInterpreter);
}

void Scripts::UpdateRender() {
    METADOT_ASSERT_E(JsContext && LuaRuntime);
    LuaRuntime->Update();
}

void Scripts::UpdateTick() {
    METADOT_ASSERT_E(JsContext && LuaRuntime);
    auto OnGameTickUpdate = (std::function<void(void)>) JsContext->eval("OnGameTickUpdate");
    OnGameTickUpdate();
}

void Scripts::LoadMuFuncs() { METADOT_ASSERT_E(MuDSL); }
