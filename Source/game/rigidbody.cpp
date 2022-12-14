// Copyright(c) 2022, KaoruXun All rights reserved.

#include "rigidbody.hpp"
#include "game/item.hpp"

#include <iostream>

RigidBody::RigidBody(b2Body *body) { this->body = body; }

RigidBody::~RigidBody() {
    // if (item) delete item;
}
