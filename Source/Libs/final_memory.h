/***
final_memory.h

-------------------------------------------------------------------------------
	About
-------------------------------------------------------------------------------

A open source single header file library for using heap memory like a stack.

The only dependencies are a C99 complaint compiler.

-------------------------------------------------------------------------------
	Getting started
-------------------------------------------------------------------------------

- Drop this file into your main C/C++ project and include it in one place you want.
- Define FMEM_IMPLEMENTATION before including this header file in your main translation unit.

-------------------------------------------------------------------------------
	Usage growable memory (Default case)
-------------------------------------------------------------------------------

#define FMEM_IMPLEMENTATION
#include <final_mem.h>

fmemMemoryBlock myMem;
if (fmemInit(&myMem, fmemFlags_Growable, FMEM_MEGABYTES(16))) {
	// Uses the first memory block
	uint8_t *smallData = fmemPushSize(&myMem, FMEM_MEGABYTES(3), fmemFlags_None);

	// Adds another memory block
	uint8_t *bigData = fmemPushSize(&myMem, FMEM_MEGABYTES(64), fmemFlags_None);
	...

	// Uses the first memory block
	uint8_t *anotherBlock = fmemPushSize(&myMem, FMEM_MEGABYTES(5), fmemFlags_None);

	// Does not fit in the first block, therefore uses the second block
	uint8_t *anotherBiggerBlock = fmemPushSize(&myMem, FMEM_MEGABYTES(9), fmemFlags_None);

	// Releases all memory blocks
	fmemRelease(&myMem);
}

// ........................

fmemMemoryBlock myMem2;
if (fmemInit(&myMem2, fmemFlags_Growable, 0)) {
	// Allocates the first block, because it was not allocated beforehand
	uint8_t *data = fmemPushSize(&myMem2, FMEM_MEGABYTES(3), fmemFlags_None);

	// ...

	// Releases all memory blocks
	fmemRelease(&myMem2);
}

// ........................

// For growable memory, init is not required when it is initialized to zero
fmemMemoryBlock myMem3 = {0};

// Allocates the first block, because it was not allocated initially
uint8_t *data = fmemPushSize(&myMem3, FMEM_MEGABYTES(3), fmemFlags_None);

// ...

// Releases all memory blocks
fmemRelease(&myMem3);

-------------------------------------------------------------------------------
	Usage fixed/static memory
-------------------------------------------------------------------------------

#define FMEM_IMPLEMENTATION
#include <final_mem.h>

fmemMemoryBlock myMem;
if (fmemInit(&myMem, fmemFlags_Fixed, FMEM_MEGABYTES(16))) {
	uint32_t *data = (uint32_t *)fmemPushSize(&myMem, sizeof(uint32_t) * 10, fmemFlags_None);
	data[0] = 1;
	data[1] = 2;

	// Returns null, size does not fit in stack block
	uint8_t *bigData = fmemPushSize(&myMem, FMEM_MEGABYTES(32), fmemFlags_None);

	...

	fmemRelease(&myMem);
}

-------------------------------------------------------------------------------
	Usage temporary memory
-------------------------------------------------------------------------------

#define FMEM_IMPLEMENTATION
#include <final_mem.h>

fmemMemoryBlock myMem;
if (fmemInit(&myMem, fmemFlags_Growable, FMEM_MEGABYTES(16))) {
	uint8_t *data = fmemPushSize(&myMem, FMEM_MEGABYTES(4), fmemFlags_None);
	data[0] = 1;
	data[1] = 2;

	// Use remaining size of the source memory block
	// Source memory block is locked now and cannot be used until the temporary memory is released
	fmemMemoryBlock tempMem;
	if (fmemBeginTemporary(&myMem, &tempMem)) {
		fmemEndTemporary(&tempMem);
	}

	// Source memory is restored and unlocked
	uint8_t *moreData = fmemPushSize(&myMem, FMEM_MEGABYTES(2), fmemFlags_None);
	moreData[0] = 128;
	moreData[1] = 223;

	fmemRelease(&myMem);
}

-------------------------------------------------------------------------------
	License
-------------------------------------------------------------------------------

Final Memory is released under the following license:

MIT License

Copyright (c) 2017-2021 Torsten Spaete

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
***/

/*!
	\file final_memory.h
	\version v0.3.0 alpha
	\author Torsten Spaete
	\brief Final Memory (FMEM) - A open source C99 single file header memory library.
*/

/*!
	\page page_changelog Changelog
	\tableofcontents

	## v0.3.0 alpha:
	- New: Added macro fmemPushStruct()
	- New: Added function fmemCreate()

	## v0.2.1 alpha:
	- Fixed: Two inline functions was not found in GCC

	## v0.2 alpha:
	- Added: New function fmemGetHeader
	- Added: Safety check for fmemBeginTemporary when passing a temporary block as source
	- Fixed: Memory is no longer wasted
	- Fixed: Fixed crash when fmemGetTotalSize was passed a fixed-size block
	- Fixed: Fixed crash when fmemGetRemainingSize was passed a fixed-size block
	- Changed: fmemType_Fixed does not allocate in sized-block anymore, instead it uses the desired size + meta size

	## v0.1.0 alpha:
	- Initial version
*/

/*!
	\page page_todo Todo
	\tableofcontents

	- Removal of single memory blocks
	- Array allocation and pushing
	- Memory partitions
		- Separated but linked, not able to free linked block
		- Just partitions of fixed size blocks

*/

#ifndef FMEM_H
#define FMEM_H

// Detect compiler
#if !defined(__cplusplus) && ((defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)) || (defined(_MSC_VER) && (_MSC_VER >= 1900)))
#define FMEM_IS_C99
#elif defined(__cplusplus)
#define FMEM_IS_CPP
#else
#error "This C/C++ compiler is not supported!"
#endif

// Api export
#if defined(FMEM_PRIVATE)
#define fmem_api static
#else
#define fmem_api extern
#endif

#if defined(FMEM_IS_C99)
//! Initialize a struct to zero (C99)
#define FMEM_ZERO_INIT \
    { 0 }
#else
//! Initialize a struct to zero (C++)
#define FMEM_ZERO_INIT \
    {}
#endif

// Includes
#include <stdbool.h>// bool
#include <stddef.h> // NULL
#include <stdint.h> // int32_t, etc.

// Functions override
#ifndef FMEM_MEMSET
#include <string.h>
#define FMEM_MEMSET(dst, val, size) memset(dst, val, size)
#endif
#ifndef FMEM_MALLOC
#if defined(__APPLE__)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#define FMEM_MALLOC(size) malloc(size)
#define FMEM_FREE(ptr) free(ptr)
#endif
#if !defined(FMEM_ASSERT) || !defined(FMEM_STATIC_ASSERT)
#include <assert.h>
#define FMEM_ASSERT(exp) assert(exp)
#define FMEM_STATIC_ASSERT(exp) static_assert(exp)
#endif

//! Null pointer
#define fmem_null NULL

//! Returns the number of bytes for the given kilobytes
#define FMEM_KILOBYTES(value) (((value) *1024ull))
//! Returns the number of bytes for the given megabytes
#define FMEM_MEGABYTES(value) ((FMEM_KILOBYTES(value) * 1024ull))
//! Returns the number of bytes for the given gigabytes
#define FMEM_GIGABYTES(value) ((FMEM_MEGABYTES(value) * 1024ull))
//! Returns the number of bytes for the given terabytes
#define FMEM_TERABYTES(value) ((FMEM_GIGABYTES(value) * 1024ull))

typedef enum fmemPushFlags {
    //! No push flags
    fmemPushFlags_None = 0,
    //! Clear region to zero
    fmemPushFlags_Clear = 1 << 0,
} fmemPushFlags;

typedef enum fmemType {
    //! Unlimited size, grows additional blocks if needed
    fmemType_Growable = 0,
    //! Limited tto a fixed size
    fmemType_Fixed,
    //! Temporary memory
    fmemType_Temporary,
} fmemType;

typedef enum fmemSizeFlags {
    //! No size flags
    fmemSizeFlags_None = 0,
    //! Returns the size for a single block only
    fmemSizeFlags_Single = 1 << 0,
    //! Include size with meta-data
    fmemSizeFlags_WithMeta = 1 << 1,
} fmemSizeFlags;

typedef struct fmemBlockHeader
{
    //! Previous block
    struct fmemMemoryBlock *prev;
    //! Next block
    struct fmemMemoryBlock *next;
} fmemBlockHeader;

typedef struct fmemMemoryBlock
{
    //! Source memory pointer if present
    void *source;
    //! Base memory pointer
    void *base;
    //! Pointer to a temporary memory block
    struct fmemMemoryBlock *temporary;
    //! Total size in bytes
    size_t size;
    //! Used size in bytes
    size_t used;
    //! Type
    fmemType type;
} fmemMemoryBlock;

//! Creates a memory block and allocates memory when size is greater than zero
fmem_api fmemMemoryBlock fmemCreate(const fmemType type, const size_t size);
//! Initializes the given block or allocates memory when size is greater than zero
fmem_api bool fmemInit(fmemMemoryBlock *block, const fmemType type, const size_t size);
//! Initializes the given block to a fixed size block from existing source memory
fmem_api bool fmemInitFromSource(fmemMemoryBlock *block, void *sourceMemory, const size_t sourceSize);
//! Release this and all appended memory blocks
fmem_api void fmemFree(fmemMemoryBlock *block);
//! Gets memory from the block by the given size
fmem_api uint8_t *fmemPush(fmemMemoryBlock *block, const size_t size, const fmemPushFlags flags);
//! Gets memory from the block by the given size and ensure address alignment
fmem_api uint8_t *fmemPushAligned(fmemMemoryBlock *block, const size_t size, const size_t alignment, const fmemPushFlags flags);
//! Gets memory for a new block with the given size
fmem_api bool fmemPushBlock(fmemMemoryBlock *src, fmemMemoryBlock *dst, const size_t size, const fmemPushFlags flags);
//! Returns the remaining size of all blocks starting by the given block
fmem_api size_t fmemGetRemainingSize(fmemMemoryBlock *block);
//! Returns the total size of all blocks starting by the given block
fmem_api size_t fmemGetTotalSize(fmemMemoryBlock *block);
//! Resets the given block usage to zero without freeing any memory
fmem_api void fmemReset(fmemMemoryBlock *block);
//! Initializes a temporary block with the remaining size of the source block
fmem_api bool fmemBeginTemporary(fmemMemoryBlock *source, fmemMemoryBlock *temporary);
//! Gives the memory back to source block from the temporary block
fmem_api void fmemEndTemporary(fmemMemoryBlock *temporary);
//! Returns the block header pointer for the given block
fmem_api fmemBlockHeader *fmemGetHeader(fmemMemoryBlock *block);

//! Gets memory for a struct from the block and return a pointer to the struct
#define fmemPushStruct(block, type, flags) (type *) fmemPush(block, sizeof(type), flags)

#endif// FMEM_H
