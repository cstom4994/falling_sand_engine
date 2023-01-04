// Metadot muscript is enhanced based on yuescript modification
// Metadot code Copyright(c) 2022-2023, KaoruXun All rights reserved.
// Yuescript code by Jin Li licensed under the MIT License
// Link to https://github.com/pigpigyyy/Yuescript

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
// IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "engine/scripting/muscript.h"

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

#include "engine/filesystem.h"
#include "engine/scripting/mu_compiler.h"
#include "engine/scripting/mu_parser.h"

using namespace std::string_view_literals;
using namespace std::string_literals;
using namespace std::chrono_literals;
using namespace std::string_literals;

#ifndef METADOT_MU_NO_MACRO
#define METADOT_MU_ARGS nullptr, openlibs
#else
#define METADOT_MU_ARGS
#endif  // METADOT_MU_NO_MACRO

#ifndef METADOT_MU_COMPILER_ONLY

extern "C" {

#include "libs/lua/host/lauxlib.h"
#include "libs/lua/host/lua.h"
#include "libs/lua/host/lualib.h"

static void init_muscript(lua_State *L) {
    if (luaL_loadfile(L, METADOT_RESLOC("data/scripts/libs/muscript.lua")) != 0) {
        std::string err = "failed to load muscript module.\n"s + lua_tostring(L, -1);
        luaL_error(L, err.c_str());
    } else {
        lua_insert(L, -2);
        if (lua_pcall(L, 1, 0, 0) != 0) {
            std::string err = "failed to init muscript module.\n"s + lua_tostring(L, -1);
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

static int mutolua(lua_State *L) {
    size_t size = 0;
    const char *input = luaL_checklstring(L, 1, &size);
    mu::MuConfig config;
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
    auto result = mu::MuCompiler(L, nullptr, sameModule).compile(s, config);
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

static int mutoast(lua_State *L) {
    size_t size = 0;
    const char *input = luaL_checklstring(L, 1, &size);
    int flattenLevel = 2;
    if (lua_isnoneornil(L, 2) == 0) {
        flattenLevel = static_cast<int>(luaL_checkinteger(L, 2));
        flattenLevel = std::max(std::min(2, flattenLevel), 0);
    }
    mu::MuParser parser;
    auto info = parser.parse<mu::File_t>({input, size});
    if (info.node) {
        lua_createtable(L, 0, 0);
        int cacheIndex = lua_gettop(L);
        auto getName = [&](mu::ast_node *node) {
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
        std::function<void(mu::ast_node *)> visit;
        visit = [&](mu::ast_node *node) {
            int count = 0;
            bool hasSep = false;
            node->visitChild([&](mu::ast_node *child) {
                if (mu::ast_is<mu::Seperator_t>(child)) {
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
                    mu::Utils::trim(str);
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

static const luaL_Reg mulib[] = {{"to_lua", mutolua}, {"to_ast", mutoast}, {"version", nullptr}, {"options", nullptr}, {"load_stacktraceplus", nullptr}, {nullptr, nullptr}};

int luaopen_mu(lua_State *L) {
#if LUA_VERSION_NUM > 501
    luaL_newlib(L, mulib);
#else
    luaL_register(L, "mu", mulib);
#endif
    lua_pushlstring(L, &mu::version.front(), mu::version.size());
    lua_setfield(L, -2, "version");
    lua_createtable(L, 0, 0);
    lua_pushlstring(L, &mu::extension.front(), mu::extension.size());
    lua_setfield(L, -2, "extension");
    lua_pushliteral(L, LUA_DIRSEP);
    lua_setfield(L, -2, "dirsep");
    lua_setfield(L, -2, "options");
    lua_pushcfunction(L, init_stacktraceplus);
    lua_setfield(L, -2, "load_stacktraceplus");
    lua_pushvalue(L, -1);
    init_muscript(L);
    return 1;
}
}

#if not(defined METADOT_MU_NO_MACRO && defined METADOT_MU_COMPILER_ONLY)
#define _DEFER(code, line) std::shared_ptr<void> _defer_##line(nullptr, [&](auto) { code; })
#define DEFER(code) _DEFER(code, __LINE__)

static void openlibs(void *state) {
    lua_State *L = static_cast<lua_State *>(state);
    luaL_openlibs(L);
#if LUA_VERSION_NUM > 501
    luaL_requiref(L, "mu", luaopen_mu, 1);
#else
    lua_pushcfunction(L, luaopen_mu);
    lua_call(L, 0, 0);
#endif
    lua_pop(L, 1);
}

void pushMu(lua_State *L, std::string_view name) {
    lua_getglobal(L, "package");                     // package
    lua_getfield(L, -1, "loaded");                   // package loaded
    lua_getfield(L, -1, "mu");                       // package loaded mu
    lua_pushlstring(L, &name.front(), name.size());  // package loaded mu name
    lua_gettable(L, -2);                             // loaded[name], package loaded mu item
    lua_insert(L, -4);                               // item package loaded mu
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
#endif  // not (defined METADOT_MU_NO_MACRO && defined METADOT_MU_COMPILER_ONLY)

static void pushLuaminify(lua_State *L) {
    if (luaL_loadfile(L, METADOT_RESLOC("data/scripts/libs/luaminify.lua")) != 0) {
        std::string err = "failed to load luaminify module.\n"s + lua_tostring(L, -1);
        luaL_error(L, err.c_str());
    } else if (lua_pcall(L, 0, 1, 0) != 0) {
        std::string err = "failed to init luaminify module.\n"s + lua_tostring(L, -1);
        luaL_error(L, err.c_str());
    }
}
#endif  // METADOT_MU_COMPILER_ONLY

std::filesystem::path getTargetFile(const std::filesystem::path &file, const std::filesystem::path &workPath, const std::filesystem::path &targetPath) {
    auto srcFile = std::filesystem::absolute(file);
    auto ext = srcFile.extension().string();
    for (auto &ch : ext) ch = std::tolower(ch);
    if (!ext.empty() && ext.substr(1) == mu::extension) {
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
    if (!std::filesystem::is_directory(srcFile) && !ext.empty() && ext.substr(1) == mu::extension) {
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

static std::string compileFile(const std::filesystem::path &file, mu::MuConfig conf, const std::filesystem::path &workPath, const std::filesystem::path &targetPath) {
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
        auto result = mu::MuCompiler{METADOT_MU_ARGS}.compile(s, conf);
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
                    auto head = "-- [mu]: "s + modulePath.string() + '\n';
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

class UpdateListener {
public:
    // void handleFileAction(efsw::WatchID, const std::string &dir, const std::string &filename,
    //                       efsw::Action action, std::string) override {
    //     switch (action) {
    //         case efsw::Actions::Add:
    //             if (auto res = compileFile(std::filesystem::path(dir) / filename, config, workPath, targetPath);
    //                 !res.empty()) {
    //                 std::cout << res;
    //             }
    //             break;
    //         case efsw::Actions::Delete: {
    //             auto targetFile = getTargetFile(std::filesystem::path(dir) / filename, workPath, targetPath);
    //             if (!targetFile.empty()) {
    //                 std::filesystem::remove(targetFile);
    //                 auto moduleFile = targetFile.lexically_relative(targetPath);
    //                 if (moduleFile.empty()) { moduleFile = targetFile; }
    //                 std::cout << "Deleted " << moduleFile.string() << '\n';
    //             }
    //             break;
    //         }
    //         case efsw::Actions::Modified:
    //             if (auto res = compileFile(std::filesystem::path(dir) / filename, config, workPath, targetPath);
    //                 !res.empty()) {
    //                 std::cout << res;
    //             }
    //             break;
    //         case efsw::Actions::Moved:
    //             break;
    //         default:
    //             break;
    //     }
    // }
    mu::MuConfig config;
    std::filesystem::path workPath;
    std::filesystem::path targetPath;
};

#if 0

int exe_mu(int narg, const char **args) {
    const char *help =
            "Usage: mu [options|files|directories] ...\n\n"
            "   -h       Print this message\n"
#ifndef METADOT_MU_COMPILER_ONLY
            "   -e str   Execute a file or raw codes\n"
            "   -m       Generate minified codes\n"
#endif  // METADOT_MU_COMPILER_ONLY
            "   -t path  Specify where to place compiled files\n"
            "   -o file  Write output to file\n"
            "   -s       Use spaces in generated codes instead of tabs\n"
            "   -p       Write output to standard out\n"
            "   -b       Dump compile time (doesn't write output)\n"
            "   -g       Dump global variables used in NAME LINE COLUMN\n"
            "   -l       Write line numbers from source codes\n"
            "   -w path  Watch changes and compile every file under directory\n"
            "   -v       Print version\n"
#ifndef METADOT_MU_COMPILER_ONLY
            "   --       Read from standard in, print to standard out\n"
            "            (Must be first and only argument)\n\n"
            "   --target=version  Specify the Lua version that codes will be generated to\n"
            "                     (version can only be 5.1, 5.2, 5.3 or 5.4)\n"
            "   --path=path_str   Append an extra Lua search path string to package.path\n\n"
            "   Execute without options to enter REPL, type symbol '$'\n"
            "   in a single line to start/stop multi-line mode\n"
#endif  // METADOT_MU_COMPILER_ONLY
            ;
#ifndef METADOT_MU_COMPILER_ONLY
    if (narg == 1) {
        lua_State *L = luaL_newstate();
        openlibs(L);
        DEFER(lua_close(L));
        pushMu(L, "insert_loader"sv);
        if (lua_pcall(L, 0, 0, 0) != 0) {
            std::cout << lua_tostring(L, -1) << '\n';
            return 1;
        }
        int count = 0;
        linenoise::SetMultiLine(false);
        linenoise::SetCompletionCallback(
                [](const char *editBuffer, std::vector<std::string> &completions) {
                    std::string buf = editBuffer;
                    std::string tmp = buf;
                    mu::Utils::trim(tmp);
                    if (tmp.empty()) return;
                    std::string pre;
                    auto pos = buf.find_first_not_of(" \t\n");
                    if (pos != std::string::npos) { pre = buf.substr(0, pos); }
                    switch (tmp[0]) {
                        case 'b':
                            completions.push_back(pre + "break");
                            break;
                        case 'c':
                            completions.push_back(pre + "class ");
                            completions.push_back(pre + "continue");
                            break;
                        case 'e':
                            completions.push_back(pre + "else");
                            completions.push_back(pre + "export ");
                            break;
                        case 'i':
                            completions.push_back(pre + "import \"");
                            break;
                        case 'g':
                            completions.push_back(pre + "global ");
                            break;
                        case 'l':
                            completions.push_back(pre + "local ");
                            break;
                        case 'm':
                            completions.push_back(pre + "macro ");
                            break;
                        case 's':
                            completions.push_back(pre + "switch ");
                            break;
                        case 'u':
                            completions.push_back(pre + "unless ");
                            break;
                        case 'w':
                            completions.push_back(pre + "with ");
                            completions.push_back(pre + "when ");
                            break;
                    }
                });
        std::cout << "Muscript "sv << mu::version << '\n';
        while (true) {
            count++;
            std::string codes;
            bool quit = linenoise::Readline("> ", codes);
            if (quit) return 0;
            linenoise::AddHistory(codes.c_str());
            mu::Utils::trim(codes);
            if (codes == "$"sv) {
                codes.clear();
                for (std::string line; !(quit = linenoise::Readline("", line));) {
                    auto temp = line;
                    mu::Utils::trim(temp);
                    if (temp == "$"sv) { break; }
                    codes += '\n';
                    codes += line;
                    linenoise::AddHistory(line.c_str());
                    mu::Utils::trim(codes);
                }
                if (quit) return 0;
            }
            codes.insert(0, "global *\n"sv);
            int top = lua_gettop(L);
            DEFER(lua_settop(L, top));
            pushMu(L, "loadstring"sv);
            lua_pushlstring(L, codes.c_str(), codes.size());
            lua_pushstring(L, ("=(repl "s + std::to_string(count) + ')').c_str());
            pushOptions(L, -1);
            const std::string_view Err = "\033[35m"sv, Val = "\033[33m"sv, Stop = "\033[0m\n"sv;
            if (lua_pcall(L, 3, 2, 0) != 0) {
                std::cout << Err << lua_tostring(L, -1) << Stop;
                continue;
            }
            if (lua_isnil(L, -2) != 0) {
                std::string err = lua_tostring(L, -1);
                auto modName = "(repl "s + std::to_string(count) + "):"s;
                if (err.substr(0, modName.size()) == modName) { err = err.substr(modName.size()); }
                auto pos = err.find(':');
                if (pos != std::string::npos) {
                    int lineNum = std::stoi(err.substr(0, pos));
                    err = std::to_string(lineNum - 1) + err.substr(pos);
                }
                std::cout << Err << err << Stop;
                continue;
            }
            lua_pop(L, 1);
            pushMu(L, "pcall"sv);
            lua_insert(L, -2);
            int last = lua_gettop(L) - 2;
            if (lua_pcall(L, 1, LUA_MULTRET, 0) != 0) {
                std::cout << Err << lua_tostring(L, -1) << Stop;
                continue;
            }
            int cur = lua_gettop(L);
            int retCount = cur - last;
            bool success = lua_toboolean(L, -retCount) != 0;
            if (success) {
                if (retCount > 1) {
                    for (int i = 1; i < retCount; ++i) {
#if LUA_VERSION_NUM > 501
                        std::cout << Val << luaL_tolstring(L, -retCount + i, nullptr) << Stop;
#else   // LUA_VERSION_NUM
                        lua_getglobal(L, "tostring");
                        lua_pushvalue(L, -retCount + i - 1);
                        lua_call(L, 1, 1);
                        std::cout << Val << lua_tostring(L, -1) << Stop;
#endif  // LUA_VERSION_NUM
                        lua_pop(L, 1);
                    }
                }
            } else {
                std::cout << Err << lua_tostring(L, -1) << Stop;
            }
        }
        std::cout << '\n';
        return 0;
    }
    bool minify = false;
#endif  // METADOT_MU_COMPILER_ONLY
    mu::MuConfig config;
    config.implicitReturnRoot = true;
    config.lintGlobalVariable = false;
    config.reserveLineNumber = false;
    config.useSpaceOverTab = false;
    bool writeToFile = true;
    bool dumpCompileTime = false;
    bool lintGlobal = false;
    bool watchFiles = false;
    std::string targetPath;
    std::string resultFile;
    std::string workPath;
    std::list<std::pair<std::string, std::string>> files;
    for (int i = 1; i < narg; ++i) {
        std::string arg = args[i];
        if (arg == "--"sv) {
            if (i != 1) {
                std::cout << help;
                return 1;
            }
            char ch;
            std::string codes;
            while ((ch = std::cin.get()) && !std::cin.eof()) { codes += ch; }
            mu::MuConfig conf;
            conf.implicitReturnRoot = true;
            conf.lintGlobalVariable = false;
            conf.reserveLineNumber = false;
            conf.useSpaceOverTab = true;
            auto result = mu::MuCompiler{METADOT_MU_ARGS}.compile(codes, conf);
            if (result.error.empty()) {
                std::cout << result.codes;
                return 0;
            } else {
                std::ostringstream buf;
                std::cout << result.error << '\n';
                return 1;
            }
#ifndef METADOT_MU_COMPILER_ONLY
        } else if (arg == "-e"sv) {
            ++i;
            if (i < narg) {
                lua_State *L = luaL_newstate();
                openlibs(L);
                DEFER(lua_close(L));
                pushMu(L, "insert_loader"sv);
                if (lua_pcall(L, 0, 0, 0) != 0) {
                    std::cout << lua_tostring(L, -1) << '\n';
                    return 1;
                }
                std::string evalStr = args[i];
                lua_newtable(L);
                i++;
                for (int j = i, index = 1; j < narg; j++) {
                    lua_pushstring(L, args[j]);
                    lua_rawseti(L, -2, index);
                    index++;
                }
                lua_setglobal(L, "arg");
                std::ifstream input(evalStr, std::ios::in);
                if (input) {
                    auto ext = std::filesystem::path(evalStr).extension().string();
                    for (auto &ch: ext) ch = std::tolower(ch);
                    if (ext == ".lua") {
                        lua_getglobal(L, "load");
                    } else {
                        pushMu(L, "loadstring"sv);
                    }
                    std::string s((std::istreambuf_iterator<char>(input)),
                                  std::istreambuf_iterator<char>());
                    lua_pushlstring(L, s.c_str(), s.size());
                    lua_pushlstring(L, evalStr.c_str(), evalStr.size());
                } else {
                    pushMu(L, "loadstring"sv);
                    lua_pushlstring(L, evalStr.c_str(), evalStr.size());
                    lua_pushliteral(L, "=(eval str)");
                }
                if (lua_pcall(L, 2, 2, 0) != 0) {
                    std::cout << lua_tostring(L, -1) << '\n';
                    return 1;
                }
                if (lua_isnil(L, -2) != 0) {
                    std::cout << lua_tostring(L, -1) << '\n';
                    return 1;
                }
                lua_pop(L, 1);
                pushMu(L, "pcall"sv);
                lua_insert(L, -2);
                int argCount = 0;
                while (i < narg) {
                    argCount++;
                    lua_pushstring(L, args[i]);
                    i++;
                }
                if (lua_pcall(L, 1 + argCount, 2, 0) != 0) {
                    std::cout << lua_tostring(L, -1) << '\n';
                    return 1;
                }
                bool success = lua_toboolean(L, -2) != 0;
                if (!success) {
                    std::cout << lua_tostring(L, -1) << '\n';
                    return 1;
                }
                return 0;
            } else {
                std::cout << help;
                return 1;
            }
        } else if (arg == "-m"sv) {
            minify = true;
#endif  // METADOT_MU_COMPILER_ONLY
        } else if (arg == "-s"sv) {
            config.useSpaceOverTab = true;
        } else if (arg == "-l"sv) {
            config.reserveLineNumber = true;
        } else if (arg == "-p"sv) {
            writeToFile = false;
        } else if (arg == "-g"sv) {
            writeToFile = false;
            lintGlobal = true;
        } else if (arg == "-t"sv) {
            ++i;
            if (i < narg) {
                targetPath = args[i];
            } else {
                std::cout << help;
                return 1;
            }
        } else if (arg == "-b"sv) {
            dumpCompileTime = true;
        } else if (arg == "-h"sv) {
            std::cout << help;
            return 0;
        } else if (arg == "-v"sv) {
            std::cout << "Muscript version: "sv << mu::version << '\n';
            return 0;
        } else if (arg == "-o"sv) {
            ++i;
            if (i < narg) {
                resultFile = args[i];
            } else {
                std::cout << help;
                return 1;
            }
        } else if (arg == "-w"sv) {
            watchFiles = true;
        } else if (arg.size() > 2 && arg.substr(0, 2) == "--"sv && arg.substr(2, 1) != "-"sv) {
            auto argStr = arg.substr(2);
            mu::Utils::trim(argStr);
            size_t idx = argStr.find('=');
            if (idx != std::string::npos) {
                auto key = argStr.substr(0, idx);
                auto value = argStr.substr(idx + 1);
                mu::Utils::trim(key);
                mu::Utils::trim(value);
                config.options[key] = value;
            } else {
                config.options[argStr] = std::string();
            }
        } else {
            if (std::filesystem::is_directory(arg)) {
                workPath = arg;
                for (auto item: std::filesystem::recursive_directory_iterator(arg)) {
                    if (!item.is_directory()) {
                        auto ext = item.path().extension().string();
                        for (char &ch: ext) ch = std::tolower(ch);
                        if (!ext.empty() && ext.substr(1) == mu::extension) {
                            files.emplace_back(item.path().string(),
                                               item.path().lexically_relative(arg).string());
                        }
                    }
                }
            } else if (watchFiles) {
                std::cout << "Error: -w can not be used with file\n"sv;
                return 1;
            } else {
                workPath = std::filesystem::path(arg).parent_path().string();
                files.emplace_back(arg, arg);
            }
        }
    }
    if (!watchFiles && files.empty()) {
        std::cout << "no input files\n"sv;
        return 0;
    }
    if (!resultFile.empty() && files.size() > 1) {
        std::cout << "Error: -o can not be used with multiple input files\n"sv;
        return 1;
    }
    if (watchFiles) {
        auto fullWorkPath = std::filesystem::absolute(std::filesystem::path(workPath)).string();
        auto fullTargetPath = fullWorkPath;
        if (!targetPath.empty()) { fullTargetPath = std::filesystem::absolute(std::filesystem::path(targetPath)).string(); }
        std::list<std::future<std::string>> results;
        for (const auto &file: files) {
            auto task = std::async(std::launch::async, [=]() {
                return compileFile(std::filesystem::absolute(file.first), config, fullWorkPath, fullTargetPath);
            });
            results.push_back(std::move(task));
        }
        for (auto &result: results) {
            std::string msg = result.get();
            if (!msg.empty()) { std::cout << msg; }
        }
        UpdateListener listener{};
        listener.config = config;
        listener.workPath = fullWorkPath;
        listener.targetPath = fullTargetPath;
        // fileWatcher.addWatch(workPath, &listener, true);
        // fileWatcher.watch();
        while (true) { std::this_thread::sleep_for(10000ms); }
        return 0;
    }
    std::list<std::future<std::tuple<int, std::string, std::string>>> results;
    for (const auto &file: files) {
        auto task = std::async(std::launch::async, [=]() {
            std::ifstream input(file.first, std::ios::in);
            if (input) {
                std::string s((std::istreambuf_iterator<char>(input)),
                              std::istreambuf_iterator<char>());
                auto conf = config;
                conf.module = file.first;
                if (!workPath.empty()) {
                    auto it = conf.options.find("path");
                    if (it != conf.options.end()) {
                        it->second += ';';
                        it->second += (std::filesystem::path(workPath) / "?.lua"sv).string();
                    } else {
                        conf.options["path"] = (std::filesystem::path(workPath) / "?.lua"sv).string();
                    }
                }
                if (dumpCompileTime) {
                    conf.profiling = true;
                    auto result = mu::MuCompiler{METADOT_MU_ARGS}.compile(s, conf);
                    if (!result.codes.empty()) {
                        std::ostringstream buf;
                        buf << file.first << " \n"sv;
                        buf << "Parse time:     "sv << std::setprecision(5)
                            << result.parseTime * 1000 << " ms\n";
                        buf << "Compile time:   "sv << std::setprecision(5)
                            << result.compileTime * 1000 << " ms\n\n";
                        return std::tuple{0, file.first, buf.str()};
                    } else {
                        std::ostringstream buf;
                        buf << "Failed to compile: "sv << file.first << '\n';
                        buf << result.error << '\n';
                        return std::tuple{1, file.first, buf.str()};
                    }
                }
                conf.lintGlobalVariable = lintGlobal;
                auto result = mu::MuCompiler{METADOT_MU_ARGS}.compile(s, conf);
                if (result.error.empty()) {
                    if (!writeToFile) {
                        if (lintGlobal) {
                            std::ostringstream buf;
                            for (const auto &global: *result.globals) {
                                buf << global.name << ' ' << global.line << ' ' << global.col
                                    << '\n';
                            }
                            return std::tuple{0, file.first, buf.str() + '\n'};
                        } else {
                            return std::tuple{0, file.first, result.codes + '\n'};
                        }
                    } else {
                        std::string targetExtension("lua"sv);
                        if (result.options) {
                            auto it = result.options->find("target_extension"s);
                            if (it != result.options->end()) { targetExtension = it->second; }
                        }
                        std::filesystem::path targetFile;
                        if (!resultFile.empty()) {
                            targetFile = resultFile;
                        } else {
                            if (!targetPath.empty()) {
                                targetFile = std::filesystem::path(targetPath) / file.second;
                            } else {
                                targetFile = file.first;
                            }
                            targetFile.replace_extension('.' + targetExtension);
                        }
                        if (targetFile.has_parent_path()) {
                            std::filesystem::create_directories(targetFile.parent_path());
                        }
                        if (result.codes.empty()) {
                            return std::tuple{0, targetFile.string(),
                                              "Built "s + file.first + '\n'};
                        }
                        std::ofstream output(targetFile, std::ios::trunc | std::ios::out);
                        if (output) {
                            const auto &codes = result.codes;
                            if (config.reserveLineNumber) {
                                auto head = "-- [mu]: "s + file.first + '\n';
                                output.write(head.c_str(), head.size());
                            }
                            output.write(codes.c_str(), codes.size());
                            return std::tuple{0, targetFile.string(),
                                              "Built "s + file.first + '\n'};
                        } else {
                            return std::tuple{1, std::string(),
                                              "Failed to write file: "s + targetFile.string() +
                                                      '\n'};
                        }
                    }
                } else {
                    std::ostringstream buf;
                    buf << "Failed to compile: "sv << file.first << '\n';
                    buf << result.error << '\n';
                    return std::tuple{1, std::string(), buf.str()};
                }
            } else {
                return std::tuple{1, std::string(), "Failed to read file: "s + file.first + '\n'};
            }
        });
        results.push_back(std::move(task));
    }
    int ret = 0;
#ifndef METADOT_MU_COMPILER_ONLY
    lua_State *L = nullptr;
    DEFER({
        if (L) lua_close(L);
    });
    if (minify) {
        L = luaL_newstate();
        luaL_openlibs(L);
        pushLuaminify(L);
    }
#endif  // METADOT_MU_COMPILER_ONLY
    std::list<std::string> errs;
    for (auto &result: results) {
        int val = 0;
        std::string file;
        std::string msg;
        std::tie(val, file, msg) = result.get();
        if (val != 0) {
            ret = val;
            errs.push_back(msg);
        } else {
#ifndef METADOT_MU_COMPILER_ONLY
            if (minify) {
                std::ifstream input(file, std::ios::in);
                if (input) {
                    std::string s;
                    if (writeToFile) {
                        s = std::string((std::istreambuf_iterator<char>(input)),
                                        std::istreambuf_iterator<char>());
                    } else {
                        s = msg;
                    }
                    input.close();
                    int top = lua_gettop(L);
                    DEFER(lua_settop(L, top));
                    lua_pushvalue(L, -1);
                    lua_pushlstring(L, s.c_str(), s.size());
                    if (lua_pcall(L, 1, 1, 0) != 0) {
                        ret = 2;
                        std::string err = lua_tostring(L, -1);
                        errs.push_back("Failed to minify: "s + file + '\n' + err + '\n');
                    } else {
                        size_t size = 0;
                        const char *minifiedCodes = lua_tolstring(L, -1, &size);
                        if (writeToFile) {
                            std::ofstream output(file, std::ios::trunc | std::ios::out);
                            output.write(minifiedCodes, size);
                            output.close();
                            std::cout << "Minified built "sv << file << '\n';
                        } else {
                            std::cout << minifiedCodes << '\n';
                        }
                    }
                } else {
                    ret = 2;
                    errs.push_back("Failed to minify: "s + file + '\n');
                }
            } else {
                std::cout << msg;
            }
#else
            std::cout << msg;
#endif  // METADOT_MU_COMPILER_ONLY
        }
    }
    for (const auto &err: errs) { std::cout << err; }
    return ret;
}

#endif