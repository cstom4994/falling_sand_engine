local cstruct = require("common.cstruct")
local dump = require("common.dump")

local backgroundlayer_cstruct = [[

struct backgroundlayer_cstruct {
    float parralaxX;
    float parralaxY;
    float moveX;
    float moveY;
};

]]

local backgroundlayer_cstruct_s = cstruct.datastruct(backgroundlayer_cstruct)

local get_px = backgroundlayer_cstruct_s:getter "struct backgroundlayer_cstruct.parralaxX"
local get_py = backgroundlayer_cstruct_s:getter "struct backgroundlayer_cstruct.parralaxY"
local get_mx = backgroundlayer_cstruct_s:getter "struct backgroundlayer_cstruct.moveX"
local get_my = backgroundlayer_cstruct_s:getter "struct backgroundlayer_cstruct.moveY"

OnBackgroundLoad = function()

    local TEST_OVERWORLD = {
        { name = "data/assets/backgrounds/TestOverworld/layer2.png", p1 = 0.125, p2 = 0.125, x1 = 1, x2 = 0 },
        { name = "data/assets/backgrounds/TestOverworld/layer3.png", p1 = 0.25, p2 = 0.25, x1 = 0, x2 = 0 },
        { name = "data/assets/backgrounds/TestOverworld/layer4.png", p1 = 0.375, p2 = 0.375, x1 = 4, x2 = 0 },
        { name = "data/assets/backgrounds/TestOverworld/layer5.png", p1 = 0.5, p2 = 0.5, x1 = 0, x2 = 0 }
    }

    local TEST_ROOM = {
        { name = "data/assets/backgrounds/TestRoom/testTile.png", p1 = 0.125, p2 = 0.125, x1 = 0, x2 = 0 },
    }

    NewBackgroundObject("TEST_OVERWORLD", 8302539, TEST_OVERWORLD) -- 0x7EAFCB
    NewBackgroundObject("TEST_ROOM", tonumber("AAAAAA", 16), TEST_ROOM)

    print(dump(TEST_OVERWORLD))
end
