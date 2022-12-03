// Copyright(c) 2022, KaoruXun All rights reserved.

#include "ReflectionFlat.hpp"
#include "MuDSL.hpp"

GameData GameData_;

void ReleaseGameData() {
    for (auto b : GameData_.biome_container) {
        if(static_cast<bool>(b)) delete b;
    }
}
