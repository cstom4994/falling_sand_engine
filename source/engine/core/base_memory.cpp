#include <array>
#include <cassert>
#include <cerrno>
#include <csetjmp>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "engine/core/io/filesystem.h"
#include "engine/utils/utility.hpp"
#include "memory.h"

namespace ME {

allocation_metrics g_allocation_metrics = {.total_allocated = 0, .total_free = 0};

u64 ME_mem_current_usage_bytes() { return g_allocation_metrics.total_allocated - g_allocation_metrics.total_free; }
f32 ME_mem_current_usage_mb() {
    u64 bytes = ME_mem_current_usage_bytes();
    return (f32)(bytes / 1048576.0f);
}

struct ME_mem_alloc_stack_t {
    void* memory;
    size_t capacity;
    size_t bytes_left;
};

#define ME_PTR_ADD(ptr, size) ((void*)(((char*)ptr) + (size)))
#define ME_PTR_SUB(ptr, size) ((void*)(((char*)ptr) - (size)))

ME_mem_alloc_stack_t* ME_mem_alloc_stack_create(void* memory_chunk, size_t size) {
    ME_mem_alloc_stack_t* stack = (ME_mem_alloc_stack_t*)memory_chunk;
    if (size < sizeof(ME_mem_alloc_stack_t)) return 0;
    *(size_t*)ME_PTR_ADD(memory_chunk, sizeof(ME_mem_alloc_stack_t)) = 0;
    stack->memory = ME_PTR_ADD(memory_chunk, sizeof(ME_mem_alloc_stack_t) + sizeof(size_t));
    stack->capacity = size - sizeof(ME_mem_alloc_stack_t) - sizeof(size_t);
    stack->bytes_left = stack->capacity;
    return stack;
}

void* ME_mem_alloc_stack_alloc(ME_mem_alloc_stack_t* stack, size_t size) {
    if (stack->bytes_left - sizeof(size_t) < size) return 0;
    void* user_mem = stack->memory;
    *(size_t*)ME_PTR_ADD(user_mem, size) = size;
    stack->memory = ME_PTR_ADD(user_mem, size + sizeof(size_t));
    stack->bytes_left -= sizeof(size_t) + size;
    return user_mem;
}

int ME_mem_alloc_stack_free(ME_mem_alloc_stack_t* stack, void* memory) {
    if (!memory) return 0;
    size_t size = *(size_t*)ME_PTR_SUB(stack->memory, sizeof(size_t));
    void* prev = ME_PTR_SUB(stack->memory, size + sizeof(size_t));
    if (prev != memory) return 0;
    stack->memory = prev;
    stack->bytes_left += sizeof(size_t) + size;
    return 1;
}

size_t ME_mem_alloc_stack_bytes_left(ME_mem_alloc_stack_t* stack) { return stack->bytes_left; }

struct ME_mem_alloc_frame_t {
    void* original;
    void* ptr;
    size_t capacity;
    size_t bytes_left;
};

ME_mem_alloc_frame_t* ME_mem_alloc_frame_create(void* memory_chunk, size_t size) {
    ME_mem_alloc_frame_t* frame = (ME_mem_alloc_frame_t*)memory_chunk;
    frame->original = ME_PTR_ADD(memory_chunk, sizeof(ME_mem_alloc_frame_t));
    frame->ptr = frame->original;
    frame->capacity = frame->bytes_left = size - sizeof(ME_mem_alloc_frame_t);
    return frame;
}

void* ME_mem_alloc_frame_alloc(ME_mem_alloc_frame_t* frame, size_t size) {
    if (frame->bytes_left < size) return 0;
    void* user_mem = frame->ptr;
    frame->ptr = ME_PTR_ADD(frame->ptr, size);
    frame->bytes_left -= size;
    return user_mem;
}

void ME_mem_alloc_frame_free(ME_mem_alloc_frame_t* frame) {
    frame->ptr = frame->original;
    frame->bytes_left = frame->capacity;
}

typedef struct ME_mem_alloc_heap_header_t {
    struct ME_mem_alloc_heap_header_t* next;
    struct ME_mem_alloc_heap_header_t* prev;
    size_t size;
} ME_mem_alloc_heap_header_t;

typedef struct ME_mem_alloc_alloc_info_t ME_mem_alloc_alloc_info_t;
struct ME_mem_alloc_alloc_info_t {
    const char* file;
    size_t size;
    int line;

    struct ME_mem_alloc_alloc_info_t* next;
    struct ME_mem_alloc_alloc_info_t* prev;
};

static ME_mem_alloc_alloc_info_t* ME_mem_alloc_alloc_head() {
    static ME_mem_alloc_alloc_info_t info;
    static int init;

    if (!init) {
        info.next = &info;
        info.prev = &info;
        init = 1;
    }

    return &info;
}

#if 1
void* ME_mem_alloc_leak_check_alloc(size_t size, const char* file, int line, size_t* statistics) {
    ME_mem_alloc_alloc_info_t* mem = (ME_mem_alloc_alloc_info_t*)ME_MALLOC_FUNC(sizeof(ME_mem_alloc_alloc_info_t) + size);

    if (!mem) return 0;

    mem->file = file;
    mem->line = line;
    mem->size = size;
    ME_mem_alloc_alloc_info_t* head = ME_mem_alloc_alloc_head();
    mem->prev = head;
    mem->next = head->next;
    head->next->prev = mem;
    head->next = mem;

    g_allocation_metrics.total_allocated += size;
    if (NULL != statistics) *statistics += size;

    return mem + 1;
}

void* ME_mem_alloc_leak_check_calloc(size_t count, size_t element_size, const char* file, int line, size_t* statistics) {
    size_t size = count * element_size;
    void* mem = ME_mem_alloc_leak_check_alloc(size, file, line, statistics);
    std::memset(mem, 0, size);
    return mem;
}

void ME_mem_alloc_leak_check_free(void* mem, size_t* statistics) {
    if (!mem) return;

    ME_mem_alloc_alloc_info_t* info = (ME_mem_alloc_alloc_info_t*)mem - 1;
    info->prev->next = info->next;
    info->next->prev = info->prev;

    g_allocation_metrics.total_free += info->size;
    if (NULL != statistics) *statistics -= info->size;

    ME_FREE_FUNC(info);
}

int ME_mem_check_leaks(bool detailed) {
    ME_mem_alloc_alloc_info_t* head = ME_mem_alloc_alloc_head();
    ME_mem_alloc_alloc_info_t* next = head->next;
    int leaks = 0;

    std::size_t leaks_size = 0;

    while (next != head) {
        if (detailed) {
            METADOT_WARN(std::format("[Mem] LEAKED {0} bytes from file \"{1}\" at line {2} from address {3}.", next->size, ME::ME_fs_get_filename(next->file), next->line, (void*)(next + 1)).c_str());
        }
        leaks_size += next->size;
        next = next->next;
        leaks = 1;
    }

    if (leaks && detailed) {
        METADOT_INFO("[Mem] Memory leaks detected (see above).");
    } else if (leaks) {
        double megabytes = static_cast<double>(leaks_size) / 1048576;
        METADOT_INFO(std::format("[Mem] Memory leaks detected with {0} bytes equal to {1:.4f} MB.", leaks_size, megabytes).c_str());
    } else {
        METADOT_BUG("[Mem] No memory leaks detected.");
    }
    return leaks;
}

int ME_mem_bytes_inuse() {
    ME_mem_alloc_alloc_info_t* head = ME_mem_alloc_alloc_head();
    ME_mem_alloc_alloc_info_t* next = head->next;
    int bytes = 0;

    while (next != head) {
        bytes += (int)next->size;
        next = next->next;
    }

    return bytes;
}

#else

inline void* ME_mem_alloc_leak_check_alloc(size_t size, char* file, int line) { return ME_MALLOC_FUNC(size); }

void* ME_mem_alloc_leak_check_calloc(size_t count, size_t element_size, char* file, int line) { return ME_CALLOC_FUNC(count, size); }

inline void ME_mem_alloc_leak_check_free(void* mem) { return ME_FREE_FUNC(mem); }

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

void ME_mem_init(int argc, char* argv[]) {}

void ME_mem_end() { ME_mem_check_leaks(false); }

void ME_mem_rungc() {}

}  // namespace ME