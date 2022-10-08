// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.


#define INC_Player

#ifndef INC_Entity
#include "Entity.hpp"
#endif
#include "Item.hpp"

#ifndef INC_World
#include "world.hpp"
#endif

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

    void render(METAENGINE_Render_Target *target, int ofsX, int ofsY) override;
    void renderLQ(METAENGINE_Render_Target *target, int ofsX, int ofsY) override;
    void setItemInHand(Item *item, World *world);

    ~Player();
};
