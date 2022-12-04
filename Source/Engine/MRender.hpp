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

namespace MetaEngine {
    typedef class Color {
    public:
        UInt8 r, g, b, a;

    public:
        Color(UInt8 r, UInt8 g, UInt8 b, UInt8 a) : r(r), g(g), b(b), a(a) {}
        Color(SDL_Color color) {
            r = color.r;
            g = color.g;
            b = color.b;
            a = color.a;
        }

        SDL_Color toSDLColor() { return SDL_Color{.r = r, .g = g, .b = b, .a = a}; }
    } Color;

    typedef class Rect {
    public:
        int x, y;
        int w, h;

    public:
        Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
        Rect(SDL_Rect rect) {
            x = rect.x;
            y = rect.y;
            w = rect.w;
            h = rect.h;
        }

        SDL_Rect toSDLRect() { return SDL_Rect{.x = x, .y = y, .w = w, .h = h}; }
    } Rect;

}// namespace MetaEngine

class METAENGINE_Color {
public:
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

public:
    METAENGINE_Color(uint8_t fr, uint8_t fg, uint8_t fb, uint8_t fa) : r(fr), g(fg), b(fb), a(fa) {}

    SDL_Color convertColor() { return {this->r, this->g, this->b, this->a}; }
};

class Drawing {
public:
    static void drawText(std::string name, std::string text, uint8_t x, uint8_t y,
                         ImVec4 col = {1.0f, 1.0f, 1.0f, 1.0f});
    static void drawTextEx(std::string name, uint8_t x, uint8_t y, std::function<void()> func);
    static b2Vec2 rotate_point(float cx, float cy, float angle, b2Vec2 p);
    static void drawPolygon(METAENGINE_Render_Target *renderer, SDL_Color col, b2Vec2 *verts, int x,
                            int y, float scale, int count, float angle, float cx, float cy);
    static uint32 darkenColor(uint32 col, float brightness);
};

#endif