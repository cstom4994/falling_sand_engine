// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_SCRIPTING_H_
#define _METADOT_SCRIPTING_H_

#include "engine/scripting/lua_wrapper.h"

typedef struct LuaCoreC {
    lua_State *L;
} LuaCoreC;

typedef struct LuaCode {
    // Status 0 = error, 1 = no problems, 2 = reloaded but not prime ran
    char status;
    char *loopFunction;
    char *scriptPath;
    char scriptName[256];
} LuaCode;

void InitLuaCoreC(LuaCoreC *_struct, lua_State *LuaCoreCppFunc(void *), void* luacorecpp);
void FreeLuaCoreC(LuaCoreC *_struct);

void LuaCodeInit(LuaCode *_struct, const char *scriptPath);
void LuaCodeUpdate(LuaCode *_struct);
void LuaCodeFree(LuaCode *_struct);

#endif