// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_RIGIDBODY_HPP_
#define _METADOT_RIGIDBODY_HPP_

#include "Engine/Internal/BuiltinBox2d.h"
#include "Engine/Renderer/RendererGPU.h"
#include "Engine/SDLWrapper.h"
#include "Materials.hpp"
#include <memory>
#include <vector>

#include "Engine/Math.hpp"

class Item;

class RigidBody {
public:
    b2Body *body = nullptr;
    C_Surface *surface = nullptr;
    R_Image *texture = nullptr;

    int matWidth = 0;
    int matHeight = 0;
    MaterialInstance *tiles = nullptr;

    // hitbox needs update
    bool needsUpdate = false;

    // surface needs to be converted to texture
    bool texNeedsUpdate = false;

    int weldX = -1;
    int weldY = -1;
    bool back = false;
    std::list<TPPLPoly> outline;
    std::list<TPPLPoly> outline2;
    float hover = 0;

    Item *item = nullptr;

    RigidBody(b2Body *body);
    ~RigidBody();
};

#endif
