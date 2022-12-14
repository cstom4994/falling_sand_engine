// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_PLAYER_HPP_
#define _METADOT_PLAYER_HPP_

#include "game_datastruct.hpp"
#include "item.hpp"
#include "world.hpp"

class World;

class Player : public WorldEntity {
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

    Player();
    ~Player();
};

#endif