// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/Legacy/Game.hpp"

#include <memory>

int main(int argc, char *argv[])
{
    const auto game = std::make_unique<Game>(argc, argv);
    return game->init(argc, argv);
}