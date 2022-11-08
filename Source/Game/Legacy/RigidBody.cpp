// Copyright(c) 2022, KaoruXun All rights reserved.


#include "RigidBody.hpp"
#include <iostream>

RigidBody::RigidBody(b2Body *body) {
    this->body = body;
}

RigidBody::~RigidBody() {
    //if (item) delete item;
    //SDL_DestroyTexture(texture);
    //SDL_FreeSurface(surface);
}
