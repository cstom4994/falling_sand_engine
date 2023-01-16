// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "ui.hpp"

#include <memory>

#include "core/core.h"
#include "core/global.hpp"
#include "engine/engine.h"
#include "engine/memory.hpp"
#include "engine_platform.h"
#include "game/controls.hpp"
#include "game/game.hpp"
#include "game/game_resources.hpp"
#include "math.hpp"
#include "renderer/renderer_gpu.h"
#include "renderer/renderer_opengl.h"
#include "renderer/renderer_utils.h"

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
    UIElement testElement3{.type = ElementType::buttonElement,
                           .minRectX = 55,
                           .minRectY = 80,
                           .maxRectX = 70,
                           .maxRectY = 90,
                           .color = {54, 54, 54, 255},
                           .text = "按钮捏",
                           .cclass = {.button = (UI_Button){.state = 0, .func = []() { METADOT_INFO("button pressed"); }}}};

    UIElement testElement4{
            .type = ElementType::progressBarElement,
            .minRectX = 55,
            .minRectY = 95,
            .maxRectX = 100,
            .maxRectY = 120,
            .color = {54, 54, 54, 255},
            .text = "进度条",
            .cclass = {.progressbar = (UI_ProgressBar){.state = 0, .bar_type = 1, .bar_current = 50.0f, .bar_limit = 1000.0f, .bar_color = {54, 54, 54, 255}, .bar_text_color = {255, 255, 255, 255}}}};

    global.uidata->elementLists.insert(std::make_pair("testElement1", testElement1));
    global.uidata->elementLists.insert(std::make_pair("testElement2", testElement2));
    global.uidata->elementLists.insert(std::make_pair("testElement3", testElement3));
    global.uidata->elementLists.insert(std::make_pair("testElement4", testElement4));
}

void UIRendererPostUpdate() { global.uidata->ImGuiCore->NewFrame(); }
void UIRendererDraw() {

    // Drawing element
    for (auto &&e : global.uidata->elementLists) {
        R_Image *Img = nullptr;
        if (e.second.texture) {
            Img = R_CopyImageFromSurface(e.second.texture->surface);
        }

        if (e.second.type == ElementType::lineElement) {
            R_Line(Render.target, e.second.minRectX, e.second.minRectY, e.second.maxRectX, e.second.maxRectY, e.second.color);
        }
        if (e.second.type == ElementType::coloredRectangle) {
            R_RectangleFilled(Render.target, e.second.minRectX, e.second.minRectY, e.second.maxRectX, e.second.maxRectY, e.second.color);
        }
        if (e.second.type == ElementType::progressBarElement) {
            int drect = e.second.maxRectX - e.second.minRectX;
            float p = e.second.cclass.progressbar.bar_current / e.second.cclass.progressbar.bar_limit;
            int c = e.second.minRectX + p * drect;
            R_RectangleRound(Render.target, e.second.minRectX, e.second.minRectY, e.second.maxRectX, e.second.maxRectY, 2.0f, e.second.color);
            R_RectangleFilled(Render.target, e.second.minRectX, e.second.minRectY, c, e.second.maxRectY, e.second.color);

            if (e.second.cclass.progressbar.bar_type == 1)
                FontCache_DrawColor(global.game->font, Render.target, e.second.minRectX, e.second.minRectY, e.second.cclass.progressbar.bar_text_color, e.second.text.c_str());
        }
        if (e.second.type == ElementType::buttonElement) {
        }
        if (e.second.type == ElementType::texturedRectangle || e.second.type == ElementType::buttonElement) {
            if (Img) {
                R_SetImageFilter(Img, R_FILTER_NEAREST);
                R_SetBlendMode(Img, R_BLEND_NORMAL);
                R_Rect dest{.x = (float)e.second.minRectX, .y = (float)e.second.minRectY, .w = (float)e.second.maxRectX, .h = (float)e.second.maxRectY};
                R_BlitRect(Img, NULL, Render.target, &dest);
            }
        }
        if (e.second.type == ElementType::textElement || e.second.type == ElementType::buttonElement) {
            FontCache_DrawColor(global.game->font, Render.target, e.second.minRectX, e.second.minRectY, e.second.color, e.second.text.c_str());
        }

        if (Img) R_FreeImage(Img);

        if (global.game->GameIsolate_.globaldef.draw_ui_debug) R_Rectangle(Render.target, e.second.minRectX, e.second.minRectY, e.second.maxRectX, e.second.maxRectY, {255, 20, 147, 255});
    }
    global.uidata->ImGuiCore->Draw();
}

F32 BoxDistence(R_Rect box, R_vec2 A) {
    if (A.x >= box.x && A.x <= box.x + box.w && A.y >= box.y && A.x <= box.y + box.h) return -1.0f;
    return 0;
}

void UIRendererUpdate() {
    global.uidata->ImGuiCore->Render();
    auto &l = global.scripts->LuaCoreCpp->s_lua;
    LuaWrapper::LuaFunction OnGameGUIUpdate = l["OnGameGUIUpdate"];
    OnGameGUIUpdate();

    for (auto &&e : global.uidata->elementLists) {
        R_Rect rect{.x = (float)e.second.minRectX, .y = (float)e.second.minRectY, .w = (float)e.second.maxRectX, .h = (float)e.second.maxRectY};
        if (e.second.type == ElementType::buttonElement) {
            if (BoxDistence(rect, GetMousePos()) < 0.0f && Controls::lmouse && e.second.cclass.button.func) {
                e.second.cclass.button.func();
            }
        }
        if (e.second.type == ElementType::progressBarElement) {
            // Test haha
            e.second.cclass.progressbar.bar_current = (e.second.cclass.progressbar.bar_current < e.second.cclass.progressbar.bar_limit) ? e.second.cclass.progressbar.bar_current + 1 : 0;
        }
    }
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