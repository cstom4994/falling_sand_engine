// Copyright(c) 2022, KaoruXun All rights reserved.

#include "EngineFuncWrap.hpp"
#include "Core/Global.hpp"
#include "Engine/LuaWrapper.hpp"
#include "Engine/PlatformDetail.h"
#include "Engine/RendererGPU.h"
#include "Engine/SDLWrapper.hpp"
#include "Engine/Scripting.hpp"
#include "Game/Game.hpp"

#include "lz4/lz4.h"
#include "lz4/lz4frame.h"
#include "lz4/lz4hc.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#ifndef METADOT_PLATFORM_WINDOWS
#include <dirent.h>
#include <unistd.h>
#define f_mkdir mkdir
#else
#include <direct.h>

#define f_mkdir(a, b) _mkdir(a)
#endif

namespace TestData {
    bool shaderOn = false;

    int pixelScale = DEFAULT_SCALE;
    int setPixelScale = DEFAULT_SCALE;

    R_Image *buffer;
    // R_Target *renderer;
    // R_Target *bufferTarget;

    UInt8 palette[COLOR_LIMIT][3] = INIT_COLORS;

    int paletteNum = 0;

    int drawOffX = 0;
    int drawOffY = 0;

    int windowWidth;
    int windowHeight;
    int drawX = 0;
    int drawY = 0;

    int lastWindowX = 0;
    int lastWindowY = 0;

    void assessWindow() {
        int winW = 0;
        int winH = 0;
        SDL_GetWindowSize(global.platform.window, &winW, &winH);

        int candidateOne = winH / SCRN_HEIGHT;
        int candidateTwo = winW / SCRN_WIDTH;

        if (winW != 0 && winH != 0) {
            pixelScale = (candidateOne > candidateTwo) ? candidateTwo : candidateOne;
            windowWidth = winW;
            windowHeight = winH;

            drawX = (windowWidth - pixelScale * SCRN_WIDTH) / 2;
            drawY = (windowHeight - pixelScale * SCRN_HEIGHT) / 2;

            R_SetWindowResolution(winW, winH);
        }
    }

    char *appPath = nullptr;
    char *scriptsPath;

    char currentWorkingDirectory[MAX_PATH];
    char lastOpenedPath[MAX_PATH];

    struct fileHandleType
    {
        FILE *fileStream;
        bool open;
        bool canWrite;
        bool eof;
    };
}// namespace TestData

#pragma region BindImage

#define clamp(v, min, max) (float) ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))

#define off(o, t) ((float) (o) -TestData::drawOffX), (float) ((t) -TestData::drawOffY)

struct imageType
{
    int width;
    int height;
    bool free;
    //        int clr;
    int lastRenderNum;
    int remap[COLOR_LIMIT];
    bool remapped;
    char **internalRep;
    SDL_Surface *surface;
    R_Image *texture;
};

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
UInt32 rMask = 0xff000000;
UInt32 gMask = 0x00ff0000;
UInt32 bMask = 0x0000ff00;
UInt32 aMask = 0x000000ff;
#else
UInt32 rMask = 0x000000ff;
UInt32 gMask = 0x0000ff00;
UInt32 bMask = 0x00ff0000;
UInt32 aMask = 0xff000000;
#endif

static char getColor(lua_State *L, int arg) {
    int color = luaL_checkinteger(L, arg) - 1;
    return static_cast<char>(
            color == -2
                    ? -1
                    : (color < 0 ? 0 : (color > (COLOR_LIMIT - 1) ? (COLOR_LIMIT - 1) : color)));
}

static UInt32 getRectC(imageType *data, int colorGiven) {
    int color = data->remap[colorGiven];// Palette remapping
    return SDL_MapRGBA(data->surface->format, TestData::palette[color][0],
                       TestData::palette[color][1], TestData::palette[color][2], 255);
}

static imageType *checkImage(lua_State *L) {
    void *ud = luaL_checkudata(L, 1, "MetaDot.Image");
    luaL_argcheck(L, ud != nullptr, 1, "`Image` expected");
    return (imageType *) ud;
}

bool freeCheck(lua_State *L, imageType *data) {
    if (data->free) {
        luaL_error(L, "Attempt to perform Image operation but Image was freed");
        return false;
    }
    return true;
}

static int newImage(lua_State *L) {
    int w = luaL_checkinteger(L, 1);
    int h = luaL_checkinteger(L, 2);
    size_t nbytes = sizeof(imageType);
    auto *a = (imageType *) lua_newuserdata(L, nbytes);

    luaL_getmetatable(L, "MetaDot.Image");
    lua_setmetatable(L, -2);

    a->width = w;
    a->height = h;
    a->free = false;
    //        a->clr = 0;
    a->lastRenderNum = TestData::paletteNum;
    for (int i = 0; i < COLOR_LIMIT; i++) { a->remap[i] = i; }

    a->internalRep = new char *[w];
    for (int i = 0; i < w; i++) {
        a->internalRep[i] = new char[h];
        memset(a->internalRep[i], -1, static_cast<size_t>(h));
    }

    a->surface = SDL_CreateRGBSurface(0, w, h, 32, rMask, gMask, bMask, aMask);

    // Init to black color
    SDL_FillRect(a->surface, nullptr, SDL_MapRGBA(a->surface->format, 0, 0, 0, 0));

    a->texture = R_CopyImageFromSurface(a->surface);
    R_SetImageFilter(a->texture, R_FILTER_NEAREST);
    R_SetSnapMode(a->texture, R_SNAP_NONE);

    return 1;
}

static int flushImage(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    R_Rect rect = {0, 0, (float) data->width, (float) data->height};
    R_UpdateImage(data->texture, &rect, data->surface, &rect);

    return 0;
}

static int renderImage(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    if (data->lastRenderNum != TestData::paletteNum || data->remapped) {
        UInt32 rectColor = SDL_MapRGBA(data->surface->format, 0, 0, 0, 0);
        SDL_FillRect(data->surface, nullptr, rectColor);

        for (int x = 0; x < data->width; x++) {
            for (int y = 0; y < data->height; y++) {
                SDL_Rect rect = {x, y, 1, 1};
                int c = data->internalRep[x][y];
                if (c >= 0) SDL_FillRect(data->surface, &rect, getRectC(data, c));
            }
        }

        R_Rect rect = {0, 0, (float) data->width, (float) data->height};
        R_UpdateImage(data->texture, &rect, data->surface, &rect);

        data->lastRenderNum = TestData::paletteNum;
        data->remapped = false;
    }

    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);

    R_Rect rect = {off(x, y)};

    int top = lua_gettop(L);
    if (top > 7) {
        R_Rect srcRect = {(float) luaL_checkinteger(L, 4), (float) luaL_checkinteger(L, 5),
                          clamp(luaL_checkinteger(L, 6), 0, data->width),
                          clamp(luaL_checkinteger(L, 7), 0, data->height)};

        int scale = luaL_checkinteger(L, 8);

        rect.w = srcRect.w * scale;
        rect.h = srcRect.h * scale;

        R_BlitRect(data->texture, &srcRect, global.game->RenderTarget_.target, &rect);
    } else if (top > 6) {
        R_Rect srcRect = {(float) luaL_checkinteger(L, 4), (float) luaL_checkinteger(L, 5),
                          (float) luaL_checkinteger(L, 6), (float) luaL_checkinteger(L, 7)};

        rect.w = srcRect.w;
        rect.h = srcRect.h;

        R_BlitRect(data->texture, &srcRect, global.game->RenderTarget_.target, &rect);
    } else if (top > 3) {
        R_Rect srcRect = {0, 0, (float) luaL_checkinteger(L, 4), (float) luaL_checkinteger(L, 5)};

        rect.w = srcRect.w;
        rect.h = srcRect.h;

        R_BlitRect(data->texture, &srcRect, global.game->RenderTarget_.target, &rect);
    } else {
        rect.w = data->width;
        rect.h = data->height;

        R_BlitRect(data->texture, nullptr, global.game->RenderTarget_.target, &rect);
    }

    return 0;
}

static int freeImage(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    for (int i = 0; i < data->width; i++) { delete data->internalRep[i]; }
    delete data->internalRep;

    SDL_FreeSurface(data->surface);
    R_FreeImage(data->texture);
    data->free = true;

    return 0;
}

static void internalDrawPixel(imageType *data, int x, int y, char c) {
    if (x >= 0 && y >= 0 && x < data->width && y < data->height) { data->internalRep[x][y] = c; }
}

static int imageGetPixel(lua_State *L) {
    imageType *data = checkImage(L);

    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);

    if (x < data->width && x >= 0 && y < data->height && y >= 0) {

        lua_pushinteger(L, data->internalRep[x][y] + 1);
    } else {
        lua_pushinteger(L, 0);
    }

    return 1;
}

static int imageRemap(lua_State *L) {
    imageType *data = checkImage(L);

    int oldColor = getColor(L, 2);
    int newColor = getColor(L, 3);

    data->remap[oldColor] = newColor;
    data->remapped = true;

    return 1;
}

static int imageDrawPixel(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);

    char color = getColor(L, 4);

    if (color >= 0) {
        UInt32 rectColor = getRectC(data, color);
        SDL_Rect rect = {x, y, 1, 1};
        SDL_FillRect(data->surface, &rect, rectColor);
        internalDrawPixel(data, x, y, color);
    }
    return 0;
}

static int imageDrawRectangle(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int w = luaL_checkinteger(L, 4);
    int h = luaL_checkinteger(L, 5);

    char color = getColor(L, 6);

    if (color >= 0) {
        UInt32 rectColor = getRectC(data, color);
        SDL_Rect rect = {x, y, w, h};
        SDL_FillRect(data->surface, &rect, rectColor);
        for (int xp = x; xp < x + w; xp++) {
            for (int yp = y; yp < y + h; yp++) { internalDrawPixel(data, xp, yp, color); }
        }
    }
    return 0;
}

static int imageBlitPixels(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int w = luaL_checkinteger(L, 4);
    int h = luaL_checkinteger(L, 5);

    unsigned long long amt = lua_rawlen(L, -1);
    int len = w * h;
    if (amt < len) {
        luaL_error(L, "blitPixels expected %d pixels, got %d", len, amt);
        return 0;
    }

    for (int i = 1; i <= len; i++) {
        lua_pushnumber(L, i);
        lua_gettable(L, -2);
        if (!lua_isnumber(L, -1)) { luaL_error(L, "Index %d is non-numeric", i); }
        auto color = static_cast<int>(lua_tointeger(L, -1) - 1);
        if (color == -1) { continue; }

        color = color == -2
                        ? -1
                        : (color < 0 ? 0 : (color > (COLOR_LIMIT - 1) ? (COLOR_LIMIT - 1) : color));

        if (color >= 0) {
            int xp = (i - 1) % w;
            int yp = (i - 1) / w;

            UInt32 rectColor = getRectC(data, color);
            SDL_Rect rect = {x + xp, y + yp, 1, 1};
            SDL_FillRect(data->surface, &rect, rectColor);
            internalDrawPixel(data, x + xp, y + yp, static_cast<char>(color));
        }

        lua_pop(L, 1);
    }

    return 0;
}

static int imageClear(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    UInt32 rectColor = SDL_MapRGBA(data->surface->format, 0, 0, 0, 0);
    SDL_FillRect(data->surface, nullptr, rectColor);
    for (int xp = 0; xp < data->width; xp++) {
        for (int yp = 0; yp < data->height; yp++) { internalDrawPixel(data, xp, yp, 0); }
    }

    return 0;
}

static int imageCopy(lua_State *L) {
    imageType *src = checkImage(L);

    void *ud = luaL_checkudata(L, 2, "MetaDot.Image");
    luaL_argcheck(L, ud != nullptr, 1, "`Image` expected");
    auto *dst = (imageType *) ud;

    int x = luaL_checkinteger(L, 3);
    int y = luaL_checkinteger(L, 4);
    int wi;
    int he;

    SDL_Rect srcRect;

    if (lua_gettop(L) > 4) {
        wi = luaL_checkinteger(L, 5);
        he = luaL_checkinteger(L, 6);
        //srcRect = { luaL_checkinteger(L, 7), luaL_checkinteger(L, 8), wi, he };
        srcRect.x = luaL_checkinteger(L, 7);
        srcRect.y = luaL_checkinteger(L, 8);
        srcRect.w = wi;
        srcRect.h = he;
    } else {
        //srcRect = { 0, 0, src->width, src->height };
        srcRect.x = 0;
        srcRect.y = 0;
        srcRect.w = src->width;
        srcRect.h = src->height;
        wi = src->width;
        he = src->height;
    }

    SDL_Rect rect = {x, y, wi, he};
    SDL_BlitSurface(src->surface, &srcRect, dst->surface, &rect);
    for (int xp = srcRect.x; xp < srcRect.x + srcRect.w; xp++) {
        for (int yp = srcRect.y; yp < srcRect.y + srcRect.h; yp++) {
            if (xp >= src->width || xp < 0 || yp >= src->height || yp < 0) { continue; }
            internalDrawPixel(dst, xp, yp, src->internalRep[xp][yp]);
        }
    }

    return 0;
}

static int imageToString(lua_State *L) {
    imageType *data = checkImage(L);
    if (data->free) {
        lua_pushstring(L, "Image(freed)");
    } else {
        lua_pushfstring(L, "Image(%dx%d)", data->width, data->height);
    }
    return 1;
}

static int imageGetWidth(lua_State *L) {
    imageType *data = checkImage(L);

    lua_pushinteger(L, data->width);
    return 1;
}

static int imageGetHeight(lua_State *L) {
    imageType *data = checkImage(L);

    lua_pushinteger(L, data->height);
    return 1;
}

static const luaL_Reg imageLib[] = {{"newImage", newImage}, {nullptr, nullptr}};

static const luaL_Reg imageLib_m[] = {{"__tostring", imageToString},
                                      {"free", freeImage},
                                      {"flush", flushImage},
                                      {"render", renderImage},
                                      {"clear", imageClear},
                                      {"drawPixel", imageDrawPixel},
                                      {"drawRectangle", imageDrawRectangle},
                                      {"blitPixels", imageBlitPixels},
                                      {"getPixel", imageGetPixel},
                                      {"remap", imageRemap},
                                      {"copy", imageCopy},
                                      {"getWidth", imageGetWidth},
                                      {"getHeight", imageGetHeight},
                                      {nullptr, nullptr}};

int metadot_bind_image(lua_State *L) {

    luaL_newmetatable(L, "metadot.image");

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2); /* pushes the metatable */
    lua_settable(L, -3);  /* metatable.__index = metatable */
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, freeImage);
    lua_settable(L, -3);

    LuaWrapper::metadot_load(L, imageLib_m, "_metadot_image_m");
    LuaWrapper::metadot_load(L, imageLib, "_metadot_image");

    return 1;
}

#pragma endregion BindImage

#pragma region BindGPU

#define off(o, t) (float) ((o) -TestData::drawOffX), (float) ((t) -TestData::drawOffY)

static int gpu_getColor(lua_State *L, int arg) {
    int color = luaL_checkinteger(L, arg) - 1;
    return color < 0 ? 0 : (color > (COLOR_LIMIT - 1) ? (COLOR_LIMIT - 1) : color);
}

static int gpu_draw_pixel(lua_State *L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);

    int color = gpu_getColor(L, 3);

    METAENGINE_Color colorS = {TestData::palette[color][0], TestData::palette[color][1],
                               TestData::palette[color][2], 255};

    R_RectangleFilled(global.game->RenderTarget_.target, off(x, y), off(x + 1, y + 1), colorS);

    return 0;
}

static int gpu_draw_rectangle(lua_State *L) {
    int color = gpu_getColor(L, 5);

    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);

    R_Rect rect = {off(x, y), (float) luaL_checkinteger(L, 3), (float) luaL_checkinteger(L, 4)};

    METAENGINE_Color colorS = {TestData::palette[color][0], TestData::palette[color][1],
                               TestData::palette[color][2], 255};
    R_RectangleFilled2(global.game->RenderTarget_.target, rect, colorS);

    return 0;
}

static int gpu_blit_pixels(lua_State *L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int w = luaL_checkinteger(L, 3);
    int h = luaL_checkinteger(L, 4);

    unsigned long long amt = lua_rawlen(L, -1);
    int len = w * h;
    if (amt < len) {
        luaL_error(L, "blitPixels expected %d pixels, got %d", len, amt);
        return 0;
    }

    for (int i = 1; i <= len; i++) {
        lua_pushnumber(L, i);
        lua_gettable(L, -2);
        if (!lua_isnumber(L, -1)) { luaL_error(L, "Index %d is non-numeric", i); }
        int color = (int) lua_tointeger(L, -1) - 1;
        if (color == -1) {
            lua_pop(L, 1);
            continue;
        }

        color = color < 0 ? 0 : (color > (COLOR_LIMIT - 1) ? (COLOR_LIMIT - 1) : color);

        int xp = (i - 1) % w;
        int yp = (i - 1) / w;

        R_Rect rect = {off(x + xp, y + yp), 1, 1};

        METAENGINE_Color colorS = {TestData::palette[color][0], TestData::palette[color][1],
                                   TestData::palette[color][2], 255};
        R_RectangleFilled2(global.game->RenderTarget_.target, rect, colorS);

        lua_pop(L, 1);
    }

    return 0;
}

static int gpu_set_clipping(lua_State *L) {
    if (lua_gettop(L) == 0) {
        R_SetClip(TestData::buffer->target, 0, 0, TestData::buffer->w, TestData::buffer->h);
        return 0;
    }

    auto x = static_cast<Sint16>(luaL_checkinteger(L, 1));
    auto y = static_cast<Sint16>(luaL_checkinteger(L, 2));
    auto w = static_cast<UInt16>(luaL_checkinteger(L, 3));
    auto h = static_cast<UInt16>(luaL_checkinteger(L, 4));

    R_SetClip(TestData::buffer->target, x, y, w, h);

    return 0;
}

static int gpu_set_palette_color(lua_State *L) {
    int slot = gpu_getColor(L, 1);

    auto r = static_cast<UInt8>(luaL_checkinteger(L, 2));
    auto g = static_cast<UInt8>(luaL_checkinteger(L, 3));
    auto b = static_cast<UInt8>(luaL_checkinteger(L, 4));

    TestData::palette[slot][0] = r;
    TestData::palette[slot][1] = g;
    TestData::palette[slot][2] = b;
    TestData::paletteNum++;

    return 0;
}

static int gpu_blit_palette(lua_State *L) {
    auto amt = (char) lua_rawlen(L, -1);
    if (amt < 1) { return 0; }

    amt = static_cast<char>(amt > COLOR_LIMIT ? COLOR_LIMIT : amt);

    for (int i = 1; i <= amt; i++) {
        lua_pushnumber(L, i);
        lua_gettable(L, -2);

        if (lua_type(L, -1) == LUA_TNUMBER) {
            lua_pop(L, 1);
            continue;
        }

        lua_pushnumber(L, 1);
        lua_gettable(L, -2);
        TestData::palette[i - 1][0] = static_cast<UInt8>(luaL_checkinteger(L, -1));

        lua_pushnumber(L, 2);
        lua_gettable(L, -3);
        TestData::palette[i - 1][1] = static_cast<UInt8>(luaL_checkinteger(L, -1));

        lua_pushnumber(L, 3);
        lua_gettable(L, -4);
        TestData::palette[i - 1][2] = static_cast<UInt8>(luaL_checkinteger(L, -1));

        lua_pop(L, 4);
    }

    TestData::paletteNum++;

    return 0;
}

static int gpu_get_palette(lua_State *L) {
    lua_newtable(L);

    for (int i = 0; i < COLOR_LIMIT; i++) {
        lua_pushinteger(L, i + 1);
        lua_newtable(L);
        for (int j = 0; j < 3; j++) {
            lua_pushinteger(L, j + 1);
            lua_pushinteger(L, TestData::palette[i][j]);
            lua_rawset(L, -3);
        }
        lua_rawset(L, -3);
    }

    return 1;
}

static int gpu_get_pixel(lua_State *L) {
    auto x = static_cast<Sint16>(luaL_checkinteger(L, 1));
    auto y = static_cast<Sint16>(luaL_checkinteger(L, 2));
    METAENGINE_Color col = R_GetPixel(TestData::buffer->target, x, y);
    for (int i = 0; i < COLOR_LIMIT; i++) {
        UInt8 *pCol = TestData::palette[i];
        if (col.r == pCol[0] && col.g == pCol[1] && col.b == pCol[2]) {
            lua_pushinteger(L, i + 1);
            return 1;
        }
    }

    lua_pushinteger(L, 1);// Should never happen
    return 1;
}

static int gpu_clear(lua_State *L) {
    if (lua_gettop(L) > 0) {
        int color = gpu_getColor(L, 1);
        METAENGINE_Color colorS = {TestData::palette[color][0], TestData::palette[color][1],
                                   TestData::palette[color][2], 255};
        R_ClearColor(global.game->RenderTarget_.target, colorS);
    } else {
        METAENGINE_Color colorS = {TestData::palette[0][0], TestData::palette[0][1],
                                   TestData::palette[0][2], 255};
        R_ClearColor(global.game->RenderTarget_.target, colorS);
    }

    return 0;
}

int *translateStack;
int tStackUsed = 0;
int tStackSize = 32;

static int gpu_translate(lua_State *L) {
    TestData::drawOffX -= luaL_checkinteger(L, -2);
    TestData::drawOffY -= luaL_checkinteger(L, -1);

    return 0;
}

static int gpu_push(lua_State *L) {
    if (tStackUsed == tStackSize) {
        tStackSize *= 2;
        translateStack = (int *) realloc(translateStack, tStackSize * sizeof(int));
    }

    translateStack[tStackUsed] = TestData::drawOffX;
    translateStack[tStackUsed + 1] = TestData::drawOffY;

    tStackUsed += 2;

    return 0;
}

static int gpu_pop(lua_State *L) {
    if (tStackUsed > 0) {
        tStackUsed -= 2;

        TestData::drawOffX = translateStack[tStackUsed];
        TestData::drawOffY = translateStack[tStackUsed + 1];

        lua_pushboolean(L, true);
        lua_pushinteger(L, tStackUsed / 2);
        return 2;
    } else {
        lua_pushboolean(L, false);
        return 1;
    }
}

static int gpu_set_fullscreen(lua_State *L) {
    auto fsc = static_cast<bool>(lua_toboolean(L, 1));
    if (!R_SetFullscreen(fsc, true)) {
        TestData::pixelScale = TestData::setPixelScale;
        R_SetWindowResolution(TestData::pixelScale * SCRN_WIDTH,
                              TestData::pixelScale * SCRN_HEIGHT);

        SDL_SetWindowPosition(global.platform.window, TestData::lastWindowX, TestData::lastWindowY);
    }

    TestData::assessWindow();

    return 0;
}

static int gpu_swap(lua_State *L) {
    METAENGINE_Color colorS = {TestData::palette[0][0], TestData::palette[0][1],
                               TestData::palette[0][2], 255};
    R_ClearColor(global.game->RenderTarget_.realTarget, colorS);

    //TestData::shader::updateShader();

    R_BlitScale(TestData::buffer, nullptr, global.game->RenderTarget_.realTarget,
                TestData::windowWidth / 2, TestData::windowHeight / 2, TestData::pixelScale,
                TestData::pixelScale);

    R_Flip(global.game->RenderTarget_.realTarget);

    R_DeactivateShaderProgram();

    return 0;
}

static const luaL_Reg gpuLib[] = {{"setPaletteColor", gpu_set_palette_color},
                                  {"blitPalette", gpu_blit_palette},
                                  {"getPalette", gpu_get_palette},
                                  {"drawPixel", gpu_draw_pixel},
                                  {"drawRectangle", gpu_draw_rectangle},
                                  {"blitPixels", gpu_blit_pixels},
                                  {"translate", gpu_translate},
                                  {"push", gpu_push},
                                  {"pop", gpu_pop},
                                  {"setFullscreen", gpu_set_fullscreen},
                                  {"getPixel", gpu_get_pixel},
                                  {"clear", gpu_clear},
                                  {"swap", gpu_swap},
                                  {"clip", gpu_set_clipping},
                                  {nullptr, nullptr}};

int metadot_bind_gpu(lua_State *L) {
    translateStack = new int[32];

    LuaWrapper::metadot_load(L, gpuLib, "_metadot_gpu");
    lua_pushnumber(L, SCRN_WIDTH);
    lua_setfield(L, -2, "width");
    lua_pushnumber(L, SCRN_HEIGHT);
    lua_setfield(L, -2, "height");

    return 1;
}

#pragma endregion BindGPU

#pragma region BindFS

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delim)) { result.push_back(item); }

    return result;
}

bool checkPath(const char *luaInput, char *varName) {
    // Normalize the path
    auto luaPath = std::string(luaInput);
    std::replace(luaPath.begin(), luaPath.end(), '/', PATH_SEPARATOR);

    char *workingFront = (luaPath[0] == PATH_SEPARATOR) ? TestData::scriptsPath
                                                        : TestData::currentWorkingDirectory;
    std::string sysPath = std::string() + workingFront + PATH_SEPARATOR + luaPath;

    // Remove consecutive path seperators (e.g. a//b///c => a/b/c)
    sysPath.erase(
            std::unique(sysPath.begin(), sysPath.end(),
                        [](const char a, const char b) { return a == b && b == PATH_SEPARATOR; }),
            sysPath.end());

    auto pathComponents = split(sysPath, PATH_SEPARATOR);

    auto builder = std::vector<std::string>();
    // For each component, evaluate it's effect
    for (auto iter = pathComponents.begin(); iter != pathComponents.end(); ++iter) {
        auto comp = *iter;
        if (comp == ".") continue;
        if (comp == "..") {
            builder.pop_back();
            continue;
        }

        builder.push_back(comp);
    }

    // Now collapse the path
    auto actualPath = std::string();
    for (auto iter = builder.begin();;) {
        actualPath += *iter;
        if (++iter == builder.end()) {
            break;
        } else {
            actualPath += PATH_SEPARATOR;
        }
    }

    auto scPath = std::string(TestData::scriptsPath);
    if (actualPath.compare(0, scPath.size(), scPath)) {
        // Maybe actualPath was missing the final / that is sometimes present in scPath
        actualPath += PATH_SEPARATOR;

        if (actualPath.compare(0, scPath.size(), scPath)) return true;
    }

    sprintf(varName, "%s", actualPath.c_str());

    return false;// success
}

static TestData::fileHandleType *checkFsObj(lua_State *L) {
    void *ud = luaL_checkudata(L, 1, "MetaDot.fsObj");
    luaL_argcheck(L, ud != nullptr, 1, "`FileHandle` expected");
    return (TestData::fileHandleType *) ud;
}

static int fsGetAttr(lua_State *L) {
    char filePath[MAX_PATH + 1];
    if (checkPath(luaL_checkstring(L, 1), filePath)) {
        lua_pushinteger(L, 0b11111111);
        return 1;
    }

    if (filePath[0] == 0) {
        lua_pushinteger(L, 0b11111111);
        return 1;
    }

#ifdef __WINDOWS__
    unsigned long attr = GetFileAttributes(filePath);

    if (attr == INVALID_FILE_ATTRIBUTES) {
        lua_pushinteger(L, 0b11111111);

        return 1;
    }

    int attrOut = ((attr & FILE_ATTRIBUTE_READONLY) != 0) * 0b00000001 +
                  ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0) * 0b00000010;

    lua_pushinteger(L, attrOut);

    return 1;
#else
    struct stat statbuf
    {
    };
    if (stat(filePath, &statbuf) != 0) {
        lua_pushinteger(L, 0b11111111);
        return 1;
    }

    int attrOut = ((access(filePath, W_OK) * 0b00000001) + (S_ISDIR(statbuf.st_mode) * 0b00000010));

    lua_pushinteger(L, attrOut);
    return 1;
#endif
}

static int fsList(lua_State *L) {
    char filePath[MAX_PATH + 1];
    if (checkPath(luaL_checkstring(L, 1), filePath)) { return 0; }

    char dummy[MAX_PATH + 1];
    auto preRoot = std::string(luaL_checkstring(L, 1)) + PATH_SEPARATOR + "..";
    bool isAtRoot = checkPath(preRoot.c_str(), dummy);

    if (filePath[0] == 0) { return 0; }

#ifdef __WINDOWS__
    WIN32_FIND_DATA fdFile;
    HANDLE hFind = NULL;

    char sPath[2048];

    //Specify a file mask. *.* = We want everything!
    sprintf(sPath, "%s\\*.*", filePath);

    if ((hFind = FindFirstFile(sPath, &fdFile)) == INVALID_HANDLE_VALUE) { return 0; }

    lua_newtable(L);
    int i = 1;

    lua_pushinteger(L, i);
    lua_pushstring(L, ".");
    lua_rawset(L, -3);
    i++;

    if (!isAtRoot) {
        lua_pushinteger(L, i);
        lua_pushstring(L, "..");
        lua_rawset(L, -3);
        i++;
    }

    do {
        //Build up our file path using the passed in

        if (strcmp(fdFile.cFileName, ".") != 0 && strcmp(fdFile.cFileName, "..") != 0) {
            lua_pushinteger(L, i);
            lua_pushstring(L, fdFile.cFileName);
            lua_rawset(L, -3);
            i++;
        }
    } while (FindNextFile(hFind, &fdFile));//Find the next file.

    FindClose(hFind);//Always, Always, clean things up!

    return 1;
#else
    DIR *dp;
    struct dirent *ep;

    dp = opendir(filePath);
    if (dp != nullptr) {
        lua_newtable(L);
        int i = 1;

        while ((ep = readdir(dp))) {
            if (isAtRoot && strcmp(ep->d_name, "..") == 0)
                continue;// Don't tell them they can go beneath root

            lua_pushinteger(L, i);
            lua_pushstring(L, ep->d_name);
            lua_rawset(L, -3);
            i++;
        }
        closedir(dp);

        return 1;
    } else {
        return 0;
    }
#endif
}

static int fsOpenFile(lua_State *L) {
    char filePath[MAX_PATH + 1];
    if (checkPath(luaL_checkstring(L, 1), filePath)) { return 0; }

    if (filePath[0] == 0) { return 0; }

    const char *mode = luaL_checkstring(L, 2);

    FILE *fileHandle_o;
    if (mode[0] == 'w' || mode[0] == 'r' || mode[0] == 'a') {
        if (mode[1] == '+') {
            if (mode[2] == 'b' || mode[2] == 0) {
                fileHandle_o = fopen(filePath, mode);
            } else {
                return luaL_error(L, "invalid file mode");
            }
        } else if (mode[1] == 'b' && strlen(mode) == 2) {
            fileHandle_o = fopen(filePath, mode);
        } else if (strlen(mode) == 1) {
            fileHandle_o = fopen(filePath, mode);
        } else {
            return luaL_error(L, "invalid file mode");
        }
    } else {
        return luaL_error(L, "invalid file mode");
    }

    if (fileHandle_o == nullptr) {
        lua_pushboolean(L, false);
        std::string error;
        switch (errno) {
#define s(s) error = std::string(s)
#define b break
            case EINVAL:
                s("Invalid mode '") + mode + "'";
                b;
            case EACCES:
                s("Permission denied");
                b;
            case EEXIST:
                s("File aready exists");
                b;
            case EFAULT:
                s("File is outside accessible address space");
                b;
            case EINTR:
                s("Open was interrupted by a signal handler");
                b;
            case EISDIR:
                s("") + luaL_checkstring(L, 1) + " is a directory";
                b;
            case ENFILE:
            case EMFILE:
                s("File descriptor limit has been reached");
                b;
            case ENAMETOOLONG:
                s("Pathname is too long");
                b;
            case ENOENT:
                s("File does not exist");
                b;
            case ENOMEM:
                s("Insufficient kernel memory avaliable");
                b;
            case ENOSPC:
                s("No room for new file");
                b;
            case ENOTDIR:
                s("A component used as a directory in path is not");
                b;
            case EROFS:
                s("File on a read-only filesystem");
                b;
#ifndef __WINDOWS__
            case EDQUOT:
                s("File does not exist, and quota of disk blocks or inodes has been exhausted");
                b;
            case ELOOP:
                s("Too many symbolic links in path resolution");
                b;
            case EOVERFLOW:
#endif
            case EFBIG:
                s("File is too large to be opened");
                b;
            default:
                s("Unknown error occurred");
                b;
#undef s
#undef b
        }

        lua_pushlstring(L, error.c_str(), error.size());

        return 2;
    }

    strcpy((char *) &TestData::lastOpenedPath, filePath + strlen(TestData::scriptsPath));
    for (int i = 0; TestData::lastOpenedPath[i] != '\0'; i++) {
        if (TestData::lastOpenedPath[i] == '\\') TestData::lastOpenedPath[i] = '/';
    }

    size_t nbytes = sizeof(TestData::fileHandleType);
    auto *outObj = (TestData::fileHandleType *) lua_newuserdata(L, nbytes);

    luaL_getmetatable(L, "MetaDot.fsObj");
    lua_setmetatable(L, -2);

    outObj->fileStream = fileHandle_o;
    outObj->open = true;
    outObj->canWrite = mode[0] != 'r';
    outObj->eof = false;

    return 1;
}

static int fsLastOpened(lua_State *L) {
    lua_pushstring(L, (char *) &TestData::lastOpenedPath);

    return 1;
}

static int fsObjWrite(lua_State *L) {
    TestData::fileHandleType *data = checkFsObj(L);

    if (!data->open) return luaL_error(L, "file handle was closed");

    if (!data->canWrite) return luaL_error(L, "file is not open for writing");

    size_t strSize;
    const char *toWrite = luaL_checklstring(L, 2, &strSize);

    size_t written = fwrite(toWrite, 1, strSize, data->fileStream);

    lua_pushboolean(L, written == strSize);

    return 1;
}

static int fsObjFlush(lua_State *L) {
    TestData::fileHandleType *data = checkFsObj(L);

    if (!data->open) return luaL_error(L, "file handle was closed");

    if (!data->canWrite) return luaL_error(L, "file is not open for writing");

    int ret = fflush(data->fileStream);

    lua_pushboolean(L, ret == 0);

    return 1;
}

long lineStrLen(const char *s, long max) {
    for (long i = 0; i < max; i++) {
        if (s[i] == '\n' || s[i] == '\r') { return i + 1; }
    }

    return max;
}

static int fsObjRead(lua_State *L) {
    TestData::fileHandleType *data = checkFsObj(L);

    if (!data->open) return luaL_error(L, "file handle was closed");

    if (data->canWrite) return luaL_error(L, "file is open for writing");

    if (data->eof) {
        lua_pushnil(L);
        return 1;
    }

    if (feof(data->fileStream)) {
        data->eof = true;
        lua_pushnil(L);
        return 1;
    }

    int num = lua_gettop(L);
    if (num == 0) {
        // Read line

        size_t bufLen = sizeof(char) * (FS_LINE_INCR + 1);
        auto *dataBuf = new char[bufLen];

        long st = ftell(data->fileStream);

        if (fgets(dataBuf, FS_LINE_INCR, data->fileStream) == nullptr) {
            lua_pushstring(L, "");
            delete[] dataBuf;
            return 1;
        }

        long dataBufLen = lineStrLen(dataBuf, FS_LINE_INCR);
        if (feof(data->fileStream)) {
            data->eof = true;
            dataBufLen = ftell(data->fileStream);
            lua_pushlstring(L, dataBuf, static_cast<size_t>(dataBufLen - st));
            delete[] dataBuf;
            return 1;
        } else if (dataBuf[dataBufLen - 1] == '\n' || dataBuf[dataBufLen - 1] == '\r') {
            lua_pushlstring(L, dataBuf, static_cast<size_t>(dataBufLen - 1));
            delete[] dataBuf;
            return 1;
        }

        for (int i = 1;; i++) {
            bufLen += sizeof(char) * FS_LINE_INCR;
            dataBuf = (char *) realloc(dataBuf, bufLen);
            if (dataBuf == nullptr)
                return luaL_error(L, "unable to allocate enough memory for read operation");

            st = ftell(data->fileStream);

            if (fgets(dataBuf + i * (FS_LINE_INCR - 1) * sizeof(char), FS_LINE_INCR,
                      data->fileStream) == nullptr) {
                lua_pushstring(L, "");
                delete[] dataBuf;
                return 1;
            }

            dataBufLen = lineStrLen(dataBuf, bufLen / sizeof(char));
            if (feof(data->fileStream)) {
                data->eof = true;
                dataBufLen = ftell(data->fileStream);
                lua_pushlstring(L, dataBuf, static_cast<size_t>(dataBufLen - st));
                delete[] dataBuf;
                return 1;
            } else if (dataBuf[dataBufLen - 1] == '\n') {
                lua_pushlstring(L, dataBuf, static_cast<size_t>(dataBufLen - 1));
                delete[] dataBuf;
                return 1;
            }
        }
    }

    int type = lua_type(L, 1 - num);
    if (type == LUA_TSTRING) {
        const char *mode = luaL_checkstring(L, 2);

        if (mode[0] == '*') {
            if (mode[1] == 'a') {
                // All

                fseek(data->fileStream, 0, SEEK_END);
                long lSize = ftell(data->fileStream);
                rewind(data->fileStream);

                auto *dataBuf = new char[lSize + 1];

                size_t result = fread(dataBuf, 1, static_cast<size_t>(lSize), data->fileStream);

                dataBuf[result] = 0;

                lua_pushlstring(L, dataBuf, result);

                data->eof = true;

                delete[] dataBuf;

                return 1;
            } else if (mode[1] == 'l') {
                // Read line

                size_t bufLen = sizeof(char) * (FS_LINE_INCR + 1);
                auto *dataBuf = new char[bufLen];

                long st = ftell(data->fileStream);

                if (fgets(dataBuf, FS_LINE_INCR, data->fileStream) == nullptr) {
                    lua_pushstring(L, "");
                    delete[] dataBuf;
                    return 1;
                }

                long dataBufLen = lineStrLen(dataBuf, FS_LINE_INCR);
                if (feof(data->fileStream)) {
                    data->eof = true;
                    dataBufLen = ftell(data->fileStream);
                    lua_pushlstring(L, dataBuf, static_cast<size_t>(dataBufLen - st));
                    delete[] dataBuf;
                    return 1;
                } else if (dataBuf[dataBufLen - 1] == '\n' || dataBuf[dataBufLen - 1] == '\r') {
                    lua_pushlstring(L, dataBuf, static_cast<size_t>(dataBufLen - 1));
                    delete[] dataBuf;
                    return 1;
                }

                for (int i = 1;; i++) {
                    bufLen += sizeof(char) * FS_LINE_INCR;
                    dataBuf = (char *) realloc(dataBuf, bufLen);
                    if (dataBuf == nullptr)
                        return luaL_error(L, "unable to allocate enough memory for read operation");

                    st = ftell(data->fileStream);

                    if (fgets(dataBuf + i * (FS_LINE_INCR - 1) * sizeof(char), FS_LINE_INCR,
                              data->fileStream) == nullptr) {
                        lua_pushstring(L, "");
                        delete[] dataBuf;
                        return 1;
                    }

                    dataBufLen = lineStrLen(dataBuf, bufLen / sizeof(char));
                    if (feof(data->fileStream)) {
                        data->eof = true;
                        dataBufLen = ftell(data->fileStream);
                        lua_pushlstring(L, dataBuf, static_cast<size_t>(dataBufLen - st));
                        delete[] dataBuf;
                        return 1;
                    } else if (dataBuf[dataBufLen - 1] == '\n' || dataBuf[dataBufLen - 1] == '\r') {
                        lua_pushlstring(L, dataBuf, static_cast<size_t>(dataBufLen - 1));
                        delete[] dataBuf;
                        return 1;
                    }
                }
            } else {
                return luaL_argerror(L, 2, "invalid mode");
            }
        } else {
            return luaL_argerror(L, 2, "invalid mode");
        }
    } else if (type == LUA_TNUMBER) {
        auto len = (int) (luaL_checkinteger(L, 2));

        // Read 'len' bytes

        auto *dataBuf = new char[len + 1];

        size_t result = fread(dataBuf, sizeof(char), static_cast<size_t>(len), data->fileStream);

        dataBuf[result] = 0;

        lua_pushlstring(L, dataBuf, result);

        delete[] dataBuf;

        return 1;
    } else {
        const char *typeN = lua_typename(L, type);
        size_t len = 16 + strlen(typeN);
        auto *eMsg = new char[len];
        sprintf(eMsg, "%s was unexpected", typeN);
        delete[] eMsg;

        return luaL_argerror(L, 2, eMsg);
    }
}

static int fsMkDir(lua_State *L) {
    char filePath[MAX_PATH + 1];
    if (checkPath(luaL_checkstring(L, 1), filePath)) {
        lua_pushboolean(L, false);
        return 1;
    }

    if (filePath[0] == 0) {
        lua_pushboolean(L, false);
        return 1;
    }

    lua_pushboolean(L, f_mkdir(filePath, 0777) + 1);
    return 1;
}

static int fsMv(lua_State *L) {
    char filePath[MAX_PATH + 1];
    if (checkPath(luaL_checkstring(L, 1), filePath)) {
        lua_pushboolean(L, false);
        return 1;
    }

    if (filePath[0] == 0) {
        lua_pushboolean(L, false);
        return 1;
    }

    char endPath[MAX_PATH + 1];
    if (checkPath(luaL_checkstring(L, 2), endPath)) {
        lua_pushboolean(L, false);
        return 1;
    }

    if (endPath[0] == 0) {
        lua_pushboolean(L, false);
        return 1;
    }

    lua_pushboolean(L, rename(filePath, endPath) + 1);
    return 1;
}

static int fsDelete(lua_State *L) {
    char filePath[MAX_PATH + 1];
    if (checkPath(luaL_checkstring(L, 1), filePath)) {
        lua_pushboolean(L, false);
        return 1;
    }

    if (filePath[0] == 0) {
        lua_pushboolean(L, false);
        return 1;
    }

    if (remove(filePath) == 0) lua_pushboolean(L, true);
    else
        lua_pushboolean(L, rmdir(filePath) + 1);

    return 1;
}

static int fsObjCloseHandle(lua_State *L) {
    TestData::fileHandleType *data = checkFsObj(L);

    if (data->open) fclose(data->fileStream);

    data->open = false;

    return 0;
}

static int fsGetCWD(lua_State *L) {
    char *path = TestData::currentWorkingDirectory + strlen(TestData::scriptsPath);
    if (path[0] == 0) {
        lua_pushstring(L, "/");
    } else {
        lua_pushstring(L, path);
    }

    return 1;
}

static int fsSetCWD(lua_State *L) {
    const char *nwd = luaL_checkstring(L, 1);

    if (nwd[0] == '\\' || nwd[0] == '/') {
        size_t nwdLen = strlen(nwd);

        size_t ln = (int) (strlen(TestData::scriptsPath) + nwdLen + 2);
        auto *concatStr = new char[ln];
        sprintf(concatStr, "%s/%s", TestData::scriptsPath, nwd);

        auto *fPath = new char[MAX_PATH];

        getFullPath(concatStr, fPath);

        delete[] concatStr;

        if (strlen(fPath) < MAX_PATH) {
            strncpy(TestData::currentWorkingDirectory, fPath, strlen(fPath));
            TestData::currentWorkingDirectory[strlen(fPath)] = 0;
        }

        delete[] fPath;
    } else {
        size_t nwdlen = strlen(nwd);

        auto ln = strlen(TestData::currentWorkingDirectory) + nwdlen + 2;
        auto *concatStr = new char[ln];
        sprintf(concatStr, "%s/%s", TestData::currentWorkingDirectory, nwd);

        auto *fPath = new char[MAX_PATH];

        getFullPath(concatStr, fPath);

        delete[] concatStr;

        if (strlen(fPath) < MAX_PATH) {
            strncpy(TestData::currentWorkingDirectory, fPath, strlen(fPath));
            TestData::currentWorkingDirectory[strlen(fPath)] = 0;
        }

        delete[] fPath;
    }

    return 0;
}

static int fsGetClipboardText(lua_State *L) {
    if (SDL_HasClipboardText()) {
        char *text = SDL_GetClipboardText();

        lua_pushstring(L, text);

        SDL_free(text);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

static int fsSetClipboardText(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);

    SDL_SetClipboardText(text);

    return 0;
}

static const luaL_Reg fsLib[] = {{"getAttr", fsGetAttr},
                                 {"open", fsOpenFile},
                                 {"list", fsList},
                                 {"delete", fsDelete},
                                 {"mkdir", fsMkDir},
                                 {"move", fsMv},
                                 {"setCWD", fsSetCWD},
                                 {"getCWD", fsGetCWD},
                                 {"getLastFile", fsLastOpened},
                                 {"getClipboard", fsGetClipboardText},
                                 {"setClipboard", fsSetClipboardText},
                                 {nullptr, nullptr}};

static const luaL_Reg fsLib_m[] = {{"read", fsObjRead},
                                   {"write", fsObjWrite},
                                   {"close", fsObjCloseHandle},
                                   {"flush", fsObjFlush},
                                   {nullptr, nullptr}};

int metadot_bind_fs(lua_State *L) {
    auto *fPath = new char[MAX_PATH];
    getFullPath(TestData::scriptsPath, fPath);

    if (strlen(fPath) < MAX_PATH) strncpy(TestData::currentWorkingDirectory, fPath, strlen(fPath));
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to initialize cwd");
        return 2;
    }

    delete[] fPath;

    luaL_newmetatable(L, "MetaDot.fsObj");

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2); /* pushes the metatable */
    lua_settable(L, -3);  /* metatable.__index = metatable */
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, fsObjCloseHandle);
    lua_settable(L, -3);

    LuaWrapper::metadot_load(L, fsLib_m, "_metadot_fs_m");
    LuaWrapper::metadot_load(L, fsLib, "_metadot_fs");

    return 1;
}

#pragma endregion BindFS

#pragma region BindLZ4

#define LZ4_DICTSIZE 65536
#define DEF_BUFSIZE 65536
#define MIN_BUFFSIZE 1024

#if LUA_VERSION_NUM < 502
#define luaL_newlib(L, function_table)                                                             \
    do {                                                                                           \
        lua_newtable(L);                                                                           \
        luaL_register(L, NULL, function_table);                                                    \
    } while (0)
#endif

#if LUA_VERSION_NUM >= 502
#define LUABUFF_NEW(lua_buff, c_buff, max_size)                                                    \
    luaL_Buffer lua_buff;                                                                          \
    char *c_buff = luaL_buffinitsize(L, &lua_buff, max_size);
#define LUABUFF_FREE(c_buff)
#define LUABUFF_PUSH(lua_buff, c_buff, size) luaL_pushresultsize(&lua_buff, size);
#else
#define LUABUFF_NEW(lua_buff, c_buff, max_size)                                                    \
    char *c_buff = malloc(max_size);                                                               \
    if (c_buff == NULL) return luaL_error(L, "out of memory");
#define LUABUFF_FREE(c_buff) free(c_buff);
#define LUABUFF_PUSH(lua_buff, c_buff, size)                                                       \
    lua_pushlstring(L, c_buff, size);                                                              \
    free(c_buff);
#endif

#define RING_POLICY_APPEND 0
#define RING_POLICY_RESET 1
#define RING_POLICY_EXTERNAL 2

static int _ring_policy(int buffer_size, int buffer_position, int data_size) {
    if (data_size > buffer_size || data_size > LZ4_DICTSIZE) return RING_POLICY_EXTERNAL;
    if (buffer_position + data_size <= buffer_size) return RING_POLICY_APPEND;
    if (data_size + LZ4_DICTSIZE > buffer_position) return RING_POLICY_EXTERNAL;
    return RING_POLICY_RESET;
}

static int _lua_table_optinteger(lua_State *L, int table_index, const char *field_name, int value) {
    int type;
    lua_getfield(L, table_index, field_name);
    type = lua_type(L, -1);
    if (type != LUA_TNIL) {
        if (type != LUA_TNUMBER) luaL_error(L, "field '%s' must be a number", field_name);
        value = lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
    return value;
}

static int _lua_table_optboolean(lua_State *L, int table_index, const char *field_name, int value) {
    int type;
    lua_getfield(L, table_index, field_name);
    type = lua_type(L, -1);
    if (type != LUA_TNIL) {
        if (type != LUA_TBOOLEAN) luaL_error(L, "field '%s' must be a boolean", field_name);
        value = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);
    return value;
}

/*****************************************************************************
 * Frame
 ****************************************************************************/

static int lz4_compress(lua_State *L) {
    size_t in_len;
    const char *in = luaL_checklstring(L, 1, &in_len);
    size_t bound, r;

    LZ4F_preferences_t stack_settings;
    LZ4F_preferences_t *settings = NULL;

    if (lua_type(L, 2) == LUA_TTABLE) {
        memset(&stack_settings, 0, sizeof(stack_settings));
        settings = &stack_settings;
        settings->compressionLevel = _lua_table_optinteger(L, 2, "compression_level", 0);
        settings->autoFlush = _lua_table_optboolean(L, 2, "auto_flush", 0);
        settings->frameInfo.blockSizeID =
                (LZ4F_blockSizeID_t) _lua_table_optinteger(L, 2, "block_size", 0);
        settings->frameInfo.blockMode = _lua_table_optboolean(L, 2, "block_independent", 0)
                                                ? LZ4F_blockIndependent
                                                : LZ4F_blockLinked;
        settings->frameInfo.contentChecksumFlag = _lua_table_optboolean(L, 2, "content_checksum", 0)
                                                          ? LZ4F_contentChecksumEnabled
                                                          : LZ4F_noContentChecksum;
    }

    bound = LZ4F_compressFrameBound(in_len, settings);

    {
        LUABUFF_NEW(b, out, bound)
        r = LZ4F_compressFrame(out, bound, in, in_len, settings);
        if (LZ4F_isError(r)) {
            LUABUFF_FREE(out)
            return luaL_error(L, "compression failed: %s", LZ4F_getErrorName(r));
        }
        LUABUFF_PUSH(b, out, r)
    }

    return 1;
}

static int lz4_decompress(lua_State *L) {
    size_t in_len;
    const char *in = luaL_checklstring(L, 1, &in_len);
    const char *p = in;
    size_t p_len = in_len;

    LZ4F_decompressionContext_t ctx = NULL;
    LZ4F_frameInfo_t info;
    LZ4F_errorCode_t code;

    code = LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION);
    if (LZ4F_isError(code)) goto decompression_failed;

    {
        luaL_Buffer b;
        luaL_buffinit(L, &b);
        while (1) {
#if LUA_VERSION_NUM >= 502
            size_t out_len = 65536;
            char *out = luaL_prepbuffsize(&b, out_len);
#else
            size_t out_len = LUAL_BUFFERSIZE;
            char *out = luaL_prepbuffer(&b);
#endif
            size_t advance = p_len;
            code = LZ4F_decompress(ctx, out, &out_len, p, &advance, NULL);
            if (LZ4F_isError(code)) goto decompression_failed;
            if (out_len == 0) break;
            p += advance;
            p_len -= advance;
            luaL_addsize(&b, out_len);
        }
        luaL_pushresult(&b);
    }

    LZ4F_freeDecompressionContext(ctx);

    return 1;

decompression_failed:
    if (ctx != NULL) LZ4F_freeDecompressionContext(ctx);
    return luaL_error(L, "decompression failed: %s", LZ4F_getErrorName(code));
}

/*****************************************************************************
 * Block
 ****************************************************************************/

static int lz4_block_compress(lua_State *L) {
    size_t in_len;
    const char *in = luaL_checklstring(L, 1, &in_len);
    int accelerate = luaL_optinteger(L, 2, 0);
    int bound, r;

    if (in_len > LZ4_MAX_INPUT_SIZE)
        return luaL_error(L, "input longer than %d", LZ4_MAX_INPUT_SIZE);

    bound = LZ4_compressBound(in_len);

    {
        LUABUFF_NEW(b, out, bound)
        r = LZ4_compress_fast(in, out, in_len, bound, accelerate);
        if (r == 0) {
            LUABUFF_FREE(out)
            return luaL_error(L, "compression failed");
        }
        LUABUFF_PUSH(b, out, r)
    }

    return 1;
}

static int lz4_block_compress_hc(lua_State *L) {
    size_t in_len;
    const char *in = luaL_checklstring(L, 1, &in_len);
    int level = luaL_optinteger(L, 2, 0);
    int bound, r;

    if (in_len > LZ4_MAX_INPUT_SIZE)
        return luaL_error(L, "input longer than %d", LZ4_MAX_INPUT_SIZE);

    bound = LZ4_compressBound(in_len);

    {
        LUABUFF_NEW(b, out, bound)
        r = LZ4_compress_HC(in, out, in_len, bound, level);
        if (r == 0) {
            LUABUFF_FREE(out)
            return luaL_error(L, "compression failed");
        }
        LUABUFF_PUSH(b, out, r)
    }

    return 1;
}

static int lz4_block_decompress_safe(lua_State *L) {
    size_t in_len;
    const char *in = luaL_checklstring(L, 1, &in_len);
    int out_len = luaL_checkinteger(L, 2);
    int r;

    LUABUFF_NEW(b, out, out_len)
    r = LZ4_decompress_safe(in, out, in_len, out_len);
    if (r < 0) {
        LUABUFF_FREE(out)
        return luaL_error(L, "corrupt input or need more output space");
    }
    LUABUFF_PUSH(b, out, r)

    return 1;
}

static int lz4_block_decompress_fast(lua_State *L) {
    size_t in_len;
    const char *in = luaL_checklstring(L, 1, &in_len);
    int out_len = luaL_checkinteger(L, 2);

    {
        int r;
        LUABUFF_NEW(b, out, out_len)
        r = LZ4_decompress_fast(in, out, out_len);
        if (r < 0) {
            LUABUFF_FREE(out)
            return luaL_error(L, "corrupt input or incorrect output length");
        }
        LUABUFF_PUSH(b, out, out_len)
    }

    return 1;
}

static int lz4_block_decompress_safe_partial(lua_State *L) {
    size_t in_len;
    const char *in = luaL_checklstring(L, 1, &in_len);
    int target_len = luaL_checkinteger(L, 2);
    int out_len = luaL_checkinteger(L, 3);
    int r;

    LUABUFF_NEW(b, out, out_len)
    r = LZ4_decompress_safe_partial(in, out, in_len, target_len, out_len);
    if (r < 0) {
        LUABUFF_FREE(out)
        return luaL_error(L, "corrupt input or need more output space");
    }
    if (target_len < r) r = target_len;
    LUABUFF_PUSH(b, out, r)

    return 1;
}

/*****************************************************************************
 * Compression Stream
 ****************************************************************************/

typedef struct
{
    LZ4_stream_t handle;
    int accelerate;
    int buffer_size;
    int buffer_position;
    char *buffer;
} lz4_compress_stream_t;

static lz4_compress_stream_t *_checkcompressionstream(lua_State *L, int index) {
    return (lz4_compress_stream_t *) luaL_checkudata(L, index, "lz4.compression_stream");
}

static int lz4_cs_reset(lua_State *L) {
    lz4_compress_stream_t *cs = _checkcompressionstream(L, 1);
    size_t in_len = 0;
    const char *in = luaL_optlstring(L, 2, NULL, &in_len);

    if (in != NULL && in_len > 0) {
        int limit_len = LZ4_DICTSIZE;
        if (limit_len > cs->buffer_size) limit_len = cs->buffer_size;
        if (in_len > limit_len) {
            in = in + in_len - limit_len;
            in_len = limit_len;
        }
        memcpy(cs->buffer, in, in_len);
        cs->buffer_position = LZ4_loadDict(&cs->handle, cs->buffer, in_len);
    } else {
        LZ4_resetStream(&cs->handle);
        cs->buffer_position = 0;
    }

    lua_pushinteger(L, cs->buffer_position);

    return 1;
}

static int lz4_cs_compress(lua_State *L) {
    lz4_compress_stream_t *cs = _checkcompressionstream(L, 1);
    size_t in_len;
    const char *in = luaL_checklstring(L, 2, &in_len);
    size_t bound = LZ4_compressBound(in_len);
    int policy = _ring_policy(cs->buffer_size, cs->buffer_position, in_len);
    int r;

    LUABUFF_NEW(b, out, bound)

    if (policy == RING_POLICY_APPEND || policy == RING_POLICY_RESET) {
        char *ring;
        if (policy == RING_POLICY_APPEND) {
            ring = cs->buffer + cs->buffer_position;
            cs->buffer_position += in_len;
        } else {// RING_POLICY_RESET
            ring = cs->buffer;
            cs->buffer_position = in_len;
        }
        memcpy(ring, in, in_len);
        r = LZ4_compress_fast_continue(&cs->handle, ring, out, in_len, bound, cs->accelerate);
        if (r == 0) {
            LUABUFF_FREE(out)
            return luaL_error(L, "compression failed");
        }
    } else {// RING_POLICY_EXTERNAL
        r = LZ4_compress_fast_continue(&cs->handle, in, out, in_len, bound, cs->accelerate);
        if (r == 0) {
            LUABUFF_FREE(out)
            return luaL_error(L, "compression failed");
        }
        cs->buffer_position = LZ4_saveDict(&cs->handle, cs->buffer, cs->buffer_size);
    }

    LUABUFF_PUSH(b, out, r)

    return 1;
}

static int lz4_cs_tostring(lua_State *L) {
    lz4_compress_stream_t *p = _checkcompressionstream(L, 1);
    lua_pushfstring(L, "lz4.compression_stream (%p)", p);
    return 1;
}

static int lz4_cs_gc(lua_State *L) {
    lz4_compress_stream_t *p = _checkcompressionstream(L, 1);
    free(p->buffer);
    return 0;
}

static const luaL_Reg compress_stream_functions[] = {
        {"reset", lz4_cs_reset},
        {"compress", lz4_cs_compress},
        {NULL, NULL},
};

static int lz4_new_compression_stream(lua_State *L) {
    int buffer_size = luaL_optinteger(L, 1, DEF_BUFSIZE);
    int accelerate = luaL_optinteger(L, 2, 1);
    lz4_compress_stream_t *p;

    if (buffer_size < MIN_BUFFSIZE) buffer_size = MIN_BUFFSIZE;

    p = (lz4_compress_stream_t *) lua_newuserdata(L, sizeof(lz4_compress_stream_t));
    LZ4_resetStream(&p->handle);
    p->accelerate = accelerate;
    p->buffer_size = buffer_size;
    p->buffer_position = 0;
    p->buffer = (char *) malloc(buffer_size);
    if (p->buffer == NULL) luaL_error(L, "out of memory");

    if (luaL_newmetatable(L, "lz4.compression_stream")) {
        // new method table
        luaL_newlib(L, compress_stream_functions);
        // metatable.__index = method table
        lua_setfield(L, -2, "__index");

        // metatable.__tostring
        lua_pushcfunction(L, lz4_cs_tostring);
        lua_setfield(L, -2, "__tostring");

        // metatable.__gc
        lua_pushcfunction(L, lz4_cs_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);

    return 1;
}

/*****************************************************************************
 * Compression Stream HC
 ****************************************************************************/

typedef struct
{
    LZ4_streamHC_t handle;
    int level;
    int buffer_size;
    int buffer_position;
    char *buffer;
} lz4_compress_stream_hc_t;

static lz4_compress_stream_hc_t *_checkcompressionstream_hc(lua_State *L, int index) {
    return (lz4_compress_stream_hc_t *) luaL_checkudata(L, index, "lz4.compression_stream_hc");
}

static int lz4_cs_hc_reset(lua_State *L) {
    lz4_compress_stream_hc_t *cs = _checkcompressionstream_hc(L, 1);
    size_t in_len = 0;
    const char *in = luaL_optlstring(L, 2, NULL, &in_len);

    if (in != NULL && in_len > 0) {
        int limit_len = LZ4_DICTSIZE;
        if (limit_len > cs->buffer_size) limit_len = cs->buffer_size;
        if (in_len > limit_len) {
            in = in + in_len - limit_len;
            in_len = limit_len;
        }
        memcpy(cs->buffer, in, in_len);
        cs->buffer_position = LZ4_loadDictHC(&cs->handle, cs->buffer, in_len);
    } else {
        LZ4_resetStreamHC(&cs->handle, cs->level);
        cs->buffer_position = 0;
    }

    lua_pushinteger(L, cs->buffer_position);

    return 1;
}

static int lz4_cs_hc_compress(lua_State *L) {
    lz4_compress_stream_hc_t *cs = _checkcompressionstream_hc(L, 1);
    size_t in_len;
    const char *in = luaL_checklstring(L, 2, &in_len);
    size_t bound = LZ4_compressBound(in_len);
    int policy = _ring_policy(cs->buffer_size, cs->buffer_position, in_len);
    int r;

    LUABUFF_NEW(b, out, bound)

    if (policy == RING_POLICY_APPEND || policy == RING_POLICY_RESET) {
        char *ring;
        if (policy == RING_POLICY_APPEND) {
            ring = cs->buffer + cs->buffer_position;
            cs->buffer_position += in_len;
        } else {// RING_POLICY_RESET
            ring = cs->buffer;
            cs->buffer_position = in_len;
        }
        memcpy(ring, in, in_len);
        r = LZ4_compress_HC_continue(&cs->handle, ring, out, in_len, bound);
        if (r == 0) {
            LUABUFF_FREE(out)
            return luaL_error(L, "compression failed");
        }
    } else {// RING_POLICY_EXTERNAL
        r = LZ4_compress_HC_continue(&cs->handle, in, out, in_len, bound);
        if (r == 0) {
            LUABUFF_FREE(out)
            return luaL_error(L, "compression failed");
        }
        cs->buffer_position = LZ4_saveDictHC(&cs->handle, cs->buffer, cs->buffer_size);
    }

    LUABUFF_PUSH(b, out, r)

    return 1;
}

static int lz4_cs_hc_tostring(lua_State *L) {
    lz4_compress_stream_hc_t *p = _checkcompressionstream_hc(L, 1);
    lua_pushfstring(L, "lz4.compression_stream_hc (%p)", p);
    return 1;
}

static int lz4_cs_hc_gc(lua_State *L) {
    lz4_compress_stream_hc_t *p = _checkcompressionstream_hc(L, 1);
    free(p->buffer);
    return 0;
}

static const luaL_Reg compress_stream_hc_functions[] = {
        {"reset", lz4_cs_hc_reset},
        {"compress", lz4_cs_hc_compress},
        {NULL, NULL},
};

static int lz4_new_compression_stream_hc(lua_State *L) {
    int buffer_size = luaL_optinteger(L, 1, DEF_BUFSIZE);
    int level = luaL_optinteger(L, 2, 0);
    lz4_compress_stream_hc_t *p;

    if (buffer_size < MIN_BUFFSIZE) buffer_size = MIN_BUFFSIZE;

    p = (lz4_compress_stream_hc_t *) lua_newuserdata(L, sizeof(lz4_compress_stream_hc_t));
    LZ4_resetStreamHC(&p->handle, level);
    p->level = level;
    p->buffer_size = buffer_size;
    p->buffer_position = 0;
    p->buffer = (char *) malloc(buffer_size);
    if (p->buffer == NULL) luaL_error(L, "out of memory");

    if (luaL_newmetatable(L, "lz4.compression_stream_hc")) {
        // new method table
        luaL_newlib(L, compress_stream_hc_functions);
        // metatable.__index = method table
        lua_setfield(L, -2, "__index");

        // metatable.__tostring
        lua_pushcfunction(L, lz4_cs_hc_tostring);
        lua_setfield(L, -2, "__tostring");

        // metatable.__gc
        lua_pushcfunction(L, lz4_cs_hc_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);

    return 1;
}

/*****************************************************************************
 * Decompression Stream
 ****************************************************************************/

typedef struct
{
    LZ4_streamDecode_t handle;
    int buffer_size;
    int buffer_position;
    char *buffer;
} lz4_decompress_stream_t;

static lz4_decompress_stream_t *_checkdecompressionstream(lua_State *L, int index) {
    return (lz4_decompress_stream_t *) luaL_checkudata(L, index, "lz4.decompression_stream");
}

static int lz4_ds_reset(lua_State *L) {
    lz4_decompress_stream_t *ds = _checkdecompressionstream(L, 1);
    size_t in_len = 0;
    const char *in = luaL_optlstring(L, 2, NULL, &in_len);

    if (in != NULL && in_len > 0) {
        int limit_len = LZ4_DICTSIZE;
        if (limit_len > ds->buffer_size) limit_len = ds->buffer_size;
        if (in_len > limit_len) {
            in = in + in_len - limit_len;
            in_len = limit_len;
        }
        memcpy(ds->buffer, in, in_len);
        ds->buffer_position = LZ4_setStreamDecode(&ds->handle, ds->buffer, in_len);
    } else {
        LZ4_setStreamDecode(&ds->handle, NULL, 0);
        ds->buffer_position = 0;
    }

    lua_pushinteger(L, ds->buffer_position);

    return 1;
}

static void _lz4_ds_save_dict(lz4_decompress_stream_t *ds, const char *dict, int dict_size) {
    int limit_len = LZ4_DICTSIZE;
    if (limit_len > ds->buffer_size) limit_len = ds->buffer_size;

    if (dict_size > limit_len) {
        dict += dict_size - limit_len;
        dict_size = limit_len;
    }

    memmove(ds->buffer, dict, dict_size);
    LZ4_setStreamDecode(&ds->handle, ds->buffer, dict_size);

    ds->buffer_position = dict_size;
}

static int lz4_ds_decompress_safe(lua_State *L) {
    lz4_decompress_stream_t *ds = _checkdecompressionstream(L, 1);
    size_t in_len;
    const char *in = luaL_checklstring(L, 2, &in_len);
    size_t out_len = luaL_checkinteger(L, 3);
    int policy = _ring_policy(ds->buffer_size, ds->buffer_position, out_len);
    int r;

    if (policy == RING_POLICY_APPEND || policy == RING_POLICY_RESET) {
        char *ring;
        size_t new_position;
        if (policy == RING_POLICY_APPEND) {
            ring = ds->buffer + ds->buffer_position;
            new_position = ds->buffer_position + out_len;
        } else {// RING_POLICY_RESET
            ring = ds->buffer;
            new_position = out_len;
        }
        r = LZ4_decompress_safe_continue(&ds->handle, in, ring, in_len, out_len);
        if (r < 0) return luaL_error(L, "corrupt input or need more output space");
        ds->buffer_position = new_position;
        lua_pushlstring(L, ring, r);
    } else {// RING_POLICY_EXTERNAL
        LUABUFF_NEW(b, out, out_len)
        r = LZ4_decompress_safe_continue(&ds->handle, in, out, in_len, out_len);
        if (r < 0) {
            LUABUFF_FREE(out)
            return luaL_error(L, "corrupt input or need more output space");
        }
        _lz4_ds_save_dict(ds, out, r);// memcpy(ds->buffer, out, out_len)
        LUABUFF_PUSH(b, out, r)
    }

    return 1;
}

static int lz4_ds_decompress_fast(lua_State *L) {
    lz4_decompress_stream_t *ds = _checkdecompressionstream(L, 1);
    size_t in_len;
    const char *in = luaL_checklstring(L, 2, &in_len);
    size_t out_len = luaL_checkinteger(L, 3);
    int policy = _ring_policy(ds->buffer_size, ds->buffer_position, out_len);
    int r;

    if (policy == RING_POLICY_APPEND || policy == RING_POLICY_RESET) {
        char *ring;
        size_t new_position;
        if (policy == RING_POLICY_APPEND) {
            ring = ds->buffer + ds->buffer_position;
            new_position = ds->buffer_position + out_len;
        } else {// RING_POLICY_RESET
            ring = ds->buffer;
            new_position = out_len;
        }
        r = LZ4_decompress_fast_continue(&ds->handle, in, ring, out_len);
        if (r < 0) return luaL_error(L, "corrupt input or need more output space");
        ds->buffer_position = new_position;
        lua_pushlstring(L, ring, out_len);
    } else {// RING_POLICY_EXTERNAL
        LUABUFF_NEW(b, out, out_len)
        r = LZ4_decompress_fast_continue(&ds->handle, in, out, out_len);
        if (r < 0) {
            LUABUFF_FREE(out)
            return luaL_error(L, "corrupt input or need more output space");
        }
        _lz4_ds_save_dict(ds, out, out_len);// memcpy(ds->buffer, out, out_len)
        LUABUFF_PUSH(b, out, out_len)
    }

    return 1;
}

static int lz4_ds_tostring(lua_State *L) {
    lz4_decompress_stream_t *p = _checkdecompressionstream(L, 1);
    lua_pushfstring(L, "lz4.decompression_stream (%p)", p);
    return 1;
}

static int lz4_ds_gc(lua_State *L) {
    lz4_decompress_stream_t *p = _checkdecompressionstream(L, 1);
    free(p->buffer);
    return 0;
}

static const luaL_Reg decompress_stream_functions[] = {
        {"reset", lz4_ds_reset},
        {"decompress_safe", lz4_ds_decompress_safe},
        {"decompress_fast", lz4_ds_decompress_fast},
        {NULL, NULL},
};

static int lz4_new_decompression_stream(lua_State *L) {
    int buffer_size = luaL_optinteger(L, 1, DEF_BUFSIZE);
    lz4_decompress_stream_t *p;

    if (buffer_size < MIN_BUFFSIZE) buffer_size = MIN_BUFFSIZE;

    p = (lz4_decompress_stream_t *) lua_newuserdata(L, sizeof(lz4_decompress_stream_t));
    LZ4_setStreamDecode(&p->handle, NULL, 0);
    p->buffer_size = buffer_size;
    p->buffer_position = 0;
    p->buffer = (char *) malloc(buffer_size);
    if (p->buffer == NULL) luaL_error(L, "out of memory");

    if (luaL_newmetatable(L, "lz4.decompression_stream")) {
        // new method table
        luaL_newlib(L, decompress_stream_functions);
        // metatable.__index = method table
        lua_setfield(L, -2, "__index");

        // metatable.__tostring
        lua_pushcfunction(L, lz4_ds_tostring);
        lua_setfield(L, -2, "__tostring");

        // metatable.__gc
        lua_pushcfunction(L, lz4_ds_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);

    return 1;
}

/*****************************************************************************
 * Export
 ****************************************************************************/

static const luaL_Reg export_functions[] = {
        /* Frame */
        {"compress", lz4_compress},
        {"decompress", lz4_decompress},
        /* Block */
        {"block_compress", lz4_block_compress},
        {"block_compress_hc", lz4_block_compress_hc},
        {"block_decompress_safe", lz4_block_decompress_safe},
        {"block_decompress_fast", lz4_block_decompress_fast},
        {"block_decompress_safe_partial", lz4_block_decompress_safe_partial},
        /* Stream */
        {"new_compression_stream", lz4_new_compression_stream},
        {"new_compression_stream_hc", lz4_new_compression_stream_hc},
        {"new_decompression_stream", lz4_new_decompression_stream},
        {NULL, NULL},
};

int metadot_bind_lz4(lua_State *L) {
    int table_index;

    LuaWrapper::metadot_load(L, export_functions, "_metadot_lz4");

    table_index = lua_gettop(L);

    lua_pushfstring(L, "%d.%d.%d", LZ4_VERSION_MAJOR, LZ4_VERSION_MINOR, LZ4_VERSION_RELEASE);
    lua_setfield(L, table_index, "version");

    lua_pushinteger(L, LZ4F_max64KB);
    lua_setfield(L, table_index, "block_64KB");
    lua_pushinteger(L, LZ4F_max256KB);
    lua_setfield(L, table_index, "block_256KB");
    lua_pushinteger(L, LZ4F_max1MB);
    lua_setfield(L, table_index, "block_1MB");
    lua_pushinteger(L, LZ4F_max4MB);
    lua_setfield(L, table_index, "block_4MB");

    return 1;
}

#pragma endregion BindLZ4