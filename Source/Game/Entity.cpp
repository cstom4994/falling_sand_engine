// Copyright(c) 2022, KaoruXun All rights reserved.


#include "Core/Global.hpp"
#include "Game/RigidBody.hpp"
#ifndef INC_Entity
#include "Entity.hpp"
#endif

#include "Game/Game.hpp"
#include "Game/Settings.hpp"

void Entity::renderLQ(METAENGINE_Render_Target *target, int ofsX, int ofsY) {
    METAENGINE_Render_Rectangle(target, x + ofsX, y + ofsY, x + ofsX + hw, y + ofsY + hh, {0xff, 0xff, 0xff, 0xff});

    auto image2 = METAENGINE_Render_CopyImageFromSurface(Textures::testAse);
    METAENGINE_Render_BlitScale(image2, NULL, global.game->RenderTarget_.target, 128, 128, 5.0f, 5.0f);
}


void Entity::render(METAENGINE_Render_Target *target, int ofsX, int ofsY) {
}

Entity::Entity() {
    //rb = new RigidBody(body);
}

Entity::~Entity() {
    //delete rb;
}
