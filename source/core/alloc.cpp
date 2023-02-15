// Copyright(c) 2022-2023, KaoruXun All rights reserved.
// Including some codes from https://github.com/mkirchner/gc

#include "alloc.hpp"

#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <limits>
#include <utility>

#ifdef _DEBUG
#include <iostream>
#endif

#include "core/stl.h"

// #include "core/c/auto_c.h"

AllocationMetrics g_AllocationMetrics{.TotalAllocated = 0, .TotalFree = 0};

#if defined(METADOT_DEBUG)
std::map<std::string_view, std::size_t> AllocationMetrics::MemoryDebugMap = {};
#endif

// Static GC Field
GCField_S(CAllocator, C);
GCField_S(LinearAllocator, S);

U64 MemCurrentUsageBytes() { return g_AllocationMetrics.TotalAllocated - g_AllocationMetrics.TotalFree; }
F32 MemCurrentUsageMB() {
    U64 bytes = MemCurrentUsageBytes();
    return (F32)(bytes / 1048576.0f);
}

// https://open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3536.html

void *operator new(size_t size) {
    // METADOT_BUG("Alloc %d memory", size);
    g_AllocationMetrics.TotalAllocated += size;

    return std::malloc(size);
}
void operator delete(void *ptr, size_t size) throw() {
    // METADOT_BUG("Free %d memory", size);
    g_AllocationMetrics.TotalFree += size;

    return std::free(ptr);
}

void *operator new(size_t size, const std::nothrow_t &) throw() {
    g_AllocationMetrics.TotalAllocated += size;

    return std::malloc(size);
}

void operator delete(void *ptr, size_t size, const std::nothrow_t &) throw() {
    g_AllocationMetrics.TotalFree += size;

    return std::free(ptr);
}

void *operator new(size_t size, size_t alignment) noexcept(false) {
    g_AllocationMetrics.TotalAllocated += size;

    return std::aligned_alloc(alignment, size);
}

void operator delete(void *ptr, size_t size, size_t alignment) throw() {
    g_AllocationMetrics.TotalFree += size;

    return std::free(ptr);
}

void *operator new(size_t size, size_t alignment, const std::nothrow_t &) throw() {
    g_AllocationMetrics.TotalAllocated += size;

    return std::aligned_alloc(alignment, size);
}

void operator delete(void *ptr, size_t size, size_t alignment, const std::nothrow_t &) throw() {
    g_AllocationMetrics.TotalFree += size;

    return std::free(ptr);
}

void *operator new[](size_t size) noexcept(false) {
    g_AllocationMetrics.TotalAllocated += size;

    return std::malloc(size);
}

void operator delete[](void *ptr, size_t size) throw() {
    g_AllocationMetrics.TotalFree += size;

    return std::free(ptr);
}

void *operator new[](size_t size, const std::nothrow_t &) throw() {
    g_AllocationMetrics.TotalAllocated += size;

    return std::malloc(size);
}

void operator delete[](void *ptr, size_t size, const std::nothrow_t &) throw() {
    g_AllocationMetrics.TotalFree += size;

    return std::free(ptr);
}

void *operator new[](size_t size, size_t alignment) noexcept(false) {
    g_AllocationMetrics.TotalAllocated += size;

    return std::aligned_alloc(alignment, size);
}

void operator delete[](void *ptr, size_t size, size_t alignment) throw() {
    g_AllocationMetrics.TotalFree += size;

    return std::free(ptr);
}

void *operator new[](size_t size, size_t alignment, const std::nothrow_t &) throw() {
    g_AllocationMetrics.TotalAllocated += size;

    return std::aligned_alloc(alignment, size);
}

void operator delete[](void *ptr, size_t size, size_t alignment, const std::nothrow_t &) throw() {
    g_AllocationMetrics.TotalFree += size;

    return std::free(ptr);
}

#define PTRSIZE sizeof(char *)

/*
 * Allocations can temporarily be tagged as "marked" an part of the
 * mark-and-sweep implementation or can be tagged as "roots" which are
 * not automatically garbage collected. The latter allows the implementation
 * of global variables.
 */
#define GC_TAG_NONE 0x0
#define GC_TAG_ROOT 0x1
#define GC_TAG_MARK 0x2

/*
 * Support for windows c compiler is added by adding this macro.
 * Tested on: Microsoft (R) C/C++ Optimizing Compiler Version 19.24.28314 for x86
 */
#if defined(_MSC_VER)
#define __builtin_frame_address(x) ((void)(x), _AddressOfReturnAddress())
#endif

static bool is_prime(size_t n) {
    /* https://stackoverflow.com/questions/1538644/c-determine-if-a-number-is-prime */
    if (n <= 3)
        return n > 1;  // as 2 and 3 are prime
    else if (n % 2 == 0 || n % 3 == 0)
        return false;  // check if n is divisible by 2 or 3
    else {
        for (size_t i = 5; i * i <= n; i += 6) {
            if (n % i == 0 || n % (i + 2) == 0) return false;
        }
        return true;
    }
}

static size_t next_prime(size_t n) {
    while (!is_prime(n)) ++n;
    return n;
}

/**
 * The allocation object.
 *
 * The allocation object holds all metadata for a memory location
 * in one place.
 */
typedef struct Allocation {
    void *ptr;                // mem pointer
    size_t size;              // allocated size in bytes
    char tag;                 // the tag for mark-and-sweep
    void (*dtor)(void *);     // destructor
    struct Allocation *next;  // separate chaining
} Allocation;

/**
 * Create a new allocation object.
 *
 * Creates a new allocation object using the system `malloc`.
 *
 * @param[in] ptr The pointer to the memory to manage.
 * @param[in] size The size of the memory range pointed to by `ptr`.
 * @param[in] dtor A pointer to a destructor function that should be called
 *                 before freeing the memory pointed to by `ptr`.
 * @returns Pointer to the new allocation instance.
 */
static Allocation *gc_allocation_new(void *ptr, size_t size, void (*dtor)(void *)) {
    Allocation *a = (Allocation *)malloc(sizeof(Allocation));
    a->ptr = ptr;
    a->size = size;
    a->tag = GC_TAG_NONE;
    a->dtor = dtor;
    a->next = NULL;
    return a;
}

/**
 * Delete an allocation object.
 *
 * Deletes the allocation object pointed to by `a`, but does *not*
 * free the memory pointed to by `a->ptr`.
 *
 * @param a The allocation object to delete.
 */
static void gc_allocation_delete(Allocation *a) { free(a); }

/**
 * The allocation hash map.
 *
 * The core data structure is a hash map that holds the allocation
 * objects and allows O(1) retrieval given the memory location. Collision
 * resolution is implemented using separate chaining.
 */
typedef struct AllocationMap {
    size_t capacity;
    size_t min_capacity;
    double downsize_factor;
    double upsize_factor;
    double sweep_factor;
    size_t sweep_limit;
    size_t size;
    Allocation **allocs;
} AllocationMap;

/**
 * Determine the current load factor of an `AllocationMap`.
 *
 * Calculates the load factor of the hash map as the quotient of the size and
 * the capacity of the hash map.
 *
 * @param am The allocationo map to calculate the load factor for.
 * @returns The load factor of the allocation map `am`.
 */
static double gc_allocation_map_load_factor(AllocationMap *am) { return (double)am->size / (double)am->capacity; }

static AllocationMap *gc_allocation_map_new(size_t min_capacity, size_t capacity, double sweep_factor, double downsize_factor, double upsize_factor) {
    AllocationMap *am = (AllocationMap *)malloc(sizeof(AllocationMap));
    am->min_capacity = next_prime(min_capacity);
    am->capacity = next_prime(capacity);
    if (am->capacity < am->min_capacity) am->capacity = am->min_capacity;
    am->sweep_factor = sweep_factor;
    am->sweep_limit = (int)(sweep_factor * am->capacity);
    am->downsize_factor = downsize_factor;
    am->upsize_factor = upsize_factor;
    am->allocs = (Allocation **)calloc(am->capacity, sizeof(Allocation *));
    am->size = 0;
    // LOG_DEBUG("Created allocation map (cap=%ld, siz=%ld)", am->capacity, am->size);
    return am;
}

static void gc_allocation_map_delete(AllocationMap *am) {
    // Iterate over the map
    // LOG_DEBUG("Deleting allocation map (cap=%ld, siz=%ld)",
    //          am->capacity, am->size);
    Allocation *alloc, *tmp;
    for (size_t i = 0; i < am->capacity; ++i) {
        if ((alloc = am->allocs[i])) {
            // Make sure to follow the chain inside a bucket
            while (alloc) {
                tmp = alloc;
                alloc = alloc->next;
                // free the management structure
                gc_allocation_delete(tmp);
            }
        }
    }
    free(am->allocs);
    free(am);
}

static size_t gc_hash(void *ptr) { return ((uintptr_t)ptr) >> 3; }

static void gc_allocation_map_resize(AllocationMap *am, size_t new_capacity) {
    if (new_capacity <= am->min_capacity) {
        return;
    }
    // Replaces the existing items array in the hash table
    // with a resized one and pushes items into the new, correct buckets
    // LOG_DEBUG("Resizing allocation map (cap=%ld, siz=%ld) -> (cap=%ld)",
    //          am->capacity, am->size, new_capacity);
    Allocation **resized_allocs = (Allocation **)calloc(new_capacity, sizeof(Allocation *));

    for (size_t i = 0; i < am->capacity; ++i) {
        Allocation *alloc = am->allocs[i];
        while (alloc) {
            Allocation *next_alloc = alloc->next;
            size_t new_index = gc_hash(alloc->ptr) % new_capacity;
            alloc->next = resized_allocs[new_index];
            resized_allocs[new_index] = alloc;
            alloc = next_alloc;
        }
    }
    free(am->allocs);
    am->capacity = new_capacity;
    am->allocs = resized_allocs;
    am->sweep_limit = am->size + am->sweep_factor * (am->capacity - am->size);
}

static bool gc_allocation_map_resize_to_fit(AllocationMap *am) {
    double load_factor = gc_allocation_map_load_factor(am);
    if (load_factor > am->upsize_factor) {
        // LOG_DEBUG("Load factor %0.3g > %0.3g. Triggering upsize.",
        //          load_factor, am->upsize_factor);
        gc_allocation_map_resize(am, next_prime(am->capacity * 2));
        return true;
    }
    if (load_factor < am->downsize_factor) {
        // LOG_DEBUG("Load factor %0.3g < %0.3g. Triggering downsize.",
        //          load_factor, am->downsize_factor);
        gc_allocation_map_resize(am, next_prime(am->capacity / 2));
        return true;
    }
    return false;
}

static Allocation *gc_allocation_map_get(AllocationMap *am, void *ptr) {
    size_t index = gc_hash(ptr) % am->capacity;
    Allocation *cur = am->allocs[index];
    while (cur) {
        if (cur->ptr == ptr) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

static Allocation *gc_allocation_map_put(AllocationMap *am, void *ptr, size_t size, void (*dtor)(void *)) {
    size_t index = gc_hash(ptr) % am->capacity;
    // LOG_DEBUG("PUT request for allocation ix=%ld", index);
    Allocation *alloc = gc_allocation_new(ptr, size, dtor);
    Allocation *cur = am->allocs[index];
    Allocation *prev = NULL;
    /* Upsert if ptr is already known (e.g. dtor update). */
    while (cur != NULL) {
        if (cur->ptr == ptr) {
            // found it
            alloc->next = cur->next;
            if (!prev) {
                // position 0
                am->allocs[index] = alloc;
            } else {
                // in the list
                prev->next = alloc;
            }
            gc_allocation_delete(cur);
            // LOG_DEBUG("AllocationMap Upsert at ix=%ld", index);
            return alloc;
        }
        prev = cur;
        cur = cur->next;
    }
    /* Insert at the front of the separate chaining list */
    cur = am->allocs[index];
    alloc->next = cur;
    am->allocs[index] = alloc;
    am->size++;
    // LOG_DEBUG("AllocationMap insert at ix=%ld", index);
    void *p = alloc->ptr;
    if (gc_allocation_map_resize_to_fit(am)) {
        alloc = gc_allocation_map_get(am, p);
    }
    return alloc;
}

static void gc_allocation_map_remove(AllocationMap *am, void *ptr, bool allow_resize) {
    // ignores unknown keys
    size_t index = gc_hash(ptr) % am->capacity;
    Allocation *cur = am->allocs[index];
    Allocation *prev = NULL;
    Allocation *next;
    while (cur != NULL) {
        next = cur->next;
        if (cur->ptr == ptr) {
            // found it
            if (!prev) {
                // first item in list
                am->allocs[index] = cur->next;
            } else {
                // not the first item in the list
                prev->next = cur->next;
            }
            gc_allocation_delete(cur);
            am->size--;
        } else {
            // move on
            prev = cur;
        }
        cur = next;
    }
    if (allow_resize) {
        gc_allocation_map_resize_to_fit(am);
    }
}

static void *gc_mcalloc(size_t size, size_t count) {
    if (!count) return malloc(size);
    return calloc(count, size);
}

static bool gc_needs_sweep(GarbageCollector *gc) { return gc->allocs->size > gc->allocs->sweep_limit; }

static void *gc_allocate(GarbageCollector *gc, size_t size, size_t count, void (*dtor)(void *)) {
    /* Allocation logic that generalizes over malloc/calloc. */

    /* Check if we reached the high-water mark and need to clean up */
    if (gc_needs_sweep(gc) && !gc->paused) {
        size_t freed_mem = gc_run(gc);
        // LOG_DEBUG("Garbage collection cleaned up %lu bytes.", freed_mem);
    }
    /* With cleanup out of the way, attempt to allocate memory */
    void *ptr = gc_mcalloc(count, size);
    size_t alloc_size = count ? count * size : size;
    /* If allocation fails, force an out-of-policy run to free some memory and try again. */
    if (!ptr && !gc->paused && (errno == EAGAIN || errno == ENOMEM)) {
        gc_run(gc);
        ptr = gc_mcalloc(count, size);
    }
    /* Start managing the memory we received from the system */
    if (ptr) {
        // LOG_DEBUG("Allocated %zu bytes at %p", alloc_size, (void *) ptr);
        Allocation *alloc = gc_allocation_map_put(gc->allocs, ptr, alloc_size, dtor);
        /* Deal with metadata allocation failure */
        if (alloc) {
            // LOG_DEBUG("Managing %zu bytes at %p", alloc_size, (void *) alloc->ptr);
            ptr = alloc->ptr;
        } else {
            /* We failed to allocate the metadata, fail cleanly. */
            free(ptr);
            ptr = NULL;
        }
    }
    return ptr;
}

static void gc_make_root(GarbageCollector *gc, void *ptr) {
    Allocation *alloc = gc_allocation_map_get(gc->allocs, ptr);
    if (alloc) {
        alloc->tag |= GC_TAG_ROOT;
    }
}

void *gc_malloc(GarbageCollector *gc, size_t size) { return gc_malloc_ext(gc, size, NULL); }

void *gc_malloc_static(GarbageCollector *gc, size_t size, void (*dtor)(void *)) {
    void *ptr = gc_malloc_ext(gc, size, dtor);
    gc_make_root(gc, ptr);
    return ptr;
}

void *gc_make_static(GarbageCollector *gc, void *ptr) {
    gc_make_root(gc, ptr);
    return ptr;
}

void *gc_malloc_ext(GarbageCollector *gc, size_t size, void (*dtor)(void *)) { return gc_allocate(gc, 0, size, dtor); }

void *gc_calloc(GarbageCollector *gc, size_t size, size_t count) { return gc_calloc_ext(gc, count, size, NULL); }

void *gc_calloc_ext(GarbageCollector *gc, size_t size, size_t count, void (*dtor)(void *)) { return gc_allocate(gc, count, size, dtor); }

void *gc_realloc(GarbageCollector *gc, void *p, size_t size) {
    Allocation *alloc = gc_allocation_map_get(gc->allocs, p);
    if (p && !alloc) {
        // the user passed an unknown pointer
        errno = EINVAL;
        return NULL;
    }
    void *q = realloc(p, size);
    if (!q) {
        // realloc failed but p is still valid
        return NULL;
    }
    if (!p) {
        // allocation, not reallocation
        Allocation *alloc = gc_allocation_map_put(gc->allocs, q, size, NULL);
        return alloc->ptr;
    }
    if (p == q) {
        // successful reallocation w/o copy
        alloc->size = size;
    } else {
        // successful reallocation w/ copy
        void (*dtor)(void *) = alloc->dtor;
        gc_allocation_map_remove(gc->allocs, p, true);
        gc_allocation_map_put(gc->allocs, q, size, dtor);
    }
    return q;
}

void gc_free(GarbageCollector *gc, void *ptr) {
    Allocation *alloc = gc_allocation_map_get(gc->allocs, ptr);
    if (alloc) {
        if (alloc->dtor) {
            alloc->dtor(ptr);
        }
        free(ptr);
        gc_allocation_map_remove(gc->allocs, ptr, true);
    } else {
        // LOG_WARNING("Ignoring request to free unknown pointer %p", (void *) ptr);
    }
}

void gc_start(GarbageCollector *gc, void *bos) { gc_start_ext(gc, bos, 1024, 1024, 0.2, 0.8, 0.5); }

void gc_start_ext(GarbageCollector *gc, void *bos, size_t initial_capacity, size_t min_capacity, double downsize_load_factor, double upsize_load_factor, double sweep_factor) {
    double downsize_limit = downsize_load_factor > 0.0 ? downsize_load_factor : 0.2;
    double upsize_limit = upsize_load_factor > 0.0 ? upsize_load_factor : 0.8;
    sweep_factor = sweep_factor > 0.0 ? sweep_factor : 0.5;
    gc->paused = false;
    gc->bos = bos;
    initial_capacity = initial_capacity < min_capacity ? min_capacity : initial_capacity;
    gc->allocs = gc_allocation_map_new(min_capacity, initial_capacity, sweep_factor, downsize_limit, upsize_limit);
    // LOG_DEBUG("Created new garbage collector (cap=%ld, siz=%ld).", gc->allocs->capacity,
    //          gc->allocs->size);
}

void gc_pause(GarbageCollector *gc) { gc->paused = true; }

void gc_resume(GarbageCollector *gc) { gc->paused = false; }

void gc_mark_alloc(GarbageCollector *gc, void *ptr) {
    Allocation *alloc = gc_allocation_map_get(gc->allocs, ptr);
    /* Mark if alloc exists and is not tagged already, otherwise skip */
    if (alloc && !(alloc->tag & GC_TAG_MARK)) {
        // LOG_DEBUG("Marking allocation (ptr=%p)", ptr);
        alloc->tag |= GC_TAG_MARK;
        /* Iterate over allocation contents and mark them as well */
        // LOG_DEBUG("Checking allocation (ptr=%p, size=%lu) contents", ptr, alloc->size);
        for (char *p = (char *)alloc->ptr; p <= (char *)alloc->ptr + alloc->size - PTRSIZE; ++p) {
            // LOG_DEBUG("Checking allocation (ptr=%p) @%lu with value %p",
            //          ptr, p - ((char *) alloc->ptr), *(void **) p);
            gc_mark_alloc(gc, *(void **)p);
        }
    }
}

void gc_mark_stack(GarbageCollector *gc) {
    // LOG_DEBUG("Marking the stack (gc@%p) in increments of %ld", (void *) gc, sizeof(char));
    void *tos = __builtin_frame_address(0);
    void *bos = gc->bos;
    /* The stack grows towards smaller memory addresses, hence we scan tos->bos.
     * Stop scanning once the distance between tos & bos is too small to hold a valid pointer */
    for (char *p = (char *)tos; p <= (char *)bos - PTRSIZE; ++p) {
        gc_mark_alloc(gc, *(void **)p);
    }
}

void gc_mark_roots(GarbageCollector *gc) {
    // LOG_DEBUG("Marking roots%s", "");
    for (size_t i = 0; i < gc->allocs->capacity; ++i) {
        Allocation *chunk = gc->allocs->allocs[i];
        while (chunk) {
            if (chunk->tag & GC_TAG_ROOT) {
                // LOG_DEBUG("Marking root @ %p", chunk->ptr);
                gc_mark_alloc(gc, chunk->ptr);
            }
            chunk = chunk->next;
        }
    }
}

void gc_mark(GarbageCollector *gc) {
    /* Note: We only look at the stack and the heap, and ignore BSS. */
    // LOG_DEBUG("Initiating GC mark (gc@%p)", (void *) gc);
    /* Scan the heap for roots */
    gc_mark_roots(gc);
    /* Dump registers onto stack and scan the stack */
    void (*volatile _mark_stack)(GarbageCollector *) = gc_mark_stack;
    jmp_buf ctx;
    memset(&ctx, 0, sizeof(jmp_buf));
    setjmp(ctx);
    _mark_stack(gc);
}

size_t gc_sweep(GarbageCollector *gc) {
    // LOG_DEBUG("Initiating GC sweep (gc@%p)", (void *) gc);
    size_t total = 0;
    for (size_t i = 0; i < gc->allocs->capacity; ++i) {
        Allocation *chunk = gc->allocs->allocs[i];
        Allocation *next = NULL;
        /* Iterate over separate chaining */
        while (chunk) {
            if (chunk->tag & GC_TAG_MARK) {
                // LOG_DEBUG("Found used allocation %p (ptr=%p)", (void *) chunk, (void *) chunk->ptr);
                /* unmark */
                chunk->tag &= ~GC_TAG_MARK;
                chunk = chunk->next;
            } else {
                // LOG_DEBUG("Found unused allocation %p (%lu bytes @ ptr=%p)", (void *) chunk, chunk->size, (void *) chunk->ptr);
                /* no reference to this chunk, hence delete it */
                total += chunk->size;
                if (chunk->dtor) {
                    chunk->dtor(chunk->ptr);
                }
                free(chunk->ptr);
                /* and remove it from the bookkeeping */
                next = chunk->next;
                gc_allocation_map_remove(gc->allocs, chunk->ptr, false);
                chunk = next;
            }
        }
    }
    gc_allocation_map_resize_to_fit(gc->allocs);
    return total;
}

void gc_unroot_roots(GarbageCollector *gc) {
    // LOG_DEBUG("Unmarking roots%s", "");
    for (size_t i = 0; i < gc->allocs->capacity; ++i) {
        Allocation *chunk = gc->allocs->allocs[i];
        while (chunk) {
            if (chunk->tag & GC_TAG_ROOT) {
                chunk->tag &= ~GC_TAG_ROOT;
            }
            chunk = chunk->next;
        }
    }
}

size_t gc_stop(GarbageCollector *gc) {
    gc_unroot_roots(gc);
    size_t collected = gc_sweep(gc);
    gc_allocation_map_delete(gc->allocs);
    return collected;
}

size_t gc_run(GarbageCollector *gc) {
    // LOG_DEBUG("Initiating GC run (gc@%p)", (void *) gc);
    gc_mark(gc);
    return gc_sweep(gc);
}

char *gc_strdup(GarbageCollector *gc, const char *s) {
    size_t len = strlen(s) + 1;
    void *_new = gc_malloc(gc, len);

    if (_new == NULL) {
        return NULL;
    }
    return (char *)memcpy(_new, s, len);
}

#pragma region engine_framework

void *metadot_aligned_alloc(size_t size, int alignment) {
    METADOT_ASSERT_E(alignment <= 256);
    void *p = METAENGINE_FW_ALLOC(size + alignment);
    if (!p) return NULL;
    size_t offset = (size_t)p & (alignment - 1);
    p = METAENGINE_ALIGN_FORWARD_PTR((char *)p + 1, alignment);
    METADOT_ASSERT_E(!(((size_t)p) & (alignment - 1)));
    *((char *)p - 1) = (char)(alignment - offset);
    return p;
}

void metadot_aligned_free(void *p) {
    if (!p) return;
    size_t offset = (size_t) * ((uint8_t *)p - 1);
    METAENGINE_FW_FREE((char *)p - (offset & 0xFF));
}

//--------------------------------------------------------------------------------------------------

void metadot_arena_init(METAENGINE_Arena *arena, int alignment, int block_size) {
    METAENGINE_MEMSET(arena, 0, sizeof(*arena));
    arena->alignment = alignment;
    arena->block_size = block_size;
}

void *metadot_arena_alloc(METAENGINE_Arena *arena, size_t size) {
    METADOT_ASSERT_E((int)size < arena->block_size);
    if (size > (size_t)(arena->end - arena->ptr)) {
        arena->ptr = (char *)metadot_aligned_alloc(arena->block_size, arena->alignment);
        arena->end = arena->ptr + arena->block_size;
        apush(arena->blocks, arena->ptr);
    }
    void *result = arena->ptr;
    arena->ptr = (char *)METAENGINE_ALIGN_FORWARD_PTR(arena->ptr + size, arena->alignment);
    METADOT_ASSERT_E(!(((size_t)(arena->ptr)) & (arena->alignment - 1)));
    METADOT_ASSERT_E(arena->ptr <= arena->end);
    return result;
}

void metadot_arena_reset(METAENGINE_Arena *arena) {
    if (arena->blocks) {
        for (int i = 0; i < alen(arena->blocks); ++i) {
            metadot_aligned_free(arena->blocks[i]);
        }
        afree(arena->blocks);
    }
    arena->ptr = NULL;
    arena->end = NULL;
    arena->blocks = NULL;
}

//--------------------------------------------------------------------------------------------------

struct METAENGINE_MemoryPool {
    int unaligned_element_size;
    int element_size;
    size_t arena_size;
    int alignment;
    uint8_t *arena;
    void *free_list;
    int overflow_count;
};

METAENGINE_MemoryPool *metadot_make_memory_pool(int element_size, int element_count, int alignment) {
    element_size = element_size > sizeof(void *) ? element_size : sizeof(void *);
    int unaligned_element_size = element_size;
    element_size = METAENGINE_ALIGN_FORWARD(element_size, alignment);
    size_t header_size = METAENGINE_ALIGN_FORWARD(sizeof(METAENGINE_MemoryPool), alignment);
    METAENGINE_MemoryPool *pool = (METAENGINE_MemoryPool *)metadot_aligned_alloc(header_size + element_size * element_count, alignment);

    pool->unaligned_element_size = unaligned_element_size;
    pool->element_size = element_size;
    pool->arena_size = (size_t)element_size * element_count;
    pool->arena = (uint8_t *)((uintptr_t)pool + header_size);
    pool->free_list = pool->arena;
    pool->overflow_count = 0;

    for (int i = 0; i < element_count - 1; ++i) {
        void **element = (void **)(pool->arena + element_size * i);
        void *next = (void *)(pool->arena + element_size * (i + 1));
        *element = next;
    };

    void **last_element = (void **)(pool->arena + element_size * (element_count - 1));
    *last_element = NULL;

    return pool;
}

void metadot_destroy_memory_pool(METAENGINE_MemoryPool *pool) {
    if (pool->overflow_count) {
        // Attempted to destroy pool without freeing all overflow allocations.
        METADOT_ASSERT_E(pool->overflow_count == 0);
    }
    METAENGINE_FW_FREE(pool);
}

void *metadot_memory_pool_alloc(METAENGINE_MemoryPool *pool) {
    void *mem = METAENGINE_MemoryPoolry_alloc(pool);
    if (!mem) {
        mem = metadot_aligned_alloc(pool->unaligned_element_size, pool->alignment);
        if (mem) {
            pool->overflow_count++;
        }
    }
    return mem;
}

void *METAENGINE_MemoryPoolry_alloc(METAENGINE_MemoryPool *pool) {
    if (pool->free_list) {
        void *mem = pool->free_list;
        pool->free_list = *((void **)pool->free_list);
        return mem;
    } else {
        return NULL;
    }
}

void metadot_memory_pool_free(METAENGINE_MemoryPool *pool, void *element) {
    int difference = (int)((uint8_t *)element - pool->arena);
    bool in_bounds = (void *)element >= pool->arena && difference <= (pool->arena_size - pool->element_size);
    if (pool->overflow_count && !in_bounds) {
        metadot_aligned_free(element);
        pool->overflow_count--;
    } else if (in_bounds) {
        *(void **)element = pool->free_list;
        pool->free_list = element;
    } else {
        // Tried to free something that definitely didn't come from this pool.
        METADOT_ASSERT_E(false);
    }
}

#pragma endregion engine_framework

#pragma region Allocator

CAllocator::CAllocator() : Allocator(0) {}

void CAllocator::Init() {}

CAllocator::~CAllocator() {}

void *CAllocator::Allocate(const std::size_t size, const std::size_t alignment) { return gc_malloc(&gc, size); }

void CAllocator::Free(void *ptr) { gc_free(&gc, ptr); }

FreeListAllocator::FreeListAllocator(const std::size_t totalSize, const PlacementPolicy pPolicy) : Allocator(totalSize) { m_pPolicy = pPolicy; }

void FreeListAllocator::Init() {
    if (m_start_ptr != nullptr) {
        gc_free(&gc, m_start_ptr);
        m_start_ptr = nullptr;
    }
    m_start_ptr = gc_malloc(&gc, m_totalSize);

    this->Reset();
}

FreeListAllocator::~FreeListAllocator() {
    gc_free(&gc, m_start_ptr);
    m_start_ptr = nullptr;
}

void *FreeListAllocator::Allocate(const std::size_t size, const std::size_t alignment) {
    const std::size_t allocationHeaderSize = sizeof(FreeListAllocator::AllocationHeader);
    const std::size_t freeHeaderSize = sizeof(FreeListAllocator::FreeHeader);
    assert("Allocation size must be bigger" && size >= sizeof(Node));
    assert("Alignment must be 8 at least" && alignment >= 8);

    // Search through the free list for a free block that has enough space to allocate our data
    std::size_t padding;
    Node *affectedNode, *previousNode;
    this->Find(size, alignment, padding, previousNode, affectedNode);
    assert(affectedNode != nullptr && "Not enough memory");

    const std::size_t alignmentPadding = padding - allocationHeaderSize;
    const std::size_t requiredSize = size + padding;

    const std::size_t rest = affectedNode->data.blockSize - requiredSize;

    if (rest > 0) {
        // We have to split the block into the data block and a free block of size 'rest'
        Node *newFreeNode = (Node *)((std::size_t)affectedNode + requiredSize);
        newFreeNode->data.blockSize = rest;
        m_freeList.insert(affectedNode, newFreeNode);
    }
    m_freeList.remove(previousNode, affectedNode);

    // Setup data block
    const std::size_t headerAddress = (std::size_t)affectedNode + alignmentPadding;
    const std::size_t dataAddress = headerAddress + allocationHeaderSize;
    ((FreeListAllocator::AllocationHeader *)headerAddress)->blockSize = requiredSize;
    ((FreeListAllocator::AllocationHeader *)headerAddress)->padding = alignmentPadding;

    m_used += requiredSize;
    m_peak = std::max(m_peak, m_used);

#ifdef _DEBUG
    std::cout << "A"
              << "\t@H " << (void *)headerAddress << "\tD@ " << (void *)dataAddress << "\tS " << ((FreeListAllocator::AllocationHeader *)headerAddress)->blockSize << "\tAP " << alignmentPadding
              << "\tP " << padding << "\tM " << m_used << "\tR " << rest << std::endl;
#endif

    return (void *)dataAddress;
}

void FreeListAllocator::Find(const std::size_t size, const std::size_t alignment, std::size_t &padding, Node *&previousNode, Node *&foundNode) {
    switch (m_pPolicy) {
        case FIND_FIRST:
            FindFirst(size, alignment, padding, previousNode, foundNode);
            break;
        case FIND_BEST:
            FindBest(size, alignment, padding, previousNode, foundNode);
            break;
    }
}

void FreeListAllocator::FindFirst(const std::size_t size, const std::size_t alignment, std::size_t &padding, Node *&previousNode, Node *&foundNode) {
    // Iterate list and return the first free block with a size >= than given size
    Node *it = m_freeList.head, *itPrev = nullptr;

    while (it != nullptr) {
        padding = AllocatorUtils::CalculatePaddingWithHeader((std::size_t)it, alignment, sizeof(FreeListAllocator::AllocationHeader));
        const std::size_t requiredSpace = size + padding;
        if (it->data.blockSize >= requiredSpace) {
            break;
        }
        itPrev = it;
        it = it->next;
    }
    previousNode = itPrev;
    foundNode = it;
}

void FreeListAllocator::FindBest(const std::size_t size, const std::size_t alignment, std::size_t &padding, Node *&previousNode, Node *&foundNode) {
    // Iterate WHOLE list keeping a pointer to the best fit
    std::size_t smallestDiff = std::numeric_limits<std::size_t>::max();
    Node *bestBlock = nullptr;
    Node *it = m_freeList.head, *itPrev = nullptr;
    while (it != nullptr) {
        padding = AllocatorUtils::CalculatePaddingWithHeader((std::size_t)it, alignment, sizeof(FreeListAllocator::AllocationHeader));
        const std::size_t requiredSpace = size + padding;
        if (it->data.blockSize >= requiredSpace && (it->data.blockSize - requiredSpace < smallestDiff)) {
            bestBlock = it;
        }
        itPrev = it;
        it = it->next;
    }
    previousNode = itPrev;
    foundNode = bestBlock;
}

void FreeListAllocator::Free(void *ptr) {
    // Insert it in a sorted position by the address number
    const std::size_t currentAddress = (std::size_t)ptr;
    const std::size_t headerAddress = currentAddress - sizeof(FreeListAllocator::AllocationHeader);
    const FreeListAllocator::AllocationHeader *allocationHeader{(FreeListAllocator::AllocationHeader *)headerAddress};

    Node *freeNode = (Node *)(headerAddress);
    freeNode->data.blockSize = allocationHeader->blockSize + allocationHeader->padding;
    freeNode->next = nullptr;

    Node *it = m_freeList.head;
    Node *itPrev = nullptr;
    while (it != nullptr) {
        if (ptr < it) {
            m_freeList.insert(itPrev, freeNode);
            break;
        }
        itPrev = it;
        it = it->next;
    }

    m_used -= freeNode->data.blockSize;

    // Merge contiguous nodes
    Coalescence(itPrev, freeNode);

#ifdef _DEBUG
    std::cout << "F"
              << "\t@ptr " << ptr << "\tH@ " << (void *)freeNode << "\tS " << freeNode->data.blockSize << "\tM " << m_used << std::endl;
#endif
}

void FreeListAllocator::Coalescence(Node *previousNode, Node *freeNode) {
    if (freeNode->next != nullptr && (std::size_t)freeNode + freeNode->data.blockSize == (std::size_t)freeNode->next) {
        freeNode->data.blockSize += freeNode->next->data.blockSize;
        m_freeList.remove(freeNode, freeNode->next);
#ifdef _DEBUG
        std::cout << "\tMerging(n) " << (void *)freeNode << " & " << (void *)freeNode->next << "\tS " << freeNode->data.blockSize << std::endl;
#endif
    }

    if (previousNode != nullptr && (std::size_t)previousNode + previousNode->data.blockSize == (std::size_t)freeNode) {
        previousNode->data.blockSize += freeNode->data.blockSize;
        m_freeList.remove(previousNode, freeNode);
#ifdef _DEBUG
        std::cout << "\tMerging(p) " << (void *)previousNode << " & " << (void *)freeNode << "\tS " << previousNode->data.blockSize << std::endl;
#endif
    }
}

void FreeListAllocator::Reset() {
    m_used = 0;
    m_peak = 0;
    Node *firstNode = (Node *)m_start_ptr;
    firstNode->data.blockSize = m_totalSize;
    firstNode->next = nullptr;
    m_freeList.head = nullptr;
    m_freeList.insert(nullptr, firstNode);
}

LinearAllocator::LinearAllocator(const std::size_t totalSize) : Allocator(totalSize) {}

void LinearAllocator::Init() {
    if (m_start_ptr != nullptr) {
        gc_free(&gc, m_start_ptr);
    }
    m_start_ptr = gc_malloc(&gc, m_totalSize);
    m_offset = 0;
}

LinearAllocator::~LinearAllocator() {
    gc_free(&gc, m_start_ptr);
    m_start_ptr = nullptr;
}

void *LinearAllocator::Allocate(const std::size_t size, const std::size_t alignment) {
    std::size_t padding = 0;
    std::size_t paddedAddress = 0;
    const std::size_t currentAddress = (std::size_t)m_start_ptr + m_offset;

    if (alignment != 0 && m_offset % alignment != 0) {
        // Alignment is required. Find the next aligned memory address and update offset
        padding = AllocatorUtils::CalculatePadding(currentAddress, alignment);
    }

    if (m_offset + padding + size > m_totalSize) {
        return nullptr;
    }

    m_offset += padding;
    const std::size_t nextAddress = currentAddress + padding;
    m_offset += size;

#ifdef _DEBUG
    std::cout << "A"
              << "\t@C " << (void *)currentAddress << "\t@R " << (void *)nextAddress << "\tO " << m_offset << "\tP " << padding << std::endl;
#endif

    m_used = m_offset;
    m_peak = std::max(m_peak, m_used);

    return (void *)nextAddress;
}

void LinearAllocator::Free(void *ptr) { assert(false && "Use Reset() method"); }

void LinearAllocator::Reset() {
    m_offset = 0;
    m_used = 0;
    m_peak = 0;
}

PoolAllocator::PoolAllocator(const std::size_t totalSize, const std::size_t chunkSize) : Allocator(totalSize) {
    assert(chunkSize >= 8 && "Chunk size must be greater or equal to 8");
    assert(totalSize % chunkSize == 0 && "Total Size must be a multiple of Chunk Size");
    this->m_chunkSize = chunkSize;
}

void PoolAllocator::Init() {
    m_start_ptr = gc_malloc(&gc, m_totalSize);
    this->Reset();
}

PoolAllocator::~PoolAllocator() { gc_free(&gc, m_start_ptr); }

void *PoolAllocator::Allocate(const std::size_t allocationSize, const std::size_t alignment) {
    assert(allocationSize == this->m_chunkSize && "Allocation size must be equal to chunk size");

    Node *freePosition = m_freeList.pop();

    assert(freePosition != nullptr && "The pool allocator is full");

    m_used += m_chunkSize;
    m_peak = std::max(m_peak, m_used);
#ifdef _DEBUG
    std::cout << "A"
              << "\t@S " << m_start_ptr << "\t@R " << (void *)freePosition << "\tM " << m_used << std::endl;
#endif

    return (void *)freePosition;
}

void PoolAllocator::Free(void *ptr) {
    m_used -= m_chunkSize;

    m_freeList.push((Node *)ptr);

#ifdef _DEBUG
    std::cout << "F"
              << "\t@S " << m_start_ptr << "\t@F " << ptr << "\tM " << m_used << std::endl;
#endif
}

void PoolAllocator::Reset() {
    m_used = 0;
    m_peak = 0;
    // Create a linked-list with all free positions
    const int nChunks = m_totalSize / m_chunkSize;
    for (int i = 0; i < nChunks; ++i) {
        std::size_t address = (std::size_t)m_start_ptr + i * m_chunkSize;
        m_freeList.push((Node *)address);
    }
}

StackAllocator::StackAllocator(const std::size_t totalSize) : Allocator(totalSize) {}

void StackAllocator::Init() {
    if (m_start_ptr != nullptr) {
        gc_free(&gc, m_start_ptr);
    }
    m_start_ptr = gc_malloc(&gc, m_totalSize);
    m_offset = 0;
}

StackAllocator::~StackAllocator() {
    gc_free(&gc, m_start_ptr);
    m_start_ptr = nullptr;
}

void *StackAllocator::Allocate(const std::size_t size, const std::size_t alignment) {
    const std::size_t currentAddress = (std::size_t)m_start_ptr + m_offset;

    std::size_t padding = AllocatorUtils::CalculatePaddingWithHeader(currentAddress, alignment, sizeof(AllocationHeader));

    if (m_offset + padding + size > m_totalSize) {
        return nullptr;
    }
    m_offset += padding;

    const std::size_t nextAddress = currentAddress + padding;
    const std::size_t headerAddress = nextAddress - sizeof(AllocationHeader);
    AllocationHeader allocationHeader{padding};
    AllocationHeader *headerPtr = (AllocationHeader *)headerAddress;
    headerPtr = &allocationHeader;

    m_offset += size;

#ifdef _DEBUG
    std::cout << "A"
              << "\t@C " << (void *)currentAddress << "\t@R " << (void *)nextAddress << "\tO " << m_offset << "\tP " << padding << std::endl;
#endif
    m_used = m_offset;
    m_peak = std::max(m_peak, m_used);

    return (void *)nextAddress;
}

void StackAllocator::Free(void *ptr) {
    // Move offset back to clear address
    const std::size_t currentAddress = (std::size_t)ptr;
    const std::size_t headerAddress = currentAddress - sizeof(AllocationHeader);
    const AllocationHeader *allocationHeader{(AllocationHeader *)headerAddress};

    m_offset = currentAddress - allocationHeader->padding - (std::size_t)m_start_ptr;
    m_used = m_offset;

#ifdef _DEBUG
    std::cout << "F"
              << "\t@C " << (void *)currentAddress << "\t@F " << (void *)((char *)m_start_ptr + m_offset) << "\tO " << m_offset << std::endl;
#endif
}

void StackAllocator::Reset() {
    m_offset = 0;
    m_used = 0;
    m_peak = 0;
}

#pragma endregion Allocator

#if defined(METADOT_LEAK_TEST)

int const MY_SIZE = 1024 * 512;

static std::array<void *, MY_SIZE> myAlloc{
        nullptr,
};

void *operator new(std::size_t sz) {
    static int counter{};
    void *ptr = std::malloc(sz);
    myAlloc.at(counter++) = ptr;
    // std::cerr << "new." << counter << ".addr.: " << ptr << " size: " << sz << std::endl;
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
    for (auto i : myAlloc) {
        if (i != nullptr) std::cout << " " << i << std::endl;
    }

    std::cout << std::endl;
}

#endif

GarbageCollector gc;

void METAENGINE_Memory_Init(int argc, char *argv[]) {
    gc_start(&gc, &argc);
    AllocationMetrics::C = (CAllocator *)gc_malloc(&gc, sizeof(CAllocator));
    new (AllocationMetrics::C) CAllocator();
    AllocationMetrics::C_Count++;
}

void METAENGINE_Memory_End() {
    if (AllocationMetrics::C) {
        AllocationMetrics::C->~CAllocator();
        gc_free(&gc, AllocationMetrics::C);
        AllocationMetrics::C_Count--;
    }
    gc_stop(&gc);
}

void METAENGINE_Memory_RunGC() { gc_run(&gc); }
