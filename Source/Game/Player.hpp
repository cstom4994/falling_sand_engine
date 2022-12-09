// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_PLAYER_HPP_
#define _METADOT_PLAYER_HPP_

#include "GameDataStruct.hpp"
#include "Item.hpp"
#include "world.hpp"

class World;

class Player : public Entity {
public:
    Item *heldItem = nullptr;
    float holdAngle = 0;
    long long startThrow = 0;
    bool holdHammer = false;
    bool holdVacuum = false;
    int hammerX = 0;
    int hammerY = 0;

    void render(R_Target *target, int ofsX, int ofsY) override;
    void renderLQ(R_Target *target, int ofsX, int ofsY) override;
    void setItemInHand(Item *item, World *world);

    ~Player();
};

#endif