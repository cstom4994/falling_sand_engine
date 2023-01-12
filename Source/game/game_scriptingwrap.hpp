// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_GAMESCRIPTWRAP_HPP_
#define _METADOT_GAMESCRIPTWRAP_HPP_

#include <cstddef>
#include <string>
#include <utility>

#include "engine/engine_scripting.hpp"
#include "game_datastruct.hpp"

void InitGameScriptingWrap();
void BindGameScriptingWrap();
void EndGameScriptingWrap();
Biome *BiomeGet(std::string name);

#endif