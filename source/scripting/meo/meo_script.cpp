// Metadot Code Copyright(c) 2022-2023, KaoruXun All rights reserved.

// https://github.com/pigpigyyy/Yuescript
// https://github.com/axilmar/parserlib

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string_view>
#include <thread>
#include <tuple>

#include "core/io/filesystem.h"
#include "meo_core.h"

using namespace std::string_view_literals;
using namespace std::string_literals;
using namespace std::chrono_literals;
using namespace std::string_literals;

#ifndef METADOT_MEO_NO_MACRO
#define METADOT_MEO_ARGS nullptr, openlibs
#else
#define METADOT_MEO_ARGS
#endif  // METADOT_MEO_NO_MACRO

#ifndef METADOT_MEO_COMPILER_ONLY

extern "C" {

#include "libs/lua/host/lauxlib.h"
#include "libs/lua/host/lua.h"
#include "libs/lua/host/lualib.h"

static void init_meoscript(lua_State *L) {
    if (luaL_loadfile(L, METADOT_RESLOC("data/scripts/libs/meoscript.lua")) != 0) {
        std::string err = "failed to load meoscript module.\n"s + lua_tostring(L, -1);
        luaL_error(L, err.c_str());
    } else {
        lua_insert(L, -2);
        if (lua_pcall(L, 1, 0, 0) != 0) {
            std::string err = "failed to init meoscript module.\n"s + lua_tostring(L, -1);
            luaL_error(L, err.c_str());
        }
    }
}

static int init_stacktraceplus(lua_State *L) {
    if (luaL_loadfile(L, METADOT_RESLOC("data/scripts/libs/stacktraceplus.lua")) != 0) {
        std::string err = "failed to load stacktraceplus module.\n"s + lua_tostring(L, -1);
        luaL_error(L, err.c_str());
    } else if (lua_pcall(L, 0, 1, 0) != 0) {
        std::string err = "failed to init stacktraceplus module.\n"s + lua_tostring(L, -1);
        luaL_error(L, err.c_str());
    }
    return 1;
}

static int meotolua(lua_State *L) {
    size_t size = 0;
    const char *input = luaL_checklstring(L, 1, &size);
    Meo::MuConfig config;
    bool sameModule = false;
    if (lua_gettop(L) == 2) {
        luaL_checktype(L, 2, LUA_TTABLE);
        lua_pushliteral(L, "lint_global");
        lua_gettable(L, -2);
        if (lua_isboolean(L, -1) != 0) {
            config.lintGlobalVariable = lua_toboolean(L, -1) != 0;
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "implicit_return_root");
        lua_gettable(L, -2);
        if (lua_isboolean(L, -1) != 0) {
            config.implicitReturnRoot = lua_toboolean(L, -1) != 0;
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "reserve_line_number");
        lua_gettable(L, -2);
        if (lua_isboolean(L, -1) != 0) {
            config.reserveLineNumber = lua_toboolean(L, -1) != 0;
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "space_over_tab");
        lua_gettable(L, -2);
        if (lua_isboolean(L, -1) != 0) {
            config.useSpaceOverTab = lua_toboolean(L, -1) != 0;
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "same_module");
        lua_gettable(L, -2);
        if (lua_isboolean(L, -1) != 0) {
            sameModule = lua_toboolean(L, -1) != 0;
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "line_offset");
        lua_gettable(L, -2);
        if (lua_isnumber(L, -1) != 0) {
            config.lineOffset = static_cast<int>(lua_tonumber(L, -1));
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "module");
        lua_gettable(L, -2);
        if (lua_isstring(L, -1) != 0) {
            config.module = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "target");
        lua_gettable(L, -2);
        if (lua_isstring(L, -1) != 0) {
            config.options["target"] = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
    }
    std::string s(input, size);
    auto result = Meo::MeoCompiler(L, nullptr, sameModule).compile(s, config);
    if (result.codes.empty() && !result.error.empty()) {
        lua_pushnil(L);
    } else {
        lua_pushlstring(L, result.codes.c_str(), result.codes.size());
    }
    if (result.error.empty()) {
        lua_pushnil(L);
    } else {
        lua_pushlstring(L, result.error.c_str(), result.error.size());
    }
    if (result.globals) {
        lua_createtable(L, static_cast<int>(result.globals->size()), 0);
        int i = 1;
        for (const auto &var : *result.globals) {
            lua_createtable(L, 3, 0);
            lua_pushlstring(L, var.name.c_str(), var.name.size());
            lua_rawseti(L, -2, 1);
            lua_pushinteger(L, var.line);
            lua_rawseti(L, -2, 2);
            lua_pushinteger(L, var.col);
            lua_rawseti(L, -2, 3);
            lua_rawseti(L, -2, i);
            i++;
        }
    } else {
        lua_pushnil(L);
    }
    return 3;
}

static int meotoast(lua_State *L) {
    size_t size = 0;
    const char *input = luaL_checklstring(L, 1, &size);
    int flattenLevel = 2;
    if (lua_isnoneornil(L, 2) == 0) {
        flattenLevel = static_cast<int>(luaL_checkinteger(L, 2));
        flattenLevel = std::max(std::min(2, flattenLevel), 0);
    }
    Meo::MeoParser parser;
    auto info = parser.parse<Meo::File_t>({input, size});
    if (info.node) {
        lua_createtable(L, 0, 0);
        int cacheIndex = lua_gettop(L);
        auto getName = [&](Meo::ast_node *node) {
            int id = node->getId();
            lua_rawgeti(L, cacheIndex, id);
            if (lua_isnil(L, -1) != 0) {
                lua_pop(L, 1);
                auto name = node->getName();
                lua_pushlstring(L, &name.front(), name.length());
                lua_pushvalue(L, -1);
                lua_rawseti(L, cacheIndex, id);
            }
        };
        std::function<void(Meo::ast_node *)> visit;
        visit = [&](Meo::ast_node *node) {
            int count = 0;
            bool hasSep = false;
            node->visitChild([&](Meo::ast_node *child) {
                if (Meo::ast_is<Meo::Seperator_t>(child)) {
                    hasSep = true;
                    return false;
                }
                count++;
                visit(child);
                return false;
            });
            switch (count) {
                case 0: {
                    lua_createtable(L, 4, 0);
                    getName(node);
                    lua_rawseti(L, -2, 1);
                    lua_pushinteger(L, node->m_begin.m_line);
                    lua_rawseti(L, -2, 2);
                    lua_pushinteger(L, node->m_begin.m_col);
                    lua_rawseti(L, -2, 3);
                    auto str = parser.toString(node);
                    Meo::Utils::trim(str);
                    lua_pushlstring(L, str.c_str(), str.length());
                    lua_rawseti(L, -2, 4);
                    break;
                }
                case 1: {
                    if (flattenLevel > 1 || (flattenLevel == 1 && !hasSep)) {
                        getName(node);
                        lua_rawseti(L, -2, 1);
                        lua_pushinteger(L, node->m_begin.m_line);
                        lua_rawseti(L, -2, 2);
                        lua_pushinteger(L, node->m_begin.m_col);
                        lua_rawseti(L, -2, 3);
                        break;
                    }
                }
                default: {
                    lua_createtable(L, count + 3, 0);
                    getName(node);
                    lua_rawseti(L, -2, 1);
                    lua_pushinteger(L, node->m_begin.m_line);
                    lua_rawseti(L, -2, 2);
                    lua_pushinteger(L, node->m_begin.m_col);
                    lua_rawseti(L, -2, 3);
                    for (int i = count, j = 4; i >= 1; i--, j++) {
                        lua_pushvalue(L, -1 - i);
                        lua_rawseti(L, -2, j);
                    }
                    lua_insert(L, -1 - count);
                    lua_pop(L, count);
                    break;
                }
            }
        };
        visit(info.node);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushlstring(L, info.error.c_str(), info.error.length());
        return 2;
    }
}

static const luaL_Reg meo_libs[] = {{"to_lua", meotolua}, {"to_ast", meotoast}, {"version", nullptr}, {"options", nullptr}, {"load_stacktraceplus", nullptr}, {nullptr, nullptr}};

int luaopen_meo(lua_State *L) {
#if LUA_VERSION_NUM > 501
    luaL_newlib(L, meo_libs);
#else
    luaL_register(L, "meo", meo_libs);
#endif
    lua_pushstring(L, "0.0.1");
    lua_setfield(L, -2, "version");
    lua_createtable(L, 0, 0);
    lua_pushlstring(L, &Meo::extension.front(), Meo::extension.size());
    lua_setfield(L, -2, "extension");
    lua_pushliteral(L, LUA_DIRSEP);
    lua_setfield(L, -2, "dirsep");
    lua_setfield(L, -2, "options");
    lua_pushcfunction(L, init_stacktraceplus);
    lua_setfield(L, -2, "load_stacktraceplus");
    lua_pushvalue(L, -1);
    init_meoscript(L);
    return 1;
}
}

#if not(defined METADOT_MEO_NO_MACRO && defined METADOT_MEO_COMPILER_ONLY)
#define _DEFER(code, line) MetaEngine::Ref<void> _defer_##line(nullptr, [&](auto) { code; })
#define DEFER(code) _DEFER(code, __LINE__)

static void openlibs(void *state) {
    lua_State *L = static_cast<lua_State *>(state);
    luaL_openlibs(L);
#if LUA_VERSION_NUM > 501
    luaL_requiref(L, "meo", luaopen_meo, 1);
#else
    lua_pushcfunction(L, luaopen_meo);
    lua_call(L, 0, 0);
#endif
    lua_pop(L, 1);
}

void pushMeo(lua_State *L, std::string_view name) {
    lua_getglobal(L, "package");                     // package
    lua_getfield(L, -1, "loaded");                   // package loaded
    lua_getfield(L, -1, "meo");                      // package loaded meo
    lua_pushlstring(L, &name.front(), name.size());  // package loaded meo name
    lua_gettable(L, -2);                             // loaded[name], package loaded meo item
    lua_insert(L, -4);                               // item package loaded meo
    lua_pop(L, 3);                                   // item
}

void pushOptions(lua_State *L, int lineOffset) {
    lua_newtable(L);
    lua_pushliteral(L, "lint_global");
    lua_pushboolean(L, 0);
    lua_rawset(L, -3);
    lua_pushliteral(L, "implicit_return_root");
    lua_pushboolean(L, 1);
    lua_rawset(L, -3);
    lua_pushliteral(L, "reserve_line_number");
    lua_pushboolean(L, 1);
    lua_rawset(L, -3);
    lua_pushliteral(L, "space_over_tab");
    lua_pushboolean(L, 0);
    lua_rawset(L, -3);
    lua_pushliteral(L, "same_module");
    lua_pushboolean(L, 1);
    lua_rawset(L, -3);
    lua_pushliteral(L, "line_offset");
    lua_pushinteger(L, lineOffset);
    lua_rawset(L, -3);
}
#endif  // not (defined METADOT_MEO_NO_MACRO && defined METADOT_MEO_COMPILER_ONLY)

static void pushLuaminify(lua_State *L) {
    if (luaL_loadfile(L, METADOT_RESLOC("data/scripts/libs/luaminify.lua")) != 0) {
        std::string err = "failed to load luaminify module.\n"s + lua_tostring(L, -1);
        luaL_error(L, err.c_str());
    } else if (lua_pcall(L, 0, 1, 0) != 0) {
        std::string err = "failed to init luaminify module.\n"s + lua_tostring(L, -1);
        luaL_error(L, err.c_str());
    }
}
#endif  // METADOT_MEO_COMPILER_ONLY

std::filesystem::path getTargetFile(const std::filesystem::path &file, const std::filesystem::path &workPath, const std::filesystem::path &targetPath) {
    auto srcFile = std::filesystem::absolute(file);
    auto ext = srcFile.extension().string();
    for (auto &ch : ext) ch = std::tolower(ch);
    if (!ext.empty() && ext.substr(1) == Meo::extension) {
        auto targetFile = targetPath / srcFile.lexically_relative(workPath);
        targetFile.replace_extension("lua"s);
        if (std::filesystem::exists(targetFile)) {
            return targetFile;
        }
    }
    return std::filesystem::path();
}

std::filesystem::path getTargetFileDirty(const std::filesystem::path &file, const std::filesystem::path &workPath, const std::filesystem::path &targetPath) {
    if (!std::filesystem::exists(file)) return std::filesystem::path();
    auto srcFile = std::filesystem::absolute(file);
    auto ext = srcFile.extension().string();
    for (auto &ch : ext) ch = std::tolower(ch);
    if (!std::filesystem::is_directory(srcFile) && !ext.empty() && ext.substr(1) == Meo::extension) {
        auto targetFile = targetPath / srcFile.lexically_relative(workPath);
        targetFile.replace_extension("lua"s);
        if (std::filesystem::exists(targetFile)) {
            auto time = std::filesystem::last_write_time(targetFile);
            auto targetTime = std::filesystem::last_write_time(srcFile);
            if (time < targetTime) {
                return targetFile;
            }
        } else {
            return targetFile;
        }
    }
    return std::filesystem::path();
}

static std::string compileFile(const std::filesystem::path &file, Meo::MuConfig conf, const std::filesystem::path &workPath, const std::filesystem::path &targetPath) {
    auto srcFile = std::filesystem::absolute(file);
    auto targetFile = getTargetFileDirty(srcFile, workPath, targetPath);
    if (targetFile.empty()) return std::string();
    std::ifstream input(srcFile, std::ios::in);
    if (input) {
        std::string s((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        auto modulePath = srcFile.lexically_relative(workPath);
        if (modulePath.empty()) {
            modulePath = srcFile;
        }
        conf.module = modulePath.string();
        if (!workPath.empty()) {
            auto it = conf.options.find("path");
            if (it != conf.options.end()) {
                it->second += ';';
                it->second += (workPath / "?.lua"sv).string();
            } else {
                conf.options["path"] = (workPath / "?.lua"sv).string();
            }
        }
        auto result = Meo::MeoCompiler{METADOT_MEO_ARGS}.compile(s, conf);
        if (result.error.empty()) {
            std::string targetExtension("lua"sv);
            if (result.options) {
                auto it = result.options->find("target_extension"s);
                if (it != result.options->end()) {
                    targetExtension = it->second;
                }
            }
            if (targetFile.has_parent_path()) {
                std::filesystem::create_directories(targetFile.parent_path());
            }
            if (result.codes.empty()) {
                return "Built "s + modulePath.string() + '\n';
            }
            std::ofstream output(targetFile, std::ios::trunc | std::ios::out);
            if (output) {
                const auto &codes = result.codes;
                if (conf.reserveLineNumber) {
                    auto head = "-- [meo]: "s + modulePath.string() + '\n';
                    output.write(head.c_str(), head.size());
                }
                output.write(codes.c_str(), codes.size());
                return "Built "s + modulePath.string() + '\n';
            } else {
                return "Failed to write file: "s + targetFile.string() + '\n';
            }
        } else {
            return "Failed to compile: "s + modulePath.string() + '\n' + result.error + '\n';
        }
    } else {
        return "Failed to read file: "s + srcFile.string() + '\n';
    }
}
