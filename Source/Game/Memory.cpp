// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Memory.hpp"
#include "imgui.h"

#include <array>
#include <iostream>

#if defined(METADOT_LEAK_TEST)

int const MY_SIZE = 1024 * 512;

static std::array<void *, MY_SIZE> myAlloc{
        nullptr,
};

void *operator new(std::size_t sz) {
    static int counter{};
    void *ptr = std::malloc(sz);
    myAlloc.at(counter++) = ptr;
    //std::cerr << "new." << counter << ".addr.: " << ptr << " size: " << sz << std::endl;
    return ptr;
}

void operator delete(void *ptr) noexcept {
    auto ind = std::distance(myAlloc.begin(), std::find(myAlloc.begin(), myAlloc.end(), ptr));
    myAlloc[ind] = nullptr;
    std::free(ptr);
}

void getInfo() {

    std::cout << std::endl;

    std::cout << "Not deallocated: " << std::endl;
    for (auto i: myAlloc) {
        if (i != nullptr) std::cout << " " << i << std::endl;
    }

    std::cout << std::endl;
}

#endif


#ifdef METADOT_GC_IMPLEMENTED

#include "ImGuiBase.h"

#include "Shlwapi.h"//StrStrI
#include "imagehlp.h"
#include <Windows.h>
#pragma comment(lib, "imagehlp.lib")
#pragma comment(lib, "Shlwapi.lib")

#define METADOT_GC_TLS __declspec(thread)

namespace MetaEngine::Memory {
    typedef void *StackInfo;

    static inline uint32_t getCallstack(uint32_t maxStackSize, void **stack, Hash *hash) {
        uint32_t count = CaptureStackBackTrace(INTERNAL_FRAME_TO_SKIP, maxStackSize, stack, (PDWORD) hash);

        if (count == maxStackSize) {
            void *tmpStack[INTERNAL_MAX_STACK_DEPTH];
            uint32_t tmpSize = CaptureStackBackTrace(INTERNAL_FRAME_TO_SKIP, INTERNAL_MAX_STACK_DEPTH, tmpStack, (PDWORD) hash);

            for (uint32_t i = 0; i < maxStackSize - 1; i++)
                stack[maxStackSize - 1 - i] = tmpStack[tmpSize - 1 - i];
            stack[0] = (void *) ~0;
        }
        return count;
    }

    namespace SymbolGetter {
        static inline const char *getSymbol(void *ptr, void *&absoluteAddress) {
            DWORD64 dwDisplacement = 0;
            DWORD64 dwAddress = DWORD64(ptr);
            HANDLE hProcess = GetCurrentProcess();

            char pSymbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO pSymbol = (PSYMBOL_INFO) pSymbolBuffer;

            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_SYM_NAME;

            if (size_t(ptr) == size_t(-1) || !SymFromAddr(hProcess, dwAddress, &dwDisplacement, pSymbol)) {
                return TRUNCATED_STACK_NAME;
            }
            absoluteAddress = (void *) (DWORD64(ptr) - dwDisplacement);
            return _strdup(pSymbol->Name);
        }
    }// namespace SymbolGetter
}// namespace MetaEngine::Memory

void MetaEngine::Memory::SymbolGetter::init() {
    HANDLE hProcess;

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

    hProcess = GetCurrentProcess();
    if (!SymInitialize(hProcess, NULL, TRUE)) {
        METADOT_GC_ASSERT(false, "SymInitialize failed.");
    }
}

#endif

