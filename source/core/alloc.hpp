// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_ALLOC_HPP_
#define _METADOT_ALLOC_HPP_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>

#include "core/core.h"
#include "core/core.hpp"
#include "core/macros.h"

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

class AllocatorUtils {
public:
    static const std::size_t CalculatePadding(const std::size_t baseAddress, const std::size_t alignment) {
        const std::size_t multiplier = (baseAddress / alignment) + 1;
        const std::size_t alignedAddress = multiplier * alignment;
        const std::size_t padding = alignedAddress - baseAddress;
        return padding;
    }

    static const std::size_t CalculatePaddingWithHeader(const std::size_t baseAddress, const std::size_t alignment, const std::size_t headerSize) {
        std::size_t padding = CalculatePadding(baseAddress, alignment);
        std::size_t neededSpace = headerSize;

        if (padding < neededSpace) {
            // Header does not fit - Calculate next aligned address that header fits
            neededSpace -= padding;

            // How many alignments I need to fit the header
            if (neededSpace % alignment > 0) {
                padding += alignment * (1 + (neededSpace / alignment));
            } else {
                padding += alignment * (neededSpace / alignment);
            }
        }

        return padding;
    }
};

class Allocator {
protected:
    std::size_t m_totalSize;
    std::size_t m_used;
    std::size_t m_peak;

public:
    Allocator(const std::size_t totalSize) : m_totalSize{totalSize}, m_used{0}, m_peak{0} {}

    virtual ~Allocator() { m_totalSize = 0; }

    virtual void *Allocate(const std::size_t size, const std::size_t alignment = 0) = 0;

    virtual void Free(void *ptr) = 0;

    virtual void Init() = 0;

    friend class Benchmark;
};

class CAllocator : public Allocator {
public:
    CAllocator();

    virtual ~CAllocator();

    virtual void *Allocate(const std::size_t size, const std::size_t alignment = 0) override;

    virtual void Free(void *ptr) override;

    virtual void Init() override;
};

template <class T>
class DoublyLinkedList {
public:
    struct Node {
        T data;
        Node *previous;
        Node *next;
    };
    Node *head;

public:
    DoublyLinkedList();

    void insert(Node *previousNode, Node *newNode);
    void remove(Node *deleteNode);

private:
    DoublyLinkedList(DoublyLinkedList &doublyLinkedList);
};

template <class T>
DoublyLinkedList<T>::DoublyLinkedList() {}

template <class T>
void DoublyLinkedList<T>::insert(Node *previousNode, Node *newNode) {
    if (previousNode == nullptr) {
        // Is the first node
        if (head != nullptr) {
            // The list has more elements
            newNode->next = head;
            newNode->next->previous = newNode;
        } else {
            newNode->next = nullptr;
        }
        head = newNode;
        head->previous = nullptr;
    } else {
        if (previousNode->next == nullptr) {
            // Is the last node
            previousNode->next = newNode;
            newNode->next = nullptr;
        } else {
            // Is a middle node
            newNode->next = previousNode->next;
            if (newNode->next != nullptr) {
                newNode->next->previous = newNode;
            }
            previousNode->next = newNode;
            newNode->previous = previousNode;
        }
    }
}

template <class T>
void DoublyLinkedList<T>::remove(Node *deleteNode) {
    if (deleteNode->previous == nullptr) {
        // Is the first node
        if (deleteNode->next == nullptr) {
            // List only has one element
            head = nullptr;
        } else {
            // List has more elements
            head = deleteNode->next;
            head->previous = nullptr;
        }
    } else {
        if (deleteNode->next == nullptr) {
            // Is the last node
            deleteNode->previous->next = nullptr;
        } else {
            // Middle node
            deleteNode->previous->next = deleteNode->next;
            deleteNode->next->previous = deleteNode->previous;
        }
    }
}

class StackAllocator : public Allocator {
protected:
    void *m_start_ptr = nullptr;
    std::size_t m_offset;

public:
    StackAllocator(const std::size_t totalSize);

    virtual ~StackAllocator();

    virtual void *Allocate(const std::size_t size, const std::size_t alignment = 0) override;

    virtual void Free(void *ptr);

    virtual void Init() override;

    virtual void Reset();

private:
    StackAllocator(StackAllocator &stackAllocator);

    struct AllocationHeader {
        std::size_t padding;
    };
};

template <class T>
class StackLinkedList {
public:
    struct Node {
        T data;
        Node *next;
    };

    Node *head;

public:
    StackLinkedList() = default;
    StackLinkedList(StackLinkedList &stackLinkedList) = delete;
    void push(Node *newNode);
    Node *pop();
};

template <class T>
void StackLinkedList<T>::push(Node *newNode) {
    newNode->next = head;
    head = newNode;
}

template <class T>
typename StackLinkedList<T>::Node *StackLinkedList<T>::pop() {
    Node *top = head;
    head = head->next;
    return top;
}

class PoolAllocator : public Allocator {
private:
    struct FreeHeader {};
    using Node = StackLinkedList<FreeHeader>::Node;
    StackLinkedList<FreeHeader> m_freeList;

    void *m_start_ptr = nullptr;
    std::size_t m_chunkSize;

public:
    PoolAllocator(const std::size_t totalSize, const std::size_t chunkSize);

    virtual ~PoolAllocator();

    virtual void *Allocate(const std::size_t size, const std::size_t alignment = 0) override;

    virtual void Free(void *ptr) override;

    virtual void Init() override;

    virtual void Reset();

private:
    PoolAllocator(PoolAllocator &poolAllocator);
};

class LinearAllocator : public Allocator {
protected:
    void *m_start_ptr = nullptr;
    std::size_t m_offset;

public:
    LinearAllocator(const std::size_t totalSize);

    virtual ~LinearAllocator();

    virtual void *Allocate(const std::size_t size, const std::size_t alignment = 0) override;

    virtual void Free(void *ptr) override;

    virtual void Init() override;

    virtual void Reset();

private:
    LinearAllocator(LinearAllocator &linearAllocator);
};

template <class T>
class SinglyLinkedList {
public:
    struct Node {
        T data;
        Node *next;
    };

    Node *head;

public:
    SinglyLinkedList();

    void insert(Node *previousNode, Node *newNode);
    void remove(Node *previousNode, Node *deleteNode);
};

template <class T>
SinglyLinkedList<T>::SinglyLinkedList() {}

template <class T>
void SinglyLinkedList<T>::insert(Node *previousNode, Node *newNode) {
    if (previousNode == nullptr) {
        // Is the first node
        if (head != nullptr) {
            // The list has more elements
            newNode->next = head;
        } else {
            newNode->next = nullptr;
        }
        head = newNode;
    } else {
        if (previousNode->next == nullptr) {
            // Is the last node
            previousNode->next = newNode;
            newNode->next = nullptr;
        } else {
            // Is a middle node
            newNode->next = previousNode->next;
            previousNode->next = newNode;
        }
    }
}

template <class T>
void SinglyLinkedList<T>::remove(Node *previousNode, Node *deleteNode) {
    if (previousNode == nullptr) {
        // Is the first node
        if (deleteNode->next == nullptr) {
            // List only has one element
            head = nullptr;
        } else {
            // List has more elements
            head = deleteNode->next;
        }
    } else {
        previousNode->next = deleteNode->next;
    }
}

class FreeListAllocator : public Allocator {
public:
    enum PlacementPolicy { FIND_FIRST, FIND_BEST };

private:
    struct FreeHeader {
        std::size_t blockSize;
    };
    struct AllocationHeader {
        std::size_t blockSize;
        char padding;
    };

    typedef SinglyLinkedList<FreeHeader>::Node Node;

    void *m_start_ptr = nullptr;
    PlacementPolicy m_pPolicy;
    SinglyLinkedList<FreeHeader> m_freeList;

public:
    FreeListAllocator(const std::size_t totalSize, const PlacementPolicy pPolicy);

    virtual ~FreeListAllocator();

    virtual void *Allocate(const std::size_t size, const std::size_t alignment = 0) override;

    virtual void Free(void *ptr) override;

    virtual void Init() override;

    virtual void Reset();

private:
    FreeListAllocator(FreeListAllocator &freeListAllocator);

    void Coalescence(Node *prevBlock, Node *freeBlock);

    void Find(const std::size_t size, const std::size_t alignment, std::size_t &padding, Node *&previousNode, Node *&foundNode);
    void FindBest(const std::size_t size, const std::size_t alignment, std::size_t &padding, Node *&previousNode, Node *&foundNode);
    void FindFirst(const std::size_t size, const std::size_t alignment, std::size_t &padding, Node *&previousNode, Node *&foundNode);
};

#define METADOT_GC_ALLOC(size) malloc(size)
#define METADOT_GC_ALLOC_ALIGNED(size, alignment) malloc(size)
#define METADOT_GC_DEALLOC(ptr) free(ptr)
#define METADOT_GC_DEALLOC_ALIGNED(ptr) free(ptr)
#define METADOT_GC_REALLOC(ptr, size) realloc(ptr, size)
#define METADOT_GC_REALLOC_ALIGNED(ptr, size, alignment) realloc(ptr, size, alignment)

#if defined(METADOT_DEBUG)
// #define ADDTODEBUGMAP(_c) AllocationMetrics::MemoryDebugMap.insert(std::make_pair(MetaEngine::Type_of<_c>.GetName(), sizeof(_c)))
// #define REMOVEDEBUGMAP(_c) AllocationMetrics::MemoryDebugMap.erase(MetaEngine::Type_of<_c>.GetName())
#define ADDTODEBUGMAP(_c) AllocationMetrics::MemoryDebugMap.insert(std::make_pair(METADOT_VARIABLE_NAME(_c), sizeof(_c)))
#define REMOVEDEBUGMAP(_c) AllocationMetrics::MemoryDebugMap.erase(METADOT_VARIABLE_NAME(_c))
#else
#define ADDTODEBUGMAP(_c)
#define REMOVEDEBUGMAP(_c)
#endif

#define METADOT_NEW(_field, _ptr, _class, ...)                                \
    {                                                                         \
        _ptr = (_class *)AllocationMetrics::_field->Allocate(sizeof(_class)); \
        new (_ptr) _class(__VA_ARGS__);                                       \
        AllocationMetrics::_field##_Count++;                                  \
        ADDTODEBUGMAP(_class);                                                \
    }

#define METADOT_NEW_ARRAY(_field, _ptr, _class, _count, ...)                          \
    {                                                                                 \
        _ptr = (_class *)AllocationMetrics::_field->Allocate(sizeof(_class[_count])); \
        new (_ptr) _class(__VA_ARGS__);                                               \
        AllocationMetrics::_field##_Count++;                                          \
        ADDTODEBUGMAP(_class);                                                        \
    }

#define METADOT_CREATE(_field, _ptr, _class, ...) \
                                                  \
    _class *_ptr = nullptr;                       \
    METADOT_NEW(_field, _ptr, _class, __VA_ARGS__)

#define METADOT_DELETE_RAW(_field, _ptr, _class_name, _class) \
    {                                                         \
        _ptr->~_class_name();                                 \
        AllocationMetrics::_field->Free(_ptr);                \
        AllocationMetrics::_field##_Count--;                  \
        REMOVEDEBUGMAP(_class);                               \
    }

#define METADOT_DELETE(_field, _ptr, _class_name) \
    { METADOT_DELETE_RAW(_field, _ptr, _class_name, _class_name); }

#define METADOT_DELETE_EX(_field, _ptr, _class_name, _class) \
    { METADOT_DELETE_RAW(_field, _ptr, _class_name, _class); }

#define GCField_R(_c, _n) \
    static _c *_n;        \
    static std::atomic<int> _n##_Count

#define GCField_S(_c, _n)                \
    _c *AllocationMetrics::_n = nullptr; \
    std::atomic<int> AllocationMetrics::_n##_Count = 0

typedef struct AllocationMetrics {
    U64 TotalAllocated;
    U64 TotalFree;

#if defined(METADOT_DEBUG)
    static std::map<std::string_view, std::size_t> MemoryDebugMap;
#endif

    GCField_R(CAllocator, C);
    GCField_R(LinearAllocator, S);
} AllocationMetrics;

extern AllocationMetrics g_AllocationMetrics;

U64 MemCurrentUsageBytes();
F32 MemCurrentUsageMB();

void METAENGINE_Memory_Init(int argc, char *argv[]);
void METAENGINE_Memory_End();
void METAENGINE_Memory_RunGC();

#if defined(METADOT_LEAK_TEST)

void *operator new(std::size_t sz);
void operator delete(void *ptr) noexcept;

void getInfo();

#endif

#endif

#endif