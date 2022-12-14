// Copyright(c) 2022, KaoruXun All rights reserved.

#include "worldentity.hpp"

#include "engine/internal/builtin_box2d.h"
#include "game/rigidbody.hpp"

void WorldEntity::render(R_Target *target, int ofsX, int ofsY) {}

void WorldEntity::renderLQ(R_Target *target, int ofsX, int ofsY) {}

WorldEntity::WorldEntity(bool isplayer) : is_player(isplayer) {}

WorldEntity::~WorldEntity() {
    // if (static_cast<bool>(rb)) delete rb;
}