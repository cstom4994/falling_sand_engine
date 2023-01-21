// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine/engine_scripting.hpp"

#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "SDL_surface.h"
#include "core/alloc.h"
#include "core/core.h"
#include "core/core.hpp"
#include "core/cpp/utils.hpp"
#include "core/debug_impl.hpp"
#include "core/global.hpp"
#include "engine/code_reflection.hpp"
#include "engine/ecs/luaecs.h"
#include "engine/engine_funcwrap.hpp"
#include "engine/filesystem.h"
#include "engine/imgui_impl.hpp"
#include "engine/internal/builtin_lpeg.h"
#include "engine/memory.hpp"
#include "engine/scripting/lua_wrapper.hpp"
#include "game/background.hpp"
#include "game/game.hpp"
#include "game/game_datastruct.hpp"
#include "game/game_resources.hpp"
#include "libs/lua/ffi.h"
#include "renderer/renderer_gpu.h"

void InitLuaCoreC(LuaCoreC *_struct, lua_State *LuaCoreCppFunc(void *), void *luacorecpp) {
    METADOT_ASSERT_E(_struct);
    _struct->L = LuaCoreCppFunc(luacorecpp);
}

void LuaCodeInit(LuaCode *_struct, const char *scriptPath) {
    METADOT_ASSERT_E(_struct);
    strcpy(_struct->scriptPath, scriptPath);
}

void LuaCodeUpdate(LuaCode *_struct) {}

void LuaCodeFree(LuaCode *_struct) {}

void func1(std::string a) { std::cout << __FUNCTION__ << " :: " << a << std::endl; }
void func2(std::string a) { std::cout << __FUNCTION__ << " :: " << a << std::endl; }

extern int LoadImguiBindings(lua_State *l);
extern "C" {
extern int luaopen_mu(lua_State *L);
}

struct MyStruct {
    void (*func1)(std::string);
    void (*func2)(std::string);
};

template <class T>
void As(T arg) {
    // auto namespac = state["Test"].get_or_create<sol::table>();
    // namespac.set_function();
}

auto f = [](auto &...args) { (..., As(args)); };

static void bindBasic() {

    MyStruct myStruct;
    Meta::StructApply(myStruct, f);
}

static std::string proxy_print(lua_State *L) {
    auto argNumber = lua_gettop(L);
    std::string s;
    s.reserve(100);
    for (int i = 1; i < argNumber + 1; ++i) {
        s += lua_tostring(L, i);
        s += i != argNumber ? ", " : "";
    }
    return s;
}

static int metadot_bug(lua_State *L) {
    METADOT_BUG("[LUA] %s", proxy_print(L).c_str());
    return 0;
}

static int metadot_info(lua_State *L) {
    METADOT_INFO("[LUA] %s", proxy_print(L).c_str());
    return 0;
}

static int metadot_trace(lua_State *L) {
    METADOT_TRACE("[LUA] %s", proxy_print(L).c_str());
    return 0;
}

static int metadot_error(lua_State *L) {
    METADOT_ERROR("[LUA] %s", proxy_print(L).c_str());
    return 0;
}

static int metadot_warn(lua_State *L) {
    METADOT_WARN("[LUA] %s", proxy_print(L).c_str());
    return 0;
}

static int catch_panic(lua_State *L) {
    auto message = lua_tostring(L, -1);
    METADOT_ERROR("[LUA] PANIC ERROR: %s", message);
    return 0;
}

static int metadot_run_lua_file_script(lua_State *L) {
    std::string string = lua_tostring(L, 1);
    auto &LuaCore = global.scripts->LuaCoreCpp->s_lua;
    METADOT_ASSERT_E(&LuaCore);
    if (SUtil::startsWith(string, "Script:")) SUtil::replaceWith(string, "Script:", METADOT_RESLOC("data/scripts/"));
    RunScriptFromFile(string.c_str());
    return 0;
}

static int metadot_exit(lua_State *L) {
    // s_lua_layer->closeConsole();
    return 0;
}

// returns table with pairs of path and isDirectory
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
    for (auto &p : std::filesystem::directory_iterator(string)) {
        lua_pushnumber(L, i + 1);  // parent table index
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

static void add_packagepath(const char *p) {
    auto &s_lua = global.scripts->LuaCoreCpp->s_lua;
    METADOT_ASSERT_E(&s_lua);
    s_lua.dostring(MetaEngine::Format("package.path = "
                                      "'{0}/?.lua;' .. package.path",
                                      p),
                   s_lua.globalTable());
}

static int R_GetTextureAttr(R_Image *image, const char *attr) {
    if (strcmp(attr, "w")) return image->w;
    if (strcmp(attr, "h")) return image->h;
    METADOT_ASSERT_E(0);
    return 0;
}

void print_error(lua_State *state) {
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

lua_State *LuaCoreCppFunc(void *luacorecpp) { return ((LuaCoreCpp *)luacorecpp)->s_lua.state(); }

void InitLuaCoreCpp(LuaCoreCpp *_struct) {

    // Init LuaCoreC
    _struct->C = (LuaCoreC *)gc_malloc(&gc, sizeof(LuaCoreC));

    InitLuaCoreC(_struct->C, LuaCoreCppFunc, (void *)_struct);

    luaopen_base(_struct->C->L);
    luaL_openlibs(_struct->C->L);

    metadot_debug_setup(_struct->C->L, "debugger", "dbg", NULL, NULL);

    lua_atpanic(_struct->C->L, catch_panic);
    lua_register(_struct->C->L, "METADOT_TRACE", metadot_trace);
    lua_register(_struct->C->L, "METADOT_INFO", metadot_info);
    lua_register(_struct->C->L, "METADOT_WARN", metadot_warn);
    lua_register(_struct->C->L, "METADOT_BUG", metadot_bug);
    lua_register(_struct->C->L, "METADOT_ERROR", metadot_error);
    lua_register(_struct->C->L, "METADOT_RUN_FILE", metadot_run_lua_file_script);
    lua_register(_struct->C->L, "runf", metadot_run_lua_file_script);
    lua_register(_struct->C->L, "exit", metadot_exit);
    lua_register(_struct->C->L, "ls", ls);

    metadot_bind_image(_struct->C->L);
    metadot_bind_gpu(_struct->C->L);
    metadot_bind_fs(_struct->C->L);
    metadot_bind_lz4(_struct->C->L);
    metadot_bind_cstructcore(_struct->C->L);
    metadot_bind_cstructtest(_struct->C->L);
    metadot_bind_uilayout(_struct->C->L);

    LoadImguiBindings(_struct->C->L);

    metadot_preload_auto(_struct->C->L, luaopen_ffi, "ffi");
    metadot_preload_auto(_struct->C->L, luaopen_mu, "mu");
    metadot_preload_auto(_struct->C->L, luaopen_lpeg, "lpeg");
    metadot_preload_auto(_struct->C->L, metadot_bind_ecs_core, "cecs");
    metadot_preload_auto(_struct->C->L, metadot_bind_ecs_test, "cecs_test");

    // s_lua.set_function("METADOT_RESLOC", [](const std::string &a) { return METADOT_RESLOC(a); });

#define REGISTER_LUAFUNC(_f) _struct->s_lua[#_f] = LuaWrapper::function(_f)

    _struct->s_lua["METADOT_RESLOC"] = LuaWrapper::function([](const char *a) { return METADOT_RESLOC(a); });
    _struct->s_lua["Eng_GetSurfaceFromTexture"] = LuaWrapper::function([](Texture *tex) { return tex->surface; });

    REGISTER_LUAFUNC(SDL_FreeSurface);
    REGISTER_LUAFUNC(R_SetImageFilter);
    REGISTER_LUAFUNC(R_CopyImageFromSurface);
    REGISTER_LUAFUNC(R_GetTextureHandle);
    REGISTER_LUAFUNC(R_GetTextureAttr);
    REGISTER_LUAFUNC(Eng_LoadTextureData);
    REGISTER_LUAFUNC(Eng_DestroyTexture);
    REGISTER_LUAFUNC(Eng_CreateTexture);
    REGISTER_LUAFUNC(metadot_buildnum);
    REGISTER_LUAFUNC(metadot_metadata);
    REGISTER_LUAFUNC(add_packagepath);
    // REGISTER_LUAFUNC(FontCache_CreateFont);
    // REGISTER_LUAFUNC(FontCache_LoadFont);
    // REGISTER_LUAFUNC(FontCache_MakeColor);
    // REGISTER_LUAFUNC(FontCache_FreeFont);

#undef REGISTER_LUAFUNC

    _struct->s_lua.dostring(MetaEngine::Format("package.path = "
                                               "'{1}/?.lua;{0}/?.lua;{0}/libs/?.lua;{0}/libs/?/init.lua;{0}/libs/"
                                               "?/?.lua;' .. package.path",
                                               METADOT_RESLOC("data/scripts"), FUtil_getExecutableFolderPath()),
                            _struct->s_lua.globalTable());

    _struct->s_lua.dostring(MetaEngine::Format("package.cpath = "
                                               "'{1}/?.{2};{0}/?.{2};{0}/libs/?.{2};{0}/libs/?/init.{2};{0}/libs/"
                                               "?/?.{2};' .. package.cpath",
                                               METADOT_RESLOC("data/scripts"), FUtil_getExecutableFolderPath(), "dylib"),
                            _struct->s_lua.globalTable());

    s_couroutineFileSrc = readStringFromFile(METADOT_RESLOC("data/scripts/common/coroutines.lua"));
    RunScriptFromFile("data/scripts/startup.lua");
}

void EndLuaCoreCpp(LuaCoreCpp *_struct) { gc_free(&gc, _struct->C); }

void RunScriptInConsole(LuaCoreCpp *_struct, const char *c) {
    luaL_loadstring(_struct->C->L, c);
    auto result = metadot_debug_pcall(_struct->C->L, 0, LUA_MULTRET, 0);
    if (result != LUA_OK) {
        print_error(_struct->C->L);
        return;
    }
}

void RunScriptFromFile(const char *filePath) {
    FUTIL_ASSERT_EXIST(filePath);

    int result = luaL_loadfile(global.scripts->LuaCoreCpp->C->L, METADOT_RESLOC(filePath));
    if (result != LUA_OK) {
        print_error(global.scripts->LuaCoreCpp->C->L);
        return;
    }
    result = metadot_debug_pcall(global.scripts->LuaCoreCpp->C->L, 0, LUA_MULTRET, 0);

    if (result != LUA_OK) {
        print_error(global.scripts->LuaCoreCpp->C->L);
    }
}

void UpdateLuaCoreCpp(LuaCoreCpp *_struct) {
    // todo store lua bytecode version instead (dont load it every tick)
    // lua_dump(_struct->C->L, &byteCodeWriterCallback, nullptr,false);
    // call coroutes
    luaL_loadstring(_struct->C->L, s_couroutineFileSrc.c_str());
    auto result = metadot_debug_pcall(_struct->C->L, 0, LUA_MULTRET, 0);

    if (result != LUA_OK) {
        print_error(_struct->C->L);
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
    LuaCoreCpp = new struct LuaCoreCpp;
    InitLuaCoreCpp(LuaCoreCpp);
    global.game->GameIsolate_.gameplayscript->RegisterLua(LuaCoreCpp->s_lua);
    global.game->GameIsolate_.gameplayscript->Create();
}

void Scripts::End() {
    global.game->GameIsolate_.gameplayscript->Destory();

    EndLuaCoreCpp(LuaCoreCpp);
    delete LuaCoreCpp;
}

void Scripts::UpdateRender() {
    METADOT_ASSERT_E(LuaCoreCpp);
    UpdateLuaCoreCpp(LuaCoreCpp);
}

void Scripts::UpdateTick() {
    METADOT_ASSERT_E(LuaCoreCpp);
    auto &luawrap = LuaCoreCpp->s_lua;
    auto OnGameTickUpdate = luawrap["OnGameTickUpdate"];
    OnGameTickUpdate();
}
