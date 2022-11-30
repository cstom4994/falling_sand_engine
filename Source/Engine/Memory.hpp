// Copyright(c) 2022, KaoruXun All rights reserved.

// Now I'm using gc(https://github.com/mkirchner/gc)
// and allocator(https://github.com/mtrebi/memory-allocators)
// which are both licensed under the MIT License
// and also the memory allocator of SDL

#ifndef _METADOT_GCMANAGER_HPP_
#define _METADOT_GCMANAGER_HPP_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>

#include "Engine/Allocator.h"
#include "Engine/SDLWrapper.hpp"
#include "Core/Core.hpp"
#include "Core/Macros.hpp"
#include "Libs/nameof.hpp"

#define METADOT_GC_ALLOC(size) SDL_malloc(size)
#define METADOT_GC_ALLOC_ALIGNED(size, alignment) SDL_malloc(size)
#define METADOT_GC_DEALLOC(ptr) SDL_free(ptr)
#define METADOT_GC_DEALLOC_ALIGNED(ptr) SDL_free(ptr)
#define METADOT_GC_REALLOC(ptr, size) SDL_realloc(ptr, size)
#define METADOT_GC_REALLOC_ALIGNED(ptr, size, alignment) SDL_realloc(ptr, size, alignment)

#if defined(METADOT_DEBUG)
#define ADDTODEBUGMAP(_c) GC::MemoryDebugMap.insert(std::make_pair(NAMEOF_TYPE(_c), sizeof(_c)))
#define REMOVEDEBUGMAP(_c) GC::MemoryDebugMap.erase(NAMEOF_TYPE(_c))
#else
#define ADDTODEBUGMAP(_c)
#define REMOVEDEBUGMAP(_c)
#endif

#define METADOT_NEW(_field, _ptr, _class, ...)                  \
    {                                                           \
        _ptr = (_class *) GC::_field->Allocate(sizeof(_class)); \
        new (_ptr) _class(__VA_ARGS__);                         \
        GC::_field##_Count++;                                   \
        ADDTODEBUGMAP(_class);                                  \
    }

#define METADOT_NEW_ARRAY(_field, _ptr, _class, _count, ...)            \
    {                                                                   \
        _ptr = (_class *) GC::_field->Allocate(sizeof(_class[_count])); \
        new (_ptr) _class(__VA_ARGS__);                                 \
        GC::_field##_Count++;                                           \
        ADDTODEBUGMAP(_class);                                          \
    }

#define METADOT_CREATE(_field, _ptr, _class, ...) \
                                                  \
    _class *_ptr = nullptr;                       \
    METADOT_NEW(_field, _ptr, _class, __VA_ARGS__)


#define METADOT_DELETE_RAW(_field, _ptr, _class_name, _class) \
    {                                                         \
        _ptr->~_class_name();                                 \
        GC::_field->Free(_ptr);                               \
        GC::_field##_Count--;                                 \
        REMOVEDEBUGMAP(_class);                               \
    }

#define METADOT_DELETE(_field, _ptr, _class_name)                   \
    {                                                               \
        METADOT_DELETE_RAW(_field, _ptr, _class_name, _class_name); \
    }

#define METADOT_DELETE_EX(_field, _ptr, _class_name, _class)   \
    {                                                          \
        METADOT_DELETE_RAW(_field, _ptr, _class_name, _class); \
    }

#define GCField_R(_c, _n) \
    static _c *_n;        \
    static std::atomic<int> _n##_Count

#define GCField_S(_c, _n) \
    _c *GC::_n = nullptr; \
    std::atomic<int> GC::_n##_Count = 0


struct GC
{
#if defined(METADOT_DEBUG)
    static std::map<std::string_view, std::size_t> MemoryDebugMap;
#endif

    GCField_R(CAllocator, C);
    GCField_R(LinearAllocator, S);
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