// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.


#include "Game.hpp"

#undef main

int main(int argc, char *argv[]) {
    const auto game = std::make_unique<Game>();
    return game->init(argc, argv);
}
