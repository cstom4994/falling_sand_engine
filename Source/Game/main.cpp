// Copyright(c) 2022, KaoruXun All rights reserved.


#include "Game/Legacy/Game.hpp"

#undef main

int main(int argc, char *argv[]) {
    const auto game = std::make_unique<Game>();
    return game->init(argc, argv);
}
