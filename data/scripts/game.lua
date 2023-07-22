-- Copyright(c) 2022-2023, KaoruXun All rights reserved.

-- function OnGameDataLoad() {
--     test.load_script("data/scripts/biomes.js");
-- }

require("game_datastruct")
require("ui.game_ui")
require("graphics")
require("audio")
require("global")
require("entities.entities")
require("fonts")
require("backgrounds.backgrounds")
require("ecs")

r3d = require("common.r3d")

local gd = game_datastruct

local cubeMesh = { { 0.0, 0.0, 0.0,
                     1.0, 0.0, 0.0,
                     0.0, 0.0, 1.0, 2 },
                   { 1.0, 0.0, 0.0,
                     0.0, 0.0, 1.0,
                     1.0, 0.0, 1.0, 3 },
                   { 0.0, 1.0, 0.0,
                     1.0, 1.0, 0.0,
                     0.0, 1.0, 1.0, 5 },
                   { 1.0, 1.0, 0.0,
                     0.0, 1.0, 1.0,
                     1.0, 1.0, 1.0, 6 },
                   { 0.0, 0.0, 0.0,
                     1.0, 0.0, 0.0,
                     0.0, 1.0, 0.0, 7 },
                   { 1.0, 0.0, 0.0,
                     0.0, 1.0, 0.0,
                     1.0, 1.0, 0.0, 9 },
                   { 0.0, 0.0, 1.0,
                     1.0, 0.0, 1.0,
                     0.0, 1.0, 1.0, 10 },
                   { 1.0, 0.0, 1.0,
                     0.0, 1.0, 1.0,
                     1.0, 1.0, 1.0, 11 },
                   { 0.0, 0.0, 0.0,
                     0.0, 0.0, 1.0,
                     0.0, 1.0, 0.0, 12 },
                   { 0.0, 0.0, 1.0,
                     0.0, 1.0, 0.0,
                     0.0, 1.0, 1.0, 13 },
                   { 1.0, 0.0, 0.0,
                     1.0, 0.0, 1.0,
                     1.0, 1.0, 0.0, 14 },
                   { 1.0, 0.0, 1.0,
                     1.0, 1.0, 0.0,
                     1.0, 1.0, 1.0, 15 }  }

OnGameEngineLoad = function()
    InitGraphics()
    InitAudio()
    InitECS()
    InitFont()
    controls_init()

    materials_init()
    materials_push()
    
    OnEntitiesTypeLoad()
end

OnGameLoad = function(game)
    Game = game

    create_biome("DEFAULT", 0)
    create_biome("TEST_1", 1)
    create_biome("TEST_1_2", 2)
    create_biome("TEST_2", 3)
    create_biome("TEST_2_2", 4)
    create_biome("TEST_3", 5)
    create_biome("TEST_3_2", 6)
    create_biome("TEST_4", 7)
    create_biome("TEST_4_2", 8)

    create_biome("PLAINS", 9)
    create_biome("MOUNTAINS", 10)
    create_biome("FOREST", 11)
end

-- function OnImGuiUpdate() {
--     const runImGui = () => {
--         try {
--             ImGui.Begin("测试中文", null, ImGui.WindowFlags.NoTitleBar);
--             ImGui.Text("Hello MetaDot world!");
--             ImGui.Button("按钮");
--             ImGui.End();
--         } catch (error) {
--             test.println("ImGui drawing error:", error);
--         }
--     };
--     runImGui();
-- }

OnGameTickUpdate = function()
    -- world:update()
end

OnUpdate = function()
    -- world:update()
end

OnRender = function()

    if gd.render.test == 1 then
        local currentDemo = 2
        local rotationScale = 0.03
        local xRotation, yRotation = 0, 0.5
        local cameraDistance = 10
        local cameraDistanceScale = 0.2

        local curTime = os.clock()
        local triangles = {}

        for i = 1, 10 do
            local cube = r3d.cloneTs(cubeMesh)
            r3d.translateTs(cube, -0.5, -0.5, -0.5)
            r3d.rotateTs(cube, curTime * 0.2, curTime * 1.0, curTime * 1.0)
            r3d.translateTs(cube, (i - 5) *  1.5, math.sin(curTime + i), 0)
            r3d.concatTs(triangles, cube, true)
        end

        r3d.rotateTs(triangles, 0, 0, xRotation)
        r3d.rotateTs(triangles, yRotation + math.pi / 2, 0, 0)
        r3d.translateTs(triangles, 0, 0, cameraDistance)
    
        -- gpu.clear(0)
        r3d.drawTs(triangles)
    end

end

-- function OnWorldInitialized()
-- function OnWorldPreUpdate()
-- function OnWorldPostUpdate()
-- function OnPausedChanged( is_paused, is_inventory_pause )
-- function OnModSettingsChanged()
-- function OnPausePreUpdate()

OnGameEngineUnLoad = function()
    METADOT_BUG("OnGameEngineUnLoad called")

    OnGameGUIEnd()

    EndFont()
    EndAudio()
    EndGraphics()
end
