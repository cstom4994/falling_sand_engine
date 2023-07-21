// Copyright(c) 2022-2023, KaoruXun All rights reserved.

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

// 重载全局 new 运算符
void* operator new(size_t size) {
    void* ptr = malloc(size);
    // 在这里可以添加其他内存跟踪代码
    return ptr;
}

// 重载全局 delete 运算符
void operator delete(void* ptr) noexcept {
    // 在这里可以添加其他内存跟踪代码
    free(ptr);
}

#endif

extern int ME_main(int argc, char* argv[]);

int main(int argc, char* argv[]) {

#ifdef CHECK_MEM
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    auto result = ME_main(argc, argv);

#ifdef CHECK_MEM
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtDumpMemoryLeaks();  // 在应用退出时显示内存泄漏报告
#endif

    return result;
}