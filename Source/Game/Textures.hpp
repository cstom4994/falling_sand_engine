// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

#include "Engine/Render/SDLWrapper.hpp"

#include <iostream>


#define INC_Textures

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

    static void initTexture();

    static C_Surface *loadTexture(std::string path);
    static C_Surface *loadTexture(std::string path, UInt32 pixelFormat);

    static C_Surface *scaleTexture(C_Surface *, float x, float y);
};
