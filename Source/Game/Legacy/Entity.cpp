// Copyright(c) 2022, KaoruXun All rights reserved.


#include "Game/Legacy/RigidBody.hpp"
#ifndef INC_Entity
#include "Entity.hpp"
#endif

#include "Game/Settings.hpp"

void Entity::renderLQ(void *target, int ofsX, int ofsY) {
    //METAENGINE_Render_Rectangle(target, x + ofsX, y + ofsY, x + ofsX + hw, y + ofsY + hh, {0xff, 0xff, 0xff, 0xff});
}


void Entity::render(void *target, int ofsX, int ofsY) {
}

Entity::Entity() {
    //rb = new RigidBody(body);
}

Entity::~Entity() {
    //delete rb;
}
