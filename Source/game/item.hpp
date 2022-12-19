// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_ITEM_HPP_
#define _METADOT_ITEM_HPP_

#include "core/core.h"
#include "game_datastruct.hpp"
#include "materials.hpp"

class ItemFlags {
public:
    static const U8 RIGIDBODY = 0b00000001;
    static const U8 FLUID_CONTAINER = 0b00000010;
    static const U8 TOOL = 0b00000100;
    static const U8 CHISEL = 0b00001000;
    static const U8 HAMMER = 0b00010000;
    static const U8 VACUUM = 0b00100000;
};

class Item {
public:
    U8 flags = 0;

    void setFlag(U8 f) { flags |= f; }

    bool getFlag(U8 f) { return flags & f; }

    C_Surface *surface = nullptr;
    R_Image *texture = nullptr;
    int pivotX = 0;
    int pivotY = 0;
    float breakSize = 16;
    std::vector<MaterialInstance> carry;
    std::vector<U16Point> fill;
    uint16_t capacity = 0;

    std::vector<Particle *> vacuumParticles;

    Item();
    ~Item();

    static Item *makeItem(U8 flags, RigidBody *rb);
    void loadFillTexture(C_Surface *tex);
};

#endif