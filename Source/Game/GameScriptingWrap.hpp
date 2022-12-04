// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GAMESCRIPTWRAP_HPP_
#define _METADOT_GAMESCRIPTWRAP_HPP_

#include <cstddef>
#include <string>
#include <utility>

#include "Engine/LuaCore.hpp"
#include "Engine/Scripting.hpp"
#include "GameDataStruct.hpp"

struct GameScriptingWrap
{
    LuaCore *MainLua = nullptr;

    void Init();
    void Bind();

    Biome *BiomeGet(std::string name);
};

#endif