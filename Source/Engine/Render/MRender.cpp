// Copyright(c) 2022, KaoruXun All rights reserved.

#include "MRender.hpp"

#include "Game/InEngine.h"
#include "Engine/Memory/Memory.hpp"

#include "external/stb_image.h"
#include "external/stb_truetype.h"
#include "external/stb_rect_pack.h"

#include "imgui.h"
#include "imgui_internal.h"

void TextFuck(std::string text, uint8_t x, uint8_t y) {
    ImGui::SetNextWindowPos(ImVec2(x, y));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("text", NULL, flags);
    ImGui::Text(text.c_str());
    ImGui::End();
}

bool Drawing::InitFont(SDL_GLContext *SDLContext) {
    static SDL_GLContext *m_SDLContext;
    m_SDLContext = SDLContext;

    return true;
}

STBTTF_Font *Drawing::LoadFont(const char *path, UInt16 ss) {
    /* load font file */
    long size;
    unsigned char *fontBuffer;

    FILE *fontFile = fopen(path, "rb");
    fseek(fontFile, 0, SEEK_END);
    size = ftell(fontFile);       /* how long is the file ? */
    fseek(fontFile, 0, SEEK_SET); /* reset */

    fontBuffer = (unsigned char *) METAENGINE_MALLOC(size);

    fread(fontBuffer, size, 1, fontFile);
    fclose(fontFile);

    /* prepare font */
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, fontBuffer, 0)) {
        std::cout << "failed to init font" << std::endl;
    }

    STBTTF_Font *font = (STBTTF_Font *) METAENGINE_MALLOC(sizeof(STBTTF_Font));

    font->l_h = ss;
    font->b_h = 512;
    font->b_w = 512;

    /* create a bitmap for the phrase */
    unsigned char *bitmap = (unsigned char *) calloc(font->b_w * font->b_h, sizeof(unsigned char));

    font->info = &info;
    font->bitmap = bitmap;

    /* calculate font scaling */
    font->scale = stbtt_ScaleForPixelHeight(font->info, font->l_h);

    return font;
}

void Drawing::drawText(METAENGINE_Render_Target *target, const char *string,
                       STBTTF_Font *font, int x, int y,
                       uint8_t fR, uint8_t fG, uint8_t fB, int align) {
    drawText(target, string, font, x, y, fR, fG, fB, true, align);
}

void Drawing::drawText(METAENGINE_Render_Target *target, const char *string,
                       STBTTF_Font *font, int x, int y,
                       uint8_t fR, uint8_t fG, uint8_t fB, bool shadow, int align) {
    //ImGui::GetForegroundDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(x, y), ImColor(fR, fG, fB, 255), string, 0, 0.0f, 0);

    //TTF_CloseFont(font);
}

void Drawing::drawTextBG(METAENGINE_Render_Target *target, const char *string,
                         STBTTF_Font *font, int x, int y,
                         uint8_t fR, uint8_t fG, uint8_t fB, SDL_Color bgCol, int align) {
    drawTextBG(target, string, font, x, y, fR, fG, fB, bgCol, true, align);
}

void Drawing::drawTextBG(METAENGINE_Render_Target *target, const char *string,
                         STBTTF_Font *font, int x, int y,
                         uint8_t fR, uint8_t fG, uint8_t fB, SDL_Color bgCol, bool shadow, int align) {
    //ImGui::GetForegroundDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(x, y), ImColor(fR, fG, fB, 255), string, 0, 0.0f, 0);

    //TTF_CloseFont(font);
}

DrawTextParams_t Drawing::drawTextParams(METAENGINE_Render_Target *renderer, const char *string,
                                         STBTTF_Font *font, int x, int y,
                                         uint8_t fR, uint8_t fG, uint8_t fB, int align) {
    return drawTextParams(renderer, string, font, x, y, fR, fG, fB, true, align);
}

DrawTextParams_t Drawing::drawTextParams(METAENGINE_Render_Target *renderer, const char *string,
                                         STBTTF_Font *font, int x, int y,
                                         uint8_t fR, uint8_t fG, uint8_t fB, bool shadow, int align) {
    //SDL_SetRenderDrawColor(NULL, fR, fG, fB, 255);
    //STBTTF_RenderText(NULL, font, x, y, string);

    //return DrawTextParams{ .string = string, .font = font, .x = x, .y = y, .fR = fR, .fG = fG, .fB = fB, .w = 64,.h = 64 };

    DrawTextParams_t dtp{.string = string, .font = font, .x = x, .y = y, .fR = fR, .fG = fG, .fB = fB, .w = 64, .h = 64};

    return dtp;
    //TTF_CloseFont(font);
}

void Drawing::drawText(METAENGINE_Render_Target *target, DrawTextParams_t pm, int x, int y, int align) {
    drawText(target, pm, x, y, true, align);
}

void Drawing::drawText(METAENGINE_Render_Target *target, DrawTextParams_t pm, int x, int y, bool shadow, int align) {
    //if (shadow) {
    //
    //    METAENGINE_Render_Blit(pm.t1, NULL, target, x + 1 - align * pm.w / 2.0f + pm.w / 2.0f, y + 1 + pm.h / 2.0f);
    //

    //    //SDL_FreeSurface(textSurface);
    //    //SDL_DestroyTexture(Message);
    //}

    //{
    //
    //    METAENGINE_Render_Blit(pm.t2, NULL, target, x - align * pm.w / 2.0f + pm.w / 2.0f, y + pm.h / 2.0f);
    //

    //    //SDL_FreeSurface(textSurface);
    //    //SDL_DestroyTexture(Message);
    //}

    //TTF_CloseFont(font);
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

void Drawing::drawPolygon(METAENGINE_Render_Target *target, SDL_Color col, b2Vec2 *verts, int x, int y, float scale, int count, float angle, float cx, float cy) {
    if (count < 2)
        return;
    b2Vec2 last = rotate_point(cx, cy, angle, verts[count - 1]);
    for (int i = 0; i < count; i++) {
        b2Vec2 rot = rotate_point(cx, cy, angle, verts[i]);
        METAENGINE_Render_Line(target, x + last.x * scale, y + last.y * scale, x + rot.x * scale, y + rot.y * scale, col);
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

