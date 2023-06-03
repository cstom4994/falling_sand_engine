// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "scripting.hpp"

#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "engine/background.hpp"
#include "engine/core/core.hpp"
#include "engine/core/cpp/utils.hpp"
#include "engine/core/debug.hpp"
#include "engine/core/global.hpp"
#include "engine/core/io/filesystem.h"
#include "engine/core/memory.h"
#include "engine/core/utils/utility.hpp"
#include "engine/engine_funcwrap.hpp"
#include "engine/game.hpp"
#include "engine/game_datastruct.hpp"
#include "engine/game_resources.hpp"
#include "engine/meta/reflection.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/ffi/ffi.h"
#include "engine/scripting/lua_wrapper.hpp"
#include "engine/scripting/lua_wrapper_ext.hpp"
#include "engine/ui/imgui_impl.hpp"

void func1(std::string a) { std::cout << __FUNCTION__ << " :: " << a << std::endl; }
void func2(std::string a) { std::cout << __FUNCTION__ << " :: " << a << std::endl; }

extern void LoadImGuiBindings(lua_State *l);

IMPLENGINE();

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
    ME::meta::struct_apply(myStruct, f);
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
    METADOT_BUG("[LUA] ", proxy_print(L).c_str());
    return 0;
}

static int metadot_info(lua_State *L) {
    METADOT_INFO("[LUA] ", proxy_print(L).c_str());
    return 0;
}

static int metadot_trace(lua_State *L) {
    METADOT_TRACE("[LUA] ", proxy_print(L).c_str());
    return 0;
}

static int metadot_error(lua_State *L) {
    METADOT_ERROR("[LUA] ", proxy_print(L).c_str());
    return 0;
}

static int metadot_warn(lua_State *L) {
    METADOT_WARN("[LUA] ", proxy_print(L).c_str());
    return 0;
}

static int catch_panic(lua_State *L) {
    auto message = lua_tostring(L, -1);
    METADOT_ERROR("[LUA] PANIC ERROR: ", message);
    return 0;
}

static int metadot_run_lua_file_script(lua_State *L) {
    std::string string = lua_tostring(L, 1);
    auto &LuaCore = Scripting::get_singleton_ptr()->Lua->s_lua;
    ME_ASSERT_E(&LuaCore);
    if (ME_str_starts_with(string, "Script:")) ME_str_replace_with(string, "Script:", METADOT_RESLOC("data/scripts/"));
    script_runfile(string.c_str());
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
        METADOT_WARN(std::format("{0} is not directory", string).c_str());
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
    auto &s_lua = Scripting::get_singleton_ptr()->Lua->s_lua;
    ME_ASSERT_E(&s_lua);
    s_lua.dostring(std::format("package.path = "
                               "'{0}/?.lua;' .. package.path",
                               p),
                   s_lua.globalTable());
}

static int R_GetTextureAttr(R_Image *image, const char *attr) {
    if (strcmp(attr, "w")) return image->w;
    if (strcmp(attr, "h")) return image->h;
    ME_ASSERT_E(0);
    return 0;
}

void print_error(lua_State *state, int result) {
    const char *message = lua_tostring(state, -1);
    METADOT_ERROR("LuaScript ERROR:\n  ", (message ? message : "no message"));

    if (result != 0) {
        switch (result) {
            case LUA_ERRRUN:
                METADOT_ERROR("Lua Runtime error");
                break;
            case LUA_ERRSYNTAX:
                METADOT_ERROR("Lua syntax error");
                break;
            case LUA_ERRMEM:
                METADOT_ERROR("Lua was unable to allocate the required memory");
                break;
            case LUA_ERRFILE:
                METADOT_ERROR("Lua was unable to find boot file");
                break;
            default:
                METADOT_ERROR("Unknown lua error: %d", result);
        }
    }

    lua_pop(state, 1);
}

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

static void InitLua(LuaCore *lc) {

    lc->L = lc->s_lua.state();

    luaopen_base(lc->L);
    luaL_openlibs(lc->L);

    lua_atpanic(lc->L, catch_panic);
    lua_register(lc->L, "METADOT_TRACE", metadot_trace);
    lua_register(lc->L, "METADOT_INFO", metadot_info);
    lua_register(lc->L, "METADOT_WARN", metadot_warn);
    lua_register(lc->L, "METADOT_BUG", metadot_bug);
    lua_register(lc->L, "METADOT_ERROR", metadot_error);
    lua_register(lc->L, "METADOT_RUN_FILE", metadot_run_lua_file_script);
    lua_register(lc->L, "runf", metadot_run_lua_file_script);
    lua_register(lc->L, "exit", metadot_exit);
    lua_register(lc->L, "ls", ls);

    ME_debug_setup(lc->L, "debugger", "dbg", NULL, NULL);

    metadot_bind_image(lc->L);
    metadot_bind_gpu(lc->L);
    metadot_bind_fs(lc->L);
    metadot_bind_lz4(lc->L);
    metadot_bind_cstructcore(lc->L);
    metadot_bind_cstructtest(lc->L);

    LoadImGuiBindings(lc->L);

    // ME_preload_auto(lc->L, luaopen_lpeg, "lpeg");

    ME_preload_auto(lc->L, ffi_module_open, "ffi");
    ME_preload_auto(lc->L, luaopen_lbind, "lbind");

#define REGISTER_LUAFUNC(_f) lc->s_lua[#_f] = LuaWrapper::function(_f)

    lc->s_lua["METADOT_RESLOC"] = LuaWrapper::function([](const char *a) { return METADOT_RESLOC(a); });
    lc->s_lua["GetSurfaceFromTexture"] = LuaWrapper::function([](Texture *tex) { return tex->surface; });
    lc->s_lua["GetWindowH"] = LuaWrapper::function([]() { return Screen.windowHeight; });
    lc->s_lua["GetWindowW"] = LuaWrapper::function([]() { return Screen.windowWidth; });

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

    lc->s_lua.dostring(std::format("package.path = "
                                   "'{1}/?.lua;{0}/?.lua;{0}/libs/?.lua;{0}/libs/?/init.lua;{0}/libs/"
                                   "?/?.lua;' .. package.path",
                                   METADOT_RESLOC("data/scripts"), normalizePath(std::filesystem::current_path().string()).c_str()),
                       lc->s_lua.globalTable());

    lc->s_lua.dostring(std::format("package.cpath = "
                                   "'{1}/?.{2};{0}/?.{2};{0}/libs/?.{2};{0}/libs/?/init.{2};{0}/libs/"
                                   "?/?.{2};' .. package.cpath",
                                   METADOT_RESLOC("data/scripts"), normalizePath(std::filesystem::current_path().string()).c_str(), "dll"),
                       lc->s_lua.globalTable());

    script_runfile("data/scripts/startup.lua");
}

static void EndLua(LuaCore *_struct) {}

void run_script_in_console(LuaCore *_struct, const char *c) {
    luaL_loadstring(_struct->L, c);
    auto result = ME_debug_pcall(_struct->L, 0, LUA_MULTRET, 0);
    if (result != LUA_OK) {
        print_error(_struct->L);
        return;
    }
}

void script_runfile(const char *filePath) {
    FUTIL_ASSERT_EXIST(filePath);

    int result = luaL_loadfile(Scripting::get_singleton_ptr()->Lua->L, METADOT_RESLOC(filePath));
    if (result != LUA_OK) {
        print_error(Scripting::get_singleton_ptr()->Lua->L);
        return;
    }
    result = ME_debug_pcall(Scripting::get_singleton_ptr()->Lua->L, 0, LUA_MULTRET, 0);

    if (result != LUA_OK) {
        print_error(Scripting::get_singleton_ptr()->Lua->L);
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
    Lua = new LuaCore;
    InitLua(Lua);
}

void Scripting::End() {

    EndLua(Lua);
    delete Lua;
}

void Scripting::Update() {
    ME_ASSERT_E(Lua);
    // luaL_loadstring(_struct->L, s_couroutineFileSrc.c_str());
    // if (metadot_debug_pcall(_struct->L, 0, LUA_MULTRET, 0) != LUA_OK) {
    //     print_error(_struct->L);
    //     return;
    // }
    auto &luawrap = Lua->s_lua;
    auto OnUpdate = luawrap["OnUpdate"];
    OnUpdate();
}

void Scripting::UpdateRender() {
    ME_ASSERT_E(Lua);
    auto &luawrap = Lua->s_lua;
    auto OnRender = luawrap["OnRender"];
    OnRender();
}

void Scripting::UpdateTick() {
    ME_ASSERT_E(Lua);
    auto &luawrap = Lua->s_lua;
    auto OnGameTickUpdate = luawrap["OnGameTickUpdate"];
    OnGameTickUpdate();
}
