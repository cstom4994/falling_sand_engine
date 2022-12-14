// Copyright(c) 2022, KaoruXun All rights reserved.

// Now I'm using gc(https://github.com/mkirchner/gc)
// and allocator(https://github.com/mtrebi/memory-allocators)
// which are both licensed under the MIT License
// and also the memory allocator of SDL

#ifndef _METADOT_GCMANAGER_HPP_
#define _METADOT_GCMANAGER_HPP_

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>

#include "core/core.hpp"
#include "core/GC.h"
#include "core/macros.h"
#include "libs/nameof.hpp"

class AllocatorUtils {
public:
    static const std::size_t CalculatePadding(const std::size_t baseAddress,
                                              const std::size_t alignment) {
        const std::size_t multiplier = (baseAddress / alignment) + 1;
        const std::size_t alignedAddress = multiplier * alignment;
        const std::size_t padding = alignedAddress - baseAddress;
        return padding;
    }

    static const std::size_t CalculatePaddingWithHeader(const std::size_t baseAddress,
                                                        const std::size_t alignment,
                                                        const std::size_t headerSize) {
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

template<class T>
class DoublyLinkedList {
public:
    struct Node
    {
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

template<class T>
DoublyLinkedList<T>::DoublyLinkedList() {}

template<class T>
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
            if (newNode->next != nullptr) { newNode->next->previous = newNode; }
            previousNode->next = newNode;
            newNode->previous = previousNode;
        }
    }
}

template<class T>
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

    struct AllocationHeader
    {
        std::size_t padding;
    };
};

template<class T>
class StackLinkedList {
public:
    struct Node
    {
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

template<class T>
void StackLinkedList<T>::push(Node *newNode) {
    newNode->next = head;
    head = newNode;
}

template<class T>
typename StackLinkedList<T>::Node *StackLinkedList<T>::pop() {
    Node *top = head;
    head = head->next;
    return top;
}

class PoolAllocator : public Allocator {
private:
    struct FreeHeader
    {
    };
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

template<class T>
class SinglyLinkedList {
public:
    struct Node
    {
        T data;
        Node *next;
    };

    Node *head;

public:
    SinglyLinkedList();

    void insert(Node *previousNode, Node *newNode);
    void remove(Node *previousNode, Node *deleteNode);
};

template<class T>
SinglyLinkedList<T>::SinglyLinkedList() {}

template<class T>
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

template<class T>
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
    enum PlacementPolicy {
        FIND_FIRST,
        FIND_BEST
    };

private:
    struct FreeHeader
    {
        std::size_t blockSize;
    };
    struct AllocationHeader
    {
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

    void Find(const std::size_t size, const std::size_t alignment, std::size_t &padding,
              Node *&previousNode, Node *&foundNode);
    void FindBest(const std::size_t size, const std::size_t alignment, std::size_t &padding,
                  Node *&previousNode, Node *&foundNode);
    void FindFirst(const std::size_t size, const std::size_t alignment, std::size_t &padding,
                   Node *&previousNode, Node *&foundNode);
};

#define METADOT_GC_ALLOC(size) malloc(size)
#define METADOT_GC_ALLOC_ALIGNED(size, alignment) malloc(size)
#define METADOT_GC_DEALLOC(ptr) free(ptr)
#define METADOT_GC_DEALLOC_ALIGNED(ptr) free(ptr)
#define METADOT_GC_REALLOC(ptr, size) realloc(ptr, size)
#define METADOT_GC_REALLOC_ALIGNED(ptr, size, alignment) realloc(ptr, size, alignment)

#if defined(METADOT_DEBUG)
#define ADDTODEBUGMAP(_c) GC::MemoryDebugMap.insert(std::make_pair(NAMEOF_TYPE(_c), sizeof(_c)))
#define REMOVEDEBUGMAP(_c) GC::MemoryDebugMap.erase(NAMEOF_TYPE(_c))
#else
#define ADDTODEBUGMAP(_c)
#define REMOVEDEBUGMAP(_c)
#endif

#define METADOT_NEW(_field, _ptr, _class, ...)                                                     \
    {                                                                                              \
        _ptr = (_class *) GC::_field->Allocate(sizeof(_class));                                    \
        new (_ptr) _class(__VA_ARGS__);                                                            \
        GC::_field##_Count++;                                                                      \
        ADDTODEBUGMAP(_class);                                                                     \
    }

#define METADOT_NEW_ARRAY(_field, _ptr, _class, _count, ...)                                       \
    {                                                                                              \
        _ptr = (_class *) GC::_field->Allocate(sizeof(_class[_count]));                            \
        new (_ptr) _class(__VA_ARGS__);                                                            \
        GC::_field##_Count++;                                                                      \
        ADDTODEBUGMAP(_class);                                                                     \
    }

#define METADOT_CREATE(_field, _ptr, _class, ...)                                                  \
                                                                                                   \
    _class *_ptr = nullptr;                                                                        \
    METADOT_NEW(_field, _ptr, _class, __VA_ARGS__)

#define METADOT_DELETE_RAW(_field, _ptr, _class_name, _class)                                      \
    {                                                                                              \
        _ptr->~_class_name();                                                                      \
        GC::_field->Free(_ptr);                                                                    \
        GC::_field##_Count--;                                                                      \
        REMOVEDEBUGMAP(_class);                                                                    \
    }

#define METADOT_DELETE(_field, _ptr, _class_name)                                                  \
    { METADOT_DELETE_RAW(_field, _ptr, _class_name, _class_name); }

#define METADOT_DELETE_EX(_field, _ptr, _class_name, _class)                                       \
    { METADOT_DELETE_RAW(_field, _ptr, _class_name, _class); }

#define GCField_R(_c, _n)                                                                          \
    static _c *_n;                                                                                 \
    static std::atomic<int> _n##_Count

#define GCField_S(_c, _n)                                                                          \
    _c *GC::_n = nullptr;                                                                          \
    std::atomic<int> GC::_n##_Count = 0

struct GC
{
#if defined(METADOT_DEBUG)
    static std::map<std::string_view, std::size_t> MemoryDebugMap;
#endif

    GCField_R(CAllocator, C);
    GCField_R(LinearAllocator, S);
};

void METAENGINE_Memory_Init(int argc, char *argv[]);
void METAENGINE_Memory_End();
void METAENGINE_Memory_RunGC();

#if defined(METADOT_LEAK_TEST)

void *operator new(std::size_t sz);
void operator delete(void *ptr) noexcept;

void getInfo();

#endif

#endif// _METADOT_GCMANAGER_HPP_
