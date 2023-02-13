#ifndef _METADOT_TTF_H_
#define _METADOT_TTF_H_

#include "libs/external/stb_truetype.h"

typedef struct {
    stbtt_fontinfo font;
    void *fontData;
    float ptsize;
    float scale;
    int baseline;
} ttf_Font;

ttf_Font *ttf_new(const void *data, int len);
void ttf_destroy(ttf_Font *self);
void ttf_ptsize(ttf_Font *self, float ptsize);
int ttf_height(ttf_Font *self);
int ttf_width(ttf_Font *self, const char *str);
void *ttf_render(ttf_Font *self, const char *str, int *w, int *h);

bool loadFontFromMemory(ttf_Font *font, const void *data, int len, int ptsize);

#endif