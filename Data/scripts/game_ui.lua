local gd = game_datastruct

OnGameGUIUpdate = function()
    if gd.ui.mainmenu_state == 1 then
        DrawMainMenuUI()
    end
end

DrawMainMenuUI = function()
    texId = R_GetTextureHandle(title);

    imgui.SetNextWindowSize(200, 250, imgui.constant.Cond.FirstUseEver)
    imgui.Begin("Demo", true, imgui.constant.WindowFlags.ShowBorders)
    imgui.Image(texId, 50, 50)
    imgui.End()
end
