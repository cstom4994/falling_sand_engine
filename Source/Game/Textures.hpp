// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_TEXTURES_HPP_
#define _METADOT_TEXTURES_HPP_

#include "Engine/Platforms/SDLWrapper.hpp"
#include "Core/Core.hpp"

#include <iostream>


#define INC_Textures

struct ase_t;

struct Aseprite
{
    ase_t *ase;     // Pointer to the cute_aseprite data.
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

    static void initTexture();

    static C_Surface *loadTexture(std::string path);
    static C_Surface *loadTexture(std::string path, UInt32 pixelFormat);
    static C_Surface *loadAseprite(std::string path);

    static C_Surface *scaleTexture(C_Surface *, float x, float y);
};

#endif