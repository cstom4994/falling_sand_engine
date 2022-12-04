// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_MRENDER_HPP_
#define _METADOT_MRENDER_HPP_

#include "Core/Core.hpp"
#include "Engine/ImGuiImplement.hpp"
#include "Engine/Internal/BuiltinBox2d.h"
#include "Engine/RendererGPU.h"
#include "Engine/SDLWrapper.hpp"

#include <functional>
#include <unordered_map>

#define ALIGN_LEFT 0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT 2

#include "external/stb_rect_pack.h"
#include "external/stb_truetype.h"

class Drawing {
public:
    static void drawText(std::string name, std::string text, uint8_t x, uint8_t y,
                         ImVec4 col = {1.0f, 1.0f, 1.0f, 1.0f});
    static void drawTextEx(std::string name, uint8_t x, uint8_t y, std::function<void()> func);
    static b2Vec2 rotate_point(float cx, float cy, float angle, b2Vec2 p);
    static void drawPolygon(METAENGINE_Render_Target *renderer, METAENGINE_Color col, b2Vec2 *verts,
                            int x, int y, float scale, int count, float angle, float cx, float cy);
    static uint32 darkenColor(uint32 col, float brightness);
};

#endif