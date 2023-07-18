-- Copyright(c) 2022-2023, KaoruXun All rights reserved.

InitGraphics = function()

    textures_init();

    -- var textures_pack = {
    --     "testTexture": { "path": "data/assets/textures/test.png" },
    --     "dirt1Texture": { "path": "data/assets/textures/testDirt.png" },
    --     "stone1": { "path": "data/assets/textures/testStone.png" },
    --     "smoothStone": { "path": "data/assets/textures/smooth_stone_128x.png" },
    --     "cobbleStone": { "path": "data/assets/textures/cobble_stone_128x.png" },
    --     "flatCobbleStone": { "path": "data/assets/textures/flat_cobble_stone_128x.png" },
    --     "smoothDirt": { "path": "data/assets/textures/smooth_dirt_128x.png" },
    --     "cobbleDirt": { "path": "data/assets/textures/cobble_dirt_128x.png" },
    --     "flatCobbleDirt": { "path": "data/assets/textures/flat_cobble_dirt_128x.png" },
    --     "softDirt": { "path": "data/assets/textures/soft_dirt.png" },
    --     "cloud": { "path": "data/assets/textures/cloud.png" },
    --     "gold": { "path": "data/assets/textures/gold.png" },
    --     "goldMolten": { "path": "data/assets/textures/moltenGold.png" },
    --     "goldSolid": { "path": "data/assets/textures/solidGold.png" },
    --     "iron": { "path": "data/assets/textures/iron.png" },
    --     "obsidian": { "path": "data/assets/textures/obsidian.png" },
    --     "caveBG": { "path": "data/assets/backgrounds/testCave.png" },
    --     "testAse": { "path": "data/assets/textures/tests/3.0_one_slice.ase" }
    -- };

    -- for (var t in textures_pack) {
    --     let textures_data = textures_pack[t];
    --     let path = textures_data["path"];
    --     test.textures_load(t, path);
    -- }

    --logoSfc = LoadTextureData("data/assets/ui/logo.png")
    --title = R_CopyImageFromSurface(GetSurfaceFromTexture(logoSfc))
    --R_SetImageFilter(title, 0)
    --DestroyTexture(logoSfc)

end

EndGraphics = function()
    textures_end()
end
