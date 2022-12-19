// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_MU_H_
#define _METADOT_MU_H_

#include "engine/scripting/mu_compiler.h"
#include "engine/scripting/mu_parser.h"

extern "C" {
#include "libs/lua/host/lauxlib.h"
#include "libs/lua/host/lua.h"
#include "libs/lua/host/lualib.h"
int luaopen_mu(lua_State *L);
}  // extern "C"

#endif