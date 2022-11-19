// Copyright(c) 2022, KaoruXun All rights reserved.


#ifndef INC_Item
#include "Item.hpp"
#endif// !INC_Item

#include "Game/Macros.hpp"

Item::Item() {}

Item::~Item() {
    METAENGINE_Render_FreeImage(texture);
    SDL_FreeSurface(surface);
}

Item *Item::makeItem(uint8_t flags, RigidBody *rb) {
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

UInt32 getpixel(C_Surface *surface, int x, int y) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    UInt8 *p = (UInt8 *) surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
        case 1:
            return *p;
            break;

        case 2:
            return *(UInt16 *) p;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
            break;

        case 4:
            return *(UInt32 *) p;
            break;

        default:
            return 0; /* shouldn't happen, but avoids warnings */
    }
}


void Item::loadFillTexture(C_Surface *tex) {
    fill.resize(capacity);
    uint32_t maxN = 0;
    for (uint16_t x = 0; x < tex->w; x++) {
        for (uint16_t y = 0; y < tex->h; y++) {

            uint32_t color = METADOT_GET_PIXEL(tex, x, y);

            // SDL_Color rgb;
            // UInt32 data = getpixel(tex, x, y);
            // SDL_GetRGB(data, tex->format, &rgb.r, &rgb.g, &rgb.b);

            // uint32_t j = data;

            // if (rgb.a > (UInt8)0)
            // {
            //     if (j - 1 > maxN) maxN = j - 1;
            //     fill.resize(maxN);
            //     fill[j - 1] = { x,y };
            // }

            if (((color >> 32) & 0xff) > 0) {
                uint32_t n = color & 0x00ffffff;

                //uint32_t t = (n * 100) / col;
                uint32_t t = n;

                fill[t - 1] = {x, y};
                if (t - 1 > maxN) maxN = t - 1;
            }
        }
    }
    fill.resize(maxN);
}
