// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GAMERESOURCES_HPP_
#define _METADOT_GAMERESOURCES_HPP_

#include "Core/Core.hpp"
#include "Engine/SDLWrapper.hpp"
#include "LuaWrapper.hpp"

#include <iostream>
#include <string>
#include <string_view>

struct ase_t;

struct Aseprite
{
    ase_t *ase;// Pointer to the cute_aseprite data.
};

class Textures {

public:
    static C_Surface *testTexture;
    static C_Surface *dirt1Texture;
    static C_Surface *stone1Texture;

    static C_Surface *smoothStone;
    static C_Surface *cobbleStone;
    static C_Surface *flatCobbleStone;
    static C_Surface *smoothDirt;
    static C_Surface *cobbleDirt;
    static C_Surface *flatCobbleDirt;
    static C_Surface *softDirt;
    static C_Surface *cloud;
    static C_Surface *gold;
    static C_Surface *goldMolten;
    static C_Surface *goldSolid;
    static C_Surface *iron;
    static C_Surface *obsidian;

    static C_Surface *caveBG;

    static C_Surface *testAse;

    static void InitTexture();

    static C_Surface *LoadTexture(std::string path);
    static C_Surface *LoadTexture(std::string path, UInt32 pixelFormat);
    static C_Surface *loadAseprite(std::string path);

    static C_Surface *scaleTexture(C_Surface *, float x, float y);
};

struct I18N
{
    void Init();
    void Load(std::string lang);
    std::string Get(std::string text);
};

#endif