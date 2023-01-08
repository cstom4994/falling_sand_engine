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

ecs = require("ecs")

OnGameEngineLoad = function()
    InitGraphics()
    InitAudio()
    InitECS()
    controls_init()
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

-- function OnGameEngineLoad() {
--     test.load_script("data/scripts/graphics.js");
--     test.load_script("data/scripts/audio.js");

--     test.controls_init();
-- }

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
    world:update()
end

-- function OnWorldInitialized()
-- function OnWorldPreUpdate()
-- function OnWorldPostUpdate()
-- function OnPausedChanged( is_paused, is_inventory_pause )
-- function OnModSettingsChanged()
-- function OnPausePreUpdate()

OnGameEngineUnLoad = function()
    EndAudio()
    EndGraphics()
end
