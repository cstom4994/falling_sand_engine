// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_REFLECTIONFLAT_HPP_
#define _METADOT_REFLECTIONFLAT_HPP_

#include <array>
#include <vector>

#include "core/macros.h"
#include "game/console.hpp"
#include "game/game_scriptingwrap.hpp"

METADOT_INLINE CVar::ItemLog &operator<<(CVar::ItemLog &log, ImVec4 &vec) {
    log << "ImVec4: [" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << "]";
    return log;
}

static void int_setter(int &my_type, int v) { my_type = v; }

static void float_setter(float &my_type, float vec) {
    // if (vec.size() != 1) return;
    my_type = vec;
}

static void imvec4_setter(ImVec4 &my_type, std::vector<int> vec) {
    if (vec.size() < 4) return;

    my_type.x = vec[0] / 255.f;
    my_type.y = vec[1] / 255.f;
    my_type.z = vec[2] / 255.f;
    my_type.w = vec[3] / 255.f;
}

struct GameData {
    int ofsX = 0;
    int ofsY = 0;

    float plPosX = 0;
    float plPosY = 0;

    float camX = 0;
    float camY = 0;

    float desCamX = 0;
    float desCamY = 0;

    float freeCamX = 0;
    float freeCamY = 0;

    std::vector<Biome *> biome_container;
};

void ReleaseGameData();

extern GameData GameData_;

#endif
