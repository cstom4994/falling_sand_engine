// Copyright(c) 2022, KaoruXun All rights reserved.

#include "RigidBody.hpp"
#include "Game/Item.hpp"

#include <iostream>

RigidBody::RigidBody(b2Body *body) { this->body = body; }

RigidBody::~RigidBody() {
    // if (item) delete item;
}
