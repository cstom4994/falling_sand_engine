// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

#include "Game/Legacy/Game.hpp"

struct Global
{
    Game *game;
    CAudioEngine audioEngine;

    Global() {}

    void tick();
};

extern Global global;