// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include <memory>

#include "game.hpp"

#ifdef main
#undef main
#endif

int main(int argc, char *argv[]) {
    const auto game = ME::create_scope<Game>(argc, argv);
    return game->init(argc, argv);
}