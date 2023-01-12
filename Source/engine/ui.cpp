// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "ui.hpp"

#include "core/global.hpp"
#include "engine/engine.h"
#include "engine/memory.hpp"
#include "game/game.hpp"
#include "renderer/renderer_gpu.h"

IMPLENGINE();

void UIRendererInit() {
    METADOT_INFO("Loading ImGUI");
    METADOT_NEW(C, global.ImGuiCore, ImGuiCore);
    global.ImGuiCore->Init();
}

void UIRendererPostUpdate() { global.ImGuiCore->NewFrame(); }
void UIRendererDraw() { global.ImGuiCore->Draw(); }

void UIRendererUpdate() {
    global.ImGuiCore->Render();
    auto &l = global.scripts->LuaCoreCpp->s_lua;
    LuaWrapper::LuaFunction OnGameGUIUpdate = l["OnGameGUIUpdate"];
    OnGameGUIUpdate();
}

void UIRendererFree() {
    global.ImGuiCore->onDetach();
    METADOT_DELETE(C, global.ImGuiCore, ImGuiCore);
}

void DrawPoint(Vector3 pos, float size, GLuint texture, U8 r, U8 g, U8 b) {
    Vector3 min = {pos.x - size, pos.y - size, 0};
    Vector3 max = {pos.x + size, pos.y + size, 0};

    if (texture) {
        // DrawRectangleTextured(min, max, texture, r, g, b);
    } else {
        // DrawRectangle(min, max, r, g, b);
    }
}

void DrawLine(Vector3 min, Vector3 max, float thickness, U8 r, U8 g, U8 b) { R_Line(Render.target, min.x, min.y, max.x, max.y, {r, g, b, 255}); }