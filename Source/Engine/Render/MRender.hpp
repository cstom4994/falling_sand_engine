// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/Core.hpp"

#include <SDL.h>

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

}// namespace MetaEngine