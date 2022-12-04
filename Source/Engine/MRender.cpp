// Copyright(c) 2022, KaoruXun All rights reserved.

#include "MRender.hpp"

#include "Core/Global.hpp"
#include "Engine/Memory.hpp"
#include "Game/ImGuiCore.hpp"
#include "Game/InEngine.h"
#include "ImGui/imgui.h"
#include <string>

void Drawing::drawText(std::string name, std::string text, uint8_t x, uint8_t y, ImVec4 col) {
    auto func = [&] { ImGui::TextColored(col, "%s", text.c_str()); };
    drawTextEx(name, x, y, func);
}

void Drawing::drawTextEx(std::string name, uint8_t x, uint8_t y, std::function<void()> func) {
    ImGui::SetNextWindowPos(
            global.ImGuiCore->GetNextWindowsPos(ImGuiWindowTags::UI_MainMenu, ImVec2(x, y)));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking |
                             ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin(name.c_str(), NULL, flags);
    func();
    ImGui::End();
}

b2Vec2 Drawing::rotate_point(float cx, float cy, float angle, b2Vec2 p) {
    float s = sin(angle);
    float c = cos(angle);

    // translate point back to origin:
    p.x -= cx;
    p.y -= cy;

    // rotate point
    float xnew = p.x * c - p.y * s;
    float ynew = p.x * s + p.y * c;

    // translate point back:
    return b2Vec2(xnew + cx, ynew + cy);
}

void Drawing::drawPolygon(METAENGINE_Render_Target *target, SDL_Color col, b2Vec2 *verts, int x,
                          int y, float scale, int count, float angle, float cx, float cy) {
    if (count < 2) return;
    b2Vec2 last = rotate_point(cx, cy, angle, verts[count - 1]);
    for (int i = 0; i < count; i++) {
        b2Vec2 rot = rotate_point(cx, cy, angle, verts[i]);
        METAENGINE_Render_Line(target, x + last.x * scale, y + last.y * scale, x + rot.x * scale,
                               y + rot.y * scale, col);
        last = rot;
    }
}

uint32 Drawing::darkenColor(uint32 color, float brightness) {
    int a = (color >> 24) & 0xFF;
    int r = (int) (((color >> 16) & 0xFF) * brightness);
    int g = (int) (((color >> 8) & 0xFF) * brightness);
    int b = (int) ((color & 0xFF) * brightness);

    return (a << 24) | (r << 16) | (g << 8) | b;
}
