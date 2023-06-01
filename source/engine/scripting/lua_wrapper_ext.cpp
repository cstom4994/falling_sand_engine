
#include "lua_wrapper_ext.hpp"

#include <algorithm>
#include <cstring>
#include <string>

void LuaVector::RegisterMetaTable(lua_State *L) {
    if (luaL_newmetatable(L, LUA_TYPE_NAME)) {
        lua_pushcfunction(L, New);
        lua_setfield(L, 1, "new");

        lua_pushcfunction(L, GarbageCollect);
        lua_setfield(L, 1, "__gc");

        lua_pushcfunction(L, ToString);
        lua_setfield(L, 1, "__tostring");

        lua_pushcfunction(L, Index);
        lua_setfield(L, 1, "__index");

        lua_pushcfunction(L, NewIndex);
        lua_setfield(L, 1, "__newindex");

        lua_pushcfunction(L, Len);
        lua_setfield(L, 1, "__len");

        lua_pushcfunction(L, Append);
        lua_setfield(L, 1, "append");

        lua_pushcfunction(L, Pop);
        lua_setfield(L, 1, "pop");

        lua_pushcfunction(L, Extend);
        lua_setfield(L, 1, "extend");

        lua_pushcfunction(L, Insert);
        lua_setfield(L, 1, "insert");

        lua_pushcfunction(L, Erase);
        lua_setfield(L, 1, "erase");

        lua_pushcfunction(L, Sort);
        lua_setfield(L, 1, "sort");

        lua_setglobal(L, LUA_TYPE_NAME);
    }
}

LuaVector *LuaVector::CheckArg(lua_State *L, int arg) { return static_cast<LuaVector *>(luaL_checkudata(L, arg, LUA_TYPE_NAME)); }

int LuaVector::New(lua_State *L) {
    auto p = static_cast<LuaVector *>(lua_newuserdata(L, sizeof(LuaVector)));
    new (p) LuaVector;
    luaL_setmetatable(L, LUA_TYPE_NAME);
    return 1;
}

int LuaVector::GarbageCollect(lua_State *L) {
    auto p = CheckArg(L, 1);
    if (p) {
        p->~LuaVector();
    }
    return 0;
}

int LuaVector::ToString(lua_State *L) {
    auto p = CheckArg(L, 1);
    if (!p) {
        return 0;
    }

    std::string str = "vector{";
    for (int i = 0; i < p->size(); i++) {
        if (i != 0) {
            str += ", ";
        }
        str += std::to_string(p->at(i));
    }
    str += "}";

    lua_pushstring(L, str.c_str());
    return 1;
}

int LuaVector::Index(lua_State *L) {
    auto p = CheckArg(L, 1);
    if (!p) {
        return 0;
    }

    switch (lua_type(L, 2)) {
        case LUA_TNUMBER: {
            lua_Integer idx = lua_tointeger(L, 2) - 1;  // convert lua indices to c indices
            if (0 <= idx && idx < p->size()) {
                lua_pushnumber(L, p->at(idx));
                return 1;
            }
            lua_pushnil(L);
            return 1;
        }
        case LUA_TSTRING: {
            const char *meta = lua_tostring(L, 2);
            luaL_getmetafield(L, 1, meta);
            return 1;
        }
        default: {
            lua_pushnil(L);
            return 1;
        }
    }
}

int LuaVector::NewIndex(lua_State *L) {
    auto p = CheckArg(L, 1);
    if (!p) {
        return 0;
    }

    lua_Integer idx = luaL_checkinteger(L, 2) - 1;  // convert lua indices to c indices
    lua_Number value = luaL_checknumber(L, 3);

    if (idx < 0) {
        return 0;
    }

    if (idx >= p->size()) {
        p->resize(idx + 1);
    }

    p->at(idx) = static_cast<float>(value);
    return 0;
}

int LuaVector::Len(lua_State *L) {
    auto p = CheckArg(L, 1);
    if (!p) {
        return 0;
    }

    lua_pushinteger(L, static_cast<lua_Integer>(p->size()));
    return 1;
}

int LuaVector::Append(lua_State *L) {
    auto p = CheckArg(L, 1);
    if (!p) {
        return 0;
    }

    lua_Number value = luaL_checknumber(L, 2);
    p->push_back(static_cast<float>(value));

    return 0;
}

int LuaVector::Pop(lua_State *L) {
    auto p = CheckArg(L, 1);
    if (!p) {
        return 0;
    }

    if (p->empty()) {
        lua_pushnil(L);
    } else {
        float back = p->back();
        p->pop_back();
        lua_pushnumber(L, back);
    }

    return 1;
}

int LuaVector::Extend(lua_State *L) {
    auto p = CheckArg(L, 1);
    if (!p) {
        return 0;
    }

    const int numParams = lua_gettop(L);
    p->reserve(p->size() + numParams - 1);
    for (int i = 2; i <= numParams; i++) {
        lua_Number value = luaL_checknumber(L, i);
        p->push_back(static_cast<float>(value));
    }

    return 0;
}

int LuaVector::Insert(lua_State *L) {
    auto p = CheckArg(L, 1);
    if (!p) {
        return 0;
    }

    lua_Integer idx = luaL_checkinteger(L, 2) - 1;  // convert lua indices to c indices
    lua_Number value = luaL_checknumber(L, 3);

    p->insert(std::next(p->begin(), idx), static_cast<float>(value));

    return 0;
}

int LuaVector::Erase(lua_State *L) {
    auto p = CheckArg(L, 1);
    if (!p) {
        return 0;
    }

    lua_Integer idx = luaL_checkinteger(L, 2) - 1;  // convert lua indices to c indices

    p->erase(std::next(p->begin(), idx));

    return 0;
}

int LuaVector::Sort(lua_State *L) {
    auto p = CheckArg(L, 1);
    if (!p) {
        return 0;
    }

    std::sort(p->begin(), p->end());

    return 0;
}

int Test(lua_State *L) {
    LuaVector *v = LuaVector::CheckArg(L, 1);
    v->push_back(1);
    return 0;
}

int test_luavector() {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    LuaVector::RegisterMetaTable(L);

    lua_register(L, "test", Test);

    const char *TEST_CODE = R"LUA(
local a = vector.new()
print(a)
print(type(a))
print(a[1])
a[1] = 3.14
print(a)
print(a[1], a[2])
print(#a)
a[2] = 6.28
print(a)
print(a[1], a[2])
print(#a)
for i, v in ipairs(a) do
    print(i, v)
end
a:append(9.42)
print(a)
a:extend(1, 2, 3)
print(a)
a:insert(2, 0.1234)
print(a)
a:sort()
print(a)
a:erase(2)
print(a)
while #a > 0 do
    local b = a:pop()
    print(a, b)
end
test(a)
print(a)
)LUA";

    if (luaL_dostring(L, TEST_CODE) != LUA_OK) {
        fprintf(stderr, "lua error: %s\n", lua_tostring(L, -1));
    }

    lua_close(L);
    return 0;
}

ME_PRIVATE(void) luaSD_stackdumpvalue(lua_State *state, luaSD_printf luasdorintf, int stackIndex, int depth, int newline, int indentLevel, int performIndent) {
    int t;
    /* ensure that the value is on the top of the stack */
    lua_pushvalue(state, stackIndex);
    t = lua_type(state, -1);
    switch (t) {
        case LUA_TNUMBER:
            lua_pushfstring(state, "%f (%s)", lua_tonumber(state, -1), luaL_typename(state, -1));
            break;
        case LUA_TSTRING:
            lua_pushfstring(state, "%s (%s)", lua_tostring(state, -1), luaL_typename(state, -1));
            break;
        case LUA_TLIGHTUSERDATA:
        case LUA_TUSERDATA: {
            if (lua_getmetatable(state, -1) == 0) {
                lua_pushfstring(state, "%s [%p] (no metatable)", luaL_typename(state, -1), lua_topointer(state, -1));
            } else {
                /* TODO: this else branch was never reached in my tests */
                lua_getfield(state, -1, "__tostring");
                if (lua_isfunction(state, -1)) {
                    if (lua_pcall(state, 0, 1, 0)) {
                        lua_pushfstring(state, "%s [%p]", luaL_typename(state, -1), lua_topointer(state, -1));
                    }
                    lua_pop(state, 1);
                } else {
                    lua_pop(state, 1);
                    luaSD_stackdumpvalue(state, luasdorintf, -1, depth + 1, 1, indentLevel + 4, 1);
                    lua_pop(state, 1);
                    lua_pushfstring(state, "%s [%p]", luaL_typename(state, -1), lua_topointer(state, -1));
                }
                lua_pop(state, 1);
            }
            break;
        }
        case LUA_TBOOLEAN: {
            const int value = lua_toboolean(state, -1);
            if (value) {
                lua_pushliteral(state, "true");
            } else {
                lua_pushliteral(state, "false");
            }
            break;
        }
        case LUA_TNIL:
            lua_pushliteral(state, "nil");
            break;
        case LUA_TFUNCTION: {
#if 0
        /* TODO: somehow get function details. */
        lua_Debug ar;
        lua_getstack(state, -1, &ar);
        if (lua_getinfo(state, "nSlLtuf", &ar) != 0) {
            lua_pushfstring(state, "%s %s %d @%d %s", ar.namewhat, ar.name, ar.nups, ar.linedefined, ar.short_src);
        } else
#endif
            { lua_pushfstring(state, "%s [%p]", luaL_typename(state, -1), lua_topointer(state, -1)); }
            break;
        }
        case LUA_TTABLE: {
            int len = 0;
            if (performIndent) {
                if (depth == 0) {
                    luasdorintf("%-5i | ", stackIndex);
                } else {
                    luasdorintf("       ", stackIndex);
                }
                for (int i = 0; i < indentLevel - 4; ++i) {
                    luasdorintf(" ");
                }
                if (indentLevel >= 4) {
                    luasdorintf("\\-- ");
                }
            }
            lua_pushnil(state);
            while (lua_next(state, -2)) {
                lua_pop(state, 1);
                ++len;
            }

            lua_pushfstring(state, "%s: %p (size: %d)", luaL_typename(state, -1), lua_topointer(state, -1), len);
            luasdorintf("%s", lua_tostring(state, -1));
            lua_pop(state, 1);

            if (depth < luaSD_MAXDEPTH) {
                lua_pushnil(state);
                while (lua_next(state, -2)) {
                    luasdorintf("\n");
                    luaSD_stackdumpvalue(state, luasdorintf, -2, depth + 1, 0, indentLevel + 4, 1);
                    luasdorintf(" = ");
                    luaSD_stackdumpvalue(state, luasdorintf, -1, depth + 1, 0, indentLevel + 4, 0);
                    if (lua_type(state, -1) == LUA_TFUNCTION && lua_type(state, -2) == LUA_TSTRING) {
                        if (!strcmp("__tostring", lua_tostring(state, -2))) {
                            if (lua_pcall(state, 0, 1, 0)) {
                                /* TODO: push object to the stack to call __tostring from */
                                if (lua_isstring(state, -1)) {
                                    luasdorintf(" (%s)", lua_tostring(state, -1));
                                } else {
                                    luasdorintf(" (failed to call __tostring)");
                                }
                            } else {
                                luasdorintf("%s", lua_tostring(state, -1));
                                lua_pop(state, 1);
                            }
                        }
                    }
                    lua_pop(state, 1);
                }
            }
            /* pop the reference copy from the stack to restore the original state */
            lua_pop(state, 1);
            if (newline) {
                luasdorintf("\n");
            }
            return;
        }
        default:
            lua_pushfstring(state, "%s [%p]", luaL_typename(state, -1), lua_topointer(state, -1));
            break;
    }

    {
        int width = 20;
        if (performIndent) {
            if (depth == 0) {
                luasdorintf("%-5i | ", stackIndex);
            } else {
                luasdorintf("       ", stackIndex);
            }
            for (int i = 0; i < indentLevel - 4; ++i) {
                luasdorintf(" ");
            }
            if (indentLevel >= 4) {
                luasdorintf("\\-- ");
            }
            width -= indentLevel;
        }
        luasdorintf("%-*s", width, lua_tostring(state, -1));
    }
    if (newline) {
        luasdorintf("\n");
    }
    /* pop the string and the reference copy from the stack to restore the original state */
    lua_pop(state, 2);
}

void luaSD_stackdump(lua_State *state, luaSD_printf printf_fn) {
    const int top = lua_gettop(state);
    printf_fn("\n--------------------start-of-stacktrace----------------\n");
    printf_fn("index | details (%i entries)\n", top);
    int i;
    for (i = -1; i >= -top; --i) {
        luaSD_stackdumpvalue(state, printf_fn, i, 0, 1, 0, 1);
    }
    printf_fn("----------------------end-of-stacktrace----------------\n\n");
}

void luaSD_stackdump_default(lua_State *state) { luaSD_stackdump(state, printf); }

#pragma region LBIND

#if LUA_VERSION_NUM < 503
LUA_API int lua53_getuservalue(lua_State *L, int idx) {
    lua_getuservalue(L, idx);
    return lua_type(L, -1);
}
LUA_API int lua53_gettable(lua_State *L, int idx) {
    lua_gettable(L, idx);
    return lua_type(L, -1);
}
LUA_API int lua53_getfield(lua_State *L, int idx, const char *field) {
    lua_getfield(L, idx, field);
    return lua_type(L, -1);
}
LUA_API int lua53_rawget(lua_State *L, int idx) {
    lua_rawget(L, idx);
    return lua_type(L, -1);
}
LUA_API int lua53_rawgeti(lua_State *L, int idx, lua_Integer n) {
    lua_rawgeti(L, idx, n);
    return lua_type(L, -1);
}
LUA_API int lua53_rawgetp(lua_State *L, int idx, const void *p) {
    lua_rawgetp(L, idx, p);
    return lua_type(L, -1);
}

LUA_API void lua53_rotate(lua_State *L, int idx, int n) {
    int i;
    if (n < 0) n += (idx < 0) ? -idx : (lua_gettop(L) - idx - 1);
    for (i = 0; i < n; ++i) lua_insert(L, -idx);
}
#endif

/* compatible apis */
#if LUA_VERSION_NUM < 502
LUA_API lua_Integer lua_tointegerx(lua_State *L, int idx, int *valid) {
    lua_Integer n;
    *valid = (n = lua_tointeger(L, idx)) != 0 || lua_isnumber(L, idx);
    return n;
}

LUA_API void lua_rawgetp(lua_State *L, int idx, const void *p) {
    lua_pushlightuserdata(L, (void *)p);
    lua_rawget(L, lbind_relindex(idx, 1));
}

LUA_API void lua_rawsetp(lua_State *L, int idx, const void *p) {
    lua_pushlightuserdata(L, (void *)p);
    lua_insert(L, -2);
    lua_rawset(L, lbind_relindex(idx, 1));
}

LUALIB_API void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup) {
    luaL_checkstack(L, nup, "too many upvalues");
    for (; l->name != NULL; l++) { /* fill the table with given functions */
        int i;
        for (i = 0; i < nup; i++) /* copy upvalues to the top */
            lua_pushvalue(L, -nup);
        lua_pushcclosure(L, l->func, nup); /* closure with those upvalues */
        lua_setfield(L, -(nup + 2), l->name);
    }
    lua_pop(L, nup); /* remove upvalues */
}

LUALIB_API const char *luaL_tolstring(lua_State *L, int idx, size_t *plen) {
    if (!luaL_callmeta(L, idx, "__tostring")) { /* no metafield? */
        switch (lua_type(L, idx)) {
            case LUA_TNUMBER:
            case LUA_TSTRING:
                lua_pushvalue(L, idx);
                break;
            case LUA_TBOOLEAN:
                lua_pushstring(L, (lua_toboolean(L, idx) ? "true" : "false"));
                break;
            case LUA_TNIL:
                lua_pushliteral(L, "nil");
                break;
            default:
                lua_pushfstring(L, "%s: %p", luaL_typename(L, idx), lua_topointer(L, idx));
                break;
        }
    }
    return lua_tolstring(L, -1, plen);
}

/* LuaJIT has its own luaL_traceback(),
 * so we do not export this, use static instead.  */
#ifdef LUA_BITSINT /* not LuaJIT */
#define LEVELS1 12 /* size of the first part of the stack */
#define LEVELS2 10 /* size of the second part of the stack */
static void luaL_traceback(lua_State *L, lua_State *L1, const char *msg, int level) {
    int top = lua_gettop(L);
    int firstpart = 1; /* still before eventual `...' */
    lua_Debug ar;
    if (msg) lua_pushfstring(L, "%s\n", msg);
    lua_pushliteral(L, "stack traceback:");
    while (lua_getstack(L1, level++, &ar)) {
        if (level > LEVELS1 && firstpart) {
            /* no more than `LEVELS2' more levels? */
            if (!lua_getstack(L1, level + LEVELS2, &ar))
                level--; /* keep going */
            else {
                lua_pushliteral(L, "\n\t...");                 /* too many levels */
                while (lua_getstack(L1, level + LEVELS2, &ar)) /* find last levels */
                    level++;
            }
            firstpart = 0;
            continue;
        }
        lua_pushliteral(L, "\n\t");
        lua_getinfo(L1, "Snl", &ar);
        lua_pushfstring(L, "%s:", ar.short_src);
        if (ar.currentline > 0) lua_pushfstring(L, "%d:", ar.currentline);
        if (*ar.namewhat != '\0') /* is there a name? */
            lua_pushfstring(L, " in function " LUA_QS, ar.name);
        else {
            if (*ar.what == 'm') /* main? */
                lua_pushfstring(L, " in main chunk");
            else if (*ar.what == 'C' || *ar.what == 't')
                lua_pushliteral(L, " ?"); /* C function or tail call */
            else
                lua_pushfstring(L, " in function <%s:%d>", ar.short_src, ar.linedefined);
        }
        lua_concat(L, lua_gettop(L) - top);
    }
    lua_concat(L, lua_gettop(L) - top);
}
#endif /* LUA_BITSINT */

#endif /* LUA_VERSION_NUM < 502 */

/* lbind information hash routine */

#define LBIND_PTRBOX 0x90127B07
#define LBIND_TYPEBOX 0x799E0B07
#define LBIND_UDBOX 0xC5E7DB07

static int lbB_retrieve(lua_State *L, unsigned id) {
    if (lua53_rawgetp(L, LUA_REGISTRYINDEX, (void *)(ptrdiff_t)id) == LUA_TNIL) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_rawsetp(L, LUA_REGISTRYINDEX, (void *)(ptrdiff_t)id);
        return 1;
    }
    return 0;
}

static void lbB_internbox(lua_State *L) {
    if (lbB_retrieve(L, LBIND_PTRBOX)) {
        lua_pushliteral(L, "v");
        lbind_setmetafield(L, -2, "__mode");
    }
}

static void lbB_typebox(lua_State *L) { lbB_retrieve(L, LBIND_TYPEBOX); }

/* light userdata utils */

int lbind_getudtypebox(lua_State *L) { return lbB_retrieve(L, LBIND_UDBOX); }

void lbind_setlightuservalue(lua_State *L, const void *p) {
    lbind_getudtypebox(L);
    lua_pushvalue(L, -2);
    lua_rawsetp(L, -2, p);
    lua_pop(L, 1);
}

int lbind_getlightuservalue(lua_State *L, const void *p) {
    lbind_getudtypebox(L);
    if (lua53_rawgetp(L, -1, p) == LUA_TNIL) {
        lua_pop(L, 2);
        return 0;
    }
    lua_remove(L, 2);
    return 1;
}

/* lbind class register */

void lbind_install(lua_State *L, lbind_Reg *libs) {
    lua_getfield(L, LUA_REGISTRYINDEX, "_PRELOAD"); /* 1 */
    if (libs != NULL) {
        for (; libs->name != NULL; ++libs) {
            lua_pushstring(L, libs->name);         /* 2 */
            lua_pushcfunction(L, libs->open_func); /* 3 */
            lua_rawset(L, -3);                     /* 2,3->1 */
        }
    }
#ifndef LBIND_NO_RUNTIME
    lua_pushstring(L, "lbind");          /* 2 */
    lua_pushcfunction(L, luaopen_lbind); /* 3 */
    lua_rawset(L, -3);                   /* 2,3->1 */
#endif
    lua_pop(L, 1); /* (1) */
}

int lbind_requiref(lua_State *L, const char *name, lua_CFunction loader) {
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED"); /* 1 */
    if (lua53_getfield(L, -1, name) != LUA_TNIL) { /* 2 */
        lua_remove(L, -2);                         /* (1) */
        return 0;
    }
    lua_pop(L, 1);
    lua_pushstring(L, name);      /* 2 */
    lua_pushcfunction(L, loader); /* 3 */
    lua_pushvalue(L, -2);         /* 2->4 */
    lua_call(L, 1, 1);            /* 3,4->3 */
    lua_pushvalue(L, -1);         /* 3->4 */
    lua_insert(L, -4);            /* 4->1 */
    /* stack: lib _LOADED name lib */
    lua_rawset(L, -3); /* 3,4->2 */
    lua_pop(L, 1);     /* (2) */
    return 1;
}

void lbind_requirelibs(lua_State *L, lbind_Reg *libs) {
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED"); /* 1 */
    for (; libs->name != NULL; ++libs) {
        lua_pushstring(L, libs->name);         /* 2 */
        lua_pushvalue(L, -1);                  /* 3 */
        if (lua53_rawget(L, -3) != LUA_TNIL) { /* 3->3 */
            lua_pop(L, 2);
            continue;
        }
        lua_pop(L, 1);
        lua_pushcfunction(L, libs->open_func); /* 3 */
        lua_pushvalue(L, -2);                  /* 2->4 */
        lua_call(L, 1, 1);                     /* 3,4->3 */
        lua_rawset(L, -5);                     /* 2,3->1 */
    }
    lua_pop(L, 1);
}

void lbind_requireinto(lua_State *L, const char *prefix, lbind_Reg *libs) {
    /* stack: table */
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED"); /* 1 */
    for (; libs->name != NULL; ++libs) {
        lua_pushstring(L, libs->name); /* 2 */
        if (prefix == NULL)
            lua_pushvalue(L, -1); /* 3 */
        else
            lua_pushfstring(L, "%s.%s", prefix, libs->name); /* 3 */
        lua_pushvalue(L, -1);                                /* 4 */
        if (lua53_rawget(L, -4) == LUA_TNIL) {               /* 4->4 */
            lua_pop(L, 1);                                   /* (4) */
            lua_pushcfunction(L, libs->open_func);           /* 4 */
            lua_pushvalue(L, -2);                            /* 3->5 */
            lua_call(L, 1, 1);                               /* 4,5->4 */
            lua_pushvalue(L, -2);                            /* 3->5 */
            lua_pushvalue(L, -2);                            /* 4->6 */
            /* stack: table [_LOADED name prefix.name ret prefix.name ret] */
            lua_rawset(L, -6); /* 4,5->1 */
        }
        lua_remove(L, -2); /* (3) */
        lua_rawset(L, -4); /* 2,3->table */
    }
    lua_pop(L, 1);
}

/* lbind utils functions */

static int lbL_traceback(lua_State *L) {
    const char *msg = lua_tostring(L, 1);
    if (msg)
        luaL_traceback(L, L, msg, 1);
    else if (!lua_isnoneornil(L, 1)) {          /* is there an error object? */
        if (!luaL_callmeta(L, 1, "__tostring")) /* try its 'tostring' metamethod */
            lua_pushliteral(L, "(no error message)");
    }
    return 1;
}

int lbind_relindex(int idx, int onstack) { return (idx > 0 || idx <= LUA_REGISTRYINDEX) ? idx : idx - onstack; }

int lbind_argferror(lua_State *L, int idx, const char *fmt, ...) {
    const char *errmsg;
    va_list argp;
    va_start(argp, fmt);
    errmsg = lua_pushvfstring(L, fmt, argp);
    va_end(argp);
    return luaL_argerror(L, idx, errmsg);
}

int lbind_typeerror(lua_State *L, int idx, const char *tname) {
    const char *real_type = lbind_type(L, idx);
    return lbind_argferror(L, idx, "%s expected, got %s", tname, real_type != NULL ? real_type : luaL_typename(L, idx));
}

int lbind_matcherror(lua_State *L, const char *extramsg) {
    lua_Debug ar;
    lua_getinfo(L, "n", &ar);
    if (ar.name == NULL) ar.name = "?";
    return luaL_error(L,
                      "no matching functions for call to %s\n"
                      "candidates are:\n%s",
                      ar.name, extramsg);
}

int lbind_copystack(lua_State *from, lua_State *to, int n) {
    int i;
    luaL_checkstack(from, n, "too many args");
    for (i = 0; i < n; ++i) lua_pushvalue(from, -n);
    lua_xmove(from, to, n);
    return n;
}

const char *lbind_dumpstack(lua_State *L, const char *msg) {
    int i, top = lua_gettop(L);
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    luaL_addstring(&b, "dump stack: ");
    luaL_addstring(&b, msg != NULL ? msg : "");
    luaL_addstring(&b, "\n---------------------------\n");
    for (i = 1; i <= top; ++i) {
        lua_pushfstring(L, "%d: ", i);
        luaL_addvalue(&b);
        lbind_tolstring(L, i, NULL);
        luaL_addvalue(&b);
        luaL_addstring(&b, "\n");
    }
    luaL_addstring(&b, "---------------------------\n");
    luaL_pushresult(&b);
    return lua_tostring(L, -1);
}

int lbind_hasfield(lua_State *L, int idx, const char *field) {
    int hasfield = lua53_getfield(L, idx, field) != LUA_TNIL;
    lua_pop(L, 1);
    return hasfield;
}

int lbind_self(lua_State *L, const void *p, const char *method, int nargs, int *ptraceback) {
    luaL_checkstack(L, nargs + 3, "too many arguments to self call");
    if (!lbind_retrieve(L, p)) return 0;             /* 1 */
    if (lua53_getfield(L, -1, method) == LUA_TNIL) { /* 2 */
        lua_pop(L, 2);
        return 0;
    }
    if (ptraceback) {
        lua_pushcfunction(L, lbL_traceback);
        lua_insert(L, -3);
        *ptraceback = lua_gettop(L) - 3;
    }
    lua_insert(L, -2);
    /* stack: traceback method object */
    return 1;
}

int lbind_pcall(lua_State *L, int nargs, int nrets) {
    int res, tb_idx;
    lua_pushcfunction(L, lbL_traceback);
    lua_insert(L, -nargs - 2);
    tb_idx = lua_gettop(L) - nargs - 1;
    res = lua_pcall(L, nargs, nrets, tb_idx);
    lua_remove(L, tb_idx);
    return res;
}

/* metatable utils */

static int lbL_libcall(lua_State *L) {
    lua_pushvalue(L, lua_upvalueindex(1));
    if (lua53_rawget(L, 1) == LUA_TNIL) {
        lua_pushfstring(L, "no such method (%s)", lua_tostring(L, lua_upvalueindex(1)));
        return luaL_argerror(L, 1, lua_tostring(L, -1));
    }
    lua_replace(L, 1);
    lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
    return lua_gettop(L);
}

static int lbM_callacc(lua_State *L, int idx, int nargs) {
    lua_CFunction f = lua_tocfunction(L, idx);
    if (f != NULL) {
        lua_settop(L, nargs);
        return f(L);
    }
    return -1;
}

static int lbM_calllut(lua_State *L, int idx, int nargs) {
    lua_CFunction f = lua_tocfunction(L, idx);
    /* look up table */
    if (f == NULL) {
        lua_pushvalue(L, 2);
        lua_rawget(L, lbind_relindex(idx, 1));
        f = lua_tocfunction(L, -1);
    }
    if (f != NULL) {
        lua_settop(L, nargs);
        return f(L);
    }
    return -1;
}

static int lbL_newindex(lua_State *L) {
    int nret;
    /* upvalue: seti, seth
     * order:
     *  - lut
     *  - accessor
     *  - normaltable
     *  - uservalue
     */
    if (!lua_isnone(L, lua_upvalueindex(1)) && (nret = lbM_calllut(L, lua_upvalueindex(1), 3)) >= 0) return nret;
    if (!lua_isnone(L, lua_upvalueindex(2)) && (nret = lbM_callacc(L, lua_upvalueindex(2), 3)) >= 0) return nret;
    if (!lua_isuserdata(L, 1)) {
        lua_settop(L, 3);
        lua_rawset(L, 1);
        return 0;
    }
    if (lua53_getuservalue(L, 1) == LUA_TNIL) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setuservalue(L, 1);
    }
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, -3);
    return 0;
}

static int lbL_index(lua_State *L) {
    int i, nret;
    /* upvalue: geti, geth, tables
     * order:
     *  - uservalue
     *  - metatable
     *  - lut
     *  - accessor
     *  - upvalue tables
     */
    if (lua_isuserdata(L, 1)) {
        if (lua53_getuservalue(L, 1) != LUA_TNIL) {
            lua_pushvalue(L, 2);
            if (lua53_rawget(L, -2) != LUA_TNIL) return 1;
        }
    }
    if (lua_getmetatable(L, 1)) {
        lua_pushvalue(L, 2);
        if (lua53_rawget(L, -2) != LUA_TNIL) return 1;
    }
    if (!lua_isnone(L, lua_upvalueindex(1)) && (nret = lbM_calllut(L, lua_upvalueindex(1), 2)) >= 0) return nret;
    if (!lua_isnone(L, lua_upvalueindex(2)) && (nret = lbM_callacc(L, lua_upvalueindex(2), 2)) >= 0) return nret;
    /* find in libtable/superlibtable */
    for (i = 3; !lua_isnone(L, lua_upvalueindex(i)); ++i) {
        lua_settop(L, 2);
        if (lua_islightuserdata(L, lua_upvalueindex(i))) {
            if (!lbind_getmetatable(L, lua_touserdata(L, lua_upvalueindex(i)))) continue;
            lua_replace(L, lua_upvalueindex(i));
        }
        lua_pushvalue(L, 2);
        if (lua53_gettable(L, lua_upvalueindex(i)) != LUA_TNIL) return 1;
    }
    return 0;
}

static void lbM_newindex(lua_State *L) {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushcclosure(L, lbL_newindex, 2);
}

static void lbM_index(lua_State *L, int ntables) {
    lua_pushnil(L);
    lua_pushnil(L);
    if (ntables != 0) lua53_rotate(L, -ntables - 2, 2);
    lua_pushcclosure(L, lbL_index, ntables + 2);
}

static void get_default_metafield(lua_State *L, int idx, int field) {
    if (field == LBIND_INDEX) {
        if (lua53_getfield(L, idx, "__index") == LUA_TNIL || lua_tocfunction(L, -1) != lbL_index) {
            lua_pop(L, 1);
            lbM_index(L, 0);
            lua_pushvalue(L, -1);
            lua_setfield(L, lbind_relindex(idx, 2), "__index");
        }
    } else if (field == LBIND_NEWINDEX) {
        if (lua53_getfield(L, idx, "__newindex") == LUA_TNIL || lua_tocfunction(L, -1) != lbL_newindex) {
            lua_pop(L, 1);
            lbM_newindex(L);
            lua_pushvalue(L, -1);
            lua_setfield(L, lbind_relindex(idx, 2), "__newindex");
        }
    }
}

static void set_cfuncupvalue(lua_State *L, lua_CFunction f, int field, int idx) {
    if ((field & LBIND_INDEX) != 0) {
        get_default_metafield(L, -1, LBIND_INDEX);
        lua_pushcfunction(L, f);
        lua_setupvalue(L, -2, idx);
        lua_pop(L, 1);
    }
    if ((field & LBIND_NEWINDEX) != 0) {
        get_default_metafield(L, -1, LBIND_NEWINDEX);
        lua_pushcfunction(L, f);
        lua_setupvalue(L, -2, idx);
        lua_pop(L, 1);
    }
}

int lbind_setmetatable(lua_State *L, const void *t) {
    if (lbind_getmetatable(L, t)) {
        lua_setmetatable(L, -2);
        return 1;
    }
    return 0;
}

int lbind_getmetatable(lua_State *L, const void *t) {
    if (lua53_rawgetp(L, LUA_REGISTRYINDEX, t) == LUA_TNIL) {
        lua_pop(L, 1);
        return 0;
    }
    return 1;
}

int lbind_setmetafield(lua_State *L, int idx, const char *field) {
    if (!lua_getmetatable(L, idx)) {
        lua_createtable(L, 0, 1);
        lua_pushvalue(L, -2);
        lua_setfield(L, -2, field);
        lua_setmetatable(L, lbind_relindex(idx, 1));
        lua_pop(L, 1);
        return 1;
    }
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, field);
    lua_pop(L, 2);
    return 0;
}

int lbind_setlibcall(lua_State *L, const char *method) {
    if (method == NULL) method = "new";
    lua_pushstring(L, method);
    lua_pushcclosure(L, lbL_libcall, 1);
    return lbind_setmetafield(L, -2, "__call");
}

void lbind_setaccessors(lua_State *L, int ntables, int field) {
    if ((field & LBIND_INDEX) != 0) {
        lua_pushnil(L);
        lua_pushnil(L);
        if (ntables > 0) lua53_rotate(L, -ntables - 2, 2);
        lua_pushcclosure(L, lbL_index, ntables + 2);
        lua_setfield(L, -2, "__index");
    }
    if ((field & LBIND_NEWINDEX) != 0) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushcclosure(L, lbL_newindex, 2);
        lua_setfield(L, -2, "__newindex");
    }
}

void lbind_sethashf(lua_State *L, lua_CFunction f, int field) { set_cfuncupvalue(L, f, field, 1); }

void lbind_setarrayf(lua_State *L, lua_CFunction f, int field) { set_cfuncupvalue(L, f, field, 2); }

void lbind_setmaptable(lua_State *L, luaL_Reg libs[], int field) {
    lua_newtable(L);
    luaL_setfuncs(L, libs, 0);
    if ((field & LBIND_INDEX) != 0) {
        get_default_metafield(L, -2, LBIND_INDEX);
        lua_pushvalue(L, -2);
        lua_setupvalue(L, -2, 1);
        lua_pop(L, 1);
    }
    if ((field & LBIND_NEWINDEX) != 0) {
        get_default_metafield(L, -2, LBIND_NEWINDEX);
        lua_pushvalue(L, -2);
        lua_setupvalue(L, -2, 1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

/* lbind userdata maintain */

typedef union {
    lbind_MaxAlign dummy; /* ensures maximum alignment for `intern' object */
    struct {
        void *instance;
        int flags;
    } o;
} lbind_Object;

#define check_size(L, n) (lua_rawlen((L), (n)) >= sizeof(lbind_Object))

static lbind_Object *lbO_new(lua_State *L, size_t objsize, int flags) {
    lbind_Object *obj;
    obj = (lbind_Object *)lua_newuserdata(L, sizeof(lbind_Object) + objsize);
    obj->o.flags = flags;
    obj->o.instance = (void *)(obj + 1);
    if (objsize != 0 && (flags & LBIND_INTERN) != 0) lbind_intern(L, obj->o.instance);
    return obj;
}

static lbind_Object *lbO_test(lua_State *L, int idx) {
    lbind_Object *obj = (lbind_Object *)lbind_touserdata(L, idx);
    if (obj != NULL) {
        if (!check_size(L, idx) || obj->o.instance == NULL) obj = NULL;
#if 0
    else {
      lbB_internbox(L); /* 1 */
      lua_rawgetp(L, -1, obj->o.instance); /* 2 */
      if (!lua_rawequal(L, lbind_relindex(idx, 2), -1))
        obj = NULL;
      lua_pop(L, 2); /* (2)(1) */
    }
#endif
    }
    return obj;
}

void *lbind_touserdata(lua_State *L, int idx) {
#ifndef LBIND_NO_PEER
    if (lua_istable(L, idx)) {
        if (lua53_getfield(L, idx, "__peer") == LUA_TNIL) {
            lua_pop(L, 1);
            return NULL;
        }
        lua_replace(L, idx);
    }
#endif
    return lua_touserdata(L, idx);
}

void *lbind_raw(lua_State *L, size_t objsize, int intern) { return lbO_new(L, objsize, intern ? LBIND_INTERN : 0)->o.instance; }

void *lbind_new(lua_State *L, size_t objsize, const lbind_Type *t) {
    lbind_Object *obj = lbO_new(L, objsize, t->flags);
    if (lbind_getmetatable(L, t)) lua_setmetatable(L, -2);
    return obj->o.instance;
}

void *lbind_wrap(lua_State *L, void *p, const lbind_Type *t) {
    lbind_Object *obj = lbO_new(L, 0, t->flags);
    obj->o.instance = p;
    if ((obj->o.flags & LBIND_INTERN) != 0) lbind_intern(L, p);
    if (lbind_getmetatable(L, t)) lua_setmetatable(L, -2);
    return p;
}

void *lbind_delete(lua_State *L, int idx) {
    void *u = NULL;
    lbind_Object *obj = (lbind_Object *)lbind_touserdata(L, idx);
    if (obj != NULL) {
        if (!check_size(L, idx)) return NULL;
        if ((u = obj->o.instance) != NULL) {
            obj->o.instance = NULL;
            obj->o.flags &= ~LBIND_TRACK;
#if LUA_VERSION_NUM < 502
            lbB_internbox(L);      /* 1 */
            lua_pushnil(L);        /* 2 */
            lua_rawsetp(L, -3, u); /* 2->1 */
            lua_pop(L, 1);         /* (1) */
#endif
        }
    }
    return u;
}

void *lbind_object(lua_State *L, int idx) {
    lbind_Object *obj = lbO_test(L, idx);
    return obj == NULL ? NULL : obj->o.instance;
}

void lbind_intern(lua_State *L, const void *p) {
    /* stack: object */
    lbB_internbox(L);
    lua_pushvalue(L, -2);
    lua_rawsetp(L, -2, p);
    lua_pop(L, 1);
}

int lbind_retrieve(lua_State *L, const void *p) {
    if (p == NULL) return 0;
    lbB_internbox(L);                          /* 1 */
    if (lua53_rawgetp(L, -1, p) == LUA_TNIL) { /* 2 */
        lua_pop(L, 2);
        return 0;
    }
    lua_remove(L, -2);
    return 1;
}

void lbind_track(lua_State *L, int idx) {
    lbind_Object *obj = lbO_test(L, idx);
    if (obj != NULL) obj->o.flags |= LBIND_TRACK;
}

void lbind_untrack(lua_State *L, int idx) {
    lbind_Object *obj = lbO_test(L, idx);
    if (obj != NULL) obj->o.flags &= ~LBIND_TRACK;
}

int lbind_hastrack(lua_State *L, int idx) {
    lbind_Object *obj = lbO_test(L, idx);
    return obj != NULL && (obj->o.flags & LBIND_TRACK) != 0;
}

/* lbind type registry */

void lbind_inittype(lbind_Type *t, const char *name) {
    t->name = name;
    t->flags = LBIND_DEFAULT_FLAG;
    t->cast = NULL;
    t->bases = NULL;
}

void lbind_setbase(lbind_Type *t, lbind_Type **bases, lbind_Cast *cast) {
    t->bases = bases;
    t->cast = cast;
    if (bases != NULL) t->flags &= LBIND_ACCESSOR;
}

int lbind_settrack(lbind_Type *t, int autotrack) {
    int old_flag = t->flags & LBIND_TRACK ? 1 : 0;
    if (autotrack)
        t->flags |= LBIND_TRACK;
    else
        t->flags &= ~LBIND_TRACK;
    return old_flag;
}

int lbind_setintern(lbind_Type *t, int autointern) {
    int old_flag = t->flags & LBIND_INTERN ? 1 : 0;
    if (autointern)
        t->flags |= LBIND_INTERN;
    else
        t->flags &= ~LBIND_INTERN;
    return old_flag;
}

lbind_Type *lbind_typeobject(lua_State *L, int idx) {
    lbind_Type *t = NULL;
    if (lua_getmetatable(L, idx)) {
        lua_getfield(L, -1, "__type");
        t = (lbind_Type *)lua_touserdata(L, -1);
        lua_pop(L, 2);
        if (t != NULL) return t;
    }
    if (lua_istable(L, idx)) {
        lua_getfield(L, idx, "__type");
        t = (lbind_Type *)lua_touserdata(L, -1);
        lua_pop(L, 1);
    }
    return t;
}

/* lbind type metatable */

static int lbL_tostring(lua_State *L) {
    lbind_tolstring(L, 1, NULL);
    return 1;
}

static int lbL_gc(lua_State *L) {
    lbind_Object *obj = (lbind_Object *)lua_touserdata(L, 1);
    if (obj != NULL && check_size(L, 1)) {
        if ((obj->o.flags & LBIND_TRACK) != 0) {
            if (lua53_getfield(L, 1, "delete") != LUA_TNIL) {
                lua_pushvalue(L, 1);
                lua_call(L, 1, 0);
            }
            if ((obj->o.flags & LBIND_TRACK) != 0) lbind_delete(L, 1);
        }
    }
    return 0;
}

static void lbT_register(lua_State *L, const char *name, const void *t) {
    /* stack: metatable */
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, name);
    lua_pushvalue(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, t);

    lbB_typebox(L);
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, name);
    lua_pushvalue(L, -2);
    lua_rawsetp(L, -2, t);
    lua_pop(L, 1);
}

static int lbT_exists(lua_State *L, const lbind_Type *t) {
    if (lua53_rawgetp(L, LUA_REGISTRYINDEX, (const void *)t) != LUA_TNIL) {
        lua_pop(L, 1);
        return 1;
    }

    if (lua53_getfield(L, LUA_REGISTRYINDEX, t->name) != LUA_TNIL) {
        lua_pop(L, 1);
        return 1;
    }

    lua_pop(L, 2);
    return 0;
}

static int lbL_agency(lua_State *L) {
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_gettable(L, 1);
    lua_insert(L, 1);
    lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
    return lua_gettop(L);
}

static void lbT_setagency(lua_State *L, const char *key) {
    /* stack: libtable mt */
    lua_pushstring(L, key);
    if (lua53_rawget(L, -3) != LUA_TNIL) { /* do not track __index */
        lua_pushfstring(L, "__%s", key);
        lua_pushstring(L, key);
        lua_pushcclosure(L, lbL_agency, 1);
        lua_rawset(L, -4);
    }
    lua_pop(L, 1);
}

int lbind_newmetatable(lua_State *L, luaL_Reg *libs, const lbind_Type *t) {
    if (lbT_exists(L, t)) return 0;

    lua_createtable(L, 0, 8);
    if (libs != NULL) luaL_setfuncs(L, libs, 0);

    /* init type metatable */
    lua_pushlightuserdata(L, (void *)t);
    lua_setfield(L, -2, "__type");

    if (!lbind_hasfield(L, -1, "__gc")) {
        lua_pushcfunction(L, lbL_gc);
        lua_setfield(L, -2, "__gc");
    }

    if (!lbind_hasfield(L, -1, "__tostring")) {
        lua_pushcfunction(L, lbL_tostring);
        lua_setfield(L, -2, "__tostring");
    }

    if ((t->flags & LBIND_ACCESSOR) != 0) {
        int nups = 0;
        int freeslots = 0;
        lbind_Type **bases = t->bases;
        if (bases != NULL) {
            for (; *bases != NULL; ++nups, ++bases) {
                if (nups > freeslots) {
                    luaL_checkstack(L, 10, "no space for base types");
                    freeslots += 10;
                }
                if (!lbind_getmetatable(L, *bases)) lua_pushlightuserdata(L, *bases);
            }
        }
        lbind_setaccessors(L, nups, LBIND_INDEX | LBIND_NEWINDEX);
    }

    else if (!lbind_hasfield(L, -1, "__index")) {
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
    }

    lbT_register(L, t->name, (const void *)t);
    return 1;
}

void lbind_setagency(lua_State *L) {
    lbT_setagency(L, "len");
#if LUA_VERSION_NUM >= 502
    lbT_setagency(L, "pairs");
    lbT_setagency(L, "ipairs");
#endif /* LUA_VERSION_NUM >= 502 */
}

/* lbind type system */

static int lbT_testmeta(lua_State *L, int idx, const lbind_Type *t) {
    if (lua_getmetatable(L, idx)) { /* does it have a metatable? */
        int res = 1;
        if (!lbind_getmetatable(L, t)) { /* get correct metatable */
            lua_pop(L, 1);               /* no such metatable? fail */
            return 0;
        }
        if (!lua_rawequal(L, -1, -2)) /* not the same? */
            res = 0;                  /* value is a userdata with wrong metatable */
        lua_pop(L, 2);
        return res;
    }
    return 0;
}

static void *lbT_trycast(lua_State *L, int idx, const lbind_Type *t) {
    lbind_Type *from_type = lbind_typeobject(L, idx);
    void *obj = NULL;
    if (from_type != NULL && from_type->cast != NULL && (obj = from_type->cast(L, idx, t)) != NULL) return obj;
    if (t->cast != NULL && (obj = t->cast(L, idx, from_type)) != NULL) return obj;
    return NULL;
}

const char *lbind_tolstring(lua_State *L, int idx, size_t *plen) {
    const char *tname = lbind_type(L, idx);
    lbind_Object *obj = lbO_test(L, idx);
    if (obj != NULL && tname)
        lua_pushfstring(L, "%s: %p", tname, obj->o.instance);
    else if (obj == NULL) {
        lbind_Object *obj = (lbind_Object *)lbind_touserdata(L, idx);
        if (obj == NULL) return luaL_tolstring(L, idx, plen);
        if (tname && check_size(L, idx))
            lua_pushfstring(L, "%s[N]: %p", tname, obj->o.instance);
        else
            lua_pushfstring(L, "userdata: %p", (void *)obj);
    }
    return lua_tolstring(L, -1, plen);
}

const char *lbind_type(lua_State *L, int idx) {
    lbind_Type *t = lbind_typeobject(L, idx);
    if (t != NULL) return t->name;
    return NULL;
}

int lbind_isa(lua_State *L, int idx, const lbind_Type *t) { return lbT_testmeta(L, idx, t) || lbT_trycast(L, idx, t) != NULL; }

void *lbind_cast(lua_State *L, int idx, const lbind_Type *t) {
    lbind_Object *obj = (lbind_Object *)lbind_touserdata(L, idx);
    if (!check_size(L, idx) || obj == NULL || obj->o.instance == NULL) return NULL;
    return lbT_testmeta(L, idx, t) ? obj->o.instance : lbT_trycast(L, idx, t);
}

int lbind_copy(lua_State *L, const void *obj, const lbind_Type *t) {
    if (!lbind_getmetatable(L, t)) /* 1 */
        return 0;
    lua_pushliteral(L, "new");             /* 2 */
    if (lua53_rawget(L, -2) == LUA_TNIL) { /* 2->2 */
        lua_pop(L, 2);                     /* (2)(1) */
        return 0;
    }
    lua_remove(L, -2); /* (1) */
    if (!lbind_retrieve(L, obj)) lbind_wrap(L, (void *)obj, t);
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        lua_pop(L, 1);
        return 0;
    }
    lbind_track(L, -1); /* enable autodeletion for copied stuff */
    return 1;
}

void *lbind_check(lua_State *L, int idx, const lbind_Type *t) {
    lbind_Object *obj = (lbind_Object *)lbind_touserdata(L, idx);
    void *u = NULL;
    if (!check_size(L, idx)) luaL_argerror(L, idx, "invalid lbind userdata");
    if (obj == NULL || obj->o.instance == NULL) {
        luaL_argerror(L, idx, "null lbind object");
        return NULL;
    }
    u = lbT_testmeta(L, idx, t) ? obj->o.instance : lbT_trycast(L, idx, t);
    if (u == NULL) lbind_typeerror(L, idx, t->name);
    return u;
}

void *lbind_test(lua_State *L, int idx, const lbind_Type *t) {
    lbind_Object *obj = (lbind_Object *)lbind_touserdata(L, idx);
    return lbT_testmeta(L, idx, t) ? obj->o.instance : lbT_trycast(L, idx, t);
}

/* lbind enum/mask support */
#ifndef LBIND_NO_ENUM
static const char *lbE_skipwhite(const char *s) {
    while (*s == '\t' || *s == '\n' || *s == '\r' || *s == ' ' || *s == '+' || *s == '|' || *s == ',') ++s;
    return s;
}

static const char *lbE_skipident(const char *s) {
    while ((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') || (*s == '-' || *s == '_')) ++s;
    return s;
}

static int lbE_parsemask(lbind_Enum *et, const char *s, int *penum, lua_State *L) {
    *penum = 0;
    while (*s != '\0') {
        const char *e;
        int inversion = 0;
        lbind_EnumItem *item;
        s = lbE_skipwhite(s);
        if (*s == '~') {
            inversion = 1;
            s = lbE_skipwhite(s + 1);
        }
        if (*s == '\0') break;
        e = lbE_skipident(s);
        if (e == s || (item = lbind_findenum(et, s, e - s)) == NULL) {
            if (L == NULL) return 0;
            if (e == s)
                return luaL_error(L, "unexpected token '%c' in %s", *s, et->name);
            else {
                lua_pushlstring(L, s, e - s);
                return luaL_error(L, "unexpected mask '%s' in %s", lua_tostring(L, -1), et->name);
            }
        }
        s = e;
        if (inversion)
            *penum &= ~item->value;
        else
            *penum |= item->value;
    }
    return 1;
}

static int lbE_icmp(int ch1, int ch2) {
    if (ch1 == ch2) return 1;
    if (ch1 >= 'A' && ch1 <= 'Z') ch1 += 'a' - 'A';
    if (ch2 >= 'A' && ch2 <= 'Z') ch1 += 'a' - 'A';
    return ch1 == ch2;
}

static int lbE_stricmp(const char *a, const char *b, size_t len) {
    size_t i;
    for (i = 0; i < len; ++i, ++a, ++b) {
        if (!lbE_icmp(*a, *b)) return *a > *b ? 1 : -1;
        if (*a == '\0') return *b == '\0' ? 0 : -1;
        if (*b == '\0') return *a != '\0' ? 1 : 0;
    }
    return 0;
}

static int lbE_toenum(lua_State *L, int idx, lbind_Enum *et, int mask, int check) {
    int type = lua_type(L, idx);
    if (type == LUA_TNUMBER)
        return (int)lua_tointeger(L, idx);
    else if (type == LUA_TSTRING) {
        size_t len;
        const char *s = lua_tolstring(L, idx, &len);
        int value;
        if (!mask) {
            lbind_EnumItem *item = lbind_findenum(et, s, len);
            if (item == NULL && check) return luaL_error(L, "invalid %s value: %s", et->name, s);
            if (item != NULL) return item->value;
        } else if (lbE_parsemask(et, s, &value, check ? L : NULL))
            return value;
    }
    if (check) lbind_typeerror(L, idx, et->name);
    return -1;
}

void lbind_initenum(lbind_Enum *et, const char *name) {
    et->name = name;
    et->nitem = 0;
    et->items = NULL;
}

lbind_EnumItem *lbind_findenum(lbind_Enum *et, const char *s, size_t len) {
    size_t b = 0, e = et->nitem - 1;
    while (b < e) {
        size_t mid = (b + e) >> 1;
        int res = lbE_stricmp(et->items[mid].name, s, len);
        if (res == 0)
            return &et->items[mid];
        else if (res < 0)
            b = mid + 1;
        else
            e = mid;
    }
    return NULL;
}

int lbind_pushmask(lua_State *L, int value, lbind_Enum *et) {
    luaL_Buffer b;
    lbind_EnumItem *items;
    int first = 1;
    if (et->items == NULL) {
        lua_pushliteral(L, "");
        return 0;
    }
    luaL_buffinit(L, &b);
    for (items = et->items; items->name != NULL; ++items) {
        if ((items->value & value) == value) {
            if (first)
                first = 0;
            else
                luaL_addchar(&b, ' ');
            luaL_addstring(&b, items->name);
            value &= ~items->value;
        }
    }
    luaL_pushresult(&b);
    return 1;
}

int lbind_pushenum(lua_State *L, const char *name, lbind_Enum *et) {
    lbind_EnumItem *item = lbind_findenum(et, name, ~(size_t)0);
    if (item == NULL) return -1;
    lua_pushinteger(L, item->value);
    return item->value;
}

int lbind_testmask(lua_State *L, int idx, lbind_Enum *et) { return lbE_toenum(L, idx, et, 1, 0); }

int lbind_checkmask(lua_State *L, int idx, lbind_Enum *et) { return lbE_toenum(L, idx, et, 1, 1); }

int lbind_testenum(lua_State *L, int idx, lbind_Enum *et) { return lbE_toenum(L, idx, et, 0, 0); }

int lbind_checkenum(lua_State *L, int idx, lbind_Enum *et) { return lbE_toenum(L, idx, et, 0, 1); }
#endif /* LBIND_NO_ENUM */

/* lbind Lua side runtime */
#ifndef LBIND_NO_RUNTIME
static lbind_Type *lbT_test(lua_State *L, int idx) {
    lbind_Type *t = (lbind_Type *)lua_touserdata(L, idx);
    lbB_typebox(L);
    lua_rawgetp(L, -1, t);
    t = (lbind_Type *)lua_touserdata(L, -1);
    lua_pop(L, 2);
    return t != NULL ? t : lbind_typeobject(L, -1);
}

static int lbL_bases(lua_State *L) {
    int i = 1;
    lbind_Type **bases, *t = lbT_test(L, 1);
    if (t == NULL) return lbind_typeerror(L, 1, "type");
    bases = t->bases;
    lua_settop(L, 2);
    if (!lua_istable(L, 2)) {
        lua_newtable(L);
        lua_replace(L, 2);
    }
    for (; *bases != NULL; ++bases) {
        if (!lbind_getmetatable(L, *bases)) lua_pushnil(L);
        lua_rawseti(L, -2, i);
    }
    lua_pushinteger(L, i);
    lua_setfield(L, -2, "n");
    return 1;
}

static int lbL_track(lua_State *L) {
    int i, top = lua_gettop(L);
    for (i = 1; i <= top; ++i) lbind_track(L, i);
    return top;
}

static int lbL_untrack(lua_State *L) {
    int i, top = lua_gettop(L);
    for (i = 1; i <= top; ++i) lbind_untrack(L, i);
    return top;
}

static int lbL_owner(lua_State *L) {
    int i, top = lua_gettop(L);
    luaL_checkstack(L, top, "no space for owner info");
    for (i = 1; i <= top; ++i) {
        if (lbind_hastrack(L, i))
            lua_pushliteral(L, "Lua");
        else
            lua_pushliteral(L, "C");
    }
    return top;
}

static int lbL_type(lua_State *L) {
    int i, top = lua_gettop(L);
    if (top == 0) {
        lbB_typebox(L);
        return 1;
    }
    for (i = 1; i <= top; ++i) {
        lbind_Type *t = lbT_test(L, i);
        lua_pushstring(L, t != NULL ? t->name : luaL_typename(L, -1));
        lua_replace(L, i);
    }
    return top;
}

static int lbL_pointer(lua_State *L) {
    int i, top = lua_gettop(L);
    if (top == 0) {
        lbB_internbox(L);
        return 1;
    }
    for (i = 1; i <= top; ++i) {
        const void *u = lbind_object(L, i);
        if (u == NULL)
            lua_pushnil(L);
        else
            lua_pushlightuserdata(L, (void *)u);
        lua_replace(L, i);
    }
    return top;
}

static int lbL_delete(lua_State *L) {
    int i, top = lua_gettop(L);
    for (i = 1; i <= top; ++i) {
        if (lua53_getfield(L, i, "delete") != LUA_TNIL) {
            lua_pushvalue(L, i);
            lua_call(L, 1, 0);
        } else {
            lua_pop(L, 1);
            lbind_delete(L, i);
        }
    }
    return 0;
}

static int lbL_isa(lua_State *L) {
    lbind_Type *t = lbT_test(L, 1);
    int i, top = lua_gettop(L);
    if (t == NULL) lbind_typeerror(L, 1, "lbind object/type");
    for (i = 2; i <= top; ++i) {
        if (!lbind_isa(L, i, t)) {
            lua_pushnil(L);
            lua_replace(L, i);
        }
    }
    return top - 1;
}

static int lbL_castto(lua_State *L) {
    lbind_Type *t = lbT_test(L, 1);
    int i, top = lua_gettop(L);
    if (t == NULL) lbind_typeerror(L, 1, "lbind object/type");
    for (i = 2; i <= top; ++i) {
        void *u = lbind_cast(L, -2, t);
        if (u == NULL)
            lua_pushnil(L);
        else if (!lbind_retrieve(L, u))
            lbind_wrap(L, u, t);
        lua_replace(L, i);
    }
    return top - 1;
}

int luaopen_lbind(lua_State *L) {
    luaL_Reg libs[] = {
#define ENTRY(name) {#name, lbL_##name}
            ENTRY(bases), ENTRY(castto), ENTRY(delete), ENTRY(isa), ENTRY(owner), ENTRY(pointer), ENTRY(track), ENTRY(type), ENTRY(untrack),
#undef ENTRY
            {NULL, NULL}};

    luaL_newlib(L, libs);
#if LUA_VERSION_NUM < 502
    lua_pushvalue(L, -1);
    lua_setglobal(L, "lbind");
#endif
    return 1;
}
#endif /* LBIND_NO_RUNTIME */

#pragma endregion
