// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

#include "Game/Legacy/Game.hpp"

struct Global
{
    Game *game = nullptr;
    
    CAudioEngine audioEngine;
    MetaEngine::GameDir GameDir;

    Client *client = nullptr;
    Server *server = nullptr;

    Global() {}

    void tick();
};

extern Global global;