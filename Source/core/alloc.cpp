// Copyright(c) 2022-2023, KaoruXun All rights reserved.
// Including some codes from https://github.com/mkirchner/gc

#include "alloc.h"

#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "core/stl.h"

// #include "core/c/auto_c.h"

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

static void *gc_mcalloc(size_t count, size_t size) {
    if (!count) return malloc(size);
    return calloc(count, size);
}

static bool gc_needs_sweep(GarbageCollector *gc) { return gc->allocs->size > gc->allocs->sweep_limit; }

static void *gc_allocate(GarbageCollector *gc, size_t count, size_t size, void (*dtor)(void *)) {
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

void *gc_calloc(GarbageCollector *gc, size_t count, size_t size) { return gc_calloc_ext(gc, count, size, NULL); }

void *gc_calloc_ext(GarbageCollector *gc, size_t count, size_t size, void (*dtor)(void *)) { return gc_allocate(gc, count, size, dtor); }

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

#pragma region lua_safe_alloc

void *lua_simple_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    (void)ud;
    (void)osize; /* not used */
    if (nsize == 0) {
        free(ptr);
        return NULL;
    } else
        return realloc(ptr, nsize);
}

struct LuaAllocator *new_allocator(void) {
    struct LuaAllocator *alloc = (LuaAllocator *)malloc(sizeof(struct LuaAllocator));
    alloc->blocks = (LuaMemBlock *)malloc(sizeof(struct LuaMemBlock) * 4);
    alloc->nb_blocks = 0;
    alloc->size_blocks = 4;
    alloc->total_allocated = 0;
#ifdef _DEBUG_ALLOC
    fprintf(stderr, "Created allocator %p\n", alloc);
#endif
    return alloc;
}

void delete_allocator(struct LuaAllocator *alloc) {
    size_t blk;
#ifdef _DEBUG_ALLOC
    fprintf(stderr, "Deleting allocator %p\n", alloc);
#endif
    for (blk = 0; blk < alloc->nb_blocks; ++blk) free(alloc->blocks[blk].ptr);
    free(alloc->blocks);
    free(alloc);
}

void *lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    struct LuaAllocator *alloc = (LuaAllocator *)ud;
    size_t blk;
#ifdef _DEBUG_ALLOC
    {
        size_t i;
        for (i = 0; i < alloc->nb_blocks; ++i) {
            fprintf(stderr, "%p %u ", alloc->blocks[i].ptr, alloc->blocks[i].size);
            if (i % 4 == 0) fprintf(stderr, "\n");
        }
        if (i % 4 != 1)
            fprintf(stderr, "\n\n");
        else
            fprintf(stderr, "\n");
    }
    fprintf(stderr,
            "Lua request on allocator %p: "
            "ptr=%p, osize=%u, nsize=%u\n",
            alloc, ptr, osize, nsize);
#endif
    if (ptr == NULL) {
        if (nsize == 0) {
#ifdef _DEBUG_ALLOC
            fprintf(stderr, "Returning NULL...\n");
#endif
            return NULL;
        }
        blk = alloc->nb_blocks;
        alloc->nb_blocks++;
        if (alloc->nb_blocks > alloc->size_blocks) {
            alloc->size_blocks *= 2;
            alloc->blocks = (LuaMemBlock *)realloc(alloc->blocks, sizeof(struct LuaMemBlock) * alloc->size_blocks);
#ifdef _DEBUG_ALLOC
            fprintf(stderr, "Growing block table to %u blocks\n", alloc->size_blocks);
#endif
        }
        alloc->blocks[blk].ptr = NULL;
        alloc->blocks[blk].size = 0;
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "Allocated new block %u\n", blk);
#endif
    } else {
        /* Bisect to the block */
        size_t low = 0, high = alloc->nb_blocks;
        while (low < high) {
            size_t mid = (low + high) / 2;
            if (alloc->blocks[mid].ptr < ptr)
                low = mid + 1;
            else
                high = mid;
        }
        blk = low;
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "Found block %u\n", blk);
#endif
    }
    assert(alloc->blocks[blk].ptr == ptr && (!ptr || alloc->blocks[blk].size == osize));
    if (nsize == 0) {
        free(ptr);
        alloc->total_allocated -= osize;
        memmove(&alloc->blocks[blk], &alloc->blocks[blk + 1], sizeof(struct LuaMemBlock) * (alloc->nb_blocks - blk - 1));
        alloc->nb_blocks--;
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "Removed block, now have %u blocks, %u bytes\n\n", alloc->nb_blocks, alloc->total_allocated);
#endif
        return NULL;
    } else {
        ptr = realloc(ptr, nsize);
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "ptr=%p ", ptr);
#endif
        alloc->blocks[blk].ptr = ptr;
        alloc->total_allocated += nsize - alloc->blocks[blk].size;
        alloc->blocks[blk].size = nsize;
        while (blk > 0 && alloc->blocks[blk].ptr < alloc->blocks[blk - 1].ptr) {
            struct LuaMemBlock tmp = alloc->blocks[blk];
            alloc->blocks[blk] = alloc->blocks[blk - 1];
            alloc->blocks[blk - 1] = tmp;
            blk--;
#ifdef _DEBUG_ALLOC
            fprintf(stderr, "< ");
#endif
        }
        while (blk + 1 < alloc->nb_blocks && alloc->blocks[blk].ptr > alloc->blocks[blk + 1].ptr) {
            struct LuaMemBlock tmp = alloc->blocks[blk];
            alloc->blocks[blk] = alloc->blocks[blk + 1];
            alloc->blocks[blk + 1] = tmp;
            blk++;
#ifdef _DEBUG_ALLOC
            fprintf(stderr, "> ");
#endif
        }
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "\n");
        fprintf(stderr, "Moved block to %u, now have %u bytes\n\n", blk, alloc->total_allocated);
#endif
        return ptr;
    }
}

#pragma endregion lua_safe_alloc

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