
// Hack from https://github.com/mtrebi/memory-allocators

#include "Allocator.h"

#include <cassert>//assert

#include <algorithm>
#include <cassert>
#include <limits>

#ifdef _DEBUG
#include <iostream>
#endif

CAllocator::CAllocator()
    : Allocator(0) {
}

void CAllocator::Init() {
}

CAllocator::~CAllocator() {
}

void *CAllocator::Allocate(const std::size_t size, const std::size_t alignment) {
    return gc_malloc(&gc, size);
}

void CAllocator::Free(void *ptr) {
    gc_free(&gc, ptr);
}


FreeListAllocator::FreeListAllocator(const std::size_t totalSize, const PlacementPolicy pPolicy)
    : Allocator(totalSize) {
    m_pPolicy = pPolicy;
}

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
    Node *affectedNode,
            *previousNode;
    this->Find(size, alignment, padding, previousNode, affectedNode);
    assert(affectedNode != nullptr && "Not enough memory");


    const std::size_t alignmentPadding = padding - allocationHeaderSize;
    const std::size_t requiredSize = size + padding;

    const std::size_t rest = affectedNode->data.blockSize - requiredSize;

    if (rest > 0) {
        // We have to split the block into the data block and a free block of size 'rest'
        Node *newFreeNode = (Node *) ((std::size_t) affectedNode + requiredSize);
        newFreeNode->data.blockSize = rest;
        m_freeList.insert(affectedNode, newFreeNode);
    }
    m_freeList.remove(previousNode, affectedNode);

    // Setup data block
    const std::size_t headerAddress = (std::size_t) affectedNode + alignmentPadding;
    const std::size_t dataAddress = headerAddress + allocationHeaderSize;
    ((FreeListAllocator::AllocationHeader *) headerAddress)->blockSize = requiredSize;
    ((FreeListAllocator::AllocationHeader *) headerAddress)->padding = alignmentPadding;

    m_used += requiredSize;
    m_peak = std::max(m_peak, m_used);

#ifdef _DEBUG
    std::cout << "A"
              << "\t@H " << (void *) headerAddress << "\tD@ " << (void *) dataAddress << "\tS " << ((FreeListAllocator::AllocationHeader *) headerAddress)->blockSize << "\tAP " << alignmentPadding << "\tP " << padding << "\tM " << m_used << "\tR " << rest << std::endl;
#endif

    return (void *) dataAddress;
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
    //Iterate list and return the first free block with a size >= than given size
    Node *it = m_freeList.head,
         *itPrev = nullptr;

    while (it != nullptr) {
        padding = Utils::CalculatePaddingWithHeader((std::size_t) it, alignment, sizeof(FreeListAllocator::AllocationHeader));
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
    Node *it = m_freeList.head,
         *itPrev = nullptr;
    while (it != nullptr) {
        padding = Utils::CalculatePaddingWithHeader((std::size_t) it, alignment, sizeof(FreeListAllocator::AllocationHeader));
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
    const std::size_t currentAddress = (std::size_t) ptr;
    const std::size_t headerAddress = currentAddress - sizeof(FreeListAllocator::AllocationHeader);
    const FreeListAllocator::AllocationHeader *allocationHeader{(FreeListAllocator::AllocationHeader *) headerAddress};

    Node *freeNode = (Node *) (headerAddress);
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
              << "\t@ptr " << ptr << "\tH@ " << (void *) freeNode << "\tS " << freeNode->data.blockSize << "\tM " << m_used << std::endl;
#endif
}

void FreeListAllocator::Coalescence(Node *previousNode, Node *freeNode) {
    if (freeNode->next != nullptr &&
        (std::size_t) freeNode + freeNode->data.blockSize == (std::size_t) freeNode->next) {
        freeNode->data.blockSize += freeNode->next->data.blockSize;
        m_freeList.remove(freeNode, freeNode->next);
#ifdef _DEBUG
        std::cout << "\tMerging(n) " << (void *) freeNode << " & " << (void *) freeNode->next << "\tS " << freeNode->data.blockSize << std::endl;
#endif
    }

    if (previousNode != nullptr &&
        (std::size_t) previousNode + previousNode->data.blockSize == (std::size_t) freeNode) {
        previousNode->data.blockSize += freeNode->data.blockSize;
        m_freeList.remove(previousNode, freeNode);
#ifdef _DEBUG
        std::cout << "\tMerging(p) " << (void *) previousNode << " & " << (void *) freeNode << "\tS " << previousNode->data.blockSize << std::endl;
#endif
    }
}

void FreeListAllocator::Reset() {
    m_used = 0;
    m_peak = 0;
    Node *firstNode = (Node *) m_start_ptr;
    firstNode->data.blockSize = m_totalSize;
    firstNode->next = nullptr;
    m_freeList.head = nullptr;
    m_freeList.insert(nullptr, firstNode);
}


LinearAllocator::LinearAllocator(const std::size_t totalSize)
    : Allocator(totalSize) {
}

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
    const std::size_t currentAddress = (std::size_t) m_start_ptr + m_offset;

    if (alignment != 0 && m_offset % alignment != 0) {
        // Alignment is required. Find the next aligned memory address and update offset
        padding = Utils::CalculatePadding(currentAddress, alignment);
    }

    if (m_offset + padding + size > m_totalSize) {
        return nullptr;
    }

    m_offset += padding;
    const std::size_t nextAddress = currentAddress + padding;
    m_offset += size;

#ifdef _DEBUG
    std::cout << "A"
              << "\t@C " << (void *) currentAddress << "\t@R " << (void *) nextAddress << "\tO " << m_offset << "\tP " << padding << std::endl;
#endif

    m_used = m_offset;
    m_peak = std::max(m_peak, m_used);

    return (void *) nextAddress;
}

void LinearAllocator::Free(void *ptr) {
    assert(false && "Use Reset() method");
}

void LinearAllocator::Reset() {
    m_offset = 0;
    m_used = 0;
    m_peak = 0;
}


PoolAllocator::PoolAllocator(const std::size_t totalSize, const std::size_t chunkSize)
    : Allocator(totalSize) {
    assert(chunkSize >= 8 && "Chunk size must be greater or equal to 8");
    assert(totalSize % chunkSize == 0 && "Total Size must be a multiple of Chunk Size");
    this->m_chunkSize = chunkSize;
}

void PoolAllocator::Init() {
    m_start_ptr = gc_malloc(&gc, m_totalSize);
    this->Reset();
}

PoolAllocator::~PoolAllocator() {
    gc_free(&gc, m_start_ptr);
}

void *PoolAllocator::Allocate(const std::size_t allocationSize, const std::size_t alignment) {
    assert(allocationSize == this->m_chunkSize && "Allocation size must be equal to chunk size");

    Node *freePosition = m_freeList.pop();

    assert(freePosition != nullptr && "The pool allocator is full");

    m_used += m_chunkSize;
    m_peak = std::max(m_peak, m_used);
#ifdef _DEBUG
    std::cout << "A"
              << "\t@S " << m_start_ptr << "\t@R " << (void *) freePosition << "\tM " << m_used << std::endl;
#endif

    return (void *) freePosition;
}

void PoolAllocator::Free(void *ptr) {
    m_used -= m_chunkSize;

    m_freeList.push((Node *) ptr);

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
        std::size_t address = (std::size_t) m_start_ptr + i * m_chunkSize;
        m_freeList.push((Node *) address);
    }
}


StackAllocator::StackAllocator(const std::size_t totalSize)
    : Allocator(totalSize) {
}

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
    const std::size_t currentAddress = (std::size_t) m_start_ptr + m_offset;

    std::size_t padding = Utils::CalculatePaddingWithHeader(currentAddress, alignment, sizeof(AllocationHeader));

    if (m_offset + padding + size > m_totalSize) {
        return nullptr;
    }
    m_offset += padding;

    const std::size_t nextAddress = currentAddress + padding;
    const std::size_t headerAddress = nextAddress - sizeof(AllocationHeader);
    AllocationHeader allocationHeader{padding};
    AllocationHeader *headerPtr = (AllocationHeader *) headerAddress;
    headerPtr = &allocationHeader;

    m_offset += size;

#ifdef _DEBUG
    std::cout << "A"
              << "\t@C " << (void *) currentAddress << "\t@R " << (void *) nextAddress << "\tO " << m_offset << "\tP " << padding << std::endl;
#endif
    m_used = m_offset;
    m_peak = std::max(m_peak, m_used);

    return (void *) nextAddress;
}

void StackAllocator::Free(void *ptr) {
    // Move offset back to clear address
    const std::size_t currentAddress = (std::size_t) ptr;
    const std::size_t headerAddress = currentAddress - sizeof(AllocationHeader);
    const AllocationHeader *allocationHeader{(AllocationHeader *) headerAddress};

    m_offset = currentAddress - allocationHeader->padding - (std::size_t) m_start_ptr;
    m_used = m_offset;

#ifdef _DEBUG
    std::cout << "F"
              << "\t@C " << (void *) currentAddress << "\t@F " << (void *) ((char *) m_start_ptr + m_offset) << "\tO " << m_offset << std::endl;
#endif
}

void StackAllocator::Reset() {
    m_offset = 0;
    m_used = 0;
    m_peak = 0;
}