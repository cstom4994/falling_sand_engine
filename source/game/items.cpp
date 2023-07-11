// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "game/items.hpp"

#include "game/player.hpp"

Item::Item() {}

Item::~Item() {
    ME_ASSERT_E(this->image);
    ME_ASSERT_E(this->surface);
}

Item *Item::makeItem(ItemFlags flags, RigidBody *rb, std::string n) {
    Item *i;

    if (rb->item != NULL) {
        i = rb->item;
    } else {
        i = alloc<Item>::safe_malloc();
        i->flags = flags;
    }

    i->surface = rb->get_surface();
    i->image = rb->get_texture();
    i->name = n;

    return i;
}

void Item::deleteItem(Item *item) {
    if (item != NULL) {
        if (item->image != NULL) R_FreeImage(item->image);
        if (item->surface != NULL) SDL_FreeSurface(item->surface);
        // ME_DELETE(item, Item);
    }
}

u32 getpixel(C_Surface *surface, int x, int y) {
    int bpp = surface->format->BytesPerPixel;

    u8 *p = (u8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
        case 1:
            return *p;
            break;

        case 2:
            return *(u16 *)p;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
            break;

        case 4:
            return *(u32 *)p;
            break;

        default:
            return 0;  // 不会触发
    }
}

void Item::loadFillTexture(C_Surface *tex) {
    fill.resize(capacity);
    u32 maxN = 0;
    for (u16 x = 0; x < tex->w; x++) {
        for (u16 y = 0; y < tex->h; y++) {
            u32 col = getpixel(tex, x, y);
            if (((col >> 24) & 0xff) > 0) {
                u32 n = col & 0x00ffffff;
                fill[n - 1] = {x, y};
                if (n - 1 > maxN) maxN = n - 1;
            }
        }
    }
    fill.resize(maxN);
}