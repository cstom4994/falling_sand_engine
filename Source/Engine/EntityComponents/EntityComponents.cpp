// Copyright(c) 2022, KaoruXun All rights reserved.

#include "EntityComponents.hpp"

#include "Engine/Internal/BuiltinBox2d.h"
#include "Game/RigidBody.hpp"

void Entity::render(R_Target *target, int ofsX, int ofsY) {}

void Entity::renderLQ(R_Target *target, int ofsX, int ofsY) {}

Entity::Entity(bool isplayer) : is_player(isplayer) {}

Entity::~Entity() {
    // if (static_cast<bool>(rb)) delete rb;
}