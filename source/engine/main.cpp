// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include <memory>

#include "game.hpp"

#ifdef main
#undef main
#endif

#ifdef _DEBUG
// #define CHECK_MEM
#endif

#ifdef CHECK_MEM
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
#endif

int main(int argc, char *argv[]) {
    const auto game = ME::create_scope<Game>(argc, argv);
    auto result = game->init(argc, argv);

#ifdef CHECK_MEM
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtDumpMemoryLeaks();  // 在应用退出时显示内存泄漏报告
#endif

    return result;
}