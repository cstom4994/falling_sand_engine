
#pragma once

#include "core.hpp"

#define ME_MEM_PLATFORM_WINDOWS

#define ME_MEM_USE_MALLOC ::malloc
#define ME_MEM_USE_REALLOC ::realloc
#define ME_MEM_USE_FREE ::free
#define ME_MEM_DEBUG_DEV 1

// Activate snapping option (use more memory !)
#define ME_MEM_CAPTURE_ACTIVATED 1
#define ME_MEM_INSTANCE_COUNT_ACTIVATED 1
#define ME_MEM_STATS 1

// #define SINGLE_THREADED 1

#ifndef ME_MEM_TRACER

#define ME_MEM_ALLOC(size) ME_MEM_USE_MALLOC(size)
#define ME_MEM_ALLOC_ALIGNED(size, alignment) ME_MEM_USE_MALLOC(size)
#define ME_MEM_DEALLOC(ptr) ME_MEM_USE_FREE(ptr)
#define ME_MEM_DEALLOC_ALIGNED(ptr) ME_MEM_USE_FREE(ptr)

// #define ME_MEM_REALLOC(ptr, size) ::realloc(ptr, size)
// #define ME_MEM_REALLOC_ALIGNED(ptr, size, alignment) ::realloc(ptr, size, alignment)

#define ME_MEM_DISPLAY(dt) \
    do {                   \
    } while (0)
#define ME_MEM_EXIT() \
    do {              \
    } while (0)
#define ME_MEM_INIT() \
    do {              \
    } while (0)
#define ME_MEM_FLUSH() \
    do {               \
    } while (0)

#else

#define ME_MEM_ALLOC(size) ::ME::mem::alloc(size)
#define ME_MEM_ALLOC_ALIGNED(size, alignment) ::ME::mem::alloc(size)
#define ME_MEM_DEALLOC(ptr) ::ME::mem::dealloc(ptr)
#define ME_MEM_DEALLOC_ALIGNED(ptr) ::ME::mem::dealloc(ptr)
// #define ME_MEM_REALLOC(ptr, size) ::ME::mem::realloc(ptr, size)
// #define ME_MEM_REALLOC_ALIGNED(ptr, size, alignment) ::ME::mem::realloc(ptr, size, alignment)
#define ME_MEM_DISPLAY(dt) ::ME::mem::display(dt)
#define ME_MEM_EXIT() ::ME::mem::exit()
#define ME_MEM_INIT() ::ME::mem::init()
#define ME_MEM_FLUSH() ::ME::mem::getChunk(true)

#include <algorithm>
#include <atomic>  //std::atomic
#include <cassert>
#include <cstdlib>  //malloc etc...
#include <mutex>

#ifndef ME_MEM_ALLOC_NUMBER_PER_CHUNK
#define ME_MEM_ALLOC_NUMBER_PER_CHUNK 1024 * 8
#endif

#ifndef ME_MEM_STACK_SIZE_PER_ALLOC
#define ME_MEM_STACK_SIZE_PER_ALLOC 50
#endif

#ifndef ME_MEM_CHUNK_NUMBER_PER_THREAD
#define ME_MEM_CHUNK_NUMBER_PER_THREAD 8
#endif

#ifndef ME_MEM_CACHE_SIZE
#define ME_MEM_CACHE_SIZE 16
#endif

#ifndef ME_MEM_ALLOC_DICTIONARY_SIZE
#define ME_MEM_ALLOC_DICTIONARY_SIZE 1024 * 16
#endif

#ifndef ME_MEM_STACK_DICTIONARY_SIZE
#define ME_MEM_STACK_DICTIONARY_SIZE 1024 * 16
#endif

#ifndef ME_MEM_TREE_DICTIONARY_SIZE
#define ME_MEM_TREE_DICTIONARY_SIZE 1024 * 16 * 16
#endif

#ifndef ME_MEM_ASSERT
#define ME_MEM_ASSERT(condition, message, ...) assert(condition)
#endif

#ifndef ME_MEM_TREAT_CHUNK
#define ME_MEM_TREAT_CHUNK(chunk) ME::mem::treatChunk(chunk)
#endif

#define ME_MEM_TLS __declspec(thread)
#define ME_MEM_ATOMIC_INITIALIZER(value) value

#include <stdint.h>

namespace ME::mem {
typedef uint32_t Hash;

ME_INLINE void *alloc(size_t size);
ME_INLINE void *allocAligned(size_t size, size_t alignment);
ME_INLINE void dealloc(void *ptr);
ME_INLINE void deallocAligned(void *ptr);
ME_INLINE void *realloc(void *ptr, size_t size);
ME_INLINE void *reallocAligned(void *ptr, size_t size, size_t alignment);
void exit();
void init();
void display(float dt);
struct Chunk;
ME_INLINE Chunk *getChunk(bool forceFlush = false);

namespace SymbolGetter {
void init();
}
static const size_t INTERNAL_MAX_STACK_DEPTH = 255;
static const size_t INTERNAL_FRAME_TO_SKIP = 2;
static const char *TRUNCATED_STACK_NAME = "Truncated\0";
static const char *UNKNOWN_STACK_NAME = "Unknown\0";
static const size_t HISTORY_FRAME_NUMBER = 120;
template <class T>
ME_INLINE Hash combineHash(const T &val, const Hash baseHash = 2166136261U);
static uint32_t getCallstack(uint32_t maxStackSize, void **stack, Hash *hash);

}  // namespace ME::mem

#endif