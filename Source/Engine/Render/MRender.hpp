// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_MRENDER_HPP_
#define _METADOT_MRENDER_HPP_

#include "Engine/Platforms/SDLWrapper.hpp"
#include "Engine/Render/renderer_gpu.h"
#include "Core/Core.hpp"
#include "box2d/b2_distance_joint.h"

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

    SDL_Color convertColor() {
        return {this->r, this->g, this->b, this->a};
    }
};

typedef struct
{
    stbtt_fontinfo *info;
    unsigned char *bitmap;
    float scale;
    int b_w;
    int b_h;
    int l_h;
} STBTTF_Font;

struct DrawTextParams_t
{
    const char *string;
    STBTTF_Font *font;
    int x;
    int y;
    uint8_t fR;
    uint8_t fG;
    uint8_t fB;
    //METAENGINE_Render_Image* t1 = nullptr;
    //METAENGINE_Render_Image* t2 = nullptr;
    int w = -1;
    int h = -1;
};

class Drawing {
public:
    static bool InitFont(C_GLContext *SDLContext);

    static STBTTF_Font *LoadFont(const char *path, UInt16 size);

    static DrawTextParams_t drawTextParams(METAENGINE_Render_Target *renderer, const char *string,
                                           STBTTF_Font *font, int x, int y,
                                           uint8_t fR, uint8_t fG, uint8_t fB, int align);

    static DrawTextParams_t drawTextParams(METAENGINE_Render_Target *renderer, const char *string,
                                           STBTTF_Font *font, int x, int y,
                                           uint8_t fR, uint8_t fG, uint8_t fB, bool shadow, int align);

    static void drawText(METAENGINE_Render_Target *renderer, const char *string,
                         STBTTF_Font *font, int x, int y,
                         uint8_t fR, uint8_t fG, uint8_t fB, int align);

    static void drawText(METAENGINE_Render_Target *renderer, const char *string,
                         STBTTF_Font *font, int x, int y,
                         uint8_t fR, uint8_t fG, uint8_t fB, bool shadow, int align);

    static void drawTextBG(METAENGINE_Render_Target *renderer, const char *string,
                           STBTTF_Font *font, int x, int y,
                           uint8_t fR, uint8_t fG, uint8_t fB, SDL_Color bgCol, int align);

    static void drawTextBG(METAENGINE_Render_Target *renderer, const char *string,
                           STBTTF_Font *font, int x, int y,
                           uint8_t fR, uint8_t fG, uint8_t fB, SDL_Color bgCol, bool shadow, int align);

    static void drawText(METAENGINE_Render_Target *renderer, DrawTextParams_t pm, int x, int y, int align);

    static void drawText(METAENGINE_Render_Target *renderer, DrawTextParams_t pm, int x, int y, bool shadow, int align);

    static b2Vec2 rotate_point(float cx, float cy, float angle, b2Vec2 p);

    static void drawPolygon(METAENGINE_Render_Target *renderer, SDL_Color col, b2Vec2 *verts, int x, int y, float scale, int count, float angle, float cx, float cy);

    static uint32 darkenColor(uint32 col, float brightness);
};

#endif