// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine_scripting.h"

#include <string.h>

#include "core/core.h"
#include "lua/host/lua.h"

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
