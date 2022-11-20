// Copyright(c) 2022, KaoruXun All rights reserved.

// Now I'm using gc(https://github.com/mkirchner/gc)
// and allocator(https://github.com/mtrebi/memory-allocators)
// which are both licensed under the MIT License
// and also the memory allocator of SDL

#ifndef _METADOT_GCMANAGER_HPP_
#define _METADOT_GCMANAGER_HPP_

#include <cstdint>

#include "Game/Core.hpp"

#include "Engine/Memory/Allocator.h"
#include "Engine/Render/SDLWrapper.hpp"

#define METADOT_GC_ALLOC(size) SDL_malloc(size)
#define METADOT_GC_ALLOC_ALIGNED(size, alignment) SDL_malloc(size)
#define METADOT_GC_DEALLOC(ptr) SDL_free(ptr)
#define METADOT_GC_DEALLOC_ALIGNED(ptr) SDL_free(ptr)
#define METADOT_GC_REALLOC(ptr, size) SDL_realloc(ptr, size)
#define METADOT_GC_REALLOC_ALIGNED(ptr, size, alignment) SDL_realloc(ptr, size, alignment)

#define METADOT_NEW(_field, _ptr, _class, ...)                  \
    {                                                           \
        _ptr = (_class *) GC::_field->Allocate(sizeof(_class)); \
        new (_ptr) _class(__VA_ARGS__);                         \
        GC::_field##_Count++;                                   \
    }

#define METADOT_NEW_ARRAY(_field, _ptr, _class, _count, ...)            \
    {                                                                   \
        _ptr = (_class *) GC::_field->Allocate(sizeof(_class[_count])); \
        new (_ptr) _class(__VA_ARGS__);                                 \
        GC::_field##_Count++;                                           \
    }

#define METADOT_DELETE(_field, _ptr, _class_name) \
    {                                             \
        _ptr->~_class_name();                     \
        GC::_field->Free(_ptr);                   \
        GC::_field##_Count--;                     \
    }

#define GCField_R(_n)      \
    static CAllocator *_n; \
    static UInt32 _n##_Count

#define GCField_S(_n)             \
    CAllocator *GC::_n = nullptr; \
    UInt32 GC::_n##_Count = 0

struct GC
{
    GCField_R(C);
};

void METAENGINE_Memory_Init(int argc, char *argv[]);
void METAENGINE_Memory_End();
void METAENGINE_Memory_RunGC();

#if defined(METADOT_LEAK_TEST)

void *operator new(std::size_t sz);
void operator delete(void *ptr) noexcept;

void getInfo();

#endif

#endif// _METADOT_GCMANAGER_HPP_