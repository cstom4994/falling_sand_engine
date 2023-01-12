-- Copyright(c) 2022-2023, KaoruXun All rights reserved.

TTF_STYLE_NORMAL        = 0x00
TTF_STYLE_BOLD          = 0x01
TTF_STYLE_ITALIC        = 0x02
TTF_STYLE_UNDERLINE     = 0x04
TTF_STYLE_STRIKETHROUGH = 0x08

InitFont = function()
    -- font1 = FontCache_CreateFont()
    -- FontCache_LoadFont(font1, METADOT_RESLOC("data/assets/fonts/ark-pixel-12px-monospaced-zh_cn.ttf"), 12,
    --     FontCache_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);

end

EndFont = function()
    -- FontCache_FreeFont(font1);
end

GetFont = function (index)
    if index == 1 then
        -- return font1
    end
end
