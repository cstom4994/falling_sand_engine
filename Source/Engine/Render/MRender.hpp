// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/Core.hpp"

// Includes Sokol GFX, Sokol GP and Sokol APP, doing all implementations.
#include "Libs/sokol/sokol_app.h"
#include "Libs/sokol/sokol_gfx.h"
#include "Libs/sokol/sokol_glue.h"
#include "Libs/sokol/sokol_gp.h"
//#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define SOKOL_IMGUI_IMPL
#include "Libs/ImGui/cimgui.h"
#include "Libs/sokol/sokol_imgui.h"

namespace MetaEngine {
    typedef class Color {
    public:
        UInt8 r, g, b, a;

    public:
        Color(UInt8 r, UInt8 g, UInt8 b, UInt8 a) : r(r), g(g), b(b), a(a) {}
        // Color(SDL_Color color) {
        //     r = color.r;
        //     g = color.g;
        //     b = color.b;
        //     a = color.a;
        // }

        // SDL_Color toSDLColor() { return SDL_Color{.r = r, .g = g, .b = b, .a = a}; }
    } Color;

}// namespace MetaEngine