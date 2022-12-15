// Copyright(c) 2022, KaoruXun All rights reserved.

// Now I'm using gc(https://github.com/mkirchner/gc) (MIT license)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct AllocationMap;

typedef struct GarbageCollector
{
    struct AllocationMap *allocs;// allocation map
    bool paused;                 // (temporarily) switch gc on/off
    void *bos;                   // bottom of stack
    size_t min_size;
} GarbageCollector;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    extern GarbageCollector gc;// Global garbage collector for all
                               // single-threaded applications

    void gc_start(GarbageCollector *gc, void *bos);
    void gc_start_ext(GarbageCollector *gc, void *bos, size_t initial_size, size_t min_size,
                      double downsize_load_factor, double upsize_load_factor, double sweep_factor);
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

#ifdef __cplusplus
}
#endif /* __cplusplus */
