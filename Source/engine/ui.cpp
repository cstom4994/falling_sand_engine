// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "ui.hpp"

#include "core/global.hpp"
#include "engine/engine.h"
#include "engine/memory.hpp"
#include "game/game.hpp"
#include "game/game_resources.hpp"
#include "renderer/renderer_gpu.h"
#include "renderer/renderer_opengl.h"

IMPLENGINE();

void UIRendererInit() {
    // UIData
    METADOT_INFO("Loading UIData");
    global.uidata = new UIData;

    METADOT_INFO("Loading ImGUI");
    METADOT_NEW(C, global.uidata->ImGuiCore, ImGuiCore);
    global.uidata->ImGuiCore->Init();

    // Test element drawing
    UIElement testElement1{.type = ElementType::texturedRectangle,
                           .minRectX = 50,
                           .minRectY = 50,
                           .maxRectX = 200,
                           .maxRectY = 200,
                           .color = {255, 255, 255, 255},
                           .texture = LoadTexture("data/assets/minecraft/textures/gui/demo_background.png")};

    UIElement testElement2{.type = ElementType::textElement, .minRectX = 55, .minRectY = 55, .maxRectX = 200, .maxRectY = 200, .color = {54, 54, 54, 255}, .text = "哈哈哈哈哈嗝"};

    global.uidata->elementLists.insert(std::make_pair("testElement1", testElement1));
    global.uidata->elementLists.insert(std::make_pair("testElement2", testElement2));
}

void UIRendererPostUpdate() { global.uidata->ImGuiCore->NewFrame(); }
void UIRendererDraw() {

    // Drawing element
    for (auto &&e : global.uidata->elementLists) {
        switch (e.second.type) {
            case ElementType::lineElement:
                R_Line(Render.target, e.second.minRectX, e.second.minRectY, e.second.maxRectX, e.second.maxRectY, e.second.color);
                break;
            case ElementType::textElement:
                FontCache_DrawColor(global.game->font, Render.target, e.second.minRectX, e.second.minRectY, e.second.color, e.second.text.c_str());
                break;
            case ElementType::coloredRectangle:
                R_RectangleFilled(Render.target, e.second.minRectX, e.second.minRectY, e.second.maxRectX, e.second.maxRectY, e.second.color);
                break;
            case ElementType::texturedRectangle:
                R_Image *Img = R_CopyImageFromSurface(e.second.texture->surface);
                R_SetImageFilter(Img, R_FILTER_NEAREST);
                R_SetBlendMode(Img, R_BLEND_NORMAL);
                R_Rect dest{.x = (float)e.second.minRectX, .y = (float)e.second.minRectY, .w = (float)e.second.maxRectX, .h = (float)e.second.maxRectY};
                R_BlitRect(Img, NULL, Render.target, &dest);
                R_FreeImage(Img);
                break;
        }
    }
    global.uidata->ImGuiCore->Draw();
}

void UIRendererUpdate() {
    global.uidata->ImGuiCore->Render();
    auto &l = global.scripts->LuaCoreCpp->s_lua;
    LuaWrapper::LuaFunction OnGameGUIUpdate = l["OnGameGUIUpdate"];
    OnGameGUIUpdate();
}

void UIRendererFree() {
    global.uidata->ImGuiCore->onDetach();
    METADOT_DELETE(C, global.uidata->ImGuiCore, ImGuiCore);

    for (auto &&e : global.uidata->elementLists) {
        if (static_cast<bool>(e.second.texture)) Eng_DestroyTexture(e.second.texture);
    }

    delete global.uidata;
}

void DrawPoint(Vector3 pos, float size, Texture *texture, U8 r, U8 g, U8 b) {
    Vector3 min = {pos.x - size, pos.y - size, 0};
    Vector3 max = {pos.x + size, pos.y + size, 0};

    if (texture) {
        // DrawRectangleTextured(min, max, texture, r, g, b);
    } else {
        // DrawRectangle(min, max, r, g, b);
    }
}

void DrawLine(Vector3 min, Vector3 max, float thickness, U8 r, U8 g, U8 b) { R_Line(Render.target, min.x, min.y, max.x, max.y, {r, g, b, 255}); }