-- Copyright(c) 2022-2023, KaoruXun All rights reserved.

require("entities.player")

local cstruct = require("common.cstruct")

local entity_cstruct = [[

struct entity_cstruct {
    float x;
    float y;
    float vx;
    float vy;
    int hw;
    int hh;
    int ground;
    int type;
};

]]

-- RigidBody *rb = nullptr;
-- b2Body *body = nullptr;

OnEntitiesTypeLoad = function()
    local entity_cstruct_s = cstruct.datastruct(entity_cstruct)
end
