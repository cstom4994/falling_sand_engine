/*
LiveMemTracer
Code and documentation https://github.com/cesarl/LiveMemTracer
*/

#pragma once


#define METADOT_GC_IMPL 1

// Will enable "capture" feature :
// Capture all stack infos and display diff between capture state
// and current state.
// (use more memory)
#define METADOT_GC_CAPTURE_ACTIVATED 1

// Will enable "instance count" feature :
// Display the number of time functions allocate
// Increment at allocation, decrement at free
// (use more memory)
#define METADOT_GC_INSTANCE_COUNT_ACTIVATED 1

// Add more stats to "(?)" menu tooltip
// Made it easy to setup dictionary size
#define METADOT_GC_STATS 1

// Number of allocs register per chunks
// Adapt it to the number of allocations your program do
// ( default : 1024 * 8 )
#define METADOT_GC_ALLOC_NUMBER_PER_CHUNK 1024

// Max depth of the stack
// ( default : 50 )
#define METADOT_GC_STACK_SIZE_PER_ALLOC 50

// Pre allocated chunks per thread
// ( default : 8 )
#define METADOT_GC_CHUNK_NUMBER_PER_THREAD 4

// Cache size (per thread cache)
// ( default : 16 )
#define METADOT_GC_CACHE_SIZE 8

// Will assert one errors, example if Dictionnary is full
#define METADOT_GC_DEBUG_DEV 1

// Disable imgui
#define METADOT_GC_IMGUI 0

// Allocation dictionnary max entry
// ( default : 1024 * 16 )
#define METADOT_GC_ALLOC_DICTIONARY_SIZE 1024 * 16

// Stack dictionnary max entry
// ( default : 1024 * 16 )
#define METADOT_GC_STACK_DICTIONARY_SIZE 1024 * 16

// Leaf dictionnary max entry
// Depend of the size of your program
// ( default : 1024 * 16 * 16 )
#define METADOT_GC_TREE_DICTIONARY_SIZE 1024 * 16 * 16

#define METADOT_GC_USE_MALLOC ::malloc
#define METADOT_GC_USE_REALLOC ::realloc
#define METADOT_GC_USE_FREE ::free

// #define METADOT_GC_TREAT_CHUNK(chunk) MyTaskScheduler::getInstancer().pushTask([=](){MetaEngine::GCManager::treatChunk(chunk);})


#if defined(_WIN32) || defined(__WINDOWS__) || defined(__WIN32__)
#define METADOT_GC_PLATFORM_WINDOWS
#define METADOT_GC_ENABLED 1
#else
#error "TODO: fix for this compiler!"
#endif

#if METADOT_GC_ENABLED == 0

#define METADOT_GC_ALLOC(size)::malloc(size)
#define METADOT_GC_ALLOC_ALIGNED(size, alignment)::malloc(size)
#define METADOT_GC_DEALLOC(ptr)::free(ptr)
#define METADOT_GC_DEALLOC_ALIGNED(ptr)::free(ptr)
#define METADOT_GC_REALLOC(ptr, size)::realloc(ptr, size)
#define METADOT_GC_REALLOC_ALIGNED(ptr, size, alignment)::realloc(ptr, size, alignment)
#define METADOT_GC_DISPLAY(dt)do{}while(0)
#define METADOT_GC_EXIT()do{}while(0)
#define METADOT_GC_INIT()do{}while(0)
#define METADOT_GC_FLUSH()do{}while(0)

#else //METADOT_GC_ENABLED

#define METADOT_GC_ALLOC(size)::MetaEngine::GCManager::alloc(size)
#define METADOT_GC_ALLOC_ALIGNED(size, alignment)::MetaEngine::GCManager::allocAligned(size, alignment)
#define METADOT_GC_DEALLOC(ptr)::MetaEngine::GCManager::dealloc(ptr)
#define METADOT_GC_DEALLOC_ALIGNED(ptr)::MetaEngine::GCManager::deallocAligned(ptr)
#define METADOT_GC_REALLOC(ptr, size)::MetaEngine::GCManager::realloc(ptr, size)
#define METADOT_GC_REALLOC_ALIGNED(ptr, size, alignment)::MetaEngine::GCManager::reallocAligned(ptr, size, alignment)
#define METADOT_GC_DISPLAY(dt)::MetaEngine::GCManager::display(dt)
#define METADOT_GC_EXIT()::MetaEngine::GCManager::exit()
#define METADOT_GC_INIT() ::MetaEngine::GCManager::init()
#define METADOT_GC_FLUSH()::MetaEngine::GCManager::getChunk(true)

#ifdef METADOT_GC_IMPL

#include <atomic>
#include <cstdlib>
#include <algorithm>
#include <cassert>

#ifdef METADOT_GC_PLATFORM_WINDOWS
#pragma warning(push)
#pragma warning(disable:4265)
#endif
#include <mutex>
#ifdef METADOT_GC_PLATFORM_WINDOWS
#pragma warning(pop)
#endif

#ifndef METADOT_GC_ALLOC_NUMBER_PER_CHUNK
#define METADOT_GC_ALLOC_NUMBER_PER_CHUNK 1024 * 8
#endif

#ifndef METADOT_GC_STACK_SIZE_PER_ALLOC
#define METADOT_GC_STACK_SIZE_PER_ALLOC 50
#endif

#ifndef METADOT_GC_CHUNK_NUMBER_PER_THREAD
#define METADOT_GC_CHUNK_NUMBER_PER_THREAD 8
#endif

#ifndef METADOT_GC_CACHE_SIZE
#define METADOT_GC_CACHE_SIZE 16
#endif

#ifndef METADOT_GC_ALLOC_DICTIONARY_SIZE
#define METADOT_GC_ALLOC_DICTIONARY_SIZE 1024 * 16
#endif

#ifndef METADOT_GC_STACK_DICTIONARY_SIZE
#define METADOT_GC_STACK_DICTIONARY_SIZE 1024 * 16
#endif

#ifndef METADOT_GC_TREE_DICTIONARY_SIZE
#define METADOT_GC_TREE_DICTIONARY_SIZE 1024 * 16 * 16
#endif

#ifndef METADOT_GC_IMGUI
#define METADOT_GC_IMGUI 1
#endif

#ifndef METADOT_GC_ASSERT
#define METADOT_GC_ASSERT(condition, message, ...) assert(condition)
#endif

#ifndef METADOT_GC_TREAT_CHUNK
#define METADOT_GC_TREAT_CHUNK(chunk) MetaEngine::GCManager::treatChunk(chunk)
#endif

#ifndef METADOT_GC_DEBUG_DEV
#define METADOT_GC_DEBUG_ASSERT(condition, message, ...) do{}while(0)
#else
#define METADOT_GC_DEBUG_ASSERT(condition, message, ...) METADOT_GC_ASSERT(condition, message, ##__VA_ARGS__)
#endif

#ifndef METADOT_GC_IMPLEMENTED
#define METADOT_GC_IMPLEMENTED 1
#else
static_assert(false, "GCManager is already implemented, do not define METADOT_GC_IMPL more than once.");
#endif

#endif

#if defined(METADOT_GC_PLATFORM_WINDOWS) 
#undef METADOT_GC_TLS
#undef METADOT_GC_INLINE
#define METADOT_GC_TLS __declspec(thread)
#define METADOT_GC_INLINE __forceinline
#define METADOT_GC_ATOMIC_INITIALIZER(value) value
#endif

#include <cstdint>

namespace MetaEngine::GCManager
{
	typedef uint32_t Hash;

	METADOT_GC_INLINE void *alloc(size_t size);
	METADOT_GC_INLINE void *allocAligned(size_t size, size_t alignment);
	METADOT_GC_INLINE void dealloc(void *ptr);
	METADOT_GC_INLINE void deallocAligned(void *ptr);
	METADOT_GC_INLINE void *realloc(void *ptr, size_t size);
	METADOT_GC_INLINE void *reallocAligned(void *ptr, size_t size, size_t alignment);
	void exit();
	void init();
	void display(float dt);
	struct Chunk;
	METADOT_GC_INLINE Chunk *getChunk(bool forceFlush = false);

	namespace SymbolGetter
	{
		void init();
	}
#ifdef METADOT_GC_IMPL
	static const size_t INTERNAL_MAX_STACK_DEPTH = 255;
	static const size_t INTERNAL_FRAME_TO_SKIP = 2;
	static const char  *TRUNCATED_STACK_NAME = "Truncated\0";
	static const char  *UNKNOWN_STACK_NAME = "Unknown\0";
	static const size_t HISTORY_FRAME_NUMBER = 120;
	template <class T> METADOT_GC_INLINE Hash combineHash(const T& val, const Hash baseHash = 2166136261U);
	static uint32_t getCallstack(uint32_t maxStackSize, void **stack, Hash *hash);
#endif
}

#endif // METADOT_GC_ENABLED
