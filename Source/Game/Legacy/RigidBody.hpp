// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_RIGIDBODY_HPP_
#define _METADOT_RIGIDBODY_HPP_

#define INC_RigidBody
#include "Engine/Platforms/SDLWrapper.hpp"
#include "Engine/Render/renderer_gpu.h"
#include "Materials.hpp"
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_math.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_shape.h>
#include <box2d/b2_world.h>
#include <memory>
#include <vector>


#include "Libs/polypartition.h"

class Item;

class RigidBody {
public:
    b2Body *body = nullptr;
    C_Surface *surface = nullptr;
    METAENGINE_Render_Image *texture = nullptr;

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