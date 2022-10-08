// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.

#pragma once

#define INC_RigidBody
#include "Engine/render/renderer_gpu.h"
#include "Materials.hpp"
#include <SDL.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_math.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_shape.h>
#include <box2d/b2_world.h>
#include <memory>
#include <vector>


#include "lib/polypartition.h"

class Item;

class RigidBody {
public:
    b2Body *body = nullptr;
    SDL_Surface *surface = nullptr;
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
