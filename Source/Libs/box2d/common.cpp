// Metadot physics engine is enhanced based on box2d modification
// Metadot code copyright(c) 2022, KaoruXun All rights reserved.
// Box2d code by Erin Catto licensed under the MIT License
// https://github.com/erincatto/box2d

/*
MIT License
Copyright (c) 2019 Erin Catto

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include "box2d/b2_block_allocator.h"
#include "box2d/b2_draw.h"
#include "box2d/b2_math.h"
#include "box2d/b2_settings.h"
#include "box2d/b2_stack_allocator.h"
#include "box2d/b2_timer.h"
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int32 b2_chunkSize = 16 * 1024;
static const int32 b2_maxBlockSize = 640;
static const int32 b2_chunkArrayIncrement = 128;

// These are the supported object sizes. Actual allocations are rounded up the next size.
static const int32 b2_blockSizes[b2_blockSizeCount] =
        {
                16, // 0
                32, // 1
                64, // 2
                96, // 3
                128,// 4
                160,// 5
                192,// 6
                224,// 7
                256,// 8
                320,// 9
                384,// 10
                448,// 11
                512,// 12
                640,// 13
};

// This maps an arbitrary allocation size to a suitable slot in b2_blockSizes.
struct b2SizeMap
{
    b2SizeMap() {
        int32 j = 0;
        values[0] = 0;
        for (int32 i = 1; i <= b2_maxBlockSize; ++i) {
            b2Assert(j < b2_blockSizeCount);
            if (i <= b2_blockSizes[j]) {
                values[i] = (uint8) j;
            } else {
                ++j;
                values[i] = (uint8) j;
            }
        }
    }

    uint8 values[b2_maxBlockSize + 1];
};

static const b2SizeMap b2_sizeMap;

struct b2Chunk
{
    int32 blockSize;
    b2Block *blocks;
};

struct b2Block
{
    b2Block *next;
};

b2BlockAllocator::b2BlockAllocator() {
    b2Assert(b2_blockSizeCount < UCHAR_MAX);

    m_chunkSpace = b2_chunkArrayIncrement;
    m_chunkCount = 0;
    m_chunks = (b2Chunk *) b2Alloc(m_chunkSpace * sizeof(b2Chunk));

    memset(m_chunks, 0, m_chunkSpace * sizeof(b2Chunk));
    memset(m_freeLists, 0, sizeof(m_freeLists));
}

b2BlockAllocator::~b2BlockAllocator() {
    for (int32 i = 0; i < m_chunkCount; ++i) {
        b2Free(m_chunks[i].blocks);
    }

    b2Free(m_chunks);
}

void *b2BlockAllocator::Allocate(int32 size) {
    if (size == 0) {
        return nullptr;
    }

    b2Assert(0 < size);

    if (size > b2_maxBlockSize) {
        return b2Alloc(size);
    }

    int32 index = b2_sizeMap.values[size];
    b2Assert(0 <= index && index < b2_blockSizeCount);

    if (m_freeLists[index]) {
        b2Block *block = m_freeLists[index];
        m_freeLists[index] = block->next;
        return block;
    } else {
        if (m_chunkCount == m_chunkSpace) {
            b2Chunk *oldChunks = m_chunks;
            m_chunkSpace += b2_chunkArrayIncrement;
            m_chunks = (b2Chunk *) b2Alloc(m_chunkSpace * sizeof(b2Chunk));
            memcpy(m_chunks, oldChunks, m_chunkCount * sizeof(b2Chunk));
            memset(m_chunks + m_chunkCount, 0, b2_chunkArrayIncrement * sizeof(b2Chunk));
            b2Free(oldChunks);
        }

        b2Chunk *chunk = m_chunks + m_chunkCount;
        chunk->blocks = (b2Block *) b2Alloc(b2_chunkSize);
#if defined(_DEBUG)
        memset(chunk->blocks, 0xcd, b2_chunkSize);
#endif
        int32 blockSize = b2_blockSizes[index];
        chunk->blockSize = blockSize;
        int32 blockCount = b2_chunkSize / blockSize;
        b2Assert(blockCount * blockSize <= b2_chunkSize);
        for (int32 i = 0; i < blockCount - 1; ++i) {
            b2Block *block = (b2Block *) ((int8 *) chunk->blocks + blockSize * i);
            b2Block *next = (b2Block *) ((int8 *) chunk->blocks + blockSize * (i + 1));
            block->next = next;
        }
        b2Block *last = (b2Block *) ((int8 *) chunk->blocks + blockSize * (blockCount - 1));
        last->next = nullptr;

        m_freeLists[index] = chunk->blocks->next;
        ++m_chunkCount;

        return chunk->blocks;
    }
}

void b2BlockAllocator::Free(void *p, int32 size) {
    if (size == 0) {
        return;
    }

    b2Assert(0 < size);

    if (size > b2_maxBlockSize) {
        b2Free(p);
        return;
    }

    int32 index = b2_sizeMap.values[size];
    b2Assert(0 <= index && index < b2_blockSizeCount);

#if defined(_DEBUG)
    // Verify the memory address and size is valid.
    int32 blockSize = b2_blockSizes[index];
    bool found = false;
    for (int32 i = 0; i < m_chunkCount; ++i) {
        b2Chunk *chunk = m_chunks + i;
        if (chunk->blockSize != blockSize) {
            b2Assert((int8 *) p + blockSize <= (int8 *) chunk->blocks ||
                     (int8 *) chunk->blocks + b2_chunkSize <= (int8 *) p);
        } else {
            if ((int8 *) chunk->blocks <= (int8 *) p && (int8 *) p + blockSize <= (int8 *) chunk->blocks + b2_chunkSize) {
                found = true;
            }
        }
    }

    b2Assert(found);

    memset(p, 0xfd, blockSize);
#endif

    b2Block *block = (b2Block *) p;
    block->next = m_freeLists[index];
    m_freeLists[index] = block;
}

void b2BlockAllocator::Clear() {
    for (int32 i = 0; i < m_chunkCount; ++i) {
        b2Free(m_chunks[i].blocks);
    }

    m_chunkCount = 0;
    memset(m_chunks, 0, m_chunkSpace * sizeof(b2Chunk));
    memset(m_freeLists, 0, sizeof(m_freeLists));
}


b2Draw::b2Draw() {
    m_drawFlags = 0;
}

void b2Draw::SetFlags(uint32 flags) {
    m_drawFlags = flags;
}

uint32 b2Draw::GetFlags() const {
    return m_drawFlags;
}

void b2Draw::AppendFlags(uint32 flags) {
    m_drawFlags |= flags;
}

void b2Draw::ClearFlags(uint32 flags) {
    m_drawFlags &= ~flags;
}


const b2Vec2 b2Vec2_zero(0.0f, 0.0f);

/// Solve A * x = b, where b is a column vector. This is more efficient
/// than computing the inverse in one-shot cases.
b2Vec3 b2Mat33::Solve33(const b2Vec3 &b) const {
    float det = b2Dot(ex, b2Cross(ey, ez));
    if (det != 0.0f) {
        det = 1.0f / det;
    }
    b2Vec3 x;
    x.x = det * b2Dot(b, b2Cross(ey, ez));
    x.y = det * b2Dot(ex, b2Cross(b, ez));
    x.z = det * b2Dot(ex, b2Cross(ey, b));
    return x;
}

/// Solve A * x = b, where b is a column vector. This is more efficient
/// than computing the inverse in one-shot cases.
b2Vec2 b2Mat33::Solve22(const b2Vec2 &b) const {
    float a11 = ex.x, a12 = ey.x, a21 = ex.y, a22 = ey.y;
    float det = a11 * a22 - a12 * a21;
    if (det != 0.0f) {
        det = 1.0f / det;
    }
    b2Vec2 x;
    x.x = det * (a22 * b.x - a12 * b.y);
    x.y = det * (a11 * b.y - a21 * b.x);
    return x;
}

///
void b2Mat33::GetInverse22(b2Mat33 *M) const {
    float a = ex.x, b = ey.x, c = ex.y, d = ey.y;
    float det = a * d - b * c;
    if (det != 0.0f) {
        det = 1.0f / det;
    }

    M->ex.x = det * d;
    M->ey.x = -det * b;
    M->ex.z = 0.0f;
    M->ex.y = -det * c;
    M->ey.y = det * a;
    M->ey.z = 0.0f;
    M->ez.x = 0.0f;
    M->ez.y = 0.0f;
    M->ez.z = 0.0f;
}

/// Returns the zero matrix if singular.
void b2Mat33::GetSymInverse33(b2Mat33 *M) const {
    float det = b2Dot(ex, b2Cross(ey, ez));
    if (det != 0.0f) {
        det = 1.0f / det;
    }

    float a11 = ex.x, a12 = ey.x, a13 = ez.x;
    float a22 = ey.y, a23 = ez.y;
    float a33 = ez.z;

    M->ex.x = det * (a22 * a33 - a23 * a23);
    M->ex.y = det * (a13 * a23 - a12 * a33);
    M->ex.z = det * (a12 * a23 - a13 * a22);

    M->ey.x = M->ex.y;
    M->ey.y = det * (a11 * a33 - a13 * a13);
    M->ey.z = det * (a13 * a12 - a11 * a23);

    M->ez.x = M->ex.z;
    M->ez.y = M->ey.z;
    M->ez.z = det * (a11 * a22 - a12 * a12);
}


b2Version b2_version = {2, 4, 1};

// Memory allocators. Modify these to use your own allocator.
void *b2Alloc_Default(int32 size) {
    return malloc(size);
}

void b2Free_Default(void *mem) {
    free(mem);
}

// You can modify this to use your logging facility.
void b2Log_Default(const char *string, va_list args) {
    vprintf(string, args);
}

FILE *b2_dumpFile = nullptr;

void b2OpenDump(const char *fileName) {
    b2Assert(b2_dumpFile == nullptr);
    b2_dumpFile = fopen(fileName, "w");
}

void b2Dump(const char *string, ...) {
    if (b2_dumpFile == nullptr) {
        return;
    }

    va_list args;
    va_start(args, string);
    vfprintf(b2_dumpFile, string, args);
    va_end(args);
}

void b2CloseDump() {
    fclose(b2_dumpFile);
    b2_dumpFile = nullptr;
}


b2StackAllocator::b2StackAllocator() {
    m_index = 0;
    m_allocation = 0;
    m_maxAllocation = 0;
    m_entryCount = 0;
}

b2StackAllocator::~b2StackAllocator() {
    b2Assert(m_index == 0);
    b2Assert(m_entryCount == 0);
}

void *b2StackAllocator::Allocate(int32 size) {
    b2Assert(m_entryCount < b2_maxStackEntries);

    b2StackEntry *entry = m_entries + m_entryCount;
    entry->size = size;
    if (m_index + size > b2_stackSize) {
        entry->data = (char *) b2Alloc(size);
        entry->usedMalloc = true;
    } else {
        entry->data = m_data + m_index;
        entry->usedMalloc = false;
        m_index += size;
    }

    m_allocation += size;
    m_maxAllocation = b2Max(m_maxAllocation, m_allocation);
    ++m_entryCount;

    return entry->data;
}

void b2StackAllocator::Free(void *p) {
    b2Assert(m_entryCount > 0);
    b2StackEntry *entry = m_entries + m_entryCount - 1;
    b2Assert(p == entry->data);
    if (entry->usedMalloc) {
        b2Free(p);
    } else {
        m_index -= entry->size;
    }
    m_allocation -= entry->size;
    --m_entryCount;

    p = nullptr;
}

int32 b2StackAllocator::GetMaxAllocation() const {
    return m_maxAllocation;
}


#if defined(_WIN32)

double b2Timer::s_invFrequency = 0.0;

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

b2Timer::b2Timer() {
    LARGE_INTEGER largeInteger;

    if (s_invFrequency == 0.0) {
        QueryPerformanceFrequency(&largeInteger);
        s_invFrequency = double(largeInteger.QuadPart);
        if (s_invFrequency > 0.0) {
            s_invFrequency = 1000.0 / s_invFrequency;
        }
    }

    QueryPerformanceCounter(&largeInteger);
    m_start = double(largeInteger.QuadPart);
}

void b2Timer::Reset() {
    LARGE_INTEGER largeInteger;
    QueryPerformanceCounter(&largeInteger);
    m_start = double(largeInteger.QuadPart);
}

float b2Timer::GetMilliseconds() const {
    LARGE_INTEGER largeInteger;
    QueryPerformanceCounter(&largeInteger);
    double count = double(largeInteger.QuadPart);
    float ms = float(s_invFrequency * (count - m_start));
    return ms;
}

#elif defined(__linux__) || defined(__APPLE__)

#include <sys/time.h>

b2Timer::b2Timer() {
    Reset();
}

void b2Timer::Reset() {
    timeval t;
    gettimeofday(&t, 0);
    m_start_sec = t.tv_sec;
    m_start_usec = t.tv_usec;
}

float b2Timer::GetMilliseconds() const {
    timeval t;
    gettimeofday(&t, 0);
    time_t start_sec = m_start_sec;
    suseconds_t start_usec = m_start_usec;

    // http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
    if (t.tv_usec < start_usec) {
        int nsec = (start_usec - t.tv_usec) / 1000000 + 1;
        start_usec -= 1000000 * nsec;
        start_sec += nsec;
    }

    if (t.tv_usec - start_usec > 1000000) {
        int nsec = (t.tv_usec - start_usec) / 1000000;
        start_usec += 1000000 * nsec;
        start_sec -= nsec;
    }
    return 1000.0f * (t.tv_sec - start_sec) + 0.001f * (t.tv_usec - start_usec);
}

#else

b2Timer::b2Timer() {
}

void b2Timer::Reset() {
}

float b2Timer::GetMilliseconds() const {
    return 0.0f;
}

#endif
