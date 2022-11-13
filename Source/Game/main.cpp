// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/Game.hpp"

#include <memory>

int main(int argc, char *argv[])
{
    const auto game = std::make_unique<Game>();
    return game->init(argc, argv);
}