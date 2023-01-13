// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include <memory>

#include "game/game.hpp"

int main(int argc, char *argv[]) {
    const auto game = std::make_unique<Game>(argc, argv);
    return game->init(argc, argv);
}