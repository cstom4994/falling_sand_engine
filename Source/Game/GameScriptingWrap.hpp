// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GAMESCRIPTWRAP_HPP_
#define _METADOT_GAMESCRIPTWRAP_HPP_

#include <cstddef>
#include <string>
#include <utility>

#include "Engine/LuaCore.hpp"
#include "Engine/Scripting.hpp"

class Biome {
public:
    int id;
    std::string name;
    explicit Biome(std::string name, int id) : name(std::move(name)), id(std::move(id)){};
};

struct GameScriptingWrap
{
    LuaCore *MainLua = nullptr;

    void Init();
    void Bind();

    Biome *BiomeGet(std::string name);
};

#endif