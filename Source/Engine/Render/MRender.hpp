// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/Core.hpp"

#include "Engine/Render/SDLWrapper.hpp"
#include "SDL_rect.h"

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