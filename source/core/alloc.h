// Copyright(c) 2022-2023, KaoruXun All rights reserved.

// Now I'm using gc(https://github.com/mkirchner/gc) (MIT license)

#ifndef _METADOT_ALLOC_H_
#define _METADOT_ALLOC_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/core.h"

struct AllocationMap;

typedef struct GarbageCollector {
    struct AllocationMap *allocs;  // allocation map
    bool paused;                   // (temporarily) switch gc on/off
    void *bos;                     // bottom of stack
    size_t min_size;
} GarbageCollector;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern GarbageCollector gc;  // Global garbage collector for all
                             // single-threaded applications

void gc_start(GarbageCollector *gc, void *bos);
void gc_start_ext(GarbageCollector *gc, void *bos, size_t initial_size, size_t min_size, double downsize_load_factor, double upsize_load_factor, double sweep_factor);
size_t gc_stop(GarbageCollector *gc);
void gc_pause(GarbageCollector *gc);
void gc_resume(GarbageCollector *gc);
size_t gc_run(GarbageCollector *gc);

void *gc_malloc(GarbageCollector *gc, size_t size);
void *gc_malloc_static(GarbageCollector *gc, size_t size, void (*dtor)(void *));
void *gc_malloc_ext(GarbageCollector *gc, size_t size, void (*dtor)(void *));
void *gc_calloc(GarbageCollector *gc, size_t count, size_t size);
void *gc_calloc_ext(GarbageCollector *gc, size_t count, size_t size, void (*dtor)(void *));
void *gc_realloc(GarbageCollector *gc, void *ptr, size_t size);
void gc_free(GarbageCollector *gc, void *ptr);
void *gc_make_static(GarbageCollector *gc, void *ptr);
char *gc_strdup(GarbageCollector *gc, const char *s);

struct LuaMemBlock {
    void *ptr;
    size_t size;
};

struct LuaAllocator {
    struct LuaMemBlock *blocks;
    size_t nb_blocks, size_blocks;
    size_t total_allocated;
};

struct LuaAllocator *new_allocator(void);
void delete_allocator(struct LuaAllocator *alloc);

void *lua_simple_alloc(void *ud, void *ptr, size_t osize, size_t nsize);
void *lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus

#pragma region engine_framework

#if !defined(METAENGINE_FW_ALLOC) && !defined(METAENGINE_FW_FREE)
#ifdef _MSC_VER  // Leak checking for debug Windows builds.
#define _CRTDBG_MAPALLOC
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <stdlib.h>
#define METAENGINE_FW_CALLOC(size) calloc(size, 1)
#define METAENGINE_FW_ALLOC(size) malloc(size)
#define METAENGINE_FW_FREE(ptr) free(ptr)
#define METAENGINE_FW_REALLOC(ptr, size) realloc(ptr, size)
#endif

//--------------------------------------------------------------------------------------------------
// Overload operator new ourselves.
// This avoids including thousands of lines of code in <new>, and also lets us hook up our own
// custom allocators without too much hassle.

#ifdef _MSC_VER
#pragma warning(disable : 4291)
#endif

enum METAENGINE_DummyEnum { METAENGINE_DUMMY_ENUM };
inline void *operator new(size_t, METAENGINE_DummyEnum, void *ptr) { return ptr; }
#define METAENGINE_FW_PLACEMENT_NEW(ptr) new (METAENGINE_DUMMY_ENUM, ptr)
#define METAENGINE_FW_NEW(T) new (METAENGINE_DUMMY_ENUM, METAENGINE_FW_ALLOC(sizeof(T))) T

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// Aligned allocation.
// Mainly useful for optimization purposes, such as SIMD alignment.

void *METADOT_CDECL metadot_aligned_alloc(size_t size, int alignment);
void METADOT_CDECL metadot_aligned_free(void *p);

//--------------------------------------------------------------------------------------------------
// Arena allocator.
// Fetches memory with reduced calls to `malloc`.
// Free all memory allocated with `metadot_arena_reset`. You can not free inidividual allocations.

typedef struct METAENGINE_Arena {
    int alignment;
    int block_size;
    char *ptr;
    char *end;
    char **blocks;
} METAENGINE_Arena;

void METADOT_CDECL metadot_arena_init(METAENGINE_Arena *arena, int alignment, int block_size);
void *METADOT_CDECL metadot_arena_alloc(METAENGINE_Arena *arena, size_t size);
void METADOT_CDECL metadot_arena_reset(METAENGINE_Arena *arena);

//--------------------------------------------------------------------------------------------------
// Memory pool allocator.

typedef struct METAENGINE_MemoryPool METAENGINE_MemoryPool;

/**
 * Constructs a new memory pool.
 * `element_size` is the fixed size each internal allocation will be.
 * `element_count` determins how big the internal pool will be.
 */
METAENGINE_MemoryPool *METADOT_CDECL metadot_make_memory_pool(int element_size, int element_count, int alignment);

/**
 * Destroys a memory pool previously created with `make_memory_pool`. Does not clean up any leftover
 * allocations from `metadot_memory_pool_alloc` that overflowed to the `malloc` backup. See `metadot_memory_pool_alloc`
 * for more details.
 */
void METADOT_CDECL metadot_destroy_memory_pool(METAENGINE_MemoryPool *pool);

/**
 * Returns a block of memory of `element_size` bytes. If the number of allocations in the pool exceeds
 * `element_count` then `malloc` is used as a fallback.
 */
void *METADOT_CDECL metadot_memory_pool_alloc(METAENGINE_MemoryPool *pool);

/**
 * The same as `metadot_memory_pool_alloc` without the `malloc` fallback -- returns `NULL` if the memory pool
 * is all used up.
 */
void *METADOT_CDECL METAENGINE_MemoryPoolry_alloc(METAENGINE_MemoryPool *pool);

/**
 * Frees an allocation previously acquired by `metadot_memory_pool_alloc` or `METAENGINE_MemoryPoolry_alloc`.
 */
void METADOT_CDECL metadot_memory_pool_free(METAENGINE_MemoryPool *pool, void *element);

#ifdef __cplusplus
}
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

namespace MetaEngine {

METADOT_INLINE void *aligned_alloc(size_t size, int alignment) { return metadot_aligned_alloc(size, alignment); }
METADOT_INLINE void aligned_free(void *ptr) { return metadot_aligned_free(ptr); }

using Arena = METAENGINE_Arena;

METADOT_INLINE void arena_init(METAENGINE_Arena *arena, int alignment, int block_size) { metadot_arena_init(arena, alignment, block_size); }
METADOT_INLINE void *arena_alloc(METAENGINE_Arena *arena, size_t size) { return metadot_arena_alloc(arena, size); }
METADOT_INLINE void arena_reset(METAENGINE_Arena *arena) { return metadot_arena_reset(arena); }

using MemoryPool = METAENGINE_MemoryPool;

METADOT_INLINE MemoryPool *make_memory_pool(int element_size, int element_count, int alignment) { return metadot_make_memory_pool(element_size, element_count, alignment); }
METADOT_INLINE void destroy_memory_pool(MemoryPool *pool) { metadot_destroy_memory_pool(pool); }
METADOT_INLINE void *memory_pool_alloc(MemoryPool *pool) { return metadot_memory_pool_alloc(pool); }
METADOT_INLINE void *MemoryPoolry_alloc(MemoryPool *pool) { return METAENGINE_MemoryPoolry_alloc(pool); }
METADOT_INLINE void memory_pool_free(MemoryPool *pool, void *element) { return metadot_memory_pool_free(pool, element); }

}  // namespace MetaEngine

#pragma endregion engine_framework

#endif

#endif