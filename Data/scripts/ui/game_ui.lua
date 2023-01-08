local gd = game_datastruct

OnGameGUIUpdate = function()
    local s = gd.ui.mainmenu_state
    if s == 1 then
        DrawMainMenuUI()
    elseif s == 1001 then
        DrawAboutUI()
    end
end

DrawMainMenuUI = function()
    texId = R_GetTextureHandle(title);

    imgui.SetNextWindowSize(200, 250, imgui.constant.Cond.FirstUseEver)
    imgui.Begin("Demo", true, imgui.constant.WindowFlags.ShowBorders)
    imgui.Image(texId, R_GetTextureAttr(title, "h") / 2, R_GetTextureAttr(title, "w") / 2)
    imgui.End()
end

DrawAboutUI = function()
    imgui.SetNextWindowSize(200, 250, imgui.constant.Cond.FirstUseEver)
    imgui.Begin("About", true, imgui.constant.WindowFlags.ShowBorders)
    imgui.Text(metadot_metadata())
    imgui.End()
end
