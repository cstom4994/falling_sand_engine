// Copyright(c) 2022, KaoruXun All rights reserved.

#include "LuaCore.hpp"
#include "Core/DebugImpl.hpp"
#include "Core/Global.hpp"
#include "Engine/CodeReflection.hpp"
#include "Engine/EngineFuncWrap.hpp"
#include "Engine/ImGuiImplement.hpp"
#include "Engine/LuaWrapper.hpp"
#include "Engine/Memory.hpp"
#include "Engine/Scripting.hpp"
#include "Game/FileSystem.hpp"
#include "Game/GameDataStruct.hpp"
#include "Game/InEngine.h"
#include "Game/Utils.hpp"
#include "Libs/lua/lua.hpp"

#include "Game/Background.hpp"

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
    METADOT_INFO("[LUA] {0}", proxy_print(L, LUACON_INFO_PREF).c_str());
    return 0;
}

static int metadot_trace(lua_State *L) {
    METADOT_TRACE("[LUA] {0}", proxy_print(L, LUACON_TRACE_PREF).c_str());
    return 0;
}

static int metadot_error(lua_State *L) {
    METADOT_ERROR("[LUA] {0}", proxy_print(L, LUACON_ERROR_PREF).c_str());
    return 0;
}

static int metadot_warn(lua_State *L) {
    METADOT_WARN("[LUA] {0}", proxy_print(L, LUACON_WARN_PREF).c_str());
    return 0;
}

static int catch_panic(lua_State *L) {
    auto message = lua_tostring(L, -1);
    METADOT_ERROR("[LUA] PANIC ERROR: {}", message);
    return 0;
}

static int metadot_run_lua_file_script(lua_State *L) {
    auto string = lua_tostring(L, 1);
    auto LuaCore = global.scripts->LuaRuntime;
    METADOT_ASSERT_E(LuaCore);
    LuaCore->RunScriptFromFile(string);
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
        METADOT_WARN("{} is not directory", string);
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
    METADOT_ERROR("LuaScript ERROR:\n  {}", (message ? message : "no message"));
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

void LuaCore::Attach() {
    m_L = s_lua.state();

    luaopen_base(m_L);
    luaL_openlibs(m_L);
    luaopen_miniz(m_L);

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

    s_lua["METADOT_RESLOC"] =
            LuaWrapper::function([](const std::string &a) { return METADOT_RESLOC(a); });

    s_lua.dostring(fmt::format("package.path = "
                               "'{1}/?.lua;{0}/?.lua;{0}/libs/?.lua;{0}/libs/?/init.lua;{0}/libs/"
                               "?/?.lua;' .. package.path",
                               METADOT_RESLOC("data/scripts"), FUtil::getExecutableFolderPath()),
                   s_lua.globalTable());
    // s_lua.dostring(
    //         fmt::format(
    //                 "package.searchpath = '{1}/?.lua;{0}/?.lua;{0}/libs/?.lua;{0}/libs/?/init.lua;{0}/libs/?/?.lua;' .. package.searchpath",
    //                 METADOT_RESLOC("data/scripts"),
    //                 FUtil::getExecutableFolderPath()),
    //         s_lua.globalTable());

    s_couroutineFileSrc = readStringFromFile(METADOT_RESLOC_STR("data/scripts/coroutines.lua"));
    RunScriptFromFile("data/scripts/startup.lua");
}

void LuaCore::Detach() {}

void LuaCore::RunScriptInConsole(lua_State *L, const char *c) {
    luaL_loadstring(m_L, c);
    auto result = lua_pcall(m_L, 0, LUA_MULTRET, 0);

    if (result != LUA_OK) {
        print_error(m_L);
        return;
    }
}

void LuaCore::RunScriptFromFile(const std::string &filePath) {
    FUTIL_ASSERT_EXIST(filePath);

    int result = luaL_loadfile(m_L, METADOT_RESLOC_STR(filePath));
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
