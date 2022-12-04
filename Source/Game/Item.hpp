// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_ITEM_HPP_
#define _METADOT_ITEM_HPP_

#include "RigidBody.hpp"
#include "Materials.hpp"
#include "GameDataStruct.hpp"

class ItemFlags {
public:
    static const uint8_t RIGIDBODY = 0b00000001;
    static const uint8_t FLUID_CONTAINER = 0b00000010;
    static const uint8_t TOOL = 0b00000100;
    static const uint8_t CHISEL = 0b00001000;
    static const uint8_t HAMMER = 0b00010000;
    static const uint8_t VACUUM = 0b00100000;
};

typedef struct UInt16Point
{
    uint16_t x;
    uint16_t y;
} UInt16Point;

class Item {
public:
    uint8_t flags = 0;

    void setFlag(uint8_t f) { flags |= f; }

    bool getFlag(uint8_t f) { return flags & f; }

    C_Surface *surface = nullptr;
    METAENGINE_Render_Image *texture = nullptr;
    int pivotX = 0;
    int pivotY = 0;
    float breakSize = 16;
    std::vector<MaterialInstance> carry;
    std::vector<UInt16Point> fill;
    uint16_t capacity = 0;

    std::vector<Particle *> vacuumParticles;

    Item();
    ~Item();

    static Item *makeItem(uint8_t flags, RigidBody *rb);
    void loadFillTexture(C_Surface *tex);
};

#endif