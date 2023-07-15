#ifndef ME_MEMORY_HPP
#define ME_MEMORY_HPP

#include <type_traits>

#include "basic_types.h"

//--------------------------------------------------------------------------------------------------------------------------------//
// MEMORY FUNCTIONS USED

#ifndef ME_MALLOC_FUNC
#define ME_MALLOC_FUNC(s) malloc((s))
#endif

#ifndef ME_FREE_FUNC
#define ME_FREE_FUNC(p) free((p))
#endif

// #ifndef ME_REALLOC
// #define ME_REALLOC(p, s) realloc(p, s)
// #endif

typedef struct ME_mem_alloc_stack_t ME_mem_alloc_stack_t;
ME_mem_alloc_stack_t* ME_mem_alloc_stack_create(void* memory_chunk, size_t size);
void* ME_mem_alloc_stack_alloc(ME_mem_alloc_stack_t* stack, size_t size);
int ME_mem_alloc_stack_free(ME_mem_alloc_stack_t* stack, void* memory);
size_t ME_mem_alloc_stack_bytes_left(ME_mem_alloc_stack_t* stack);

typedef struct ME_mem_alloc_frame_t ME_mem_alloc_frame_t;
ME_mem_alloc_frame_t* ME_mem_alloc_frame_create(void* memory_chunk, size_t size);
void* ME_mem_alloc_frame_alloc(ME_mem_alloc_frame_t* frame, size_t size);
void ME_mem_alloc_frame_free(ME_mem_alloc_frame_t* frame);

// define these to your own user definition as necessary
#ifndef ME_MALLOC
#define ME_MALLOC(size) ME_mem_alloc_leak_check_alloc((size), (char*)__FILE__, __LINE__)
#endif

#ifndef ME_FREE
#define ME_FREE(mem) ME_mem_alloc_leak_check_free(mem)
#endif

#ifndef ME_CALLOC
#define ME_CALLOC(count, element_size) ME_mem_alloc_leak_check_calloc(count, element_size, (char*)__FILE__, __LINE__)
#endif

#ifndef ME_NEW
#define ME_NEW(_name, _class, ...)      \
    (_class*)ME_MALLOC(sizeof(_class)); \
    new ((void*)_name) _class(__VA_ARGS__)
#endif

template <typename T>
struct alloc {
    template <typename... Args>
    static T* safe_malloc(Args&&... args) {
        void* mem = ME_MALLOC(sizeof(T));
        if (!mem) {
        }
        return new (mem) T(std::forward<Args>(args)...);
    }
};

#ifndef ME_DELETE
#define ME_DELETE(_name, _class) \
    _name->~_class();            \
    ME_FREE(_name)
#endif

void* ME_mem_alloc_leak_check_alloc(size_t size, const char* file, int line);
void* ME_mem_alloc_leak_check_calloc(size_t count, size_t element_size, const char* file, int line);
void ME_mem_alloc_leak_check_free(void* mem);
int ME_mem_check_leaks(bool detailed);
int ME_mem_bytes_inuse();

typedef struct allocation_metrics {
    u64 total_allocated;
    u64 total_free;
} allocation_metrics;

extern allocation_metrics g_allocation_metrics;

u64 ME_mem_current_usage_bytes();
f32 ME_mem_current_usage_mb();

void ME_mem_init(int argc, char* argv[]);
void ME_mem_end();
void ME_mem_rungc();

// #define ME_LEAK_TEST

#if defined(ME_LEAK_TEST)

void* operator new(std::size_t sz);
void operator delete(void* ptr) noexcept;

void ME_get_memory_leak();

#endif

#endif