// Copyright(c) 2022, KaoruXun All rights reserved.

function OnGameDataLoad() {
    test.load_script("data/scripts/biomes.js");
}

function OnGameEngineLoad() {
    test.load_script("data/scripts/graphics.js");
    test.load_script("data/scripts/audio.js");

    test.controls_init();
}

function OnImGuiUpdate() {
    const runImGui = () => {
        try {
            ImGui.Begin("测试中文", null, ImGui.WindowFlags.NoTitleBar);
            ImGui.Text("Hello MetaDot world!");
            ImGui.Button("按钮");
            ImGui.End();
        } catch (error) {
            test.println("ImGui drawing error:", error);
        }
    };
    runImGui();
}

function OnGameTickUpdate() {

}

// function OnWorldInitialized()
// function OnWorldPreUpdate()
// function OnWorldPostUpdate()
// function OnPausedChanged( is_paused, is_inventory_pause )
// function OnModSettingsChanged()
// function OnPausePreUpdate()