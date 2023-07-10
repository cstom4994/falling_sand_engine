// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_GLOBAL_HPP
#define ME_GLOBAL_HPP

#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <unordered_map>

#include "engine/audio/audio.h"
#include "engine/core/core.hpp"
#include "engine/core/io/filesystem.h"
#include "engine/core/platform.h"
#include "engine/engine.h"
#include "engine/game_shaders.hpp"
#include "engine/meta/reflection.hpp"
#include "engine/ui/imgui_impl.hpp"

class Game;
class Scripting;
class UIData;

struct I18N {
    void Init();
    void Load(std::string lang);
    std::string Get(std::string text);
};

struct Global {
    Game *game = nullptr;
    AudioEngine audio;
    I18N I18N;
};

extern Global global;

//--------------------------------------------------------------------------------------------------------------------------------//
// USEFUL STRUCTS:

typedef struct MEivec2 {
    int32_t x, y;
} MEivec2;

typedef struct MEivec3 {
    int32_t x, y, z;
} MEivec3;

typedef struct MEivec4 {
    int32_t x, y, z, w;
} MEivec4;

typedef struct MEuvec2 {
    uint32_t x, y;
} MEuvec2;

typedef struct MEuvec3 {
    uint32_t x, y, z;
} MEuvec3;

typedef struct MEuvec4 {
    uint32_t x, y, z, w;
} MEuvec4;

typedef struct MEcolor {
    uint8_t r, g, b;
} MEcolor;

// byte vec3
typedef struct MEbvec3 {
    uint8_t x, y, z;
} MEbvec3;

typedef float MEf3[3];

enum MEf3_index { X, Y, Z, W };

// MEf44:
//          c.0   c.1   c.2    c.3
//    r.0 | 0.x   4.x   8.x    12.x |
//    r.1 | 1.y   5.y   9.y    13.y |
//    r.2 | 2.z   6.z   10.z   14.z |
//    r.3 | 3.w   7.w   11.w   15.w |
typedef float MEf44[4 * 4];

#endif