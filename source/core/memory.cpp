#include "memory.h"

#include <array>
#include <cassert>
#include <cerrno>
#include <csetjmp>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "core/memory_tracer.hpp"
// #include "utility/utility.hpp"

allocation_metrics g_allocation_metrics = {.total_allocated = 0, .total_free = 0};

u64 ME_mem_current_usage_bytes() { return g_allocation_metrics.total_allocated - g_allocation_metrics.total_free; }
f32 ME_mem_current_usage_mb() {
    u64 bytes = ME_mem_current_usage_bytes();
    return (f32)(bytes / 1048576.0f);
}

// // https://open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3536.html

// void *operator new(size_t size) {
//     // ME_BUG("Alloc %d memory", size);
//     g_allocation_metrics.TotalAllocated += size;

//     return std::malloc(size);
// }
// void operator delete(void *ptr, size_t size) throw() {
//     // ME_BUG("Free %d memory", size);
//     g_allocation_metrics.TotalFree += size;

//     return std::free(ptr);
// }

// void *operator new(size_t size, const std::nothrow_t &) throw() {
//     g_allocation_metrics.TotalAllocated += size;

//     return std::malloc(size);
// }

// void operator delete(void *ptr, size_t size, const std::nothrow_t &) throw() {
//     g_allocation_metrics.TotalFree += size;

//     return std::free(ptr);
// }

// // void *operator new(size_t size, size_t alignment) noexcept(false) {
// //     g_allocation_metrics.TotalAllocated += size;
// //
// //     return std::aligned_alloc(alignment, size);
// // }

// void operator delete(void *ptr, size_t size, size_t alignment) throw() {
//     g_allocation_metrics.TotalFree += size;

//     return std::free(ptr);
// }

// // void *operator new(size_t size, size_t alignment, const std::nothrow_t &) throw() {
// //     g_allocation_metrics.TotalAllocated += size;
// //
// //     return std::aligned_alloc(alignment, size);
// // }

// void operator delete(void *ptr, size_t size, size_t alignment, const std::nothrow_t &) throw() {
//     g_allocation_metrics.TotalFree += size;

//     return std::free(ptr);
// }

// void *operator new[](size_t size) noexcept(false) {
//     g_allocation_metrics.TotalAllocated += size;

//     return std::malloc(size);
// }

// void operator delete[](void *ptr, size_t size) throw() {
//     g_allocation_metrics.TotalFree += size;

//     return std::free(ptr);
// }

// void *operator new[](size_t size, const std::nothrow_t &) throw() {
//     g_allocation_metrics.TotalAllocated += size;

//     return std::malloc(size);
// }

// void operator delete[](void *ptr, size_t size, const std::nothrow_t &) throw() {
//     g_allocation_metrics.TotalFree += size;

//     return std::free(ptr);
// }

// // void *operator new[](size_t size, size_t alignment) noexcept(false) {
// //     g_allocation_metrics.TotalAllocated += size;
// //
// //     return std::aligned_alloc(alignment, size);
// // }

// void operator delete[](void *ptr, size_t size, size_t alignment) throw() {
//     g_allocation_metrics.TotalFree += size;

//     return std::free(ptr);
// }

// // void *operator new[](size_t size, size_t alignment, const std::nothrow_t &) throw() {
// //     g_allocation_metrics.TotalAllocated += size;
// //
// //     return std::aligned_alloc(alignment, size);
// // }

// void operator delete[](void *ptr, size_t size, size_t alignment, const std::nothrow_t &) throw() {
//     g_allocation_metrics.TotalFree += size;

//     return std::free(ptr);
// }

struct ME_memory_alloc_stack_t {
    void* memory;
    size_t capacity;
    size_t bytes_left;
};

#define ME_PTR_ADD(ptr, size) ((void*)(((char*)ptr) + (size)))
#define ME_PTR_SUB(ptr, size) ((void*)(((char*)ptr) - (size)))

ME_memory_alloc_stack_t* ME_memory_alloc_stack_create(void* memory_chunk, size_t size) {
    ME_memory_alloc_stack_t* stack = (ME_memory_alloc_stack_t*)memory_chunk;
    if (size < sizeof(ME_memory_alloc_stack_t)) return 0;
    *(size_t*)ME_PTR_ADD(memory_chunk, sizeof(ME_memory_alloc_stack_t)) = 0;
    stack->memory = ME_PTR_ADD(memory_chunk, sizeof(ME_memory_alloc_stack_t) + sizeof(size_t));
    stack->capacity = size - sizeof(ME_memory_alloc_stack_t) - sizeof(size_t);
    stack->bytes_left = stack->capacity;
    return stack;
}

void* ME_memory_alloc_stack_alloc(ME_memory_alloc_stack_t* stack, size_t size) {
    if (stack->bytes_left - sizeof(size_t) < size) return 0;
    void* user_mem = stack->memory;
    *(size_t*)ME_PTR_ADD(user_mem, size) = size;
    stack->memory = ME_PTR_ADD(user_mem, size + sizeof(size_t));
    stack->bytes_left -= sizeof(size_t) + size;
    return user_mem;
}

int ME_memory_alloc_stack_free(ME_memory_alloc_stack_t* stack, void* memory) {
    if (!memory) return 0;
    size_t size = *(size_t*)ME_PTR_SUB(stack->memory, sizeof(size_t));
    void* prev = ME_PTR_SUB(stack->memory, size + sizeof(size_t));
    if (prev != memory) return 0;
    stack->memory = prev;
    stack->bytes_left += sizeof(size_t) + size;
    return 1;
}

size_t ME_memory_alloc_stack_bytes_left(ME_memory_alloc_stack_t* stack) { return stack->bytes_left; }

struct ME_memory_alloc_frame_t {
    void* original;
    void* ptr;
    size_t capacity;
    size_t bytes_left;
};

ME_memory_alloc_frame_t* ME_memory_alloc_frame_create(void* memory_chunk, size_t size) {
    ME_memory_alloc_frame_t* frame = (ME_memory_alloc_frame_t*)memory_chunk;
    frame->original = ME_PTR_ADD(memory_chunk, sizeof(ME_memory_alloc_frame_t));
    frame->ptr = frame->original;
    frame->capacity = frame->bytes_left = size - sizeof(ME_memory_alloc_frame_t);
    return frame;
}

void* ME_memory_alloc_frame_alloc(ME_memory_alloc_frame_t* frame, size_t size) {
    if (frame->bytes_left < size) return 0;
    void* user_mem = frame->ptr;
    frame->ptr = ME_PTR_ADD(frame->ptr, size);
    frame->bytes_left -= size;
    return user_mem;
}

void ME_memory_alloc_frame_free(ME_memory_alloc_frame_t* frame) {
    frame->ptr = frame->original;
    frame->bytes_left = frame->capacity;
}

typedef struct ME_memory_alloc_heap_header_t {
    struct ME_memory_alloc_heap_header_t* next;
    struct ME_memory_alloc_heap_header_t* prev;
    size_t size;
} ME_memory_alloc_heap_header_t;

typedef struct ME_memory_alloc_alloc_info_t ME_memory_alloc_alloc_info_t;
struct ME_memory_alloc_alloc_info_t {
    const char* file;
    size_t size;
    int line;

    struct ME_memory_alloc_alloc_info_t* next;
    struct ME_memory_alloc_alloc_info_t* prev;
};

static ME_memory_alloc_alloc_info_t* ME_memory_alloc_alloc_head() {
    static ME_memory_alloc_alloc_info_t info;
    static int init;

    if (!init) {
        info.next = &info;
        info.prev = &info;
        init = 1;
    }

    return &info;
}

#if 1
void* ME_memory_alloc_leak_check_alloc(size_t size, const char* file, int line) {
    ME_memory_alloc_alloc_info_t* mem = (ME_memory_alloc_alloc_info_t*)ME_MALLOC_FUNC(sizeof(ME_memory_alloc_alloc_info_t) + size);

    if (!mem) return 0;

    mem->file = file;
    mem->line = line;
    mem->size = size;
    ME_memory_alloc_alloc_info_t* head = ME_memory_alloc_alloc_head();
    mem->prev = head;
    mem->next = head->next;
    head->next->prev = mem;
    head->next = mem;

    g_allocation_metrics.total_allocated += size;

    return mem + 1;
}

void* ME_memory_alloc_leak_check_calloc(size_t count, size_t element_size, const char* file, int line) {
    size_t size = count * element_size;
    void* mem = ME_memory_alloc_leak_check_alloc(size, file, line);
    std::memset(mem, 0, size);
    return mem;
}

void ME_memory_alloc_leak_check_free(void* mem) {
    if (!mem) return;

    ME_memory_alloc_alloc_info_t* info = (ME_memory_alloc_alloc_info_t*)mem - 1;
    info->prev->next = info->next;
    info->next->prev = info->prev;

    g_allocation_metrics.total_free += info->size;

    ME_FREE_FUNC(info);
}

int ME_memory_check_leaks() {
    ME_memory_alloc_alloc_info_t* head = ME_memory_alloc_alloc_head();
    ME_memory_alloc_alloc_info_t* next = head->next;
    int leaks = 0;

    while (next != head) {
        // ME_WARN(std::format("[Mem] LEAKED {0} bytes from file \"{1}\" at line {2} from address {3}.", next->size, next->file, next->line, (void*)(next + 1)).c_str());
        next = next->next;
        leaks = 1;
    }

    if (leaks) {
        // ME_INFO("[Mem] Memory leaks detected (see above).");
    } else {
        // ME_INFO("[Mem] No memory leaks detected.");
    }
    return leaks;
}

int ME_memory_bytes_inuse() {
    ME_memory_alloc_alloc_info_t* head = ME_memory_alloc_alloc_head();
    ME_memory_alloc_alloc_info_t* next = head->next;
    int bytes = 0;

    while (next != head) {
        bytes += (int)next->size;
        next = next->next;
    }

    return bytes;
}

#else

inline void* ME_memory_alloc_leak_check_alloc(size_t size, char* file, int line) { return ME_MALLOC_FUNC(size); }

void* ME_memory_alloc_leak_check_calloc(size_t count, size_t element_size, char* file, int line) { return ME_CALLOC_FUNC(count, size); }

inline void ME_memory_alloc_leak_check_free(void* mem) { return ME_FREE_FUNC(mem); }

inline int ME_CHECK_FOR_LEAKS() { return 0; }
inline int ME_BYTES_IN_USE() { return 0; }

#endif

#if defined(ME_LEAK_TEST)

int const MY_SIZE = 1024 * 512;

static std::array<void*, MY_SIZE> myAlloc{
        nullptr,
};

void* operator new(std::size_t sz) {
    static int counter{};
    void* ptr = std::malloc(sz);
    myAlloc.at(counter++) = ptr;
    // std::cout << "new." << counter << ".addr.: " << ptr << " size: " << sz << std::endl;
    return ptr;
}

void operator delete(void* ptr) noexcept {
    auto ind = std::distance(myAlloc.begin(), std::find(myAlloc.begin(), myAlloc.end(), ptr));
    myAlloc[ind] = nullptr;
    std::free(ptr);
}

void ME_get_memory_leak() {

    std::cout << std::endl;
    std::cout << "Not deallocated: " << std::endl;
    for (auto i : myAlloc) {
        if (i != nullptr) std::cout << " " << i << std::endl;
    }
    std::cout << std::endl;
}

#endif

void ME_mem_init(int argc, char* argv[]) { ME_MEM_INIT(); }

void ME_mem_end() {
    ME_MEM_EXIT();
    ME_memory_check_leaks();
}

void ME_mem_rungc() {}
