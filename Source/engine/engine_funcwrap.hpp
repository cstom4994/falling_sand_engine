// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_ENGINEFUNCWRAP_HPP_
#define _METADOT_ENGINEFUNCWRAP_HPP_

struct lua_State;

#define DEFAULT_SCALE 2
#define SCRN_WIDTH 280
#define SCRN_HEIGHT 160
#define COLOR_LIMIT 16
#define INIT_COLORS                                                                                \
    {                                                                                              \
        {24, 24, 24}, {29, 43, 82}, {126, 37, 83}, {0, 134, 81}, {171, 81, 54}, {86, 86, 86},      \
                {157, 157, 157}, {255, 0, 76}, {255, 163, 0}, {255, 240, 35}, {0, 231, 85},        \
                {41, 173, 255}, {130, 118, 156}, {255, 119, 169}, {254, 204, 169},                 \
                {236, 236, 236},                                                                   \
    }

int metadot_bind_image(lua_State *L);
int metadot_bind_gpu(lua_State *L);
int metadot_bind_fs(lua_State *L);
int metadot_bind_lz4(lua_State *L);

#endif