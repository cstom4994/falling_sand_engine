// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once
#include <iostream>

#include "Game/Core.hpp"
#include "Libs/raylib/raylib.h"

#define INC_Textures

class Textures {

public:
    static Texture testTexture;
    static Texture dirt1Texture;
    static Texture stone1Texture;

    static Texture smoothStone;
    static Texture cobbleStone;
    static Texture flatCobbleStone;
    static Texture smoothDirt;
    static Texture cobbleDirt;
    static Texture flatCobbleDirt;
    static Texture softDirt;
    static Texture cloud;
    static Texture gold;
    static Texture goldMolten;
    static Texture goldSolid;
    static Texture iron;
    static Texture obsidian;

    static Texture caveBG;

    static Texture loadTexture(std::string path);
    static Texture loadTexture(std::string path, UInt32 pixelFormat);

    static Texture scaleTexture(Texture , float x, float y);
};
