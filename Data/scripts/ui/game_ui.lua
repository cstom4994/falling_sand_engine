local gd = game_datastruct

GetUIState = function()
    return gd.ui.state
end

OnGameGUIUpdate = function()
    local s = gd.ui.state
    -- if s == 1 then
    --     DrawMainMenuUI2()
    -- elseif s == 1001 then
    --     DrawAboutUI()
    -- end

    DrawMainMenuUI(Game)
    DrawDebugUI(Game)
end

DrawMainMenuUI2 = function()
    imgui.SetNextWindowSize(200, 250, imgui.constant.Cond.FirstUseEver)
    imgui.Begin("Demo", true, imgui.constant.WindowFlags.ShowBorders)
    imgui.Image(R_GetTextureHandle(title), R_GetTextureAttr(title, "h") / 2, R_GetTextureAttr(title, "w") / 2)
    imgui.End()
end

DrawAboutUI = function()
    imgui.SetNextWindowSize(200, 250, imgui.constant.Cond.FirstUseEver)
    imgui.Begin("About", true, imgui.constant.WindowFlags.ShowBorders)
    imgui.Text(metadot_metadata())
    imgui.End()
end
