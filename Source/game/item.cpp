// Copyright(c) 2022, KaoruXun All rights reserved.

#include "item.hpp"

#include "core/core.h"
#include "core/macros.h"
#include "engine/renderer/renderer_utils.h"

Item::Item() {}

Item::~Item() {
    R_FreeImage(texture);
    SDL_FreeSurface(surface);
}

Item *Item::makeItem(U8 flags, RigidBody *rb) {
    Item *i;

    if (rb->item != NULL) {
        i = rb->item;
        i->surface = rb->surface;
        i->texture = rb->texture;
    } else {
        i = new Item();
        i->flags = flags;
        i->surface = rb->surface;
        i->texture = rb->texture;
    }

    return i;
}

U32 getpixel(C_Surface *surface, int x, int y) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    U8 *p = (U8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
        case 1:
            return *p;
            break;

        case 2:
            return *(U16 *)p;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
            break;

        case 4:
            return *(U32 *)p;
            break;

        default:
            return 0; /* shouldn't happen, but avoids warnings */
    }
}

void Item::loadFillTexture(C_Surface *tex) {
    fill.resize(capacity);
    U32 maxN = 0;
    for (U16 x = 0; x < tex->w; x++) {
        for (U16 y = 0; y < tex->h; y++) {

            U32 color = R_GET_PIXEL(tex, x, y);

            // SDL_Color rgb;
            // U32 data = getpixel(tex, x, y);
            // SDL_GetRGB(data, tex->format, &rgb.r, &rgb.g, &rgb.b);

            // U32 j = data;

            // if (rgb.a > (U8)0)
            // {
            //     if (j - 1 > maxN) maxN = j - 1;
            //     fill.resize(maxN);
            //     fill[j - 1] = { x,y };
            // }

            if (((color >> 32) & 0xff) > 0) {
                U32 n = color & 0x00ffffff;

                // U32 t = (n * 100) / col;
                U32 t = n;

                fill[t - 1] = {x, y};
                if (t - 1 > maxN) maxN = t - 1;
            }
        }
    }
    fill.resize(maxN);
}
