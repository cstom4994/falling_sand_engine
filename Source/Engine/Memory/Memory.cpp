// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Memory.hpp"

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

void METAENGINE_Memory_Init(int argc, char *argv[]) {
    gc_start(&gc, &argc);
}

void METAENGINE_Memory_End() {
    gc_stop(&gc);
}

