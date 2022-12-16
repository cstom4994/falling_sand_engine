// Metadot muscript is enhanced based on yuescript modification
// Metadot code copyright(c) 2022, KaoruXun All rights reserved.
// Yuescript code by Jin Li licensed under the MIT License
// Link to https://github.com/pigpigyyy/Yuescript

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "engine/scripting/muscript/mu_compiler.h"
#include "engine/scripting/muscript/mu_parser.h"

using namespace std::string_literals;

extern "C"
{

#include "libs/lua/host/lauxlib.h"
#include "libs/lua/host/lua.h"
#include "libs/lua/host/lualib.h"

#include "engine/scripting/muscript/muscript.h"

#include "engine/scripting/muscript/stacktraceplus.h"

    static void init_muscript(lua_State *L) {
        if (luaL_loadbuffer(L, muscriptCodes, sizeof(muscriptCodes) / sizeof(muscriptCodes[0]) - 1,
                            "=(muscript)") != 0) {
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
        if (luaL_loadbuffer(L, stpCodes, sizeof(stpCodes) / sizeof(stpCodes[0]) - 1,
                            "=(stacktraceplus)") != 0) {
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
            if (lua_isboolean(L, -1) != 0) { config.reserveLineNumber = lua_toboolean(L, -1) != 0; }
            lua_pop(L, 1);
            lua_pushliteral(L, "space_over_tab");
            lua_gettable(L, -2);
            if (lua_isboolean(L, -1) != 0) { config.useSpaceOverTab = lua_toboolean(L, -1) != 0; }
            lua_pop(L, 1);
            lua_pushliteral(L, "same_module");
            lua_gettable(L, -2);
            if (lua_isboolean(L, -1) != 0) { sameModule = lua_toboolean(L, -1) != 0; }
            lua_pop(L, 1);
            lua_pushliteral(L, "line_offset");
            lua_gettable(L, -2);
            if (lua_isnumber(L, -1) != 0) {
                config.lineOffset = static_cast<int>(lua_tonumber(L, -1));
            }
            lua_pop(L, 1);
            lua_pushliteral(L, "module");
            lua_gettable(L, -2);
            if (lua_isstring(L, -1) != 0) { config.module = lua_tostring(L, -1); }
            lua_pop(L, 1);
            lua_pushliteral(L, "target");
            lua_gettable(L, -2);
            if (lua_isstring(L, -1) != 0) { config.options["target"] = lua_tostring(L, -1); }
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
            for (const auto &var: *result.globals) {
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

    static const luaL_Reg mulib[] = {{"to_lua", mutolua},
                                     {"to_ast", mutoast},
                                     {"version", nullptr},
                                     {"options", nullptr},
                                     {"load_stacktraceplus", nullptr},
                                     {nullptr, nullptr}};

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
