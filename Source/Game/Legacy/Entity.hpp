// Copyright(c) 2022, KaoruXun All rights reserved.

#include "box2d/b2_body.h"
#define INC_Entity


#ifndef INC_RigidBody
#include "RigidBody.hpp"
#endif
#include "Engine/Render/RendererGPU.h"

class Entity {
public:
    float x = 0;
    float y = 0;
    float vx = 0;
    float vy = 0;
    int hw = 14;
    int hh = 26;
    bool ground = false;
    RigidBody *rb = nullptr;
    b2Body *body = nullptr;

    virtual void render(METAENGINE_Render_Target *target, int ofsX, int ofsY);
    virtual void renderLQ(METAENGINE_Render_Target *target, int ofsX, int ofsY);
    Entity();
    ~Entity();
};
