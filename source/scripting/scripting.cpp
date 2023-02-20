// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "scripting.hpp"

#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "background.hpp"
#include "core/alloc.hpp"
#include "core/core.h"
#include "core/core.hpp"
#include "core/cpp/utils.hpp"
#include "core/debug.hpp"
#include "core/global.hpp"
#include "core/io/filesystem.h"
#include "core/profiler/profiler_lua.h"
#include "engine/engine_funcwrap.hpp"
#include "game.hpp"
#include "game_datastruct.hpp"
#include "game_resources.hpp"
#include "internal/builtin_lpeg.h"
#include "libs/lua/ffi.h"
#include "meta/meta.hpp"
#include "renderer/renderer_gpu.h"
#include "scripting/lua/lua_wrapper.hpp"
#include "ui/imgui/imgui_impl.hpp"

void LuaCodeInit(LuaCode *_struct, const char *scriptPath) {
    METADOT_ASSERT_E(_struct);
    strcpy(_struct->scriptPath, scriptPath);
}

void LuaCodeUpdate(LuaCode *_struct) {}

void LuaCodeFree(LuaCode *_struct) {}

void func1(std::string a) { std::cout << __FUNCTION__ << " :: " << a << std::endl; }
void func2(std::string a) { std::cout << __FUNCTION__ << " :: " << a << std::endl; }

extern int LoadImGuiBindings(lua_State *l);
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
    auto &LuaCore = Scripting::GetSingletonPtr()->Lua->s_lua;
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
    auto &s_lua = Scripting::GetSingletonPtr()->Lua->s_lua;
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

lua_State *LuaCoreCppFunc(void *luacorecpp) { return ((LuaCore *)luacorecpp)->s_lua.state(); }

static void InitLua(LuaCore *_struct) {

    _struct->L = _struct->s_lua.state();

    luaopen_base(_struct->L);
    luaL_openlibs(_struct->L);

    lua_atpanic(_struct->L, catch_panic);
    lua_register(_struct->L, "METADOT_TRACE", metadot_trace);
    lua_register(_struct->L, "METADOT_INFO", metadot_info);
    lua_register(_struct->L, "METADOT_WARN", metadot_warn);
    lua_register(_struct->L, "METADOT_BUG", metadot_bug);
    lua_register(_struct->L, "METADOT_ERROR", metadot_error);
    lua_register(_struct->L, "METADOT_RUN_FILE", metadot_run_lua_file_script);
    lua_register(_struct->L, "runf", metadot_run_lua_file_script);
    lua_register(_struct->L, "exit", metadot_exit);
    lua_register(_struct->L, "ls", ls);

    metadot_debug_setup(_struct->L, "debugger", "dbg", NULL, NULL);

    metadot_bind_image(_struct->L);
    metadot_bind_gpu(_struct->L);
    metadot_bind_fs(_struct->L);
    metadot_bind_lz4(_struct->L);
    metadot_bind_cstructcore(_struct->L);
    metadot_bind_cstructtest(_struct->L);
    metadot_bind_uilayout(_struct->L);
    metadot_bind_profiler(_struct->L);

    LoadImGuiBindings(_struct->L);

    metadot_preload_auto(_struct->L, luaopen_ffi, "ffi");
    metadot_preload_auto(_struct->L, luaopen_mu, "mu");
    metadot_preload_auto(_struct->L, luaopen_lpeg, "lpeg");

    // s_lua.set_function("METADOT_RESLOC", [](const std::string &a) { return METADOT_RESLOC(a); });

#define REGISTER_LUAFUNC(_f) _struct->s_lua[#_f] = LuaWrapper::function(_f)

    _struct->s_lua["METADOT_RESLOC"] = LuaWrapper::function([](const char *a) { return METADOT_RESLOC(a); });
    _struct->s_lua["GetSurfaceFromTexture"] = LuaWrapper::function([](Texture *tex) { return tex->surface; });

    REGISTER_LUAFUNC(SDL_FreeSurface);
    REGISTER_LUAFUNC(R_SetImageFilter);
    REGISTER_LUAFUNC(R_CopyImageFromSurface);
    REGISTER_LUAFUNC(R_GetTextureHandle);
    REGISTER_LUAFUNC(R_GetTextureAttr);
    REGISTER_LUAFUNC(LoadTextureData);
    REGISTER_LUAFUNC(DestroyTexture);
    REGISTER_LUAFUNC(CreateTexture);
    REGISTER_LUAFUNC(metadot_buildnum);
    REGISTER_LUAFUNC(metadot_metadata);
    REGISTER_LUAFUNC(add_packagepath);

#undef REGISTER_LUAFUNC

    _struct->s_lua.dostring(MetaEngine::Format("package.path = "
                                               "'{1}/?.lua;{0}/?.lua;{0}/libs/?.lua;{0}/libs/?/init.lua;{0}/libs/"
                                               "?/?.lua;' .. package.path",
                                               METADOT_RESLOC("data/scripts"), metadot_fs_getExecutableFolderPath()),
                            _struct->s_lua.globalTable());

    _struct->s_lua.dostring(MetaEngine::Format("package.cpath = "
                                               "'{1}/?.{2};{0}/?.{2};{0}/libs/?.{2};{0}/libs/?/init.{2};{0}/libs/"
                                               "?/?.{2};' .. package.cpath",
                                               METADOT_RESLOC("data/scripts"), metadot_fs_getExecutableFolderPath(), "dylib"),
                            _struct->s_lua.globalTable());

    s_couroutineFileSrc = readStringFromFile(METADOT_RESLOC("data/scripts/common/coroutines.lua"));
    RunScriptFromFile("data/scripts/startup.lua");
}

static void EndLua(LuaCore *_struct) {}

void RunScriptInConsole(LuaCore *_struct, const char *c) {
    luaL_loadstring(_struct->L, c);
    auto result = metadot_debug_pcall(_struct->L, 0, LUA_MULTRET, 0);
    if (result != LUA_OK) {
        print_error(_struct->L);
        return;
    }
}

void RunScriptFromFile(const char *filePath) {
    FUTIL_ASSERT_EXIST(filePath);

    int result = luaL_loadfile(Scripting::GetSingletonPtr()->Lua->L, METADOT_RESLOC(filePath));
    if (result != LUA_OK) {
        print_error(Scripting::GetSingletonPtr()->Lua->L);
        return;
    }
    result = metadot_debug_pcall(Scripting::GetSingletonPtr()->Lua->L, 0, LUA_MULTRET, 0);

    if (result != LUA_OK) {
        print_error(Scripting::GetSingletonPtr()->Lua->L);
    }
}

static void UpdateLua(LuaCore *_struct) {
    // todo store lua bytecode version instead (dont load it every tick)
    // lua_dump(_struct->L, &byteCodeWriterCallback, nullptr,false);
    // call coroutes
    luaL_loadstring(_struct->L, s_couroutineFileSrc.c_str());
    auto result = metadot_debug_pcall(_struct->L, 0, LUA_MULTRET, 0);

    if (result != LUA_OK) {
        print_error(_struct->L);
        return;
    }
}

static void InitMeo() {}

static void EndMeo() {}

static void UpdateMeo(){

}

#if 0
{

#define VAR_HERE() int, float, std::string
    using Type_Tuple = std::tuple<VAR_HERE()>;

    Type_Tuple tpt;
    std::apply([&](auto &&...args) {
        (printf("this type %s\n", typeid(args).name()), ...);
    },
               tpt);

}
#endif

void Scripting::Init() {
    Lua = new struct LuaCore;
    InitLua(Lua);
}

void Scripting::End() {

    EndLua(Lua);
    delete Lua;
}

void Scripting::Update() {
    METADOT_ASSERT_E(Lua);
    UpdateLua(Lua);
}

void Scripting::UpdateTick() {
    METADOT_ASSERT_E(Lua);
    auto &luawrap = Lua->s_lua;
    auto OnGameTickUpdate = luawrap["OnGameTickUpdate"];
    OnGameTickUpdate();
}
